// Copyright 2013, Rolf Meyer
// See LICENCE for more information

#include <string.h>

#include "reader.h"
#include "desfire.h"
#include "utils.h"

reader_data *reader_data_from_info(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  return static_cast<reader_data *>(
    v8::Local<v8::External>::Cast(
      GetPrivate(info.This(), Nan::New("data").ToLocalChecked())
    )->Value()
  );
}


#if ! defined(USE_LIBNFC)

#if NODE_VERSION_AT_LEAST(0, 12, 0)
void reader_timer_callback(uv_timer_t *handle) {
#else
void reader_timer_callback(uv_timer_t *handle, int timer_status) {
#endif
  reader_data *data = static_cast<reader_data *>(handle->data);
  LONG res;
  DWORD event;
  v8::Local<v8::String> status;
  v8::Local<v8::Object> reader = Nan::New<v8::Object>(data->self);
  reader->Set(Nan::New("name").ToLocalChecked(), Nan::New(data->name.c_str()).ToLocalChecked());

  res = SCardGetStatusChange(data->context->context, 1, &data->state, 1);
  if(res == SCARD_S_SUCCESS) {
    event = data->state.dwEventState;
    if(event & SCARD_STATE_CHANGED) {
      data->state.dwCurrentState = event;
      if(event & SCARD_STATE_IGNORE) {
        status = Nan::New("ignore").ToLocalChecked();
      } else if(event & SCARD_STATE_UNKNOWN) {
        status = Nan::New("unknown").ToLocalChecked();
      } else if(event & SCARD_STATE_UNAVAILABLE) {
        status = Nan::New("unavailable").ToLocalChecked();
      } else if(event & SCARD_STATE_EMPTY) {
        status = Nan::New("empty").ToLocalChecked();
      } else if(event & SCARD_STATE_PRESENT) {
        status = Nan::New("present").ToLocalChecked();
      } else if(event & SCARD_STATE_ATRMATCH) {
        status = Nan::New("atrmatch").ToLocalChecked();
      } else if(event & SCARD_STATE_EXCLUSIVE) {
        status = Nan::New("exclusive").ToLocalChecked();
      } else if(event & SCARD_STATE_INUSE) {
        status = Nan::New("inuse").ToLocalChecked();
      } else if(event & SCARD_STATE_MUTE) {
        status = Nan::New("mute").ToLocalChecked();
      }

      // Prepare readerObject event
      reader->Set(Nan::New("status").ToLocalChecked(), status);

      // Card object, will be eventually filled lateron
      if(event & SCARD_STATE_PRESENT) {
        if(data->state.cbAtr>0) {
        }

        // Establishes a connection to a smart card contained by a specific reader.
        FreefareTag *tags = freefare_get_tags_pcsc(data->context, data->state.szReader);
        // XXX: With PCSC tags is always length 2 with {tag, NULL} we assume this is allways the case here!!!!
        for(int i = 0; (!res) && tags && tags[i]; i++) {
          if(tags[i] && freefare_get_tag_type(tags[i]) == MIFARE_DESFIRE) {

            card_data *cardData = new card_data(data, tags);
            cardData->tag = tags[i];
            v8::Local<v8::Object> card = Nan::New<v8::Object>();
            card->Set(Nan::New("type").ToLocalChecked(), Nan::New("desfire").ToLocalChecked());
            SetPrivate(card, Nan::New("data").ToLocalChecked(), Nan::New<v8::External>(cardData));

            card->Set(Nan::New("info").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardInfo)->GetFunction());
            card->Set(Nan::New("masterKeyInfo").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardMasterKeyInfo)->GetFunction());
            card->Set(Nan::New("keyVersion").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardKeyVersion)->GetFunction());
            card->Set(Nan::New("freeMemory").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardFreeMemory)->GetFunction());
            card->Set(Nan::New("setKey").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardSetKey)->GetFunction());
            card->Set(Nan::New("setAid").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardSetAid)->GetFunction());
            card->Set(Nan::New("format").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardFormat)->GetFunction());
            card->Set(Nan::New("createNdef").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardCreateNdef)->GetFunction());
            card->Set(Nan::New("readNdef").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardReadNdef)->GetFunction());
            card->Set(Nan::New("writeNdef").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardWriteNdef)->GetFunction());
            card->Set(Nan::New("free").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardFree)->GetFunction());

            const unsigned argc = 3;
            v8::Local<v8::Value> argv[argc] = { Nan::Undefined(), reader, card };
            Nan::Call(Nan::New<v8::Function>(data->callback), Nan::GetCurrentContext()->Global(), argc, argv);

            //delete cardData;
          }
        }
        //freefare_free_tags(tags);
      } else {
        const unsigned argc = 3;
        v8::Local<v8::Value> argv[argc] = { Nan::Undefined(), reader, Nan::Undefined() };
        Nan::Call(Nan::New<v8::Function>(data->callback), Nan::GetCurrentContext()->Global(), argc, argv);
      }
    }
  } else if(static_cast<unsigned int>(res) == SCARD_E_TIMEOUT) {
      reader->Set(Nan::New("status").ToLocalChecked(), Nan::New("timeout").ToLocalChecked());
      const unsigned argc = 3;
      v8::Local<v8::Value> argv[argc] = { Nan::Undefined(), reader, Nan::Undefined() };
      Nan::Call(Nan::New<v8::Function>(data->callback), Nan::GetCurrentContext()->Global(), argc, argv);
  } else {
      reader->Set(Nan::New("status").ToLocalChecked(), Nan::New("unknown").ToLocalChecked());
      const unsigned argc = 3;
      v8::Local<v8::Value> argv[argc] = { Nan::Undefined(), reader, Nan::Undefined() };
      Nan::Call(Nan::New<v8::Function>(data->callback), Nan::GetCurrentContext()->Global(), argc, argv);
  }
}
#else // USE_LIBNFC
void reader_timer_callback(uv_timer_t *handle, int timer_status) {
  reader_data *data = static_cast<reader_data *>(handle->data);
  v8::Local<v8::String> status;
  v8::Local<v8::Object> reader = Nan::New<v8::Object>(data->self);
  GuardReader reader_guard(data, true);
  reader->Set(Nan::New("name").ToLocalChecked(), Nan::New(data->name.c_str()).ToLocalChecked());

  if (!data->device) {
    reader_guard->unlock();
    reader->Set(Nan::New("status").ToLocalChecked(), Nan::New("unavailable").ToLocalChecked());
    const unsigned argc = 3;
    v8::Local<v8::Value> argv[argc] = {
      Nan::New("No NFC device associated with this reader").ToLocalChecked(),
      reader,
      Nan::Undefined()
    };
    Nan::Call(Nan::New<v8::Function>(data->callback), Nan::GetCurrentContext()->Global(), argc, argv);
    return;
  }

  FreefareTag *tags = freefare_get_tags(data->device);
  int err = nfc_device_get_last_error(data->device);
  nfc_device_set_property_bool(data->device, NP_INFINITE_SELECT, false);
  reader_guard->unlock();
  // return on all but success cases
  // for succes, we have to distinghish between empty and present
  if (err != NFC_SUCCESS && err == data->last_err) {
    freefare_free_tags(tags);
    return;
  }
  if (err == NFC_SUCCESS && tags != NULL && tags[0] != NULL) { // found more than 0 tags -> present
    data->last_err = err;

    FreefareTag t = NULL;
    FreefareTag old_tag;
    size_t len;
    char *old_tag_uid, *t_uid;
    // we definetly found new tags if previous search did not get any
    // else we have to compare in the loop below
    bool found_new_tag = data->last_uids.size() == 0;
    for(std::vector<char *>::const_iterator i = data->last_uids.begin(); !found_new_tag && i != data->last_uids.end(); ++i) {
      found_new_tag = true; // assume tag is new
      old_tag_uid = *i;
      int tag_count = 0;
      for (t = tags[0]; t != NULL && found_new_tag; t=tags[++tag_count]) {
        t_uid = freefare_get_tag_uid(t);
        if(0 == strcmp(old_tag_uid, t_uid)) {
          found_new_tag = false; // unless it matches a known uid
        }
        free(t_uid);
      }
    }

    // XXX: What happens when an existing tag get's a new uid?!
    // this would trigger a freefare_free_tags here.
    if(!found_new_tag) {
      return;
    } else {
      for(std::vector<char *>::iterator i = data->last_uids.begin(); i != data->last_uids.end(); ++i){
        free(*i);
        *i = NULL;
      }
    }
    data->last_uids.clear();
    reader->Set(Nan::New("status").ToLocalChecked(), Nan::New("present").ToLocalChecked());

    t = NULL;
    int tag_count = 0;
    bool tags_used = false;
    smart_tags *st = new smart_tags(tags);
    for (t = tags[0]; t != NULL; t=tags[++tag_count]) {
      data->last_uids.push_back(freefare_get_tag_uid(t));
      // TODO: do something with the tag
      if(freefare_get_tag_type(t) == DESFIRE) {
        tags_used = true;

        card_data *cardData = new card_data(data, st);
        cardData->tag = t;
        Nan::Local<v8::Object> card = Nan::New<v8::Object>();
        card->Set(Nan::New("type").ToLocalChecked(), Nan::New("desfire").ToLocalChecked());
        SetPrivate(card, Nan::New("data").ToLocalChecked(), Nan::New<v8::External>(cardData);

        card->Set(Nan::New("info").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardInfo)->GetFunction());
        card->Set(Nan::New("masterKeyInfo").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardMasterKeyInfo)->GetFunction());
        card->Set(Nan::New("keyVersion").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardKeyVersion)->GetFunction());
        card->Set(Nan::New("freeMemory").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardFreeMemory)->GetFunction());
        card->Set(Nan::New("setKey").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardSetKey)->GetFunction());
        card->Set(Nan::New("setAid").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardSetAid)->GetFunction());
        card->Set(Nan::New("format").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardFormat)->GetFunction());
        card->Set(Nan::New("createNdef").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardCreateNdef)->GetFunction());
        card->Set(Nan::New("readNdef").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardReadNdef)->GetFunction());
        card->Set(Nan::New("writeNdef").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardWriteNdef)->GetFunction());
        card->Set(Nan::New("free").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CardFree)->GetFunction());

        const unsigned argc = 3;
        Nan::Local<v8::Value> argv[argc] = { Nan::Undefined(), reader, card };
        data->callback->Call(Nan::GetCurrentContext()->Global(), argc, argv);
        //delete cardData;
      }
    }
    if(!tags_used) {
      freefare_free_tags(tags);
    }
  } else { // not tag found
    data->last_err = err;
    if (err == NFC_SUCCESS) {
      if(data->last_uids.size() == 0) {
      // empty -> empty, no change
        return;
      }
      // present -> empty
      data->last_uids.clear();
      status = Nan::New("empty");
    } else if(err == NFC_EIO) {
      status = Nan::New("ioerror");
    } else if(err == NFC_EINVARG) {
      // XXX: should not happen
    } else if(err == NFC_EDEVNOTSUPP) {
      status = Nan::New("invalid");
    } else if(err == NFC_ENOTSUCHDEV) {
      status = Nan::New("invalid");
    } else if(err == NFC_ENOTIMPL) {
      status = Nan::New("invalid");
    } else if(err == NFC_EOVFLOW) {
      status = Nan::New("overflow");
    } else if(err == NFC_ETIMEOUT) {
      status = Nan::New("timeout");
    } else if(err == NFC_EOPABORTED) {
      status = Nan::New("aborted");
    } else if(err == NFC_ETGRELEASED) {
      status = Nan::New("released");
    } else if(err == NFC_ERFTRANS) {
      status = Nan::New("error");
    } else if(err == NFC_EMFCAUTHFAIL) {
      status = Nan::New("authfail");
    } else if(err == NFC_ESOFT) {
      status = Nan::New("error");
    } else if(err == NFC_ECHIP){
      status = Nan::New("brokenchip");
    }
    else
    {
      status = make_status("unknown");
    }
    reader->Set(Nan::New("status"), status.ToLocalChecked());
    /*
    Came here because err changed. So we call the callback function
    */
    const unsigned argc = 3;
    Nan::Local<Value> argv[argc] = {
      Nan::Local<v8::Value>::New(Nan::Undefined()),
      Nan::Local<v8::Value>::New(reader),
      Nan::Local<v8::Value>::New(Nan::Undefined())
    };
    data->callback->Call(Nan::GetCurrentContext()->Global(), argc, argv);
  }
  return;
}
#endif // USE_LIBNFC

