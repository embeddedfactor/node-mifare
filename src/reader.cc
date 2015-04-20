// Copyright 2013, Rolf Meyer
// See LICENCE for more information

#include <string.h>

#include "reader.h"
#include "desfire.h"

#ifndef USE_LIBNFC
void reader_timer_callback(uv_timer_t *handle, int timer_status) {
  HandleScope scope;
  reader_data *data = static_cast<reader_data *>(handle->data);
  LONG res;
  DWORD event;
  Local<String> status;
  Local<Object> reader = Local<Object>::New(data->self);
  reader->Set(String::NewSymbol("name"), String::New(data->name.c_str()));

  res = SCardGetStatusChange(data->context->context, 1, &data->state, 1);
  if(res == SCARD_S_SUCCESS) {
    event = data->state.dwEventState;
    if(event & SCARD_STATE_CHANGED) {
      data->state.dwCurrentState = event;
      if(event & SCARD_STATE_IGNORE) {
        status = v8::String::New("ignore");
      } else if(event & SCARD_STATE_UNKNOWN) {
        status = v8::String::New("unknown");
      } else if(event & SCARD_STATE_UNAVAILABLE) {
        status = v8::String::New("unavailable");
      } else if(event & SCARD_STATE_EMPTY) {
        status = v8::String::New("empty");
      } else if(event & SCARD_STATE_PRESENT) {
        status = v8::String::New("present");
      } else if(event & SCARD_STATE_ATRMATCH) {
        status = v8::String::New("atrmatch");
      } else if(event & SCARD_STATE_EXCLUSIVE) {
        status = v8::String::New("exclusive");
      } else if(event & SCARD_STATE_INUSE) {
        status = v8::String::New("inuse");
      } else if(event & SCARD_STATE_MUTE) {
        status = v8::String::New("mute");
      }

      // Prepare readerObject event
      reader->Set(String::NewSymbol("status"), v8::Local<v8::Value>::New(status));

      // Card object, will be eventually filled lateron
      if(event & SCARD_STATE_PRESENT) {
        if(data->state.cbAtr>0) {
        }

        // Establishes a connection to a smart card contained by a specific reader.
        MifareTag *tags = freefare_get_tags_pcsc(data->context, data->state.szReader);
        // XXX: With PCSC tags is always length 2 with {tag, NULL} we assume this is allways the case here!!!!
        smart_tags *st = new smart_tags(tags);
        for(int i = 0; (!res) && tags[i]; i++) {
          if(tags[i] && freefare_get_tag_type(tags[i]) == DESFIRE) {

            card_data *cardData = new card_data(data, st);
            cardData->tag = tags[i];
            Local<Object> card = Object::New();
            card->Set(String::NewSymbol("type"), String::New("desfire"));
            card->SetHiddenValue(String::NewSymbol("data"), External::Wrap(cardData));

            card->Set(String::NewSymbol("info"), FunctionTemplate::New(CardInfo)->GetFunction());
            card->Set(String::NewSymbol("masterKeyInfo"), FunctionTemplate::New(CardMasterKeyInfo)->GetFunction());
            card->Set(String::NewSymbol("keyVersion"), FunctionTemplate::New(CardKeyVersion)->GetFunction());
            card->Set(String::NewSymbol("freeMemory"), FunctionTemplate::New(CardFreeMemory)->GetFunction());
            card->Set(String::NewSymbol("setKey"), FunctionTemplate::New(CardSetKey)->GetFunction());
            card->Set(String::NewSymbol("setAid"), FunctionTemplate::New(CardSetAid)->GetFunction());
            card->Set(String::NewSymbol("format"), FunctionTemplate::New(CardFormat)->GetFunction());
            card->Set(String::NewSymbol("createNdef"), FunctionTemplate::New(CardCreateNdef)->GetFunction());
            card->Set(String::NewSymbol("readNdef"), FunctionTemplate::New(CardReadNdef)->GetFunction());
            card->Set(String::NewSymbol("writeNdef"), FunctionTemplate::New(CardWriteNdef)->GetFunction());
            card->Set(String::NewSymbol("free"), FunctionTemplate::New(CardFree)->GetFunction());

            const unsigned argc = 3;
            Local<Value> argv[argc] = {
              Local<Value>::New(Undefined()),
              Local<Value>::New(reader),
              Local<Value>::New(card)
            };
            data->callback->Call(Context::GetCurrent()->Global(), argc, argv);

            //delete cardData;
          }
        }
        //freefare_free_tags(tags);
      } else {
        const unsigned argc = 3;
        Local<Value> argv[argc] = {
          Local<Value>::New(Undefined()),
          Local<Value>::New(reader),
          Local<Value>::New(Undefined())
        };
        data->callback->Call(Context::GetCurrent()->Global(), argc, argv);
      }
    }
  } else if(static_cast<unsigned int>(res) == SCARD_E_TIMEOUT) {
      reader->Set(String::NewSymbol("status"), v8::Local<v8::Value>::New(v8::String::New("timeout")));
      const unsigned argc = 3;
      Local<Value> argv[argc] = {
        Local<Value>::New(Undefined()),
        Local<Value>::New(reader),
        Local<Value>::New(Undefined())
      };
      data->callback->Call(Context::GetCurrent()->Global(), argc, argv);
  } else {
      reader->Set(String::NewSymbol("status"), v8::Local<v8::Value>::New(v8::String::New("unknown")));
      const unsigned argc = 3;
      Local<Value> argv[argc] = {
        Local<Value>::New(Undefined()),
        Local<Value>::New(reader),
        Local<Value>::New(Undefined())
      };
      data->callback->Call(Context::GetCurrent()->Global(), argc, argv);
  }
}
#else // USE_LIBNFC
inline Local<String> make_status(const char* const s) {
  return v8::String::New(s);
}
void reader_timer_callback(uv_timer_t *handle, int timer_status) {
  HandleScope scope;
  reader_data *data = static_cast<reader_data *>(handle->data);
  Local<String> status;
  Local<Object> reader = Local<Object>::New(data->self);
  reader->Set(String::NewSymbol("name"), String::New(data->name.c_str()));
  pthread_mutex_lock(&data->mDevice);
  if (!data->device) {
    pthread_mutex_unlock(&data->mDevice);
    reader->Set(String::NewSymbol("status"), v8::Local<v8::Value>::New(make_status("unavailable")));
    const unsigned argc = 3;
    Local<Value> err = String::New("No NFC device associated with this reader");
    Local<Value> argv[argc] = {
      Local<Value>::New(err),
      Local<Value>::New(reader),
      Local<Value>::New(Undefined())
    };
    data->callback->Call(Context::GetCurrent()->Global(), argc, argv);
    return;
  }

  MifareTag *tags = freefare_get_tags(data->device);
  int err = nfc_device_get_last_error(data->device);
  pthread_mutex_unlock(&data->mDevice);
  // return on all but success cases
  // for succes, we have to distinghish between empty and present
  if (err != NFC_SUCCESS && err == data->last_err) {
    freefare_free_tags(tags);
    scope.Close(Undefined());
    return;
  }
  if (err == NFC_SUCCESS && tags != NULL && tags[0] != NULL) { // found more than 0 tags -> present
    data->last_err = err;

    MifareTag t = NULL;
    MifareTag old_tag;
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
      scope.Close(Undefined());
      return;
    } else {
      for(std::vector<char *>::iterator i = data->last_uids.begin(); i != data->last_uids.end(); ++i){
        free(*i);
        *i = NULL;
      }
    }
    data->last_uids.clear();
    reader->Set(String::NewSymbol("status"), v8::Local<v8::Value>::New(make_status("present")));

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
        Local<Object> card = Object::New();
        card->Set(String::NewSymbol("type"), String::New("desfire"));
        card->SetHiddenValue(String::NewSymbol("data"), External::Wrap(cardData));

        card->Set(String::NewSymbol("info"), FunctionTemplate::New(CardInfo)->GetFunction());
        card->Set(String::NewSymbol("masterKeyInfo"), FunctionTemplate::New(CardMasterKeyInfo)->GetFunction());
        card->Set(String::NewSymbol("keyVersion"), FunctionTemplate::New(CardKeyVersion)->GetFunction());
        card->Set(String::NewSymbol("freeMemory"), FunctionTemplate::New(CardFreeMemory)->GetFunction());
        card->Set(String::NewSymbol("setKey"), FunctionTemplate::New(CardSetKey)->GetFunction());
        card->Set(String::NewSymbol("setAid"), FunctionTemplate::New(CardSetAid)->GetFunction());
        card->Set(String::NewSymbol("format"), FunctionTemplate::New(CardFormat)->GetFunction());
        card->Set(String::NewSymbol("createNdef"), FunctionTemplate::New(CardCreateNdef)->GetFunction());
        card->Set(String::NewSymbol("readNdef"), FunctionTemplate::New(CardReadNdef)->GetFunction());
        card->Set(String::NewSymbol("writeNdef"), FunctionTemplate::New(CardWriteNdef)->GetFunction());
        card->Set(String::NewSymbol("free"), FunctionTemplate::New(CardFree)->GetFunction());

        const unsigned argc = 3;
        Local<Value> argv[argc] = {
          Local<Value>::New(Undefined()),
          Local<Value>::New(reader),
          Local<Value>::New(card)
        };
        data->callback->Call(Context::GetCurrent()->Global(), argc, argv);
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
        scope.Close(Undefined());
        return;
      }
      // present -> empty
      data->last_uids.clear();
      status = make_status("empty");
    } else if(err == NFC_EIO) {
      status = make_status("ioerror");
    } else if(err == NFC_EINVARG) {
      // XXX: should not happen
    } else if(err == NFC_EDEVNOTSUPP) {
      status = make_status("invalid");
    } else if(err == NFC_ENOTSUCHDEV) {
      status = make_status("invalid");
    } else if(err == NFC_ENOTIMPL) {
      status = make_status("invalid");
    } else if(err == NFC_EOVFLOW) {
      status = make_status("overflow");
    } else if(err == NFC_ETIMEOUT) {
      status = make_status("timeout");
    } else if(err == NFC_EOPABORTED) {
      status = make_status("aborted");
    } else if(err == NFC_ETGRELEASED) {
      status = make_status("released");
    } else if(err == NFC_ERFTRANS) {
      status = make_status("error");
    } else if(err == NFC_EMFCAUTHFAIL) {
      status = make_status("authfail");
    } else if(err == NFC_ESOFT) {
      status = make_status("error");
    } else if(err == NFC_ECHIP){
      status = make_status("brokenchip");
    }
    else
    {
      status = make_status("unknown");
    }
    reader->Set(String::NewSymbol("status"), v8::Local<v8::Value>::New(status));
    /*
    Came here because err changed. So we call the callback function
    */
    const unsigned argc = 3;
    Local<Value> argv[argc] = {
      Local<Value>::New(Undefined()),
      Local<Value>::New(reader),
      Local<Value>::New(Undefined())
    };
    data->callback->Call(Context::GetCurrent()->Global(), argc, argv);
  }
  scope.Close(Undefined());
  return;
}
#endif // USE_LIBNFC

