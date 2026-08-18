# SPDX-License-Identifier: MIT

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import display, spi
from esphome.const import (
    CONF_AUTO_CLEAR_ENABLED,
    CONF_BUSY_PIN,
    CONF_DC_PIN,
    CONF_ID,
    CONF_LAMBDA,
    CONF_MODEL,
    CONF_RESET_PIN,
)

CODEOWNERS = ["@AStoker"]
DEPENDENCIES = ["spi"]

waveshare_4in2b_v2_ns = cg.esphome_ns.namespace("waveshare_4in2b_v2")
Waveshare4In2BV2 = waveshare_4in2b_v2_ns.class_(
    "Waveshare4In2BV2", cg.PollingComponent, spi.SPIDevice, display.DisplayBuffer
)
Model = waveshare_4in2b_v2_ns.enum("Model")

MODELS = {
    "auto": Model.MODEL_AUTO,
    "old": Model.MODEL_OLD,
    "new": Model.MODEL_NEW,
}

CONFIG_SCHEMA = (
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(Waveshare4In2BV2),
            cv.Required(CONF_DC_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_RESET_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_BUSY_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_MODEL, default="auto"): cv.enum(MODELS, lower=True),
            # ESPHome's clear colour is COLOR_OFF == Color(0, 0, 0), which is black *ink* on a
            # three-colour panel, so the usual auto-clear would repaint the screen black before
            # every frame. Clear explicitly with it.fill(Color(255, 255, 255)) instead.
            cv.Optional(CONF_AUTO_CLEAR_ENABLED, default=False): cv.boolean,
        }
    )
    .extend(cv.polling_component_schema("5min"))
    .extend(spi.spi_device_schema(cs_pin_required=True))
)

FINAL_VALIDATE_SCHEMA = spi.final_validate_device_schema(
    "waveshare_4in2b_v2", require_mosi=True, require_miso=False
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await display.register_display(var, config)
    await spi.register_spi_device(var, config, write_only=True)

    cg.add(var.set_dc_pin(await cg.gpio_pin_expression(config[CONF_DC_PIN])))
    cg.add(var.set_reset_pin(await cg.gpio_pin_expression(config[CONF_RESET_PIN])))
    cg.add(var.set_busy_pin(await cg.gpio_pin_expression(config[CONF_BUSY_PIN])))
    cg.add(var.set_model(config[CONF_MODEL]))

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(display.DisplayRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))
