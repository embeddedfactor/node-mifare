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
#ifndef USE_LIBNFC
static pcsc_context *context;
#else
#include <signal.h>
static nfc_context *context = NULL;
void deinitialize_nfc_context(int p) {
  if (context)
    nfc_exit(context);
}
#endif
static std::vector<reader_data *> readers_data;
static Nan::Persistent<v8::Object> readers_global(Nan::New<v8::Object>());


/**
 * Get Names of the Readers connected to the computer
 * @param hContext The SCard Context used to search
 * @return An Array of Strings with reader names
 **/
void getReader(const Nan::FunctionCallbackInfo<v8::Value>& info) {
#ifndef USE_LIBNFC
  LONG res;
  char *reader_names;
#else
  size_t numDevices;
  const size_t MAX_READERS = 16;
  nfc_connstring reader_names[MAX_READERS];
#endif
  char *reader_iter = NULL;
  int   reader_count = 0;
  // Allocate buffer. We assume autoallocate is not present (on Mac OS X anyway)
  v8::Local<v8::Object> readers_local = Nan::New<v8::Object>(readers_global);

  if(info.Length() > 0){
    Nan::ThrowError("This function does not take any arguments");
    return;
  }

#ifndef USE_LIBNFC
  res = pcsc_list_devices(context, &reader_names);
  if(res != SCARD_S_SUCCESS || reader_names[0] == '\0') {
    //delete [] reader_names;
    Nan::ThrowError("Unable to list readers");
    return;
  }
#else
  numDevices = nfc_list_devices(context, reader_names, MAX_READERS);
  if(numDevices == 0) {
    info.GetReturnValue().Set(Nan::New<v8::Object>(readers_global));
    return;
  }
#endif

  // Clean before use
  for(std::vector<reader_data *>::iterator iter;iter!=readers_data.end();iter++) {
    reader_data *data = *iter;
#ifndef USE_LIBNFC
    delete [] data->state.szReader;
#else
    if (data->device)
      nfc_close(data->device);
#endif
    delete data;
  }
  readers_data.clear();

  // Get number of readers from null separated string
#ifndef USE_LIBNFC
  reader_iter = reader_names;
  while(*reader_iter != '\0') {
    readers_data.push_back(new reader_data(reader_iter, context));
#else
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
    readers_data.push_back(new reader_data(reader_iter, context, NULL));
#endif
    // Node Object:
    v8::Local<v8::External> data = Nan::New<v8::External>(readers_data.back());
    v8::Local<v8::Object> reader = Nan::New<v8::Object>();
    reader->Set(Nan::New("name").ToLocalChecked(), Nan::New(reader_iter).ToLocalChecked());
    reader->Set(Nan::New("listen").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(ReaderListen)->GetFunction());
    reader->Set(Nan::New("setLed").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(ReaderSetLed)->GetFunction());
    reader->Set(Nan::New("release").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(ReaderRelease)->GetFunction());
    reader->SetHiddenValue(Nan::New("data").ToLocalChecked(), data);
    readers_local->Set(Nan::New(reader_iter).ToLocalChecked(), reader);
#ifndef USE_LIBNFC
    reader_iter += strlen(reader_iter)+1;
    reader_count++;
#else
    reader_iter += sizeof(nfc_connstring);
#endif
  }
  //delete [] reader_names;

  info.GetReturnValue().Set(readers_local);
  return;
}

/**
 * Node.js initialization function
 * @param exports The Commonjs module exports object
 **/

void init(v8::Local<v8::Object> exports) {
#ifndef USE_LIBNFC
  pcsc_init(&context);
#else
  nfc_init(&context);
#endif
  if(!context) {
    Nan::ThrowError("Cannot establish context");
    return;
  }

  exports->Set(Nan::New("getReader").ToLocalChecked(),
      Nan::New<v8::FunctionTemplate>(getReader)->GetFunction());
  exports->Set(Nan::New("setSleep").ToLocalChecked(),
      Nan::New<v8::FunctionTemplate>(mifare_set_sleep)->GetFunction());
	// AtExit(&PCSC::close);
}

NODE_MODULE(node_mifare, init)
