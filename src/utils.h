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

#if !defined(_NOEXCEPT)
#define _NOEXCEPT throw()
#endif

/**
 * Standard exception type for this node addon.
 * Can only be thrown and catched in this addon.
 * Globaly try/catch is disabled in nodejs.
 **/
class MifareError : public std::exception {
  public:
    /**
     * This exception takes a message and a id to construct
     * @param msg The error message.
     * @param id The error code.
     **/
    MifareError(const char *msg = NULL, const int id = 0) : m_id(id), m_msg(msg) {}

    /**
     * For some reason the destructor is explecite not allowed to throw anything in nodejs 0.10.x.
     * Although it is standard behavior in C++, it is very dangerous.
     **/
    virtual ~MifareError() _NOEXCEPT {}

    /**
     * Returns the message
     * @return The exception message.
     **/
    virtual const char *what() const _NOEXCEPT {
      return m_msg;
    }

    /**
     * Returns the id
     * @return The exception id.
     **/
    virtual int id() const _NOEXCEPT {
      return m_id;
    }

  private:
    const int m_id;
    const char *m_msg;
};

/**
 * Retries a clousre for retries times.
 * The clousre returns a boolean if it is false it will be retried until the maximum time are reached.
 * @param retries Number of times to try a clousre.
 * @param try_f The clousre function to call.
 */
void retry(int retries, bool (*try_f)());

/**
 * Returnes a valid result object and attaches it to the info scope.
 * @param info The InfoScope object used by the javascript function.
 * @param data The data object to return.
 * @return void The result is directly attached to the info scope.
 **/
void validResult(const Nan::FunctionCallbackInfo<v8::Value> &info, v8::Local<v8::Value> data);

/**
 * Returns a boolean result object with the value true and attaches it to the info scope.
 * @param info The InfoScope object used by the javascript function.
 * @return void The result is directly attached to the info scope.
 **/
void validTrue(const Nan::FunctionCallbackInfo<v8::Value> &info);

/**
 * Generates a error result object and attaches it to the info scope
 * @param info The InfoScope object used by the javascript function.
 * @param no The exception code.
 * @param msg The exception message.
 * @param res An internal error code from the underlying implementation (PCSC).
 * @return Returns a MifareError object to throw.
 **/
MifareError errorResult(const Nan::FunctionCallbackInfo<v8::Value> &info, int no, const char *msg, unsigned int res=0);

/**
 * Generates a error result object and attaches it to the info scope
 * @param info The InfoScope object used by the javascript function.
 * @param no The exception code.
 * @param msg The exception message.
 * @param res An internal error code from the underlying implementation (PCSC).
 * @return Returns a MifareError object to throw.
 **/
MifareError errorResult(const Nan::FunctionCallbackInfo<v8::Value> &info, int no, const std::string msg, unsigned int res=0);

/**
 * Make an node::Buffer from unsigned char pointer and length
 * @param data The pointer to the data
 * @param len The length of the data
 * @return An nodejs Buffer object
 **/
v8::Local<v8::Object> buffer(uint8_t *data, size_t len);

/**
 * Set a default sleep value to delay commands sent to the card reader.
 * The usage of this library has shown, that very often the reader cannot
 * transfere the data fast enought to the card before the next command is issued.
 * This results in an error which can be minified by waiting.
 * This is not the best way of dealing with the error.
 * It might even better to retry the command until it is correctly transfered.
 * @see retry
 * @param an int in microseconds.
 **/
void mifare_set_sleep(const Nan::FunctionCallbackInfo<v8::Value> &v8info);

/**
 * Issue a sleep for a predetermend time.
 * To delay commands sent to the card reader
 * @see mifare_set_sleep
 **/
void mifare_sleep();

// NaN Helper for nodejs 7.1.0
v8::Local<v8::Value> GetPrivate(v8::Local<v8::Object> object, v8::Local<v8::String> key);
void SetPrivate(v8::Local<v8::Object> object, v8::Local<v8::String> key, v8::Local<v8::Value> value);
#endif // UTILS_H
