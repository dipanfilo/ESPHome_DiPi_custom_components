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

uint8_t YorkData::calc_cs_() const {
  uint8_t cs = 0;
  for (uint8_t idx = 0; idx < OFFSET_CS; idx++) {
       cs += selectRightNibble(this->data_[idx]);
    if (i < 7)
      cs += selectLeftNibble(this->data_[idx]);
  }
  return selectRightNibble(cs);
}


bool YorkData::is_compliment(const YorkData &rhs) const {
  return std::equal(this->data_.begin(), this->data_.end(), rhs.data_.begin(),
                    [](const uint8_t &a, const uint8_t &b) { return a + b == 255; });
}

void YorkProtocol::encode(RemoteTransmitData *dst, const YorkData &src) {
  dst->set_carrier_frequency(38000);
  dst->reserve(2 + 64 + 64 + 3);
  dst->item(HEADER_HIGH_US, HEADER_LOW_US);
  for (uint8_t idx = 0; idx < 8; idx++) {
    for (uint8_t mask = 1UL; mask != 0; mask <<= 1)
      dst->item(BIT_MARK_US, (src[idx] & mask) ? BIT_ONE_LOW_US : BIT_ZERO_LOW_US);
  }

  dst->item(BIT_HIGH_US, END_PULS);
  dst->mark(HEADER_HIGH_US);
}


static bool decode_data(RemoteReceiveData &src, YorkData &dst) {
  for (unsigned idx = 0; idx < 8; idx++) {
    uint8_t data = 0;
    for (uint8_t mask = 1UL; mask != 0; mask <<= 1) {
      if (!src.expect_mark(BIT_HIGH_US))
        return false;
      if (src.expect_space(BIT_ONE_LOW_US)) {
        data |= mask;
      } else if (!src.expect_space(BIT_ZERO_LOW_US)) {
        return false;
      }
    }
    dst[idx] = data;
  }
  return true;
}

optional<YorkData> MideaProtocol::decode(RemoteReceiveData src) {
  YorkData out;
  if (src.expect_item(HEADER_HIGH_US, HEADER_LOW_US) && decode_data(src, out) && out.is_valid() &&
      src.expect_item(BIT_HIGH_US, END_PULS) && src.src.expect_mark(HEADER_HIGH_US))
    return out;
  return {};
}

void MideaProtocol::dump(const YorkData &data) { ESP_LOGI(TAG, "Received York: %s", data.to_string().c_str()); }

}  // namespace remote_base
}  // namespace esphome
