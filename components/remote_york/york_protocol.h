#pragma once

#include "esphome/core/component.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"
#include "york_typedefs.h"

#include <cinttypes>

namespace esphome {
namespace york {


/*
 * ABSTRACT CLASS for generating code to send to the AC using the protocol used
 * by York AC remote controllers. This was written based on results of reverse engineering 
 * the York ECGS01-i remotes.
 */
class YORKProtocol : public RemoteProtocol<YORKData> {

    public:
        // Constructor
        DaikinYorkACRemoteProtocol()
        {
            // Initialize the settings with default values
            settings.operationMode      = OPERATION_MODE_COOL;
            settings.fanMode            = FAN_MODE_AUTO;
            settings.currentTime.hour   = 0;
            settings.currentTime.minute = 0;
            settings.onTimer.hour       = 0;
            settings.onTimer.halfHour   = true;
            settings.onTimer.active     = false;
            settings.offTimer.hour      = 0;
            settings.offTimer.halfHour  = true;
            settings.offTimer.active    = false;
            settings.temperature        = 28;
            settings.swing              = true;
            settings.sleep              = false;
        }

        void setOperationMode(operation_mode_t operationMode);

        void setFanMode(fan_mode_t fanMode);

        void setTime(time_struct_t currentTime);

        void setOnTimer(timer_struct_t onTimer);

        void setOffTimer(timer_struct_t offTimer);

        void setTemperature(int temperature);

        void setSleep(bool active);

        void setSwing(bool active);

        // Get data bytes of the signal we will be sending to the AC.
        void encode(RemoteTransmitData *dst, const YORKData &data) override;

        // Get the raw timings based on the data bytes         
        optional<YORKData> decode(RemoteReceiveData src, const YORKData &data) override;

        // Dump data
        void dump(const YORKData &data) override;


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
  TEMPLATABLE_VALUE(uint8_t, operationMode      )// = OPERATION_MODE_COOL;
  TEMPLATABLE_VALUE(uint8_t, fanMode            )// = FAN_MODE_AUTO;
  TEMPLATABLE_VALUE(uint8_t, currentTime_hour   )// = 0;
  TEMPLATABLE_VALUE(uint8_t, currentTime_minute )// = 0;
  TEMPLATABLE_VALUE(uint8_t, onTimer_hour       )// = 0;
  TEMPLATABLE_VALUE(uint8_t, onTimer_halfHour   )// = true;
  TEMPLATABLE_VALUE(uint8_t, onTimer_active     )// = false;
  TEMPLATABLE_VALUE(uint8_t, offTimer_hour      )// = 0;
  TEMPLATABLE_VALUE(uint8_t, offTimer_halfHour  )// = true;
  TEMPLATABLE_VALUE(uint8_t, offTimer_active    )// = false;
  TEMPLATABLE_VALUE(uint8_t, temperature        )// = 28;
  TEMPLATABLE_VALUE(uint8_t, swing              )// = true;
  TEMPLATABLE_VALUE(uint8_t, sleep              )// = false;


  void encode(RemoteTransmitData *dst, Ts... x) override {
    YORKData data{};

//  data.data = this->data_.value(x...);
//  data.nbits = this->nbits_.value(x...);
    YORKProtocol().encode(dst, data);
  }
};

}  // namespace remote_base
}  // namespace esphome
