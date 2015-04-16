// Copyright 2013, Rolf Meyer
// See LICENCE for more information

#ifndef MIFARE_H
#define MIFARE_H

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


/**
 * Get Names of the Readers connected to the computer
 * @param hContext The SCard Context used to search
 * @return An Array of Strings with reader names
 **/
Handle<Value> getReader(const Arguments& args);

/**
 * Node.js initialization function
 * @param exports The Commonjs module exports object
 **/
void init(Handle<Object> exports);

#endif // MIFARE_H
