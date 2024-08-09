#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include <array>
#include <cinttypes>
#include <utility>
#include <vector>

namespace esphome {
namespace york {

/*
 * The following typedef blocks define custom variable types which hold data
 * structures specific to this library.
 */

// Pulse and pause lengths
const int pulseLength  = 368; // 368 us pulse
const int pauseLength0 = -368; // 368 us space
const int pauseLength1 = -944; // 944 us space

// The "beginning of transmission" signal consists of the following
// pulse/pause pairs
const int beginTransmission[6] = {
    9788, -9676, // 9.788ms pulse, 9.676ms pause
    9812, -9680, // 9.812ms pulse, 9.680ms pause
    4652, -2408  // 4.652ms pulse, 2.408ms pause
};

// The "end of transmission" signal consists of the following pulses
// and pause
const int endTransmission[3] = {
    368,   // 368us pulse
    -20340, // 20.34ms pause
    4624   // 4.624 ms pulse
};

// Temperature
const uint8_t YORK_TEMP_MIN = 18;  // Celsius
const uint8_t YORK_TEMP_MAX = 30;  // Celsius

// The time_struct_t type is used to define a variable for storing the hour and
// minute of the current time. This will be used to send the date/time to the
// AC indoor unit.
struct time_struct_t {
    unsigned int hour;
    unsigned int minute;
};

// The timer_struct_t type is used to define a variable to hold the auto on/off
// data to send to the AC indoor unit. As with the remote controller, you can
// only define a time with 30 minute increments.
struct timer_struct_t {
    unsigned int hour;
    bool halfHour;
    bool active;
};

// The OperationMode type is used to define a variable to hold the desired
// operation mode of the AC.
enum OperationMode {
    OPERATION_MODE_DRY  = 0b0001,
    OPERATION_MODE_COOL = 0b0010,
    OPERATION_MODE_FAN  = 0b0100
};

// The FanMode type is used to define a variable to hold the fan mode of the AC.
enum FanMode {
    FAN_MODE_AUTO           = 0b0001,
    FAN_MODE_MANUAL_SPEED_3 = 0b0010,
    FAN_MODE_MANUAL_SPEED_2 = 0b0100,
    FAN_MODE_MANUAL_SPEED_1 = 0b1000,
    FAN_MODE_QUIET          = 0b1001,
    FAN_MODE_TURBO          = 0b0011,
};

// The SwingModeVertical is used to define a variable to hold the fan mode of the AC.
enum SwingModeVertical {
    SWING_MODE_VERTICAL_OFF  = 0b0000,
    SWING_MODE_VERTICAL_AUTO = 0b0001,
};





// The class YORKData is used to define a variable to hold all the current
// setting of the air conditioner.
class YORKData {
  public:
    // Constructor
    YORKData()
    {
        // Initialize the settings with default values
        this->setHeader();
        this->setOperationMode(OPERATION_MODE_COOL);
        this->setFanMode(FAN_MODE_AUTO);
        this->setcurrentTime(0, 0);
        this->setOnTimer(0, false, false);
        this->setOffTimer(0, false, false);
        this->setTemperature(24);
        this->setSwing(false);
        this->setSleep(false);

    }

    // BYTE 0: The binary data header is 8 bits long 0x16. It seems to be a binary
    // representation of the ASCII character 'Synchronous Idle (SYN) control character' 
    void setHeader() { 
      this->data_[0]  = 0x16;
    }
    
    // BYTE 1: right nibble is for operation mode and left nibble is for fan mode
    void setOperationMode(OperationMode operationMode) {
      this->data_[1] &= 0b11110000;
      if((operationMode == OPERATION_MODE_DRY) || (operationMode == OPERATION_MODE_COOL) || (operationMode == OPERATION_MODE_FAN)) {
        this->data_[1] |= ((byte) static_cast<OperationMode>(operationMode));
      } else {
        this->data_[1] |= OPERATION_MODE_COOL;
      }
    }
    OperationMode getOperationMode() {
      return static_cast<OperationMode>(this->data_[1] & 0b00001111);
    }
    void setFanMode( FanMode fanMode) {
      this->data_[1] &= 0b00001111;
      if((fanMode == FAN_MODE_AUTO) || (fanMode == FAN_MODE_MANUAL_SPEED_1) || (fanMode == FAN_MODE_MANUAL_SPEED_2) || (fanMode == FAN_MODE_MANUAL_SPEED_3) || (fanMode == FAN_MODE_QUIET) || (fanMode == FAN_MODE_TURBO)) {
        this->data_[1] |= (byte)((static_cast<FanMode>(fanMode)) << 4 );
      } else {
        this->data_[1] |= (byte)(FAN_MODE_AUTO << 4);
      }
    }
    FanMode getFanMode() {
      return static_cast<FanMode>(this->data_[1] >> 4);
    } 
  
    // BYTE 2: right nibble is the right digit of current time in minutes (0M)
    // and the left nibble is the left digit of the current time in minutes (M0)
    // BYTE 3: right nibble is the right digit of the current time in hours (0H)
    // and the left nibble is the left digit of the current time in hours (H0)
    void setcurrentTime( uint8_t hour,  uint8_t minute) {
      if ((hour <= 23 && hour >= 0) && (minute <= 59 && minute >= 0)) {
        this->data_[2] = (byte)(minute % 10);
        this->data_[2] |= (byte)((minute / 10) << 4);
        this->data_[3] = (byte)(hour % 10);
        this->data_[3] |= (byte)((hour / 10) << 4);
      } else {
        this->data_[2] = 0;
        this->data_[3] = 0;
      }
    }
    time_struct_t getcurrentTime() {
      time_struct_t currentTime;
      currentTime.minute = ((this->data_[2] >> 4) * 10) + (data_[2] & 0b00001111);
      currentTime.hour = ((this->data_[3] >> 4) * 10) + (data_[3] & 0b00001111);
      return currentTime;
    }

