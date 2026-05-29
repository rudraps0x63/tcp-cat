#include <js.h>
#include <uv.h>

struct TcpCat {
  sockaddr addr;
  uv_tcp_t *socket;
  uv_connect_t *conn;
  uv_write_t *writeConn;

  js_env_t *env;
  js_ref_t *receiver;
  js_ref_t *onReadCb;

  js_deferred_t *connPromise;
  js_deferred_t *writePromise;
  js_deferred_t *closePromise;

  static constexpr size_t MAX_ALLOC_BUFSIZE = 65536;
  char response[MAX_ALLOC_BUFSIZE];

  TcpCat(uv_loop_t *loop) {
    socket = new uv_tcp_t;
    conn = new uv_connect_t;
    writeConn = new uv_write_t;

    uv_tcp_init(loop, socket);
  }

  virtual ~TcpCat() {
    delete socket;
    delete conn;
    delete writeConn;
  }
};
