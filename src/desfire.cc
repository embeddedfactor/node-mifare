// Copyright 2013, Rolf Meyer
// See LICENCE for more information

#include "desfire.h"
#include "utils.h"

v8::Local<v8::Object> DesfireCreate(ReaderData *reader, FreefareTag *tagList, FreefareTag activeTag) {
  DesfireData *cardData = new DesfireData(reader, tagList);
  cardData->tag = activeTag;
  v8::Local<v8::Object> card = Nan::New<v8::Object>();
  Nan::Set(card, Nan::New("type").ToLocalChecked(), Nan::New("desfire").ToLocalChecked());
  Nan::SetPrivate(card, Nan::New("data").ToLocalChecked(), Nan::New<v8::External>(cardData));

  Nan::SetMethod(card, "info", DesfireInfo);
  Nan::SetMethod(card, "masterKeyInfo", DesfireMasterKeyInfo);
  Nan::SetMethod(card, "keyVersion", DesfireKeyVersion);
  Nan::SetMethod(card, "freeMemory", DesfireFreeMemory);
  Nan::SetMethod(card, "setKey", DesfireSetKey);
  Nan::SetMethod(card, "setAid", DesfireSetAid);
  Nan::SetMethod(card, "format", DesfireFormat);
  Nan::SetMethod(card, "createNdef", DesfireCreateNdef);
  Nan::SetMethod(card, "readNdef", DesfireReadNdef);
  Nan::SetMethod(card, "writeNdef", DesfireWriteNdef);
  Nan::SetMethod(card, "free", DesfireFree);
  return card;
}

void DesfireInfo(const Nan::FunctionCallbackInfo<v8::Value> &v8info) {
  try {
    v8::Local<v8::Object> card = Nan::New<v8::Object>();
    struct mifare_desfire_version_info info;

    if(v8info.Length()!=0) {
      throw errorResult(v8info, 0x12302, "This function takes no arguments");
    }
    { // Guarded realm;
      DesfireGuardTag tag(v8info);
      tag.retry(0x12304, "Fetch Tag Version Info",
                [&]()mutable->res_t{return mifare_desfire_get_version(tag, &info);});
    }

    v8::Local<v8::Array> uid = Nan::New<v8::Array>(7);
    for(unsigned int j=0; j<7; j++) {
      uid->Set(j, Nan::New(info.uid[j]));
    }
    card->Set(Nan::New("uid").ToLocalChecked(), uid);

    v8::Local<v8::Array> bno = Nan::New<v8::Array>(5);
    for(unsigned int j=0; j<5; j++) {
      bno->Set(j, Nan::New(info.batch_number[j]));
    }
    card->Set(Nan::New("batchNumber").ToLocalChecked(), bno);

    v8::Local<v8::Object> pdate = Nan::New<v8::Object>();
    pdate->Set(Nan::New("week").ToLocalChecked(), Nan::New(info.production_week));
    pdate->Set(Nan::New("year").ToLocalChecked(), Nan::New(info.production_year));
    card->Set(Nan::New("production").ToLocalChecked(), pdate);

    v8::Local<v8::Object> hardware = Nan::New<v8::Object>();
    hardware->Set(Nan::New("vendorId").ToLocalChecked(), Nan::New(info.hardware.vendor_id));
    hardware->Set(Nan::New("type").ToLocalChecked(), Nan::New(info.hardware.type));
    hardware->Set(Nan::New("subtype").ToLocalChecked(), Nan::New(info.hardware.subtype));

    v8::Local<v8::Object> hw_version = Nan::New<v8::Object>();
    hw_version->Set(Nan::New("major").ToLocalChecked(), Nan::New(info.hardware.version_major));
    hw_version->Set(Nan::New("minor").ToLocalChecked(), Nan::New(info.hardware.version_minor));
    hardware->Set(Nan::New("version").ToLocalChecked(), hw_version);
    hardware->Set(Nan::New("storageSize").ToLocalChecked(), Nan::New(info.hardware.storage_size));
    hardware->Set(Nan::New("protocol").ToLocalChecked(), Nan::New(info.hardware.protocol));
    card->Set(Nan::New("hardware").ToLocalChecked(), hardware);

    v8::Local<v8::Object> software = Nan::New<v8::Object>();
    software->Set(Nan::New("vendorId").ToLocalChecked(), Nan::New(info.software.vendor_id));
    software->Set(Nan::New("type").ToLocalChecked(), Nan::New(info.software.type));
    software->Set(Nan::New("subtype").ToLocalChecked(), Nan::New(info.software.subtype));

    v8::Local<v8::Object> sw_version = Nan::New<v8::Object>();
    sw_version->Set(Nan::New("major").ToLocalChecked(), Nan::New(info.software.version_major));
    sw_version->Set(Nan::New("minor").ToLocalChecked(), Nan::New(info.software.version_minor));
    software->Set(Nan::New("version").ToLocalChecked(), sw_version);

    software->Set(Nan::New("storageSize").ToLocalChecked(), Nan::New(info.software.storage_size));
    software->Set(Nan::New("protocol").ToLocalChecked(), Nan::New(info.software.protocol));
    card->Set(Nan::New("software").ToLocalChecked(), software);

    v8info.GetReturnValue().Set(card);
  } catch(MifareError err) {
    // The error is already assigned to the InfoScope
  }
}

