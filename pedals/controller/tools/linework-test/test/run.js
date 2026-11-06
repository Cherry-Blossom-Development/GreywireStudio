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

  console.log('9. Firmware update');
  {
    // packing: 64 bytes take 74, and come back the same
    const d = Array.from({ length: 64 }, (_, i) => (i * 37 + 200) & 255);
    const pk = LW.pack7(d);
    check(pk.length === 74 && pk.every(b => b < 128) && JSON.stringify(LW.unpack7(pk, 64)) === JSON.stringify(d), 'pack7 / unpack7');
    // a .uf2 for the firmware alone, and the refusal of one with the loader
    const uf2 = (addr, data) => {
      const b = new Uint8Array(512), v = new DataView(b.buffer);
      v.setUint32(0, 0x0A324655, true); v.setUint32(4, 0x9E5D5157, true); v.setUint32(8, 0x2000, true);
      v.setUint32(12, addr, true); v.setUint32(16, data.length, true); v.setUint32(28, 0xE48BFF56, true);
      b.set(data, 32); v.setUint32(508, 0x0AB16F30, true);
      return b;
    };
    const page = k => new Uint8Array(256).fill(k);
    const file = new Uint8Array([...uf2(0x10008000, page(1)), ...uf2(0x10008100, page(2))]);
    const img = LW.firmwareImage(file, 'x.uf2');
    check(img.length === 512 && img[0] === 1 && img[511] === 2, 'reads a firmware .uf2');
    try { LW.firmwareImage(uf2(0x10000000, page(1)), 'x_full.uf2'); check(false, 'should refuse a .uf2 with the loader'); }
    catch (e) { check(/loader/.test(e.message), 'refuses a .uf2 with the loader: ' + e.message); }

    // send 8 KB to the pedal at position 2 (channel 2) and watch it restart with a new version
    const before = (await client.scan())[1];
    const image = new Uint8Array(8 * 1024 + 10).map((_, i) => (i * 7 + (i >> 8)) & 255);
    const stages = new Set();
    let blocksOut = 0, most = 0;
    const outs = new Map();
    client.onMessage = (m, dir) => {
      if (LW.isLinework(m) && LW.cmdOf(m) === LW.CMD.FW_DATA && dir === 'out') { blocksOut++; outs.set(LW.qqOf(m), 1); }
      if (LW.isLinework(m) && LW.cmdOf(m) === LW.CMD.ACK && LW.bodyOf(m)[1] === LW.CMD.FW_DATA) outs.delete(LW.qqOf(m));
      most = Math.max(most, outs.size);
    };
    const ticker = setInterval(() => run(50), 5);   // keeps simulated time moving while it waits
    let got;
    try {
      got = await client.sendFirmware({ serial: before.serial, channel: before.channel }, image, p => stages.add(p.stage));
    } finally { clearInterval(ticker); }
    const blocks = Math.ceil(image.length / 64);
    check(got && got.join('.') === '0.1.1', 'the pedal restarted and reports 0.1.1: ' + got);
    check(['erasing', 'sending', 'checking', 'restarting'].every(s => stages.has(s)), 'progress stages ' + [...stages]);
    check(blocksOut === blocks, `each block sent once (${blocksOut} of ${blocks})`);
    check(most > 1 && most <= 4, `several blocks in flight, never more than 4 (most ${most})`);
    await run(1000);                                // Switchyard rescans, since the pedal lost its place
    const after = await client.pedalList();
    check(after.length === 3 && after[1].position === 2 && after[1].channel === before.channel && after[1].firmware.join('.') === '0.1.1',
      'pedal list: new version, same place and channel: ' + JSON.stringify(after.map(p => [p.position, p.channel, p.firmware.join('.')])));
    const kept = await client.readPreset(1, 4);
    check(JSON.stringify(kept) === '[[20,11],[21,22],[22,33],[80,127]]', 'presets kept through the update');

    // a pedal that isn't there: no answer to the begin
    try {
      await client.sendFirmware({ serial: Array(10).fill(0x7E), channel: 9 }, image, () => {}, { beginTimeout: 300 });
      check(false, 'an unknown serial should fail');
    } catch (e) { check(/no answer/.test(e.message), 'unknown serial: ' + e.message); }
    client.onMessage = (m, dir) => log.push(dir + ' ' + LW.describe(m));
  }

  check(LW.crc32(Buffer.from('123456789')) === 0xCBF43926, 'CRC-32 check value');
  check(log.some(l => l.startsWith('in Position: part 3, song 2.1')), 'log descriptions');

  console.log(`\n${checks} checks, ${fails} failed`);
  br.stdin.write('Q\n');
  process.exitCode = fails ? 1 : 0;
}

main().catch(e => { console.log('ERROR', e); br.kill(); process.exitCode = 1; });
