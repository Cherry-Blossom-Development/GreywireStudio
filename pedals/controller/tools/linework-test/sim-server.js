/* Serves the test page and connects it to the simulated loop (firmware/test/bridge.c), so the
 * tool can be tried without any hardware. Plain Node, no packages.
 *   make -C ../../firmware/test bridge     (once)
 *   node sim-server.js                       then open http://localhost:8765 */
'use strict';
const http = require('http');
const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');

const PORT = Number(process.env.PORT) || 8765;
const exe = path.join(__dirname, '../../firmware/test', process.platform === 'win32' ? 'bridge.exe' : 'bridge');
if (!fs.existsSync(exe)) {
  console.error('Build the bridge first: make -C pedals/controller/firmware/test bridge');
  process.exit(1);
}

let bridge, out = '', waiting = [], startup = '';
function start() {
  bridge = spawn(exe, [], { stdio: ['pipe', 'pipe', 'inherit'] });
  waiting.push(chunk => { startup = chunk; });       // the power-up scan's report, passed on with the first command
  bridge.stdout.on('data', d => {
    out += d.toString().replace(/\r/g, '');            // Windows line ends
    let i;
    while ((i = out.indexOf('OK\n')) >= 0) {
      const chunk = out.slice(0, i);
      out = out.slice(i + 3);
      const w = waiting.shift();
      if (w) w(chunk);
    }
  });
  bridge.on('exit', () => { console.log('bridge stopped'); process.exit(1); });
}
start();

let chain = Promise.resolve();
function command(line) {                               // one at a time, in order
  const p = chain.then(() => new Promise(r => {
    waiting.push(chunk => { const s = startup + chunk; startup = ''; r(s); });
    bridge.stdin.write(line.replace(/[\r\n]+/g, ' ') + '\n');
  }));
  chain = p.catch(() => {});
  return p;
}

const TYPES = { '.html': 'text/html', '.js': 'text/javascript', '.css': 'text/css' };
http.createServer((req, res) => {
  if (req.url === '/sim/ping') { res.end('ok'); return; }
  if (req.url === '/sim' && req.method === 'POST') {
    let body = '';
    req.on('data', d => { body += d; });
    req.on('end', () => command(body).then(r => res.end(r), e => { res.statusCode = 500; res.end(String(e)); }));
    return;
  }
  const file = path.join(__dirname, req.url === '/' ? 'index.html' : path.normalize(req.url).replace(/^([/\\])+/, ''));
  if (!file.startsWith(__dirname) || !TYPES[path.extname(file)] || !fs.existsSync(file)) {
    res.statusCode = 404;
    res.end('not found');
    return;
  }
  res.setHeader('Content-Type', TYPES[path.extname(file)]);
  fs.createReadStream(file).pipe(res);
}).listen(PORT, '127.0.0.1', () => console.log(`Linework test tool: http://localhost:${PORT}`));
