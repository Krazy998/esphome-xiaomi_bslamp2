#pragma once

/**
 * Colour model of the original (Yeelight) Mi Bedside Lamp 2 firmware.
 *
 * This is a re-implementation of the colour mixing code of the stock
 * firmware (yeelink.light.bslamp2, "yglc" light controller), reverse
 * engineered from a full flash backup. See issue #126.
 *
 * The stock firmware supports two hardware variants (different LED sets),
 * each with its own calibration data. The variant is NOT detected from
 * the hardware: it is a setting called "sub_model" that is stored in the
 * Xiaomi NVS area of the flash (key "app_data.sub_model", written at the
 * factory / by the Yeelight cloud with the miIO "set_ps" command).
 *
 *   sub_model 0 -> HardwareVariant::DEFAULT
 *   sub_model 1 -> HardwareVariant::HONGLI  (firmware logs "light mixer for hongli")
 *
 * All values computed here are "on" duty cycles in the range 0.0 - 1.0,
 * in R, G, B, W order. The caller is responsible for converting these
 * into the GPIO output levels (the RGB channels of this lamp are driven
 * inverted, i.e. 1.0 = off).
 */

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace esphome {
namespace xiaomi {
namespace bslamp2 {

enum class HardwareVariant : int {
  LEGACY = -2,   // the pre-existing measured tables of this component
  AUTO = -1,     // try to read sub_model from leftover stock firmware data
  DEFAULT = 0,   // sub_model 0
  HONGLI = 1,    // sub_model 1
};

inline const char *hardware_variant_to_str(HardwareVariant v) {
  switch (v) {
    case HardwareVariant::LEGACY: return "legacy (measured tables)";
    case HardwareVariant::AUTO: return "auto";
    case HardwareVariant::DEFAULT: return "default (sub_model 0)";
    case HardwareVariant::HONGLI: return "hongli (sub_model 1)";
  }
  return "unknown";
}

struct LineEq {  // y = slope * x + intercept
  float slope;
  float intercept;
};

struct XY {
  float x;
  float y;
};

struct CTEntry {
  uint16_t kelvin_from;
  float r, g, b, w;  // percentages
};

struct ColorModel {
  // 3x3 matrix, solving [X/Y, 1, Z/Y] -> RGB channel weights.
  std::array<float, 9> rgb_matrix;
  // Gamut lines: 0..2 = lines through the white point, 3 = G-B edge, 4 = R-G edge, 5 = R-B edge.
  std::array<LineEq, 6> lines;
  XY prim_r, prim_g, prim_b;
  // LED turn-on compensation in percent (R, G, B, W).
  std::array<float, 4> offset;
  // Colour temperature table: step function, no interpolation.
  std::array<CTEntry, 15> ct_table;
  uint8_t ct_entries;
};

// clang-format off
// Tables copied from the stock firmware (DROM 0x3f414184 / 0x3f41421c).
static const ColorModel COLOR_MODEL_DEFAULT = {
  {1.224074f, -0.286317f, -0.211063f,
   -1.143753f, 2.528182f, 0.043872f,
   0.035504f, -0.080821f, 0.484313f},
  {{{-1.321507f, 0.954193f}, {1.300224f, -0.140904f}, {-0.338028f, 0.543394f},
    {19.070578f, -2.80468f}, {-0.794165f, 0.856988f}, {0.478051f, -0.01766f}}},
  {0.6875f, 0.311f}, {0.18433f, 0.7106f}, {0.1499f, 0.054f},
  {3.0f, 3.0f, 3.0f, 6.0f},
  {{
    {1700, 100.0f, 64.50f, 0.00f, 0.77f},
    {2000, 100.0f, 75.52f, 0.00f, 3.40f},
    {2200, 100.0f, 80.81f, 0.00f, 6.45f},
    {2400, 100.0f, 84.59f, 0.00f, 11.31f},
    {2700, 100.0f, 86.09f, 0.00f, 28.31f},
    {3000, 90.0f, 67.65f, 0.00f, 72.32f},
    {3200, 23.1f, 0.00f, 1.67f, 90.00f},
    {3400, 0.0f, 0.00f, 0.00f, 100.0f},
    {4000, 0.0f, 72.46f, 49.98f, 75.00f},
    {4500, 0.0f, 100.0f, 63.41f, 58.39f},
    {5000, 0.0f, 100.0f, 70.97f, 45.70f},
    {5500, 0.0f, 100.0f, 67.77f, 35.34f},
    {5800, 0.0f, 100.0f, 81.44f, 38.45f},
    {6000, 0.0f, 100.0f, 77.60f, 32.66f},
    {6500, 0.0f, 100.0f, 80.65f, 29.29f},
  }},
  15,
};

// Tables copied from the stock firmware (DROM 0x3f413d4c / 0x3f413de4).
static const ColorModel COLOR_MODEL_HONGLI = {
  {1.591914f, -0.306039f, -0.297601f,
   -0.847025f, 1.932919f, 0.059615f,
   0.035076f, -0.081751f, 0.430256f},
  {{{-1.328352f, 0.929766f}, {1.442394f, -0.182966f}, {-0.3141f, 0.522443f},
    {223.933334f, -35.269787f}, {-0.771825f, 0.840332f}, {0.483979f, -0.031824f}}},
  {0.6945f, 0.3043f}, {0.1607f, 0.7163f}, {0.1577f, 0.0445f},
  {3.921569f, 3.137255f, 3.137255f, 3.921569f},
  {{
    {1700, 100.0f, 39.82f, 0.000f, 0.17f},
    {1900, 100.0f, 49.48f, 0.000f, 2.81f},
    {2100, 100.0f, 51.82f, 0.000f, 4.87f},
    {2600, 100.0f, 53.57f, 0.000f, 10.31f},
    {3000, 100.0f, 52.05f, 0.000f, 22.91f},
    {3200, 100.0f, 45.45f, 0.000f, 45.77f},
    {3400, 0.0f, 0.00f, 0.000f, 100.0f},
    {4000, 0.0f, 35.532f, 41.526f, 90.00f},
    {4500, 1.0f, 80.00f, 72.584f, 78.576f},
    {5000, 0.0f, 100.0f, 88.42f, 64.14f},
    {5400, 0.0f, 100.0f, 89.85f, 49.41f},
    {5700, 0.0f, 100.0f, 91.28f, 44.10f},
    {5900, 0.0f, 100.0f, 92.11f, 41.25f},
    {6500, 0.0f, 100.0f, 94.79f, 36.06f},
    {0, 0, 0, 0, 0},
  }},
  14,
};
// clang-format on

inline const ColorModel *color_model_for(HardwareVariant v) {
  switch (v) {
    case HardwareVariant::DEFAULT: return &COLOR_MODEL_DEFAULT;
    case HardwareVariant::HONGLI: return &COLOR_MODEL_HONGLI;
    default: return nullptr;
  }
}

struct RGBWDuty {
  float r, g, b, w;  // on-duty, 0.0 - 1.0
};

namespace color_model_impl {

static const float PWM_MAX = 4095.0f;
static const float MIN_BRIGHTNESS = 10.0f;  // percent, stock firmware normal light

// FUN_401213ac: 1..100 % maps linearly onto MIN_BRIGHTNESS..100 %.
inline float brightness_factor(float percent) {
  if (percent <= 0.0f)
    return 0.0f;
  percent = std::min(std::max(percent, 1.0f), 100.0f);
  const float a = (MIN_BRIGHTNESS - 100.0f) / -99.0f;
  const float c = (100.0f - MIN_BRIGHTNESS * 100.0f) / -99.0f;
  return (c + a * percent) / 100.0f;
}

// FUN_40122540: LED turn-on compensation. Works on a 0..1 duty.
inline float apply_offset(float duty, float k_percent) {
  if (duty <= 0.0f || k_percent == 0.0f)
    return duty <= 0.0f ? 0.0f : duty;
  const float k = k_percent / 100.0f;
  return k + (1.0f - k) * duty;
}

// Quantise like the stock firmware does (12 bit, +0.49 rounding).
inline float quantise(float duty) {
  float v = std::floor(duty * PWM_MAX + 0.49f);
  return std::min(std::max(v, 0.0f), PWM_MAX) / PWM_MAX;
}

// FUN_40121e20. Note: the stock firmware uses 0.004045 as the threshold.
inline float srgb_linear(float c) { return c <= 0.004045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f); }

inline XY srgb_to_xy(float r, float g, float b) {
  const float R = srgb_linear(r), G = srgb_linear(g), B = srgb_linear(b);
  const float X = R * 0.4124f + G * 0.3576f + B * 0.1805f;
  const float Y = R * 0.2126f + G * 0.7152f + B * 0.0722f;
  const float Z = R * 0.0193f + G * 0.1192f + B * 0.9505f;
  const float s = X + Y + Z;
  if (s <= 0.0f)
    return {0.3127f, 0.3290f};
  return {X / s, Y / s};
}

inline bool above(const LineEq &l, const XY &p) { return p.y - (l.intercept + l.slope * p.x) > 0.0f; }

// FUN_401214f8: 9/10/11 = inside, 12 = beyond G-B, 13 = beyond R-G, 14 = beyond R-B.
inline int classify(const ColorModel &m, const XY &p) {
  const bool above0 = above(m.lines[0], p);
  if (!above(m.lines[1], p)) {
    if (!above(m.lines[2], p))
      return above(m.lines[5], p) ? 11 : 14;
  } else if (!above0) {
    const auto &l = m.lines[3];
    return (p.y - (l.intercept + l.slope * p.x)) * l.slope > 0.0f ? 12 : 9;
  }
  return above(m.lines[4], p) ? 13 : 10;
}

// FUN_40122e60 + FUN_40122d90 + FUN_40122904.
inline void xy_to_rgb_weights(const ColorModel &m, XY p, float out[3]) {
  switch (classify(m, p)) {
    case 12:
      p.x = std::min(std::max(p.x, m.prim_b.x), m.prim_g.x);
      p.y = m.lines[3].intercept + m.lines[3].slope * p.x;
      break;
    case 13:
      p.x = std::max(std::min(p.x, m.prim_r.x), m.prim_g.x);
      p.y = m.lines[4].intercept + m.lines[4].slope * p.x;
      break;
    case 14:
      p.x = std::min(std::max(p.x, m.prim_b.x), m.prim_r.x);
      p.y = m.lines[5].intercept + m.lines[5].slope * p.x;
      break;
    default:
      break;
  }
  const float X = p.x / p.y;
  const float Z = (1.0f - p.x - p.y) / p.y;
  float sum = 0.0f;
  for (int i = 0; i < 3; i++) {
    out[i] = m.rgb_matrix[3 * i] * X + m.rgb_matrix[3 * i + 1] + m.rgb_matrix[3 * i + 2] * Z;
    sum += out[i];
  }
  float mx = 0.0f;
  for (int i = 0; i < 3; i++) {
    out[i] /= sum;
    mx = std::max(mx, out[i]);
  }
  for (int i = 0; i < 3; i++)
    out[i] = mx > 0.0f ? std::max(out[i] / mx, 0.0f) : 0.0f;
}

inline bool near(float a, float b, float tol) { return std::fabs(a - b) < tol; }

}  // namespace color_model_impl

/**
 * RGB light (FUN_4012330c). r, g, b in 0..1 (ESPHome color values),
 * brightness in 0..1. The white channel is always off in RGB mode.
 */
inline RGBWDuty color_model_rgb(const ColorModel &m, float r, float g, float b, float brightness) {
  using namespace color_model_impl;
  const float bf = brightness_factor(brightness * 100.0f);
  const int r8 = (int) std::lround(r * 255.0f), g8 = (int) std::lround(g * 255.0f), b8 = (int) std::lround(b * 255.0f);
  const float full = quantise(bf);

  // FUN_4012222c: pure primaries drive a single LED, without turn-on offset.
  if (r8 == 255 && g8 == 0 && b8 == 0) return {full, 0, 0, 0};
  if (r8 == 0 && g8 == 255 && b8 == 0) return {0, full, 0, 0};
  if (r8 == 0 && g8 == 0 && b8 == 255) return {0, 0, full, 0};
  const XY p = srgb_to_xy(r, g, b);
  if (near(p.x, 0.64f, 0.01f) && near(p.y, 0.33f, 0.01f)) return {full, 0, 0, 0};
  if (near(p.x, 0.30f, 0.005f) && near(p.y, 0.60f, 0.005f)) return {0, full, 0, 0};
  if (near(p.x, 0.15f, 0.01f) && near(p.y, 0.06f, 0.01f)) return {0, 0, full, 0};

  float wgt[3];
  xy_to_rgb_weights(m, p, wgt);
  return {
    quantise(apply_offset(wgt[0] * bf, m.offset[0])),
    quantise(apply_offset(wgt[1] * bf, m.offset[1])),
    quantise(apply_offset(wgt[2] * bf, m.offset[2])),
    0.0f,
  };
}

/**
 * White light (FUN_40122f68, CT mode). mired as used by ESPHome,
 * brightness in 0..1.
 */
inline RGBWDuty color_model_ct(const ColorModel &m, float mired, float brightness) {
  using namespace color_model_impl;
  const float bf = brightness_factor(brightness * 100.0f);
  float kelvin = mired > 0.0f ? 1000000.0f / mired : 6500.0f;
  kelvin = std::min(std::max(kelvin, 1700.0f), 6500.0f);
  // FUN_40121a98: entry i is used for kelvin_i <= K < kelvin_{i+1}. Rounding
  // makes HA's 1700 K (588 mired = 1700.68 K) and 6500 K land in the intended rows.
  const uint16_t k = (uint16_t) std::lround(kelvin);
  const CTEntry *e = &m.ct_table[0];
  for (uint8_t i = 0; i < m.ct_entries; i++) {
    const uint16_t next = (i + 1 < m.ct_entries) ? m.ct_table[i + 1].kelvin_from : 10000;
    if (m.ct_table[i].kelvin_from <= k && k < next) {
      e = &m.ct_table[i];
      break;
    }
  }
  return {
    quantise(apply_offset(e->r / 100.0f * bf, m.offset[0])),
    quantise(apply_offset(e->g / 100.0f * bf, m.offset[1])),
    quantise(apply_offset(e->b / 100.0f * bf, m.offset[2])),
    quantise(apply_offset(e->w / 100.0f * bf, m.offset[3])),
  };
}

/**
 * Parse the stock firmware's "minvs" key/value store for app_data.sub_model.
 * Record layout: 55 AA | flags(2, FFFF = current) | crc(4) | value_len(2) |
 * id(2) | key_len(1) | FF FF FF | key | value.
 * Returns the sub_model value, or -1 when not found.
 */
inline int parse_stock_sub_model(const uint8_t *data, size_t len) {
  static const char KEY[] = "app_data.sub_model";
  const size_t klen = sizeof(KEY) - 1;
  int found = -1;
  bool found_current = false;
  for (size_t i = 16; i + klen + 1 <= len; i++) {
    if (memcmp(data + i, KEY, klen) != 0)
      continue;
    const uint8_t *h = data + i - 16;
    if (h[0] != 0x55 || h[1] != 0xAA || h[12] != klen)
      continue;
    const uint16_t vlen = h[8] | (h[9] << 8);
    if (vlen < 1 || vlen > 3 || i + klen + vlen > len)
      continue;
    int v = 0;
    bool ok = true;
    for (uint16_t j = 0; j < vlen; j++) {
      const uint8_t c = data[i + klen + j];
      if (c < '0' || c > '9') { ok = false; break; }
      v = v * 10 + (c - '0');
    }
    if (!ok)
      continue;
    const bool current = h[2] == 0xFF && h[3] == 0xFF;
    // Prefer the last record that is still marked current.
    if (current || !found_current) {
      found = v;
      found_current = current;
    }
  }
  return found;
}

}  // namespace bslamp2
}  // namespace xiaomi
}  // namespace esphome
