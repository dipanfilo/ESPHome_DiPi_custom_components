import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import climate_ir
from esphome.const import (
    CONF_HEAT_ACTION,
    CONF_ID,
    )

AUTO_LOAD = ["climate_ir"]
CODEOWNERS = ["@panwil"]

york_ir_ns = cg.esphome_ns.namespace("york_ir")
YorkIR = york_ir_ns.class_("YorkIR", climate_ir.ClimateIR)


CONFIG_SCHEMA = climate_ir.CLIMATE_IR_WITH_RECEIVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(YorkIR),
        cv.Optional(CONF_HEAT_ACTION): automation.validate_automation(single=False),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await climate_ir.register_climate_ir(var, config)
#    cg.add(var.set_fahrenheit(config[CONF_USE_FAHRENHEIT]))
