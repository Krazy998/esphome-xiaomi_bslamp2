#pragma once

#include "../common.h"
#include "../light_hal.h"
#include "color_handler.h"
#include "color_model.h"

namespace esphome {
namespace xiaomi {
namespace bslamp2 {

/**
 * This class can handle the GPIO outputs for the RGB light mode,
 * based on RGB color values + brightness.
 *
 * The duty cycles are computed with the colour model of the original
 * firmware (see color_model.h), using the calibration data for the
 * selected hardware variant.
 */
class ColorHandlerRGB : public ColorHandler {
 public:
  void set_color_model(const ColorModel *model) {
    if (model != nullptr)
      color_model_ = model;
  }

  bool set_light_color_values(light::LightColorValues v) {
    light_mode = LIGHT_MODE_RGB;

    if (v.get_color_mode() != light::ColorMode::RGB) {
      return false;
    }

    auto duty = color_model_rgb(*color_model_, v.get_red(), v.get_green(), v.get_blue(), v.get_brightness());

    // The RGB channels are driven inverted (1.0 = off).
    // The white output channel is always off for RGB.
    red = 1.0f - duty.r;
    green = 1.0f - duty.g;
    blue = 1.0f - duty.b;
    white = 0.0f;

    return true;
  }

 protected:
  const ColorModel *color_model_{&COLOR_MODEL_DEFAULT};
};

}  // namespace bslamp2
}  // namespace xiaomi
}  // namespace esphome
