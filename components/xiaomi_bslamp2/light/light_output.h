#pragma once

#include "../common.h"
#include "../light_hal.h"
#include "color_handler_chain.h"
#include "light_transformer.h"
#include "esphome/core/component.h"
#include "esphome/components/ledc/ledc_output.h"
#include "color_model.h"
#include <vector>

#ifdef USE_ESP32
#include "esp_flash.h"
#endif

namespace esphome {
namespace xiaomi {
namespace bslamp2 {

/**
 * A LightOutput class for the Xiaomi Mijia Bedside Lamp 2.
 *
 * The function of this class is to translate a required light state
 * into actual physicial GPIO output signals to drive the device's LED
 * circuitry. It forms the glue between the physical device and the
 * logical light color input.
 */
class XiaomiBslamp2LightOutput : public Component, public light::LightOutput {
 public:
  void set_parent(LightHAL *light) { light_ = light; }

  /**
   * Select the colour model. HardwareVariant::AUTO tries to read the stock
   * firmware's "sub_model" setting from leftover data in flash and falls
   * back to HardwareVariant::DEFAULT when it cannot be found.
   */
  void set_hardware_variant(int variant) {
    configured_variant_ = static_cast<HardwareVariant>(variant);
    hardware_variant_ = configured_variant_;
    if (configured_variant_ == HardwareVariant::AUTO) {
      int sub_model = read_stock_sub_model_();
      if (sub_model == 0 || sub_model == 1) {
        hardware_variant_ = static_cast<HardwareVariant>(sub_model);
        auto_detected_ = true;
        ESP_LOGI(TAG, "Found stock firmware sub_model=%d in flash", sub_model);
      } else {
        hardware_variant_ = HardwareVariant::DEFAULT;
        ESP_LOGW(TAG, "hardware_variant auto: no stock firmware sub_model found in flash (got %d), "
                      "using 'default'. Set hardware_variant explicitly if colours look wrong.", sub_model);
      }
    }
    color_model_ = color_model_for(hardware_variant_);
    color_handler_chain.set_color_model(color_model_);
    ESP_LOGI(TAG, "Colour model: %s", hardware_variant_to_str(hardware_variant_));
    apply_current_state();
  }

  HardwareVariant get_hardware_variant() const { return hardware_variant_; }
  const char *get_hardware_variant_str() const { return hardware_variant_to_str(hardware_variant_); }
  bool is_hardware_variant_auto_detected() const { return auto_detected_; }

  void set_night_light_color_temperature_calibration(float red, float green, float blue) {
    night_light_calibration_ = {red, green, blue};
    color_handler_chain.set_night_light_color_temperature_calibration(night_light_calibration_);
    apply_current_state();
  }

  /**
   * Returns a LightTraits object, which is used to explain to the outside
   * world (e.g. Home Assistant) what features are supported by this device.
   */
  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::RGB, light::ColorMode::COLOR_TEMPERATURE});
    traits.set_min_mireds(MIRED_MIN);
    traits.set_max_mireds(MIRED_MAX);
    return traits;
  }

  std::unique_ptr<light::LightTransformer> create_default_transition() override {
    return make_unique<XiaomiBslamp2LightTransitionTransformer>(
      light_,
      &light_mode_callback_,
      &state_callback_,
      night_light_calibration_,
      color_model_);
  }

  void add_on_light_mode_callback(std::function<void(std::string)> &&callback) {
    light_mode_callback_.add(std::move(callback));
  }

  void add_on_state_callback(std::function<void(light::LightColorValues)> &&callback) {
    state_callback_.add(std::move(callback));
  }

  /**
   * Applies a requested light state to the physicial GPIO outputs.
   */
  void write_state(light::LightState *state) {
    state_ = state;
    auto values = state->current_values;

    color_handler_chain.set_light_color_values(values);
    light_mode_callback_.call(color_handler_chain.light_mode);
    state_callback_.call(values);

    // Note: one might think that it is more logical to turn on the LED
    // circuitry master switch after setting the individual channels,
    // but this is the order that was used by the original firmware. I
    // tried to stay as close as possible to the original behavior, so
    // that's why these GPIOs are turned on at this point.
    if (values.get_state() != 0)
      light_->turn_on();

    // Apply the GPIO output levels as defined by the color handler.
    light_->set_state(&color_handler_chain);

    if (values.get_state() == 0)
      light_->turn_off();
  }

  void apply_current_state() {
    if (state_ != nullptr)
      write_state(state_);
  }

 protected:
  /**
   * The stock firmware keeps its settings in the "minvs" partition at
   * 0x3f8000 (16 kB). Flashing ESPHome does not necessarily erase that
   * area, so the sub_model setting may still be readable. Note that the
   * ESPHome (esp-idf) NVS partition overlaps this area, so it may also
   * have been wiped. Returns -1 when not found.
   */
  int read_stock_sub_model_() {
#ifdef USE_ESP32
    static const uint32_t MINVS_ADDR = 0x3f8000;
    static const size_t MINVS_SIZE = 0x4000;
    std::vector<uint8_t> buf(MINVS_SIZE);
    if (esp_flash_read(nullptr, buf.data(), MINVS_ADDR, MINVS_SIZE) != ESP_OK) {
      ESP_LOGW(TAG, "Unable to read flash at 0x%06x", (unsigned) MINVS_ADDR);
      return -1;
    }
    return parse_stock_sub_model(buf.data(), buf.size());
#else
    return -1;
#endif
  }

  HardwareVariant configured_variant_{HardwareVariant::LEGACY};
  HardwareVariant hardware_variant_{HardwareVariant::LEGACY};
  bool auto_detected_{false};
  const ColorModel *color_model_{nullptr};
  LightHAL *light_;
  light::LightState *state_{nullptr};
  ColorHandlerChain color_handler_chain;
  CallbackManager<void(std::string)> light_mode_callback_{};
  CallbackManager<void(light::LightColorValues)> state_callback_{};
  NightLightCalibration night_light_calibration_{DEFAULT_NIGHT_LIGHT_CALIBRATION};
};

}  // namespace bslamp2
}  // namespace xiaomi
}  // namespace esphome
