const binding = require('./binding')

class TcpCat {
  constructor (opts = {}) {
    if (!opts.ip || !opts.port)
      throw new TypeError("Construction failed, some keys are missing.");

    if (typeof opts.ip !== 'string' || typeof opts.port !== 'number') {
      throw new TypeError("Construction failed, invalid types for given keys.");
    }

    let family = 4;
    if (opts.ip.includes(':'))
      family = 6;
    this._handle = binding.tcpCatNew(opts.ip, family, opts.port);
  }

  async connect() {
    this._isDisposed();

    this._connected = await binding.tcpCatConnect(this._handle);
  }

  async makeRequest (request) {
    this._isDisposed();

    if (typeof request !== 'string')
      throw new TypeError("Expected a string object for request");

    if (!this._connected)
      throw new Error("Cannot make requests to remote host unless connection is established.");

    this._madeRequest = await binding.tcpCatMakeRequest(this._handle, request);
  }

  async read (callback = null) {
    this._isDisposed();

    if (!this._madeRequest)
      throw new Error("Cannot read unless a request has been made prior.");

    binding.tcpCatRead(this._handle, this, callback);
  }

  async dispose () {
    this._isDisposed();

    this._disposed = await binding.tcpCatDispose(this._handle);
  }

  _isDisposed () {
    if (this._disposed)
      throw new Error("Reusing a disposed object!");
  }
}

const tcpCat = async function(ip, port, request) {
  return new Promise(async (resolve, reject) => {
    let tcp = null;
    const response = [];

    try {
      tcp = new TcpCat({ ip, port });
    } catch (e) {
      reject(e);

      return;
    }

    const onReadCb = (obj, data) => {
      if (data !== null) {
        response.push(Buffer.from(data));
      } else { // Reading over, can be disposed of
        resolve(Buffer.concat(response))
        tcp.dispose();
      }
    };

    await tcp.connect();
    await tcp.makeRequest(request);
    tcp.read(onReadCb);
  });
}

module.exports = { TcpCat, tcpCat };