void DesfireMasterKeyInfo(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  try {
    res_t res;
    uint8_t settings;
    uint8_t max_keys;

    if(info.Length()!=0) {
      throw errorResult(info, 0x12306, "This function takes no arguments");
    }
    DesfireGuardTag tag_guard(info);

    res = mifare_desfire_get_key_settings(tag_guard, &settings, &max_keys);
    if(!res) {
      v8::Local<v8::Object> key = Nan::New<v8::Object>();
      key->Set(Nan::New("configChangable").ToLocalChecked(), Nan::New((settings & 0x08)!=0));
      key->Set(Nan::New("freeCreateDelete").ToLocalChecked(), Nan::New((settings & 0x04)!=0));
      key->Set(Nan::New("freeDirectoryList").ToLocalChecked(), Nan::New((settings & 0x02)!=0));
      key->Set(Nan::New("keyChangable").ToLocalChecked(), Nan::New((settings & 0x01)!=0));
      key->Set(Nan::New("maxKeys").ToLocalChecked(), Nan::New((max_keys)));
      return info.GetReturnValue().Set(key);
    } else if (AUTHENTICATION_ERROR == mifare_desfire_last_picc_error(tag_guard)) {
      throw errorResult(info, 0x12307, "LOCKED", tag_guard.error());
    } else {
      throw errorResult(info, 0x12307, freefare_strerror(tag_guard), tag_guard.error());
    }
  } catch(MifareError err) {
    // The error is already assigned to the InfoScope
  }
}

void DesfireName(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  try {
    if(info.Length()!=0) {
      throw errorResult(info, 0x12302, "This function takes no arguments");
    }

    DesfireGuardTag tag(info);
    info.GetReturnValue().Set(Nan::New(tag.name()).ToLocalChecked());
  } catch(MifareError err) {
    // The error is already assigned to the InfoScope
  }
}

void DesfireKeyVersion(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  try {
    uint8_t version;
    if(info.Length()!=1 || !info[0]->IsNumber()) {
      throw errorResult(info, 0x12302, "This function takes a key number as arguments");
    }
    DesfireGuardTag tag(info);
    tag.retry(0x12308, "Fetch Tag Version Information",
              [&]()mutable->res_t{ return mifare_desfire_get_key_version(tag, Nan::To<uint32_t>(info[0]).FromJust(), &version);});
    info.GetReturnValue().Set(Nan::New(version));
  } catch(MifareError err) {
    // The error is already assigned to the InfoScope
  }
}

void DesfireFreeMemory(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  try {
    uint32_t size;
    if(info.Length()!=0) {
      throw errorResult(info, 0x12302, "This function takes no arguments");
    }

    DesfireGuardTag tag(info);
    tag.retry(0x12309, "Free Memory",
              [&]()mutable->res_t{return mifare_desfire_free_mem(tag, &size);});
    info.GetReturnValue().Set(Nan::New(size));
  } catch(MifareError err) {
    // The error is already assigned to the InfoScope
  }
}

