#pragma once

#include <algorithm>

#include "common.h"
#include "constants.h"
#include "utility.h"

#include "esphome/core/defines.h"
#include "esphome/components/fastled_helper/utils.h"

namespace esphome::matrix_lamp {

// ************* ДЛЯ РАЗРАБОТЧИКОВ *****
// The 16 bit version of our coordinates
static uint16_t x;
static uint16_t y;
static uint16_t z;

// This is the array that we keep our computed noise values in
#if (WIDTH > HEIGHT)
static uint8_t noise[WIDTH][WIDTH];
#else
static uint8_t noise[HEIGHT][HEIGHT];
#endif

// CRGBPalette16 currentPalette(PartyColors_p);
static uint8_t colorLoop = 1;
static uint8_t ihue = 0;

static void fillNoiseLED();
static void fillnoise8();

#ifdef DEF_MADNESS
static void madnessNoiseRoutine() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      uint8_t tmp = random8(9U);
      //                       scale | speed
      setModeSettings(30U + tmp * tmp, 20U + random8(41U));
    }
#endif  // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;

    loadingFlag = false;
  }

  fillnoise8();

  for (uint8_t j = 0U; j < HEIGHT; j++) {
    for (uint8_t i = 0U; i < WIDTH; i++) {
      CRGB thisColor = CHSV(noise[j][i], 255U, noise[i][j]);
      drawPixelXY(i, j, thisColor);
    }
  }
  ihue += 1;
}
#endif

#ifdef DEF_RAINBOW
static void rainbowNoiseRoutine() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      uint8_t tmp = random8(10U);
      //                       scale | speed
      setModeSettings(20U + tmp * tmp, 1U + random8(23U));
    }
#endif  // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    currentPalette = RainbowColors_p;

    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;

    colorLoop = 1U;

    loadingFlag = false;
  }

  fillNoiseLED();
}
#endif

#ifdef DEF_RAINBOW_STRIPE
static void rainbowStripeNoiseRoutine() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                         scale | speed
      setModeSettings(8U + random8(17U), 1U + random8(9U));
    }
#endif  // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    currentPalette = RainbowStripeColors_p;

    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;

    colorLoop = 1U;

    loadingFlag = false;
  }

  fillNoiseLED();
}
#endif

#ifdef DEF_ZEBRA
static void zebraNoiseRoutine() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                          scale | speed
      setModeSettings(12U + random8(16U), 1U + random8(9U));
    }
#endif  // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    // 'black out' all 16 palette entries...
    fill_solid(currentPalette, 16, CRGB::Black);

    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;

    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;

    colorLoop = 1U;

    loadingFlag = false;
  }

  fillNoiseLED();
}
#endif

#ifdef DEF_FOREST
static void forestNoiseRoutine() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                          scale | speed
      setModeSettings(70U + random8(31U), 2U + random8(24U));
    }
#endif  // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    currentPalette = ForestColors_p;

    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;

    colorLoop = 0U;

    loadingFlag = false;
  }

  fillNoiseLED();
}
#endif

#ifdef DEF_OCEAN
static void oceanNoiseRoutine() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                         scale | speed
      setModeSettings(6U + random8(25U), 4U + random8(8U));
    }
#endif  // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    currentPalette = OceanColors_p;

    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;

    colorLoop = 0U;

    loadingFlag = false;
  }

  fillNoiseLED();
}
#endif

#ifdef DEF_PLASMA
static void plasmaNoiseRoutine() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      uint8_t tmp = random8(10U);
      //                       scale | speed
      setModeSettings(20U + tmp * tmp, 1U + random8(27U));
    }
#endif  // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    currentPalette = PartyColors_p;

    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;

    colorLoop = 1U;

    loadingFlag = false;
  }

  fillNoiseLED();
}
#endif

#ifdef DEF_CLOUDS
static void cloudsNoiseRoutine() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      setModeSettings(15U + random8(36U), 1U + random8(10U));
    }
#endif  // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;

    currentPalette = CloudColors_p;
    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;
    colorLoop = 0U;
  }
  fillNoiseLED();
}
#endif

#ifdef DEF_LAVA
static void lavaNoiseRoutine() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      uint8_t tmp = random8(9U);
      //                       scale | speed
      setModeSettings(10U + tmp * tmp, 5U + random8(16U));
    }
#endif  // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    currentPalette = LavaColors_p;

    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;

    colorLoop = 0U;

    loadingFlag = false;
  }

  fillNoiseLED();
}
#endif

#ifdef DEF_TASTEHONEY
// ========== Taste of Honey ============
//         SRS code by © Stepko
//        Adaptation © SlingMaster
//               Смак Меду
// --------------------------------------

static void TasteHoney() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                         scale | speed
      setModeSettings(random8(1U, 255U), random8(150U, 255U));
    }
