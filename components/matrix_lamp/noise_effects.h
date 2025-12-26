#pragma once

#include "common.h"
#include "constants.h"
#include "utility.h"

#include "esphome/components/fastled_helper/utils.h"

namespace esphome {
namespace matrix_lamp {

// ************* НАСТРОЙКИ *************
/*
// "масштаб" эффектов. Чем меньше, тем крупнее!
#define MADNESS_SCALE 100
#define CLOUD_SCALE 30
#define LAVA_SCALE 50
#define PLASMA_SCALE 30
#define RAINBOW_SCALE 30
#define RAINBOW_S_SCALE 20
#define ZEBRA_SCALE 30
#define FOREST_SCALE 120
#define OCEAN_SCALE 90
*/


// ************* ДЛЯ РАЗРАБОТЧИКОВ *****
// The 16 bit version of our coordinates
static uint16_t x;
static uint16_t y;
static uint16_t z;

// This is the array that we keep our computed noise values in
#define MAX_DIMENSION (max(WIDTH, HEIGHT))
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
        setModeSettings(30U+tmp*tmp, 20U+random8(41U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;
  }
  fillnoise8();
  for (uint8_t i = 0; i < WIDTH; i++)
  {
    for (uint8_t j = 0; j < HEIGHT; j++)
    {
      CRGB thisColor = CHSV(noise[j][i], 255, noise[i][j]);
      drawPixelXY(i, j, thisColor);                         //leds[XY(i, j)] = CHSV(noise[j][i], 255, noise[i][j]);
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
        setModeSettings(20U+tmp*tmp, 1U+random8(23U));
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
        setModeSettings(8U+random8(17U), 1U+random8(9U));
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
        setModeSettings(12U+random8(16U), 1U+random8(9U));
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
    loadingFlag = false;
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(70U+random8(31U), 2U+random8(24U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

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
        setModeSettings(6U+random8(25U), 4U+random8(8U));
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
        setModeSettings(20U+tmp*tmp, 1U+random8(27U));
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
        setModeSettings(15U+random8(36U), 1U+random8(10U));
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
        setModeSettings(10U+tmp*tmp, 5U+random8(16U));
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
  byte index;
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      // scale | speed
      setModeSettings(random8(1U, 255U), random8(150U, 255U));
    }
    #endif

    loadingFlag = false;
    hue = modes[currentMode].Scale * 2.55;
    index = modes[currentMode].Scale / 10;
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
  const byte PADDING = HEIGHT * 0.25;
  const byte step1 = WIDTH;
  const double freq = 3000;
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
    fadeToBlackBy(leds, NUM_LEDS, step1);
  }
  // body if big height matrix ---------
  for (uint16_t y = 0; y < HEIGHT; y++) {
    for (uint16_t x = 0; x < WIDTH; x++) {

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
          uint32_t xx = beatsin16(step1, 0, (HEIGHT - PADDING * 2 - 1) * 256, 0, x * freq);
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
static void fillNoiseLED()
{
  uint8_t dataSmoothing = 0;
  if (speed < 50)
  {
    dataSmoothing = 200 - (speed * 4);
  }
  for (uint8_t i = 0; i < MAX_DIMENSION; i++)
  {
    int32_t ioffset = scale * i;
    for (uint8_t j = 0; j < MAX_DIMENSION; j++)
    {
      int32_t joffset = scale * j;

      uint8_t data = fastled_helper::perlin8(x + ioffset, y + joffset, z);

      data = qsub8(data, 16);
      data = qadd8(data, scale8(data, 39));

      if (dataSmoothing)
      {
        uint8_t olddata = noise[i][j];
        uint8_t newdata = scale8( olddata, dataSmoothing) + scale8( data, 256 - dataSmoothing);
        data = newdata;
      }

      noise[i][j] = data;
    }
  }
  z += speed;

  // apply slow drift to X and Y, just for visual variation.
  x += speed / 8;
  y -= speed / 16;

  for (uint8_t i = 0; i < WIDTH; i++)
  {
    for (uint8_t j = 0; j < HEIGHT; j++)
    {
      uint8_t index = noise[j][i];
      uint8_t bri   = noise[i][j];
      // if this palette is a 'loop', add a slowly-changing base value
      if ( colorLoop)
      {
        index += ihue;
      }
      // brighten up, as the color palette itself often contains the
      // light/dark dynamic range desired
      if ( bri > 127 )
      {
        bri = 255;
      }
      else
      {
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
  for (uint8_t i = 0; i < MAX_DIMENSION; i++)
  {
    int32_t ioffset = scale * i;
    for (uint8_t j = 0; j < MAX_DIMENSION; j++)
    {
      int32_t joffset = scale * j;
      noise[i][j] = fastled_helper::perlin8(x + ioffset, y + joffset, z);
    }
  }
  z += speed;
}

}  // namespace matrix_lamp
}  // namespace esphome