void DesfireSetAid(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  try {
    DesfireData *data = DesfireData_from_info(info);
    if(!data) {
      throw errorResult(info, 0x12301, "Card is already free");
    }

    if(info.Length()!=1 || !info[0]->IsNumber() || Nan::To<uint32_t>(info[0]).FromJust() > 0xFFFFFF) {
      throw errorResult(info, 0x12302, "This function takes the aid as argument a number smaller than 0x1000000");
    }
    if(data->aid) {
      free(data->aid);
    }
    data->aid = mifare_desfire_aid_new(Nan::To<uint32_t>(info[0]).FromJust());
    validTrue(info);
  } catch(MifareError err) {
    // The error is already assigned to the InfoScope
  }
}

void DesfireSetKey(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  try {
    typedef MifareDESFireKey (*callback_t)(const uint8_t *);
    callback_t callbacks[4][2] = {
      {mifare_desfire_des_key_new, mifare_desfire_des_key_new_with_version},
      {mifare_desfire_3des_key_new, mifare_desfire_3des_key_new_with_version},
      {mifare_desfire_3k3des_key_new, mifare_desfire_3k3des_key_new_with_version},
      {mifare_desfire_aes_key_new, NULL}
    };

    DesfireData *data = DesfireData_from_info(info);
    if(!data) {
      throw errorResult(info, 0x12301, "Card is already free");
    }

    uint8_t key_val[24];
    bool version = false;
    //uint8_t aes_ver = 0;
    int type = 0;
    uint32_t max_len = 8;
    std::string error = (
      "This function takes up to four arguments. "
      "The first is the key an must be an array of 8, 16 or 24 bytes, depending on the type. "
      "The second is the type a string out of (des|3des|3k3des|aes) which defines also the length of the key. "
      "If you choose \"des\" the key is 8 numbers long. If you choose 3des or aes it is 16 numbers long. "
      "For 3k3des it has to be 24 numbers long. it will default to \"des\"."
      "The third is version a boolean describing wether the key is versioned. It will default to false"
      "If you choose to use an \"aes\" key the fourth argument is the aes version a number smaller 255"
    );
    if(info.Length()==0 || info.Length()>4 || !info[0]->IsArray() ||
        (info.Length()>1 && !info[1]->IsString()) ||
        (info.Length()>2 && !info[2]->IsBoolean()) ||
        (info.Length()>3 && !info[3]->IsUint32()) ||
        (info.Length()>3 && Nan::To<uint32_t>(info[3]).FromJust()>255)
      ) {
      throw errorResult(info, 0x12302, error);
    }

    if(info.Length()>2) {
      version = info[2]->BooleanValue();
    }

    if(info.Length()>1) {
      v8::Local<v8::String> t = v8::Local<v8::String>::Cast(info[1]);
      if(t->Equals(Nan::New("aes").ToLocalChecked())) {
        type = 3;
        max_len = 16;
      } else if(t->Equals(Nan::New("3k3des").ToLocalChecked())) {
        type = 2;
        max_len = 24;
      } else if(t->Equals(Nan::New("3des").ToLocalChecked())) {
        type = 1;
        max_len = 16;
      }
    }

    //if(type == 3 && info.Length() > 3) {
    //  aes_ver = (uint8_t)(Nan::To<uint32_t>(info[3]).FromJust()&0xFF);
    //}

    if(info[0]->IsArray()) {
      v8::Local<v8::Array> key = v8::Local<v8::Array>::Cast(info[0]);
      if(key->Length()!=max_len) {
        throw errorResult(info, 0x12302, error);
      }
      for(uint32_t i=0; i<max_len; i++) {
        v8::Local<v8::Value> k = key->Get(i);
        if(!k->IsInt32()) {
          throw errorResult(info, 0x12302, error);
        }
        if(Nan::To<int32_t>(k).FromJust()>255) {
          throw errorResult(info, 0x12302, error);
        }
        key_val[i] = (uint8_t)(Nan::To<int32_t>(k).FromJust() & 0xFF);
      }
      if(data->key) {
        mifare_desfire_key_free(data->key);
      }
      callback_t cb = callbacks[type][version];
      if(cb) {
        data->key = cb(key_val);
      } else {
        mifare_desfire_aes_key_new_with_version(key_val, 0);
      }
    }
    for(uint32_t i = 0; i < max_len; i++) {
      key_val[i] = 0;
    }

    info.GetReturnValue().Set(info.This());
  } catch(MifareError err) {
    // The error is already assigned to the InfoScope
  }
}

