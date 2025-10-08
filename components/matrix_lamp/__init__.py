"""Matrix Lamp component for ESPHome."""

import io
import json
import logging
from pathlib import Path
from urllib.parse import urlparse

import esphome.codegen as cg
import esphome.components.image as esp_image
import esphome.config_validation as cv
import requests
from esphome import automation, core
from esphome.components import display
from esphome.components.image import CONF_ALPHA_CHANNEL, IMAGE_TYPE
from esphome.components.light.effects import register_addressable_effect
from esphome.components.light.types import AddressableLightEffect
from esphome.components.template.number import TemplateNumber
from esphome.const import (
    CONF_DISPLAY,
    CONF_FILE,
    CONF_HEIGHT,
    CONF_ID,
    CONF_MODE,
    CONF_NAME,
    CONF_RANDOM,
    CONF_RAW_DATA_ID,
    CONF_RESIZE,
    CONF_TRIGGER_ID,
    CONF_URL,
    CONF_WIDTH,
)
from esphome.core import CORE, HexInt
from esphome.cpp_generator import RawExpression

from .const import (
    CONF_BITMAP,
    CONF_CACHE,
    CONF_FRAMEDURATION,
    CONF_FRAMEINTERVAL,
    CONF_ICONS,
    CONF_INTENSITY_ID,
    CONF_LAMEID,
    CONF_MATRIX_ID,
    CONF_MATRIX_TYPE,
    CONF_ON_EFFECT_START,
    CONF_ORIENTATION,
    CONF_PINGPONG,
    CONF_RGB565ARRAY,
    CONF_SCALE_ID,
    CONF_SPEED_ID,
    DEF_TYPE,
    EFF_DNA,
    ICONHEIGHT,
    ICONWIDTH,
    IS_8X8,
    MATRIX_LAMP_EFFECTS,
    MAXFRAMES,
    MAXICONS,
)

_LOGGER = logging.getLogger(__name__)

CODEOWNERS = ["@andrewjswan"]

DEPENDENCIES = ["light", "fastled_helper"]

AUTO_LOAD = ["matrix_lamp", "image", "animation", "display"]

logging.info("Load Matrix Lamp component https://github.com/andrewjswan/matrix-lamp")

matrix_lamp_ns = cg.esphome_ns.namespace("matrix_lamp")
MATRIX_LAMP = matrix_lamp_ns.class_("MatrixLamp", cg.Component)
MATRIX_EFECT = matrix_lamp_ns.class_("MatrixLampLightEffect", AddressableLightEffect)
MATRIX_ICONS = matrix_lamp_ns.class_("MatrixLamp_Icon")

EffectStartTrigger = matrix_lamp_ns.class_(
    "MatrixLampEffectStartTrigger",
    automation.Trigger.template(cg.std_string),
)

MATRIX_LAMP_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(MATRIX_LAMP),
        cv.Optional(CONF_DISPLAY): cv.use_id(display),
        cv.Optional(CONF_WIDTH, default="16"): cv.templatable(cv.positive_int),
        cv.Optional(CONF_HEIGHT, default="16"): cv.templatable(cv.positive_int),
        cv.Optional(CONF_RANDOM, default=False): cv.templatable(cv.boolean),
        cv.Optional(CONF_ORIENTATION): cv.templatable(cv.int_range(min=0, max=7)),
        cv.Optional(CONF_MATRIX_TYPE): cv.templatable(cv.int_range(min=0, max=1)),
        cv.Optional(CONF_INTENSITY_ID): cv.use_id(TemplateNumber),
        cv.Optional(CONF_SCALE_ID): cv.use_id(TemplateNumber),
        cv.Optional(CONF_SPEED_ID): cv.use_id(TemplateNumber),
        cv.Optional(CONF_BITMAP, default=False): cv.boolean,
        cv.Optional(CONF_ICONS): cv.All(
            cv.ensure_list(
                {
                    cv.Required(CONF_ID): cv.declare_id(MATRIX_ICONS),
                    cv.Exclusive(CONF_FILE, "uri"): cv.file_,
                    cv.Exclusive(CONF_URL, "uri"): cv.url,
                    cv.Exclusive(CONF_LAMEID, "uri"): cv.string,
                    cv.Exclusive(CONF_RGB565ARRAY, "uri"): cv.string,
                    cv.Optional(CONF_RESIZE): cv.dimensions,
                    cv.Optional(CONF_FRAMEDURATION, default="0"): cv.templatable(cv.positive_int),
                    cv.Optional(CONF_PINGPONG, default=False): cv.boolean,
                    cv.GenerateID(CONF_RAW_DATA_ID): cv.declare_id(cg.uint8),
                },
            ),
            cv.Length(max=MAXICONS),
        ),
        cv.Optional(CONF_CACHE, default=False): cv.templatable(cv.boolean),
        cv.Optional(CONF_FRAMEINTERVAL, default="192"): cv.templatable(cv.positive_int),
        cv.Optional(CONF_ON_EFFECT_START): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(EffectStartTrigger),
            },
        ),
    },
)

