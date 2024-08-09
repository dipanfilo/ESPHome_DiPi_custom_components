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
  ESP_LOGI(TAG, "Received YORK: data0=0x%08" PRIX32 , data.data);
  ESP_LOGI(TAG, "Received YORK: data1=0x%08" PRIX32 , data.data1);
  ESP_LOGI(TAG, "Received YORK: currentTime=%d:%d", data.currentTime.hour, data.currentTime.minute);
  ESP_LOGI(TAG, "Received YORK: offTime=%d:%d active= %d", data.offTimer.hour, (data.offTimer.halfHour ? 30 : 0), data.offTimer.active);
  ESP_LOGI(TAG, "Received YORK: onTime=%d:%d active= %d", data.onTimer.hour, (data.onTimer.halfHour ? 30 : 0), data.onTimer.active);
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
void YORKProtocol::setDataFromBytes(YORKData *data, const byte byteStream[8])
{
  // BYTE 0: The binary data header is 8 bits long. It seems to be a binary
  // representation of the ASCII character 'h' (probably just short for
  // "header")
  //data->header = byteStream[0]; 

  // BYTE 1: right nibble is for operation mode and left nibble is for fan
  // mode
  data->operationMode = static_cast<operation_mode_t>(byteStream[1] & 0b00001111);
  data->fanMode = static_cast<fan_mode_t>(byteStream[1] >> 4);

  // BYTE 2: right nibble is the right digit of current time in minutes (0M)
  // and left nibble is the left digit of the current time in minutes (M0)
  data->currentTime.minute = ((byteStream[2] >> 4) * 10) + (byteStream[2] & 0b00001111);

  // BYTE 3: right nibble is the right digit of the current time in hours (0H)
  // and the left nibble is the left digit of the current time in hours (H0)
  data->currentTime.hour = ((byteStream[3] >> 4) * 10) + (byteStream[3] & 0b00001111);

  // BYTE 4: right nibble is the right digit of the on timer time in hours
  // and the first two bits of the left nibble is the left digit of the on
  // timer time in hours. The third bit of the nibble is 1 when the on
  // timer time is at half past the hour, else 0. The last bit is 1 only when
  // the on timer is active
  data->onTimer.hour =          (((byteStream[4] & 0b00110000) >> 4) * 10) + (byteStream[4] & 0b00001111);
  data->onTimer.halfHour = (bool)((byteStream[4] & 0b01000000) >> 6);
  data->onTimer.active   = (bool)((byteStream[4] & 0b10000000) >> 7);

  // BYTE 5: right nibble is the right digit of the off timer time in hours
  // and the first two bits of the left nibble is the left digit of the off
  // timer time in hours. The third bit of the nibble is 1 when the off
  // timer time is at half past the hour, else 0. The last bit is 1 only when
  // the off timer is active
  data->offTimer.hour =          (((byteStream[5] & 0b00110000) >> 4) * 10) + (byteStream[5] & 0b00001111);
  data->offTimer.halfHour = (bool)((byteStream[5] & 0b01000000) >> 6);
  data->offTimer.active   = (bool)((byteStream[5] & 0b10000000) >> 7);
  
  // BYTE 6: right nibble is the right digit (1s) of the temperature in
  // Celcius and the left nibble is the left digit (10s) of the temperature
  // in Celcius
  data->temperature = ((byteStream[6] >> 4) * 10) + (byteStream[6] & 0b00001111);
  
  // BYTE 7: right nibble is a concatenation of 4-bits: Louvre Swing On/Off +
  // Sleep Mode + 1 + Power Toggle. Left nibble is the reverse bit order
  // checksum of all the reverse bit order nibbles before it.
  data->swing = (bool)((byteStream[7] & 0b00000001)); 
  data->sleep = (bool)((byteStream[7] & 0b00000010) >> 1);

}
void YORKProtocol::getDataBytes(const YORKData *data, byte *byteStream) {

  byte tmpByte;
  int checksum = 0;

  // BYTE 0: The binary data header is 8 bits long. It seems to be a binary
  // representation of the ASCII character 'Synchronous Idle (SYN)control character' 
  byteStream[0] = (byte)data->header;

  // BYTE 1: right nibble is for operation mode and left nibble is for fan mode
  tmpByte = ((byte)data->operationMode);
  tmpByte |= ((byte)data->fanMode) << 4;

  // Append BYTE 1 to byteStream
  byteStream[1] = tmpByte;

  // BYTE 2: right nibble is the right digit of current time in minutes (0M)
  // and left nibble is the left digit of the current time in minutes (M0)
  tmpByte = (byte)(data->currentTime.minute % 10);
  tmpByte |= (byte)((data->currentTime.minute / 10) << 4);

  // Append BYTE 2 to byteStream
  byteStream[2] = tmpByte;

  // BYTE 3: right nibble is the right digit of the current time in hours (0H)
  // and the left nibble is the left digit of the current time in hours (H0)
  tmpByte = (byte)(data->currentTime.hour % 10);
  tmpByte |= (byte)((data->currentTime.hour / 10) << 4);

  // Append BYTE 3 to byteStream
  byteStream[3] = tmpByte;

  // BYTE 4: right nibble is the right digit of the on timer time in hours
  // and the first two bits of the left nibble is the left digit of the on
  // timer time in hours. The third bit of the nibble is 1 when the on
  // timer time is at half past the hour, else 0. The last bit is 1 only when
  // the on timer is active
  tmpByte = (byte)(data->onTimer.hour % 10);
  tmpByte |= (byte)((data->onTimer.hour / 10) << 4);
  tmpByte |= data->onTimer.halfHour ? 0b01000000 : 0b00000000;
  tmpByte |= data->onTimer.active ? 0b10000000 : 0b00000000;

  // Append BYTE 4 to byteStream
  byteStream[4] = tmpByte;

  // BYTE 5: right nibble is the right digit of the off timer time in hours
  // and the first two bits of the left nibble is the left digit of the off
  // timer time in hours. The third bit of the nibble is 1 when the off
  // timer time is at half past the hour, else 0. The last bit is 1 only when
  // the off timer is active
  tmpByte = (byte)(data->offTimer.hour % 10);
  tmpByte |= (byte)((data->offTimer.hour / 10) << 4);
  tmpByte |= data->offTimer.halfHour ? 0b01000000 : 0b00000000;
  tmpByte |= data->offTimer.active ? 0b10000000 : 0b00000000;

  // Append BYTE 5 to byteStream
  byteStream[5] = tmpByte;

  // BYTE 6: Left nibble is the right digit (1s) of the temperature in
  // Celcius and the right nibble is the left digit (10s) of the temperature
  // in Celcius
  tmpByte = (byte)(data->temperature % 10);
  tmpByte |= (byte)((data->temperature / 10) << 4);

  // Append BYTE 6 to byteStream
  byteStream[6] = tmpByte;

  // BYTE 7: Left nibble is a concatenation of 4-bits: Louvre Swing On/Off +
  // Sleep Mode + 1 + Power Toggle. Right nibble is the reverse bit order
  // checksum of all the reverse bit order nibbles before it.
  tmpByte = (data->swing ? 0b0001 : 0b0000);  // Louvre Swing On/Off
  tmpByte |= (data->sleep ? 0b0010 : 0b0000); // Sleep Mode On/Off
  tmpByte |= 0b0100;                             // This bit is always 1
  //tmpByte |= (powerToggle ? 0b1000 : 0b0000);    // Power toggle bit

  // Append left half of BYTE 7 to byteStream
  byteStream[7] = tmpByte;

  //calc checksum
  for (int i = 0; i < 8; i++)
  {
      // Add reverse left nibble value
      checksum += selectNibble(byteStream[i], false);
      // Add reverse right nibble value
      if (i < 7) {
        checksum += selectNibble(byteStream[i], true);
      }
  }

  // OR checksum with BYTE 7 of byteStream
  byteStream[7] |= (byte)(selectNibble(checksum, false) << 4);
}

