#include "york_protocol.h"
#include "esphome/core/log.h"

namespace esphome {
namespace remote_base {

static const char *const TAG = "remote.york";

static const uint32_t HEADER_HIGH_US = 4652;
static const uint32_t HEADER_LOW_US = 2408;

static const uint32_t BIT_HIGH_US = 368;
static const uint32_t BIT_ONE_LOW_US = 944;
static const uint32_t BIT_ZERO_LOW_US = 368;

static const uint32_t END_PULS = 20340;

void YORKProtocol::encode(RemoteTransmitData *dst, const YORKData &data) {
  dst->set_carrier_frequency(38000);
  dst->reserve(2 + data.nbits * 2u);

  dst->item(HEADER_HIGH_US, HEADER_LOW_US);

  for (uint32_t mask = 1UL << (data.nbits - 1); mask != 0; mask >>= 1) {
    if (data.data & mask) {
      dst->item(BIT_HIGH_US, BIT_ONE_LOW_US);
    } else {
      dst->item(BIT_HIGH_US, BIT_ZERO_LOW_US);
    }
  }

  dst->mark(BIT_HIGH_US);
}
optional<YORKData> YORKProtocol::decode(RemoteReceiveData src) {
  YORKData out;

  out.data = 0;
  out.nbits = 0;

  uint8_t recived_data[8];
  byte recived_checksum = 0;
  byte calculated_checksum = 0; 

  bool End_OK = false;

  if (!src.expect_item(HEADER_HIGH_US, HEADER_LOW_US))
    return {};

  for (uint8_t index = 0; index < 8; index++) {
    for (uint8_t mask = 1UL << 7; mask != 0; mask >>= 1) {
      if (src.expect_item(BIT_HIGH_US, BIT_ONE_LOW_US)) {
        recived_data[index] |= mask;
      } else if (src.expect_item(BIT_HIGH_US, BIT_ZERO_LOW_US)) {
        recived_data[index] &= ~mask;
      } else {
        return {};
      }
    }
  }

  recived_checksum = recived_data[7] & 0b00001111;

  //check the recived data checksum
  for (int i = 0; i < 8; i++)
  {
    // Add reverse left nibble value
    calculated_checksum += reverseNibble(recived_data[i], true);
    // Add reverse right nibble value
    if (i < 7)
      calculated_checksum += reverseNibble(recived_data[i]);
  }
  calculated_checksum = reverseNibble(calculated_checksum);

  if(!(recived_checksum = calculated_checksum))
  {
    ESP_LOGI(TAG, "Received YORK data are not valid");
    return {};
  }







  out.data = (((byte) recived_data[0]) << 24) | (((byte) recived_data[1]) << 16) | (((byte) recived_data[2]) << 8) | (((byte) recived_data[3]));

  if (src.expect_item(BIT_HIGH_US, END_PULS)) {
    out.nbits = 1;
    if (src.expect_mark(HEADER_HIGH_US)) {
      out.nbits = 1;
    }
  }

  return out;
}
void YORKProtocol::dump(const YORKData &data) {
  ESP_LOGI(TAG, "Received YORK: data=0x%08" PRIX32 ", nbits=%d", data.data, data.nbits);
}

}  // namespace remote_base
}  // namespace esphome


// 134 = 2 + 64 + 64 + 3
