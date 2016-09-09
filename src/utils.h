#ifndef UTILS_H
#define UTILS_H

#include <nan.h>
#include <iostream>
#include <cstring>

void validResult(const Nan::FunctionCallbackInfo<v8::Value> &info, v8::Local<v8::Value> data);
void validTrue(const Nan::FunctionCallbackInfo<v8::Value> &info);
void errorResult(const Nan::FunctionCallbackInfo<v8::Value> &info, int no, const char *msg);
void errorResult(const Nan::FunctionCallbackInfo<v8::Value> &info, int no, const std::string msg);
v8::Local<v8::Object> buffer(uint8_t *data, size_t len);
void mifare_set_sleep(const Nan::FunctionCallbackInfo<v8::Value> &v8info);
void mifare_sleep();

#endif // UTILS_H
