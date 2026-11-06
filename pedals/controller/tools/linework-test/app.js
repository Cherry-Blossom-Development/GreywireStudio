/* The test page: Web MIDI (or the simulated loop) on one side, lw.js's Client on the other. */
'use strict';
const $ = id => document.getElementById(id);
let client = null, pedals = [], lastPos = null;

// ---------------------------------------------------------------- log

const logEl = $('log');
function log(m, dir) {
  const d = LW.describe(m) || (m[0] === 0xF0 ? 'System exclusive' : 'MIDI');
  const e = document.createElement('div');
  e.className = 'e';
  const t = new Date().toTimeString().slice(0, 8);
  e.innerHTML = `<span class="t">${t}</span><span class="${dir}">${dir === 'out' ? '&rarr;' : '&larr;'}</span> ` +
    `${escapeHtml(d)}<span class="h">${LW.hex(m)}</span>`;
  const atBottom = logEl.scrollTop + logEl.clientHeight >= logEl.scrollHeight - 20;
  logEl.appendChild(e);
  while (logEl.childElementCount > 2000) logEl.firstChild.remove();
  if (atBottom) logEl.scrollTop = logEl.scrollHeight;
}
const escapeHtml = s => s.replace(/[&<>]/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;' }[c]));
$('log-hex').onchange = () => logEl.classList.toggle('nohex', !$('log-hex').checked);
$('btn-clear').onclick = () => { logEl.innerHTML = ''; };

// ---------------------------------------------------------------- connecting

function connected(send, label) {
  client = new LW.Client(send);
  client.onMessage = log;
  client.onPosition = showPosition;
  client.onStatus = showStatus;
  $('conn-state').textContent = label;
  $('conn-state').className = 'badge ok';
  document.querySelectorAll('main button').forEach(b => { b.disabled = false; });
  $('btn-fw-stop').disabled = true;
}

function setDisabled() {
  document.querySelectorAll('main button').forEach(b => { if (b.id !== 'btn-clear') b.disabled = true; });
}
setDisabled();

let access = null;
async function initMidi() {
  if (!navigator.requestMIDIAccess) {
    $('conn-midi').innerHTML = '<span class="note err">This browser has no Web MIDI. Use Chrome or Edge.</span>';
    return;
  }
  try {
    access = await navigator.requestMIDIAccess({ sysex: true });
  } catch (e) {
    $('conn-midi').innerHTML = '<span class="note err">MIDI access was refused (it needs the system exclusive permission).</span>';
    return;
  }
  const fill = () => {
    for (const [sel, ports] of [[$('midi-in'), access.inputs], [$('midi-out'), access.outputs]]) {
      const keep = sel.value;
      sel.innerHTML = '';
      for (const p of ports.values()) {
        const o = new Option(p.name, p.id);
        if (p.id === keep || (!keep && /switchyard/i.test(p.name))) o.selected = true;
        sel.add(o);
      }
      if (!sel.options.length) sel.add(new Option('(none found)', ''));
    }
  };
  fill();
  access.onstatechange = fill;
}

$('btn-midi').onclick = () => {
  if (!access) return;
  const inp = access.inputs.get($('midi-in').value), outp = access.outputs.get($('midi-out').value);
  if (!inp || !outp) { alertNote('conn-state', 'Pick a MIDI in and out'); return; }
  for (const p of access.inputs.values()) p.onmidimessage = null;
  connected(m => outp.send(m), 'Connected: ' + outp.name);
  inp.onmidimessage = e => client.receive(e.data);
};

function alertNote(id, text) { $(id).textContent = text; $(id).className = 'badge bad'; }

// ---------------------------------------------------------------- the simulated loop (sim-server.js)

let simQueue = Promise.resolve(), simBusy = 0;
function simCmd(line) {
  simBusy++;
  simQueue = simQueue.then(() => fetch('/sim', { method: 'POST', body: line }))
    .then(r => r.text()).then(text => {
      for (const l of text.split('\n')) {
        if (l.startsWith('A ')) client.receive(LW.parseHex(l.slice(2)));
        else if (l.startsWith('O ')) logOther(LW.parseHex(l.slice(2)));
        else if (l.startsWith('D ')) simDisplay(l.slice(2));
      }
    }).catch(e => log([], 'in')).finally(() => { simBusy--; });
  return simQueue;
}
function logOther(m) {
  const e = document.createElement('div');
  e.className = 'e';
  e.innerHTML = `<span class="t">${new Date().toTimeString().slice(0, 8)}</span>&#8599; Other-brands out: ` +
    `${escapeHtml(LW.describe(m))}<span class="h">${LW.hex(m)}</span>`;
  logEl.appendChild(e);
  logEl.scrollTop = logEl.scrollHeight;
}
function simDisplay(s) {             // "TTTT dp": 4 characters, decimal point mask
  const text = s.slice(0, 4), dp = Number(s.slice(5));
  let out = '';
  for (let i = 0; i < 4; i++) out += (text[i] === ' ' ? ' ' : text[i]) + ((dp >> i) & 1 ? '.' : '');
  $('readout').textContent = out;
}

fetch('/sim/ping').then(r => { if (r.ok) $('btn-sim').hidden = false; }).catch(() => {});
$('btn-sim').onclick = () => {
  connected(m => { simCmd('M ' + LW.hex(m)); simCmd('R 20'); }, 'Simulated loop');
  $('sim-panel').hidden = false;
  $('conn-midi').hidden = true;
  $('btn-sim').hidden = true;
  simCmd('R 0');
  setInterval(() => { if (!simBusy) simCmd('R 100'); }, 100);   // keep simulated time moving
  const names = ["Switchyard → pedal 1", 'Pedal 1 → 2', 'Pedal 2 → 3', 'Pedal 3 → Switchyard'];
  names.forEach((n, k) => {
    const b = document.createElement('button');
    let cut = false;
    b.textContent = 'Pull cable: ' + n;
    b.onclick = () => { cut = !cut; simCmd((cut ? 'C ' : 'F ') + k); b.textContent = (cut ? 'Plug in cable: ' : 'Pull cable: ') + n; };
    $('sim-cables').appendChild(b);
  });
};
$('sim-up').onclick = () => simCmd('S +1');
$('sim-down').onclick = () => simCmd('S -1');

// ---------------------------------------------------------------- showing answers

const pad2 = n => String(n).padStart(2, '0');
function showPosition(p) {
  lastPos = p;
  if (!$('sim-panel').hidden) { /* the simulated display drives the readout */ }
  else $('readout').textContent = `${pad2(p.song)}.${p.part}`;
  $('pos-why').textContent = `Part ${p.index + 1}: song ${p.song}, part ${p.part} (${p.whyText || LW.WHY[p.why]})`;
  $('jump-n').value = p.index + 1;
}
function showStatus(s) {
  $('loop-state').textContent = s.broken ? 'Loop BROKEN: ' + s.whereText : `Loop working, ${s.pedals} pedal(s)`;
  $('loop-state').className = 'badge ' + (s.broken ? 'bad' : 'ok');
}
function showPedals(list) {
  pedals = list;
  const tb = $('pedals').tBodies[0];
  tb.innerHTML = '';
  if (!list.length) tb.innerHTML = '<tr><td colspan="6" class="note">No pedals found.</td></tr>';
  for (const p of list) {
    const tr = tb.insertRow();
    tr.innerHTML = `<td>${p.position}</td><td>${p.channel <= 15 ? p.channel + 1 : '-'}</td><td class="mono">${LW.hex(p.model)}</td>` +
      `<td>${p.firmware.join('.')}</td><td class="mono">${LW.serialHex(p.serial)}</td><td></td>`;
    const b = document.createElement('button');
    b.textContent = 'Use';
    b.title = 'Fill in this pedal\'s channel below';
    b.onclick = () => { $('pd-ch').value = p.channel + 1; };
    tr.cells[5].appendChild(b);
  }
  const sel = $('fw-pedal'), keep = sel.value;
  sel.innerHTML = list.length ? '' : '<option value="">(scan the loop first)</option>';
  for (const p of list) {
    const o = new Option(`Position ${p.position}: ch ${p.channel <= 15 ? p.channel + 1 : '-'}, model ${LW.hex(p.model)}, ` +
      `firmware ${p.firmware.join('.')}`, LW.serialHex(p.serial));
    if (o.value === keep) o.selected = true;
    sel.add(o);
  }
}

/* run an action, showing errors (and optionally the result) in a note */
async function act(noteId, fn, ok) {
  const n = $(noteId);
  n.className = 'note';
  n.textContent = '...';
  try {
    const r = await fn();
    n.className = 'note good';
    const t = ok ? ok(r) : 'Done';
    if (t !== null) n.textContent = t;      // null: the answer's own handler has written the note
  } catch (e) {
    n.className = 'note err';
    n.textContent = e.message;
  }
}
const num = id => Number($(id).value);

// ---------------------------------------------------------------- buttons

$('btn-info').onclick = () => act('info', () => client.info(), i =>
  `Firmware ${i.firmware.join('.')}, spec ${i.spec}, serial ${LW.serialHex(i.serial)}, 9V ${i.power9v ? 'connected' : 'not connected'}, ` +
  `phone ${i.charging ? 'charging' : 'not charging'}`);
$('btn-status').onclick = () => act('info', () => client.loopStatus(), s => { showStatus(s); return 'Loop status updated'; });
$('btn-pos').onclick = () => act('pos-why', () => client.position(), p => `Part ${p.index + 1}: song ${p.song}, part ${p.part}`);
const posText = () => lastPos ? `Part ${lastPos.index + 1}: song ${lastPos.song}, part ${lastPos.part} (${LW.WHY[lastPos.why]})` : null;
$('btn-jump').onclick = () => act('pos-why', () => client.jump(num('jump-n') - 1), posText);
$('btn-next').onclick = () => act('pos-why', () => client.jump((lastPos ? lastPos.index : -1) + 1), posText);
$('btn-prev').onclick = () => act('pos-why', () => client.jump(Math.max(0, (lastPos ? lastPos.index : 1) - 1)), posText);

$('btn-scan').onclick = () => act('pd-out', () => client.scan(), l => { showPedals(l); return `Scan found ${l.length} pedal(s)`; });
$('btn-list').onclick = () => act('pd-out', () => client.pedalList(), l => { showPedals(l); return `${l.length} pedal(s)`; });
$('btn-pc').onclick = () => act('pd-out', async () => client.programChange(num('pd-ch') - 1, num('pd-slot') - 1), () => 'Sent');
$('btn-store').onclick = () => act('pd-out', () => client.storePreset(num('pd-ch') - 1, num('pd-slot') - 1, null), () => 'Stored');
$('btn-read').onclick = () => act('pd-out', () => client.readPreset(num('pd-ch') - 1, num('pd-slot') - 1),
  pairs => `Preset ${num('pd-slot')}: ` + pairs.map(([c, v]) => `CC${c}=${v}`).join(', '));
$('btn-report').onclick = () => act('pd-out', () => client.reportSettings(num('pd-ch') - 1),
  pairs => 'Settings: ' + pairs.map(([c, v]) => `CC${c}=${v}`).join(', '));

let fwStop = false;
$('btn-fw').onclick = () => act('fw-out', async () => {
  const p = pedals.find(x => LW.serialHex(x.serial) === $('fw-pedal').value);
  if (!p) throw new Error('Pick a pedal (press Scan loop first)');
  if (p.channel > 15) throw new Error('That pedal has no channel yet; scan the loop again');
  const f = $('fw-file').files[0];
  if (!f) throw new Error('Pick a firmware file');
  const image = LW.firmwareImage(new Uint8Array(await f.arrayBuffer()), f.name);
  fwStop = false;
  $('btn-fw').disabled = true;
  $('btn-fw-stop').disabled = false;
  const t0 = Date.now(), kb = (image.length / 1024).toFixed(1);
  const words = { erasing: 'Erasing the spare copy', checking: 'Checking', restarting: 'Restarting the pedal' };
  try {
    const got = await client.sendFirmware(p, image, s => {
      $('fw-bar').value = s.total ? s.done / s.total : 0;
      const secs = (Date.now() - t0) / 1000;
      $('fw-out').textContent = s.stage === 'sending'
        ? `Sending ${kb} KB: block ${s.done} of ${s.total} (${(s.done * 64 / 1024 / Math.max(secs, 0.1)).toFixed(1)} KB/s)`
        : words[s.stage] + '...';
    }, { cancel: () => fwStop });
    return { got, secs: (Date.now() - t0) / 1000 };
  } finally {
    $('btn-fw').disabled = false;
    $('btn-fw-stop').disabled = true;
  }
}, ({ got, secs }) => {
  client.pedalList().then(showPedals).catch(() => {});
  return got ? `Done in ${secs.toFixed(0)} s. The pedal restarted with firmware ${got.join('.')}.`
    : `Sent and checked in ${secs.toFixed(0)} s, but the pedal hasn't introduced itself yet. Press Scan loop to check its version.`;
});
$('btn-fw-stop').onclick = () => { fwStop = true; };

$('btn-upload').onclick = () => act('sl-out', async () => {
  const sl = LW.parseSetlist($('sl-text').value, pedals);
  sl.id = num('sl-id');
  const crc = await client.uploadSetlist(sl, s => { $('sl-out').textContent = s + '...'; });
  return { sl, crc };
}, ({ sl, crc }) => `Sent: ${sl.parts.length} part(s), ${sl.table.length} pedal(s), CRC ${crc.toString(16).toUpperCase()}. ` +
  'Switchyard stays on its part number; press Jump to apply.');
$('btn-slinfo').onclick = () => act('sl-out', () => client.setlistInfo(), i =>
  `Setlist ${i.id}: ${i.parts} part(s), ${i.pedals} pedal(s), wrap ${i.wrap ? 'on' : 'off'}, CRC ${i.crc.toString(16).toUpperCase()}`);

$('ob-kind').onchange = () => {
  const cc = $('ob-kind').value === 'cc';
  $('ob-v1-label').textContent = cc ? 'CC number' : 'Preset';
  $('ob-v2-wrap').hidden = !cc;
};
$('btn-ob').onclick = () => act('info', () => {
  const cc = $('ob-kind').value === 'cc';
  const v1 = cc ? num('ob-v1') : num('ob-v1') - 1;
  if (v1 < 0 || v1 > 127) throw new Error(cc ? 'CC numbers are 0-127' : 'presets are 1-128');
  return client.otherSend(cc ? 'cc' : 'pc', num('ob-ch') - 1, v1, num('ob-v2'));
}, () => 'Sent to the other-brands MIDI out');

$('btn-raw').onclick = () => act('info', async () => client.out(LW.parseHex($('raw').value)), () => 'Sent');

initMidi();
