// Copyright 2013, Rolf Meyer
// See LICENCE for more information
#ifndef DESFIRE_H
#define DESFIRE_H

#include <nan.h>
#include <vector>
#include <iostream>
#include <cstring>
#include <memory>

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
#include "utils.h"
#include <cstdlib>

class CardData {
  public:
    CardData(ReaderData *reader, FreefareTag *tags) : reader(reader), tags(tags) {
      uint8_t null[8] = {0,0,0,0,0,0,0,0};
      key = mifare_desfire_des_key_new(null);
      aid = mifare_desfire_aid_new(0x000001);
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
      freefare_free_tags(tags.get());
    }

    ReaderData *reader;
    FreefareTag tag;
    std::shared_ptr<FreefareTag> tags;
    MifareDESFireKey key;
    MifareDESFireAID aid;
};

inline CardData *CardData_from_info(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  CardData *data = static_cast<CardData *>(
    v8::Local<v8::External>::Cast(
      GetPrivate(info.This(), Nan::New("data").ToLocalChecked())
    )->Value()
  );

  if(!data) {
    throw errorResult(info, 0x12301, "Card is already free");
  }

  return data;
}

class GuardTag {
  public:
    GuardTag(const Nan::FunctionCallbackInfo<v8::Value> &info, bool active = false) : m_info(info), m_data(CardData_from_info(info)), m_reader(m_data->reader), m_tag(m_data->tag), m_active(false) {
      if(active) {
        guard();
      }
    }

    virtual ~GuardTag() {
      unguard();
    }

    operator CardData*() {
      return m_data;
    }

    operator ReaderData*() {
      return m_reader;
    }

    operator FreefareTag() {
      return m_tag;
    }

    unsigned int error() {
      return freefare_internal_error(m_tag);
    }

    void guard() {
      if(!m_active) {
        int res = 0;
        uv_mutex_lock(  &m_reader->mDevice);
        res = mifare_desfire_connect(m_tag);
        if(res) {
          uv_mutex_unlock(&m_reader->mDevice);
          throw errorResult(m_info, 0x12303, "Can't conntect to Mifare DESFire target.", error());
        }
        mifare_sleep();
      }
      m_active = true;
    }

    void unguard() {
      if(m_active) {
        mifare_desfire_disconnect(m_tag);
        uv_mutex_unlock(&m_reader->mDevice);
      }
      m_active = false;
    }

  private:
    const Nan::FunctionCallbackInfo<v8::Value> &m_info;
    CardData *m_data;
    ReaderData *m_reader;
    FreefareTag m_tag;
    bool m_active;
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
