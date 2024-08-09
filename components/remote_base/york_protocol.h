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

    // packege
    uint32_t data;
    uint32_t data1;

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
};

class YORKProtocol : public RemoteProtocol<YORKData> {
 public:
  void encode(RemoteTransmitData *dst, const YORKData &data) override;
  optional<YORKData> decode(RemoteReceiveData src) override;
  void dump(const YORKData &data) override;
  
 private:
  void setDataFromBytes(YORKData *data, const byte byteStream[8]);
  void getDataBytes(const YORKData *data, byte *byteStream);
  byte selectNibble(byte nibble, bool leftNibble);

 protected:
  // Air conditioner settings
  YORKData settings_;
};

DECLARE_REMOTE_PROTOCOL(YORK)

template<typename... Ts> class YORKAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(operation_mode_t, operationMode      );// = OPERATION_MODE_COOL;
  TEMPLATABLE_VALUE(fan_mode_t, fanMode            );// = FAN_MODE_AUTO;
  TEMPLATABLE_VALUE(uint8_t, currentTime_hour   );// = 0;
  TEMPLATABLE_VALUE(uint8_t, currentTime_minute );// = 0;
  TEMPLATABLE_VALUE(uint8_t, onTimer_hour       );// = 0;
  TEMPLATABLE_VALUE(bool, onTimer_halfHour      );// = true;
  TEMPLATABLE_VALUE(bool, onTimer_active        );// = false;
  TEMPLATABLE_VALUE(uint8_t, offTimer_hour      );// = 0;
  TEMPLATABLE_VALUE(bool, offTimer_halfHour     );// = true;
  TEMPLATABLE_VALUE(bool, offTimer_active       );// = false;
  TEMPLATABLE_VALUE(uint8_t, temperature        );// = 28;
  TEMPLATABLE_VALUE(bool, swing                 );// = true;
  TEMPLATABLE_VALUE(bool, sleep                 );// = false;

  void encode(RemoteTransmitData *dst, Ts... x) override {
    YORKData data{};
    data.operationMode = this->operationMode_.value(x...);
    data.fanMode = this->fanMode_.value(x...);
    data.currentTime.hour = this->currentTime_hour_.value(x...);
    data.currentTime.minute = this->currentTime_minute_.value(x...);
    data.onTimer.hour = this->onTimer_hour_.value(x...);
    data.onTimer.halfHour = this->onTimer_halfHour_.value(x...);
    data.onTimer.active = this->onTimer_active_.value(x...);
    data.offTimer.hour = this->offTimer_hour_.value(x...);
    data.offTimer.halfHour = this->offTimer_halfHour_.value(x...);
    data.offTimer.active = this->offTimer_active_.value(x...);
    data.temperature = this->temperature_.value(x...);
    data.swing = this->swing_.value(x...);
    data.sleep = this->sleep_.value(x...);
    YORKProtocol().encode(dst, data);
  }
};

}  // namespace remote_base
}  // namespace esphome
