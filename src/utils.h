#ifndef UTILS_H
#define UTILS_H

#include <nan.h>
#include <iostream>
#include <cstring>

void retry(int retries, bool (*try_f)());

void validResult(const Nan::FunctionCallbackInfo<v8::Value> &info, v8::Local<v8::Value> data);
void validTrue(const Nan::FunctionCallbackInfo<v8::Value> &info);
void errorResult(const Nan::FunctionCallbackInfo<v8::Value> &info, int no, const char *msg, unsigned int res=0);
void errorResult(const Nan::FunctionCallbackInfo<v8::Value> &info, int no, const std::string msg, unsigned int res=0);
v8::Local<v8::Object> buffer(uint8_t *data, size_t len);
void mifare_set_sleep(const Nan::FunctionCallbackInfo<v8::Value> &v8info);
void mifare_sleep();

// NaN Helper for nodejs 7.1.0
v8::Local<v8::Value> GetPrivate(v8::Local<v8::Object> object, v8::Local<v8::String> key);
void SetPrivate(v8::Local<v8::Object> object, v8::Local<v8::String> key, v8::Local<v8::Value> value);
#endif // UTILS_H
