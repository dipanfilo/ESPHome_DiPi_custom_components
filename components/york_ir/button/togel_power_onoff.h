#pragma once

#include "esphome/components/button/button.h"
#include "../york_ir.h"

namespace esphome {
namespace york_ir {

class TogelPowerOnOff : public button::Button, public Parented<YorkClimateIR> {
 public:
  TogelPowerOnOff() = default;

 protected:
  void press_action() override;
};

}  // namespace york_ir
}  // namespace esphome
