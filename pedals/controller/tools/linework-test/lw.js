/* Linework Control Specification 0.2, the phone app's side: building and reading messages,
 * and a client that talks to Switchyard with sequence numbers, timeouts and retries.
 * No browser code here, so the same file runs in the test page and under Node (test/run.js).
 * Loaded as a plain script (it sets window.LW), so the page also works opened as a file. */
(function (root) {
  'use strict';

  // The spec asks for the header to be one constant: our own MIDI Association ID will replace 7D.
  const HEADER = [0xF0, 0x7D, 0x4C, 0x57];
  const H = HEADER.length;

  const CMD = {
    POSITION: 0x01, PEDAL_INFO: 0x02, ASSIGN: 0x03, LOOP_BREAK: 0x08,
    REPORT_REQ: 0x10, REPORT: 0x11, STORE: 0x20, READ_REQ: 0x21, READ: 0x22,
    FW_BEGIN: 0x30, FW_DATA: 0x31, FW_FINISH: 0x32, ACK: 0x7E,
    INFO_REQ: 0x40, INFO: 0x41, SCAN: 0x42, LIST_REQ: 0x44, LIST_COUNT: 0x45, LIST_ENTRY: 0x46,
    STATUS_REQ: 0x48, STATUS: 0x49, OTHER_SEND: 0x4A,
    SL_BEGIN: 0x50, SL_PEDAL: 0x51, SL_PART: 0x52, SL_FINISH: 0x53, SL_INFO_REQ: 0x54, SL_INFO: 0x55,
    POS_REQ: 0x58, POS: 0x59, JUMP: 0x5A,
  };
  const CMD_NAME = {};
  for (const k in CMD) CMD_NAME[CMD[k]] = k;

  const STATUS = ['OK', 'Unknown command', 'Value out of range', 'Checksum failed', 'Busy',
    'Firmware check failed', 'No setlist loaded', 'Setlist check failed'];
  const WHY = ['Up', 'Down', 'Up at the end (no wrap)', 'Down at the start (no wrap)', 'Jump', 'Power-up', 'Get position'];

  // ---------------------------------------------------------------- byte helpers

  const b7 = v => v & 0x7F;
  const put14 = v => [v & 0x7F, (v >> 7) & 0x7F];
  const get14 = (b, i) => (b[i] & 0x7F) | ((b[i + 1] & 0x7F) << 7);
  const put32 = v => { const o = []; for (let i = 0; i < 5; i++) o.push(Math.floor(v / 2 ** (7 * i)) & 0x7F); return o; };
  const get32 = (b, i) => { let v = 0; for (let k = 0; k < 5; k++) v += (b[i + k] & 0x7F) * 2 ** (7 * k); return v >>> 0; };

  const CRC_TABLE = (() => {
    const t = new Uint32Array(256);
    for (let n = 0; n < 256; n++) {
      let c = n;
      for (let k = 0; k < 8; k++) c = (c & 1) ? (0xEDB88320 ^ (c >>> 1)) : (c >>> 1);
      t[n] = c >>> 0;
    }
    return t;
  })();
  function crc32(bytes, crc = 0) {
    crc = (~crc) >>> 0;
    for (const b of bytes) crc = (CRC_TABLE[(crc ^ b) & 0xFF] ^ (crc >>> 8)) >>> 0;
    return (~crc) >>> 0;
  }

  function msg(cmd, qq, body = []) {
    return [...HEADER, b7(cmd), b7(qq), ...body.map(b7), 0xF7];
  }
  function isLinework(m) {
    return m.length >= H + 3 && HEADER.every((v, i) => m[i] === v) && m[m.length - 1] === 0xF7;
  }
  const cmdOf = m => m[H];
  const qqOf = m => m[H + 1];
  const bodyOf = m => Array.from(m.slice(H + 2, m.length - 1));   // the bytes between qq and F7

  const hex = m => Array.from(m, b => b.toString(16).toUpperCase().padStart(2, '0')).join(' ');
  function parseHex(s) {
    const t = s.trim().split(/[\s,]+/).filter(Boolean).map(x => parseInt(x, 16));
    if (t.some(v => isNaN(v) || v < 0 || v > 255)) throw new Error('not hex bytes: ' + s);
    return t;
  }
  const serialHex = s => s.map(b => b.toString(16).toUpperCase().padStart(2, '0')).join('');

  // ---------------------------------------------------------------- reading answers

  function pedalEntry(b) {   // pp dd m0 m1 aa bb cc ss s0..s9
    return {
      position: b[0], channel: b[1], model: [b[2], b[3]], firmware: [b[4], b[5], b[6]],
      spec: b[7], serial: b.slice(8, 18),
    };
  }

  /* A one-line description of any message, for the log */
  function describe(m) {
    m = Array.from(m);
    const s = m[0];
    if (s === 0xFE) return 'Active Sensing';
    if (s >= 0xF8) return 'Real-time ' + hex([s]);
    if ((s & 0xF0) === 0xC0) return `Program change ch ${(s & 15) + 1}: preset ${m[1] + 1}`;
    if ((s & 0xF0) === 0xB0) return `Control change ch ${(s & 15) + 1}: CC ${m[1]} = ${m[2]}`;
    if (s === 0xF0 && m[1] === 0x7E && m[3] === 0x06 && m[4] === 0x01) return 'Identity request';
    if (s === 0xF0 && m[1] === 0x7E && m[3] === 0x06 && m[4] === 0x02) {
      const p = m.slice(5 + H - 1);
      return `Identity reply: ch ${m[2] === 0x7F ? 'none' : m[2] + 1}, model ${hex(p.slice(0, 2))}, firmware ${p[2]}.${p[3]}.${p[4]}, spec ${p[5]}`;
    }
    if (!isLinework(m)) return '';
    const c = cmdOf(m), q = qqOf(m), b = bodyOf(m);
    const name = CMD_NAME[c] || ('command ' + hex([c]));
    const qs = ` [qq ${hex([q])}]`;
    switch (c) {
      case CMD.ACK: return `Acknowledgement of ${CMD_NAME[b[1]] || hex([b[1]])} from ${b[0] === 0x7F ? 'Switchyard' : 'ch ' + (b[0] + 1)}: ${STATUS[b[2]] || b[2]}${qs}`;
      case CMD.POSITION: return `Position count ${b[0]}${qs}`;
      case CMD.PEDAL_INFO: return `Pedal info: position ${b[0]}, ch ${b[1] === 0x7F ? 'none' : b[1] + 1}, serial ${serialHex(b.slice(2))}`;
      case CMD.LOOP_BREAK: return `Loop break report from position ${b[0]}`;
      case CMD.REPORT: return `Settings from ch ${b[0] + 1}: ${pairsText(b.slice(1))}${qs}`;
      case CMD.READ: return `Preset ${b[1] + 1} on ch ${b[0] + 1}: ${pairsText(b.slice(2))}${qs}`;
      case CMD.INFO: return `Switchyard info: firmware ${b[0]}.${b[1]}.${b[2]}, spec ${b[3]}, serial ${serialHex(b.slice(4, 14))}, 9V ${b[14] & 1 ? 'yes' : 'no'}, charging ${b[14] & 2 ? 'yes' : 'no'}${qs}`;
      case CMD.LIST_COUNT: return `Pedal list: ${b[0]} pedal(s)${qs}`;
      case CMD.LIST_ENTRY: { const p = pedalEntry(b); return `Pedal ${p.position}: ch ${p.channel + 1}, model ${hex(p.model)}, firmware ${p.firmware.join('.')}, serial ${serialHex(p.serial)}${qs}`; }
      case CMD.STATUS: return `Loop status: ${b[0] ? 'BROKEN' : 'working'}, ${b[1]} pedal(s), ${b[0] ? whereText(b[2]) : ''}${qs}`;
      case CMD.SL_INFO: return `Setlist info: ID ${get32(b, 0)}, CRC ${get32(b, 5).toString(16).toUpperCase()}, wrap ${b[10] & 1 ? 'on' : 'off'}, ${b[11]} pedal(s), ${get14(b, 12)} part(s)${qs}`;
      case CMD.POS: return `Position: part ${get14(b, 0) + 1}, song ${b[2]}.${b[3]} (${WHY[b[4]] || b[4]})${qs}`;
      default: return name + qs;
    }
  }
  const pairsText = b => { const o = []; for (let i = 0; i + 1 < b.length; i += 2) o.push(`CC${b[i]}=${b[i + 1]}`); return o.join(' '); };
  function whereText(bb) {
    if (bb === 0x00) return "look at the cable from Switchyard's MIDI out";
    if (bb === 0x7F) return "look at the last pedal or the cable back into Switchyard";
    return `look at pedal ${bb} or the cables either side of it`;
  }

  // ---------------------------------------------------------------- client

  class Client {
    /* send(bytes): sends one MIDI message to Switchyard. Call receive(bytes) for each message
     * that comes back. Events: onMessage(m, dir), onPosition(p), onStatus(s). */
    constructor(send, opts = {}) {
      this.send = send;
      this.seq = 0x3F;
      this.pending = [];
      this.timeout = opts.timeout || 1000;
      this.tries = opts.tries || 3;
      this.onMessage = () => {};
      this.onPosition = () => {};
      this.onStatus = () => {};
    }

    nextSeq() { this.seq = this.seq >= 0x7F ? 0x40 : this.seq + 1; return this.seq; }

    out(m) { this.onMessage(m, 'out'); this.send(m); }

    receive(data) {
      const m = Array.from(data);
      this.onMessage(m, 'in');
      if (isLinework(m)) {
        const c = cmdOf(m), b = bodyOf(m);
        if (c === CMD.POS) this.onPosition({ index: get14(b, 0), song: b[2], part: b[3], why: b[4], whyText: WHY[b[4]] });
        if (c === CMD.STATUS) this.onStatus({ broken: !!b[0], pedals: b[1], where: b[2], whereText: b[0] ? whereText(b[2]) : '' });
      }
      for (const p of [...this.pending]) {
        let r;
        try { r = p.match(m, p.qq); } catch (e) { r = undefined; }
        if (r !== undefined) {
          this.pending.splice(this.pending.indexOf(p), 1);
          clearTimeout(p.timer);
          p.resolve(r);
        }
      }
    }

    /* build(qq) -> message; match(m, qq) -> undefined to keep waiting, or the result.
     * Retries with a new sequence number after the timeout, three tries in all (spec). */
    request(build, match, opts = {}) {
      const timeout = opts.timeout || this.timeout, tries = opts.tries || this.tries;
      return new Promise((resolve, reject) => {
        let left = tries;
        const attempt = () => {
          const qq = this.nextSeq();
          const p = { qq, match, resolve };
          p.timer = setTimeout(() => {
            this.pending.splice(this.pending.indexOf(p), 1);
            if (--left > 0) attempt();
            else reject(new Error('no answer after ' + tries + ' tries'));
          }, timeout);
          this.pending.push(p);
          this.out(build(qq));
        };
        attempt();
      });
    }

    // an acknowledgement of cmd with our qq: resolves to its status number
    static ackOf(cmd) {
      return (m, qq) => (isLinework(m) && cmdOf(m) === CMD.ACK && qqOf(m) === qq && bodyOf(m)[1] === cmd)
        ? bodyOf(m)[2] : undefined;
    }

    static answer(cmd, read, ackCmd) {
      return (m, qq) => {
        if (!isLinework(m) || qqOf(m) !== qq) return undefined;
        if (cmdOf(m) === cmd) return read(bodyOf(m));
        if (cmdOf(m) === CMD.ACK && bodyOf(m)[1] === ackCmd) throw new LWError(bodyOf(m)[2]);
        return undefined;
      };
    }

    async ackRequest(cmd, body, opts) {
      const st = await this.request(qq => msg(cmd, qq, body), Client.ackOf(cmd), opts);
      if (st !== 0) throw new LWError(st);
      return st;
    }

    // ---- Switchyard

    info() {
      return this.request(qq => msg(CMD.INFO_REQ, qq), Client.answer(CMD.INFO, b => ({
        firmware: b.slice(0, 3), spec: b[3], serial: b.slice(4, 14), power9v: !!(b[14] & 1), charging: !!(b[14] & 2),
      })));
    }

    identity() {
      return this.request(() => [0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7], m =>
        (m[0] === 0xF0 && m[1] === 0x7E && m[3] === 0x06 && m[4] === 0x02 && m[2] === 0x7F)
          ? { model: m.slice(5 + H - 1, 7 + H - 1), firmware: m.slice(7 + H - 1, 10 + H - 1), spec: m[10 + H - 1] } : undefined);
    }

    _list(cmd, timeout) {
      let want = -1;
      const got = [];
      return this.request(qq => msg(cmd, qq), (m, qq) => {
        if (!isLinework(m) || qqOf(m) !== qq) return undefined;
        if (cmdOf(m) === CMD.LIST_COUNT) want = bodyOf(m)[0];
        else if (cmdOf(m) === CMD.LIST_ENTRY) got.push(pedalEntry(bodyOf(m)));
        else return undefined;
        return (want >= 0 && got.length >= want) ? got.slice() : undefined;
      }, { timeout });
    }
    pedalList() { return this._list(CMD.LIST_REQ); }
    scan() { return this._list(CMD.SCAN, 3000); }      // a scan takes a few hundred ms

    loopStatus() {
      return this.request(qq => msg(CMD.STATUS_REQ, qq), Client.answer(CMD.STATUS, b =>
        ({ broken: !!b[0], pedals: b[1], where: b[2], whereText: b[0] ? whereText(b[2]) : '' })));
    }

    position() {
      return this.request(qq => msg(CMD.POS_REQ, qq), Client.answer(CMD.POS, b =>
        ({ index: get14(b, 0), song: b[2], part: b[3], why: b[4] }), CMD.POS_REQ));
    }

    jump(index) { return this.ackRequest(CMD.JUMP, put14(index)); }

    // kind 'pc' (value = preset 0-127) or 'cc' (number, value); channel 0-15
    otherSend(kind, channel, v1, v2 = 0) {
      return this.ackRequest(CMD.OTHER_SEND, [kind === 'cc' ? 1 : 0, channel, v1, kind === 'cc' ? v2 : 0]);
    }

    setlistInfo() {
      return this.request(qq => msg(CMD.SL_INFO_REQ, qq), Client.answer(CMD.SL_INFO, b => ({
        id: get32(b, 0), crc: get32(b, 5), wrap: !!(b[10] & 1), pedals: b[11], parts: get14(b, 12),
      }), CMD.SL_INFO_REQ));
    }

    /* setlist = { id, wrap, table: [{serial:[10]} | {channel}], parts: [{song, part, presets:[[ix, slot], ...]}] }
     * Entries go without waiting (spec: they're not acknowledged one by one); the CRC at
     * Setlist finish checks them all. Resolves with the setlist's CRC. */
    async uploadSetlist(sl, progress = () => {}) {
      const nparts = sl.parts.length;
      if (sl.table.length < 1 || sl.table.length > 32) throw new Error('the pedal table needs 1-32 entries');
      if (nparts < 1 || nparts > 4000) throw new Error('a setlist needs 1-4000 parts');
      progress('erasing the spare copy');
      await this.ackRequest(CMD.SL_BEGIN, [...put32(sl.id >>> 0), sl.wrap ? 1 : 0, sl.table.length, ...put14(nparts)],
        { timeout: 8000, tries: 1 });          // erasing 320 KB of flash takes a while
      let crc = 0;
      const failures = [];
      const watch = (m) => {                  // entries are only answered if something is wrong
        if (isLinework(m) && cmdOf(m) === CMD.ACK && (bodyOf(m)[1] === CMD.SL_PEDAL || bodyOf(m)[1] === CMD.SL_PART))
          failures.push(STATUS[bodyOf(m)[2]]);
      };
      const before = this.onMessage;
      this.onMessage = (m, dir) => { before(m, dir); if (dir === 'in') watch(m); };
      try {
        const qq = this.nextSeq();
        const entry = (cmd, body) => { const m = msg(cmd, qq, body); crc = crc32(bodyOf(m), crc); this.out(m); };
        sl.table.forEach((e, ix) => entry(CMD.SL_PEDAL, e.serial ? [ix, 0, ...e.serial] : [ix, 1, e.channel]));
        sl.parts.forEach((p, i) => {
          entry(CMD.SL_PART, [...put14(i), p.song, p.part, ...p.presets.flat()]);
          if (i % 50 === 49) progress(`sent ${i + 1} of ${nparts} parts`);
        });
        progress('checking');
        await this.ackRequest(CMD.SL_FINISH, put32(crc), { timeout: 3000 });
      } finally {
        this.onMessage = before;
      }
      if (failures.length) throw new Error('Switchyard refused an entry: ' + failures[0]);
      return crc;
    }

    // ---- pedals, through the loop (addressed by channel 0-15)

    static checkRange(ch, slot) {
      if (!(Number.isInteger(ch) && ch >= 0 && ch <= 15)) throw new RangeError('channel must be 0-15 (MIDI channels 1-16)');
      if (slot !== undefined && !(Number.isInteger(slot) && slot >= 0 && slot <= 127))
        throw new RangeError('preset slot must be 0-127 (presets 1-128)');
    }
    async reportSettings(ch) {
      Client.checkRange(ch);
      return this.request(qq => msg(CMD.REPORT_REQ, qq, [ch]), Client.answer(CMD.REPORT, b => pairsOf(b.slice(1)), CMD.REPORT_REQ));
    }
    async readPreset(ch, slot) {
      Client.checkRange(ch, slot);
      return this.request(qq => msg(CMD.READ_REQ, qq, [ch, slot]), Client.answer(CMD.READ, b => pairsOf(b.slice(2)), CMD.READ_REQ));
    }
    // pairs: [[cc, value], ...], or null to store the pedal's current settings
    async storePreset(ch, slot, pairs) {
      Client.checkRange(ch, slot);
      return this.ackRequest(CMD.STORE, pairs ? [ch, slot, 0, ...pairs.flat()] : [ch, slot, 1]);
    }
    programChange(ch, slot) { Client.checkRange(ch, slot); this.out([0xC0 | ch, slot]); }
    controlChange(ch, cc, v) { Client.checkRange(ch); this.out([0xB0 | ch, cc & 0x7F, v & 0x7F]); }
  }
  const pairsOf = b => { const o = []; for (let i = 0; i + 1 < b.length; i += 2) o.push([b[i], b[i + 1]]); return o; };

  class LWError extends Error {
    constructor(st) { super(STATUS[st] || 'status ' + st); this.status = st; }
  }

  // ---------------------------------------------------------------- setlist text format

  /* A short text format for typing a setlist in the test page:
   *   wrap: yes
   *   pedal A = scan 1        a Linework pedal: the pedal at position 1 in the last scan
   *   pedal F = serial 0A1B2C3D4E5F60718293   (or by serial, 20 hex digits)
   *   pedal S = other 3       another brand's pedal on MIDI channel 3
   *   1.1  A:5 S:10           song 1 part 1: preset 5 on A, preset 10 on S
   *   1.2  F:7
   * Presets and channels are counted from 1, as players see them. # starts a comment. */
  function parseSetlist(text, scanned = []) {
    const table = [], names = {}, parts = [];
    let wrap = false;
    text.split(/\r?\n/).forEach((raw, n) => {
      const line = raw.replace(/#.*/, '').trim();
      if (!line) return;
      const err = msg => { throw new Error(`line ${n + 1}: ${msg}`); };
      let m;
      if ((m = line.match(/^wrap\s*:\s*(yes|no|on|off)$/i))) { wrap = /yes|on/i.test(m[1]); return; }
      if ((m = line.match(/^pedal\s+(\w+)\s*=\s*(scan|serial|other)\s+(\S+)$/i))) {
        const [, name, kind, arg] = m;
        if (names[name] !== undefined) err(`pedal ${name} is defined twice`);
        let e;
        if (/scan/i.test(kind)) {
          const p = scanned.find(x => x.position === Number(arg));
          if (!p) err(`no pedal at position ${arg} in the last scan (press Scan loop first)`);
          e = { serial: p.serial.slice() };
        } else if (/serial/i.test(kind)) {
          if (!/^[0-9a-f]{20}$/i.test(arg)) err('a serial is 20 hex digits');
          e = { serial: arg.match(/../g).map(x => parseInt(x, 16) & 0x7F) };
        } else {
          const ch = Number(arg);
          if (!(ch >= 1 && ch <= 16)) err('channels are 1-16');
          e = { channel: ch - 1 };
        }
        names[name] = table.length;
        table.push(e);
        return;
      }
      if ((m = line.match(/^(\d+)\.(\d+)((?:\s+\w+:\d+)*)$/))) {
        const song = Number(m[1]), part = Number(m[2]);
        if (song < 1 || song > 99 || part < 1 || part > 99) err('songs and parts are 1-99');
        const presets = [];
        for (const t of m[3].trim().split(/\s+/).filter(Boolean)) {
          const [name, slot] = t.split(':');
          if (names[name] === undefined) err(`pedal ${name} isn't defined`);
          if (!(Number(slot) >= 1 && Number(slot) <= 128)) err('presets are 1-128');
          presets.push([names[name], Number(slot) - 1]);
        }
        if (presets.length > 32) err('at most 32 pedals per part');
        parts.push({ song, part, presets });
        return;
      }
      err(`can't read "${line}"`);
    });
    if (!table.length) throw new Error('no pedals defined');
    if (!parts.length) throw new Error('no parts');
    return { wrap, table, parts };
  }

  const LW = {
    HEADER, CMD, STATUS, WHY, msg, isLinework, cmdOf, qqOf, bodyOf, crc32, put14, get14, put32, get32,
    hex, parseHex, serialHex, describe, whereText, Client, LWError, parseSetlist,
  };
  if (typeof module !== 'undefined' && module.exports) module.exports = LW;
  else root.LW = LW;
})(typeof window !== 'undefined' ? window : globalThis);
