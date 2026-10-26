import esphome.codegen as cg
from esphome.components import audio, microphone
import esphome.config_validation as cv
from esphome.const import (
    CONF_BITS_PER_SAMPLE,
    CONF_CHANNELS,
    CONF_ID,
    CONF_MICROPHONE,
)

from .. import decimator_ns

AUTO_LOAD = ["audio"]
DEPENDENCIES = ["microphone"]

# The XMOS firmware of the ReSpeaker Lite and the XVF3800 processes at 16 kHz and outputs 48 kHz over I2S.
SOURCE_SAMPLE_RATE = 48000
SAMPLE_RATE = 16000

DecimatorMicrophone = decimator_ns.class_(
    "DecimatorMicrophone", cg.Component, microphone.Microphone
)


def _set_stream_limits(config):
    # Only the sample rate changes; the bits and channels are those selected from the source microphone
    source = config[CONF_MICROPHONE]
    audio.set_stream_limits(
        min_bits_per_sample=source[CONF_BITS_PER_SAMPLE],
        max_bits_per_sample=source[CONF_BITS_PER_SAMPLE],
        min_channels=len(source[CONF_CHANNELS]),
        max_channels=len(source[CONF_CHANNELS]),
        min_sample_rate=SAMPLE_RATE,
        max_sample_rate=SAMPLE_RATE,
    )(config)
    return config


CONFIG_SCHEMA = cv.All(
    microphone.MICROPHONE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(DecimatorMicrophone),
            cv.Required(CONF_MICROPHONE): microphone.microphone_source_schema(
                min_bits_per_sample=16,
                max_bits_per_sample=32,
                min_channels=1,
                max_channels=2,
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    _set_stream_limits,
)

FINAL_VALIDATE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_MICROPHONE): microphone.final_validate_microphone_source_schema(
            "decimator", sample_rate=SOURCE_SAMPLE_RATE
        ),
    },
    extra=cv.ALLOW_EXTRA,
)


async def to_code(config):
    mic_source = await microphone.microphone_source_to_code(config[CONF_MICROPHONE])
    var = cg.new_Pvariable(config[CONF_ID], mic_source)
    await cg.register_component(var, config)
    await microphone.register_microphone(var, config)
