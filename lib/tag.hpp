#include <vector>

#include <nfc/nfc.h>
#include <freefare.h>

#include "keys.hpp"
#include "types.hpp"

#pragma once

#include "tag.hpp"

namespace nfcdoorz::nfc {

  class Tag;

  class TagInterface {};

  class DESFireTagInterface : TagInterface {
  public:
    static const enum freefare_tag_type type = MIFARE_DESFIRE;
    DESFireTagInterface(Tag &tag);

    void setStoredUID(UID_t uid);
    UID_t getStoredUID();

    bool connect();
    bool authenticate(uint8_t key_id, MifareDESFireKey key);
    bool format();
    bool disconnect();
    std::vector<AppID_t> get_application_ids();
    bool set_configuration(bool disable_format, bool enable_random_uid);
    char *get_uid();
    bool set_default_key(MifareDESFireKey key);

    bool change_key(uint8_t key_id, MifareDESFireKey new_key, MifareDESFireKey old_key);

    bool delete_application(MifareDESFireAID aid);
    bool create_application_aes(MifareDESFireAID aid, uint8_t settings, uint8_t numKeys);
    bool select_application(MifareDESFireAID aid);

    bool create_std_data_file(uint8_t file_no, uint8_t communication_settings, uint16_t access_rights, uint32_t file_size);
    bool create_backup_data_file(uint8_t file_no, uint8_t communication_settings, uint16_t access_rights, uint32_t file_size);
    bool create_value_file(uint8_t file_no, uint8_t communication_settings, uint16_t access_rights, int32_t lower_limit, int32_t upper_limit, int32_t value, uint8_t limited_credit_enable);
    bool create_linear_record_file(uint8_t file_no, uint8_t communication_settings, uint16_t access_rights, uint32_t record_size, uint32_t max_number_of_records);
    bool create_cyclic_record_file(uint8_t file_no, uint8_t communication_settings, uint16_t access_rights, uint32_t record_size, uint32_t max_number_of_records);
  private:
    bool _connected = false;
    FreefareTag _tag = nullptr;
    UID_t _uid;
  };

}
