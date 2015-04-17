// Copyright 2013, Rolf Meyer
// See LICENCE for more information
#ifndef DESFIRE_H
#define DESFIRE_H

#include <node.h>
#include <v8.h>
#include <vector>
#include <iostream>
#include <cstring>

#if defined(__APPLE__) || defined(__linux__)
#include <PCSC/winscard.h>
#include <PCSC/wintypes.h>
#else
#include <winscard.h>
#endif
#include <freefare_pcsc.h>

#include "reader.h"
#include <cstdlib>
#include <pthread.h>

using namespace v8;
using namespace node;

struct smart_tags {
  smart_tags(MifareTag *tags) {
    this->tags = tags;
    this->cards_using_this_instance = 0;
    pthread_mutex_init(&this->m, NULL);
  };
  ~smart_tags() {
    freefare_free_tags(this->tags);
    pthread_mutex_destroy(&this->m);
  };
  void lock() {
    pthread_mutex_lock(&this->m);
  };
  void unlock() {
    pthread_mutex_unlock(&this->m);
  };
  unsigned int cards_using_this_instance;
  MifareTag *tags;
  pthread_mutex_t m;
};

struct card_data {
  card_data(reader_data *reader, smart_tags *t) : reader(reader) {
    uint8_t null[8] = {0,0,0,0,0,0,0,0};
    key = mifare_desfire_des_key_new(null);
    aid = mifare_desfire_aid_new(0x000001);
    this->tags = t;
    t->cards_using_this_instance++;
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
    if(this->tags) {
      this->tags->lock();
      int tags_ref_count = --this->tags->cards_using_this_instance;
      this->tags->unlock();
      if(tags_ref_count == 0){
        delete this->tags;
      }
    }
  }
  
  reader_data *reader;
  MifareTag tag;
  smart_tags *tags;
  MifareDESFireKey key;
  MifareDESFireAID aid;
};


Handle<Value> CardInfo(const Arguments& args);

Handle<Value> CardMasterKeyInfo(const Arguments& args);

Handle<Value> CardName(const Arguments& args);

Handle<Value> CardKeyVersion(const Arguments& args);

Handle<Value> CardFreeMemory(const Arguments& args);

Handle<Value> CardSetAid(const Arguments& args);

Handle<Value> CardSetKey(const Arguments & args);

Handle<Value> CardFormat(const Arguments& args);

Handle<Value> CardCreateNdef(const Arguments &args);

/**
 * Helper function to locate and read TVL of a desfire ndef sector
 * @return Might return a result object. This is only used when res is lesser 0 otherwise the object is empty.
 */
Local<Object> CardReadNdefTVL(card_data *data, int &res, uint8_t &file_no, uint16_t &ndefmaxlen, MifareDESFireKey key_app);

Handle<Value> CardReadNdef(const Arguments& args);

Handle<Value> CardWriteNdef(const Arguments& args);

Handle<Value> CardCreateNdef(const Arguments& args);

Handle<Value> CardFree(const Arguments& args);

#endif // DESFIRE_H
