## tcp-cat 🐈‍⬛

A small Bare addon that uses libuv and libjs to make requests to a given remote host (IPv4/IPv6 supported). A simple example
has been added in the form of the `test.js`.

It exposes a class called `TcpCat` and a free function `tcpCat` (that builds upon the class) for making requests to
a host at a given port number:
```javascript
const req = 'GET / HTTP/1.1\r\nHost: cloudflare.com\r\nConnection: close\r\n\r\n';
const res = await tcpCat('1.1.1.1', 80, req);
console.log(res.toString()); // Logs HTTP/1.1 301 Moved Permanently ...
```

### Dependencies
We require: NPM, [Bare](https://github.com/holepunchto/bare), `bare-make` (provides convenience functions to make a build system) and `cmake-bare`.

### API: `TcpCat`
#### `constructor ({ ip: string, port: number })`

Initializes the native instance with the given IP and port number.

---

#### `connect`

Establishes a TCP connection using the native binding. This must be called before sending any request.

---

#### `makeRequest (req: string)`

Sends a raw request string to the connected socket. The request must be a string and requires an active connection.

---

#### `read (callback: function(instance, data))`

Starts reading from the TCP stream. It accepts a callback function which receives streamed data chunks. The callback is invoked multiple times until the stream ends. The end of the stream is indicated by a `null` value.

---

#### `dispose`

Releases the native TCP handle and marks the instance as unusable. Once disposed, the instance cannot be reused.

---

### API: `tcpCat`

#### `tcpCat (ip: string, port: number, request: string)`

Launches a `TcpCat` instance and uses it to make a request.

---

### Testing
Clone the repository, and then in your terminal run:
```bash
$ npm i
$ bare-make generate # Specify --debug for a debug build
$ bare-make build
$ bare-make install
```
This will create a `build` as well as `prebuilds/` directories, which will contain the binary executables according to your machine.
Now, all we need to do is:
```bash
$ bare test.js
```

### Contributing
Contributions are welcome!
