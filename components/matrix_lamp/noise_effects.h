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

//CRGBPalette16 currentPalette(PartyColors_p);
static uint8_t colorLoop = 1;
static uint8_t ihue = 0;

static void fillNoiseLED();
static void fillnoise8();

#ifdef DEF_MADNESS
static void madnessNoiseRoutine() {
  if (loadingFlag)   {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        uint8_t tmp = random8(9U);
        //                       scale | speed
        setModeSettings(30U + tmp * tmp, 20U + random8(41U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

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
      if (selectedSettings){
        uint8_t tmp = random8(10U);
        //                       scale | speed
        setModeSettings(20U + tmp * tmp, 1U + random8(23U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

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
      if (selectedSettings){
        //                         scale | speed
        setModeSettings(8U + random8(17U), 1U + random8(9U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

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
      if (selectedSettings){
        //                          scale | speed
        setModeSettings(12U + random8(16U), 1U + random8(9U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    // 'black out' all 16 palette entries...
    fill_solid(currentPalette, 16, CRGB::Black);

    // and set every fourth one to white.
    currentPalette[ 0] = CRGB::White;
    currentPalette[ 4] = CRGB::White;
    currentPalette[ 8] = CRGB::White;
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
      if (selectedSettings){
        //                          scale | speed
        setModeSettings(70U + random8(31U), 2U + random8(24U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

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
      if (selectedSettings){
        //                         scale | speed
        setModeSettings(6U + random8(25U), 4U + random8(8U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

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
      if (selectedSettings){
        uint8_t tmp = random8(10U);
        //                       scale | speed
        setModeSettings(20U + tmp * tmp, 1U + random8(27U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

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
static void cloudsNoiseRoutine()
{
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(15U + random8(36U), 1U + random8(10U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

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
static void lavaNoiseRoutine()
{
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        uint8_t tmp = random8(9U);
        //                       scale | speed
        setModeSettings(10U + tmp * tmp, 5U + random8(16U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

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

    hue = (uint8_t)(modes[currentMode].Scale * 2.55f);

    const uint8_t palette_idx = modes[currentMode].Scale / 10U;
    switch (palette_idx) {
      case 0U: currentPalette = PartyColors_p;       break;
      case 1U: currentPalette = LavaColors_p;        break;
      case 2U:
      case 3U: currentPalette = ForestColors_p;      break;
      case 4U: currentPalette = CloudColors_p;       break;
      default: currentPalette = AlcoholFireColors_p; break;
    }

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  fillNoiseLED();

  CRGB col = CHSV(hue, 255U, 255U);

  for (uint8_t x = 0U; x < WIDTH; x++) {
    const int16_t x_mul255 = (int16_t)((x << 8U) - x);  // x * 255

    for (uint8_t y = 0U; y < HEIGHT; y++) {
      const uint8_t n0 = noise[x][y];
      const uint8_t n1 = (x + 1U < WIDTH)  ? noise[x + 1U][y] : n0;
      const uint8_t n2 = (y + 1U < HEIGHT) ? noise[x][y + 1U] : n0;

      const int8_t xl = (int8_t)(n0 - n1);
      const int8_t yl = (int8_t)(n0 - n2);

      const int16_t y_mul255 = (int16_t)((y << 8U) - y);  // y * 25

      // Вычисление смещения субпиксельной WU-координаты
      const int16_t xa = x_mul255 + (int16_t)((xl * ((int16_t)(n0 + n1) << 1U)) >> 3U);
      const int16_t ya = y_mul255 + (int16_t)((yl * ((int16_t)(n0 + n2) << 1U)) >> 3U);

      // Отрисовка сглаженного WU-пикселя по смещенному вектору освещения
      wu_pixel((uint32_t)xa, (uint32_t)ya, &col);
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
  constexpr uint8_t PADDING = HEIGHT * 0.25f;
  constexpr double freq = 3000;

  static int64_t frameCount;
  static byte index;

  // ---------------------
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      setModeSettings(128, random8(4, 254U));
    }
    #endif

    loadingFlag = false;

    hue = 0;
    frameCount = 0;
    currentPalette = LavaColors_p;
    index = modes[currentMode].Scale / 25;

    // ---------------------
    clearNoiseArr();
    if (index < 1) {
      currentPalette = LavaColors_p;
      currentPalette[8] = CRGB::DarkRed;
    } else {
      if (custom_eff) {
        currentPalette = PartyColors_p;
      } else {
        currentPalette = AlcoholFireColors_p;
      }
    }
    FastLED.clear();
  }

  // change color --------
  frameCount++;
  uint8_t t1 = cos8((42 * frameCount) / 30);
  uint8_t t2 = cos8((35 * frameCount) / 30);
  uint8_t t3 = cos8((38 * frameCount) / 30);

  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  // ---------------------

  uint16_t ms = millis();

  // float mn = 255.0 / 13.8;
  float mn = 255.0 / WIDTH; // 27.6;

  if (modes[currentMode].Scale < 50) {
    fillNoiseLED();
    memset8(&noise2[1][0][0], 255, (WIDTH + 1) * (HEIGHT + 1));
  } else {
    fadeToBlackBy(leds, NUM_LEDS, WIDTH);
  }
  // body if big height matrix ---------
  for (uint16_t y = 0U; y < HEIGHT; y++) {
    for (uint16_t x = 0U; x < WIDTH; x++) {

      if ( (y <= PADDING - 1) | (y >=  HEIGHT - PADDING) ) {
        r = sin8((x - 8) * cos8((y + 20) * 4) / 4);
        g = cos8((y << 3) + t1 + cos8((t3 >> 2) + (x << 3)));
        b = cos8((y << 3) + t2 + cos8(t1 + x + (g >> 2)));

        g = exp_gamma[g];
        b = exp_gamma[b];

        // if (modes[currentMode].Scale < 50) {
        if (index % 2U == 0) {
          // green blue magenta --
          if (b < 20) b = exp_gamma[r];
          r = (g < 128) ? exp_gamma[b] / 3 : 0;
        } else {
          // green blue yellow ---
          if (g < 20) g = exp_gamma[r];
          r = (b < 128) ? exp_gamma[g] / 2 : 0;
        }
        if ( (y == PADDING - 1) | (y ==  HEIGHT - PADDING) ) {
          r = 0;
          g = 0;
          b = 0;
        }
        leds[XY(x, y)] = CRGB(r, g, b);
      } else {
        // ---------------------
        CRGB col;
        if (modes[currentMode].Scale < 50) {
          uint8_t n0 = noise2[0][x][y];
          uint8_t n1 = noise2[0][x + 1][y];
          uint8_t n2 = noise2[0][x][y + 1];
          int8_t xl = n0 - n1;
          int8_t yl = n0 - n2;
          int16_t xa = (x * 255) + ((xl * ((n0 + n1) << 1)) >> 3);
          int16_t ya = (y * 255) + ((yl * ((n0 + n2) << 1)) >> 3);

          col = CHSV(hue, 255U, 255U);
          wu_pixel(xa, ya, &col);
          // ---------------------
        } else {
          uint32_t xx = beatsin16(WIDTH, 0, (HEIGHT - PADDING * 2 - 1) * 256, 0, x * freq);
          uint32_t yy = x * 256;

          if (hue < 80) {
            col = CHSV(0, 255U, 255U);
          } else {
            col = CHSV(hue, 255U, 255U);
          }
          wu_pixel (yy, xx + PADDING * 256, &col);
        }
      }
    }
    if (modes[currentMode].Scale > 50) {
      if (step % WIDTH == 0U) hue++;
    }
  }

  // -----------------
  step++;
}
#endif


// ************* СЛУЖЕБНЫЕ *************
static void fillNoiseLED() {
  uint8_t dataSmoothing = 0U;

  if (speed < 50U) {
    dataSmoothing = (uint8_t)(200U - (speed << 2U)); // speed * 4
  }

  // Инвариант веса сглаживания для FastLED scale8
  const uint8_t smoothing_inv = (uint8_t)(255U - dataSmoothing);

  // ФАЗА 1: Генерация сырого Перлин-шума в буфер noise
  for (uint8_t i = 0U; i < MAX_SIDE; i++) {
    const int32_t ioffset = (int32_t)(scale * i);

    for (uint8_t j = 0U; j < MAX_SIDE; j++) {
      const int32_t joffset = (int32_t)(scale * j);

      uint8_t data = fastled_helper::perlin8(x + ioffset, y + joffset, z);

      data = qsub8(data, 16U);
      data = qadd8(data, scale8(data, 39U));

      if (dataSmoothing != 0U) {
        const uint8_t olddata = noise[i][j];
        data = (uint8_t)(scale8(olddata, dataSmoothing) + scale8(data, smoothing_inv));
      }

      noise[i][j] = data;
    }
  }

  // Обновление фаз и смещений осей шума
  z += speed;
  x += (speed >> 3U); // speed / 8
  y -= (speed >> 4U); // speed / 16

  const uint8_t current_ihue = ihue;
  const bool has_color_loop = colorLoop;

  // ФАЗА 2: Перенос кадра на leds
  for (uint8_t j = 0U; j < HEIGHT; j++) {
    for (uint8_t i = 0U; i < WIDTH; i++) {
      uint8_t index = noise[j][i];
      uint8_t bri   = noise[i][j];

      if (has_color_loop) {
        index += current_ihue;
      }

      if (bri > 127U) {
        bri = 255U;
      } else {
        bri = dim8_raw((uint8_t)(bri << 1U)); // bri * 2
      }

      drawPixelXY(i, j, ColorFromPalette(currentPalette, index, bri));
    }
  }

  ihue += 1;
}

static void fillnoise8() {
  // Линейный двумерный цикл генерации базового 3D-шума Перлина
  for (uint8_t i = 0U; i < MAX_SIDE; i++) {
    const int32_t ioffset = (int32_t)(scale * i);

    for (uint8_t j = 0U; j < MAX_SIDE; j++) {
      const int32_t joffset = (int32_t)(scale * j);

      noise[i][j] = fastled_helper::perlin8(x + ioffset, y + joffset, z);
    }
  }

  z += speed;
}

}  // namespace esphome::matrix_lamp
