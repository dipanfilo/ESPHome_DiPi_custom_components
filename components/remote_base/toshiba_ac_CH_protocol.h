#pragma once

#include "remote_base.h"
#include <vector>
#include <algorithm>

namespace esphome::remote_base {

struct ToshibaAcCHData {
    uint8_t nbits; 
    std::vector<uint8_t> data;

    bool operator==(const ToshibaAcCHData &rhs) const { 
        return (nbits == rhs.nbits) && (data == rhs.data);
    }
};

class ToshibaAcCHProtocol : public RemoteProtocol<ToshibaAcCHData> {
 public:
  void encode(RemoteTransmitData *dst, const ToshibaAcCHData &data) override;
  optional<ToshibaAcCHData> decode(RemoteReceiveData src) override;
  void dump(const ToshibaAcCHData &data) override;
};

DECLARE_REMOTE_PROTOCOL(ToshibaAcCH)

template<typename... Ts> 
class ToshibaAcCHAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  // For dynamic template values (lambdas)
  TEMPLATABLE_VALUE(ToshibaAcCHData, data)

  // Optimization: For static const arrays parsed from raw configurations
  void set_data_static(const uint8_t *data_ptr, size_t size, uint8_t nbits) {
    this->static_data_.nbits = nbits;
    this->static_data_.data.assign(data_ptr, data_ptr + size);
    this->is_static_ = true;
  }

  void encode(RemoteTransmitData *dst, Ts... x) override {
    if (this->is_static_) {
      ToshibaAcCHProtocol().encode(dst, this->static_data_);
    } else {
      ToshibaAcCHData dynamic_data = this->data_.value(x...);
      ToshibaAcCHProtocol().encode(dst, dynamic_data);
    }
  }

 protected:
  bool is_static_{false};
  ToshibaAcCHData static_data_{};
};

}  // namespace esphome::remote_base