CONFIG_SCHEMA = cv.All(MATRIX_LAMP_SCHEMA)


async def to_code(config) -> None:  # noqa: ANN001 C901 PLR0912 PLR0915
    """Code generation entry point."""
    var = cg.new_Pvariable(config[CONF_ID])

    cg.add_library("fastled/FastLED", "3.10.1")

    cg.add_define("WIDTH", config[CONF_WIDTH])
    cg.add_define("HEIGHT", config[CONF_HEIGHT])

    if CONF_INTENSITY_ID in config:
        intensity_number = await cg.get_variable(config[CONF_INTENSITY_ID])
        cg.add(var.set_intensity(intensity_number))
    if CONF_SCALE_ID in config:
        scale_number = await cg.get_variable(config[CONF_SCALE_ID])
        cg.add(var.set_scale(scale_number))
    if CONF_SPEED_ID in config:
        speed_number = await cg.get_variable(config[CONF_SPEED_ID])
        cg.add(var.set_speed(speed_number))

    if CONF_ORIENTATION in config:
        cg.add_define("ORIENTATION", config[CONF_ORIENTATION])
    if CONF_MATRIX_TYPE in config:
        cg.add_define("MATRIX_TYPE", config[CONF_MATRIX_TYPE])

    if config[CONF_RANDOM]:
        cg.add_define("RANDOM_SETTINGS_IN_CYCLE_MODE")
        logging.info("[X] Random mode")

    if CONF_DISPLAY in config:
        cg.add_define("MATRIX_LAMP_USE_DISPLAY")
        disp = await cg.get_variable(config[CONF_DISPLAY])
        cg.add(var.set_display(disp))
        logging.info("[X] Display")

    if CONF_DISPLAY in config and config[CONF_BITMAP]:
        cg.add_define("MATRIX_LAMP_BITMAP_MODE")
        logging.info("[X] Bitmap mode")

    if CONF_DISPLAY in config:
        cg.add_define("MAXICONS", MAXICONS)

        from PIL import Image  # noqa: PLC0415

        def rgb565_888(v565):  # noqa: ANN001 ANN202
            """RGB 565 to 888."""
            b = ((v565) & 0x001F) << 3
            g = ((v565) & 0x07E0) >> 3
            r = ((v565) & 0xF800) >> 8
            return (r, g, b)

        logging.info("Icons: Preparing icons, this may take some seconds.")

        yaml_string = ""
        icon_count = 0

        for conf in config[CONF_ICONS]:
            if CONF_FILE in conf:
                path = CORE.relative_config_path(conf[CONF_FILE])
                try:
                    image = Image.open(path)
                except Exception as e:  # noqa: BLE001
                    msg = f" Icons: Could not load image file {path}: {e}"
                    raise core.EsphomeError(msg)  # noqa: B904

            elif CONF_LAMEID in conf:
                path = CORE.relative_config_path(".cache/icons/lameid/" + conf[CONF_LAMEID])
                if config[CONF_CACHE] and Path(path).is_file():
                    try:
                        image = Image.open(path)
                        logging.info(" Icons: Load %s from cache.", conf[CONF_LAMEID])
                    except Exception as e:  # noqa: BLE001
                        msg = f" Icons: Could not load image file {path}: {e}"
                        raise core.EsphomeError(msg)  # noqa: B904
                else:
                    r = requests.get(
                        "https://developer.lametric.com/content/apps/icon_thumbs/"
                        + conf[CONF_LAMEID],
                        headers={
                            "Cache-Control": "no-cache, no-store, must-revalidate",
                            "Pragma": "no-cache",
                        },
                        timeout=4.0,
                    )
                    if r.status_code != requests.codes.ok:
                        msg = (
                            " Icons: Could not download image file "
                            f"{conf[CONF_LAMEID]}: {conf[CONF_ID]}"
                        )
                        raise core.EsphomeError(msg)
                    image = Image.open(io.BytesIO(r.content))

                    if config[CONF_CACHE]:
                        Path(Path(path).parent).mkdir(parents=True, exist_ok=True)
                        with Path(path).open(mode="wb") as f:
                            f.write(r.content)
                            f.close()
                        logging.info(" Icons: Save %s to cache.", conf[CONF_LAMEID])

            elif CONF_URL in conf:
                a = urlparse(conf[CONF_URL])
                path = CORE.relative_config_path(".cache/icons/url/" + Path(a.path).name)
                if config[CONF_CACHE] and Path(path).is_file():
                    try:
                        image = Image.open(path)
                        logging.info(" Icons: Load %s from cache.", conf[CONF_URL])
                    except Exception as e:  # noqa: BLE001
                        msg = f" Icons: Could not load image file {path}: {e}"
                        raise core.EsphomeError(msg)  # noqa: B904
                else:
                    r = requests.get(
                        conf[CONF_URL],
                        headers={
                            "Cache-Control": "no-cache, no-store, must-revalidate",
                            "Pragma": "no-cache",
                        },
                        timeout=4.0,
                    )
                    if r.status_code != requests.codes.ok:
                        msg = (
                            " Icons: Could not download image file "
                            f"{conf[CONF_URL]}: {conf[CONF_ID]}"
                        )
                        raise core.EsphomeError(msg)
                    image = Image.open(io.BytesIO(r.content))

                    if config[CONF_CACHE]:
                        Path(Path(path).parent).mkdir(parents=True, exist_ok=True)
                        with Path(path).open(mode="wb") as f:
                            f.write(r.content)
                            f.close()
                        logging.info(" Icons: Save %s to cache.", conf[CONF_URL])

            elif CONF_RGB565ARRAY in conf:
                r = list(json.loads(conf[CONF_RGB565ARRAY]))
                if len(r) == IS_8X8:
                    image = Image.new("RGB", [8, 8])
                    for y in range(8):
                        for x in range(8):
                            image.putpixel((x, y), rgb565_888(r[x + y * 8]))

            width, height = image.size

            if CONF_RESIZE in conf:
                new_width_max, new_height_max = conf[CONF_RESIZE]
                ratio = min(new_width_max / width, new_height_max / height)
                width, height = int(width * ratio), int(height * ratio)

            if hasattr(image, "n_frames"):
                frame_count = min(image.n_frames, MAXFRAMES)
                logging.info(" Icons: Animation %s with %s frame(s)", conf[CONF_ID], frame_count)
            else:
                frame_count = 1

            if (width != ICONWIDTH) and (height != ICONHEIGHT):
                logging.warning(" Icons: Icon wrong size valid 8x8: %s skipped!", conf[CONF_ID])
            else:
                if conf[CONF_FRAMEDURATION] == 0:
                    try:
                        duration = image.info["duration"]
                    except:  # noqa: E722
                        duration = config[CONF_FRAMEINTERVAL]
                else:
                    duration = conf[CONF_FRAMEDURATION]

                yaml_string += f'"{conf[CONF_ID]}",'
                icon_count += 1

                dither = Image.Dither.NONE
                transparency = CONF_ALPHA_CHANNEL
                invert_alpha = False

                total_rows = height * frame_count
                encoder = IMAGE_TYPE[DEF_TYPE](
                    width,
                    total_rows,
                    transparency,
                    dither,
                    invert_alpha,
                )
                for frame_index in range(frame_count):
                    image.seek(frame_index)
                    pixels = encoder.convert(image.resize((width, height)), path).getdata()
                    for row in range(height):
                        for col in range(width):
                            encoder.encode(pixels[row * width + col])
                        encoder.end_row()

                rhs = [HexInt(x) for x in encoder.data]
                prog_arr = cg.progmem_array(conf[CONF_RAW_DATA_ID], rhs)

                image_type = esp_image.get_image_type_enum(DEF_TYPE)
                trans_value = esp_image.get_transparency_enum(encoder.transparency)

                logging.debug("Icons: icon %s %s", conf[CONF_ID], rhs)

                cg.new_Pvariable(
                    conf[CONF_ID],
                    prog_arr,
                    width,
                    height,
                    frame_count,
                    image_type,
                    str(conf[CONF_ID]),
                    conf[CONF_PINGPONG],
                    duration,
                    trans_value,
                )
                cg.add(var.add_icon(RawExpression(str(conf[CONF_ID]))))

        logging.info(
            "Icons: [%s] List of icons for e.g. blueprint:\n\n\r[%s]\n",
            icon_count,
            yaml_string,
        )

    if config.get(CONF_ON_EFFECT_START, []):
        cg.add_define("MATRIX_LAMP_TRIGGERS")
        logging.info("[X] On Effect Start trigger")
        for conf in config.get(CONF_ON_EFFECT_START, []):
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
            await automation.build_automation(
                trigger,
                [
                    (cg.uint8, "effect"),
                    (cg.uint8, "intensity"),
                    (cg.uint8, "speed"),
                    (cg.uint8, "scale"),
                ],
                conf,
            )

    await cg.register_component(var, config)


@register_addressable_effect(
    "matrix_lamp_effect",
    MATRIX_EFECT,
    "Matrix Lamp",
    {
        cv.GenerateID(CONF_MATRIX_ID): cv.use_id(MATRIX_LAMP),
        cv.Optional(CONF_MODE, default=EFF_DNA): cv.one_of(*MATRIX_LAMP_EFFECTS, upper=True),
    },
)
async def matrix_lamp_light_effect_to_code(config, effect_id) -> AddressableLightEffect:  # noqa: ANN001
    """Effect registration entry point."""
    parent = await cg.get_variable(config[CONF_MATRIX_ID])

    effect = cg.new_Pvariable(effect_id, config[CONF_NAME])

    cg.add(effect.set_mode(MATRIX_LAMP_EFFECTS.index(config[CONF_MODE])))
    cg.add_define("DEF_" + config[CONF_MODE])
    cg.add(effect.set_matrix_lamp(parent))
    logging.debug(
        "Effect: %s [%d] %s",
        config[CONF_MODE],
        MATRIX_LAMP_EFFECTS.index(config[CONF_MODE]),
        "DEF_" + config[CONF_MODE],
    )
    return effect
