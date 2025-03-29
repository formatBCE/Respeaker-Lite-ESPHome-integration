import hashlib
from pathlib import Path
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome import automation, core, external_files
from esphome.components import i2c, binary_sensor, text_sensor
from esphome.const import (
    CONF_ID,
    CONF_ON_ERROR,
    CONF_RAW_DATA_ID,
    CONF_RESET_PIN,
    CONF_TRIGGER_ID,
    CONF_URL,
    CONF_VERSION,
)
from esphome.core import HexInt

AUTO_LOAD = [ "binary_sensor", "text_sensor" ]
CODEOWNERS = ["@iDJMic"]
DEPENDENCIES = ["i2c"]

CONF_MUTE_STATE= "mute_state"
CONF_FIRMWARE_VERSION= "firmware_version"
CONF_FIRMWARE = "firmware"
CONF_MD5 = "md5"
CONF_ON_BEGIN = "on_begin"
CONF_ON_END = "on_end"
CONF_ON_PROGRESS = "on_progress"

DOMAIN = "respeaker_lite"

respeaker_lite_ns = cg.esphome_ns.namespace("respeaker_lite")
RespeakerLite = respeaker_lite_ns.class_("RespeakerLite", i2c.I2CDevice, cg.Component)
RespeakerLiteFlashAction = respeaker_lite_ns.class_("RespeakerLiteFlashAction", automation.Action)

DFUEndTrigger = respeaker_lite_ns.class_("DFUEndTrigger", automation.Trigger.template())
DFUErrorTrigger = respeaker_lite_ns.class_("DFUErrorTrigger", automation.Trigger.template())
DFUProgressTrigger = respeaker_lite_ns.class_(
    "DFUProgressTrigger", automation.Trigger.template()
)
DFUStartTrigger = respeaker_lite_ns.class_("DFUStartTrigger", automation.Trigger.template())


def _compute_local_file_path(url: str) -> Path:
    h = hashlib.new("sha256")
    h.update(url.encode())
    key = h.hexdigest()[:8]
    base_dir = external_files.compute_local_file_dir(DOMAIN)
    return base_dir / key


def download_firmware(config):
    url = config[CONF_URL]
    path = _compute_local_file_path(url)
    external_files.download_content(url, path)

    try:
        with open(path, "r+b") as f:
            firmware_bin = f.read()
    except FileNotFoundError as e:
        raise cv.Invalid(f"Could not open firmware file {path}: {e}") from e

    firmware_bin_md5 = hashlib.md5(firmware_bin).hexdigest()
    if firmware_bin_md5 != config[CONF_MD5]:
        raise cv.Invalid(f"{CONF_MD5} is not consistent with file contents")

    return config


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(RespeakerLite),
            cv.Required(CONF_RESET_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_MUTE_STATE): binary_sensor.binary_sensor_schema(),
            cv.Optional(CONF_FIRMWARE_VERSION): text_sensor.text_sensor_schema(),
            cv.GenerateID(CONF_RAW_DATA_ID): cv.declare_id(cg.uint8),
            cv.Optional(CONF_FIRMWARE): cv.All(
                {
                    cv.Required(CONF_URL): cv.url,
                    cv.Required(CONF_VERSION): cv.version_number,
                    cv.Required(CONF_MD5): cv.All(cv.string, cv.Length(min=32, max=32)),
                    cv.Optional(CONF_ON_BEGIN): automation.validate_automation(
                        {
                            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                                DFUStartTrigger
                            ),
                        }
                    ),
                    cv.Optional(CONF_ON_END): automation.validate_automation(
                        {
                            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                                DFUEndTrigger
                            ),
                        }
                    ),
                    cv.Optional(CONF_ON_ERROR): automation.validate_automation(
                        {
                            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                                DFUErrorTrigger
                            ),
                        }
                    ),
                    cv.Optional(CONF_ON_PROGRESS): automation.validate_automation(
                        {
                            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                                DFUProgressTrigger
                            ),
                        }
                    ),
                },
                download_firmware,
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x42))
)


OTA_RESPEAKER_LITE_FLASH_ACTION_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(RespeakerLite),
    }
)


@automation.register_action(
    "respeaker_lite.flash",
    RespeakerLiteFlashAction,
    OTA_RESPEAKER_LITE_FLASH_ACTION_SCHEMA,
)
async def respeaker_lite_flash_action_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)

    return var


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    pin = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
    cg.add(var.set_reset_pin(pin))

    if CONF_MUTE_STATE in config:
        mute_state = await binary_sensor.new_binary_sensor(config[CONF_MUTE_STATE])
        cg.add(var.set_mute_state(mute_state))
    if CONF_FIRMWARE_VERSION in config:
        firmware_version = await text_sensor.new_text_sensor(config[CONF_FIRMWARE_VERSION])
        cg.add(var.set_firmware_version(firmware_version))

    if config_fw := config.get(CONF_FIRMWARE):
        firmware_version = config_fw[CONF_VERSION].split(".")
        path = _compute_local_file_path(config_fw[CONF_URL])

        try:
            with open(path, "r+b") as f:
                firmware_bin = f.read()
        except FileNotFoundError as e:
            raise core.EsphomeError(f"Could not open firmware file {path}: {e}")

        # Convert retrieved binary file to an array of ints
        rhs = [HexInt(x) for x in firmware_bin]
        # Create an array which will reside in program memory and set the pointer to it
        firmware_bin_arr = cg.progmem_array(config[CONF_RAW_DATA_ID], rhs)
        cg.add(var.set_firmware_bin(firmware_bin_arr, len(rhs)))
        cg.add(
            var.set_firmware_version(
                int(firmware_version[0]),
                int(firmware_version[1]),
                int(firmware_version[2]),
            )
        )

        use_state_callback = False
        for conf in config_fw.get(CONF_ON_BEGIN, []):
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
            await automation.build_automation(trigger, [], conf)
            use_state_callback = True
        for conf in config_fw.get(CONF_ON_PROGRESS, []):
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
            await automation.build_automation(trigger, [(float, "x")], conf)
            use_state_callback = True
        for conf in config_fw.get(CONF_ON_END, []):
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
            await automation.build_automation(trigger, [], conf)
            use_state_callback = True
        for conf in config_fw.get(CONF_ON_ERROR, []):
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
            await automation.build_automation(trigger, [(cg.uint8, "x")], conf)
            use_state_callback = True
        if use_state_callback:
            cg.add_define("USE_RESPEAKER_LITE_STATE_CALLBACK")