#include <assert.h>
#include <bare.h>
#include <js.h>
#include <utf.h>
#include <uv.h>

#include <string>
#include <iostream>

#include "tcp_cat.h"
#include "util/util.h"

static void
onCloseCb(uv_handle_t *handle)
{
  int err;
  TcpCat *ctx = (TcpCat *)handle->data;
  js_env_t *env = ctx->env;
  js_handle_scope_t *scope;
  js_value_t *result;

  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  err = js_get_boolean(env, true, &result);
  assert(err == 0);

  err = js_resolve_deferred(env, ctx->closePromise, result);
  assert(err == 0);

  delete ctx;
  ctx = nullptr;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}


static js_value_t *
tcpCatDispose(js_env_t *env, js_callback_info_t *info)
{
  int err;
  TcpCat *ctx;
  js_value_t **args = Util::collectArguments(env, info, nullptr);
  js_value_t *promise;

  err = js_get_value_external(env, args[0], (void **)&ctx);
  assert(err == 0);

  err = js_create_promise(env, &ctx->closePromise, &promise);
  assert(err == 0);

  ctx->socket->data = ctx;
  uv_close((uv_handle_t *)ctx->socket, onCloseCb);

  return promise;
}

static void
allocCb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
  buf->base = ((TcpCat *)handle->data)->response;
  buf->len = TcpCat::MAX_ALLOC_BUFSIZE;
}


static void
onReadCb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
  int err;
  TcpCat *ctx = (TcpCat *)stream->data;
  js_env_t *env = ctx->env;
  js_handle_scope_t *scope;
  js_value_t *receiver;
  js_value_t *onReadCb;
  js_value_t *args[2]{ };

  if (!nread) // buf->base is stack allocd, no need to free
    return;

  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  err = js_create_external(env, (void *)ctx, nullptr, nullptr, &args[0]);
  assert(err == 0);

  if (nread == UV_EOF) {
    err = js_get_null(env, &args[1]);
    assert(err == 0);
  } else if (nread < 0) {
    err = js_get_null(env, &args[1]);
    assert(err == 0);

    js_value_t *error = Util::getJsError(env, "ERR:onReadCb", "Callback for read from stream failed.");
  } else {
    void *data;
    err = js_create_arraybuffer(env, nread, &data, &args[1]);
    assert(err == 0);
    memcpy(data, buf->base, nread);
  }

  err = js_get_reference_value(env, ctx->receiver, &receiver);
  assert(err == 0);

  err = js_get_reference_value(env, ctx->onReadCb, &onReadCb);
  assert(err == 0);

  err = js_call_function(env, receiver, onReadCb, 2, args, nullptr);
  assert(err == 0);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}


static js_value_t *
tcpCatRead(js_env_t *env, js_callback_info_t *info)
{
  int err;
  auto args = Util::collectArguments(env, info, nullptr);
  TcpCat *ctx;
  js_value_t *result;

  err = js_get_value_external(env, args[0], (void **)&ctx);
  assert(err == 0);

  err = js_get_undefined(env, &result);
  assert(err == 0);


  ctx->socket->data = ctx;
  if (uv_read_start((uv_stream_t *)ctx->socket, allocCb, onReadCb) < 0) {
    err = js_throw_error(env, "ERR:uv_read_start", "Failed to read response from remote host.");
    assert(err == 0);

    return result;
  }

  err = js_create_reference(env, args[1], 1, &ctx->receiver);
  assert(err == 0);

  err = js_create_reference(env, args[2], 1, &ctx->onReadCb);
  assert(err == 0);

  return result;
}

static void
onWriteCb(uv_write_t *req, int status)
{
  int err;
  TcpCat *ctx = (TcpCat *)req->data;
  js_env_t *env = ctx->env;
  js_handle_scope_t *scope;
  js_value_t *result;

  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  if (status < 0) {
    auto error = Util::getJsError(env, "ERR:onWriteCb", "Callback for write to stream failed.");

    err = js_reject_deferred(env, ctx->writePromise, error);
    assert(err == 0);

    err = js_close_handle_scope(env, scope);
    assert(err == 0);

    return;
  }

  err = js_get_boolean(env, true, &result);
  assert(err == 0);

  err = js_resolve_deferred(env, ctx->writePromise, result);
  ctx->writePromise = nullptr;
  assert(err == 0);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}


