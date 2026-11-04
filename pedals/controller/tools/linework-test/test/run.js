/* Checks lw.js (the test page's protocol code) against the real Switchyard core, through the
 * simulated loop in pedals/controller/firmware/test/bridge.c.
 *   make -C ../../firmware/test bridge     (once)
 *   node test/run.js                         (from tools/linework-test) */
'use strict';
const { spawn } = require('child_process');
const path = require('path');
const LW = require('../lw.js');

const exe = path.join(__dirname, '../../../firmware/test', process.platform === 'win32' ? 'bridge.exe' : 'bridge');
const br = spawn(exe, [], { stdio: ['pipe', 'pipe', 'inherit'] });

let buf = '', client, display = '', other = [], waiting = [];
br.stdout.on('data', d => {
  buf += d;
  let i;
  while ((i = buf.indexOf('\n')) >= 0) {
    const line = buf.slice(0, i).trim();
    buf = buf.slice(i + 1);
    if (line.startsWith('A ')) client && client.receive(LW.parseHex(line.slice(2)));
    else if (line.startsWith('O ')) other.push(LW.parseHex(line.slice(2)));
    else if (line.startsWith('D ')) display = line.slice(2);
    else if (line === 'OK') { const w = waiting.shift(); w && w(); }
  }
});
const cmd = s => new Promise(r => { waiting.push(r); br.stdin.write(s + '\n'); });
const run = ms => cmd('R ' + ms);

// The bridge only moves when told to: send each message, then let the loop run a little,
// so answers come back well inside the client's 1 s timeout.
let queue = Promise.resolve();
function send(m) { queue = queue.then(() => cmd('M ' + LW.hex(m))).then(() => run(30)); }

let fails = 0, checks = 0;
function check(ok, what) { checks++; if (!ok) { fails++; console.log('  FAIL: ' + what); } }

async function main() {
  await new Promise(r => waiting.push(r));          // power-up scan done
  client = new LW.Client(send, { timeout: 1000 });
  const log = [];
  client.onMessage = (m, dir) => log.push(dir + ' ' + LW.describe(m));
  let lastPos = null, lastStatus = null;
  client.onPosition = p => { lastPos = p; };
  client.onStatus = s => { lastStatus = s; };

  console.log('1. Info and identity');
  const info = await client.info();
  check(info.firmware.join('.') === '0.1.0' && info.spec === 2 && info.power9v, 'Switchyard info ' + JSON.stringify(info));
  const id = await client.identity();
  check(LW.hex(id.model) === '02 00', 'identity model 02 00');

  console.log('2. Scan loop and pedal list');
  const pedals = await client.scan();
  check(pedals.length === 3 && pedals.map(p => p.position).join() === '1,2,3', 'scan found 3 pedals in order');
  check(pedals.map(p => p.channel).join() === '0,1,2', 'channels 1-3');
  const list = await client.pedalList();
  check(list.length === 3 && LW.serialHex(list[2].serial) === LW.serialHex(pedals[2].serial), 'pedal list matches the scan');

  console.log('3. Presets through the loop');
  await client.storePreset(1, 4, [[20, 11], [21, 22], [22, 33], [80, 127]]);
  const rd = await client.readPreset(1, 4);
  check(JSON.stringify(rd) === '[[20,11],[21,22],[22,33],[80,127]]', 'read preset back ' + JSON.stringify(rd));
  const st = await client.reportSettings(0);
  check(st.length === 4 && st[0][0] === 20, 'report settings');
  await client.storePreset(0, 6, null);              // store the current settings
  try { await client.readPreset(1, 200); check(false, 'slot 200 should fail'); }
  catch (e) { check(true, ''); }

  console.log('4. Setlist from the text format');
  const text = `
    wrap: no
    pedal A = scan 2       # the second pedal on the loop
    pedal B = scan 1
    pedal S = other 4
    1.1  A:5 S:10
    1.2  B:7
    2.1  A:6 B:8 S:11
  `;
  const sl = LW.parseSetlist(text, pedals);
  sl.id = 4242;
  const steps = [];
  const crc = await client.uploadSetlist(sl, s => steps.push(s));
  const si = await client.setlistInfo();
  check(si.id === 4242 && si.crc === crc && si.parts === 3 && si.pedals === 3 && !si.wrap, 'setlist info ' + JSON.stringify(si));

  console.log('5. Jump, stepping and position');
  other = [];
  await client.jump(0);
  await run(30);
  check(lastPos && lastPos.index === 0 && lastPos.song === 1 && lastPos.part === 1, 'position report after jump');
  check(other.length === 1 && LW.hex(other[0]) === 'C3 09', 'other brand got preset 10 on ch 4: ' + other.map(LW.hex));
  check(display === '011  2', 'display shows 01.1 (got "' + display + '")');
  await cmd('S +1'); await run(30);
  check(lastPos.index === 1 && lastPos.why === 0, 'Up to part 2');
  await cmd('S +1'); await cmd('S +1'); await run(30);
  check(lastPos.index === 2 && lastPos.why === 2, 'Up at the end without wrap');
  const pos = await client.position();
  check(pos.index === 2 && pos.why === 6, 'Get position');
  try { await client.jump(9); check(false, 'jump to a missing part should fail'); }
  catch (e) { check(e.status === 2, 'jump to a missing part: ' + e.message); }

  console.log('6. Other brands');
  other = [];
  await client.otherSend('cc', 2, 7, 90);
  check(other.length === 1 && LW.hex(other[0]) === 'B2 07 5A', 'control change out of the other-brands jack');

  console.log('7. Loop break');
  await cmd('C 2');
  await run(3500);
  check(lastStatus && lastStatus.broken && lastStatus.where === 2, 'break reported at pedal 2: ' + JSON.stringify(lastStatus));
  const ls = await client.loopStatus();
  check(ls.broken && /pedal 2/.test(ls.whereText), 'loop status: ' + ls.whereText);
  await cmd('F 2');
  await run(2000);
  check(lastStatus && !lastStatus.broken, 'repair reported');

  console.log('8. Setlist text errors');
  for (const [bad, why] of [['pedal A = scan 9\n1.1 A:1', 'position 9'], ['pedal A = other 3\n1.1 B:1', "B isn't"],
    ['pedal A = other 3\n1.1 A:129', '1-128'], ['pedal A = other 17\n1.1 A:1', '1-16'], ['nonsense', "can't read"]]) {
    try { LW.parseSetlist(bad, pedals); check(false, 'should reject: ' + bad); }
    catch (e) { check(e.message.includes(why), `"${bad}" -> ${e.message}`); }
  }

  check(LW.crc32(Buffer.from('123456789')) === 0xCBF43926, 'CRC-32 check value');
  check(log.some(l => l.startsWith('in Position: part 3, song 2.1')), 'log descriptions');

  console.log(`\n${checks} checks, ${fails} failed`);
  br.stdin.write('Q\n');
  process.exitCode = fails ? 1 : 0;
}

main().catch(e => { console.log('ERROR', e); br.kill(); process.exitCode = 1; });
