// Copyright 2013, Rolf Meyer
// See LICENCE for more information

#include "desfire.h"
#include "utils.h"

card_data *card_data_from_info(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  //card_data *data = static_cast<card_data *>(External::Unwrap(self->GetHiddenValue(String::NewSymbol("data"))));
  return static_cast<card_data *>(
    v8::Local<v8::External>::Cast(
      info.This()->GetHiddenValue(
        Nan::New("data").ToLocalChecked()
      )
    )->Value()
  );
}

#if ! defined(USE_LIBNFC)
  typedef LONG res_t;
#else
  typedef int res_t;
#endif

class GuardTag {
  public:
    GuardTag(FreefareTag tag, res_t &res, bool connect = false) : m_tag(tag), m_connected(false) {
      if(connect) {
        res = this->connect();
      }
    }
    ~GuardTag() { disconnect(); }
    int connect() {
      int res = 0;
      if(!m_connected) {
        res = mifare_desfire_connect(m_tag);
        mifare_sleep();
      }
      m_connected = true;
      return res;
    }
    void disconnect() { if( m_connected) mifare_desfire_disconnect(m_tag); m_connected = false; }
  private:
    FreefareTag m_tag;
    bool m_connected;
};

class GuardReader {
  public:
    GuardReader(reader_data *reader, bool lock = false)
      : m_reader(reader), m_locked(false)
      { if(lock) this->lock(); }
    ~GuardReader() { unlock(); }
    void lock()   { if(!m_locked) uv_mutex_lock(  &m_reader->mDevice); m_locked = true; }
    void unlock() { if( m_locked) uv_mutex_unlock(&m_reader->mDevice); m_locked = false; }
  private:
    reader_data *m_reader;
    bool m_locked;
};

