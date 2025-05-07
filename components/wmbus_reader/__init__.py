import esphome.config_validation as cv
import esphome.codegen as cg

from esphome.const import CONF_PAGES, CONF_DISPLAY_ID
from esphome.components.display import Display
from esphome.components import font, image

CONF_MANAGER_ID = "manager_id"
BaseSensor = cg.esphome_ns.namespace("wmbus_meter").class_("BaseSensor", cg.Component)
ScreenManager = cg.esphome_ns.namespace("wmbus_reader").class_("DisplayScreenManager")


CODEOWNERS = ["@kubasaw"]
AUTO_LOAD = ["image", "font"]

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_MANAGER_ID): cv.declare_id(ScreenManager),
        cv.GenerateID(CONF_DISPLAY_ID): cv.use_id(Display),
        cv.Optional(CONF_PAGES): [cv.use_id(BaseSensor)],
    }
)

font.MULTI_CONF_NO_DEFAULT = image.MULTI_CONF_NO_DEFAULT = True


async def to_code(config):
    cg.add_define("ESPHOME_PROJECT_NAME", "IoTLabs.wM-Bus Reader")
    cg.add_define("ESPHOME_PROJECT_VERSION", "1.0.0")
    cg.add_define("ESPHOME_PROJECT_VERSION_30", "1.0.0")

    manager = cg.new_Pvariable(
        config[CONF_MANAGER_ID],
        await cg.get_variable(config[CONF_DISPLAY_ID]),
    )

    for page in config.get(CONF_PAGES, []):
        cg.add(manager.add_sensor(await cg.get_variable(page)))

    cg.add(manager.sync())
