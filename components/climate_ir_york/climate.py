import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir
from esphome.const import CONF_ID

CODEOWNERS = ["@Rdipiwill"]
AUTO_LOAD = ["climate_ir"]

mitsubishi_ns = cg.esphome_ns.namespace("york")
MitsubishiClimate = mitsubishi_ns.class_("YorkClimate", climate_ir.ClimateIR)

CONF_SET_FAN_MODE = "set_fan_mode"
SetFanMode = mitsubishi_ns.enum("FanMode")
SETFANMODE = {
    "auto": FanMode.FAN_MODE_AUTO,
    "1levels": FanMode.FAN_MODE_MANUAL_SPEED_1,
    "2levels": FanMode.FAN_MODE_MANUAL_SPEED_2,
    "3levels": FanMode.FAN_MODE_MANUAL_SPEED_3,
    "quite": FanMode.FAN_MODE_QUIET,
    "turbo": FanMode.FAN_MODE_TURBO,
}


CONF_SUPPORTS_DRY = "supports_dry"
CONF_SUPPORTS_FAN_ONLY = "supports_fan_only"

CONF_VERTICAL_DEFAULT = "vertical_default"
VerticalDirections = mitsubishi_ns.enum("SwingModeVertical")
VERTICAL_DIRECTIONS = {
    "auto": SwingModeVertical.SWING_MODE_VERTICAL_AUTO,
    "off": SwingModeVertical.SWING_MODE_VERTICAL_OFF,
}


CONFIG_SCHEMA = climate_ir.CLIMATE_IR_WITH_RECEIVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(YorkClimate),
        cv.Optional(CONF_SET_FAN_MODE, default="2levels"): cv.enum(SETFANMODE),
        cv.Optional(CONF_SUPPORTS_DRY, default=True): cv.boolean,
        cv.Optional(CONF_SUPPORTS_FAN_ONLY, default=True): cv.boolean,
        cv.Optional(CONF_VERTICAL_DEFAULT, default="off"): cv.enum(
            VERTICAL_DIRECTIONS
        ),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await climate_ir.register_climate_ir(var, config)

    cg.add(var.set_fan_mode(config[CONF_SET_FAN_MODE]))
    cg.add(var.set_supports_dry(config[CONF_SUPPORTS_DRY]))
    cg.add(var.set_supports_fan_only(config[CONF_SUPPORTS_FAN_ONLY]))
    cg.add(var.set_vertical_default(config[CONF_VERTICAL_DEFAULT]))
