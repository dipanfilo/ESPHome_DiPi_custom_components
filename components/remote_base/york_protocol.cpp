#include "york_protocol.h"
#include "esphome/core/log.h"
#include <cinttypes>

namespace esphome {
namespace remote_base {

static const char *const TAG = "remote.york";

/////////////////////////////////
//                             //
// York ECGS01-i Specific Code //
//                             //
/////////////////////////////////

static const uint32_t HEADER_HIGH_US = 4652;
static const uint32_t HEADER_LOW_US = 2408;

static const uint32_t BIT_HIGH_US = 368;
static const uint32_t BIT_ONE_LOW_US = 944;
static const uint32_t BIT_ZERO_LOW_US = 368;

static const uint32_t END_PULS = 20340;

void YORKProtocol::encode(RemoteTransmitData *dst, const YORKData &data) {
  dst->set_carrier_frequency(38000);
  dst->reserve(2 + 64 + 64 + 3);

  dst->item(HEADER_HIGH_US, HEADER_LOW_US);

  for (uint8_t index = 0; index < 8; index++) {
    for (uint8_t mask = 1UL; mask != 0; mask <<= 1) {
      if(data.data[index] & mask) {
        dst->item(BIT_HIGH_US, BIT_ONE_LOW_US);
      } else {
        dst->item(BIT_HIGH_US, BIT_ZERO_LOW_US);
      }
    }
  }

  dst->item(BIT_HIGH_US, END_PULS);
  dst->mark(HEADER_HIGH_US);
}
optional<YORKData> YORKProtocol::decode(RemoteReceiveData src) {
  YORKData out;
   
  byte recived_checksum = 0;
  byte calculated_checksum = 0; 

  if (!src.expect_item(HEADER_HIGH_US, HEADER_LOW_US))
    return {};

  for (uint8_t index = 0; index < 8; index++) {
    uint8_t data = 0;
    for (uint8_t mask = 1UL; mask != 0; mask <<= 1) {
      if (src.expect_item(BIT_HIGH_US, BIT_ONE_LOW_US)) {
        data |= mask;
      } else if (src.expect_item(BIT_HIGH_US, BIT_ZERO_LOW_US)) {
        data &= ~mask;
      } else {
        return {};
      }
    }
    out.data.push_back(data);
  }

  recived_checksum = selectLeftNibble(out.data[7]);

  //check the recived data checksum
  for (int i = 0; i < 8; i++) {
    // Add reverse left nibble value
    calculated_checksum += selectRightNibble(out.data[i]);
    // Add reverse right nibble value
    if (i < 7)
      calculated_checksum += selectLeftNibble(out.data[i]);
  }
  calculated_checksum = selectLeftNibble(calculated_checksum);

  if(!(recived_checksum == calculated_checksum)) {
    ESP_LOGI(TAG, "Received YORK data dont have a valid checksum: calc_checksum=0x%1", calculated_checksum);
    return {};
  }

  if ((!src.expect_item(BIT_HIGH_US, END_PULS)) || (!src.expect_mark(HEADER_HIGH_US))) {
    ESP_LOGI(TAG, "Received YORK data dont have a valid finalpuls");
    //return {};
  }
  
  return out;
}

std::string YORKProtocol::format_data_(const std::vector<uint8_t> &data) {
  std::string out;
  for (uint8_t byte : data) {
    char buf[6];
    sprintf(buf, "0x%02X,", byte);
    out += buf;
  }
  out.pop_back();
  return out;
}

void YORKProtocol::dump(const YORKData &data) {
  auto data_str = format_data_(data.data);
  ESP_LOGI(TAG, "Received YORK: data=[%s]", data_str.c_str());
}


}  // namespace remote_base
}  // namespace esphome
