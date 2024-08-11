#pragma once

#include "remote_base.h"

#include <vector>


#define selectLeftNibble(nibble) ((nibble >> 4 ) & 0xF)
#define selectRightNibble(nibble) (nibble & 0xF)


namespace esphome {
namespace remote_base {
    
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// take a look to ABBWelcomeData class maybe adjustit like this one whit set and get functions
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
struct YORKData {
  std::vector<uint8_t> data;

  bool operator==(const YORKData &rhs) const { return  data == rhs.data; }
};

class YORKProtocol : public RemoteProtocol<YORKData> {
 public:
  void encode(RemoteTransmitData *dst, const YORKData &data) override;
  optional<YORKData> decode(RemoteReceiveData src) override;
  void dump(const YORKData &data) override;

private:
  std::string format_data_(const std::vector<uint8_t> &data);
};

DECLARE_REMOTE_PROTOCOL(YORK)

template<typename... Ts> class YORKAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(std::vector<uint8_t>, data)

  void set_data(const std::vector<uint8_t> &data) { data_ = data; }
  void encode(RemoteTransmitData *dst, Ts... x) override {
    YORKData data{};
    data.data = this->data_.value(x...);
    YORKProtocol().encode(dst, data);
  }
};

}  // namespace remote_base
}  // namespace esphome
