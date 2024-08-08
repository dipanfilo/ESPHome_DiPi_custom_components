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
    OPERATION_MODE_DRY  = 0b0001,
    OPERATION_MODE_COOL = 0b0010,
    OPERATION_MODE_FAN  = 0b0100
} operation_mode_t;

// The fan_mode_t type is used to define a variable to hold the fan mode of the
// AC.
typedef enum {
    FAN_MODE_AUTO           = 0b0001,
    FAN_MODE_MANUAL_SPEED_3 = 0b0010,
    FAN_MODE_MANUAL_SPEED_2 = 0b0100,
    FAN_MODE_MANUAL_SPEED_1 = 0b1000,
    FAN_MODE_QUIET          = 0b1001,
    FAN_MODE_TURBO          = 0b0011,
} fan_mode_t;

// The ac_settings_t type is used to define a variable to hold all the current
// setting of the air conditioner.
struct YORKData{
    // Header
    const char header = 0x16; //Synchronous Idle (SYN) is the ASCII control character

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
    uint32_t data1;
    uint8_t nbits;

};

class YORKProtocol : public RemoteProtocol<YORKData> {
 public:
  void encode(RemoteTransmitData *dst, const YORKData &data) override;
  optional<YORKData> decode(RemoteReceiveData src) override;
  void dump(const YORKData &data) override;
  void setDataFromBytes(YORKData *data, const byte byteStream[8]);
  void getDataBytes(const YORKData *data, byte *byteStream);

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
  // This method takes the left or right nibble and return it to the caller. 
  byte selectNibble(byte nibble, bool leftNibble = false)
  {
   if (!leftNibble)
   {
    return nibble & 0xF;
   }
   return nibble >> 4;
  }
};

DECLARE_REMOTE_PROTOCOL(YORK)

template<typename... Ts> class YORKAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(uint8_t, operationMode      )// = OPERATION_MODE_COOL;
  TEMPLATABLE_VALUE(uint8_t, fanMode            )// = FAN_MODE_AUTO;
  TEMPLATABLE_VALUE(uint8_t, currentTime_hour   )// = 0;
  TEMPLATABLE_VALUE(uint8_t, currentTime_minute )// = 0;
  TEMPLATABLE_VALUE(uint8_t, onTimer_hour       )// = 0;
  TEMPLATABLE_VALUE(bool, onTimer_halfHour      )// = true;
  TEMPLATABLE_VALUE(bool, onTimer_active        )// = false;
  TEMPLATABLE_VALUE(uint8_t, offTimer_hour      )// = 0;
  TEMPLATABLE_VALUE(bool, offTimer_halfHour     )// = true;
  TEMPLATABLE_VALUE(bool, offTimer_active       )// = false;
  TEMPLATABLE_VALUE(uint8_t, temperature        )// = 28;
  TEMPLATABLE_VALUE(bool, swing                 )// = true;
  TEMPLATABLE_VALUE(bool, sleep                 )// = false;

  void encode(RemoteTransmitData *dst, Ts... x) override {
    //YORKData data{};
    settings.operationMode = this->operationMode.value(x...);
    settings.fanMode = this->fanMode_.value(x...);
    settings.currentTime.hour = this->currentTime_hour_.value(x...);
    settings.currentTime.minute = this->currentTime_minute_.value(x...);
    settings.onTimer.hour = this->onTimer_hour_.value(x...);
    settings.onTimer.halfHour = this->onTimer_halfHour_.value(x...);
    settings.onTimer.active = this->onTimer_active_.value(x...);
    settings.offTimer.hour = this->offTimer_hour_.value(x...);
    settings.offTimer.halfHour = this->offTimer_halfHour_.value(x...);
    settings.offTimer.active = this->offTimer_active_.value(x...);
    settings.temperature = this->temperature_.value(x...);
    settings.swing = this->swing_.value(x...);
    settings.sleep = this->sleep_.value(x...);
    YORKProtocol().encode(dst, settings);
  }
};

}  // namespace remote_base
}  // namespace esphome
