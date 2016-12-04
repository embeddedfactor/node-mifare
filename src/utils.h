#ifndef UTILS_H
#define UTILS_H

#include <nan.h>
#include <iostream>
#include <cstring>
#include <exception>

#ifndef USE_LIBNFC
#if defined(__APPLE__) || defined(__linux__)
#include <PCSC/winscard.h>
#include <PCSC/wintypes.h>
#else
#include <winscard.h>
#endif
#include <freefare_pcsc.h>
#else // USE_LIBNFC
#include <nfc/nfc.h>
#include <freefare_nfc.h>
#endif // USE_LIBNFC

#if defined(USE_LIBNFC)
  typedef int res_t;
#else
  typedef LONG res_t;
#endif

class MifareError : public std::exception {
  public:
    MifareError(const char *msg = NULL, const int id = 0) : m_id(id), m_msg(msg) {}

    virtual ~MifareError() {}

    virtual const char *what() const {
      return m_msg;
    }

    virtual int id() const {
      return m_id;
    }

  private:
    const int m_id;
    const char *m_msg;
};

class ReaderError : public MifareError {};
class TagError : public MifareError {};
class DESFireTagError : public TagError {};

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