#endif

    clearNoiseArr();

    hue = (uint8_t) (modes[currentMode].Scale * 2.55f);

    const uint8_t palette_idx = modes[currentMode].Scale / 10U;
    switch (palette_idx) {
      case 0U:
        currentPalette = PartyColors_p;
        break;
      case 1U:
        currentPalette = LavaColors_p;
        break;
      case 2U:
      case 3U:
        currentPalette = ForestColors_p;
        break;
      case 4U:
        currentPalette = CloudColors_p;
        break;
      default:
        currentPalette = AlcoholFireColors_p;
        break;
    }

    ledsClear();  // esphome: FastLED.clear();

    loadingFlag = false;
  }

  fillNoiseLED();

  CRGB col = CHSV(hue, 255U, 255U);

  for (uint8_t x = 0U; x < WIDTH; x++) {
    const int16_t x_mul255 = (int16_t) ((x << 8U) - x);  // x * 255

    for (uint8_t y = 0U; y < HEIGHT; y++) {
      const uint8_t n0 = noise[x][y];
      const uint8_t n1 = (x + 1U < WIDTH) ? noise[x + 1U][y] : n0;
      const uint8_t n2 = (y + 1U < HEIGHT) ? noise[x][y + 1U] : n0;

      const int8_t xl = (int8_t) (n0 - n1);
      const int8_t yl = (int8_t) (n0 - n2);

      const int16_t y_mul255 = (int16_t) ((y << 8U) - y);  // y * 25

      // Вычисление смещения субпиксельной WU-координаты
      const int16_t xa = x_mul255 + (int16_t) ((xl * ((int16_t) (n0 + n1) << 1U)) >> 3U);
      const int16_t ya = y_mul255 + (int16_t) ((yl * ((int16_t) (n0 + n2) << 1U)) >> 3U);

      // Отрисовка сглаженного WU-пикселя по смещенному вектору освещения
      wu_pixel((uint32_t) xa, (uint32_t) ya, &col);
    }
  }
}
#endif

#ifdef DEF_POPURI
// ============== Popuri ===============
//             © SlingMaster
//                Попурі
// =====================================
static void Popuri() {
  constexpr uint8_t PADDING = (uint8_t) (HEIGHT * 0.25f);
  constexpr float freq = 3000.0f;

  // step      => frameCount (8-битный счетчик кадров анимации)
  // deltaHue2 => Индекс масштаба (index)
  // hue       => Базовый оттенок цвета

  // ---------------------
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //            scale | speed
      setModeSettings(128U, random8(4U, 254U));
    }
#endif

    hue = 0U;
    step = 0U;                                    // frameCount
    deltaValue = modes[currentMode].Scale / 25U;  // index

    if (deltaValue < 1U) {
      currentPalette = LavaColors_p;
      currentPalette[8] = CRGB::DarkRed;
    } else {
      currentPalette = custom_eff ? PartyColors_p : AlcoholFireColors_p;
    }

    ledsClear();  // esphome: FastLED.clear();

    loadingFlag = false;
  }

  // change color --------
  const uint8_t t1 = cos8_t((uint16_t) ((42U * step) * inv30));  // 1.0f / 30.0f ≈ 0.0333333f
  const uint8_t t2 = cos8_t((uint16_t) ((35U * step) * inv30));
  const uint8_t t3 = cos8_t((uint16_t) ((38U * step) * inv30));

  const uint8_t scale_val = modes[currentMode].Scale;
  const bool is_even_index = ((deltaValue & 0x01U) == 0U);

  const uint8_t t3_div2 = t3 >> 2U;

  if (scale_val < 50U) {
    fillNoiseLED();
  } else {
    fadeToBlackBy(leds, NUM_LEDS, WIDTH);
  }

  for (uint8_t x = 0U; x < WIDTH; x++) {
    const int16_t x_mul255 = (int16_t) ((x << 8U) - x);  // x * 255
    const uint16_t x_freq = (uint16_t) ((float) x * freq);
    const uint32_t yy = (uint32_t) (x << 8U);  // x * 256

    for (uint8_t y = 0U; y < HEIGHT; y++) {
      uint8_t r = 0U;
      uint8_t g = 0U;
      uint8_t b = 0U;

      if ((y <= (uint8_t) (PADDING - 1U)) || (y >= (uint8_t) (HEIGHT - PADDING))) {
        // --------------------------------------------------------------------
        // Плазменная рамка по краям матрицы
        // --------------------------------------------------------------------
        r = sin8_t((uint8_t) (((int16_t) x - 8) * cos8_t((uint8_t) ((y + 20U) << 2U)) >> 2U));
        g = cos8_t((uint8_t) (((y << 3U)) + t1 + cos8_t((uint8_t) (t3_div2 + (x << 3U)))));
        b = cos8_t((uint8_t) (((y << 3U)) + t2 + cos8_t((uint8_t) (t1 + x + (g >> 2U)))));

        g = exp_gamma[g];
        b = exp_gamma[b];

        if (is_even_index) {
          if (b < 20U)
            b = exp_gamma[r];
          r = (g < 128U) ? (uint8_t) (exp_gamma[b] / 3U) : 0U;
        } else {
          if (g < 20U)
            g = exp_gamma[r];
          r = (b < 128U) ? (uint8_t) (exp_gamma[g] >> 1U) : 0U;
        }

        // Черная разделительная черта по границе PADDING
        if ((y == (uint8_t) (PADDING - 1U)) || (y == (uint8_t) (HEIGHT - PADDING))) {
          r = 0U;
          g = 0U;
          b = 0U;
        }
        leds[XY(x, y)] = CRGB(r, g, b);

      } else {
        // --------------------------------------------------------------------
        // Центральная зона кадра
        // --------------------------------------------------------------------
        CRGB col;

        if (scale_val < 50U) {
          // Стекающий мёд
          const uint8_t n0 = noise[x][y];
          const uint8_t n1 = (x + 1U < WIDTH) ? noise[x + 1U][y] : n0;
          const uint8_t n2 = (y + 1U < HEIGHT) ? noise[x][y + 1U] : n0;

          const int8_t xl = (int8_t) (n0 - n1);
          const int8_t yl = (int8_t) (n0 - n2);

          const int16_t y_mul255 = (int16_t) ((y << 8U) - y);  // y * 255

          const int16_t xa = x_mul255 + (int16_t) ((xl * ((int16_t) (n0 + n1) << 1U)) >> 3U);
          const int16_t ya = y_mul255 + (int16_t) ((yl * ((int16_t) (n0 + n2) << 1U)) >> 3U);

          col = CHSV(hue, 255U, 255U);
          wu_pixel((uint32_t) xa, (uint32_t) ya, &col);
        } else {
          // Волновые змейки WU Уитни
          const uint32_t xx = beatsin16(WIDTH, 0U, (uint16_t) ((HEIGHT - (PADDING << 1U) - 1U) << 8U), 0U, x_freq);

          col = (hue < 80U) ? CHSV(0U, 255U, 255U) : CHSV(hue, 255U, 255U);
          wu_pixel(yy, (uint32_t) (xx + (PADDING << 8U)), &col);  // PADDING * 256
        }
      }
    }

    if (scale_val > 50U) {
      if ((step % WIDTH) == 0U) {
        hue++;
      }
    }
  }

  // -----------------
  step++;  // инкремент 8-битного frameCount кадра
}
#endif