void CardInfo(const Nan::FunctionCallbackInfo<v8::Value> &v8info) {
  res_t res;
  v8::Local<v8::Object> card = Nan::New<v8::Object>();
  card_data *data = card_data_from_info(v8info);
  struct mifare_desfire_version_info info;
  if(!data) {
    return errorResult(v8info, 0x12301, "Card is already free");
  }

  if(v8info.Length()!=0) {
    return errorResult(v8info, 0x12302, "This function takes no arguments");
  }
  { // Guarded realm;
    GuardReader reader_guard(data->reader, true);
    GuardTag tag_guard(data->tag, res, true);
    if(res) {
      return errorResult(v8info, 0x12303, "Can't conntect to Mifare DESFire target.");
    }

    res = mifare_desfire_get_version(data->tag, &info);
    if(res) {
      return errorResult(v8info, 0x12304, freefare_strerror(data->tag));
    }
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
}

void CardMasterKeyInfo(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  res_t res;
  uint8_t settings;
  uint8_t max_keys;
  card_data *data = card_data_from_info(info);
  if(!data) {
    return  errorResult(info, 0x12305, "Card is already free");
  }

  if(info.Length()!=0) {
    return errorResult(info, 0x12306, "This function takes no arguments");
  }
  GuardReader reader_guard(data->reader, true);
  GuardTag tag_guard(data->tag, res, true);
  if(res < 0) {
    return errorResult(info, 0x12303, "Can't conntect to Mifare DESFire target.");
  }

  res = mifare_desfire_get_key_settings(data->tag, &settings, &max_keys);
  if(!res) {
    v8::Local<v8::Object> key = Nan::New<v8::Object>();
    key->Set(Nan::New("configChangable").ToLocalChecked(), Nan::New((settings & 0x08)!=0));
    key->Set(Nan::New("freeCreateDelete").ToLocalChecked(), Nan::New((settings & 0x04)!=0));
    key->Set(Nan::New("freeDirectoryList").ToLocalChecked(), Nan::New((settings & 0x02)!=0));
    key->Set(Nan::New("keyChangable").ToLocalChecked(), Nan::New((settings & 0x01)!=0));
    key->Set(Nan::New("maxKeys").ToLocalChecked(), Nan::New((max_keys)));
    return info.GetReturnValue().Set(key);
  } else if (AUTHENTICATION_ERROR == mifare_desfire_last_picc_error(data->tag)) {
    return errorResult(info, 0x12307, "LOCKED");
  } else {
    return errorResult(info, 0x12307, freefare_strerror(data->tag));
  }
}

void CardName(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  card_data *data = card_data_from_info(info);
  if(!data) {
    return errorResult(info, 0x12301, "Card is already free");
  }

  res_t res;
  if(info.Length()!=0) {
    return errorResult(info, 0x12302, "This function takes no arguments");
  }

  GuardReader reader_guard(data->reader, true);
  GuardTag tag_guard(data->tag, res, true);
  if(res) {
    return errorResult(info, 0x12303, "Can't conntect to Mifare DESFire target.");
  }
  info.GetReturnValue().Set(Nan::New(freefare_get_tag_friendly_name(data->tag)).ToLocalChecked());
}

void CardKeyVersion(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  res_t res;
  uint8_t version;
  card_data *data = card_data_from_info(info);
  if(!data) {
    return errorResult(info, 0x12301, "Card is already free");
  }

  if(info.Length()!=1 || !info[0]->IsNumber()) {
    return errorResult(info, 0x12302, "This function takes a key number as arguments");
  }
  GuardReader reader_guard(data->reader, true);
  GuardTag tag_guard(data->tag, res, true);
  if(res) {
    errorResult(info, 0x12303, "Can't conntect to Mifare DESFire target.");
    return;
  }

  res = mifare_desfire_get_key_version(data->tag, info[0]->ToUint32()->Value(), &version);
  if(res) {
    errorResult(info, 0x12308, freefare_strerror(data->tag));
    return;
  }
  info.GetReturnValue().Set(Nan::New(version));
}

void CardFreeMemory(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  res_t res;
  uint32_t size;
  card_data *data = card_data_from_info(info);
  if(!data) {
    return errorResult(info, 0x12301, "Card is already free");
  }

  if(info.Length()!=0) {
    return errorResult(info, 0x12302, "This function takes no arguments");
  }

  GuardReader reader_guard(data->reader, true);
  GuardTag tag_guard(data->tag, res, true);
  if(res) {
    return errorResult(info, 0x12303, "Can't conntect to Mifare DESFire target.");
  }

  res = mifare_desfire_free_mem(data->tag, &size);
  if(res) {
    return errorResult(info, 0x12309, freefare_strerror(data->tag));
  }
  info.GetReturnValue().Set(Nan::New(size));
}

void CardSetAid(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  card_data *data = card_data_from_info(info);
  if(!data) {
    return errorResult(info, 0x12301, "Card is already free");
  }

  if(info.Length()!=1 || !info[0]->IsNumber() || info[0]->ToUint32()->Value() > 0xFFFFFF) {
    return errorResult(info, 0x12302, "This function takes the aid as argument a number smaller than 0x1000000");
  }
  if(data->aid) {
    free(data->aid);
  }
  data->aid = mifare_desfire_aid_new(info[0]->ToUint32()->Value());
  validTrue(info);
}

void CardSetKey(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  typedef MifareDESFireKey (*callback_t)(const uint8_t *);
  callback_t callbacks[4][2] = {
    {mifare_desfire_des_key_new, mifare_desfire_des_key_new_with_version},
    {mifare_desfire_3des_key_new, mifare_desfire_3des_key_new_with_version},
    {mifare_desfire_3k3des_key_new, mifare_desfire_3k3des_key_new_with_version},
    {mifare_desfire_aes_key_new, NULL}
  };

  card_data *data = card_data_from_info(info);
  if(!data) {
    return errorResult(info, 0x12301, "Card is already free");
  }

  uint8_t key_val[24];
  bool version = false;
  uint8_t aes_ver = 0;
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
      (info.Length()>3 && info[3]->ToUint32()->Value()>255)
    ) {
    return errorResult(info, 0x12302, error);
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

  if(type == 3 && info.Length() > 3) {
    aes_ver = (uint8_t)(info[3]->ToUint32()->Value()&0xFF);
  }

  if(info[0]->IsArray()) {
    v8::Local<v8::Array> key = v8::Local<v8::Array>::Cast(info[0]);
    if(key->Length()!=max_len) {
      return errorResult(info, 0x12302, error);
    }
    for(uint32_t i=0; i<max_len; i++) {
      v8::Local<v8::Value> k = key->Get(i);
      if(!k->IsInt32()) {
        return errorResult(info, 0x12302, error);
      }
      if(((int32_t)k->ToInt32()->Value())>255) {
        return errorResult(info, 0x12302, error);
      }
      key_val[i] = (uint8_t)(((uint32_t)k->ToUint32()->Value()) & 0xFF);
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
}

void CardFormat(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  card_data *data = card_data_from_info(info);
  if(!data) {
    return errorResult(info, 0x12301, "Card is already free");
  }

  uint8_t key_data_picc[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  if(info.Length()>1 || (info.Length()==1 && !info[0]->IsObject())) {
    return errorResult(info, 0x12302, "The only argument to listen has to be a callback function");
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
  res_t res;

  GuardReader reader_guard(data->reader, true);
  GuardTag tag_guard(data->tag, res, true);
  if(res < 0) {
    return errorResult(info, 0x12303, "Can't connect to Mifare DESFire target.");
  }

  MifareDESFireKey key_picc = mifare_desfire_des_key_new_with_version(key_data_picc);
  res = mifare_desfire_authenticate(data->tag, 0, key_picc);
  if(res < 0) {
    return errorResult(info, 0x12310, "Can't authenticate on Mifare DESFire target.");
  }
  mifare_desfire_key_free(key_picc);

  res = mifare_desfire_change_key_settings(data->tag, flags);
  if(res < 0) {
    return errorResult(info, 0x12311, "ChangeKeySettings failed");
  }
  res = mifare_desfire_format_picc(data->tag);
  if(res < 0) {
    return errorResult(info, 0x12312, "Can't format PICC.");
  }
  validTrue(info);
}

void CardCreateNdef(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  res_t res = 0;
  uint8_t file_no = 0;
  //uint16_t ndef_max_len = 0;
  char *ndef_msg = NULL;
  uint16_t ndef_msg_len = 0;

  uint8_t ndef_read_key[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  uint8_t *key_data_picc = ndef_read_key;
  uint8_t *key_data_app = ndef_read_key;

  card_data *data = card_data_from_info(info);
  if(!data) {
    return errorResult(info, 0x12301, "Card is already free");
  }

  if(info.Length()!=1 || !node::Buffer::HasInstance(info[0])) {
    return errorResult(info, 0x12302, "This function takes a buffer to write to a tag");
  }

  GuardReader reader_guard(data->reader, true);
  GuardTag tag_gurad(data->tag, res, true);
  if(res) {
    return errorResult(info, 0x12303, "Can't conntect to Mifare DESFire target.");
  }

  struct mifare_desfire_version_info cardinfo;
  res = mifare_desfire_get_version(data->tag, &cardinfo);
  if(res < 0) {
    return errorResult(info, 0x12304, freefare_strerror(data->tag));
  }

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
  res = mifare_desfire_select_application(data->tag, NULL);
  if(res < 0) {
    return errorResult(info, 0x12313, "Application selection failed");
  }

  MifareDESFireKey key_picc;
  MifareDESFireKey key_app;
  key_picc = mifare_desfire_des_key_new_with_version(key_data_picc);
  key_app = mifare_desfire_des_key_new_with_version(key_data_app);

  // Authentication with PICC master key MAY be needed to issue ChangeKeySettings command
  res = mifare_desfire_authenticate(data->tag, 0, key_picc);
  if(res < 0) {
    return errorResult(info, 0x12310, "Authentication with PICC master key failed");
  }

  MifareDESFireAID aid;
  if(ndef_mapping == 1) {
    uint8_t key_settings;
    uint8_t max_keys;
    mifare_desfire_get_key_settings(data->tag, &key_settings, &max_keys);
    if((key_settings & 0x08) == 0x08) {

      // Send Mifare DESFire ChangeKeySetting to change the PICC master key settings into :
      // bit7-bit4 equal to 0000b
      // bit3 equal to Xb, the configuration of the PICC master key MAY be changeable or frozen
      // bit2 equal to 0b, CreateApplication and DeleteApplication commands are allowed with PICC master key authentication
      // bit1 equal to 0b, GetApplicationIDs, and GetKeySettings are allowed with PICC master key authentication
      // bit0 equal to Xb, PICC masterkey MAY be frozen or changeable
      res = mifare_desfire_change_key_settings(data->tag, 0x09);
      if(res < 0) {
        return errorResult(info, 0x12311, "ChangeKeySettings failed");
      }
    }

    // Mifare DESFire Create Application with AID equal to EEEE10h, key settings equal to 0x09, NumOfKeys equal to 01h
    aid = mifare_desfire_aid_new(0xEEEE10);
    res = mifare_desfire_create_application(data->tag, aid, 0x09, 1);
    if(res < 0) {
      return errorResult(info, 0x12314, "Application creation failed. Try format before running create.");
    }

    // Mifare DESFire SelectApplication (Select previously creates application)
    res = mifare_desfire_select_application(data->tag, aid);
    if(res < 0) {
      return errorResult(info, 0x12313, "Application selection failed");
    }
    free(aid);

    // Authentication with NDEF Tag Application master key (Authentication with key 0)
    res = mifare_desfire_authenticate(data->tag, 0, key_app);
    if(res < 0) {
      return errorResult(info, 0x12310, "Authentication with NDEF Tag Application master key failed");
    }

    // Mifare DESFire ChangeKeySetting with key settings equal to 00001001b
    res = mifare_desfire_change_key_settings(data->tag, 0x09);
    if(res < 0) {
      return errorResult(info, 0x12311, "ChangeKeySettings failed");
    }

    // Mifare DESFire CreateStdDataFile with FileNo equal to 03h (CC File DESFire FID), ComSet equal to 00h,
    // AccesRights equal to E000h, File Size bigger equal to 00000Fh
    res = mifare_desfire_create_std_data_file(data->tag, 0x03, MDCM_PLAIN, 0xE000, 0x00000F);
    if(res < 0) {
      return errorResult(info, 0x12315, "CreateStdDataFile failed");
    }

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

    res = mifare_desfire_write_data(data->tag, 0x03, 0, sizeof(capability_container_file_content), capability_container_file_content);
    if(res < 0) {
      return errorResult(info, 0x12316, "Write CC file content failed");
    }

    // Mifare DESFire CreateStdDataFile with FileNo equal to 04h (NDEF FileDESFire FID), CmmSet equal to 00h, AccessRigths
    // equal to EEE0h, FileSize equal to 000EE0h (3808 Bytes)
    res = mifare_desfire_create_std_data_file(data->tag, 0x04, MDCM_PLAIN, 0xEEE0, 0x000EE0);
    if(res < 0) {
      return errorResult(info, 0x12317, "CreateStdDataFile failed");
    }
  } else if(ndef_mapping == 2) {
    // Mifare DESFire Create Application with AID equal to 000001h, key settings equal to 0x0F, NumOfKeys equal to 01h,
    // 2 bytes File Identifiers supported, File-ID equal to E110h
    aid = mifare_desfire_aid_new(0x000001);
    uint8_t app[] = { 0xd2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01 };
    res = mifare_desfire_create_application_iso(data->tag, aid, 0x0F, 0x21, 0, 0xE110, app, sizeof(app));
    if(res < 0) {
      return errorResult(info, 0x12314, "Application creation failed. Try format before running.");
    }

    // Mifare DESFire SelectApplication (Select previously creates application)
    res = mifare_desfire_select_application(data->tag, aid);
    if(res < 0) {
      return errorResult(info, 0x12313, "Application selection failed");
    }
    free(aid);

    // Authentication with NDEF Tag Application master key (Authentication with key 0)
    res = mifare_desfire_authenticate(data->tag, 0, key_app);
    if(res < 0) {
      return errorResult(info, 0x12310, "Authentication with NDEF Tag Application master key failed");
    }

    // Mifare DESFire CreateStdDataFile with FileNo equal to 01h (DESFire FID), ComSet equal to 00h,
    // AccesRights equal to E000h, File Size bigger equal to 00000Fh, ISO File ID equal to E103h
    res = mifare_desfire_create_std_data_file_iso(data->tag, 0x01, MDCM_PLAIN, 0xE000, 0x00000F, 0xE103);
    if(res < 0) {
      return errorResult(info, 0x12316, "CreateStdDataFileIso failed");
    }

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
    res = mifare_desfire_write_data(data->tag, 0x01, 0, sizeof(capability_container_file_content), capability_container_file_content);
    if(res < 0) {
      return errorResult(info, 0x12317, "Write CC file content failed");
    }

    // Mifare DESFire CreateStdDataFile with FileNo equal to 02h (DESFire FID), CmmSet equal to 00h, AccessRigths
    // equal to EEE0h, FileSize equal to ndefmaxsize (0x000800, 0x001000 or 0x001E00)
    res = mifare_desfire_create_std_data_file_iso(data->tag, 0x02, MDCM_PLAIN, 0xEEE0, ndef_max_size, 0xE104);
    if(res < 0) {
      return errorResult(info, 0x12318, "CreateStdDataFileIso failed");
    }
  }
  mifare_desfire_key_free(key_picc);
  mifare_desfire_key_free(key_app);

  res = mifare_desfire_write_data(data->tag, file_no, 2, ndef_msg_len, (uint8_t*)ndef_msg);
  if(res < 0) {
    return errorResult(info, 0x12319, "Writing ndef message faild");
  }
  validTrue(info);
}



int CardReadNdefTVL(const Nan::FunctionCallbackInfo<v8::Value> &v8info, card_data *data, uint8_t &file_no, uint16_t &ndef_max_len, MifareDESFireKey key_app) {
  int version;
  res_t res;
  uint8_t *cc_data;
  // #### Get Version
  // We've to track DESFire version as NDEF mapping is different
  struct mifare_desfire_version_info info;
  res = mifare_desfire_get_version(data->tag, &info);
  if(res < 0) {
    errorResult(v8info, 0x12304, freefare_strerror(data->tag));
    return res;
  }
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
  res = mifare_desfire_select_application(data->tag, aid);
  if(res < 0) {
    errorResult(v8info, 0x12313, "Application selection failed. No NDEF application found.");
    return res;
  }
  free(aid);

  // ### Authentication
  // NDEF Tag Application master key (Authentication with key 0)
  res = mifare_desfire_authenticate(data->tag, 0, key_app);
  if(res < 0) {
    errorResult(v8info, 0x12310, "Authentication with NDEF Tag Application master key failed");
    return res;
  }

  // ### Read index
  // Read Capability Container file E103
  uint8_t lendata[22]; // cf FIXME in mifare_desfire.c read_data()
  if(version == 0) {
      res = mifare_desfire_read_data(data->tag, 0x03, 0, 2, lendata);
  } else {
      // There is no more relationship between DESFire FID and ISO FileID...
      // Let's assume it's in FID 01h as proposed in the spec
      res = mifare_desfire_read_data(data->tag, 0x01, 0, 2, lendata);
  }

  if(res < 0 || res > 2) {
    errorResult(v8info, 0x12320, "Reading the ndef capability container file (E103) failed");
    return -12320;
  }
  uint32_t cclen = (((uint16_t)lendata[0]) << 8) + ((uint16_t)lendata[1]);
  if(cclen < 15) {
    errorResult(v8info, 0x12321, "The read ndef capability container file (E103) is to short");
    return -12315;
  }
  if(!(cc_data = new uint8_t[cclen + 20])) { // cf FIXME in mifare_desfire.c read_data()
    errorResult(v8info, 0x12322, "Allocation of ndef capability container file (E103) failed");
    return -12342;
  }
  if(version == 0) {
      res = mifare_desfire_read_data(data->tag, 0x03, 0, cclen, cc_data);
  } else {
      res = mifare_desfire_read_data(data->tag, 0x01, 0, cclen, cc_data);
  }
  if(res < 0) {
    errorResult(v8info, 0x12320, "Reading the ndef capability container file data (E103) failed");
    return res;
  }

  // Search NDEF File Control TLV
  uint32_t off = 7;
  while(((off + 7) < cclen) && (cc_data[off] != 0x04)) {
      off += cc_data[off + 1] + 2; // Skip TLV entry
  }

  if(off + 7 >= cclen) {
    errorResult(v8info, 0x12323, "We've reached the end of the ndef capability container file (E103) and did not find the ndef TLV");
    return -12343;
  }
  if(cc_data[off + 2] != 0xE1) {
    errorResult(v8info, 0x12324, "Found unknown ndef file reference");
    return -12344;
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

void CardReadNdef(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  res_t res;
  uint8_t file_no;
  uint16_t ndef_msg_len_max;
  uint16_t ndef_msg_len;
  uint8_t *ndef_msg;
  uint8_t ndef_read_key[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  card_data *data = card_data_from_info(info);
  if(!data) {
    return errorResult(info, 0x12301, "Card is already free");
  }
  if(info.Length()!=0) {
    return errorResult(info, 0x12302, "This function does not take any arguments");
  }
  GuardReader reader_guard(data->reader, true);
  GuardTag tag_guard(data->tag, res, true);
  if(res) {
    return errorResult(info, 0x12303, "Can't conntect to Mifare DESFire target.");
  }
  MifareDESFireKey key_app;
  key_app = mifare_desfire_des_key_new_with_version(ndef_read_key);
  res = CardReadNdefTVL(info, data, file_no, ndef_msg_len_max, key_app);
  mifare_desfire_key_free(key_app);
  if(res < 0) { return; } // Error message set by CardReaderNdevTVL
  if(!(ndef_msg = new uint8_t[ndef_msg_len_max + 20])) { // cf FIXME in mifare_desfire.c read_data()
    return errorResult(info, 0x12325, "Allocation of ndef file failed");
  }
  uint8_t lendata[20]; // cf FIXME in mifare_desfire.c read_data()
  res = mifare_desfire_read_data(data->tag, file_no, 0, 2, lendata);
  if(res < 0) {
    return errorResult(info, 0x12326, "Reading of ndef file failed");
  }
  ndef_msg_len = (((uint16_t)lendata[0]) << 8) + ((uint16_t)lendata[1]);
  if(ndef_msg_len + 2 > ndef_msg_len_max) {
    return errorResult(info, 0x12327, "Declared ndef size larger than max ndef size");
  }
  if(ndef_msg_len == 0) {
    return errorResult(info, 0x12332, "Declared ndef size is zero last write was faulty");
  }
  res = mifare_desfire_read_data(data->tag, file_no, 2, ndef_msg_len, ndef_msg);
  if(res < 0) {
    return errorResult(info, 0x12326, "Reading ndef message faild");
  }
  if(res != ndef_msg_len){
    return errorResult(info, 0x12329, "Reading full ndef message failed");
  }
  v8::Local<v8::Object> result = buffer(ndef_msg, ndef_msg_len);
  result->Set(Nan::New("maxLength").ToLocalChecked(), Nan::New(ndef_msg_len_max));
  if(ndef_msg) {
    delete [] ndef_msg;
  }
  validResult(info, result);
}

void CardWriteNdef(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  res_t res;
  uint8_t file_no;
  uint8_t  ndef_read_key[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  uint16_t ndef_msg_len;
  uint16_t ndef_msg_len_zero = 0;
  uint16_t ndef_msg_len_max;
  uint8_t  ndef_msg_len_bigendian[2];
  uint8_t *ndef_msg;
  v8::Local<v8::Object> result = Nan::New<v8::Object>();
  card_data *data = card_data_from_info(info);
  if(!data) {
    return errorResult(info, 0x12301, "Card is already free");
  }
  if(info.Length()!=1 || !node::Buffer::HasInstance(info[0])) {
    return errorResult(info, 0x12302, "This function takes a buffer to write to a tag");
  }
  ndef_msg_len = node::Buffer::Length(info[0]);
  ndef_msg = reinterpret_cast<uint8_t *>(node::Buffer::Data(info[0]));
  GuardReader reader_guard(data->reader, true);
  GuardTag tag_guard(data->tag, res, true);
  if(res) {
    return errorResult(info, 0x12303, "Can't conntect to Mifare DESFire target.");
  }

  MifareDESFireKey key_app;
  key_app = mifare_desfire_des_key_new_with_version(ndef_read_key);

  res = CardReadNdefTVL(info, data, file_no, ndef_msg_len_max, key_app);
  if(res < 0) { return; } // errorResult is set by CardReadNdefTVL
  result->Set(Nan::New("maxLength").ToLocalChecked(), Nan::New(ndef_msg_len_max));
  if(ndef_msg_len > ndef_msg_len_max) {
    return errorResult(info, 0x12327, "Supplied NDEF larger than max NDEF size");
  }

  ndef_msg_len_bigendian[0] = (uint8_t)((ndef_msg_len) >> 8);
  ndef_msg_len_bigendian[1] = (uint8_t)(ndef_msg_len);
  //Mifare DESFire WriteData to write the content of the NDEF File with NLEN equal to NDEF Message length and NDEF Message
  res = mifare_desfire_write_data(data->tag, file_no, 0, 2, (uint8_t*)&ndef_msg_len_zero);
  if(res < 0) {
    return errorResult(info, 0x12328, "Writing ndef message size pre faild");
  }
  res = mifare_desfire_write_data(data->tag, file_no, 2, ndef_msg_len, reinterpret_cast<uint8_t*>(ndef_msg));
  if(res != ndef_msg_len) {
    return errorResult(info, 0x12329, "Writing full ndef message failed");
  }
  if(res < 0) {
    return errorResult(info, 0x12330, "Writing ndef message failed");
  }
  res = mifare_desfire_write_data(data->tag, file_no, 0, 2, ndef_msg_len_bigendian);
  if(res < 0) {
    return errorResult(info, 0x12331, "Writing ndef message size post faild");
  }
  validTrue(info);
}

void CardFree(const Nan::FunctionCallbackInfo<v8::Value> &info) {
  card_data *data = card_data_from_info(info);
  if(!data) {
    return errorResult(info, 0x12322, "Card is already free");
  }

  if(info.Length()!=0) {
    return errorResult(info, 0x12321, "This function takes no arguments");
  }

  delete data;
  data = NULL;
  validTrue(info);
}


