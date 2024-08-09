#include "climate_ir_york.h"
#include "esphome/core/log.h"

namespace esphome {
namespace york {

static const char *const TAG = "york.climate";



climate::ClimateTraits YorkClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(this->sensor_ != nullptr);
  traits.set_supports_action(false);
  traits.set_supports_heat(false);
  traits.set_visual_min_temperature(YORK_TEMP_MIN);
  traits.set_visual_max_temperature(YORK_TEMP_MAX);
  traits.set_visual_temperature_step(1.0f);
  traits.set_supported_modes({climate::CLIMATE_MODE_OFF});

  if (this->supports_cool_)
    traits.add_supported_mode(climate::CLIMATE_MODE_COOL);
  if (this->supports_heat_)
    traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);

  if (this->supports_cool_ && this->supports_heat_)
    traits.add_supported_mode(climate::CLIMATE_MODE_HEAT_COOL);

  if (this->supports_dry_)
    traits.add_supported_mode(climate::CLIMATE_MODE_DRY);
  if (this->supports_fan_only_)
    traits.add_supported_mode(climate::CLIMATE_MODE_FAN_ONLY);

  // Default to only 3 levels in ESPHome even if most unit supports 4. The 3rd level is not used.
  traits.set_supported_fan_modes(
      {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH});
  if (this->fan_mode_ == FAN_MODE_QUIET)
    traits.add_supported_fan_mode(climate::CLIMATE_FAN_QUIET);
  if (this->fan_mode_ >= FAN_MODE_TURBO)
    traits.add_supported_fan_mode(climate::CLIMATE_FAN_FOCUS);

  traits.set_supported_swing_modes({climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL});

  traits.set_supported_presets({climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_ECO,
                                climate::CLIMATE_PRESET_BOOST, climate::CLIMATE_PRESET_SLEEP});

  return traits;
}

