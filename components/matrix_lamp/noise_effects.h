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
static void madnessNoiseRoutine()
{
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        uint8_t tmp = random8(9U);
        setModeSettings(30U + tmp * tmp, 20U + random8(41U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;

    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;
  }

  fillnoise8();
  for (uint8_t i = 0U; i < WIDTH; i++)
  {
    for (uint8_t j = 0U; j < HEIGHT; j++)
    {
      CRGB thisColor = CHSV(noise[j][i], 255, noise[i][j]);
      drawPixelXY(i, j, thisColor);
    }
  }
  ihue += 1;
}
#endif

#ifdef DEF_RAINBOW
static void rainbowNoiseRoutine()
{
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        uint8_t tmp = random8(10U);
        setModeSettings(20U + tmp * tmp, 1U + random8(23U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;

    currentPalette = RainbowColors_p;
    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;
    colorLoop = 1;
  }

  fillNoiseLED();
}
#endif

#ifdef DEF_RAINBOW_STRIPE
static void rainbowStripeNoiseRoutine()
{
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(8U + random8(17U), 1U + random8(9U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;

    currentPalette = RainbowStripeColors_p;
    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;
    colorLoop = 1;
  }
  fillNoiseLED();
}
#endif

#ifdef DEF_ZEBRA
static void zebraNoiseRoutine()
{
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(12U + random8(16U), 1U + random8(9U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;

    // 'black out' all 16 palette entries...
    fill_solid(currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[ 0] = CRGB::White;
    currentPalette[ 4] = CRGB::White;
    currentPalette[ 8] = CRGB::White;
    currentPalette[12] = CRGB::White;

    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;
    colorLoop = 1;
  }
  fillNoiseLED();
}
#endif

#ifdef DEF_FOREST
static void forestNoiseRoutine()
{
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(70U + random8(31U), 2U + random8(24U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;

    currentPalette = ForestColors_p;
    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;
    colorLoop = 0;
  }
  fillNoiseLED();
}
#endif

#ifdef DEF_OCEAN
static void oceanNoiseRoutine()
{
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(6U + random8(25U), 4U + random8(8U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;

    currentPalette = OceanColors_p;
    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;
    colorLoop = 0;
  }

  fillNoiseLED();
}
#endif

#ifdef DEF_PLASMA
static void plasmaNoiseRoutine()
{
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        uint8_t tmp = random8(10U);
        setModeSettings(20U + tmp * tmp, 1U + random8(27U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;

    currentPalette = PartyColors_p;
    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;
    colorLoop = 1;
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
    colorLoop = 0;
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
        setModeSettings(10U + tmp * tmp, 5U + random8(16U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;

    currentPalette = LavaColors_p;
    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;
    colorLoop = 0;
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
  uint8_t index;
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      // scale | speed
      setModeSettings(random8(1U, 255U), random8(150U, 255U));
    }
    #endif

    loadingFlag = false;

    hue = modes[currentMode].Scale * 2.55f;
    index = modes[currentMode].Scale / 10U;

    clearNoiseArr();
    switch (index) {
      case 0:
        currentPalette = PartyColors_p;
        break;
      case 1:
        currentPalette = LavaColors_p;
        break;
      case 2:
      case 3:
        currentPalette = ForestColors_p;
        break;
      case 4:
        currentPalette = CloudColors_p;
        break;
      default :
        currentPalette = AlcoholFireColors_p;
        break;
    }
    ledsClear(); // esphome: FastLED.clear();
  }

  fillNoiseLED();
  memset8(&noise2[1][0][0], 255, (WIDTH + 1) * (HEIGHT + 1));
  for (byte x = 0; x < WIDTH; x++) {
    for (byte y = 0; y < HEIGHT; y++) {
      uint8_t n0 = noise2[0][x][y];
      uint8_t n1 = noise2[0][x + 1][y];
      uint8_t n2 = noise2[0][x][y + 1];

      int8_t xl = n0 - n1;
      int8_t yl = n0 - n2;

      int16_t xa = (x * 255) + ((xl * ((n0 + n1) << 1)) >> 3);
      int16_t ya = (y * 255) + ((yl * ((n0 + n2) << 1)) >> 3);

      CRGB col = CHSV(hue, 255U, 255U);
      wu_pixel(xa, ya, &col);
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
  constexpr uint8_t PADDING = (uint8_t)(HEIGHT * 0.25f);
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
    step = 0U;                                   // frameCount
    deltaValue = modes[currentMode].Scale / 25U; // index

    if (deltaValue < 1U) {
      curPalette = &LavaColors_p;
      (*curPalette)[8U] = CRGB::DarkRed;
    } else {
      curPalette = custom_eff ? &PartyColors_p : &AlcoholFireColors_p;
    }
    
    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  // change color --------
  const uint8_t t1 = cos8((uint16_t)((42U * step) * 0.0333333f)); // 1.0f / 30.0f ≈ 0.0333333f
  const uint8_t t2 = cos8((uint16_t)((35U * step) * 0.0333333f));
  const uint8_t t3 = cos8((uint16_t)((38U * step) * 0.0333333f));
  
  const uint8_t scale_val = modes[currentMode].Scale;
  const bool is_even_index = ((deltaValue & 0x01U) == 0U);

  const uint8_t t3_div2 = t3 >> 2U;

  if (scale_val < 50U) {
    fillNoiseLED();
  } else {
    fadeToBlackBy(leds, NUM_LEDS, WIDTH);
  }

  for (uint8_t x = 0U; x < WIDTH; x++) {
    const int16_t x_mul255 = (int16_t)((x << 8U) - x);    // x * 255
    const uint16_t x_freq = (uint16_t)((float)x * freq);
    const uint32_t yy = (uint32_t)(x << 8U);              // x * 256

    for (uint8_t y = 0U; y < HEIGHT; y++) {
      uint8_t r = 0U;
      uint8_t g = 0U;
      uint8_t b = 0U;

      if ((y <= (uint8_t)(PADDING - 1U)) || (y >= (uint8_t)(HEIGHT - PADDING))) {
        // --------------------------------------------------------------------
        // Плазменная рамка по краям матрицы
        // --------------------------------------------------------------------
        r = sin8((uint8_t)(((int16_t)x - 8) * cos8((uint8_t)((y + 20U) << 2U)) >> 2U));
        g = cos8((uint8_t)(((y << 3U)) + t1 + cos8((uint8_t)(t3_div2 + (x << 3U)))));
        b = cos8((uint8_t)(((y << 3U)) + t2 + cos8((uint8_t)(t1 + x + (g >> 2U)))));

        g = exp_gamma[g];
        b = exp_gamma[b];

        if (is_even_index) {
          if (b < 20U) b = exp_gamma[r];
          r = (g < 128U) ? (uint8_t)(exp_gamma[b] / 3U) : 0U;
        } else {
          if (g < 20U) g = exp_gamma[r];
          r = (b < 128U) ? (uint8_t)(exp_gamma[g] >> 1U) : 0U; 
        }

        // Черная разделительная черта по границе PADDING
        if ((y == (uint8_t)(PADDING - 1U)) || (y == (uint8_t)(HEIGHT - PADDING))) {
          r = 0U; g = 0U; b = 0U;
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
          const uint8_t n1 = (x + 1U < WIDTH)  ? noise[x + 1U][y] : n0;
          const uint8_t n2 = (y + 1U < HEIGHT) ? noise[x][y + 1U] : n0;

          const int8_t xl = (int8_t)(n0 - n1);
          const int8_t yl = (int8_t)(n0 - n2);

          const int16_t y_mul255 = (int16_t)((y << 8U) - y);  // y * 255

          const int16_t xa = x_mul255 + (int16_t)((xl * ((int16_t)(n0 + n1) << 1U)) >> 3U);
          const int16_t ya = y_mul255 + (int16_t)((yl * ((int16_t)(n0 + n2) << 1U)) >> 3U);

          col = CHSV(hue, 255U, 255U);
          wu_pixel((uint32_t)xa, (uint32_t)ya, &col);
        } 
        else {
          // Волновые змейки WU Уитни
          const uint32_t xx = beatsin16(WIDTH, 0U, (uint16_t)((HEIGHT - (PADDING << 1U) - 1U) << 8U), 0U, x_freq);
          
          col = (hue < 80U) ? CHSV(0U, 255U, 255U) : CHSV(hue, 255U, 255U);
          wu_pixel(yy, (uint32_t)(xx + (PADDING << 8U)), &col); // PADDING * 256
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
  step++; // инкремент 8-битного frameCount кадра
}
#endif


// ************* СЛУЖЕБНЫЕ *************
static void fillNoiseLED()
{
  uint8_t dataSmoothing = 0;

  if (speed < 50) {
    dataSmoothing = 200 - (speed * 4);
  }

  for (uint8_t i = 0U; i < MAX_SIDE; i++) {
    int32_t ioffset = scale * i;

    for (uint8_t j = 0U; j < MAX_SIDE; j++) {
      int32_t joffset = scale * j;

      uint8_t data = fastled_helper::perlin8(x + ioffset, y + joffset, z);

      data = qsub8(data, 16);
      data = qadd8(data, scale8(data, 39));

      if (dataSmoothing) {
        uint8_t olddata = noise[i][j];
        uint8_t newdata = scale8(olddata, dataSmoothing) + scale8(data, 256 - dataSmoothing);
        data = newdata;
      }

      noise[i][j] = data;
    }
  }
  z += speed;

  // apply slow drift to X and Y, just for visual variation.
  x += speed / 8U;
  y -= speed / 16U;

  for (uint8_t i = 0U; i < WIDTH; i++) {
    for (uint8_t j = 0U; j < HEIGHT; j++) {
      uint8_t index = noise[j][i];
      uint8_t bri   = noise[i][j];
      // if this palette is a 'loop', add a slowly-changing base value
      if (colorLoop) {
        index += ihue;
      }
      // brighten up, as the color palette itself often contains the
      // light/dark dynamic range desired
      if (bri > 127) {
        bri = 255;
      } else {
        bri = dim8_raw( bri * 2);
      }
      CRGB color = ColorFromPalette(currentPalette, index, bri);
      drawPixelXY(i, j, color);                             //leds[XY(i, j)] = color;
    }
  }
  ihue += 1;
}

static void fillnoise8()
{
  for (uint8_t i = 0U; i < MAX_SIDE; i++) {
    int32_t ioffset = scale * i;
    for (uint8_t j = 0U; j < MAX_SIDE; j++) {
      int32_t joffset = scale * j;
      noise[i][j] = fastled_helper::perlin8(x + ioffset, y + joffset, z);
    }
  }
  z += speed;
}

}  // namespace esphome::matrix_lamp
