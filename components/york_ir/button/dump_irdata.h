#pragma once

#include "esphome/components/button/button.h"
#include "../york_ir.h"

namespace esphome {
namespace york_ir {

class DumpIRData : public button::Button, public Parented<YorkClimateIR> {
 public:
  DumpIRData() = default;

 protected:
  void press_action() override;
};

}  // namespace york_ir
}  // namespace esphome