static js_value_t *
tcpCatMakeRequest(js_env_t *env, js_callback_info_t *info)
{
  int err;
  TcpCat *ctx;
  std::string request;
  auto args = Util::collectArguments(env, info, nullptr);
  js_value_t *arraybuffer;
  void *data;
  size_t size;
  uv_buf_t buf;
  js_value_t *promise;
  js_handle_scope_t *scope;

  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  err = js_get_value_external(env, args[0], (void **)&ctx);
  assert(err == 0);

  request = Util::collectString(env, args[1]);

  err = js_create_promise(env, &ctx->writePromise, &promise);
  assert(err == 0);

  ctx->writeConn->data = ctx;
  buf = uv_buf_init((char *)request.data(), request.size());
  if (uv_write(ctx->writeConn, (uv_stream_t *)ctx->socket, &buf, 1, onWriteCb) < 0) {
    js_value_t *result;

    err = js_get_undefined(env, &result);
    assert(err == 0);

    err = js_throw_error(env, "ERR:uv_write", "Failed to write to remote host.");
    assert(err == 0);

    return result;
  }

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  return promise;
}


static void
onConnectCb(uv_connect_t *req, int status)
{
  int err;
  TcpCat *ctx = (TcpCat *)req->data;
  js_env_t *env = ctx->env;
  js_handle_scope_t *scope;
  js_value_t *result;

  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  if (status < 0) {
    auto error = Util::getJsError(env, "ERR:onConnectCb", "Callback for connect to remote host failed.");

    err = js_reject_deferred(env, ctx->connPromise, error);
    assert(err == 0);

    err = js_close_handle_scope(env, scope);
    assert(err == 0);

    return;
  }

  err = js_get_boolean(env, true, &result);
  assert(err == 0);

  err = js_resolve_deferred(env, ctx->connPromise, result);
  ctx->connPromise = nullptr;
  assert(err == 0);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}


static js_value_t *
tcpCatConnect(js_env_t *env, js_callback_info_t *info)
{
  int err;
  auto args = Util::collectArguments(env, info, nullptr);
  TcpCat *ctx;
  js_value_t *promise;
  js_handle_scope_t *scope;

  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  err = js_get_value_external(env, args[0], (void **)&ctx);
  assert(err == 0);

  err = js_create_promise(env, &ctx->connPromise, &promise);
  assert(err == 0);

  ctx->conn->data = ctx;
  if (uv_tcp_connect(ctx->conn, ctx->socket, &ctx->addr, onConnectCb) < 0) {
    js_value_t *result;

    err = js_get_undefined(env, &result);
    assert(err == 0);

    err = js_throw_error(env, "ERR:uv_tcp_connect", "Failed to connect to remote host.");
    assert(err == 0);

    return result;
  }

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  return promise;
}


static js_value_t *
tcpCatNew(js_env_t *env, js_callback_info_t *info) {
  int err;
  size_t size = 0;
  auto args = Util::collectArguments(env, info, &size);
  uv_loop_t *loop;
  TcpCat *ctx = nullptr;
  js_value_t *result;

  err = js_get_env_loop(env, &loop);
  assert(err == 0);

  ctx = new TcpCat{ loop };
  ctx->env = env;
  {
    auto ip = Util::collectString(env, args[0]);
    uint family;
    uint port;

    err = js_get_value_uint32(env, args[1], &family);
    assert(err == 0);

    err = js_get_value_uint32(env, args[2], &port);
    assert(err == 0);

    if (family == 4)
      err = uv_ip4_addr(ip.c_str(), port, (sockaddr_in *)&ctx->addr);
    else
      err = uv_ip6_addr(ip.c_str(), port, (sockaddr_in6 *)&ctx->addr);
    assert(err >= 0);
  }

  err = js_create_external(env, (void *)ctx, nullptr, nullptr, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
tcp_cat_exports(js_env_t *env, js_value_t *exports) {
  int err;

#define V(name, fn) \
  { \
    js_value_t *val; \
    err = js_create_function(env, name, -1, fn, NULL, &val); \
    assert(err == 0); \
    err = js_set_named_property(env, exports, name, val); \
    assert(err == 0); \
  }

  V("tcpCatNew", tcpCatNew) // ctor
  V("tcpCatMakeRequest", tcpCatMakeRequest)
  V("tcpCatConnect", tcpCatConnect)
  V("tcpCatRead", tcpCatRead)
  V("tcpCatDispose", tcpCatDispose)
#undef V

  return exports;
}

BARE_MODULE(tcp_cat, tcp_cat_exports)