void ReaderRelease(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  reader_data *data = reader_data_from_info(info);
  if(info.Length()!=0) {
    Nan::ThrowError("release does not take any arguments");
  } else {
    //if(data->timer) {
      uv_timer_stop(&data->timer);
    //}
#ifndef USE_LIBNFC
#else
    GuardReader reader_guard(data, true);
    if (data->device)
      nfc_close(data->device);
#endif
    data->callback.Reset();
    data->self.Reset();
    info.GetReturnValue().Set(info.This());
  }
}

void ReaderListen(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  reader_data *data = reader_data_from_info(info);
  if(info.Length()!=1 || !info[0]->IsFunction()) {
    Nan::ThrowError("The only argument to listen has to be a callback function");
  } else{

#ifndef USE_LIBNFC
#else
    GuardReader reader_guard(data, true);
    if (data->context && data->device == NULL)
      data->device = nfc_open(data->context, data->name.c_str());
#endif
    data->callback.Reset(Nan::Persistent<v8::Function>(info[0].As<v8::Function>()));
    data->self.Reset(Nan::Persistent<v8::Object>(info.This()));

    uv_timer_init(uv_default_loop(), &data->timer);
    uv_timer_start(&data->timer, reader_timer_callback, 500, 250);
    info.GetReturnValue().Set(info.This());
  }
}

