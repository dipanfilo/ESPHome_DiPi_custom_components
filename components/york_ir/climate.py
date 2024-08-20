import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import climate, sensor, remote_base, button
from esphome.const import CONF_SENSOR, CONF_ID

AUTO_LOAD = ["sensor", "remote_base"]
DEPENDENCIES = ["climate", "remote_transmitter"]

CODEOWNERS = ["@panwil"]



york_ir_ns = cg.esphome_ns.namespace("york_ir")
YorkClimateIR = york_ir_ns.class_(
                                    "YorkClimateIR",
                                    climate.Climate,
                                    cg.PollingComponent,
                                    remote_base.RemoteReceiverListener,
                                    remote_base.RemoteTransmittable,
                                )


CONF_YORK_ID = "york_ir_id"
CONF_IGNOR_RX_AFTER_TX = "ignore_rx_after_tx_ms"
CONF_DELAY_UPDATE_AFTER_FORZE_POWER_BUTTON = "delay_updata_after_forze_power_button_s"

CONFIG_SCHEMA = cv.All(
    climate.CLIMATE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(YorkClimateIR),
            cv.Optional(CONF_SENSOR): cv.use_id(sensor.Sensor),
            cv.Optional(remote_base.CONF_RECEIVER_ID): cv.use_id(remote_base.RemoteReceiverBase),
            cv.Optional(CONF_IGNOR_RX_AFTER_TX, default=500, ): cv.int_range(min = 1),
            cv.Optional(CONF_DELAY_UPDATE_AFTER_FORZE_POWER_BUTTON, default=90, ): cv.int_range(min = 60),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(remote_base.REMOTE_TRANSMITTABLE_SCHEMA)
    .extend(cv.polling_component_schema("100ms"))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
    await remote_base.register_transmittable(var, config)
    if remote_base.CONF_RECEIVER_ID in config:
        await remote_base.register_listener(var, config)
    if sensor_id := config.get(CONF_SENSOR):
        sens = await cg.get_variable(sensor_id)
        cg.add(var.set_sensor(sens))
    if CONF_IGNOR_RX_AFTER_TX in config:
        cg.add(
            var.set_ignore_rx_after_tx(config[CONF_IGNOR_RX_AFTER_TX])
        )
    if CONF_DELAY_UPDATE_AFTER_FORZE_POWER_BUTTON in config:
        cg.add(
            var.set_delay_after_power_forze_button(config[CONF_DELAY_UPDATE_AFTER_FORZE_POWER_BUTTON])
        )
    