void DesfireFormat(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  try {
    uint8_t key_data_picc[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    if(info.Length()>1 || (info.Length()==1 && !info[0]->IsObject())) {
      throw errorResult(info, 0x12302, "The only argument is an options object with members: {configChangable:bool, freeCreateDelete:bool, freeDirectoryList:bool, keyChangable:bool}");
    }
    // Send Mifare DESFire ChangeKeySetting to change the PICC master key settings into :
    // bit7-bit4 equal to 0000b
    // bit3 equal to 1b, the configuration of the PICC master key MAY be changeable or frozen
    // bit2 equal to 1b, CreateApplication and DeleteApplication commands are allowed without PICC master key authentication
    // bit1 equal to 1b, GetApplicationIDs, and GetKeySettings are allowed without PICC master key authentication
    // bit0 equal to 1b, PICC masterkey MAY be frozen or changeable
    v8::Local<v8::Object> options = info.Length()==1? v8::Local<v8::Object>::Cast(info[0]) : Nan::New<v8::Object>();
    bool configChangable = options->Has(Nan::New("configChangeable").ToLocalChecked()) ?
      options->Get(Nan::New("configChangable").ToLocalChecked())->IsTrue() :
      true;
    bool freeCreateDelete = options->Has(Nan::New("freeCreateDelete").ToLocalChecked()) ?
      options->Get(Nan::New("freeCreateDelete").ToLocalChecked())->IsTrue() :
      true;
    bool freeDirectoryList = options->Has(Nan::New("freeDirectoryList").ToLocalChecked()) ?
      options->Get(Nan::New("freeDirectoryList").ToLocalChecked())->IsTrue() :
      true;
    bool keyChangable = options->Has(Nan::New("keyChangable").ToLocalChecked()) ?
      options->Get(Nan::New("keyChangable").ToLocalChecked())->IsTrue() :
      true;
    uint8_t flags = (configChangable << 3) | (freeCreateDelete << 2) | (freeDirectoryList << 1) | (keyChangable << 0);

    DesfireGuardTag tag(info);
    MifareDESFireKey key_picc = mifare_desfire_des_key_new_with_version(key_data_picc);
    tag.retry(0x12310, "Authenticate on Mifare DESFire target",
              [&]()mutable->res_t{return mifare_desfire_authenticate(tag, 0, key_picc);});
    mifare_desfire_key_free(key_picc);
    mifare_sleep();
    tag.retry(0x12311, "Change Key Settings",
              [&]()mutable->res_t{return mifare_desfire_change_key_settings(tag, flags);});
    mifare_sleep();
    tag.retry(0x12312, "Format PICC",
              [&]()mutable->res_t{return mifare_desfire_format_picc(tag);});
    validTrue(info);
  } catch(MifareError err) {
    // The error is already assigned to the InfoScope
  }
}

void DesfireCreateNdef(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  try {
    uint8_t ndef_read_key[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t *key_data_picc = ndef_read_key;
    uint8_t *key_data_app = ndef_read_key;

    if(info.Length()>=1) {
      throw errorResult(info, 0x12302, "This function takes no arguments");
    }

    DesfireGuardTag tag(info);
    struct mifare_desfire_version_info cardinfo;
    tag.retry(0x12304, "Fetch Version Infomation",
              [&]()mutable->res_t{return mifare_desfire_get_version(tag, &cardinfo);});

    int ndef_mapping;
    switch(cardinfo.software.version_major) {
    case 0: {
        ndef_mapping = 1;
      } break;
    case 1:
    default: // newer version? let's assume it supports latest mapping too
        ndef_mapping = 2;
    }

    /* Initialised Formatting Procedure. See section 6.5.1 and 8.1 of Mifare DESFire as Type 4 Tag document*/
    // Send Mifare DESFire Select Application with AID equal to 000000h to select the PICC level
    tag.retry(0x12313, "Select Application",
              [&]()mutable->res_t{return mifare_desfire_select_application(tag, NULL);});

    MifareDESFireKey key_picc;
    MifareDESFireKey key_app;
    key_picc = mifare_desfire_des_key_new_with_version(key_data_picc);
    key_app = mifare_desfire_des_key_new_with_version(key_data_app);

    // Authentication with PICC master key MAY be needed to issue ChangeKeySettings command
    tag.retry(0x12310, "Authentication with PICC master key",
              [&]()mutable->res_t{return mifare_desfire_authenticate(tag, 0, key_picc);});

    MifareDESFireAID aid;
    if(ndef_mapping == 1) {
      uint8_t key_settings;
      uint8_t max_keys;
      mifare_desfire_get_key_settings(tag, &key_settings, &max_keys);
      if((key_settings & 0x08) == 0x08) {

        // Send Mifare DESFire ChangeKeySetting to change the PICC master key settings into :
        // bit7-bit4 equal to 0000b
        // bit3 equal to Xb, the configuration of the PICC master key MAY be changeable or frozen
        // bit2 equal to 0b, CreateApplication and DeleteApplication commands are allowed with PICC master key authentication
        // bit1 equal to 0b, GetApplicationIDs, and GetKeySettings are allowed with PICC master key authentication
        // bit0 equal to Xb, PICC masterkey MAY be frozen or changeable
        tag.retry(0x12311, "Change Key Settings",
                  [&]()mutable->res_t{return mifare_desfire_change_key_settings(tag, 0x09);});
      }

      // Mifare DESFire Create Application with AID equal to EEEE10h, key settings equal to 0x09, NumOfKeys equal to 01h
      aid = mifare_desfire_aid_new(0xEEEE10);
      tag.retry(0x12314, "Application creation (Try format before running create if failing)",
                [&]()mutable->res_t{return mifare_desfire_create_application(tag, aid, 0x09, 1);});
      // Mifare DESFire SelectApplication (Select previously creates application)
      tag.retry(0x12313, "Application selection",
                [&]()mutable->res_t{return mifare_desfire_select_application(tag, aid);});
      free(aid);

      // Authentication with NDEF Tag Application master key (Authentication with key 0)
      tag.retry(0x12310, "Authentication with NDEF Tag Application master key",
                [&]()mutable->res_t{return mifare_desfire_authenticate(tag, 0, key_app);});

      // Mifare DESFire ChangeKeySetting with key settings equal to 00001001b
      tag.retry(0x12311, "Change Key Settings",
                [&]()mutable->res_t{return mifare_desfire_change_key_settings(tag, 0x09);});

      // Mifare DESFire CreateStdDataFile with FileNo equal to 03h (CC File DESFire FID), ComSet equal to 00h,
      // AccesRights equal to E000h, File Size bigger equal to 00000Fh
      tag.retry(0x12315, "Create StDataFile",
                [&]()mutable->res_t{return mifare_desfire_create_std_data_file(tag, 0x03, MDCM_PLAIN, 0xE000, 0x00000F);});

      // Mifare DESFire WriteData to write the content of the CC File with CClEN equal to 000Fh,
      // Mapping Version equal to 10h,MLe equal to 003Bh, MLc equal to 0034h, and NDEF File Control TLV
      // equal to T =04h, L=06h, V=E1 04 (NDEF ISO FID=E104h) 0E E0 (NDEF File size =3808 Bytes) 00 (free read access)
      // 00 free write access
      uint8_t capability_container_file_content[15] = {
        0x00, 0x0F,     // CCLEN: Size of this capability container.CCLEN values are between 000Fh and FFFEh
        0x10,           // Mapping version
        0x00, 0x3B,     // MLe: Maximum data size that can be read using a single ReadBinary command. MLe = 000Fh-FFFFh
        0x00, 0x34,     // MLc: Maximum data size that can be sent using a single UpdateBinary command. MLc = 0001h-FFFFh
        0x04, 0x06,     // T & L of NDEF File Control TLV, followed by 6 bytes of V:
        0xE1, 0x04,     //   File Identifier of NDEF File
        0x0E, 0xE0,     //   Maximum NDEF File size of 3808 bytes
        0x00,           //   free read access
        0x00            //   free write acces
      };

      tag.retry(0x12316, "Write CC file content",
                [&]()mutable->res_t{return mifare_desfire_write_data(tag, 0x03, 0, sizeof(capability_container_file_content), capability_container_file_content);});

      // Mifare DESFire CreateStdDataFile with FileNo equal to 04h (NDEF FileDESFire FID), CmmSet equal to 00h, AccessRigths
      // equal to EEE0h, FileSize equal to 000EE0h (3808 Bytes)
      tag.retry(0x12317, "Create StdDataFile",
                [&]()mutable->res_t{return mifare_desfire_create_std_data_file(tag, 0x04, MDCM_PLAIN, 0xEEE0, 0x000EE0);});
    } else if(ndef_mapping == 2) {
      // Mifare DESFire Create Application with AID equal to 000001h, key settings equal to 0x0F, NumOfKeys equal to 01h,
      // 2 bytes File Identifiers supported, File-ID equal to E110h
      aid = mifare_desfire_aid_new(0x000001);
      uint8_t app[] = { 0xd2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01 };
      tag.retry(0x12314, "Application Creation",
                [&]()mutable->res_t{return mifare_desfire_create_application_iso(tag, aid, 0x0F, 0x21, 0, 0xE110, app, sizeof(app));});

      // Mifare DESFire SelectApplication (Select previously creates application)
      tag.retry(0x12313, "Application Selection",
                [&]()mutable->res_t{return mifare_desfire_select_application(tag, aid);});
      free(aid);

      // Authentication with NDEF Tag Application master key (Authentication with key 0)
      tag.retry(0x12310, "Authentication with NDEF Tag Application master key",
                [&]()mutable->res_t{return mifare_desfire_authenticate(tag, 0, key_app);});

      // Mifare DESFire CreateStdDataFile with FileNo equal to 01h (DESFire FID), ComSet equal to 00h,
      // AccesRights equal to E000h, File Size bigger equal to 00000Fh, ISO File ID equal to E103h
      tag.retry(0x12316, "Create StdDataFileIso",
                [&]()mutable->res_t{return mifare_desfire_create_std_data_file_iso(tag, 0x01, MDCM_PLAIN, 0xE000, 0x00000F, 0xE103);});

      // Mifare DESFire WriteData to write the content of the CC File with CClEN equal to 000Fh,
      // Mapping Version equal to 20h,MLe equal to 003Bh, MLc equal to 0034h, and NDEF File Control TLV
      // equal to T =04h, L=06h, V=E1 04 (NDEF ISO FID=E104h) 0xNNNN (NDEF File size = 0x0800/0x1000/0x1E00 bytes)
      // 00 (free read access) 00 free write access
      uint8_t capability_container_file_content[15] = {
        0x00, 0x0F,     // CCLEN: Size of this capability container.CCLEN values are between 000Fh and FFFEh
        0x20,           // Mapping version
        0x00, 0x3B,     // MLe: Maximum data size that can be read using a single ReadBinary command. MLe = 000Fh-FFFFh
        0x00, 0x34,     // MLc: Maximum data size that can be sent using a single UpdateBinary command. MLc = 0001h-FFFFh
        0x04, 0x06,     // T & L of NDEF File Control TLV, followed by 6 bytes of V:
        0xE1, 0x04,     //   File Identifier of NDEF File
        0x04, 0x00,     //   Maximum NDEF File size of 1024 bytes
        0x00,           //   free read access
        0x00            //   free write acces
      };

      uint16_t ndef_max_size = 0x0800;
      uint16_t announcedsize = 1 << (cardinfo.software.storage_size >> 1);
      if(announcedsize >= 0x1000) {
        ndef_max_size = 0x1000;
      }
      if(announcedsize >= 0x1E00) {
        ndef_max_size = 0x1E00;
      }
      capability_container_file_content[11] = ndef_max_size >> 8;
      capability_container_file_content[12] = ndef_max_size & 0xFF;
      tag.retry(0x12317, "Write CC file content",
                [&]()mutable->res_t{return mifare_desfire_write_data(tag, 0x01, 0, sizeof(capability_container_file_content), capability_container_file_content);});

      // Mifare DESFire CreateStdDataFile with FileNo equal to 02h (DESFire FID), CmmSet equal to 00h, AccessRigths
      // equal to EEE0h, FileSize equal to ndefmaxsize (0x000800, 0x001000 or 0x001E00)
      tag.retry(0x12318, "Create StdDataFileIso",
                [&]()mutable->res_t{return mifare_desfire_create_std_data_file_iso(tag, 0x02, MDCM_PLAIN, 0xEEE0, ndef_max_size, 0xE104);});
    }
    mifare_desfire_key_free(key_picc);
    mifare_desfire_key_free(key_app);

    validTrue(info);
  } catch(MifareError err) {
    // The error is already assigned to the InfoScope
  }
}



int DesfireReadNdefTVL(const Nan::FunctionCallbackInfo<v8::Value> &v8info, DesfireGuardTag &tag, uint8_t &file_no, uint16_t &ndef_max_len, MifareDESFireKey key_app) {
  int version;
  res_t res;
  uint8_t *cc_data;
  // #### Get Version
  // We've to track DESFire version as NDEF mapping is different
  struct mifare_desfire_version_info info;
  tag.retry(0x12304, "Fetch Tag Version Info",
            [&]()mutable->res_t{return mifare_desfire_get_version(tag, &info);});
  version = info.software.version_major;

  // ### Select app
  // Mifare DESFire SelectApplication (Select application)
  MifareDESFireAID aid;
  if(version == 0) {
      aid = mifare_desfire_aid_new(0xEEEE10);
  } else {
      // There is no more relationship between DESFire AID and ISO AID...
      // Let's assume it's in AID 000001h as proposed in the spec
      aid = mifare_desfire_aid_new(0x000001);
  }
  tag.retry(0x12313, "Application selection (NDEF application)",
            [&]()mutable->res_t{return mifare_desfire_select_application(tag, aid);});
  free(aid);

  // ### Authentication
  // NDEF Tag Application master key (Authentication with key 0)
  tag.retry(0x12310, "Authentication with NDEF Tag Application master key",
            [&]()mutable->res_t{return mifare_desfire_authenticate(tag, 0, key_app);});

  // ### Read index
  // Read Capability Container file E103
  uint8_t lendata[22]; // cf FIXME in mifare_desfire.c read_data()
  res = tag.retry(0x12320, "Reading the ndef capability container file length",
                  [&]()mutable->res_t{if(version == 0) {
                          return mifare_desfire_read_data(tag, 0x03, 0, 2, lendata);
                        } else {
                          // There is no more relationship between DESFire FID and ISO FileID...
                          // Let's assume it's in FID 01h as proposed in the spec
                          return mifare_desfire_read_data(tag, 0x01, 0, 2, lendata);
                        }});

  if(res > 2) {
    throw errorResult(v8info, 0x12320, "Reading the ndef capability container file length to long");
  }
  uint32_t cclen = (((uint16_t)lendata[0]) << 8) + ((uint16_t)lendata[1]);
  if(cclen < 15) {
    throw errorResult(v8info, 0x12321, "The read ndef capability container file (E103) is to short");
  }
  if(!(cc_data = new uint8_t[cclen + 20])) { // cf FIXME in mifare_desfire.c read_data()
    throw errorResult(v8info, 0x12322, "Allocation of ndef capability container file (E103) failed");
  }
  res = tag.retry(0x12320, "Reading the ndef capability container file",
                  [&]()mutable->res_t{if(version == 0) {
                          return mifare_desfire_read_data(tag, 0x03, 0, cclen, cc_data);
                        } else {
                          return mifare_desfire_read_data(tag, 0x01, 0, cclen, cc_data);
                        }});
  // Search NDEF File Control TLV
  uint32_t off = 7;
  while(((off + 7) < cclen) && (cc_data[off] != 0x04)) {
      off += cc_data[off + 1] + 2; // Skip TLV entry
  }

  if(off + 7 >= cclen) {
    throw errorResult(v8info, 0x12323, "We've reached the end of the ndef capability container file (E103) and did not find the ndef TLV");
  }
  if(cc_data[off + 2] != 0xE1) {
    throw errorResult(v8info, 0x12324, "Found unknown ndef file reference");
  }

  // ### Get file
  if(version == 0) {
      file_no = cc_data[off + 3];
  } else{
      // There is no more relationship between DESFire FID and ISO FileID...
      // Let's assume it's in FID 02h as proposed in the spec
      file_no = 2;
  }
  // Swap endianess
  ndef_max_len = (((uint16_t)cc_data[off + 4]) << 8) + ((uint16_t)cc_data[off + 5]);
  delete [] cc_data;
  return 0;
}

void DesfireReadNdef(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  try {
    res_t res;
    uint8_t file_no;
    uint16_t ndef_msg_len_max;
    uint16_t ndef_msg_len;
    uint8_t *ndef_msg;
    uint8_t ndef_read_key[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    if(info.Length()!=0) {
      throw errorResult(info, 0x12302, "This function does not take any arguments");
    }
    DesfireGuardTag tag(info);
    MifareDESFireKey key_app;
    key_app = mifare_desfire_des_key_new_with_version(ndef_read_key);
    res = DesfireReadNdefTVL(info, tag, file_no, ndef_msg_len_max, key_app);
    mifare_desfire_key_free(key_app);
    if(!(ndef_msg = new uint8_t[ndef_msg_len_max + 20])) { // cf FIXME in mifare_desfire.c read_data()
      throw errorResult(info, 0x12325, "Allocation of ndef file failed");
    }
    uint8_t lendata[20]; // cf FIXME in mifare_desfire.c read_data()
    tag.retry(0x12326, "Reading of NDEF file",
              [&]()mutable->res_t{return mifare_desfire_read_data(tag, file_no, 0, 2, lendata);});
    ndef_msg_len = (((uint16_t)lendata[0]) << 8) + ((uint16_t)lendata[1]); // uint16_t endianess swap
    if(ndef_msg_len + 2 > ndef_msg_len_max) {
      throw errorResult(info, 0x12327, "Declared ndef size larger than max ndef size");
    }
    if(ndef_msg_len == 0) {
      throw errorResult(info, 0x12332, "Declared ndef size is zero last write was faulty");
    }
    res = tag.retry(0x12326, "Reading NDEF message faild",
                    [&]()mutable->res_t{return mifare_desfire_read_data(tag, file_no, 2, ndef_msg_len, ndef_msg);});
    if(res != ndef_msg_len){
      throw errorResult(info, 0x12329, "Reading full ndef message failed");
    }
    v8::Local<v8::Object> result = buffer(ndef_msg, ndef_msg_len);
    result->Set(Nan::New("maxLength").ToLocalChecked(), Nan::New(ndef_msg_len_max));
    if(ndef_msg) {
      delete [] ndef_msg;
    }
    validResult(info, result);
  } catch(MifareError err) {
    // The error is already assigned to the InfoScope
  }
}

void DesfireWriteNdef(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  try {
    res_t res;
    uint8_t file_no;
    uint8_t  ndef_read_key[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint16_t ndef_msg_len;
    uint16_t ndef_msg_len_zero = 0;
    uint16_t ndef_msg_len_max;
    uint8_t  ndef_msg_len_bigendian[2];
    uint8_t *ndef_msg;
    v8::Local<v8::Object> result = Nan::New<v8::Object>();
    if(info.Length()!=1 || !node::Buffer::HasInstance(info[0])) {
      throw errorResult(info, 0x12302, "This function takes a buffer to write to a tag");
    }
    ndef_msg_len = node::Buffer::Length(info[0]);
    ndef_msg = reinterpret_cast<uint8_t *>(node::Buffer::Data(info[0]));
    DesfireGuardTag tag(info);

    MifareDESFireKey key_app;
    key_app = mifare_desfire_des_key_new_with_version(ndef_read_key);

    DesfireReadNdefTVL(info, tag, file_no, ndef_msg_len_max, key_app);
    result->Set(Nan::New("maxLength").ToLocalChecked(), Nan::New(ndef_msg_len_max));
    if(ndef_msg_len > ndef_msg_len_max) {
      throw errorResult(info, 0x12327, "Supplied NDEF larger than max NDEF size");
    }

    ndef_msg_len_bigendian[0] = (uint8_t)((ndef_msg_len) >> 8);
    ndef_msg_len_bigendian[1] = (uint8_t)(ndef_msg_len);
    //Mifare DESFire WriteData to write the content of the NDEF File with NLEN equal to NDEF Message length and NDEF Message
    tag.retry(0x12328, "Write NDEF message size (zero)",
              [&]()mutable->res_t{return mifare_desfire_write_data(tag, file_no, 0, 2, (uint8_t*)&ndef_msg_len_zero);});
    res = tag.retry(0x12330, "Write NDEF message",
                    [&]()mutable->res_t{return mifare_desfire_write_data(tag, file_no, 2, ndef_msg_len, reinterpret_cast<uint8_t*>(ndef_msg));});
    if(res != ndef_msg_len) {
      throw errorResult(info, 0x12329, "Writing full ndef message failed");
    }
    tag.retry(0x12331, "Write ndef message size (real)",
              [&]()mutable->res_t{return mifare_desfire_write_data(tag, file_no, 0, 2, ndef_msg_len_bigendian);});
    validTrue(info);
  } catch(MifareError err) {
    // The error is already assigned to the InfoScope
  }
}

void DesfireFree(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  try {
    DesfireData *data = DesfireData_from_info(info);
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