void YORKProtocol::setOperationMode(YORKData *data, operation_mode_t operationMode) {
  data->operationMode = operationMode;
}
void YORKProtocol::setFanMode(YORKData *data, fan_mode_t fanMode) {
  data->fanMode = fanMode;
}
void YORKProtocol::setcurrentTime(YORKData *data, uint8_t hour,  uint8_t minute) {
  if ((hour <= 23 && hour >= 0) && (minute <= 59 && minute >= 0)) {
    data->currentTime.hour = hour;
    data->currentTime.minute = minute;
  } else {
    data->currentTime.hour = 0;
    data->currentTime.minute = 0;
  }
}
void YORKProtocol::setOnTimer(YORKData *data, uint8_t hour, bool halfhour, bool active) {
  if (hour <= 23 && hour >= 0) {
    data->onTimer.hour = hour;
    data->onTimer.halfHour = halfhour;
    data->onTimer.active = active;
  } else {
    data->onTimer.hour = 0;
    data->onTimer.hour = false;
    data->onTimer.active = false;
  }
}
void YORKProtocol::setOffTimer(YORKData *data, uint8_t hour, bool halfhour, bool active) {
  if (hour <= 23 && hour >= 0) {
    data->offTimer.hour = hour;
    data->offTimer.halfHour = halfhour;
    data->offTimer.active = active;
  } else {
    data->offTimer.hour = 0;
    data->offTimer.hour = false;
    data->offTimer.active = false;
  }
}
void YORKProtocol::setTemperature(YORKData *data, int temperature) {
  if (temperature >= 16 && temperature <= 30) {
    data->temperature = temperature;
  } else {
      data->temperature = 24;
  }
}
void YORKProtocol::setSleep(YORKData *data, bool active) {
  data->sleep = active;
}
void YORKProtocol::setSwing(YORKData *data, bool active) {
  data->sleep = active;
}

byte YORKProtocol::selectNibble(byte nibble, bool leftNibble) {
  if (!leftNibble) {
    return nibble & 0xF;
  }
  return nibble >> 4;
}

}  // namespace remote_base
}  // namespace esphome
