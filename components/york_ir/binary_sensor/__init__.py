import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_FAN,
)
from ..climate import (
    CONF_YORK_ID,
    YorkClimateIR,
)

CODEOWNERS = ["@panwil"]
BinarySensorTypeEnum = YorkClimateIR.enum("SubBinarySensorType", True)

# York AC PowerOn sensors
CONF_POWER_ON_STATUS = "power_on_status"

SENSOR_TYPES = {
    CONF_POWER_ON_STATUS: binary_sensor.binary_sensor_schema(
        icon=ICON_FAN,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),

}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_YORK_ID): cv.use_id(YorkClimateIR),
    }
).extend({cv.Optional(type): schema for type, schema in SENSOR_TYPES.items()})


async def to_code(config):
    paren = await cg.get_variable(config[CONF_YORK_ID])

    for type_ in SENSOR_TYPES:
        if conf := config.get(type_):
            sens = await binary_sensor.new_binary_sensor(conf)
            binary_sensor_type = getattr(BinarySensorTypeEnum, type_.upper())
            cg.add(paren.set_sub_binary_sensor(binary_sensor_type, sens))
