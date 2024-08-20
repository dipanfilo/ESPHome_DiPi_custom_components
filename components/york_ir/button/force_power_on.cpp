#include "force_power_on.h"

namespace esphome {
namespace york_ir {

void ForcePowerOn::press_action() { this->parent_->button_force_power_on(); }

}  // namespace york_ir
}  // namespace esphome
