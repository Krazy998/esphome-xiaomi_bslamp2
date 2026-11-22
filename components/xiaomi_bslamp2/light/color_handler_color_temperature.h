#pragma once

#include "../common.h"
#include "../light_hal.h"
#include "color_handler.h"
#include "color_model.h"

namespace esphome {
namespace xiaomi {
namespace bslamp2 {

/**
 * The minimum color temperature in mired.
 * Same as supported by the original firmware.
 */
static const int MIRED_MIN = 153;

/**
 * The maximum color temperature in mired.
 * Same as supported by the original firmware.
 */
static const int MIRED_MAX = 588;

/**
 * This class can handle the GPIO outputs for the white light mode,
 * based on color temperature + brightness.
 *
 * The duty cycles are computed with the colour model of the original
 * firmware (see color_model.h), using the calibration data for the
 * selected hardware variant.
 */
class ColorHandlerColorTemperature : public ColorHandler {
 public:
  void set_color_model(const ColorModel *model) {
    if (model != nullptr)
      color_model_ = model;
  }

  bool set_light_color_values(light::LightColorValues v) {
    light_mode = LIGHT_MODE_WHITE;

    if (v.get_color_mode() != light::ColorMode::COLOR_TEMPERATURE) {
      return false;
    }

    auto duty = color_model_ct(*color_model_, clamp_temperature_(v.get_color_temperature()),
                               clamp_brightness_(v.get_brightness()));

    // The RGB channels are driven inverted (1.0 = off), white is not.
    red = 1.0f - duty.r;
    green = 1.0f - duty.g;
    blue = 1.0f - duty.b;
    white = duty.w;

    return true;
  }

 protected:
  const ColorModel *color_model_{&COLOR_MODEL_DEFAULT};

  float clamp_temperature_(float temperature) {
    if (temperature > MIRED_MAX)
      temperature = MIRED_MAX;
    else if (temperature < MIRED_MIN)
      temperature = MIRED_MIN;
    return temperature;
  }

  float clamp_brightness_(float brightness) {
    if (brightness < 0.01f)
      brightness = 0.01f;
    else if (brightness > 1.00f)
      brightness = 1.00f;
    return brightness;
  }
};

}  // namespace bslamp2
}  // namespace xiaomi
}  // namespace esphome
