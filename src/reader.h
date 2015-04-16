// Copyright 2013, Rolf Meyer
// See LICENCE for more information
#ifndef READER_H
#define READER_H

#include <node.h>
#include <v8.h>
#include <node_buffer.h>
#include <vector>
#include <iostream>
#include <cstring>

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
#include <cstdlib>


using namespace v8;
using namespace node;



struct reader_data {
  /**
   * Create a new reader status instance
   * @param name The name of the reader
   * @return The SCARD_READERSTATE object representating this object
   */
  reader_data(const char* name,
#ifndef USE_LIBNFC
      pcsc_context *hContext
#else
      nfc_context *context,
      nfc_device *device
#endif
  )
  {
    this->name = std::string(name);
    this->timer.data = this;
#ifndef USE_LIBNFC
    this->context = hContext;
    this->state.szReader = this->name.c_str();
    this->state.dwCurrentState = SCARD_STATE_UNAWARE;
    this->state.pvUserData = this;
#else
    this->context = context;
    this->last_err = NFC_ENOTSUCHDEV;
    this->device = device;
#endif
  }

  std::string name;
  uv_timer_t timer;
#ifndef USE_LIBNFC
  SCARD_READERSTATE state;
  pcsc_context *context;
#else
  nfc_context *context;
  int last_err;
  std::vector< char* > last_uids;
  nfc_device *device;
#endif
  Persistent<Function> callback;
  Persistent<Object> self;
};

void reader_timer_callback(uv_timer_t *handle, int timer_status);
Handle<Value> ReaderRelease(const Arguments& args);
Handle<Value> ReaderListen(const Arguments& args);
Handle<Value> ReaderSetLed(const Arguments& args);

#endif // READER_H