void YorkClimate::transmit_state() {

  switch (this->mode) {
  
    case climate::CLIMATE_MODE_DRY:
      remote_state[6] = MITSUBISHI_MODE_DRY;
      remote_state[8] = MITSUBISHI_MODE_A_DRY;
      break;
    case climate::CLIMATE_MODE_COOL:
      remote_state[6] = MITSUBISHI_MODE_COOL;
      remote_state[8] = MITSUBISHI_MODE_A_COOL;
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      remote_state[6] = MITSUBISHI_MODE_FAN_ONLY;
      remote_state[8] = MITSUBISHI_MODE_A_AUTO;
      break;
    case climate::CLIMATE_MODE_OFF:
    default:
      remote_state[6] = MITSUBISHI_MODE_COOL;
      remote_state[8] = MITSUBISHI_MODE_A_COOL;
      if (this->supports_heat_) {
        remote_state[6] = MITSUBISHI_MODE_HEAT;
        remote_state[8] = MITSUBISHI_MODE_A_HEAT;
      }
      remote_state[5] = MITSUBISHI_OFF;
      break;
  }

  // Temperature
  if (this->mode == climate::CLIMATE_MODE_DRY) {
    remote_state[7] = 24 - MITSUBISHI_TEMP_MIN;  // Remote sends always 24°C if "Dry" mode is selected
  } else {
    remote_state[7] = (uint8_t) roundf(
        clamp<float>(this->target_temperature, MITSUBISHI_TEMP_MIN, MITSUBISHI_TEMP_MAX) - MITSUBISHI_TEMP_MIN);
  }

  

  ESP_LOGD(TAG, "default_horizontal_direction_: %02X", this->default_horizontal_direction_);

  // Fan Speed & Vertical Vane
  // Map of Climate fan mode to this device expected value
  // For 3Level: Low = 1, Medium = 2, High = 3
  // For 4Level: Low = 1, Middle = 2, Medium = 3, High = 4
  // For 5Level: Low = 1, Middle = 2, Medium = 3, High = 4
  // For 4Level + Quiet: Low = 1, Middle = 2, Medium = 3, High = 4, Quiet = 5

  switch (this->fan_mode.value()) {
    case climate::CLIMATE_FAN_LOW:
      remote_state[9] = 1;
      break;
    case climate::CLIMATE_FAN_MEDIUM:
      if (this->fan_mode_ == MITSUBISHI_FAN_3L) {
        remote_state[9] = 2;
      } else {
        remote_state[9] = 3;
      }
      break;
    case climate::CLIMATE_FAN_HIGH:
      if (this->fan_mode_ == MITSUBISHI_FAN_3L) {
        remote_state[9] = 3;
      } else {
        remote_state[9] = 4;
      }
      break;
    case climate::CLIMATE_FAN_MIDDLE:
      remote_state[9] = 2;
      break;
    case climate::CLIMATE_FAN_QUIET:
      remote_state[9] = 5;
      break;
    default:
      remote_state[9] = MITSUBISHI_FAN_AUTO;
      break;
  }

  ESP_LOGD(TAG, "fan: %02x state: %02x", this->fan_mode.value(), remote_state[9]);

  // Vertical Vane
  switch (this->swing_mode) {
    case climate::CLIMATE_SWING_VERTICAL:
    case climate::CLIMATE_SWING_BOTH:
      remote_state[9] = remote_state[9] | MITSUBISHI_VERTICAL_VANE_SWING | MITSUBISHI_OTHERWISE;  // Vane Swing
      break;
    case climate::CLIMATE_SWING_OFF:
    default:
      remote_state[9] = remote_state[9] | this->default_vertical_direction_ |
                        MITSUBISHI_OTHERWISE;  // Off--> vertical default position
      break;
  }

  ESP_LOGD(TAG, "default_vertical_direction_: %02X", this->default_vertical_direction_);

  // Special modes
  switch (this->preset.value()) {
    case climate::CLIMATE_PRESET_ECO:
      remote_state[6] = MITSUBISHI_MODE_COOL | MITSUBISHI_OTHERWISE;
      remote_state[8] = (remote_state[8] & ~7) | MITSUBISHI_MODE_A_COOL;
      remote_state[14] = MITSUBISHI_ECONOCOOL;
      break;
    case climate::CLIMATE_PRESET_SLEEP:
      remote_state[9] = MITSUBISHI_FAN_AUTO;
      remote_state[14] = MITSUBISHI_NIGHTMODE;
      break;
    case climate::CLIMATE_PRESET_BOOST:
      remote_state[6] |= MITSUBISHI_OTHERWISE;
      remote_state[15] = MITSUBISHI_POWERFUL;
      break;
    case climate::CLIMATE_PRESET_NONE:
    default:
      break;
  }

  // Checksum
  for (int i = 0; i < 17; i++) {
    remote_state[17] += remote_state[i];
  }

  ESP_LOGD(TAG, "sending: %02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X",
           remote_state[0], remote_state[1], remote_state[2], remote_state[3], remote_state[4], remote_state[5],
           remote_state[6], remote_state[7], remote_state[8], remote_state[9], remote_state[10], remote_state[11],
           remote_state[12], remote_state[13], remote_state[14], remote_state[15], remote_state[16], remote_state[17]);

  auto transmit = this->transmitter_->transmit();
  auto *data = transmit.get_data();

  data->set_carrier_frequency(38000);
  // repeat twice
  for (uint8_t r = 0; r < 2; r++) {
    // Header
    data->mark(MITSUBISHI_HEADER_MARK);
    data->space(MITSUBISHI_HEADER_SPACE);
    // Data
    for (uint8_t i : remote_state) {
      for (uint8_t j = 0; j < 8; j++) {
        data->mark(MITSUBISHI_BIT_MARK);
        bool bit = i & (1 << j);
        data->space(bit ? MITSUBISHI_ONE_SPACE : MITSUBISHI_ZERO_SPACE);
      }
    }
    // Footer
    if (r == 0) {
      data->mark(MITSUBISHI_BIT_MARK);
      data->space(MITSUBISHI_MIN_GAP);  // Pause before repeating
    }
  }
  data->mark(MITSUBISHI_BIT_MARK);

  transmit.perform();
}












bool YorkClimate::parse_state_frame_(const uint8_t frame[]) { return false; }

