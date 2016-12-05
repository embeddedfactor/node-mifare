#include "utils.h"
#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

void retry(int retries, bool (*try_f)()) {
  int count = retries;
  while(count>0) {
    if(!try_f()) {
      break;
    }
    --count;
  }
}

void validResult(const Nan::FunctionCallbackInfo<v8::Value> &info, v8::Local<v8::Value> data) {
  v8::Local<v8::Object> result = Nan::New<v8::Object>();
  result->Set(Nan::New("err").ToLocalChecked(), Nan::New<v8::Array>());
  result->Set(Nan::New("data").ToLocalChecked(), data);
  info.GetReturnValue().Set(result);
}

void validTrue(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  validResult(info, Nan::New<v8::Boolean>(true));
}

MifareError errorResult(const Nan::FunctionCallbackInfo<v8::Value> &info, int no, const std::string msg, unsigned int res) {
  return errorResult(info, no, msg.c_str(), res);
}

MifareError errorResult(const Nan::FunctionCallbackInfo<v8::Value> &info, int no, const char *msg, unsigned int res) {
  v8::Local<v8::Object> result = Nan::New<v8::Object>();
  v8::Local<v8::Array> errors = Nan::New<v8::Array>();
  v8::Local<v8::Object> error = Nan::New<v8::Object>();

  error->Set(Nan::New("code").ToLocalChecked(), Nan::New(no));
  error->Set(Nan::New("msg").ToLocalChecked(), Nan::New(msg).ToLocalChecked());
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
  mifare_sleep_msec = v8info[0]->ToInt32()->Value();
}

void mifare_sleep() {
#if defined(_WIN32)
  Sleep(mifare_sleep_msec);
#else
  usleep(mifare_sleep_msec * 1000);
#endif
}

#if NODE_VERSION_AT_LEAST(6, 0, 0)
  v8::Local<v8::Value> GetPrivate(v8::Local<v8::Object> object,
                                  v8::Local<v8::String> key) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Private> privateKey = v8::Private::ForApi(isolate, key);
    v8::Local<v8::Value> value;
    v8::Maybe<bool> result = object->HasPrivate(context, privateKey);
    if (!(result.IsJust() && result.FromJust()))
      return v8::Local<v8::Value>();
    if (object->GetPrivate(context, privateKey).ToLocal(&value))
      return value;
    return v8::Local<v8::Value>();
  }

  void SetPrivate(v8::Local<v8::Object> object,
                  v8::Local<v8::String> key,
                  v8::Local<v8::Value> value) {
    if (value.IsEmpty())
      return;
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Private> privateKey = v8::Private::ForApi(isolate, key);
    object->SetPrivate(context, privateKey, value);
  }
#else
  v8::Local<v8::Value> GetPrivate(v8::Local<v8::Object> object,
                                      v8::Local<v8::String> key) {
    return object->GetHiddenValue(key);
  }

  void SetPrivate(v8::Local<v8::Object> object,
                      v8::Local<v8::String> key,
                      v8::Local<v8::Value> value) {
    object->SetHiddenValue(key, value);
  }
#endif