    // BYTE 4: right nibble is the right digit of the on timer time in hours
    // and the first two bits of the left nibble is the left digit of the on
    // timer time in hours. The third bit of the nibble is 1 when the on
    // timer time is at half past the hour, else 0. The last bit is 1 only when
    // the on timer is active
    void setOnTimer(uint8_t hour, bool halfhour, bool active) {
      if (hour <= 23 && hour >= 0) {
        this->data_[4] = (byte)(hour % 10);
        this->data_[4] |= (byte)((hour / 10) << 4);
        this->data_[4] |= halfhour ? 0b01000000 : 0b00000000;
        this->data_[4] |= active ? 0b10000000 : 0b00000000;
      } else {
        this->data_[4] = 0;
      }
    }
    timer_struct_t getOnTimer() {
      timer_struct_t onTimer;
      onTimer.hour =         ((((this->data_[4] & 0b00110000) >> 4) * 10) + (this->data_[4] & 0b00001111));
      onTimer.halfHour = (bool)((this->data_[4] & 0b01000000) >> 6);
      onTimer.active   = (bool)((this->data_[4] & 0b10000000) >> 7);
      return onTimer;
    }    
    
    // BYTE 5: right nibble is the right digit of the off timer time in hours
    // and the first two bits of the left nibble is the left digit of the off
    // timer time in hours. The third bit of the nibble is 1 when the off
    // timer time is at half past the hour, else 0. The last bit is 1 only when
    // the off timer is active
    void setOffTimer(uint8_t hour, bool halfhour, bool active) {
      if (hour <= 23 && hour >= 0) {
        this->data_[5] = (byte)(hour % 10);
        this->data_[5] |= (byte)((hour / 10) << 4);
        this->data_[5] |= halfhour ? 0b01000000 : 0b00000000;
        this->data_[5] |= active ? 0b10000000 : 0b00000000;
      } else {
        this->data_[5] = 0;
      }
    }
    timer_struct_t getOffTimer() {
      timer_struct_t offTimer;
      offTimer.hour =         ((((this->data_[5] & 0b00110000) >> 4) * 10) + (this->data_[5] & 0b00001111));
      offTimer.halfHour = (bool)((this->data_[5] & 0b01000000) >> 6);
      offTimer.active   = (bool)((this->data_[5] & 0b10000000) >> 7);
      return offTimer;
    }

    // BYTE 6: Left nibble is the right digit (1s) of the temperature in
    // Celcius and the right nibble is the left digit (10s) of the temperature
    // in Celcius
    void setTemperature(int temperature) {
      if((temperature <= YORK_TEMP_MAX && temperature >= YORK_TEMP_MIN)) {
        this->data_[6] = (byte)(temperature % 10);
        this->data_[6] |= (byte)((temperature / 10) << 4);
      } else {
        this->data_[6] = (byte)(24 % 10);
        this->data_[6] |= (byte)((24 / 10) << 4);
      }
    }
    uint8_t getTemperature() {
      return ((this->data_[6] >> 4) * 10) + (this->data_[6] & 0b00001111);
    }

    // BYTE 7: right nibble is a concatenation of 4-bits: Louvre Swing On/Off +
    // Sleep Mode + 1 + Power Toggle. Left nibble is the reverse bit order
    // checksum of all the reverse bit order nibbles before it.
    void setSleep(bool active) {
      this->data_[7] |= (active ? 0b0010 : 0b0000); // Sleep Mode On/Off
    }
    bool getSleep() {
      return (bool)((this->data_[7] & 0b00000010) >> 1);
    }
    void setSwing(bool active){
      this->data_[7] = (active ? 0b0001 : 0b0000);  // Louvre Swing On/Off
    }
    bool getSwing() {
      return (bool)((this->data_[7] & 0b00000001));
    }
    


    const uint8_t *data() const { return this->data_.data(); }
    std::vector<uint8_t> get_data() const {
      std::vector<uint8_t> data(this->data_.begin(), this->data_.begin() + 8);
      return data;
    }

    bool is_valid() const {
      return this->data_[0] == 0x16 &&  (uint8_t)(((this->data_[7] & 0b11110000) >> 4) == this->calc_cs_());
    }


    std::string to_string(uint8_t max_print_bytes = 255) const {
      std::string info;
      if (this->is_valid()) {
        info += str_sprintf("Data: %s", format_hex_pretty(this->getdata()).c_str());
      } else {
        info = "[Invalid]";
      }

      return info;
    }


  protected:
    std::array<uint8_t, 8> data_;
    uint8_t calc_cs_() {
      int checksum = 0;  
      for (int i = 0; i < 8; i++)
      {
        // Add reverse left nibble value
        checksum += selectNibble(this->data_[i], false);
        // Add reverse right nibble value
        if (i < 7) { 
            checksum += selectNibble(this->data_[i], true);
        } 
      }
      return (byte)(selectNibble(checksum, false) << 4);
    }
    byte selectNibble(byte nibble, bool leftNibble) {
      if (!leftNibble) {
        return nibble & 0xF;
      }
      return nibble >> 4;
    }
};

}  // namespace remote_base
}  // namespace esphome