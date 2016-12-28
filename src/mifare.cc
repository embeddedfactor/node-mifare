// Copyright 2013, Rolf Meyer
// See LICENCE for more information

#include <nan.h>
#include <node_buffer.h>
#include <vector>
#include <iostream>
#include <cstring>


#include "reader.h"
#include "utils.h"

/**
 * plugin global secure card context
 **/
#if defined(USE_LIBNFC)
#include <signal.h>

#define mifare_init nfc_init
#define mifare_exit nfc_exit
typedef nfc_context mifare_context;

#else
#define mifare_init pcsc_init
#define mifare_exit pcsc_exit
typedef pcsc_context mifare_context;
#endif

static mifare_context *context = NULL;
static std::vector<ReaderData *> readers_data;
static Nan::Persistent<v8::Object> readers_global(Nan::New<v8::Object>());


/**
 * Get Names of the Readers connected to the computer
 * @param hContext The SCard Context used to search
 * @return An Array of Strings with reader names
 **/
void getReader(const Nan::FunctionCallbackInfo<v8::Value>& info) {
#if defined(USE_LIBNFC)
  const size_t MAX_READERS = 16;
  size_t numDevices;
  nfc_connstring reader_names[MAX_READERS];
#else
  LONG res;
  char *reader_names;
#endif
  char *reader_iter = NULL;
  int   reader_count = 0;
  // Allocate buffer. We assume autoallocate is not present (on Mac OS X anyway)
  v8::Local<v8::Object> readers_local = Nan::New<v8::Object>(readers_global);

  if(info.Length() > 0){
    Nan::ThrowError("This function does not take any arguments");
    return;
  }

  if(context) {
    mifare_exit(context);
  }
  mifare_init(&context);
  if(!context) {
    Nan::ThrowError("Cannot establish context");
    return;
  }
#if defined(USE_LIBNFC)
  numDevices = nfc_list_devices(context, reader_names, MAX_READERS);
  if(numDevices == 0) {
    info.GetReturnValue().Set(Nan::New<v8::Object>(readers_global));
    return;
  }
#else
  res = pcsc_list_devices(context, &reader_names);
  if(res != SCARD_S_SUCCESS || reader_names[0] == '\0') {
    Nan::ThrowError("Unable to list readers");
    return;
  }
#endif

  // Clean before use
  for(std::vector<ReaderData *>::iterator iter = readers_data.begin();iter!=readers_data.end();iter++) {
    ReaderData *data = *iter;
    if(data) {
      delete data;
    }
    *iter = NULL;
  }
  readers_data.clear();

  // Get number of readers from null separated string
#if defined(USE_LIBNFC)
  reader_iter = reader_names[0];
  char *names_end = reader_names[0] + (MAX_READERS * sizeof(nfc_connstring));
  while(reader_iter <= names_end) {
    // see if we can claim it
    nfc_device *dev = NULL;
    dev = nfc_open(context, reader_iter);
    if (dev == NULL) {
      // XXX: failed to open connstring
      reader_iter += sizeof(nfc_connstring);
      continue;
    }
    std::cout << "Found device: " << nfc_device_get_name(dev) << std::endl;
    nfc_close(dev);
#else
  reader_iter = reader_names;
  while(*reader_iter != '\0') {
#endif
    readers_data.push_back(new ReaderData(reader_iter, context));
    // Node Object:
    v8::Local<v8::External> data = Nan::New<v8::External>(readers_data.back());
    v8::Local<v8::Object> reader = Nan::New<v8::Object>();
    Nan::Set(reader, Nan::New("name").ToLocalChecked(), Nan::New(reader_iter).ToLocalChecked());
    Nan::SetMethod(reader, "listen", ReaderListen);
    Nan::SetMethod(reader, "release", ReaderRelease);
    Nan::Set(readers_local, Nan::New(reader_iter).ToLocalChecked(), reader);
    Nan::SetPrivate(reader, Nan::New("data").ToLocalChecked(), data);
#if defined(USE_LIBNFC)
    reader_iter += sizeof(nfc_connstring);
#else
    reader_iter += strlen(reader_iter)+1;
    reader_count++;
#endif
  }
  info.GetReturnValue().Set(readers_local);
  return;
}

/**
 * Node.js NaN initialization function
 **/

NAN_MODULE_INIT(init) {
  Nan::Export(target, "getReader", getReader);
  Nan::Export(target, "setSleep", mifare_set_sleep);
}

NODE_MODULE(node_mifare, init)
