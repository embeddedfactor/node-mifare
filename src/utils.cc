#include "utils.h"
#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

void validResult(const Nan::FunctionCallbackInfo<v8::Value> &info, v8::Local<v8::Value> data) {
  v8::Local<v8::Object> result = Nan::New<v8::Object>();
  result->Set(Nan::New("err").ToLocalChecked(), Nan::New<v8::Array>());
  result->Set(Nan::New("data").ToLocalChecked(), data);
  info.GetReturnValue().Set(result);
}

void validTrue(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  validResult(info, Nan::New<v8::Boolean>(true));
}

MifareError errorResult(const Nan::FunctionCallbackInfo<v8::Value> &info, int no, const std::string msg, unsigned int res, const std::string msg2) {
  return errorResult(info, no, msg.c_str(), res, msg2.c_str());
}

MifareError errorResult(const Nan::FunctionCallbackInfo<v8::Value> &info, int no, const char *msg, unsigned int res, const char *msg2) {
  v8::Local<v8::Object> result = Nan::New<v8::Object>();
  v8::Local<v8::Array> errors = Nan::New<v8::Array>();
  v8::Local<v8::Object> error = Nan::New<v8::Object>();

  error->Set(Nan::New("code").ToLocalChecked(), Nan::New(no));
  error->Set(Nan::New("msg").ToLocalChecked(), Nan::New(msg).ToLocalChecked());
  error->Set(Nan::New("msg2").ToLocalChecked(), Nan::New(msg2).ToLocalChecked());
  error->Set(Nan::New("res").ToLocalChecked(), Nan::New(res));
  errors->Set(0, error);
  result->Set(Nan::New("err").ToLocalChecked(), errors);
  result->Set(Nan::New("data").ToLocalChecked(), Nan::Undefined());
  info.GetReturnValue().Set(result);
  return MifareError(msg, no);
}

v8::Local<v8::Object> buffer(uint8_t *data, size_t len) {
  v8::Local<v8::Object> result = Nan::New<v8::Object>();
  result->Set(
    Nan::New("ndef").ToLocalChecked(),
    Nan::CopyBuffer(reinterpret_cast<char *>(data), len).ToLocalChecked()
  );
  return result;
}

static int mifare_sleep_msec = 0;

void mifare_set_sleep(const Nan::FunctionCallbackInfo<v8::Value> &v8info) {
  if(v8info.Length()!=1 || !v8info[0]->IsNumber()) {
    throw errorResult(v8info, 0x12302, "This function takes a key number as arguments");
  }
  mifare_sleep_msec = Nan::To<int32_t>(v8info[0]).FromJust();
}

void mifare_sleep() {
#if defined(_WIN32)
  Sleep(mifare_sleep_msec);
#else
  usleep(mifare_sleep_msec * 1000);
#endif
}
