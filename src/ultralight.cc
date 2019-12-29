// Copyright 2019, Rolf Meyer
// See LICENCE for more information

#include "ultralight.h"
#include "utils.h"

v8::Local<v8::Object> UltralightCreate(ReaderData *reader, FreefareTag *tagList, FreefareTag activeTag) {
  UltralightData *cardData = new UltralightData(reader, tagList);
  cardData->tag = activeTag;
  v8::Local<v8::Object> card = Nan::New<v8::Object>();
  Nan::Set(card, Nan::New("type").ToLocalChecked(), Nan::New("ultralight").ToLocalChecked());
  Nan::SetPrivate(card, Nan::New("data").ToLocalChecked(), Nan::New<v8::External>(cardData));

  Nan::SetMethod(card, "info", UltralightInfo);
  //Nan::SetMethod(card, "freeMemory", CardFreeMemory);
  //Nan::SetMethod(card, "format", CardFormat);
  //Nan::SetMethod(card, "createNdef", CardCreateNdef);
  //Nan::SetMethod(card, "readNdef", CardReadNdef);
  //Nan::SetMethod(card, "writeNdef", CardWriteNdef);
  Nan::SetMethod(card, "free", UltralightFree);
  return card;
}

void UltralightInfo(const Nan::FunctionCallbackInfo<v8::Value> &v8info) {
  try {
    v8::Local<v8::Object> card = Nan::New<v8::Object>();
    char *uid_c;

    if(v8info.Length()!=0) {
      throw errorResult(v8info, 0x12302, "This function takes no arguments");
    }
    { // Guarded realm;
      UltralightGuardTag tag(v8info);
      tag.retry(0x12304, "Fetch Tag Version Info",
                [&]()mutable->res_t{uid_c = freefare_get_tag_uid (tag); return 0;});
    }

    v8::Local<v8::Array> uid = Nan::New<v8::Array>(7);
    for(unsigned int j=0; j<7; j++) {
      uid->Set(j, Nan::New(uid_c[j]));
    }
    card->Set(Nan::New("uid").ToLocalChecked(), uid);

    v8::Local<v8::Array> bno = Nan::New<v8::Array>(5);
    for(unsigned int j=7; j<14; j++) {
      bno->Set(j-7, Nan::New(uid_c[j]));
    }
    card->Set(Nan::New("batchNumber").ToLocalChecked(), bno);
    v8info.GetReturnValue().Set(card);
    free(uid_c);
  } catch(MifareError err) {
    // The error is already assigned to the InfoScope
  }
}

void UltralightName(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  try {
    if(info.Length()!=0) {
      throw errorResult(info, 0x12302, "This function takes no arguments");
    }

    UltralightGuardTag tag(info);
    info.GetReturnValue().Set(Nan::New(tag.name()).ToLocalChecked());
  } catch(MifareError err) {
    // The error is already assigned to the InfoScope
  }
}

void UltralightFreeMemory(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  try {
    //uint32_t size;
    if(info.Length()!=0) {
      throw errorResult(info, 0x12302, "This function takes no arguments");
    }

    info.GetReturnValue().Set(Nan::New(0));
  } catch(MifareError err) {
    // The error is already assigned to the InfoScope
  }
}

void UltralightFree(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  try {
    UltralightData *data = UltralightData_from_info(info);
    if(!data) {
      throw errorResult(info, 0x12322, "Card is already free");
    }

    if(info.Length()!=0) {
      throw errorResult(info, 0x12321, "This function takes no arguments");
    }

    delete data;
    data = NULL;
    validTrue(info);
  } catch(MifareError err) {
    // The error is already assigned to the InfoScope
  }
}

