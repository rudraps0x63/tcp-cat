const test = require('brittle');
const { tcpCat } = require('./index.js');

test('tcpCat', async function (t) {
  t.test('GET request', async function (t) {
    const req = 'GET / HTTP/1.1\r\nHost: cloudflare.com\r\nConnection: close\r\n\r\n';
    const res = await tcpCat('1.1.1.1', 80, req);

    console.log(res.toString());
    t.ok(res.toString().includes('HTTP/1.1 301'))
  })
})
