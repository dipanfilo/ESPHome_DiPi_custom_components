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

  uint8_t recived_data[8];
  byte recived_checksum = 0;
  byte calculated_checksum = 0; 

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
  for (int i = 0; i < 8; i++) {
    // Add reverse left nibble value
    calculated_checksum += reverseNibble(recived_data[i], true);
    // Add reverse right nibble value
    if (i < 7)
      calculated_checksum += reverseNibble(recived_data[i]);
  }
  calculated_checksum = reverseNibble(calculated_checksum);

  if(!(recived_checksum = calculated_checksum)) {
    ESP_LOGI(TAG, "Received YORK data are not valid");
    return {};
  }

  out.data = (((byte) recived_data[0]) << 24) | (((byte) recived_data[1]) << 16) | (((byte) recived_data[2]) << 8) | (((byte) recived_data[3]));

  if (src.expect_item(BIT_HIGH_US, END_PULS)) {
    if (src.expect_mark(HEADER_HIGH_US)) {
      out.nbits = 1;
    }
  }

  SetDataToBytes(const &recived_data);

  return out;
}
void YORKProtocol::dump(const YORKData &data) {
  ESP_LOGI(TAG, "Received YORK: data=0x%08" PRIX32 ", nbits=%d", data.data, data.nbits);
  ESP_LOGI(TAG, "Received YORK: currentTime=%d:%d", data.currentTime.hour, data.currentTime.minute);
  ESP_LOGI(TAG, "Received YORK: offTime=%d:%d active= %d", data.offTimer.hour, data.offTimer.halfHour, data.offTimer.active);
  ESP_LOGI(TAG, "Received YORK: offTime=%d:%d active= %d", data.onTimer.hour, data.onTimer.halfHour, data.onTimer.active);
  ESP_LOGI(TAG, "Received YORK: setpoint=%d", data.temperature);
  ESP_LOGI(TAG, "Received YORK: operationMode=%d", data.operationMode);
  ESP_LOGI(TAG, "Received YORK: fanMode=%d", data.fanMode);
  ESP_LOGI(TAG, "Received YORK: swing=%d", data.swing);
  ESP_LOGI(TAG, "Received YORK: sleep=%d", data.sleep);
}


/*
 * This method generates a 64-bit (8 bytes or 16 nibbles) long stream for the
 * York ECGS01-i Remote Controller containing an 8-bit header, 1-nibble for AC
 * operation mode, 1-nibble fan mode, 2-bytes for current time, 1-byte for
 * on-timer, 1-byte for off timer, 1-byte for temperature setting, 1-nibble for
 * swing mode/sleep mode/power button, 1-nibble checksum. You might notice that
 * the auto on and auto off bytes are swapped in position compared to the
 * Daikin DGS01 Remote Controller.
 */
void SetDataFromBytes(byte byteStream[8])
{

    // BYTE 0: The binary data header is 8 bits long. It seems to be a binary
    // representation of the ASCII character 'h' (probably just short for
    // "header")
    //settings.header = byteStream[0]; 

    // BYTE 1: Left nibble is for operation mode and right nibble is for fan
    // mode
    settings.operationMode = static_cast<operation_mode_t>(byteStream[1] >> 4);
    settings.fanMode = static_cast<fan_mode_t>(byteStream[1] & 0b00001111);

    // BYTE 2: Left nibble is the right digit of current time in minutes (0M)
    // and right nibble is the left digit of the current time in minutes (M0)
    settings.currentTime.minute = ((byteStream[2] >> 4) * 10) + (byteStream[2] & 0b00001111);

    // BYTE 3: Left nibble is the right digit of the current time in hours (0H)
    // and the left nibble is the left digit of the current time in hours (H0)
    settings.currentTime.hour = ((byteStream[3] >> 4) * 10) + (byteStream[3] & 0b00001111);

    // BYTE 4: Left nibble is the right digit of the on timer time in hours
    // and the first two bits of the right nibble is the left digit of the on
    // timer time in hours. The third bit of the nibble is 1 when the on
    // timer time is at half past the hour, else 0. The last bit is 1 only when
    // the on timer is active
    settings.onTimer.hour =  (((byteStream[4] >> 2) & 0b00000011) * 10) + (byteStream[4] >> 4);
    settings.onTimer.halfHour = (bool)((byteStream[4] & 0b00000010 ) >> 1);
    settings.onTimer.active = (bool)(byteStream[4] & 0b00000001);

    // BYTE 5: Left nibble is the right digit of the off timer time in hours
    // and the first two bits of the right nibble is the left digit of the off
    // timer time in hours. The third bit of the nibble is 1 when the off
    // timer time is at half past the hour, else 0. The last bit is 1 only when
    // the off timer is active
    settings.onTimer.hour =  (((byteStream[5] >> 2) & 0b00000011) * 10) + (byteStream[5] >> 4);
    settings.onTimer.halfHour = (bool)((byteStream[5] & 0b00000010 ) >> 1);
    settings.onTimer.active = (bool)(byteStream[5] & 0b00000001);
    
    // BYTE 6: Left nibble is the right digit (1s) of the temperature in
    // Celcius and the right nibble is the left digit (10s) of the temperature
    // in Celcius
    settings.temperature = ((byteStream[6] >> 4) * 10) + (byteStream[6] & 0b00001111);
    
    // BYTE 7: Left nibble is a concatenation of 4-bits: Louvre Swing On/Off +
    // Sleep Mode + 1 + Power Toggle. Right nibble is the reverse bit order
    // checksum of all the reverse bit order nibbles before it.
    byteStream[7] = byteStream[7] >> 4;
    settings.swing = (bool)((byteStream[7] & 0b1000) >> 3); 
    settings.sleep = (bool)((byteStream[7] & 0b0100) >> 2);
  }





}  // namespace remote_base
}  // namespace esphome


// 134 = 2 + 64 + 64 + 3
