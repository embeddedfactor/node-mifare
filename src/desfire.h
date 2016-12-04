// Copyright 2013, Rolf Meyer
// See LICENCE for more information
#ifndef DESFIRE_H
#define DESFIRE_H

#include <nan.h>
#include <vector>
#include <iostream>
#include <cstring>

#if ! defined(USE_LIBNFC)
#if defined(__APPLE__) || defined(__linux__)
#include <PCSC/winscard.h>
#include <PCSC/wintypes.h>
#else
#include <winscard.h>
#endif
#include <freefare_pcsc.h>
#else
#include <freefare_nfc.h>
#endif // ! USE_LIBNFC

#include "reader.h"
#include <cstdlib>

#if defined(USE_LIBNFC)
struct smart_tags {
  smart_tags(FreefareTag *tags) {
    this->tags = tags;
    this->cards_using_this_instance = 0;
    uv_mutex_init(&this->m);
  };
  ~smart_tags() {
    freefare_free_tags(this->tags);
    uv_mutex_destroy(&this->m);
  };
  void lock() {
    uv_mutex_lock(&this->m);
  };
  void unlock() {
    uv_mutex_unlock(&this->m);
  };
  unsigned int aquire() {
    return ++cards_using_this_instance;
  }
  unsigned int release() {
    lock();
    unsigned int tags_ref_count = --cards_using_this_instance;
    unlock();
    if(tags_ref_count == 0) {
      delete tags;
      tags = NULL;
    }
    return tags_ref_count;
  }
  unsigned int cards_using_this_instance;
  FreefareTag *tags;
  uv_mutex_t m;
};
#endif

struct card_data {
  card_data(reader_data *reader,
#if defined(USE_LIBNFC)
    smart_tags *t
#else
    FreefareTag *t
#endif
    ) : reader(reader) {
    uint8_t null[8] = {0,0,0,0,0,0,0,0};
    key = mifare_desfire_des_key_new(null);
    aid = mifare_desfire_aid_new(0x000001);
    tags = t;
#if defined(USE_LIBNFC)
    t->aquire();
#endif
  }

  ~card_data() {
    if(key) {
      mifare_desfire_key_free(key);
      key = NULL;
    }
    if(aid) {
      free(aid);
      aid = NULL;
    }
    if(tags) {
#if defined(USE_LIBNFC)
      tags->release();
#else
      freefare_free_tags(tags);
#endif
    }
  }

  reader_data *reader;
  FreefareTag tag;
#if defined(USE_LIBNFC)
  smart_tags *tags;
#else
  FreefareTag *tags;
#endif
  MifareDESFireKey key;
  MifareDESFireAID aid;
};


card_data *card_data_from_info(const Nan::FunctionCallbackInfo<v8::Value> &info);

void CardInfo(const Nan::FunctionCallbackInfo<v8::Value> &info);

void CardMasterKeyInfo(const Nan::FunctionCallbackInfo<v8::Value> &info);

void CardName(const Nan::FunctionCallbackInfo<v8::Value> &info);

void CardKeyVersion(const Nan::FunctionCallbackInfo<v8::Value> &info);

void CardFreeMemory(const Nan::FunctionCallbackInfo<v8::Value> &info);

void CardSetAid(const Nan::FunctionCallbackInfo<v8::Value> &info);

void CardSetKey(const Nan::FunctionCallbackInfo<v8::Value> &info);

void CardFormat(const Nan::FunctionCallbackInfo<v8::Value> &info);

void CardCreateNdef(const Nan::FunctionCallbackInfo<v8::Value> &info);

/**
 * Helper function to locate and read TVL of a desfire ndef sector
 * @return Might return a result object. This is only used when res is lesser 0 otherwise the object is empty.
 */
int CardReadNdefTVL(const Nan::FunctionCallbackInfo<v8::Value> &info, card_data *data, uint8_t &file_no, uint16_t &ndefmaxlen, MifareDESFireKey key_app);

void CardReadNdef(const Nan::FunctionCallbackInfo<v8::Value> &info);

void CardWriteNdef(const Nan::FunctionCallbackInfo<v8::Value> &info);

void CardCreateNdef(const Nan::FunctionCallbackInfo<v8::Value> &info);

void CardFree(const Nan::FunctionCallbackInfo<v8::Value> &info);

#endif // DESFIRE_H
