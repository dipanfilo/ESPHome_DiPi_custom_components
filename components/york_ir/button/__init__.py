import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from ..climate import (
    CONF_YORK_ID,
    YorkClimateIR,
    york_ir_ns,
)

CODEOWNERS = ["@panwil"]
ForcePowerOn = york_ir_ns.class_("ForcePowerOn", button.Button)
ForcePowerOff = york_ir_ns.class_("ForcePowerOff", button.Button)
TogelPowerOnOff = york_ir_ns.class_("TogelPowerOnOff", button.Button)
DumpIRData = york_ir_ns.class_("DumpIRData", button.Button)

# York IR buttons
CONF_FORCE_AC_ON = "force_power_on"
CONF_FORCE_AC_OFF = "force_power_off"
CONF_TOGEL_AC_ONOFF = "togel_power_onoff"
CONF_DUMP_IR_DATA = "dump_ir_data"


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_YORK_ID): cv.use_id(YorkClimateIR),
        cv.Optional(CONF_FORCE_AC_ON): button.button_schema(
            ForcePowerOn,
        ),
        cv.Optional(CONF_FORCE_AC_OFF): button.button_schema(
            ForcePowerOff,
        ),
        cv.Optional(CONF_TOGEL_AC_ONOFF): button.button_schema(
            TogelPowerOnOff,
        ),
        cv.Optional(CONF_DUMP_IR_DATA): button.button_schema(
            DumpIRData,
        ),
    }
)


async def to_code(config):
    for button_type in [CONF_FORCE_AC_ON, CONF_FORCE_AC_OFF, CONF_TOGEL_AC_ONOFF, CONF_DUMP_IR_DATA]:
        if conf := config.get(button_type):
            btn = await button.new_button(conf)
            await cg.register_parented(btn, config[CONF_YORK_ID])
