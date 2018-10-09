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
#include <functional>

/* A data object collecting all interesting details for a tag */
class CardData {
  public:
    /* The data object is created from a reader data object and a freefare tag object */
    CardData(ReaderData *reader, FreefareTag *tags) : reader(reader), tags(tags) {
      uint8_t null[8] = {0,0,0,0,0,0,0,0};
      key = mifare_desfire_des_key_new(null);
      aid = mifare_desfire_aid_new(0x000001);
    }

    /* If destroyed it will free the tags as well */
    ~CardData() {
      if(key) {
        mifare_desfire_key_free(key);
        key = NULL;
      }
      if(aid) {
        free(aid);
        aid = NULL;
      }
      if(tag) {
        freefare_free_tags(tags);
      }
      tag = NULL;
      tags = NULL;
    }

    ReaderData *reader;
    FreefareTag tag;
    // The shared pointer makes problems. tags is not an C++ object and needs an special destructor
    //std::shared_ptr<FreefareTag> tags;
    FreefareTag *tags;
    MifareDESFireKey key;
    MifareDESFireAID aid;
};

/* Extracts Tag data object from nodejs info context */
inline CardData *CardData_from_info(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  CardData *data = static_cast<CardData *>(
    v8::Local<v8::External>::Cast(
      Nan::GetPrivate(info.This(), Nan::New("data").ToLocalChecked()).ToLocalChecked()
    )->Value()
  );

  if(!data) {
    throw errorResult(info, 0x12301, "Card is already free");
  }
  return data;
}

/* GuardTag a wrapper class for FreefareTag which can be used transperent and is also a scope guard for the connection to the card */
class GuardTag {
  public:
    /* Constructor. Guards the tag imideatly if active is true.
     * Extracts the tag, reader and data from the info object.
     * It also provides a retry function which executes a closure/lamda.
     * On negative result an error is detected and the the internal error state of the card reader service is read.
     * In case of communication error the closure is reexecuted n tries on other error an exeption is thrown. */
    GuardTag(const Nan::FunctionCallbackInfo<v8::Value> &info, bool active = true)
      : m_info(info), m_data(CardData_from_info(info)), m_reader(m_data->reader), m_active(false) {
      // We store the m_data->reader pointer as m_reader in case m_data is destroyed for some reason.
      if(active) {
        guard();
      }
    }

    /* Destructor. Unguards the tag. */
    virtual ~GuardTag() {
      unguard();
    }

    /* Returns the card data */
    CardData* data() {
      return m_data;
    }

    /* Returns the reader which read the tag */
    ReaderData* reader() {
      return m_reader;
    }

    /* The guard wrapps a FreefareTag and is implicite usable as one */
    operator FreefareTag() {
      return m_data->tag;
    }

    /* Retry a closure/lambda n times and throw an error on failiur with pos_code and name */
    res_t retry(unsigned int pos_code, const char *name, std::function<res_t ()> try_f, int tries=3) {
      res_t ret_code = 0;
      unsigned int int_code = 0;
      while(tries>0) {
        mifare_clear_internal_error(m_data->tag);
        ret_code = try_f();
        int_code = error();
        --tries;
        if(ret_code>=0) {
          return ret_code;
        } else { // ERROR ret is negative
          if(int_code==28) {
            // ILLEGAL_COMMAND: Propably due to to short time for initialization
            continue;
          } else if(int_code==0x80100010) {
            // CMD ERROR: Propably due to to short time for initialization
            continue;
          } else {
            throw errorResult(m_info, pos_code, errorString(), int_code, name);
          }
        }
      }
      return ret_code;
    }

    /* Return the friendly name of the tag */
    const char *name() {
      if(m_data) {
        return freefare_get_tag_friendly_name(m_data->tag);
      } else {
        return "UNKNOWN";
      }
    }

    /* Returns the error number of the underlying service */
    unsigned int error() {
      if(m_data) {
        return freefare_internal_error(m_data->tag);
      } else {
        return 0;
      }
    }

    /* Returns the error string od the underlying service */
    const char *errorString() {
      if(m_data) {
        return freefare_strerror(m_data->tag);
      } else {
        return "data struct is NULL";
      }
    }

    /* Lock cardreader for exclusive access for threads inside this app and connect to card if possible
     * Throws error on failiur */
    void guard() {
      if(!m_active) {
        int res = 0;
        uv_mutex_lock(  &m_reader->mDevice);
        if(m_data) {
          while(1) {
            mifare_clear_internal_error(m_data->tag);
            res = mifare_desfire_connect(m_data->tag);
            /*if(res==240) { // ERROR_VC_DISCONNECTED - Card needs reconnect
              res = mifare_desfire_reconnect(m_data->tag);
            }*/
            if(res && error() == 0x8010000B) {
              // SCARD_E_SHARING_VIOLATION
              // The smart card cannot be accessed because of other connections outstanding
              mifare_sleep();
              continue;
            } else if(res) {
              uv_mutex_unlock(&m_reader->mDevice);
              throw errorResult(m_info, 0x12303, errorString(), error(), "Can't conntect to Mifare DESFire target.");
              break;
            } else {
              break;
            }
          }
        }
        mifare_sleep();
      }
      m_active = true;
    }

    /* Unlocks card reader after exclusive access and disconnects from card if possible
     * Will allways success (Ignores errors) */
    void unguard() {
      if(m_active) {
        if(m_data&&m_data->tag) {
          mifare_desfire_disconnect(m_data->tag);
        }
        uv_mutex_unlock(&m_reader->mDevice);
      }
      m_active = false;
    }

  private:
    const Nan::FunctionCallbackInfo<v8::Value> &m_info;
    CardData *m_data;
    ReaderData *m_reader;
    bool m_active;
};

v8::Local<v8::Object> CardCreate(ReaderData *reader, FreefareTag *tagList, FreefareTag activeTag);

/** Extract tag data from info nodejs info object */
CardData *CardData_from_info(const Nan::FunctionCallbackInfo<v8::Value> &info);

/** Get tag information as an javascript object */
void CardInfo(const Nan::FunctionCallbackInfo<v8::Value> &info);

/** Get master key information from tag as an javascript object */
void CardMasterKeyInfo(const Nan::FunctionCallbackInfo<v8::Value> &info);

/** Return readable name of the card */
void CardName(const Nan::FunctionCallbackInfo<v8::Value> &info);

/** Read version information from the card */
void CardKeyVersion(const Nan::FunctionCallbackInfo<v8::Value> &info);

/** Free card data memory and destroy card */
void CardFreeMemory(const Nan::FunctionCallbackInfo<v8::Value> &info);

/** Set AID for tag access. */
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

void CardFree(const Nan::FunctionCallbackInfo<v8::Value> &info);

#endif // DESFIRE_H
