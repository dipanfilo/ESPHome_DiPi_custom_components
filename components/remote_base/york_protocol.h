#pragma once

#include <array>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "remote_base.h"

#define selectLeftNibble(nibble) ((nibble >> 4 ) & 0xF)
#define selectRightNibble(nibble) (nibble & 0xF)

namespace esphome {
namespace remote_base {


// Pulse and pause lengths
//const int pulseLength  = 368; // 368 us pulse
//const int pauseLength0 = -368; // 368 us space
//const int pauseLength1 = -944; // 944 us space

// The "beginning of transmission" signal consists of the following
// pulse/pause pairs
//const int beginTransmission[2] = {
//    4652, -2408  // 4.652ms pulse, 2.408ms pause
//};

// The "end of transmission" signal consists of the following pulses and pause
//const int endTransmission[3] = {
//    368,   // 368us pulse
//    -20340, // 20.34ms pause
//    4624   // 4.624 ms pulse
//};

// BYTE 0: The binary data header is 8 bits long 0x16. It seems to be a binary
// representation of the ASCII character 'Synchronous Idle (SYN) control character' 
// BYTE 1: right nibble is for operation mode and left nibble is for fan mode
// BYTE 2: right nibble is the right digit of current time in minutes (0M)
// and the left nibble is the left digit of the current time in minutes (M0)
// BYTE 3: right nibble is the right digit of the current time in hours (0H)
// and the left nibble is the left digit of the current time in hours (H0)
// BYTE 4: right nibble is the right digit of the on timer time in hours
// and the first two bits of the left nibble is the left digit of the on
// timer time in hours. The third bit of the nibble is 1 when the on
// timer time is at half past the hour, else 0. The last bit is 1 only when
// the on timer is active
// BYTE 5: right nibble is the right digit of the off timer time in hours
// and the first two bits of the left nibble is the left digit of the off
// timer time in hours. The third bit of the nibble is 1 when the off
// timer time is at half past the hour, else 0. The last bit is 1 only when
// the off timer is active
// BYTE 6: Left nibble is the right digit (1s) of the temperature in
// Celcius and the right nibble is the left digit (10s) of the temperature
// in Celcius
// BYTE 7: right nibble is a concatenation of 4-bits: Louvre Swing On/Off +
// Sleep Mode + 1 + Power Toggle. Left nibble is the reverse bit order
// checksum of all the reverse bit order nibbles before it.



class YorkData {
 public:
  // Make default
  YorkData() {}
  // Make from initializer_list
  YorkData(std::initializer_list<uint8_t> data) {
    std::copy_n(data.begin(), std::min(data.size(), this->data_.size()), this->data_.begin());
  }
  // Make from vector
  YorkData(const std::vector<uint8_t> &data) {
    std::copy_n(data.begin(), std::min(data.size(), this->data_.size()), this->data_.begin());
  }

  uint8_t *data() { return this->data_.data(); }
  const uint8_t *data() const { return this->data_.data(); }
  uint8_t size() const { return this->data_.size(); }
  bool is_valid() const { return selectLeftNibble(this->data_[OFFSET_CS]) == this->calc_cs_(); }
  void finalize() { this->data_[OFFSET_CS] = this->calc_cs_(); }
  bool is_compliment(const YorkData &rhs) const;
  std::string to_string() const { return format_hex_pretty(this->data_.data(), this->data_.size()); }
  // compare only 40-bits
  bool operator==(const YorkData &rhs) const {
    return std::equal(this->data_.begin(), this->data_.begin() + OFFSET_CS, rhs.data_.begin());
  }
  template<typename T> T to() const { return T(*this); }
  uint8_t &operator[](size_t idx) { return this->data_[idx]; }
  const uint8_t &operator[](size_t idx) const { return this->data_[idx]; }

 protected:
  uint8_t get_value_(uint8_t idx, uint8_t mask = 255, uint8_t shift = 0) const {
    return (this->data_[idx] >> shift) & mask;
  }
  void set_value_(uint8_t idx, uint8_t value, uint8_t mask = 255, uint8_t shift = 0) {
    this->data_[idx] &= ~(mask << shift);
    this->data_[idx] |= (value << shift);
  }
  void set_mask_(uint8_t idx, bool state, uint8_t mask = 255) { this->set_value_(idx, state ? mask : 0, mask); }
  static const uint8_t OFFSET_CS = 7;
  // 64-bits data
  std::array<uint8_t, 8> data_;
  // Calculate checksum
  uint8_t calc_cs_() const;
};

class YorkProtocol : public RemoteProtocol<YorkData> {
 public:
  void encode(RemoteTransmitData *dst, const YorkData &src) override;
  optional<YorkData> decode(RemoteReceiveData src) override;
  void dump(const YorkData &data) override;
};

DECLARE_REMOTE_PROTOCOL(York)

template<typename... Ts> class YorkAction : public RemoteTransmitterActionBase<Ts...> {
  TEMPLATABLE_VALUE(std::vector<uint8_t>, code)

  void encode(RemoteTransmitData *dst, Ts... x) override {
    YorkData data(this->code_.value(x...));
    data.finalize();
    YorkProtocol().encode(dst, data);
  }
};

}  // namespace remote_base
}  // namespace esphome