Handle<Value> ReaderRelease(const Arguments &args) {
  HandleScope scope;
  Local<Object> self = args.This();
  reader_data *data = static_cast<reader_data *>(External::Unwrap(self->GetHiddenValue(String::NewSymbol("data"))));
  if(args.Length()!=0) {
    ThrowException(Exception::TypeError(String::New("release does not take any arguments")));
    return scope.Close(Undefined());
  }

  //if(data->timer) {
    uv_timer_stop(&data->timer);
  //}
#ifndef USE_LIBNFC
  SCardReleaseContext(data->context->context);
#else
  pthread_mutex_lock(&data->mDevice);
  if (data->device)
    nfc_close(data->device);
  pthread_mutex_unlock(&data->mDevice);
#endif
  data->callback.Dispose();
  data->callback.Clear();
  data->self.Dispose();
  data->self.Clear();
  return scope.Close(args.This()); 
}

Handle<Value> ReaderListen(const Arguments& args) {
  HandleScope scope;
  Local<Object> self = args.This();
  reader_data *data = static_cast<reader_data *>(External::Unwrap(self->GetHiddenValue(String::NewSymbol("data"))));
  if(args.Length()!=1 || !args[0]->IsFunction()) {
    ThrowException(Exception::TypeError(String::New("The only argument to listen has to be a callback function")));
    return scope.Close(Undefined());
  }

#ifndef USE_LIBNFC
#else
  pthread_mutex_lock(&data->mDevice);
  if (data->context && data->device == NULL)
    data->device = nfc_open(data->context, data->name.c_str());
  pthread_mutex_unlock(&data->mDevice);

#endif
  data->callback = Persistent<Function>::New(Local<Function>::Cast(args[0]));
  data->self = Persistent<Object>::New(self);

  uv_timer_init(uv_default_loop(), &data->timer);
  uv_timer_start(&data->timer, reader_timer_callback, 500, 250);
  return scope.Close(args.This()); 
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
Handle<Value> ReaderSetLed(const Arguments& args) {
  HandleScope scope;
  Local<Object> self = args.This();
#ifndef USE_LIBNFC
  const uint32_t sSize = 9;
  uint8_t sBuffer[sSize] = {0xFF, 0x00, 0x40, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00};
  uint8_t rBuffer[262];
  const uint8_t pos[5] = {3, 5, 6, 7, 8};
  DWORD rSize = 262;
  uint32_t rLength = 0;
  SCARDHANDLE hCard;
  DWORD dwActiveProtocol;
  LONG rv;

  if(args.Length()==0 || args.Length() > 5) {
    ThrowException(Exception::TypeError(String::New("This function must have up to 5 unsigned chars as arguments")));
    return scope.Close(Undefined());
  }

  for(int i = 0; i < args.Length(); i++) {
    sBuffer[pos[i]] = args[i]->ToUint32()->Value();
  }

  reader_data *data = static_cast<reader_data *>(External::Unwrap(self->GetHiddenValue(String::NewSymbol("data"))));
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
  return scope.Close(args.This()); 
}

