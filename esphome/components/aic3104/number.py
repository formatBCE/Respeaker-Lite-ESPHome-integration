import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import (
    ENTITY_CATEGORY_CONFIG,
    UNIT_DECIBEL,
)

from .audio_dac import AIC3104, aic3104_ns

CODEOWNERS = ["@formatBCE"]

CONF_AIC3104_ID = "aic3104_id"

# Optional output gain of the codec, 0 to +9 dB in 1 dB steps. There is no
# entity and no register write unless a user adds this platform. It is meant
# for an external amplifier on the line out: the on-board amplifier distorts
# above about 2.5-3 W, so the default stays at 0 dB.
#
# The gain is added after the DAC volume that the media player sets. A high
# gain with the volume near 100 % clips, so raise it one step at a time.
AIC3104OutputGain = aic3104_ns.class_(
    "AIC3104OutputGain", number.Number, cg.Component
)

CONFIG_SCHEMA = (
    number.number_schema(
        AIC3104OutputGain,
        entity_category=ENTITY_CATEGORY_CONFIG,
        unit_of_measurement=UNIT_DECIBEL,
        icon="mdi:volume-plus",
    )
    .extend(
        {
            cv.GenerateID(CONF_AIC3104_ID): cv.use_id(AIC3104),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await number.new_number(config, min_value=0, max_value=9, step=1)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_AIC3104_ID])
    cg.add(var.set_parent(parent))
