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
struct SmartTags {
  SmartTags(FreefareTag *tags) {
    this->tags = tags;
    this->cards_using_this_instance = 0;
    uv_mutex_init(&this->m);
  };
  ~SmartTags() {
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

struct CardData {
  CardData(ReaderData *reader,
#if defined(USE_LIBNFC)
    SmartTags *t
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

  ~CardData() {
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

  ReaderData *reader;
  FreefareTag tag;
#if defined(USE_LIBNFC)
  SmartTags *tags;
#else
  FreefareTag *tags;
#endif
  MifareDESFireKey key;
  MifareDESFireAID aid;
};


CardData *CardData_from_info(const Nan::FunctionCallbackInfo<v8::Value> &info);

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
int CardReadNdefTVL(const Nan::FunctionCallbackInfo<v8::Value> &info, CardData *data, uint8_t &file_no, uint16_t &ndefmaxlen, MifareDESFireKey key_app);

void CardReadNdef(const Nan::FunctionCallbackInfo<v8::Value> &info);

void CardWriteNdef(const Nan::FunctionCallbackInfo<v8::Value> &info);

void CardCreateNdef(const Nan::FunctionCallbackInfo<v8::Value> &info);

void CardFree(const Nan::FunctionCallbackInfo<v8::Value> &info);

#endif // DESFIRE_H
