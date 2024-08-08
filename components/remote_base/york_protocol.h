#pragma once

#include "esphome/core/component.h"
#include "remote_base.h"

#include <cinttypes>

namespace esphome {
namespace remote_base {

/*
 * The following typedef blocks define custom variable types which hold data
 * structures specific to this library.
 */

// The time_struct_t type is used to define a variable for storing the hour and
// minute of the current time. This will be used to send the date/time to the
// AC indoor unit.
typedef struct {
    unsigned int hour;
    unsigned int minute;
} time_struct_t;

// The timer_struct_t type is used to define a variable to hold the auto on/off
// data to send to the AC indoor unit. As with the remote controller, you can
// only define a time with 30 minute increments.
typedef struct {
    unsigned int hour;
    bool halfHour;
    bool active;
} timer_struct_t;

// The operation_mode_t type is used to define a variable to hold the desired
// operation mode of the AC.
typedef enum {
    OPERATION_MODE_DRY  = 0b1000,
    OPERATION_MODE_COOL = 0b0100,
    OPERATION_MODE_FAN  = 0b0010
} operation_mode_t;

// The fan_mode_t type is used to define a variable to hold the fan mode of the
// AC.
typedef enum {
    FAN_MODE_MANUAL_SPEED_1 = 0b0001,
    FAN_MODE_MANUAL_SPEED_2 = 0b0010,
    FAN_MODE_MANUAL_SPEED_3 = 0b0100,
    FAN_MODE_AUTO           = 0b1000,
    FAN_MODE_QUIET          = 0b1001,
    FAN_MODE_TURBO          = 0b1100
} fan_mode_t;

// The ac_settings_t type is used to define a variable to hold all the current
// setting of the air conditioner.
typedef struct {
    // Header
    const char header = 'h';

    // Operation mode
    operation_mode_t operationMode;

    // Fan Mode
    fan_mode_t fanMode;

    // Current Time
    time_struct_t currentTime;

    // Off Timer
    timer_struct_t offTimer;

    // On Timer
    timer_struct_t onTimer;

    // Temperature Setting
    int temperature;

    // Louvre Swing
    bool swing;

    // Sleep Mode
    bool sleep;

    // NOTE: We are not going to store the power button flag in this struct
    // since that is a toggle type of switch which is only sent to the AC when
    // the remote controller power button is pressed. Only the AC knows its own
    // current powered state since the remote controller does not receive
    // feedback from the AC.





    // OLD Data LG Protocol for testing
    uint32_t data;
    uint8_t nbits;

} YORKData;

class YORKProtocol : public RemoteProtocol<YORKData> {
 public:
  void encode(RemoteTransmitData *dst, const YORKData &data) override;
  optional<YORKData> decode(RemoteReceiveData src) override;
  void dump(const YORKData &data) override;
    void YORKProtocol::SetDataFromBytes(byte byteStream[8]);
    
 private:
  
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

  // Air conditioner settings
  YORKData settings;

 protected:
  /*
  * containing nibble manipulation methods to be used for various
  * operations such as checksumming.
  */
  // This is simply a lookup table containing the reversed bit order
  // nibbles of numbers 0 through 15
  const byte reverseNibbleLookup[16] = {
      0b0000, 0b1000, 0b0100, 0b1100, 0b0010, 0b1010, 0b0110, 0b1110,
      0b0001, 0b1001, 0b0101, 0b1101, 0b0011, 0b1011, 0b0111, 0b1111
  };

  // This method takes a nibble and uses the reverseNibbleLookup array to map the
  // reverse byte order nibble and return it to the caller. The leftNibble
  // argument is true if we want the reverse byte order nibble of the left nibble
  // of a byte.
  byte reverseNibble(byte nibble, bool leftNibble = false)
  {
   if (!leftNibble)
   {
    return reverseNibbleLookup[nibble & 0xF];
   }
   return reverseNibbleLookup[nibble >> 4];
  }
};

DECLARE_REMOTE_PROTOCOL(YORK)

template<typename... Ts> class YORKAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(uint32_t, data)
  TEMPLATABLE_VALUE(uint8_t, nbits)

  void encode(RemoteTransmitData *dst, Ts... x) override {
    YORKData data{};
    data.data = this->data_.value(x...);
    data.nbits = this->nbits_.value(x...);
    YORKProtocol().encode(dst, data);
  }
};

}  // namespace remote_base
}  // namespace esphome
