#include "york_protocol.h"
#include "esphome/core/log.h"

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

  byte dataByteStream[8];

  getDataBytes(&data, dataByteStream);

  dst->item(HEADER_HIGH_US, HEADER_LOW_US);

  for (uint8_t index = 0; index < 8; index++) {
    for (uint8_t mask = 1UL; mask != 0; mask <<= 1) {
      if(dataByteStream[index] & mask) {
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

  uint8_t recived_data[8];
   
  byte recived_checksum = 0;
  byte calculated_checksum = 0; 

  if (!src.expect_item(HEADER_HIGH_US, HEADER_LOW_US))
    return {};

  for (uint8_t index = 0; index < 8; index++) {
    for (uint8_t mask = 1UL; mask != 0; mask <<= 1) {
      if (src.expect_item(BIT_HIGH_US, BIT_ONE_LOW_US)) {
        recived_data[index] |= mask;
      } else if (src.expect_item(BIT_HIGH_US, BIT_ZERO_LOW_US)) {
        recived_data[index] &= ~mask;
      } else {
        return {};
      }
    }
  }

  recived_checksum = ((recived_data[7] & 0b11110000) >> 4) ;

  //check the recived data checksum
  for (int i = 0; i < 8; i++) {
    // Add reverse left nibble value
    calculated_checksum += selectNibble(recived_data[i], false);
    // Add reverse right nibble value
    if (i < 7)
      calculated_checksum += selectNibble(recived_data[i], true);
  }
  calculated_checksum = selectNibble(calculated_checksum, false);

  if(!(recived_checksum == calculated_checksum)) {
    ESP_LOGI(TAG, "Received YORK data dont have a valid checksum: calc_checksum=0x%08" PRIX8 , calculated_checksum);
    return {};
  }

  if ((!src.expect_item(BIT_HIGH_US, END_PULS)) || (!src.expect_mark(HEADER_HIGH_US))) {
     ESP_LOGI(TAG, "Received YORK data dont have a valid finalpuls");
    return {};
  }

  out.data  = (((byte) recived_data[0]) << 24)| (((byte) recived_data[1]) << 16)| (((byte) recived_data[2]) << 8)| ((byte) recived_data[3]);
  out.data1 = (((byte) recived_data[4]) << 24)| (((byte) recived_data[5]) << 16)| (((byte) recived_data[6]) << 8)| ((byte) recived_data[7]);

  setDataFromBytes(&out, recived_data);

  return out;
}
void YORKProtocol::dump(const YORKData &data) {
  //ESP_LOGI(TAG, "Received YORK: data0=0x%08" PRIX32 , data.data);
  //ESP_LOGI(TAG, "Received YORK: data1=0x%08" PRIX32 , data.data1);
  ESP_LOGI(TAG, "Received YORK: currentTime=%d:%d", data.getcurrentTime().hour, data.getcurrentTime().minute);
}


}  // namespace remote_base
}  // namespace esphome
