#include <js.h>

#include <string>

namespace Util {

static js_value_t **
collectArguments(js_env_t *env, js_callback_info_t *info, size_t *size)
{
  int err;
  size_t sz = 0;
  js_value_t **args = NULL;

  err = js_get_callback_info(env, info, &sz, NULL, NULL, NULL);
  assert(err == 0 && "Failed to get size of arguments.");

  args = new js_value_t*[sz];
  assert(args != NULL && "Failed to allocate memory.");

  err = js_get_callback_info(env, info, &sz, args, NULL, NULL);
  assert(err == 0 && "Failed to get arguments.");

  if (size)
    *size = sz;

  return args;
}

static std::string
collectString(js_env_t *env, js_value_t *value)
{
  int err;
  size_t len = 0;
  utf8_t *str = NULL;

  err = js_get_value_string_utf8(env, value, NULL, 0, &len);
  str = new utf8_t[len + 1];
  str[len] = '\0';
  err = js_get_value_string_utf8(env, value, str, len, NULL);

  return (char *)str;
}

static js_value_t *
getJsError(js_env_t *env, const char *code = nullptr, const char *message = nullptr)
{
  int err;
  js_value_t *error;
  js_value_t *c;
  js_value_t *msg;

  js_create_string_utf8(env, (utf8_t *)code, strlen(code), &c);
  js_create_string_utf8(env, (utf8_t *)code, strlen(code), &msg);
  err = js_create_error(env, c, msg, &error);

  return error;
}

}