#if defined (_WIN32)
#  define IOCTL_CCID_ESCAPE_SCARD_CTL_CODE SCARD_CTL_CODE(3500)
#elif defined(__APPLE__)
#  include <wintypes.h>
#  define IOCTL_CCID_ESCAPE_SCARD_CTL_CODE (((0x31) << 16) | ((3500) << 2))
#elif defined (__FreeBSD__) || defined (__OpenBSD__)
#  define IOCTL_CCID_ESCAPE_SCARD_CTL_CODE (((0x31) << 16) | ((3500) << 2))
#elif defined (__linux__)
#  include <reader.h>
// Escape IOCTL tested successfully:
#  define IOCTL_CCID_ESCAPE_SCARD_CTL_CODE SCARD_CTL_CODE(1)
#else
#    error "Can't determine serial string for your system"
#endif
#include <iomanip>
void ReaderSetLed(const Nan::FunctionCallbackInfo<v8::Value>& info) {
#ifndef USE_LIBNFC
  const uint32_t sSize = 9;
  uint8_t sBuffer[sSize] = {0xFF, 0x00, 0x40, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00};
  uint8_t rBuffer[262];
  const uint8_t pos[5] = {3, 5, 6, 7, 8};
  DWORD rSize = 262;
  SCARDHANDLE hCard;
  DWORD dwActiveProtocol;
  LONG rv;

  if(info.Length()==0 || info.Length() > 5) {
    Nan::ThrowError("This function must have up to 5 unsigned chars as arguments");
  } else {

    for(int i = 0; i < info.Length(); i++) {
      sBuffer[pos[i]] = info[i]->ToUint32()->Value();
    }

    reader_data *data = reader_data_from_info(info);
    rv = SCardConnect(data->context->context, data->name.c_str(), SCARD_SHARE_DIRECT, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
    //rv = SCardControl(hCard, IOCTL_CCID_ESCAPE_SCARD_CTL_CODE, sBuffer, sSize, rBuffer, rSize, &rLength);
    int retCode = SCardTransmit(hCard, NULL, sBuffer, sSize, NULL, rBuffer, &rSize);
    std::cout << "rv: " << std::hex << rv << std::endl;
    std::cout << "sent: "
              << std::hex << std::setw(2) << std::setfill('0')
              << (int)sBuffer[0] << " "
              << std::hex << std::setw(2) << std::setfill('0')
              << (int)sBuffer[1] << " "
              << std::hex << std::setw(2) << std::setfill('0')
              << (int)sBuffer[2] << " "
              << std::hex << std::setw(2) << std::setfill('0')
              << (int)sBuffer[3] << " "
              << std::hex << std::setw(2) << std::setfill('0')
              << (int)sBuffer[4] << " "
              << std::hex << std::setw(2) << std::setfill('0')
              << (int)sBuffer[5] << " "
              << std::hex << std::setw(2) << std::setfill('0')
              << (int)sBuffer[6] << " "
              << std::hex << std::setw(2) << std::setfill('0')
              << (int)sBuffer[7] << " "
              << std::hex << std::setw(2) << std::setfill('0')
              << (int)sBuffer[8] << " "
              << std::endl;
    std::cout << "retCode: " << std::hex << retCode << std::endl;
    rv = SCardDisconnect(hCard, SCARD_LEAVE_CARD);

#endif
    info.GetReturnValue().Set(info.This());
  }
}

