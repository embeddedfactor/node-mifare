// Copyright 2013, Rolf Meyer
// See LICENCE for more information

#include <node.h>
#include <v8.h>
#include <node_buffer.h>
#include <vector>
#include <iostream>
#include <cstring>


#include "reader.h"

using namespace v8;
using namespace node;

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

/**
 * Get Names of the Readers connected to the computer
 * @param hContext The SCard Context used to search
 * @return An Array of Strings with reader names
 **/
Handle<Value> getReader(const Arguments& args) {
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

  HandleScope scope;
  Persistent<Object> readers;

  if(args.Length() > 0){
    ThrowException(Exception::TypeError(String::New("This function does not take any arguments")));
    return scope.Close(Undefined());
  }

  // Allocate buffer. We assume autoallocate is not present (on Mac OS X anyway)
  readers = Persistent<Object>::New(Object::New());
#ifndef USE_LIBNFC
  res = pcsc_list_devices(context, &reader_names);
  if(res != SCARD_S_SUCCESS || reader_names[0] == '\0') {
    //delete [] reader_names;
    ThrowException(Exception::Error(String::New("Unable to list readers")));
    return scope.Close(Undefined());
  }
#else
  numDevices = nfc_list_devices(context, reader_names, MAX_READERS);
  if(numDevices == 0) {
    return scope.Close(readers);
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
    Local<External> data = Local<External>::New(External::New(readers_data.back()));
    Local<Object> reader = Local<Object>::New(Object::New());
    reader->Set(String::NewSymbol("name"), String::New(reader_iter));
    reader->Set(String::NewSymbol("listen"), FunctionTemplate::New(ReaderListen)->GetFunction());
    reader->Set(String::NewSymbol("setLed"), FunctionTemplate::New(ReaderSetLed)->GetFunction());
    reader->Set(String::NewSymbol("release"), FunctionTemplate::New(ReaderRelease)->GetFunction());
    reader->SetHiddenValue(String::NewSymbol("data"), data);
    readers->Set(String::NewSymbol(reader_iter), reader);
#ifndef USE_LIBNFC
    reader_iter += strlen(reader_iter)+1;
    reader_count++;
#else
    reader_iter += sizeof(nfc_connstring);
#endif
  }
  //delete [] reader_names;

  return scope.Close(readers); 
}

/**
 * Node.js initialization function
 * @param exports The Commonjs module exports object
 **/

void init(Handle<Object> exports) {
#ifndef USE_LIBNFC
  pcsc_init(&context);
#else
  nfc_init(&context);
  if (signal(SIGTERM, deinitialize_nfc_context) != SIG_IGN)
    signal(SIGABRT, deinitialize_nfc_context);
  if (signal(SIGHUP, deinitialize_nfc_context) != SIG_IGN)
    signal(SIGHUP, deinitialize_nfc_context);
  if (signal(SIGINT, deinitialize_nfc_context) != SIG_IGN)
    signal(SIGINT, deinitialize_nfc_context);
#endif
  if(!context) {
    ThrowException(Exception::Error(String::New("Cannot establish context")));
    return; 
  }

  exports->Set(String::NewSymbol("getReader"),
      FunctionTemplate::New(getReader)->GetFunction());
	// AtExit(&PCSC::close);
}

NODE_MODULE(node_mifare, init)