// ************* СЛУЖЕБНЫЕ *************
static void fillNoiseLED() {
  uint8_t dataSmoothing = 0U;

  if (speed < 50U) {
    dataSmoothing = (uint8_t) (200U - (speed << 2U));  // speed * 4
  }

  // Инвариант веса сглаживания для FastLED scale8
  const uint8_t smoothing_inv = (uint8_t) (255U - dataSmoothing);

  // ФАЗА 1: Генерация сырого Перлин-шума в буфер noise
  for (uint8_t i = 0U; i < MAX_SIDE; i++) {
    const int32_t ioffset = (int32_t) (scale * i);

    for (uint8_t j = 0U; j < MAX_SIDE; j++) {
      const int32_t joffset = (int32_t) (scale * j);

      uint8_t data = fastled_helper::perlin8(x + ioffset, y + joffset, z);

      data = qsub8(data, 16U);
      data = qadd8(data, scale8(data, 39U));

      if (dataSmoothing != 0U) {
        const uint8_t olddata = noise[i][j];
        data = (uint8_t) (scale8(olddata, dataSmoothing) + scale8(data, smoothing_inv));
      }

      noise[i][j] = data;
    }
  }

  // Обновление фаз и смещений осей шума
  z += speed;
  x += (speed >> 3U);  // speed / 8
  y -= (speed >> 4U);  // speed / 16

  const uint8_t current_ihue = ihue;
  const bool has_color_loop = colorLoop;

  // ФАЗА 2: Перенос кадра на leds
  for (uint8_t j = 0U; j < HEIGHT; j++) {
    for (uint8_t i = 0U; i < WIDTH; i++) {
      uint8_t index = noise[j][i];
      uint8_t bri = noise[i][j];

      if (has_color_loop) {
        index += current_ihue;
      }

      if (bri > 127U) {
        bri = 255U;
      } else {
        bri = dim8_raw((uint8_t) (bri << 1U));  // bri * 2
      }

      drawPixelXY(i, j, color_from_palette(currentPalette, index, bri));
    }
  }

  ihue += 1;
}

static void fillnoise8() {
  // Линейный двумерный цикл генерации базового 3D-шума Перлина
  for (uint8_t i = 0U; i < MAX_SIDE; i++) {
    const int32_t ioffset = (int32_t) (scale * i);

    for (uint8_t j = 0U; j < MAX_SIDE; j++) {
      const int32_t joffset = (int32_t) (scale * j);

      noise[i][j] = fastled_helper::perlin8(x + ioffset, y + joffset, z);
    }
  }

  z += speed;
}

}  // namespace esphome::matrix_lamp
