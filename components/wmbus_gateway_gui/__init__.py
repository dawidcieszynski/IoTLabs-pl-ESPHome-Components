import esphome.config_validation as cv
import esphome.codegen as cg

from esphome.const import CONF_PAGES, CONF_DISPLAY_ID, CONF_ID

from esphome.components.binary_sensor import BinarySensor
from esphome.components.ssd1306_base import SSD1306

BaseSensor = cg.esphome_ns.namespace("wmbus_meter").class_("BaseSensor", cg.Component)
gui_ns = cg.esphome_ns.namespace("wmbus_gateway_gui")
DisplayManager = gui_ns.class_("DisplayManager", cg.PollingComponent)

CONF_BUTTON_ID = "button_id"

CODEOWNERS = ["@kubasaw"]
AUTO_LOAD = ["network"]

REGISTERED_SENSORS = []

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(DisplayManager),
        cv.GenerateID(CONF_DISPLAY_ID): cv.use_id(SSD1306),
        cv.GenerateID(CONF_BUTTON_ID): cv.use_id(BinarySensor),
        cv.Optional(CONF_PAGES, default=REGISTERED_SENSORS): [cv.use_id(BaseSensor)],
    }
)


def register_sensor_for_display(conf):
    if CONF_ID in conf:
        REGISTERED_SENSORS.append(conf[CONF_ID])
    return conf


async def to_code(config):
    cg.add_define("ESPHOME_PROJECT_NAME", "IoTLabs.wM-Bus Gateway")
    cg.add_define("ESPHOME_PROJECT_VERSION", "1.0.0")
    cg.add_define("ESPHOME_PROJECT_VERSION_30", "1.0.0")

    manager = cg.new_Pvariable(
        config[CONF_ID],
        await cg.get_variable(config[CONF_DISPLAY_ID]),
        await cg.get_variable(config[CONF_BUTTON_ID]),
    )

    for page in config[CONF_PAGES]:
        cg.add(manager.add_sensor(await cg.get_variable(page)))

    await cg.register_component(manager, config)
