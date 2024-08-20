#pragma once

#include "esphome/components/button/button.h"
#include "../york_ir.h"

namespace esphome {
namespace york_ir {

class ForcePowerOff : public button::Button, public Parented<YorkClimateIR> {
 public:
  ForcePowerOff() = default;

 protected:
  void press_action() override;
};

}  // namespace york_ir
}  // namespace esphome