bool YorkClimate::on_receive(remote_base::RemoteReceiveData data) {
  uint8_t state_frame[18] = {};

  if (!data.expect_item(MITSUBISHI_HEADER_MARK, MITSUBISHI_HEADER_SPACE)) {
    ESP_LOGV(TAG, "Header fail");
    return false;
  }

  for (uint8_t pos = 0; pos < 18; pos++) {
    uint8_t byte = 0;
    for (int8_t bit = 0; bit < 8; bit++) {
      if (data.expect_item(MITSUBISHI_BIT_MARK, MITSUBISHI_ONE_SPACE)) {
        byte |= 1 << bit;
      } else if (!data.expect_item(MITSUBISHI_BIT_MARK, MITSUBISHI_ZERO_SPACE)) {
        ESP_LOGV(TAG, "Byte %d bit %d fail", pos, bit);
        return false;
      }
    }
    state_frame[pos] = byte;

    // Check Header && Footer
    if ((pos == 0 && byte != MITSUBISHI_BYTE00) || (pos == 1 && byte != MITSUBISHI_BYTE01) ||
        (pos == 2 && byte != MITSUBISHI_BYTE02) || (pos == 3 && byte != MITSUBISHI_BYTE03) ||
        (pos == 4 && byte != MITSUBISHI_BYTE04) || (pos == 13 && byte != MITSUBISHI_BYTE13) ||
        (pos == 16 && byte != MITSUBISHI_BYTE16)) {
      ESP_LOGV(TAG, "Bytes 0,1,2,3,4,13 or 16 fail - invalid value");
      return false;
    }
  }

  // On/Off and Mode
  if (state_frame[5] == MITSUBISHI_OFF) {
    this->mode = climate::CLIMATE_MODE_OFF;
  } else {
    switch (state_frame[6]) {
      case MITSUBISHI_MODE_HEAT:
        this->mode = climate::CLIMATE_MODE_HEAT;
        break;
      case MITSUBISHI_MODE_DRY:
        this->mode = climate::CLIMATE_MODE_DRY;
        break;
      case MITSUBISHI_MODE_COOL:
        this->mode = climate::CLIMATE_MODE_COOL;
        break;
      case MITSUBISHI_MODE_FAN_ONLY:
        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
        break;
      case MITSUBISHI_MODE_AUTO:
        this->mode = climate::CLIMATE_MODE_HEAT_COOL;
        break;
    }
  }

  // Temp
  this->target_temperature = state_frame[7] + MITSUBISHI_TEMP_MIN;

  // Fan
  uint8_t fan = state_frame[9] & 0x07;  //(Bit 0,1,2 = Speed)
  // Map of Climate fan mode to this device expected value
  // For 3Level: Low = 1, Medium = 2, High = 3
  // For 4Level: Low = 1, Middle = 2, Medium = 3, High = 4
  // For 5Level: Low = 1, Middle = 2, Medium = 3, High = 4
  // For 4Level + Quiet: Low = 1, Middle = 2, Medium = 3, High = 4, Quiet = 5
  climate::ClimateFanMode modes_mapping[8] = {
      climate::CLIMATE_FAN_AUTO,
      climate::CLIMATE_FAN_LOW,
      this->fan_mode_ == MITSUBISHI_FAN_3L ? climate::CLIMATE_FAN_MEDIUM : climate::CLIMATE_FAN_MIDDLE,
      this->fan_mode_ == MITSUBISHI_FAN_3L ? climate::CLIMATE_FAN_HIGH : climate::CLIMATE_FAN_MEDIUM,
      climate::CLIMATE_FAN_HIGH,
      climate::CLIMATE_FAN_QUIET,
      climate::CLIMATE_FAN_AUTO,
      climate::CLIMATE_FAN_AUTO};
  this->fan_mode = modes_mapping[fan];

  // Wide Vane
  uint8_t wide_vane = state_frame[8] & 0xF0;  // Bits 4,5,6,7
  switch (wide_vane) {
    case MITSUBISHI_WIDE_VANE_SWING:
      this->swing_mode = climate::CLIMATE_SWING_HORIZONTAL;
      break;
    default:
      this->swing_mode = climate::CLIMATE_SWING_OFF;
      break;
  }

  // Vertical Vane
  uint8_t vertical_vane = state_frame[9] & 0x38;  // Bits 3,4,5
  switch (vertical_vane) {
    case MITSUBISHI_VERTICAL_VANE_SWING:
      if (this->swing_mode == climate::CLIMATE_SWING_HORIZONTAL) {
        this->swing_mode = climate::CLIMATE_SWING_BOTH;
      } else {
        this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
      }
      break;
  }

  switch (state_frame[14]) {
    case MITSUBISHI_ECONOCOOL:
      this->preset = climate::CLIMATE_PRESET_ECO;
      break;
    case MITSUBISHI_NIGHTMODE:
      this->preset = climate::CLIMATE_PRESET_SLEEP;
      break;
  }

  ESP_LOGV(TAG, "Receiving: %s", format_hex_pretty(state_frame, 18).c_str());

  this->publish_state();
  return true;
}

}  // namespace mitsubishi
}  // namespace esphome
