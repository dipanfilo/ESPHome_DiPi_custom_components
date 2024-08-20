#include "force_power_off.h"

namespace esphome {
namespace york_ir {

void ForcePowerOff::press_action() { this->parent_->button_force_power_off(); }

}  // namespace york_ir
}  // namespace esphome
