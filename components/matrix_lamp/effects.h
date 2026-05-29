#pragma once

#include "common.h"
#include "constants.h"
#include "effect_data.h"
#include "effect_palette.h"
#include "utility.h"

#include "esphome/core/defines.h"
#include "esphome/components/fastled_helper/utils.h"

#define SQRT_VARIANT sqrtf                                // выбор основной функции для вычисления квадратного корня sqrtf или sqrt3 для ускорения

namespace esphome::matrix_lamp {

// ============= ЭФФЕКТЫ ===============
// несколько общих переменных и буферов, которые могут использоваться в любом эффекте
#define NUM_LAYERSMAX (2U)

static uint8_t hue, hue2;                                 // постепенный сдвиг оттенка или какой-нибудь другой цикличный счётчик
static uint8_t deltaHue, deltaHue2;                       // ещё пара таких же, когда нужно много
static uint8_t step;                                      // какой-нибудь счётчик кадров или последовательностей операций
static uint8_t pcnt;                                      // какой-то счётчик какого-то прогресса
static uint8_t deltaValue;                                // просто повторно используемая переменная
static float speedfactor;                                 // регулятор скорости в эффектах реального времени
static float emitterX, emitterY;                          // какие-то динамичные координаты
static CRGB ledsbuff[NUM_LEDS];                           // копия массива leds[] целиком
static uint8_t noise3d[NUM_LAYERSMAX][WIDTH][HEIGHT];     // двухслойная маска или хранилище свойств в размер всей матрицы
static uint8_t line[WIDTH];                               // свойство пикселей в размер строки матрицы
static uint8_t shiftHue[HEIGHT];                          // свойство пикселей в размер столбца матрицы
static uint8_t shiftValue[HEIGHT];                        // свойство пикселей в размер столбца матрицы ещё одно
static uint16_t ff_x, ff_y, ff_z;                         // большие счётчики

static int8_t noise2[2][WIDTH + 1][HEIGHT + 1];

// массивы состояния объектов, которые могут использоваться в любом эффекте
#define trackingOBJECT_MAX_COUNT (100U)                                         // максимальное количество отслеживаемых объектов (очень влияет на расход памяти)
static float    trackingObjectPosX[trackingOBJECT_MAX_COUNT];
static float    trackingObjectPosY[trackingOBJECT_MAX_COUNT];
static float    trackingObjectSpeedX[trackingOBJECT_MAX_COUNT];
static float    trackingObjectSpeedY[trackingOBJECT_MAX_COUNT];
static float    trackingObjectShift[trackingOBJECT_MAX_COUNT];
static uint8_t  trackingObjectHue[trackingOBJECT_MAX_COUNT];
static uint8_t  trackingObjectState[trackingOBJECT_MAX_COUNT];
static bool     trackingObjectIsShift[trackingOBJECT_MAX_COUNT];

#define enlargedOBJECT_MAX_COUNT (WIDTH * 2U)                                   // максимальное количество сложных отслеживаемых объектов (меньше, чем trackingOBJECT_MAX_COUNT)
static uint16_t  enlargedObjectNUM;                                             // используемое в эффекте количество объектов
static long      enlargedObjectTime[enlargedOBJECT_MAX_COUNT];
static float     liquidLampHot[enlargedOBJECT_MAX_COUNT];
static float     liquidLampSpf[enlargedOBJECT_MAX_COUNT];
static unsigned  liquidLampMX[enlargedOBJECT_MAX_COUNT];
static unsigned  liquidLampSC[enlargedOBJECT_MAX_COUNT];
static unsigned  liquidLampTR[enlargedOBJECT_MAX_COUNT];

static uint8_t custom_eff = 0U;

// --------------------------------------------------------------------------------------

#if defined(DEF_RAINBOW_RINGS) || defined(DEF_STARS_NIGHT) || defined(DEF_FIRESPARKS)
static uint32_t lastUpdateTime;
#endif
#if defined(DEF_RAINBOW_RINGS) || defined(DEF_BUTTERFLY) || defined(DEF_COLORED_PYTHON)
static uint32_t colorChangeTime;
#endif

// --------------------------------------------------------------------------------------

#if defined(DEF_CHRISTMAS_TREE) || defined(DEF_LIGHTFILTER)
static void clearNoiseArr() {
  for (uint8_t x = 0U; x < WIDTH; x++) {
    for (uint8_t y = 0U; y < HEIGHT; y++) {
      noise3d[0][x][y] = 0;
      noise3d[1][x][y] = 0;
    }
  }
}
#endif

// --------------------------------------------------------------------------------------

#ifdef DEF_SPARKLES
// ------------- конфетти --------------
#define FADE_OUT_SPEED (70U)                                             // скорость затухания
static void sparklesRoutine()
{
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(4U+random8(97U), 99U+random8(125U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    for (uint16_t i = 0U; i < NUM_LEDS; i++)
      if (random8(3U))
        leds[i].nscale8(random8());
      else
        leds[i] = 0U;

    loadingFlag = false;
  }

  const uint8_t max_sparks = modes[currentMode].Scale;
  for (uint8_t i = 0U; i < max_sparks; i++)
  {
    uint8_t x = random8(WIDTH);
    uint8_t y = random8(HEIGHT);

    uint16_t led_index = XY(x, y);
    if ((uint32_t)leds[led_index] == 0U) {
      leds[led_index] = CHSV(random8(), 255U, 255U);
    }
  }

  dimAll(256U - FADE_OUT_SPEED);
}
#endif


#ifdef DEF_WATERFALL
// =============- новый огонь / водопад -===============
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 55, suggested range 20-100
#define COOLINGNEW (32U)
// 8  практически сплошной поток красивой подсвеченной воды ровным потоком сверху донизу. будто бы на столе стоит маленький "родничок"
// 20 ровный водопад с верщиной на свету, где потоки летящей воды наверху разбиваются ветром в белую пену
// 32 уже не ровный водопад, у которого струи воды долетают до земли неравномерно
// чем больше параметр, тем больше тени снизу
// 55 такое, как на видео

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKINGNEW (80U) // 30 // 120 // 90 // 60
// 80 почти все белые струи сверху будут долетать до низа - хорошо при выбранном ползунке Масштаб = 100 (белая вода без подкрашивания)
// 50 чуть больше половины будет долетать. для цветных вариантов жидкости так более эффектно

static void fire2012WithPalette() {
  if (loadingFlag) {
    loadingFlag = false;
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(random8(7U) ? 46U + random8(26U) : 100U, 195U + random8(40U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
  }

  constexpr uint8_t cooling_limit = ((COOLINGNEW * 10U) / HEIGHT) + 2U;

  CRGBPalette16 customPalette;
  const bool is_waterfall = (modes[currentMode].Scale == 100);
  if (!is_waterfall) {
    // 2.57f * Scale переводим в целые числа: (Scale * 257) / 100
    // 2.57f вместо 2.55f, потому что 100 для белого цвета
    uint8_t hue_val = ((uint16_t)modes[currentMode].Scale * 257U) / 100U;
    customPalette = CRGBPalette16(
      CRGB::Black,
      CHSV(hue_val, 255U, 255U),
      CHSV(hue_val, 128U, 255U),
      CRGB::White
    );
  }

  for (uint8_t x = 0U; x < WIDTH; x++) {
    // Step 1.  Cool down every cell a little
    for (uint8_t i = 0U; i < HEIGHT; i++) {
      noise3d[0][x][i] = qsub8(noise3d[0][x][i], random8(0, cooling_limit));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (uint8_t k = MAX_Y; k >= 2; k--) {
      noise3d[0][x][k] = ((uint16_t)noise3d[0][x][k - 1] + noise3d[0][x][k - 2] + noise3d[0][x][k - 2]) / 3U;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if (random8() < SPARKINGNEW) {
      uint8_t y = random8(2U);
      noise3d[0][x][y] = qadd8(noise3d[0][x][y], random8(160U, 255U));
    }

    // Step 4.  Map from heat cells to LED colors
    for (uint8_t j = 0U; j < HEIGHT; j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      uint8_t colorindex = scale8(noise3d[0][x][j], 240);
      uint16_t led_idx = XY(x, MAX_Y - j);
      if (led_idx < NUM_LEDS) {
        if (is_waterfall)
          leds[led_idx] = ColorFromPalette(WaterfallColors_p, colorindex);
        else
          leds[led_idx] = ColorFromPalette(customPalette, colorindex);
      }
    }
  }
}
#endif


#ifdef DEF_FIRE
// ------------- Огонь -----------------
#define SPARKLES              (1U)                       // вылетающие угольки вкл выкл
#define UNIVERSE_FIRE                                    // универсальный огонь 2-в-1 Цветной + Белый

// uint8_t pcnt = 0U;                                    // внутренний делитель кадров для поднимающегося пламени - переменная вынесена в общий пул, чтобы использовать повторно
// uint8_t deltaHue = 16U;                               // текущее смещение пламени (hueMask) - переменная вынесена в общий пул, чтобы использовать повторно
// uint8_t shiftHue[HEIGHT];                             // массив дороожки горизонтального смещения пламени (hueMask) - вынесен в общий пул массивов переменных
// uint8_t deltaValue = 16U;                             // текущее смещение пламени (hueValue) - переменная вынесена в общий пул, чтобы использовать повторно
// uint8_t shiftValue[HEIGHT];                           // массив дороожки горизонтального смещения пламени (hueValue) - вынесен в общий пул массивов переменных

// these values are substracetd from the generated values to give a shape to the animation
// static const uint8_t valueMask[8][16] PROGMEM =

// these are the hues for the fire,
// should be between 0 (red) to about 25 (yellow)
// static const uint8_t hueMask[8][16] PROGMEM =

static unsigned char matrixValue[HEIGHT][WIDTH];          // это массив для эффекта Огонь

static void generateLine();
static void shiftUp();
static void drawFrame(uint8_t pcnt, bool isColored);

static void fireRoutine(bool isColored)
{
  if (loadingFlag) {
    memset(matrixValue, 0U, sizeof(matrixValue));          // это массив для эффекта Огонь. странно, что его нужно залить нулями

    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(random8(30U) ? 1U + random8(100U) : 100U, 200U + random8(35U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;

    generateLine();
    pcnt = 0;
  }

  if (pcnt >= 30) {                                         // внутренний делитель кадров для поднимающегося пламени
    shiftUp();                                              // смещение кадра вверх
    generateLine();                                         // перерисовать новую нижнюю линию случайным образом
    pcnt = 0;
  }
  // drawFrame(pcnt, (strcmp(isColored, "C") == 0));        // прорисовка экрана
  drawFrame(pcnt, isColored);                               // для прошивки где стоит логический параметр
  pcnt += 25;                                               // делитель кадров: задает скорость подъема пламени 25/100 = 1/4
}

//---------------------------------------
// Randomly generate the next line (matrix row)
static void generateLine() {
  for (uint8_t x = 0U; x < WIDTH; x++) {
    line[x] = random8(127, 255);                            // заполнение случайным образом нижней линии (127, 255) - менее контрастное, (64, 255) - оригинал
  }
}

//---------------------------------------
static void shiftUp() {                                     // подъем кадра
  for (uint8_t y = MAX_Y; y > 0U; y--) {
    for (uint8_t x = 0U; x < WIDTH; x++) {
      uint8_t newX = x % 16U;                               // сократил формулу без доп. проверок
      if (y > 7U) continue;
      matrixValue[y][newX] = matrixValue[y - 1U][newX];     // смещение пламени (только для зоны очага)
    }
  }

  for (uint8_t x = 0U; x < WIDTH; x++) {                    // прорисовка новой нижней линии
    uint8_t newX = x % 16U;                                 // сократил формулу без доп. проверок
    matrixValue[0U][newX] = line[newX];
  }
}

//---------------------------------------
// draw a frame, interpolating between 2 "key frames"
// @param pcnt percentage of interpolation
static void drawFrame(uint8_t pcnt, bool isColored) {              // прорисовка нового кадра
  int32_t nextv;
#ifdef UNIVERSE_FIRE                                               // если определен универсальный огонь
  uint8_t baseHue = ((uint16_t)(modes[currentMode].Scale - 1U) * 26U) / 10U;
#else
  uint8_t baseHue = isColored ? 255U : 0U;
#endif
  uint8_t baseSat = (modes[currentMode].Scale < 100U) ? 255U : 0U; // вычисление базового оттенка

  // first row interpolates with the "next" line
  // random(0U, 2U)= скорость смещения языков чем больше 2U - тем медленнее
  // 15U, 17U - амплитуда качания -1...+1 относительно 16U
  // высчитываем плавную дорожку смещения всполохов для нижней строки
  // так как в последствии координаты точки будут исчисляться из остатка, то за базу можем принять кратную ширину матрицы hueMask
  // ширина матрицы hueMask = 16, поэтому нам нужно получить диапазон чисел от 15 до 17
  // далее к предыдущему значению прибавляем случайную 1 и отнимаем случайную 1 - это позволит плавным образом менять значение смещения
  if (random8(2U) != 0U) {
    int8_t shift = (int8_t)random8(2U) - (int8_t)random8(2U);
    shiftHue[0] = std::clamp((int)(shiftHue[0] + shift), 15, 17);
  }
  deltaHue = shiftHue[0]; // заносим это значение в стэк

  // random(0U, 3U)= скорость смещения очага чем больше 3U - тем медленнее
  // 15U, 17U - амплитуда качания -1...+1 относительно 16U
  if (random8(3U) != 0U) {
    int8_t shift = (int8_t)random8(2U) - (int8_t)random8(2U);
    shiftValue[0] = std::clamp((int)(shiftValue[0] + shift), 15, 17);
  }
  deltaValue = shiftValue[0];

  uint8_t inv_pcnt = 100U - pcnt;                                                           // Предрассчитываем инверсию шага фазы для целочисленного смешивания кадров
  for (uint8_t x = 0U; x < WIDTH; x++) {                                                    // прорисовка нижней строки (сначала делаем ее, так как потом будем пользоваться ее значением смещения)
    uint8_t newX = x % 16;                                                                  // сократил формулу без доп. проверок
    nextv = (int32_t)(((inv_pcnt * matrixValue[0][newX]) +                                  // расчет значения яркости относительно valueMask и нижерасположенной строки.
                      ((uint16_t)pcnt * line[newX])) / 100U)
            - pgm_read_byte(&valueMask[0][(x + deltaValue) % 16U]);
    CRGB color = CHSV(                                                                      // вычисление цвета и яркости пикселя
                   baseHue + pgm_read_byte(&hueMask[0][(x + deltaHue) % 16U]),              // H - смещение всполохов
                   baseSat,                                                                 // S - когда колесо масштаба = 100 - белый огонь (экономим на 1 эффекте)
                   (uint8_t)std::max((int32_t)0, nextv)                                     // V
                 );
    leds[XY(x, 0)] = color;                                                                 // прорисовка цвета очага
  }

  // Each row interpolates with the one before it
  for (uint8_t y = MAX_Y; y > 0U; y--) {                                              // прорисовка остальных строк с учетом значения низлежащих
    deltaHue = shiftHue[y];                                                                 // извлекаем положение
    shiftHue[y] = shiftHue[y - 1U];                                                         // подготавлеваем значение смешения для следующего кадра основываясь на предыдущем
    deltaValue = shiftValue[y];                                                             // извлекаем положение
    shiftValue[y] = shiftValue[y - 1U];                                                     // подготавлеваем значение смешения для следующего кадра основываясь на предыдущем

    for (uint8_t x = 0U; x < WIDTH; x++) {                                                  // пересчет координаты x для текущей строки
      if (y >= 8U) {
        drawPixelXY(x, y, 0U);                                                              // стираем строчку с искрами
      }

      uint8_t newX = x % 16U;                                                               // функция поиска позиции значения яркости для матрицы valueMask
      if (y < 8U) {                                                                         // если строка представляет очаг
        nextv = (int32_t)(((inv_pcnt * matrixValue[y][newX]) +                              // расчет значения яркости относительно valueMask и нижерасположенной строки
                          ((uint16_t)pcnt * matrixValue[y - 1U][newX])) / 100U)
                - pgm_read_byte(&valueMask[y][(x + deltaValue) % 16U]);

        CRGB color = CHSV(                                                                  // определение цвета пикселя
                       baseHue + pgm_read_byte(&hueMask[y][(x + deltaHue) % 16U]),          // H - смещение всполохов
                       baseSat,                                                             // S - когда колесо масштаба =100 - белый огонь (экономим на 1 эффекте)
                       (uint8_t)std::max((int32_t)0, nextv)                                 // V
                     );
        leds[XY(x, y)] = color;
      }
      else if (y == 8U && SPARKLES) {                                                       // если это самая нижняя строка искр - формитуем искорку из пламени
        if (random8(20U) == 0 && getPixColorXY(x, y - 1U) != 0U)
          drawPixelXY(x, y, getPixColorXY(x, y - 2U));                                      // 20 = обратная величина количества искр
      }
      else if (SPARKLES) {                                                                  // если это не самая нижняя строка искр - перемещаем искорку выше
        // старая версия для яркости
        newX = (random8(4U)) ? x : (x + WIDTH + random8(2U) - random8(2U)) % WIDTH ;        // с вероятностью 1/3 смещаем искорку влево или вправо
        if (getPixColorXY(x, y - 1U) > 0U)
          drawPixelXY(newX, y, getPixColorXY(x, y - 1U));                                   // рисуем искорку на новой строчке
      }
    }
  }
}
#endif


#ifdef DEF_RAINBOW_VER
// ------------- радуга три в одной -------------
static void rainbowHorVertRoutine(bool isVertical) {
  const uint8_t outer_limit = isVertical ? WIDTH : HEIGHT;
  const uint8_t inner_limit = isVertical ? HEIGHT : WIDTH;

  const uint8_t step = (modes[currentMode].Scale % 67U) * 2U;

  for (uint8_t i = 0U; i < outer_limit; i++) {
    CHSV thisColor = CHSV((uint8_t)(hue + i * step), 255U, 255U);

    for (uint8_t j = 0U; j < inner_limit; j++)
      drawPixelXY(isVertical ? i : j, isVertical ? j : i, thisColor);
  }
}

static void rainbowRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        uint8_t tmp = 7U + random8(50U);
        if (tmp>14) tmp += 19U;
        if (tmp>67) tmp += 6U;
        setModeSettings(tmp, 150U + random8(86U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
  }

  hue += 4U;

  const uint8_t current_scale = modes[currentMode].Scale;
  if (current_scale < 34U)                                                   // если масштаб до 34
    rainbowHorVertRoutine(false);
  else if (current_scale > 67U)                                              // если масштаб больше 67
    rainbowHorVertRoutine(true);
  else {                                                                     // для масштабов посередине
    // Оригинал: twirlFactor = 9.0f * (current_scale - 33) / 100.0f
    // В целых числах с точностью 1/256: (9 * 256 * (current_scale - 33)) / 100 = (2304 * (current_scale - 33)) / 100
    // На сколько оборотов будет закручена матрица, [0..3]
    uint32_t twirl_fixed = (2304U * (uint32_t)(current_scale - 33U)) / 100U;

    // Множитель (255 / MAX_SIDE) также переводим в масштаб Fixed-Point.
    // Шаг по X: ((float)WIDTH / (float)HEIGHT) * (255.0f / MAX_SIDE)
    float stepX_f = ((float)WIDTH / (float)HEIGHT) * (255.0f / (float)MAX_SIDE);
    uint16_t stepX_fixed = (uint16_t)(stepX_f * 256.0f);

    // Шаг по Y: twirlFactor * (255.0f / MAX_SIDE)
    // Так как twirl_fixed уже умножен на 256, просто умножаем на 255 и делим на MAX_SIDE
    uint32_t stepY_fixed = (twirl_fixed * 255U) / MAX_SIDE;

    for (uint8_t i = 0U; i < WIDTH; i++) {
      // Предрассчитываем базовую составляющую цвета для текущего столбца X
      // Сдвиг >> 8 возвращает число из Fixed-Point обратно в диапазон 0..255
      uint16_t base_x_color = (i * stepX_fixed);

      for (uint8_t j = 0U; j < HEIGHT; j++) {
        // Итоговый сдвиг цвета: hue + (компонента_X + компонента_Y) -> сдвиг обратно из 16-битного Fixed-Point
        uint8_t calculated_hue = hue + (uint8_t)((base_x_color + (j * stepY_fixed)) >> 8);
        drawPixelXY(i, j, CHSV(calculated_hue, 255U, 255U));
      }
    }
  }
}
#endif


#if defined(DEF_PULSE) || defined(DEF_PULSE_RAINBOW) || defined(DEF_PULSE_WHITE)
// -------------- эффект пульс ------------
// Stefan Petrick's PULSE Effect mod by PalPalych for GyverLamp

// CRGBPalette16 palette; не используется
// uint8_t currentRadius = 4; // будет pcnt
// uint8_t pulsCenterX = 0;   // random8(WIDTH - 5U) + 3U;
// uint8_t pulsCenterY = 0;   // random8(HEIGHT - 5U) + 3U;
// uint16_t _rc;              // вроде, не используется
// uint8_t _pulse_hue;        //  заменено на deltaHue из общих переменных
// uint8_t _pulse_hueall;     // заменено на hue2 из общих переменных
// uint8_t _pulse_delta;      // заменено на deltaHue2 из общих переменных
// uint8_t pulse_hue;         // заменено на hue из общих переменных

static void pulseRoutine(uint8_t PMode) {
  if (loadingFlag) {
    loadingFlag = false;
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        setModeSettings(1U + random8(100U), 170U+random8(62U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
  }

  CRGB _pulse_color;

  dimAll(248U);

  if (step <= pcnt) {
    uint8_t base_hue = 0U;
    const uint8_t scale_val = modes[currentMode].Scale;

    // case 1U:                    // 1 - случайные диски
    // case 2U:                    // 2...17 - перелив цвета дисков
    // case 3U:                    // 18...33 - выбор цвета дисков
    // case 4U:                    // 34...50 - дискоцветы
    // case 5U:                    // 51...67 - пузыри цветы
    // case 6U:                    // 68...83 - выбор цвета пузырей
    // case 7U:                    // 84...99 - перелив цвета пузырей
    // case 8U:                    // 100 - случайные пузыри

    if (PMode == 1U) base_hue = hue;
    else if (PMode == 2U || PMode == 7U || PMode == 8U) base_hue = hue2;
    else if (PMode == 3U || PMode == 6U) base_hue = scale8(scale_val, 255U); // Замена Scale * 2.55f

    const bool is_bubble_mode = (PMode >= 5U);
    const uint16_t angle_step = 128U / (step + 1U);

    for (uint8_t i = 0U; i < step; i++) {
      uint8_t cos_val = cos8(angle_step * (i + 1U));

      uint8_t _dark = qmul8(2U, cos_val);
      uint8_t _sat  = is_bubble_mode ? qsub8(255U, cos_val) : 255U;

      if (PMode == 4U || PMode == 5U) {
        deltaHue += scale_val;
        _pulse_color = CHSV(deltaHue, _sat, _dark);
      } else {
        _pulse_color = CHSV(base_hue, _sat, _dark);
      }

      drawCircle(emitterX, emitterY, i, _pulse_color);
    }
  }
  else
  {
    emitterX = random8(WIDTH - 5U) + 3U;
    emitterY = random8(HEIGHT - 5U) + 3U;
    deltaHue2 = modes[currentMode].Scale;
    hue2 += deltaHue2;
    hue = random8(0U, 255U);
    pcnt = random8(WIDTH >> 2U, (WIDTH >> 1U) + 1U);
    step = 0;
  }

  step++;
}
#endif

#ifdef DEF_POOL
// ------------- цвет + вода в бассейне ------------------
// (с) SottNick. 03.2020
// эффект иммеет шов на стыке краёв матрицы (сзади лампы, как и у других эффектов), зато адаптирован для нестандартных размеров матриц.
// можно было бы сделать абсолютно бесшовный вариант для конкретной матрицы (16х16), но уже была бы заметна зацикленность анимации.

// далее идёт массив из 25 кадров анимации с маской бликов на воде (размер картинки больше размера матрицы, чтобы повторяемость картинки была незаметной)
// бесшовную анимированную текстуру бликов делал в программе Substance Designer (30 дней бесплатно работает) при помощи плагина Bruno Caustics Generator
// но сразу под такой мелкий размер текстура выходит нечёткой, поэтому пришлось делать крупную и потом в фотошопе доводить её до ума
// конвертировал в массив через сервис https://littlevgl.com/image-to-c-array,
// чтобы из ч/б картинки получить массив для коррекции параметра насыщенности цвета, использовал настройки True color -> C array
// последовательность замен полученных блоков массива в ворде: "^p  0x"->"^p  {0x"  ...  ", ^p"->"},^p" ... "},^p#endif"->"}^p },^p {"
// static const uint8_t aquariumGIF[25][32][32] PROGMEM =
//
// uint8_t step = 0U;  // GIFframe = 0U; текущий кадр анимации (не важно, какой в начале)
// uint8_t deltaHue = 0U; // GIFshiftx = 0U; какой-то там сдвиг текстуры по радиусу лампы
// uint8_t deltaHue2 = 0U; // GIFshifty = 0U; какой-то там сдвиг текстуры по высоте

#define CAUSTICS_BR                     (100U)                // яркость бликов в процентах (от чистого белого света)

static void poolRoutine()
{
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(47U + random8(28U), 201U + random8(38U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    hue = scale8(modes[currentMode].Scale, 255U); // Scale * 2.55f;
    fillAll(CHSV(hue, 255U, 255U));
    deltaHue = 0U;
    deltaHue2 = 0U;

    loadingFlag = false;
  }

  if (modes[currentMode].Speed != 255U) // если регулятор скорости на максимуме, то будет работать старый эффект "цвет" (без анимации бликов воды)
  {
    if (step > 24U) {                   // количество кадров в анимации -1 (отсчёт с нуля)
      step = 0U;
    }

    if (step > 0U && step < 3U) {       // пару раз за цикл анимации двигаем текстуру по радиусу лампы. а может и не двигаем. как повезёт
      if (random8(2U) == 0U) {
        deltaHue++;
        if (deltaHue > 31U) {
          deltaHue = 0U;
        }
      }
    }

    if (step > 11U && step < 14U) {     // пару раз за цикл анимации двигаем текстуру по вертикали. а может и не двигаем. как повезёт
      if (random8(2U) == 0U) {
        deltaHue2++;
        if (deltaHue2 > 31U) {
          deltaHue2 = 0U;
        }
      }
    }

    // Чтобы регулятор Масштаб начал вместо цвета регулировать яркость бликов, нужно определить #define SCALE_BRI
    #ifdef SCALE_BRI
    uint8_t dynamic_scale = ((uint16_t)modes[currentMode].Scale * 255U) / 100U;
    #endif

    for (uint8_t x = 0U; x < WIDTH ; x++) {
      // y % 32, x % 32 - это для масштабирования эффекта на лампы размером большим, чем размер анимации 32х32, а также для произвольного сдвига текстуры
      // Заменяем тяжелый % 32U быстрой побитовой маской & 31U
      uint8_t texture_x = (x + deltaHue) & 31U;     // (x + deltaHue) % 32U

      for (uint8_t y = 0U; y < HEIGHT; y++) {
        uint8_t texture_y = (y + deltaHue2) & 31U;  // (y + deltaHue2) % 32U

        uint8_t val = pgm_read_byte(&aquariumGIF[step][texture_y][texture_x]);
        #ifdef SCALE_BRI
        uint8_t final_sat = 255U - scale8_video(val, dynamic_scale);
        #else
        uint8_t final_sat = 255U - (val * CAUSTICS_BR) / 100U;
        #endif

        leds[XY(x, y)] = CHSV(hue, final_sat, 255U);
      }
    }
    step++;
  }
}
#endif


#ifdef DEF_COLORS
// ------------- цвета - 2 -----------------
#define DELAY_MULTIPLIER (20U) //при задержке между кадрами примерно в 50 мс с этим множителем получится 1 с на единицу бегунка Скорость

static void colorsRoutine2()
{
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(1U + random8(255U), 210U+random8(46U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    deltaValue = 255U - modes[currentMode].Speed + 1U;

    step = deltaValue; // чтообы при старте эффекта сразу покрасить лампу (для бугунка Масштаб от 246 до 9)
    deltaHue = 1U;     // чтообы при старте эффекта сразу покрасить лампу (для бегунка Масштаб от 10 до 245)
    hue2 = 0U;

    loadingFlag = false;
  }

  const uint8_t current_scale = modes[currentMode].Scale;

  // РЕЖИМ 1: Маленький или очень большой Масштаб — регулярная циклическая смена цвета
  // Меняем цвет на это значение регулярно (каждый цикл кратный значению Скорость)
  if (current_scale < 10U || current_scale > 245U) {
    if (step >= deltaValue) {
      hue += current_scale;
      step = 0U;
      fillAll(CHSV(hue, 255U, 255U));
    } else {
      step++;
    }
  }
  // РЕЖИМ 2: Средний Масштаб — пульсирующее качание цвета с паузами
  // Смену цвета делаем как бы пульсацией (поменяли, пауза, поменяли, пауза)
  else {
    if (deltaHue != 0) {
      // Плавное перетекание тона (эффект "качания" вокруг оси цвета)
      if (deltaHue > 127U) {
        hue--;
        deltaHue++;
      } else {
        hue++;
        deltaHue--;
      }
      fillAll(CHSV(hue, 255U, 255U));
    } else {
      // Блок таймеров и накопления задержек, когда качание завершено (deltaHue == 0)
      if (step >= deltaValue) {
        deltaHue = current_scale;
        step = 0U;
      } else {
        if (hue2 >= DELAY_MULTIPLIER) {
          step++;
          hue2 = 0U;
        } else {
          hue2++;
        }
      }
    }
  }
}
#endif


#ifdef DEF_SNOW
// ------------- снегопад ----------
static void snowRoutine()
{
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(88U + random8(9U), 170U + random8(36U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
  }

  // Сдвигаем всё вниз
  for (uint8_t x = 0U; x < WIDTH; x++) {
    for (uint8_t y = 0U; y < MAX_Y; y++) {
      // Читаем пиксель строго сверху XY(x, y + 1) и записываем его вниз XY(x, y)
      leds[XY(x, y)] = leds[XY(x, y + 1U)];
    }
  }

  // Кэшируем масштаб
  const uint8_t current_scale = modes[currentMode].Scale;
  const uint8_t rand_limit = (current_scale >= 100U) ? 1U : (100U - current_scale);

  // --- Отрисовка новой верхней строки ---
  for (uint8_t x = 0U; x < WIDTH; x++) {
    // Заполняем случайно верхнюю строку
    // а также не даём двум блокам по вертикали вместе быть
    if (leds[XY(x, HEIGHT - 2U)] == CRGB::Black && (random8(rand_limit) == 0U)) {
      // Вычисляем HEX-маску вычитания.
      // Шаги: 0x000000, 0x101010, 0x202020, 0x303030
      uint32_t subtract_mask = (uint32_t)random8(4U) * 0x101010UL;

      // Вычитаем маску из базового цвета.
      // FastLED сделает это мгновенно на уровне регистров процессора.
      leds[XY(x, MAX_Y)] = (uint32_t)0xE0FFFFUL - subtract_mask;
    } else {
      // Гасим пиксель, если снежинка не родилась
      leds[XY(x, MAX_Y)] = CRGB::Black;
    }
  }
}
#endif


#ifdef DEF_STARFALL
// ------------- метель ----------------------------------------------
// SNOWSTORM / МЕТЕЛЬ # STARFALL / ЗВЕЗДОПАД
// v1.0 - Updating for GuverLamp v1.7 by PalPalych 12.03.2020
// v1.1 - Fix wrong math & full screen drawing by PalPalych 14.03.2020
// v1.2 - Code optimisation + pseudo 3d by PalPalych 21.04.2020
// v1.3 - Code optimisation by andrewjswan 19.05.2026

#define e_sns_DENSE (32U)    // плотность снега - меньше = плотнее

static void stormRoutine2()  // сворачиваем 2 эффекта в 1
{
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        uint8_t tmp = 175U + random8(39U);
        if (tmp & 0x01) {
          setModeSettings(50U + random8(51U), tmp);
        } else {
          setModeSettings(50U + random8(24U), tmp);
        }
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
  }

  const bool isColored = modes[currentMode].Speed & 0x01;  // сворачиваем 2 эффекта в 1
  const uint8_t current_scale = modes[currentMode].Scale;

  // заполняем головами комет
  uint8_t Saturation = 0U;    // цвет хвостов
  uint8_t e_TAIL_STEP = 127U; // длина хвоста

  if (isColored) {
    Saturation = scale8(current_scale, 255U); // Scale * 2.55f
  } else {
    e_TAIL_STEP = 255U - scale8(current_scale, 250U); // Scale * 2.5f
  }

  // Случайная генерация голов комет на верхней строчке
  for (uint8_t x = 0U; x < MAX_X; x++) {
    if (random8(e_sns_DENSE) == 0U) {
      uint8_t cx  = wrapX(x);

      if (leds[XY(cx,           MAX_Y)] == CRGB::Black &&
          leds[XY(wrapX(x + 1), MAX_Y)] == CRGB::Black &&
          leds[XY(wrapX(x - 1), MAX_Y)] == CRGB::Black) {
        leds[XY(cx, MAX_Y)] = CHSV(random8(), Saturation, random8(64U, 255U));
      }
    }
  }

  // Сдвигаем по диагонали
  for (int8_t y = HEIGHT - 2U; y >= 0; y--) {
    for (uint8_t x = 0U; x < WIDTH; x++) {
      leds[XY(wrapX(x + 1U), y)] = leds[XY(x, y + 1U)];
    }
  }

  // Уменьшаем яркость верхней линии, формируем "хвосты"
  for (uint8_t i = 0U; i < WIDTH; i++) {
    fadePixel(i, MAX_Y, e_TAIL_STEP);
  }
}
#endif


#ifdef DEF_MATRIX
// ------------- матрица ---------------
static void matrixRoutine()
{
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(1U + random8(90U), 165U + random8(66U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
  }

  const uint8_t current_scale = modes[currentMode].Scale;
  // Безопасный инвертированный масштаб для генератора случайных чисел
  uint8_t scale_inverted = (current_scale >= 100U) ? 1U : (100U - current_scale);
  // Предрассчитываем лимиты для random16, убирая умножение из внутренних циклов
  uint16_t spawn_limit = (uint16_t)scale_inverted * HEIGHT;                                      // HEIGHT * 3 - для длинных хвостов
  uint16_t head_ignore_limit = 7U * HEIGHT;

  for (uint8_t x = 0U; x < WIDTH; x++) {
    // обрабатываем нашу матрицу снизу вверх до второй сверху строчки
    for (uint8_t y = 0U; y < MAX_Y; y++) {
      uint32_t thisColor  = (uint32_t)leds[XY(x, y)];                                            // берём цвет нашего пикселя
      uint32_t upperColor = (uint32_t)leds[XY(x, y + 1U)];                                       // берём цвет пикселя над нашим

      if (upperColor >= 0x900000 && random16(head_ignore_limit) != 0U) {                         // если выше нас максимальная яркость, игнорим этот факт с некой вероятностью или опускаем цепочку ниже
        leds[XY(x, y)] = upperColor;
      } else if (thisColor == 0U && random16(spawn_limit) == 0U) {                               // если наш пиксель ещё не горит, иногда зажигаем новые цепочки
        leds[XY(x, y)] = 0x9BF800;
      } else if (thisColor <= 0x050800) {                                                        // если наш пиксель почти погас, стараемся сделать затухание медленней
        if (thisColor >= 0x030000) {
          leds[XY(x, y)] = 0x020300;
        } else if (thisColor != 0U) {
          leds[XY(x, y)] = CRGB::Black;
        }
      } else if (thisColor >= 0x900000) {                                                        // если наш пиксель максимальной яркости, резко снижаем яркость
        leds[XY(x, y)] = 0x558800;
      } else {
        leds[XY(x, y)] = thisColor - 0x0A1000;                                                   // в остальных случаях снижаем яркость на 1 уровень
        // leds[XY(x, y)] = thisColor - 0x050800;                                                // для длинных хвостов
      }
    }

    // Аналогично обрабатываем верхний ряд пикселей матрицы
    const uint8_t top_y = MAX_Y;
    uint32_t thisColor = (uint32_t)leds[XY(x, top_y)];

    if (thisColor == 0U) {                                                                       // если наш верхний пиксель не горит, заполняем его с вероятностью .Scale
      if (random8(scale_inverted) == 0U) {
        leds[XY(x, top_y)] = 0x9BF800;
      }
    } else if (thisColor <= 0x050800) {                                                         // если наш верхний пиксель почти погас, стараемся сделать затухание медленней
      if (thisColor >= 0x030000) {
        leds[XY(x, top_y)] = 0x020300;
      } else {
        leds[XY(x, top_y)] = CRGB::Black;
      }
    } else if (thisColor >= 0x900000) {                                                         // если наш верхний пиксель максимальной яркости, резко снижаем яркость
      leds[XY(x, top_y)] = 0x558800;
    } else {
      leds[XY(x, top_y)] = thisColor - 0x0A1000;                                                // в остальных случаях снижаем яркость на 1 уровень
      // leds[XY(x, top_y)] = thisColor - 0x050800;                                             // для длинных хвостов
    }
  }
}
#endif


#if defined(DEF_BUTTERFLYS) || defined(DEF_BUTTERFLYS_LAMP)
// ------------- Светлячки 2 - Светлячки в банке - Мотыльки - Лампа с мотыльками --------------
// (c) SottNick
// optimization by andrewjswan

// #define trackingOBJECT_MAX_COUNT           (100U)      // максимальное количество мотыльков
#define BUTTERFLY_FIX_COUNT               (20U)           // количество мотыльков для режима, когда бегунок Масштаб регулирует цвет
// float trackingObjectPosX[trackingOBJECT_MAX_COUNT];
// float trackingObjectPosY[trackingOBJECT_MAX_COUNT];
// float trackingObjectSpeedX[trackingOBJECT_MAX_COUNT];
// float trackingObjectSpeedY[trackingOBJECT_MAX_COUNT];
// float trackingObjectShift[trackingOBJECT_MAX_COUNT];
// uint8_t trackingObjectHue[trackingOBJECT_MAX_COUNT];
// uint8_t trackingObjectState[trackingOBJECT_MAX_COUNT];

static void butterflysRoutine(bool isColored)
{
  const bool isWings = modes[currentMode].Speed & 0x01;
  const uint8_t current_scale = modes[currentMode].Scale;

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      if (isColored){
        uint8_t tmp = 66U + random8(83U);
        setModeSettings((tmp & 0x01) ? 65U + random8(36U) : 15U + random8(26U), tmp);
      } else {
        setModeSettings(random8(21U) ? (random8(3U) ? 2U + random8(98U) : 1U) : 100U, 20U + random8(155U));
      }
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    speedfactor = (float)modes[currentMode].Speed / 2048.0f + 0.001f;
    if (isColored) { // для режима смены цвета фона фиксируем количество мотыльков
      deltaValue = (current_scale > trackingOBJECT_MAX_COUNT) ? trackingOBJECT_MAX_COUNT : modes[currentMode].Scale;
    } else {
      deltaValue = BUTTERFLY_FIX_COUNT;
    }

    for (uint8_t i = 0U; i < trackingOBJECT_MAX_COUNT; i++) {
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = random8(HEIGHT);
      trackingObjectSpeedX[i] = 0.0f;
      trackingObjectSpeedY[i] = 0.0f;
      trackingObjectShift[i] = 0.0f;
      trackingObjectHue[i] = (isColored) ? random8() : 255U;
      trackingObjectState[i] = 255U;
    }

    // для инверсии, чтобы сто раз не пересчитывать
    if (current_scale != 1U) {
      hue = ((uint16_t)(current_scale - 1U) * 26U) / 10U;
    } else {
      hue = random8();
    }

    if (current_scale == 100U) { // вместо белого будет желтоватая лампа
      hue = 31U;
      hue2 = 170U;
    } else {
      hue2 = 255U;
    }

    loadingFlag = false;
  }

  if (isWings && isColored) {
    dimAll(35U);                           // для крылышков
  } else {
    ledsClear();                           // esphome: FastLED.clear();
  }

  if (++step >= deltaValue)
    step = 0U;

  for (uint8_t i = 0U; i < deltaValue; i++) {
    float sp_X = trackingObjectSpeedX[i];
    float sp_Y = trackingObjectSpeedY[i];

    trackingObjectPosX[i] += sp_X * speedfactor;
    trackingObjectPosY[i] += sp_Y * speedfactor;

    // Циклическая телепортация по горизонтали X
    if (trackingObjectPosX[i] < 0.0f) {
      trackingObjectPosX[i] = MAX_X + trackingObjectPosX[i];
    }
    if (trackingObjectPosX[i] > MAX_X) {
      trackingObjectPosX[i] = trackingObjectPosX[i] + 1.0f - (float)WIDTH;
    }

    // Отскок от жестких вертикальных краев Y
    if (trackingObjectPosY[i] < 0.0f) {
      trackingObjectPosY[i] = -trackingObjectPosY[i];
      sp_Y = -sp_Y;
    }
    if (trackingObjectPosY[i] > MAX_Y) {
      trackingObjectPosY[i] = (float)(HEIGHT << 1U) - 2.0f - trackingObjectPosY[i];
      sp_Y = -sp_Y;
    }

    // Проворот траектории полета мотылька
    float absX = std::abs(sp_X);
    float absY = std::abs(sp_Y);
    float maxspeed = absX + absY;                                                      // максимальная суммарная скорость

    if (maxspeed == std::abs(sp_X + sp_Y))
    {
      if (sp_X > 0.0f) {                                                               // правый верхний сектор вектора
        sp_X += trackingObjectShift[i];
        if (sp_X > maxspeed) {                                                         // если вектор переехал вниз
          sp_X = maxspeed + maxspeed - sp_X;
          sp_Y = sp_X - maxspeed;
        } else {
          sp_Y = maxspeed - std::abs(sp_X);
        }
      } else {                                                                         // левый нижний сектор
        sp_X -= trackingObjectShift[i];
        if (sp_X + maxspeed < 0.0f) {                                                  // если вектор переехал вверх
          sp_X = 0.0f - sp_X - maxspeed - maxspeed;
          sp_Y = maxspeed - std::abs(sp_X);
        } else {
          sp_Y = std::abs(sp_X) - maxspeed;
        }
      }
    } else {                                                                           // левый верхний и правый нижний секторы вектора
      if (sp_X > 0.0f)                                                                 // правый нижний сектор
      {
        sp_X -= trackingObjectShift[i];
        if (sp_X > maxspeed) {                                                        // если вектор переехал наверх
          sp_X = maxspeed + maxspeed - sp_X;
          sp_Y = maxspeed - sp_X;
        } else {
          sp_Y = std::abs(sp_X) - maxspeed;
        }
      } else {                                                                         // левый верхний сектор
        sp_X += trackingObjectShift[i];
        if (sp_X + maxspeed < 0.0f) {                                                  // если вектор переехал вниз
          sp_X = 0.0f - sp_X - maxspeed - maxspeed;
          sp_Y = 0.0f - sp_X - maxspeed;
        } else {
          sp_Y = maxspeed - std::abs(sp_X);
        }
      }
    }

    // Возвращаем измененную скорость обратно в массив
    trackingObjectSpeedX[i] = sp_X;
    trackingObjectSpeedY[i] = sp_Y;

    // Полет или приземление
    if (trackingObjectState[i] == 255U) {
      if (step == i && random8(2U) == 0U) {
        trackingObjectState[i] = random8(220U, 244U);

        float randX = ((float)random8(101U) * inv20) + 1.0f; // (float)random8(101U) / 20.0f + 1.0f
        trackingObjectSpeedX[i] = (random8(2U) == 0U) ? -randX : randX;

        float randY = ((float)random8(101U) * inv20) + 1.0f; // (float)random8(101U) / 20.0f + 1.0f
        trackingObjectSpeedY[i] = (random8(2U) == 0U) ? -randY : randY;

        // проворот траектории
        uint8_t random_limit = (uint8_t)((std::abs(trackingObjectSpeedX[i]) + std::abs(trackingObjectSpeedY[i])) * 20.0f + 2.0f);
        float shift_val = (float)random8(random_limit) * inv200; // / 200.0f;
        trackingObjectShift[i] = (random8(2U) == 0U) ? -shift_val : shift_val;
      }
    } else {
      if (step == i) {
        trackingObjectState[i]++;
      }

      uint8_t state_tmp = 255U - trackingObjectState[i];
      if (state_tmp == 0U || ((uint16_t)(trackingObjectPosX[i] * state_tmp) % state_tmp == 0U && (uint16_t)(trackingObjectPosY[i] * state_tmp) % state_tmp == 0U)) {
        trackingObjectPosX[i] = (int)(trackingObjectPosX[i] + 0.5f);  // round(trackingObjectPosX[i]);
        trackingObjectPosY[i] = (int)(trackingObjectPosY[i] + 0.5f);  // round(trackingObjectPosY[i]);
        trackingObjectSpeedX[i] = 0.0f;
        trackingObjectSpeedY[i] = 0.0f;
        trackingObjectShift[i] = 0.0f;
        trackingObjectState[i] = 255U;
      }
    }

    // Финальная яркость мотылька
    uint8_t final_v;
    if (isWings) {
      final_v = (trackingObjectState[i] == 255U) ? 255U : 128U + (random8(2U) * 111U);
    } else {
      final_v = trackingObjectState[i];
    }
    drawPixelXYF(trackingObjectPosX[i], trackingObjectPosY[i], CHSV(trackingObjectHue[i], 255U, final_v));
  }

  // Постобработка кадра
  if (isColored) {
    // Ещё раз рисуем всех Мотыльков, которые "сидят на стекле"
    for (uint8_t i = 0U; i < deltaValue; i++) {
      if (trackingObjectState[i] == 255U) {
        drawPixelXY(trackingObjectPosX[i], trackingObjectPosY[i], CHSV(trackingObjectHue[i], 255U, 255U));
      }
    }
  } else {
    // Теперь инверсия всей матрицы
    if (current_scale == 1U) {
      if (++deltaHue == 0U) {
        hue++;
      }
    }

    // Быстрая поканальная инверсия заднего фона всей матрицы
    for (uint16_t i = 0U; i < NUM_LEDS; i++) {
      leds[i] = CHSV(hue, hue2, qsub8(255U, leds[i].r));
    }
  }
}
#endif


#ifdef DEF_LIGHTERS
// ------------- светлячки --------------
// #define LIGHTERS_AM           (100U)  // для экономии памяти берём trackingOBJECT_MAX_COUNT
// #define trackingOBJECT_MAX_COUNT
// int32_t lightersPos[2U][LIGHTERS_AM];
// float trackingObjectPosX[trackingOBJECT_MAX_COUNT];
// float trackingObjectPosY[trackingOBJECT_MAX_COUNT];
// int8_t lightersSpeed[2U][LIGHTERS_AM];
// float trackingObjectSpeedX[trackingOBJECT_MAX_COUNT];
// float trackingObjectSpeedY[trackingOBJECT_MAX_COUNT];
// CHSV lightersColor[LIGHTERS_AM];
// uint8_t trackingObjectHue[trackingOBJECT_MAX_COUNT];
// uint8_t step; // раньше называлось uint8_t loopCounter;
static void lightersRoutine()
{
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(14U+random8(43U), 100U+random8(81U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    if (modes[currentMode].Scale > trackingOBJECT_MAX_COUNT) {
      modes[currentMode].Scale = trackingOBJECT_MAX_COUNT;
    }

    for (uint8_t i = 0U; i < trackingOBJECT_MAX_COUNT; i++)
    {
      trackingObjectPosX[i]   = (int16_t)random8(WIDTH) * 10;   // random(0, WIDTH * 10);
      trackingObjectPosY[i]   = (int16_t)random8(HEIGHT) * 10;  // random(0, HEIGHT * 10);
      trackingObjectSpeedX[i] = (int16_t)random8(21U) - 10;     // random(-10, 10);
      trackingObjectSpeedY[i] = (int16_t)random8(21U) - 10;     // random(-10, 10);
      trackingObjectHue[i]    = random8();
    }

    loadingFlag = false;
  }

  ledsClear(); // esphome: FastLED.clear();

  if (++step > 20U) step = 0U;

  const uint8_t lighters_count = modes[currentMode].Scale;

  // Предрассчитываем константные границы сетки х10 для быстрой проверки
  constexpr int16_t max_x_ten = MAX_X * 10U;
  constexpr int16_t max_y_ten = MAX_Y * 10U;
  constexpr int16_t limit_x_ten = WIDTH * 10U;

  for (uint8_t i = 0U; i < lighters_count; i++) {
    int16_t pos_x = trackingObjectPosX[i];
    int16_t pos_y = trackingObjectPosY[i];
    int16_t sp_x  = trackingObjectSpeedX[i];
    int16_t sp_y  = trackingObjectSpeedY[i];

    if (step == 0U) { // меняем скорость каждые 255 отрисовок
      sp_x += ((int16_t)random8(7U) - 3);  // random(-3, 4);
      sp_y += ((int16_t)random8(7U) - 3);  // random(-3, 4);

      sp_x = std::clamp((int)sp_x, -20, 20);  // constrain(trackingObjectSpeedX[i], -20, 20);
      sp_y = std::clamp((int)sp_y, -20, 20);  // constrain(trackingObjectSpeedY[i], -20, 20);
    }

    pos_x += sp_x;
    pos_y += sp_y;

    // Бесшовное зацикливание по горизонтали X
    if (pos_x < 0) {
      pos_x = max_x_ten;
    }
    if (pos_x >= limit_x_ten) {
      pos_x = 0;
    }

    // Жесткий отскок от верхнего и нижнего краев Y
    if (pos_y < 0) {
      pos_y = 0;
      sp_y = -sp_y;
    }
    if (pos_y >= max_y_ten) {
      pos_y = max_y_ten;
      sp_y = -sp_y;
    }

    trackingObjectPosX[i]   = pos_x;
    trackingObjectPosY[i]   = pos_y;
    trackingObjectSpeedX[i] = sp_x;
    trackingObjectSpeedY[i] = sp_y;

    uint8_t screen_x = (uint16_t)pos_x / 10U;
    uint8_t screen_y = (uint16_t)pos_y / 10U;
    drawPixelXY(screen_x, screen_y, CHSV(trackingObjectHue[i], 255U, 255U));
  }
}
#endif


#ifdef DEF_LIGHTER_TRACES
// --------------------------- светлячки со шлейфом ---------------------
#define BALLS_AMOUNT          (3U)                          // количество "шариков"
#define CLEAR_PATH            (1U)                          // очищать путь
#define BALL_TRACK            (1U)                          // (0 / 1) - вкл/выкл следы шариков
#define TRACK_STEP            (70U)                         // длина хвоста шарика (чем больше цифра, тем хвост короче)

static int16_t coord[BALLS_AMOUNT][2U];
static int8_t vector[BALLS_AMOUNT][2U];
static CRGB ballColors[BALLS_AMOUNT];

static void ballsRoutine()
{
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      setModeSettings(1U + random8(100U) , 190U + random8(31U));
    }
    #endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    const int16_t start_x = (int16_t)CENTER_X * 10;
    const int16_t start_y = (int16_t)CENTER_Y * 10;
    const uint8_t current_scale = modes[currentMode].Scale;

    for (uint8_t j = 0U; j < BALLS_AMOUNT; j++) {
      // забиваем случайными данными
      coord[j][0U] = start_x;
      int8_t sign_x = (random8(2U) == 0U) ? 1 : -1;
      vector[j][0U] = (int8_t)random8(4U, 15U) * sign_x; // оригинальный диапазон [4..14]

      coord[j][1U] = start_y;
      int8_t sign_y = (random8(2U) == 0U) ? 1 : -1;
      vector[j][1U] = (int8_t)random8(4U, 15U) * sign_y; // оригинальный диапазон [4..14]

      // Цвет зависит от масштаба
      ballColors[j] = CHSV((current_scale * (j + 1U)) % 256U, 255U, 255U);
    }

    loadingFlag = false;
  }

  // Управление шлейфом (хвостами) светлячков
  if (!BALL_TRACK) {                                        // режим без следов шариков
    ledsClear(); // esphome: FastLED.clear();
  } else {                                                  // режим со следами
    dimAll(256U - TRACK_STEP);
  }

  // Предрассчитываем жесткие границы сетки x10 для осей X и Y
  constexpr int16_t max_x_ten = MAX_X * 10U;
  constexpr int16_t max_y_ten = MAX_Y * 10U;

  // Движение и отрисовка шариков
  for (uint8_t j = 0U; j < BALLS_AMOUNT; j++) {
    for (uint8_t i = 0U; i < 2U; i++) {
      coord[j][i] += vector[j][i];
      if (coord[j][i] < 0) {
        coord[j][i] = 0;
        vector[j][i] = -vector[j][i];
      }
    }

    if (coord[j][0U] > max_x_ten) {
      coord[j][0U] = max_x_ten;
      vector[j][0U] = -vector[j][0U];
    }
    if (coord[j][1U] > max_y_ten) {
      coord[j][1U] = max_y_ten;
      vector[j][1U] = -vector[j][1U];
    }

    float render_x = (float)coord[j][0U] * inv10;
    float render_y = (float)coord[j][1U] * inv10;
    drawPixelXYF(render_x, render_y, ballColors[j]);
  }
}
#endif


#ifdef DEF_PAINTBALL
// ------------- пейнтбол -------------
#define BORDERTHICKNESS (1U)  // глубина бордюра для размытия яркой частицы:
                              // 0U - без границы (резкие края)
                              // 1U - 1 пиксель (среднее размытие)
                              // 2U - 2 пикселя (глубокое размытие)
constexpr uint8_t paintWidth = WIDTH - BORDERTHICKNESS * 2U;
constexpr uint8_t paintHeight = HEIGHT - BORDERTHICKNESS * 2U;

static void lightBallsRoutine()
{
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(1U + random8(100U) , 230U + random8(16U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
  }

  // Apply some blurring to whatever's already on the matrix
  // Note that we never actually clear the matrix, we just constantly
  // blur it repeatedly. Since the blurring is 'lossy', there's
  // an automatic trend toward black -- by design.
  // uint8_t blurAmount = dim8_raw(beatsin8(3, 64, 100));
  // blur2d(leds, WIDTH, HEIGHT, blurAmount);
  blurScreen(dim8_raw(beatsin8(3, 64, 100)));

  // Use two out-of-sync sine waves
  const uint16_t i = beatsin16(79, 0, 255); // 91
  const uint16_t j = beatsin16(67, 0, 255); // 109
  const uint16_t k = beatsin16(53, 0, 255); // 73
  const uint16_t m = beatsin16(97, 0, 255); // 123

  // The color of each point shifts over time, each at a different speed.
  const uint32_t ms = millis() / (((uint32_t)modes[currentMode].Scale >> 2U) + 1U); // Scale / 4 + 1

  leds[XY(highByte(i * paintWidth) + BORDERTHICKNESS, highByte(j * paintHeight) + BORDERTHICKNESS)] += CHSV(ms / 29, 200U, 255U);
  leds[XY(highByte(j * paintWidth) + BORDERTHICKNESS, highByte(k * paintHeight) + BORDERTHICKNESS)] += CHSV(ms / 41, 200U, 255U);
  leds[XY(highByte(k * paintWidth) + BORDERTHICKNESS, highByte(m * paintHeight) + BORDERTHICKNESS)] += CHSV(ms / 37, 200U, 255U);
  leds[XY(highByte(m * paintWidth) + BORDERTHICKNESS, highByte(i * paintHeight) + BORDERTHICKNESS)] += CHSV(ms / 53, 200U, 255U);
}
#endif


#ifdef DEF_WHITE_COLOR
// ------------- ещё более белый свет (с вертикальным вариантом) -------------
// (c) SottNick
#define BORDERLAND (2U) // две дополнительные единицы бегунка Масштаб на границе вертикального и горизонтального варианта эффекта (с каждой стороны границы) будут для света всеми светодиодами в полную силу
static void whiteColorStripeRoutine()
{
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      setModeSettings(11U + random8(83U), 1U + random8(255U / WIDTH + 1U) * WIDTH);
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    ledsClear(); // esphome: FastLED.clear();

    const uint8_t current_scale = modes[currentMode].Scale;
    const uint8_t current_speed = modes[currentMode].Speed;

    constexpr uint8_t divisor = 50U - BORDERLAND;

    uint8_t thisSize = HEIGHT;
    uint8_t halfScale = current_scale;
    if (halfScale > 50U) {
      thisSize = WIDTH;
      halfScale = 101U - halfScale;
    }
    halfScale = constrain(halfScale, 0U, divisor);

    uint8_t center = (thisSize - 1) >> 1;  // (uint8_t)round(thisSize / 2.0f) - 1U;
    uint8_t offset = (uint8_t)(!(thisSize & 0x01));

    uint16_t product = (uint16_t)center * halfScale;
    uint8_t fullFill = product / divisor;
    uint8_t iPol = ((product % divisor) * 255U) / divisor;

    uint8_t saturation = map(current_speed, 0U, 255U, 0U, 170U);

    int16_t threshold_full = (int16_t)center - fullFill - 1;
    int16_t threshold_fade = (int16_t)center - fullFill - 2;

    for (int16_t i = center; i >= 0; i--) {
      CRGB color = CHSV(
                     45U,                                                                              // определяем тон
                     saturation,                                                                       // определяем насыщенность
                     i > threshold_full                                                                // определяем яркость
                     ? 255U                                                                            // для центральных горизонтальных полос
                     : iPol * (i > threshold_fade));                                                   // для остальных горизонтальных полос яркость равна либо 255, либо 0 в зависимости от масштаба

      if (current_scale <= 50U) {
        uint8_t mirrorY = HEIGHT + offset - i - 2U;
        for (uint8_t x = 0U; x < WIDTH; x++) {
          drawPixelXY(x, i, color);                                                                    // при чётной высоте матрицы максимально яркими отрисуются 2 центральных горизонтальных полосы
          drawPixelXY(x, mirrorY, color);                                                              // при нечётной - одна, но дважды
        }
      } else {
        uint8_t shiftL = ((uint16_t)i + current_speed - 1U) % WIDTH;
        uint8_t shiftR = ((uint16_t)(WIDTH * 2U) + offset - i + current_speed - 3U) % WIDTH;
        for (uint8_t y = 0U; y < HEIGHT; y++) {
          drawPixelXY(shiftL, y, color);                                                               // при чётной ширине матрицы максимально яркими отрисуются 2 центральных вертикальных полосы
          drawPixelXY(shiftR, y, color);                                                               // при нечётной - одна, но дважды
        }
      }
    }

    loadingFlag = false;
  }
}
#endif

// --------------------------- эффект кометы ----------------------

// далее идут общие процедуры для эффектов от Stefan Petrick, а непосредственно Комета - в самом низу
static int8_t zD;
static int8_t zF;
// The coordinates for 3 16-bit noise spaces.
#define NUM_LAYERS (1U) // в кометах используется 1 слой, но для огня 2018 нужно 2

static uint32_t noise32_x[NUM_LAYERSMAX];
static uint32_t noise32_y[NUM_LAYERSMAX];
static uint32_t noise32_z[NUM_LAYERSMAX];
static uint32_t scale32_x[NUM_LAYERSMAX];
static uint32_t scale32_y[NUM_LAYERSMAX];

static uint8_t noisesmooth;
static bool eNs_isSetupped;

static void eNs_setup() {
  noisesmooth = 200;
  for (uint8_t i = 0U; i < NUM_LAYERS; i++) {
    noise32_x[i] = random16();
    noise32_y[i] = random16();
    noise32_z[i] = random16();
    scale32_x[i] = 6000;
    scale32_y[i] = 6000;
  }
  eNs_isSetupped = true;
}

static void FillNoise(int8_t layer) {
  const uint32_t base_noise_x = noise32_x[layer];
  const uint32_t base_noise_y = noise32_y[layer];
  const uint32_t base_noise_z = noise32_z[layer];
  const uint32_t scale_x      = scale32_x[layer];
  const uint32_t scale_y      = scale32_y[layer];

  const uint8_t inv_smooth    = 255U - noisesmooth;

  for (uint8_t i = 0U; i < WIDTH; i++) {
    int32_t ioffset = (int32_t)scale_x * (i - CENTER_X_MINOR);
    uint32_t current_x_noise = base_noise_x + ioffset;

    for (uint8_t j = 0U; j < HEIGHT; j++) {
      int32_t joffset = (int32_t)scale_y * (j - CENTER_Y_MINOR);
      uint32_t current_y_noise = base_noise_y + joffset;

      int8_t data = fastled_helper::perlin16(current_x_noise, current_y_noise, base_noise_z) >> 8;
      int8_t olddata = noise3d[layer][i][j];
      noise3d[layer][i][j] = scale8(olddata, noisesmooth) + scale8(data, inv_smooth);
    }
  }
}

/* эти функции в данных эффектах не используются, но на всякий случай уже адаптированы
static void MoveX(int8_t delta) {
  //CLS2();
  for (uint8_t y = 0U; y < HEIGHT; y++) {
    for (uint8_t x = 0U; x < WIDTH - delta; x++) {
      ledsbuff[XY(x, y)] = leds[XY(x + delta, y)];
    }
    for (uint8_t x = WIDTH - delta; x < WIDTH; x++) {
      ledsbuff[XY(x, y)] = leds[XY(x + delta - WIDTH, y)];
    }
  }
  //CLS();
  // write back to leds
  memcpy(leds, ledsbuff, sizeof(CRGB)* NUM_LEDS);
  //какого хера тут было поштучное копирование - я хз
  //for (uint8_t y = 0U; y < HEIGHT; y++) {
  //  for (uint8_t x = 0U; x < WIDTH; x++) {
  //    leds[XY(x, y)] = ledsbuff[XY(x, y)];
  //  }
  //}
}

static void MoveY(int8_t delta) {
  //CLS2();
  for (uint8_t x = 0U; x < WIDTH; x++) {
    for (uint8_t y = 0U; y < HEIGHT - delta; y++) {
      ledsbuff[XY(x, y)] = leds[XY(x, y + delta)];
    }
    for (uint8_t y = HEIGHT - delta; y < HEIGHT; y++) {
      ledsbuff[XY(x, y)] = leds[XY(x, y + delta - HEIGHT)];
    }
  }
  //CLS();
  // write back to leds
  memcpy(leds, ledsbuff, sizeof(CRGB)* NUM_LEDS);
  //какого хера тут было поштучное копирование - я хз
  //for (uint8_t y = 0U; y < HEIGHT; y++) {
  //  for (uint8_t x = 0U; x < WIDTH; x++) {
  //    leds[XY(x, y)] = ledsbuff[XY(x, y)];
  //  }
  //}
}
*/

static void MoveFractionalNoiseX(int8_t amplitude = 1, float shift = 0) {
  for (uint8_t y = 0U; y < HEIGHT; y++) {
    int16_t amount = ((int16_t)noise3d[0][0][y] - 128) * 2 * amplitude + shift * 256;

    uint16_t abs_amount = (amount < 0) ? -amount : amount;
    int8_t delta = abs_amount >> 8;
    int8_t fraction = abs_amount & 255;

    uint8_t ease_inv_frac = ease8InOutApprox(255 - fraction);
    uint8_t ease_frac     = ease8InOutApprox(fraction);

    for (uint8_t x = 0 ; x < WIDTH; x++) {
      if (amount < 0) {
        zD = x - delta;
        zF = zD - 1;
      } else {
        zD = x + delta;
        zF = zD + 1;
      }

      CRGB PixelA = CRGB::Black  ;
      if ((zD >= 0) && (zD < WIDTH)) PixelA = leds[XY(zD, y)];

      CRGB PixelB = CRGB::Black ;
      if ((zF >= 0) && (zF < WIDTH)) PixelB = leds[XY(zF, y)];

      ledsbuff[XY(x, y)] = (PixelA.nscale8(ease_inv_frac)) + (PixelB.nscale8(ease_frac));
    }
  }
  memcpy(leds, ledsbuff, sizeof(CRGB)* NUM_LEDS);
}

static void MoveFractionalNoiseY(int8_t amplitude = 1, float shift = 0) {
  for (uint8_t x = 0U; x < WIDTH; x++) {
    int16_t amount = ((int16_t)noise3d[0][x][0] - 128) * 2 * amplitude + shift * 256;

    uint16_t abs_amount = (amount < 0) ? -amount : amount;
    int8_t delta = abs_amount >> 8;
    int8_t fraction = abs_amount & 255;

    uint8_t ease_inv_frac = ease8InOutApprox(255 - fraction);
    uint8_t ease_frac     = ease8InOutApprox(fraction);

    for (uint8_t y = 0 ; y < HEIGHT; y++) {
      if (amount < 0) {
        zD = y - delta;
        zF = zD - 1;
      } else {
        zD = y + delta;
        zF = zD + 1;
      }

      CRGB PixelA = CRGB::Black ;
      if ((zD >= 0) && (zD < HEIGHT)) PixelA = leds[XY(x, zD)];

      CRGB PixelB = CRGB::Black ;
      if ((zF >= 0) && (zF < HEIGHT)) PixelB = leds[XY(x, zF)];

      ledsbuff[XY(x, y)] = (PixelA.nscale8(ease_inv_frac)) + (PixelB.nscale8(ease_frac));
    }
  }
  memcpy(leds, ledsbuff, sizeof(CRGB)* NUM_LEDS);
}

#ifdef DEF_COMET_TWO
// NoiseSmearing(by StefanPetrick) Effect mod for GyverLamp by PalPalych
static void MultipleStream() { // 2 comets
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        hue = random8();
        hue2 = hue + 85U;
        setModeSettings(1U + random8(25U), 185U + random8(36U));
      } else {
        hue = 0U;   // 0xFF0000
        hue2 = 43U; // 0xFFFF00
      }
    #else
      hue = 0U;     // 0xFF0000
      hue2 = 43U;   // 0xFFFF00
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    trackingObjectState[0] = OCTANT_X;
    trackingObjectState[1] = OCTANT_Y;

    uint8_t denom = MAX_X - trackingObjectState[0] - trackingObjectState[0];
    trackingObjectShift[0] = (denom > 0) ? (255U / denom) : 1U;
    denom = MAX_Y - trackingObjectState[1] - trackingObjectState[1];
    trackingObjectShift[1] = (denom > 0) ? (255U / denom) : 1U;

    trackingObjectState[2] = QUARTER_X;
    trackingObjectState[3] = QUARTER_Y;

    denom = MAX_X - trackingObjectState[2] - trackingObjectState[2];
    trackingObjectShift[2] = (denom > 0) ? (255U / denom) : 1U;
    denom = MAX_Y - trackingObjectState[3] - trackingObjectState[3];
    trackingObjectShift[3] = (denom > 0) ? (255U / denom) : 1U;

    loadingFlag = false;
  }

  dimAll(255U - (modes[currentMode].Scale << 1));  // * 2);

  const uint32_t current_ms = millis();

  // gelb im Kreis
  uint8_t xx = trackingObjectState[0] + sin8(current_ms / 10U) / trackingObjectShift[0];
  uint8_t yy = trackingObjectState[1] + cos8(current_ms / 10U) / trackingObjectShift[1];
  if (xx < WIDTH && yy < HEIGHT) {
    leds[XY(xx, yy)] = CHSV(hue2, 255, 255);
  }

  // rot in einer Acht
  xx = trackingObjectState[2] + sin8(current_ms / 46U) / trackingObjectShift[2];
  yy = trackingObjectState[3] + cos8(current_ms / 15U) / trackingObjectShift[3];
  if (xx < WIDTH && yy < HEIGHT) {
    leds[XY(xx, yy)] = CHSV(hue, 255, 255);
  }

  // Noise
  noise32_x[0] += 3000U;
  noise32_y[0] += 3000U;
  noise32_z[0] += 3000U;
  scale32_x[0] = 8000U;
  scale32_y[0] = 8000U;

  FillNoise(0);
  MoveFractionalNoiseX(3, 0.33f);
  MoveFractionalNoiseY(3);
}
#endif


#ifdef DEF_COMET_THREE
static void MultipleStream2() { // 3 comets
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        hue = random8();
        hue2 = hue + 85U;
        deltaHue = hue2 + 85U;
        setModeSettings(1U + random8(25U), 185U + random8(36U));
      }
      else{
        hue = 0U;                                                                   // 0xFF0000
        hue2 = 43U;                                                                 // 0xFFFF00
        deltaHue = 171U;                                                            // 0x0000FF;
      }
    #else
      hue = 0U;                                                                     // 0xFF0000
      hue2 = 43U;                                                                   // 0xFFFF00
      deltaHue = 171U;                                                              // 0x0000FF;
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    trackingObjectState[0] = OCTANT_X;
    trackingObjectState[1] = OCTANT_Y;

    uint8_t denom = MAX_X - trackingObjectState[0] - trackingObjectState[0];
    trackingObjectShift[0] = (denom > 0) ? (255U / denom) : 1U;
    denom = MAX_Y - trackingObjectState[1] - trackingObjectState[1];
    trackingObjectShift[1] = (denom > 0) ? (255U / denom) : 1U;

    trackingObjectState[2] = QUARTER_X;
    trackingObjectState[3] = QUARTER_Y;

    denom = MAX_X - trackingObjectState[2] - trackingObjectState[2];
    trackingObjectShift[2] = (denom > 0) ? (255U / denom) : 1U;
    denom = MAX_Y - trackingObjectState[3] - trackingObjectState[3];
    trackingObjectShift[3] = (denom > 0) ? (255U / denom) : 1U;

    loadingFlag = false;
  }

  dimAll(255U - (modes[currentMode].Scale << 1));  // * 2);

  const uint32_t current_ms = millis();

  uint8_t xx = trackingObjectState[0] + sin8(current_ms / 10U) / trackingObjectShift[0];
  uint8_t yy = trackingObjectState[1] + cos8(current_ms / 9U) / trackingObjectShift[1];
  if (xx < WIDTH && yy < HEIGHT)
    leds[XY(xx, yy)] += CHSV(deltaHue, 255, 255);

  xx = trackingObjectState[2] + sin8(current_ms / 10U) / trackingObjectShift[2];
  yy = trackingObjectState[3] + cos8(current_ms / 7U) / trackingObjectShift[3];
  if (xx < WIDTH && yy < HEIGHT)
    leds[XY(xx, yy)] += CHSV(hue, 255, 255);

  leds[XY(CENTER_X_MINOR, CENTER_Y_MINOR)] += CHSV(hue2, 255, 255);

  noise32_x[0] += 3000U;
  noise32_y[0] += 3000U;
  noise32_z[0] += 3000U;
  scale32_x[0] = 8000U;
  scale32_y[0] = 8000U;

  FillNoise(0);
  MoveFractionalNoiseX(2);
  MoveFractionalNoiseY(2, 0.33f);
}
#endif


#ifdef DEF_FIREFLY
static void MultipleStream3() { // Fireline
  if (loadingFlag) {
    loadingFlag = false;
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        setModeSettings(1U + random8(26U), 180U + random8(45U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
  }

  blurScreen(20); // без размытия как-то пиксельно, по-моему...

  dimAll(255U - (modes[currentMode].Scale << 1));  // * 2);

  for (uint8_t i = 1U; i < WIDTH; i += 3) {
    leds[XY(i, CENTER_Y_MINOR)] += CHSV(i << 1, 255, 255);  // CHSV(i * 2 , 255, 255);
  }

  // Noise
  noise32_x[0] += 3000U;
  noise32_y[0] += 3000U;
  noise32_z[0] += 3000U;
  scale32_x[0] = 8000U;
  scale32_y[0] = 8000U;

  FillNoise(0);
  MoveFractionalNoiseY(3);
  MoveFractionalNoiseX(3);
}
#endif


/*
static void MultipleStream4() { // Comet
  dimAll(255U - (modes[currentMode].Scale << 1));  // * 2);

  leds[XY(CENTER_X_MINOR, CENTER_Y_MINOR)] += CHSV(millis(), 255, 255);

  // Noise
  noise32_x[0] += 2000U;
  noise32_y[0] += 2000U;
  noise32_z[0] += 2000U;
  scale32_x[0] = 4000U;
  scale32_y[0] = 4000U;

  FillNoise(0);
  MoveFractionalNoiseX(6);
  MoveFractionalNoiseY(5, -0.5f);
}
*/


#ifdef DEF_FIREFLY_TOP
static void MultipleStream5() { // Fractorial Fire
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      setModeSettings(1U + random8(26U), 180U + random8(45U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
  }

  blurScreen(20); // без размытия как-то пиксельно, по-моему...

  dimAll(255U - (modes[currentMode].Scale << 1));  // * 2);

  for (uint8_t i = 1U; i < WIDTH; i += 2) {
    leds[XY(i, MAX_Y)] += CHSV(i << 1, 255, 255);  // CHSV(i * 2, 255, 255);
  }

  // Noise
  noise32_x[0] += 3000U;
  noise32_y[0] += 3000U;
  noise32_z[0] += 3000U;
  scale32_x[0] = 8000U;
  scale32_y[0] = 8000U;

  FillNoise(0);
  MoveFractionalNoiseY(2, 1);
  MoveFractionalNoiseX(2);
}
#endif


#ifdef DEF_SNAKE
static void MultipleStream8() { // Windows
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        setModeSettings(random8(2U) ? 1U : 2U + random8(99U), 155U + random8(76U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    if (modes[currentMode].Scale > 1U)
      hue = ((uint16_t)(modes[currentMode].Scale - 2U) * 26U) / 10U;  // * 2.6f;
    else
      hue = random8();

    loadingFlag = false;
  }

  if (modes[currentMode].Scale <= 1U)
    hue++;

  dimAll(96); // < -- затухание эффекта для последующего кадра на 96 / 255 * 100 = 37%

  for (uint8_t y = 2U; y < HEIGHT-1; y += 5) {
    uint8_t y_plus_4 = y + 4U;

    for (uint8_t x = 2U; x < WIDTH-1; x += 5) {
      uint8_t x_plus_4 = x + 4U;

      leds[XY(x, y)]          += CHSV(x * y + hue, 255, 255);
      leds[XY(x + 1, y)]      += CHSV(x_plus_4 * y + hue, 255, 255);
      leds[XY(x, y + 1)]      += CHSV(x * y_plus_4 + hue, 255, 255);
      leds[XY(x + 1, y + 1)]  += CHSV(x_plus_4 * y_plus_4 + hue, 255, 255);
    }
  }

  // Noise
  noise32_x[0] += 3000U;
  noise32_y[0] += 3000U;
  noise32_z[0] += 3000U;
  scale32_x[0] = 8000U;
  scale32_y[0] = 8000U;

  FillNoise(0);
  MoveFractionalNoiseX(3);
  MoveFractionalNoiseY(3);
}
#endif


#ifdef DEF_COMET
//  Follow the Rainbow Comet by Palpalych (Effect for GyverLamp 02/03/2020) //

// Кометы обычные
static void RainbowCometRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        setModeSettings(10U + random8(91U), 185U + random8(51U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
  }

  dimAll(254U); // < -- затухание эффекта для последующего кадра

  // CRGB _eNs_color = CHSV(millis() / modes[currentMode].Scale * 2, 255, 255);
  CRGB _eNs_color = CHSV((millis() << 1) / modes[currentMode].Scale, 255, 255);

  leds[XY(CENTER_X_MINOR,      CENTER_Y_MINOR)]      += _eNs_color;
  leds[XY(CENTER_X_MINOR + 1U, CENTER_Y_MINOR)]      += _eNs_color;
  leds[XY(CENTER_X_MINOR,      CENTER_Y_MINOR + 1U)] += _eNs_color;
  leds[XY(CENTER_X_MINOR + 1U, CENTER_Y_MINOR + 1U)] += _eNs_color;

  // Noise
  noise32_x[0] += 1500U;
  noise32_y[0] += 1500U;
  noise32_z[0] += 1500U;
  scale32_x[0] = 8000U;
  scale32_y[0] = 8000U;

  FillNoise(0);
  MoveFractionalNoiseX(CENTER_X - 1U);
  MoveFractionalNoiseY(CENTER_Y - 1U);
}
#endif


#ifdef DEF_COMET_COLOR
// Кометы белые и одноцветные
static void ColorCometRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(random8(20U) ? 1U + random8(99U) : 100U, 185U + random8(51U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
  }

  dimAll(254U); // < -- затухание эффекта для последующего кадра

  const uint8_t current_scale = modes[currentMode].Scale;
  CRGB _eNs_color = CRGB::White;

  if (current_scale < 100U) {
    _eNs_color = CHSV(((uint16_t)current_scale * 257U) / 100U, 255, 255); // * 2.57f вместо * 2.55f, потому что при 100 будет белый цвет
  }

  leds[XY(CENTER_X_MINOR,      CENTER_Y_MINOR)]      += _eNs_color;
  leds[XY(CENTER_X_MINOR + 1U, CENTER_Y_MINOR)]      += _eNs_color;
  leds[XY(CENTER_X_MINOR,      CENTER_Y_MINOR + 1U)] += _eNs_color;
  leds[XY(CENTER_X_MINOR + 1U, CENTER_Y_MINOR + 1U)] += _eNs_color;

  // Noise
  noise32_x[0] += 1500U;
  noise32_y[0] += 1500U;
  noise32_z[0] += 1500U;
  scale32_x[0] = 8000U;
  scale32_y[0] = 8000U;

  FillNoise(0);
  MoveFractionalNoiseX(CENTER_X - 1U);
  MoveFractionalNoiseY(CENTER_Y - 1U);
}
#endif


#ifdef DEF_BBALLS
// --------------------------- эффект мячики ----------------------
//  BouncingBalls2014 is a program that lets you animate an LED strip
//  to look like a group of bouncing balls
//  Daniel Wilson, 2014
//  https://github.com/githubcdr/Arduino/blob/master/bouncingballs/bouncingballs.ino
//  With BIG thanks to the FastLED community!
//  адаптация от SottNick

// #define bballsGRAVITY           (-9.81)                                     // Downward (negative) acceleration of gravity in m/s^2
// #define bballsH0                (1)                                         // Starting height, in meters, of the ball (strip length)
// #define enlargedOBJECT_MAX_COUNT            (WIDTH * 2)                     // максимальное количество мячиков прикручено при адаптации для бегунка Масштаб
// uint8_t enlargedObjectNUM;                                                  // Number of bouncing balls you want (recommend < 7, but 20 is fun in its own way) ...
//                                                                               количество мячиков теперь задаётся бегунком, а не константой
// uint8_t bballsCOLOR[enlargedOBJECT_MAX_COUNT] ;                             // прикручено при адаптации для разноцветных мячиков
// будем использовать uint8_t trackingObjectHue[trackingOBJECT_MAX_COUNT];
// uint8_t bballsX[enlargedOBJECT_MAX_COUNT] ;                                 // прикручено при адаптации для распределения мячиков по радиусу лампы
// будем использовать uint8_t trackingObjectState[trackingOBJECT_MAX_COUNT];
// bool trackingObjectIsShift[enlargedOBJECT_MAX_COUNT] ;                      // прикручено при адаптации для того, чтобы мячики не стояли на месте
// static float bballsVImpact0 = SQRT_VARIANT(-2 * bballsGRAVITY * bballsH0);  // Impact velocity of the ball when it hits the ground if "dropped" from the top of the strip
// float bballsVImpact[enlargedOBJECT_MAX_COUNT] ;                             // As time goes on the impact velocity will change, so make an array to store those values
// будем использовать float trackingObjectSpeedY[trackingOBJECT_MAX_COUNT];
// uint16_t   bballsPos[enlargedOBJECT_MAX_COUNT] ;                            // The integer position of the dot on the strip (LED index)
// будем использовать float trackingObjectPosY[trackingOBJECT_MAX_COUNT];
// long  enlargedObjectTime[enlargedOBJECT_MAX_COUNT] ;                        // The clock time of the last ground strike
// float bballsCOR[enlargedOBJECT_MAX_COUNT] ;                                 // Coefficient of Restitution (bounce damping)
// будем использовать float trackingObjectShift[trackingOBJECT_MAX_COUNT];

constexpr float bballsGRAVITY  = -9.81f;                                             // Downward (negative) acceleration of gravity in m/s^2
constexpr float bballsH0       = 1.0f;                                               // Starting height, in meters, of the ball (strip length)
constexpr float bballsVImpact0 = __builtin_sqrtf(-2.0f * bballsGRAVITY * bballsH0);  // Impact velocity of the ball when it hits the ground if "dropped" from the top of the strip

static void BBallsRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      setModeSettings(26U + random8(32U), random8(3U) ? ((random8(4U) ? 127U : 0U) + 9U + random8(12U)) : (random8(4U) ? 255U : 127U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    enlargedObjectNUM = ((uint16_t)(modes[currentMode].Scale - 1U) * (enlargedOBJECT_MAX_COUNT - 1U)) / 99U + 1U;
    if (enlargedObjectNUM > enlargedOBJECT_MAX_COUNT) {
      enlargedObjectNUM = enlargedOBJECT_MAX_COUNT;
    }

    uint16_t num_squared = (uint16_t)enlargedObjectNUM * enlargedObjectNUM;
    for (uint8_t i = 0 ; i < enlargedObjectNUM ; i++) {                                               // Initialize variables
      trackingObjectHue[i]     = random8();
      trackingObjectState[i]   = random8(0U, WIDTH);
      enlargedObjectTime[i]    = millis();
      trackingObjectPosY[i]    = 0U;                                                                  // Balls start on the ground
      trackingObjectSpeedY[i]  = bballsVImpact0;                                                      // And "pop" up at vImpact0
      trackingObjectShift[i]   = 0.90f - (float)i / (float)num_squared;;                              // это, видимо, прыгучесть. для каждого мячика уникальная изначально
      trackingObjectIsShift[i] = false;

      hue2 = (modes[currentMode].Speed > 127U) ? 255U : 0U;                                           // цветные или белые мячики
      hue = (modes[currentMode].Speed == 128U) ? 255U : 254U - modes[currentMode].Speed % 128U * 2U;  // скорость угасания хвостов 0 = моментально
    }

    loadingFlag = false;
  }

  float bballsHi;
  float bballsTCycle;

  if (deltaValue++ & 0x01) deltaHue++;                                                                // постепенное изменение оттенка мячиков (закомментировать строчку, если не нужно)

  dimAll(hue);

  constexpr float height_multiplier = MAX_Y / bballsH0;

  const uint32_t current_ms = millis();

  for (uint8_t i = 0 ; i < enlargedObjectNUM ; i++) {
    bballsTCycle = (float)(current_ms - enlargedObjectTime[i]) * inv1000;                              // Calculate the time since the last time the ball was on the ground

    // A little kinematics equation calculates positon as a function of time,
    // acceleration (gravity) and intial velocity
    bballsHi = 0.5f * bballsGRAVITY * bballsTCycle * bballsTCycle + trackingObjectSpeedY[i] * bballsTCycle;

    if (bballsHi < 0.0f) {
      enlargedObjectTime[i] = current_ms;
      bballsHi = 0.0f;                                                                                // If the ball crossed the threshold of the "ground," put it back on the ground
      trackingObjectSpeedY[i] = trackingObjectShift[i] * trackingObjectSpeedY[i] ;                    // and recalculate its new upward velocity as it's old velocity * COR

      if (trackingObjectSpeedY[i] < 0.01f) {                                                          // If the ball is barely moving, "pop" it back up at vImpact0
        uint8_t rand_base = random8(4U, 9U);
        uint16_t rand_squared = (uint16_t)rand_base * rand_base;

        trackingObjectShift[i] = 0.90f - (float)random8(9U) / (float)rand_squared;;                   // сделал, чтобы мячики меняли свою прыгучесть каждый цикл
        trackingObjectIsShift[i] = trackingObjectShift[i] >= 0.89f;                                   // если мячик максимальной прыгучести, то разрешаем ему сдвинуться
        trackingObjectSpeedY[i] = bballsVImpact0;
      }
    }

    int16_t calculated_pos = (int16_t)(bballsHi * height_multiplier + 0.5f);                          // Map "h" to a "pos" integer index position on the LED strip
    if (calculated_pos < 0) calculated_pos = 0;
    else if (calculated_pos >= HEIGHT) calculated_pos = MAX_Y;
    trackingObjectPosY[i] = calculated_pos;

    if (trackingObjectIsShift[i] && (trackingObjectPosY[i] == MAX_Y)) {                         // если мячик получил право, то пускай сдвинется на максимальной высоте 1 раз
      trackingObjectIsShift[i] = false;
      if (trackingObjectHue[i] & 0x01) {                                                              // нечётные налево, чётные направо
        if (trackingObjectState[i] == 0U) trackingObjectState[i] = MAX_X;
        else --trackingObjectState[i];
      } else {
        if (trackingObjectState[i] == MAX_X) trackingObjectState[i] = 0U;
        else ++trackingObjectState[i];
      }
    }

    leds[XY(trackingObjectState[i], trackingObjectPosY[i])] = CHSV(trackingObjectHue[i] + deltaHue, hue2, 255U);
  }
}
#endif


#ifdef DEF_SPIRO
// --------------------------- эффект спирали ----------------------
/*
 * Aurora: https://github.com/pixelmatix/aurora
 * https://github.com/pixelmatix/aurora/blob/sm3.0-64x64/PatternSpiro.h
 * Copyright (c) 2014 Jason Coon
 * Неполная адаптация SottNick
 * Оптимизация рантайма andrewjswan
 */

static uint8_t spirotheta1 = 0;
static uint8_t spirotheta2 = 0;
// uint8_t spirohueoffset = 0; // будем использовать переменную сдвига оттенка hue из эффектов Радуга

// Радиусы (четверть размера)
static constexpr uint8_t spiroradiusx = QUARTER_X;
static constexpr uint8_t spiroradiusy = QUARTER_Y;

// Используем ранее созданные центры
static constexpr uint8_t spirocenterX = CENTER_X;
static constexpr uint8_t spirocenterY = CENTER_Y;

// Минимальные границы
static constexpr uint8_t spirominx = spirocenterX - spiroradiusx;
static constexpr uint8_t spirominy = spirocenterY - spiroradiusy;

// Максимальные границы с учетом четности
// (WIDTH % 2 == 0) вернет 1 для четных и 0 для нечетных
static constexpr uint8_t spiromaxx = spirocenterX + spiroradiusx - (WIDTH % 2 == 0);
static constexpr uint8_t spiromaxy = spirocenterY + spiroradiusy - (HEIGHT % 2 == 0);

// Вычисляем константные диапазоны для базовых функций маппинга
static constexpr uint8_t spiro_range_x = spiromaxx - spirominx;
static constexpr uint8_t spiro_range_y = spiromaxy - spirominy;
static constexpr uint8_t spiro_range_radius_x = (spiroradiusx << 1); // spiroradiusx * 2
static constexpr uint8_t spiro_range_radius_y = (spiroradiusy << 1); // spiroradiusy * 2

static uint8_t spirocount = 1U;
static uint8_t spirooffset = 256U / spirocount;

static boolean spiroincrement = false;
static boolean spirohandledChange = false;

static inline uint8_t mapsin8_fast(uint8_t theta, uint8_t lowest, uint8_t range) {
  return lowest + scale8(sin8(theta), range);
}

static inline uint8_t mapcos8_fast(uint8_t theta, uint8_t lowest, uint8_t range) {
  return lowest + scale8(cos8(theta), range);
}

static void spiroRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      uint8_t rnd = random8(6U);
      if (rnd > 1U) rnd++;
      if (rnd > 3U) rnd++;
      setModeSettings(rnd * 11U + 3U, random8(10U) ? 2U + random8(26U) : 255U);
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette();

    loadingFlag = false;
  }

  blurScreen(20); // @Palpalych советует делать размытие

  dimAll(255U - modes[currentMode].Speed / 10U);

  boolean change = false;

  for (uint8_t i = 0U; i < spirocount; i++) {
    uint8_t current_offset = i * spirooffset;
    uint8_t theta1_calculated = spirotheta1 + current_offset;
    uint8_t theta2_calculated = spirotheta2 + current_offset;

    uint8_t x = mapsin8_fast(theta1_calculated, spirominx, spiro_range_x);
    uint8_t y = mapcos8_fast(theta1_calculated, spirominy, spiro_range_y);

    uint8_t x2 = mapsin8_fast(theta2_calculated, x - spiroradiusx, spiro_range_radius_x);
    uint8_t y2 = mapcos8_fast(theta2_calculated, y - spiroradiusy, spiro_range_radius_y);

    if (x2 < WIDTH && y2 < HEIGHT) {
      leds[XY(x2, y2)] += ColorFromPalette(*curPalette, hue + current_offset);
    }

    if (x2 == spirocenterX && y2 == spirocenterY) {
      change = true;
    }
  }

  spirotheta1 += 1U;
  spirotheta2 += 2U;

  EVERY_N_MILLIS(75) {
    if (change && !spirohandledChange) {
      spirohandledChange = true;

      if (spirocount >= WIDTH || spirocount == 1U) {
        spiroincrement = !spiroincrement;
      }

      if (spiroincrement) {
        if(spirocount >= 4U)
          spirocount <<= 1;  // spirocount *= 2;
        else
          spirocount += 1U;
      } else {
        if(spirocount > 4U)
          spirocount >>= 1;  // spirocount /= 2;
        else
          spirocount -= 1U;
      }

      spirooffset = 256U / spirocount;
    }

    if(!change) spirohandledChange = false;
  }

  hue += 1U;
}
#endif


#ifdef DEF_METABALLS
// --------------------------- эффект МетаБолз ----------------------
// https://gist.github.com/StefanPetrick/170fbf141390fafb9c0c76b8a0d34e54
// Stefan Petrick's MetaBalls Effect mod by PalPalych for GyverLamp
/*
  Metaballs proof of concept by Stefan Petrick (mod by Palpalych for GyverLamp 27/02/2020)
  ...very rough 8bit math here...
  read more about the concept of isosurfaces and metaballs:
  https://www.gamedev.net/articles/programming/graphics/exploring-metaballs-and-isosurfaces-in-2d-r2556
*/
static void MetaBallsRoutine() {
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      setModeSettings(random8(8U) * 11U + 1U + random8(11U), 50U + random8(121U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette();

    speedfactor = modes[currentMode].Speed / 127.0f;

    loadingFlag = false;
  }

  // get some 2 random moving points
  const uint16_t param1 = millis() * speedfactor;

  #if (WIDTH < 16) || (HEIGHT < 16)
  const uint8_t x2 = remap(fastled_helper::perlin8(param1, 25355, 685), 0, 255, 0, MAX_X);
  const uint8_t y2 = remap(fastled_helper::perlin8(param1, 355, 11685), 0, 255, 0, MAX_Y);

  const uint8_t x3 = remap(fastled_helper::perlin8(param1, 55355, 6685), 0, 255, 0, MAX_X);
  const uint8_t y3 = remap(fastled_helper::perlin8(param1, 25355, 22685), 0, 255, 0, MAX_Y);
  #else
  const uint8_t x2 = fastled_helper::perlin8(param1, 25355, 685) / WIDTH;
  const uint8_t y2 = fastled_helper::perlin8(param1, 355, 11685) / HEIGHT;

  const uint8_t x3 = fastled_helper::perlin8(param1, 55355, 6685) / WIDTH;
  const uint8_t y3 = fastled_helper::perlin8(param1, 25355, 22685) / HEIGHT;
  #endif

  // and one Lissajou function
  uint8_t x1 = beatsin8(23 * speedfactor, 0, MAX_X);
  uint8_t y1 = beatsin8(28 * speedfactor, 0, MAX_Y);

  const bool is_scale_100 = (modes[currentMode].Scale == 100U);

  for (uint8_t y = 0U; y < HEIGHT; y++) {
    #if (WIDTH < 16) && (HEIGHT < 16)
      uint8_t dy1 = std::abs(y - y1);
      uint8_t dy2 = std::abs(y - y2);
      uint8_t dy3 = std::abs(y - y3);
    #else
      uint16_t dy1_sq = (uint16_t)std::abs(y - y1) * std::abs(y - y1);
      uint16_t dy2_sq = (uint16_t)std::abs(y - y2) * std::abs(y - y2);
      uint16_t dy3_sq = (uint16_t)std::abs(y - y3) * std::abs(y - y3);
    #endif

    for (uint8_t x = 0U; x < WIDTH; x++) {
      // calculate distances of the 3 points from actual pixel
      // and add them together with weightening
      #if (WIDTH < 16) && (HEIGHT < 16)
        // Манхэттенская метрика с использованием предрассчитанных по Y значений
        uint8_t dist = 2 * (std::abs(x - x1) + dy1);
        dist += (std::abs(x - x2) + dy2);
        dist += (std::abs(x - x3) + dy3);
        // fast heuristic method for converting the distance calculated using the Manhattan metric to an approximate value of the Euclidean distance
        dist >>= 1;
      #else
        // Евклидова метрика: подставляем готовые dy_sq, экономя умножения
        uint16_t dx_sq = (uint16_t)std::abs(x - x1) * std::abs(x - x1);
        uint8_t dist = 2 * SQRT_VARIANT(dx_sq + dy1_sq);

        dx_sq = (uint16_t)std::abs(x - x2) * std::abs(x - x2);
        dist += SQRT_VARIANT(dx_sq + dy2_sq);

        dx_sq = (uint16_t)std::abs(x - x3) * std::abs(x - x3);
        dist += SQRT_VARIANT(dx_sq + dy3_sq);
      #endif

      // inverse result
      uint8_t color = (dist == 0) ? 255U : 1000U / dist;

      // map color between thresholds
      if (color > 0 && color < 60) {
        if (is_scale_100)
          drawPixelXY(x, y, CHSV(color * 9, 255, 255));  // это оригинальный цвет эффекта
        else
          drawPixelXY(x, y, ColorFromPalette(*curPalette, color * 9));
      } else {
        if (is_scale_100)
          drawPixelXY(x, y, CHSV(0, 255, 255));          // в оригинале центральный глаз почему-то красный
        else
          drawPixelXY(x, y, ColorFromPalette(*curPalette, 0U));
      }
    }
  }

  // show the 3 points, too
  drawPixelXY(x1, y1, CRGB(255, 255, 255));
  drawPixelXY(x2, y2, CRGB(255, 255, 255));
  drawPixelXY(x3, y3, CRGB(255, 255, 255));
}
#endif


#ifdef DEF_SINUSOID3
// ***** SINUSOID3 / СИНУСОИД3 ***** + попытка повторить все остальные версии

/*
  Sinusoid3 by Stefan Petrick (mod by Palpalych for GyverLamp 27/02/2020)
  read more about the concept: https://www.youtube.com/watch?v=mubH-w_gwdA
  https://gist.github.com/StefanPetrick/dc666c1b4851d5fb8139b73719b70149
*/

// v1.7.0 - Updating for GuverLamp v1.7 by PalPalych 12.03.2020
// 2nd upd by Stepko https://wokwi.com/arduino/projects/287675911209222664
// 3rd proper by SottNick
// Optimization by andrewjswan

static void Sinusoid3Routine()
{
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        uint8_t tmp = random8(100U);
        setModeSettings(tmp + 1U, 4U + random8(183U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    deltaValue = (modes[currentMode].Speed - 1U) % 9U;                          // количество режимов

    emitterX = CENTER_X_F;
    emitterY = CENTER_Y_F;

    speedfactor = 0.00145f * modes[currentMode].Speed + 0.015f;                 // speed of the movement along the Lissajous curves //const float speedfactor =

    loadingFlag = false;
  }

  const float e_s3_size = 3.0f * modes[currentMode].Scale / 100.0f + 2.0f;      // amplitude of the curves
  const uint32_t time_shift = millis() & 0xFFFFFF; // overflow protection

  const uint16_t _scale = (((modes[currentMode].Scale - 1U) % 9U) * 10U + 80U) << 7U; // = remap(scale, 1, 255, 0.1, 3);
  const uint16_t _scale3 = ((modes[currentMode].Scale - 1U) % 9U) * 1638U + 3276U;    // для спиралей на sin16
  const float _scale2 = (float)((modes[currentMode].Scale - 1U) % 9U) * inv5 + 0.4f;  // для спиралей на sinf

  const uint32_t phase_shift_raw = time_shift * speedfactor;

  const float time_speed_factor = (float)time_shift * speedfactor;
  const float case34_phase = time_speed_factor * 100.0f;
  const float case5_phaseB = time_speed_factor * inv200;
  const float case5_phaseR = time_speed_factor * 0.0055f;

  const float center1x = float(e_s3_size * sin16(speedfactor * 72.0874f * time_shift)) / 0x7FFF - emitterX;
  const float center1y = float(e_s3_size * cos16(speedfactor * 98.301f  * time_shift)) / 0x7FFF - emitterY;
  const float center2x = float(e_s3_size * sin16(speedfactor * 68.8107f * time_shift)) / 0x7FFF - emitterX;
  const float center2y = float(e_s3_size * cos16(speedfactor * 65.534f  * time_shift)) / 0x7FFF - emitterY;
  const float center3x = float(e_s3_size * sin16(speedfactor * 134.3447f * time_shift)) / 0x7FFF - emitterX;
  const float center3y = float(e_s3_size * cos16(speedfactor * 170.3884f * time_shift)) / 0x7FFF - emitterY;

  switch (deltaValue) {
    case 0:  // Sinusoid I
      for (uint8_t y = 0U; y < HEIGHT; y++) {
        float cy1 = (float)y + center1y; float cy1_sq = cy1 * cy1;
        float cy3 = (float)y + center3y; float cy3_sq = cy3 * cy3;

        for (uint8_t x = 0U; x < WIDTH; x++) {
          CRGB color = CRGB::Black;

          float cx = (float)x + center1x;
          color.r = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy1_sq))) / 32767.0f); // / 0x7FFF

          cx = (float)x + center3x;
          color.b = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy3_sq))) / 32767.0f);

          drawPixelXY(x, y, color);
        }
      }
      break;
    case 1:  // Sinusoid II
      for (uint8_t y = 0U; y < HEIGHT; y++) {
        float cy1 = (float)y + center1y; float cy1_sq = cy1 * cy1;
        float cy2 = (float)y + center2y; float cy2_sq = cy2 * cy2;

        for (uint8_t x = 0U; x < WIDTH; x++) {
          CRGB color = CRGB::Black;

          float cx = (float)x + center1x;
          color.r = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy1_sq))) / 32767.0f);

          cx = (float)x + center2x;
          uint8_t v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy2_sq))) / 32767.0f);
          color.g = (v - (min(v, color.r) >> 1)) >> 1;
          color.b = color.g >> 2;
          color.r = max(v, color.r);

          drawPixelXY(x, y, color);
        }
      }
      break;
    case 2:  // Sinusoid III
      for (uint8_t y = 0U; y < HEIGHT; y++) {
        float cy1 = (float)y + center1y; float cy1_sq = cy1 * cy1;
        float cy2 = (float)y + center2y; float cy2_sq = cy2 * cy2;
        float cy3 = (float)y + center3y; float cy3_sq = cy3 * cy3;

        for (uint8_t x = 0U; x < WIDTH; x++) {
          CRGB color = CRGB::Black;
          float cx = (float)x + center1x;
          color.r = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy1_sq))) / 32767.0f);

          cx = (float)x + center2x;
          color.b = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy2_sq))) / 32767.0f);

          cx = (float)x + center3x;
          color.g = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy3_sq))) / 32767.0f);

          drawPixelXY(x, y, color);
        }
      }
      break;
    case 3:  // Sinusoid IV
      for (uint8_t y = 0U; y < HEIGHT; y++) {
        float cy1 = (float)y + center1y; float cy1_sq = cy1 * cy1;
        float cy2 = (float)y + center2y; float cy2_sq = cy2 * cy2;
        float cy3 = (float)y + center3y; float cy3_sq = cy3 * cy3;

        for (uint8_t x = 0U; x < WIDTH; x++) {
          CRGB color = CRGB::Black;
          float cx = (float)x + center1x;
          uint8_t v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy1_sq) + case34_phase)) / 32767.0f);
          color.r = ~v;

          cx = (float)x + center2x;
          v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy2_sq) + case34_phase)) / 32767.0f);
          color.g = ~v;

          cx = (float)x + center3x;
          v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy3_sq) + case34_phase)) / 32767.0f);
          color.b = ~v;

          drawPixelXY(x, y, color);
        }
      }
      break;
    case 4:  // changed by stepko // colored sinusoid
      {
        float ampR = beatsin16(2, 1000, 1750) / 2550.0f;
        float ampB = beatsin16(1, 570, 1050) / 2250.0f;
        float ampG = beatsin16(3, 1900, 2550) / 2550.0f;

        float phaseB = 13.0f * time_speed_factor;
        float phaseG = 41.0f * time_speed_factor;

        for (uint8_t y = 0U; y < HEIGHT; y++) {
          float cy1 = (float)y + center1y; float cy1_sq = cy1 * cy1;

          for (uint8_t x = 0U; x < WIDTH; x++) {
            CRGB color = CRGB::Black;

            float cx = (float)x + center1x;
            float dist = SQRT_VARIANT(cx * cx + cy1_sq);

            color.r = 127 * (1 + float(sin16(_scale * ampR * dist)) / 32767.0f);
            color.g = 127 * (1 + float(cos16(_scale * ampG * dist + phaseG)) / 32767.0f);
            color.b = 127 * (1 + float(sin16(_scale * ampB * dist + phaseB)) / 32767.0f);

            drawPixelXY(x, y, color);
          }
        }
      }
      break;
    case 5:  // changed by stepko // sinusoid in net
      for (uint8_t y = 0U; y < HEIGHT; y++) {
        float cy1 = (float)y + center1y; float cy1_sq = cy1 * cy1;
        uint8_t v_r = ~uint8_t(127 * (1 + float(sin16(_scale * ((float)y + case5_phaseR))) / 32767.0f));

        for (uint8_t x = 0U; x < WIDTH; x++) {
          CRGB color = CRGB::Black;
          float cx = (float)x + center1x;
          uint8_t v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy1_sq) + time_speed_factor * 5.0f)) / 32767.0f);
          color.g = ~v;

          v = 127 * (1 + float(sin16(_scale * ((float)x + case5_phaseB))) / 32767.0f);
          color.b = ~v;
          color.r = v_r;

          drawPixelXY(x, y, color);
        }
      }
      break;
    case 6:  // changed by stepko // spiral
      for (uint8_t y = 0U; y < HEIGHT; y++) {
        float cy1 = (float)y + center1y; float cy1_sq = cy1 * cy1;
        float cy2 = (float)y + center2y; float cy2_sq = cy2 * cy2;
        float cy3 = (float)y + center3y; float cy3_sq = cy3 * cy3;

        for (uint8_t x = 0U; x < WIDTH; x++) {
          CRGB color = CRGB::Black;

          float cx = (float)x + center1x;
          uint8_t v = 127 * (1 + sinf(3 * atan2(cy1, cx) + _scale2 * hypot(cy1, cx)));
          float d = SQRT_VARIANT(cx * cx + cy1_sq) * inv10;
          if (d < 0.06f) d = 0.06f;
          if (d < 1.0f) v = constrain(v - int16_t(1.0f / (d * d)), 0, 255);
          color.r = v;

          cx = (float)x + center2x;
          v = 127 * (1 + sinf(3 * atan2(cy2, cx) + _scale2 * hypot(cy2, cx)));
          d = SQRT_VARIANT(cx * cx + cy2_sq) * inv10;
          if (d < 0.06f) d = 0.06f;
          if (d < 1.0f) v = constrain(v - int16_t(1.0f / (d * d)), 0, 255);
          color.b = v;

          cx = (float)x + center3x;
          v = 127 * (1 + float(sin16(atan2(cy3, cx) * 31255 + _scale3 * hypot(cy3, cx))) / 32767.0f);
          d = SQRT_VARIANT(cx * cx + cy3_sq) * inv10;
          if (d < 0.06f) d = 0.06f;
          if (d < 1.0f) v = constrain(v - int16_t(1.0f / (d * d)), 0, 255);
          color.g = v;

          drawPixelXY(x, y, color);
        }
      }
      break;
    case 7: // variant by SottNick
      for (uint8_t y = 0U; y < HEIGHT; y++) {
        float cy1 = (float)y + center1y; float cy1_sq = cy1 * cy1;
        float cy3 = (float)y + center3y; float cy3_sq = cy3 * cy3;

        for (uint8_t x = 0U; x < WIDTH; x++) {
          CRGB color = CRGB::Black;

          float cx = (float)x + center1x;
          uint8_t v = 127 * (1 + float(sin16(atan2(cy1, cx) * 31255 + _scale3 * hypot(cy1, cx))) / 32767.0f);
          float d = SQRT_VARIANT(cx * cx + cy1_sq) * inv10;
          if (d < 0.06f) d = 0.06f;
          if (d < 1.0f) v = constrain(v - int16_t(1.0f / (d * d)), 0, 255);
          color.g = v;

          cx = (float)x + center3x;
          v = 127 * (1 + float(sin16(atan2(cy3, cx) * 31255 + _scale3 * hypot(cy3, cx))) / 32767.0f);
          d = SQRT_VARIANT(cx * cx + cy3_sq) * inv10;
          if (d < 0.06f) d = 0.06f;
          if (d < 1.0f) v = constrain(v - int16_t(1.0f / (d * d)), 0, 255);
          color.r = v;

          drawPixelXY(x, y, color);
        }
      }
      break;
    case 8:  // variant by SottNick
      for (uint8_t y = 0U; y < HEIGHT; y++) {
        float cy1 = (float)y + center1y; float cy1_sq = cy1 * cy1;
        float cy2 = (float)y + center2y; float cy2_sq = cy2 * cy2;

        for (uint8_t x = 0U; x < WIDTH; x++) {
          CRGB color = CRGB::Black;

          float cx = (float)x + center1x;
          uint8_t v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy1_sq))) / 32767.0f);
          color.g = v;

          cx = (float)x + center2x;
          v = 127 * (1 + float(sin16(atan2(cy2, cx) * 31255 + _scale3 * hypot(cy2, cx))) / 32767.0f);
          float d = SQRT_VARIANT(cx * cx + cy2_sq) * inv16; // 1 / 16.0f
          if (d < 0.06f) d = 0.06f;
          if (d < 1.0f) v = constrain(v - int16_t(1.0f / (d * d)), 0, 255);
          color.g = max(v, color.g);
          color.b = v;

          drawPixelXY(x, y, color);
        }
      }
      break;
  }
}
#endif


#ifdef DEF_WATERFALL_4IN1
// ============= водо / огне / лава / радуга / хренопад ===============
// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.

static void fire2012WithPalette4in1() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      uint8_t tmp = random(3U);
      if (tmp == 0U)
        tmp = 16U + random8(16U);
      else if (tmp == 1U)
        tmp = 48U;
      else
        tmp = 80U + random8(4U);
      setModeSettings(tmp, 185U + random8(40U)); // 16-31, 48, 80-83 - остальное отстой
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
  }

  const uint8_t current_scale = modes[currentMode].Scale;
  const TProgmemRGBPalette16*  current_palette = &RainbowStripeColors_p;  // Aurora
  if (current_scale < 16)      current_palette = &LavaColors_p;           // Lavafall
  else if (current_scale < 32) current_palette = &HeatColors_p;           // Firefall
  else if (current_scale < 48) current_palette = &WaterfallColors4in1_p;  // Waterfall
  else if (current_scale < 64) current_palette = &CloudColors_p;          // Skyfall
  else if (current_scale < 80) current_palette = &ForestColors_p;         // Forestfall
  else if (current_scale < 96) current_palette = &RainbowColors_p;        // Rainbowfall

  uint8_t rCOOLINGNEW = constrain((uint16_t)(current_scale % 16) * 32 / HEIGHT + 16, 1, 255);

  // Array of temperature readings at each simulation cell
  // static uint8_t heat[WIDTH][HEIGHT]; будет noise3d[0][WIDTH][HEIGHT]

  for (uint8_t x = 0U; x < WIDTH; x++) {
    // Step 1.  Cool down every cell a little
    for (uint8_t i = 0U; i < HEIGHT; i++) {
      noise3d[0][x][i] = qsub8(noise3d[0][x][i], random8(0, rCOOLINGNEW));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (uint8_t k = MAX_Y; k >= 2; k--) {
      noise3d[0][x][k] = ((uint16_t)noise3d[0][x][k - 1] + noise3d[0][x][k - 2] + noise3d[0][x][k - 2]) / 3U;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if (random8() < SPARKINGNEW) {
      uint8_t y = random8(2U);
      noise3d[0][x][y] = qadd8(noise3d[0][x][y], random8(160U, 255U));
    }

    // Step 4.  Map from heat cells to LED colors
    for (uint8_t j = 0U; j < HEIGHT; j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      uint8_t colorindex = scale8(noise3d[0][x][j], 240U);
      leds[XY(x, MAX_Y - j)] = ColorFromPalette(*current_palette, colorindex);
    }
  }
}
#endif


#ifdef DEF_PRISMATA
// ============= ЭФФЕКТ ПРИЗМАТА ===============
// Prismata Loading Animation
// https://github.com/pixelmatix/aurora/blob/master/PatternPendulumWave.h
// Адаптация от (c) SottNick
static void PrismataRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        setModeSettings(1U + random8(100U), 35U + random8(100U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette();

    loadingFlag = false;
  }

  const uint8_t current_scale = modes[currentMode].Scale;
  const uint8_t current_speed = modes[currentMode].Speed;

  hue++; // используем переменную сдвига оттенка из функций радуги, чтобы не занимать память

  blurScreen(20); // @Palpalych посоветовал делать размытие
  dimAll(255U - ((current_scale - 1U) % 11U * 3U));

  const uint32_t time_base = (uint32_t)millis() * 28U * current_speed;

  for (uint8_t x = 0U; x < WIDTH; x++) {
    // вместо 28 в оригинале было 280, умножения на .Speed не было, а вместо >> 17 было (<< 8) >>24.
    // короче, оригинальная скорость достигается при бегунке .Speed = 20
    uint8_t beat = (uint8_t)((time_base * accum88(x + 1U)) >> 17);
    uint8_t y = scale8(sin8(beat), MAX_Y);

    drawPixelXY(x, y, ColorFromPalette(*curPalette, x * 7U + hue));
  }
}
#endif


template <class T>

class Vector2 {
public:
    T x, y;

    Vector2() :x(0), y(0) {}
    Vector2(T x, T y) : x(x), y(y) {}
    Vector2(const Vector2& v) : x(v.x), y(v.y) {}

    Vector2& operator=(const Vector2& v) {
        x = v.x;
        y = v.y;
        return *this;
    }

    bool isEmpty() {
        return x == 0 && y == 0;
    }

    bool operator==(const Vector2& v) {
        return x == v.x && y == v.y;
    }

    bool operator!=(const Vector2& v) {
        return !(*this == v);
    }

    Vector2 operator+(const Vector2& v) const {
        return Vector2(x + v.x, y + v.y);
    }

    Vector2 operator-(const Vector2& v) const {
        return Vector2(x - v.x, y - v.y);
    }

    Vector2& operator+=(const Vector2& v) {
        x += v.x;
        y += v.y;
        return *this;
    }

    Vector2& operator-=(const Vector2& v) {
        x -= v.x;
        y -= v.y;
        return *this;
    }

    Vector2 operator+(float s) const {
        return Vector2(x + s, y + s);
    }
    Vector2 operator-(float s) const {
        return Vector2(x - s, y - s);
    }
    Vector2 operator*(float s) const {
        return Vector2(x * s, y * s);
    }
    Vector2 operator/(float s) const {
        // return Vector2(x / s, y / s);
        float inv = 1.0f / s;
        return Vector2(x * inv, y * inv);
    }

    Vector2& operator+=(float s) {
        x += s;
        y += s;
        return *this;
    }
    Vector2& operator-=(float s) {
        x -= s;
        y -= s;
        return *this;
    }
    Vector2& operator*=(float s) {
        x *= s;
        y *= s;
        return *this;
    }
    Vector2& operator/=(float s) {
        float inv = 1.0f / s;
        x *= inv;
        y *= inv;
        return *this;
    }

    void set(T x, T y) {
        this->x = x;
        this->y = y;
    }

    void rotate(float deg) {
        float theta = deg / 180.0f * (float)M_PI;
        float c = std::cos(theta);
        float s = std::sin(theta);
        T tx = x * c - y * s;
        T ty = x * s + y * c;
        x = tx;
        y = ty;
    }

    Vector2& normalize() {
        float len = length();
        if (len == 0) return *this;
        *this *= (1.0f / len);
        return *this;
    }

    float dist(Vector2& v) const {
        Vector2 d(v.x - x, v.y - y);
        return d.length();
    }

    float length() const {
        return SQRT_VARIANT(x * x + y * y);
    }

    float mag() const {
        return length();
    }

    float magSq() {
        return (x * x + y * y);
    }

    void truncate(float length) {
        float angle = atan2f((float)y, (float)x);
        x = length * std::cos(angle);
        y = length * std::sin(angle);
    }

    Vector2 ortho() const {
        return Vector2(y, -x);
    }

    static float dot(const Vector2& v1, const Vector2& v2) {
        return v1.x * v2.x + v1.y * v2.y;
    }

    static float cross(const Vector2& v1, const Vector2& v2) {
        return (v1.x * v2.y) - (v1.y * v2.x);
    }

    void limit(float max) {
        if (magSq() > (max * max)) {
            normalize();
            *this *= max;
        }
    }
};

typedef Vector2<float> PVector;

// Boid class
// Methods for Separation, Cohesion, Alignment added

class Boid {
  public:

    PVector location;
    PVector velocity;
    PVector acceleration;

    float maxforce;    // Maximum steering force
    float maxspeed;    // Maximum speed

    float desiredseparation = 4;
    float neighbordist = 8;
    float mass;

    uint8_t colorIndex = 0;

    boolean enabled = true;

    Boid() {}

    Boid(float x, float y) {
      acceleration = PVector(0, 0);
      velocity = PVector(randomf(), randomf());
      location = PVector(x, y);
      maxspeed = 1.5f;
      maxforce = 0.05f;
    }

    static float randomf() {
      // return mapfloat(random(0, 255), 0, 255, -0.5f, 0.5f);
      // random8() дает 0..255.
      // Умножение на (1.0f / 255.0f) дает случайное число от 0.0f до 1.0f.
      // Вычитание 0.5f сдвигает диапазон в [-0.5f .. 0.5f]
      return ((float)random8() * (1.0f / 255.0f)) - 0.5f;
    }

    // static float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
    //  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    // }

    void run(Boid boids [], uint8_t boidCount) {
      flock(boids, boidCount);
      update();
      // wrapAroundBorders();
      // render();
    }

    // Method to update location
    void update() {
      // Update velocity
      velocity += acceleration;
      // Limit speed
      velocity.limit(maxspeed);
      location += velocity;
      // Reset acceleration to 0 each cycle
      acceleration *= 0;
    }

    void applyForce(PVector force) {
      // We could add mass here if we want A = F / M
      acceleration += force;
    }

    void repelForce(PVector obstacle, float radius) {
      // Force that drives boid away from obstacle.

      PVector futPos = location + velocity; // Calculate future position for more effective behavior.
      PVector dist = obstacle - futPos;

      // Быстрая проверка по квадрату расстояния без вызова тяжелого корня
      float dSq = dist.magSq();
      float radiusSq = radius * radius;

      if (dSq <= radiusSq) {
        // Вычисляем точную дистанцию только для тех, кто реально вошел в радиус
        float d = SQRT_VARIANT(dSq);

        PVector repelVec = location - obstacle;
        repelVec.normalize();

        // Сила расталкивания: чем ближе к препятствию, тем сильнее импульс
        float scale = (d > 0.01f) ? (1.0f / d) : 100.0f;

        // Масштабируем вектор расталкивания
        repelVec *= (maxforce * 7.0f * scale);

        // Если нужно обнулять вертикальную силу, чтобы boid не задирал нос (или не падал),
        // делаем это явно, если вектор направлен против основного движения
        // Оригинальное условие "repelVec.mag() < 0" заменено на логичную проверку знака Y
        if (repelVec.y < 0.0f) {
          repelVec.y = 0.0f;
        }

        applyForce(repelVec);
      }
    }

    // We accumulate a new acceleration each time based on three rules
    void flock(Boid boids [], uint8_t boidCount) {
      PVector sep = separate(boids, boidCount);   // Separation
      PVector ali = align(boids, boidCount);      // Alignment
      PVector coh = cohesion(boids, boidCount);   // Cohesion

      // Arbitrarily weight these forces
      sep *= 1.5f;
      // ali *= 1.0f;
      // coh *= 1.0f;

      // Add the force vectors to acceleration
      applyForce(sep);
      applyForce(ali);
      applyForce(coh);
    }

    // Separation
    // Method checks for nearby boids and steers away
    PVector separate(Boid boids [], uint8_t boidCount) {
      PVector steer = PVector(0, 0);
      uint8_t count = 0;

      float desiredSepSq = desiredseparation * desiredseparation;

      // For every boid in the system, check if it's too close
      for (uint8_t i = 0U; i < boidCount; i++) {
        const Boid& other = boids[i];
        if (!other.enabled)
          continue;

        PVector diff = location - other.location;
        float dSq = diff.magSq();

        // If the distance is greater than 0 and less than an arbitrary amount (0 when you are yourself)
        if ((dSq > 0.0f) && (dSq < desiredSepSq)) {
          diff /= dSq;  // Weight by distance

          steer += diff;
          count++;      // Keep track of how many
        }
      }

      // Average -- divide by how many
      if (count > 0) {
        steer /= (float) count;
      }

      // As long as the vector is greater than 0
      if (steer.magSq() > 0) {
        // Implement Reynolds: Steering = Desired - Velocity
        steer.normalize();
        steer *= maxspeed;
        steer -= velocity;
        steer.limit(maxforce);
      }
      return steer;
    }

    // Alignment
    // For every nearby boid in the system, calculate the average velocity
    PVector align(Boid boids [], uint8_t boidCount) {
      PVector sum = PVector(0, 0);
      uint8_t count = 0;

      float neighborDistSq = neighbordist * neighbordist;

      for (uint8_t i = 0U; i < boidCount; i++) {
        const Boid& other = boids[i];
        if (!other.enabled)
          continue;

        PVector diff = location - other.location;
        float dSq = diff.magSq();
        if ((dSq > 0.0f) && (dSq < neighborDistSq)) {
          sum += other.velocity;
          count++;
        }
      }

      if (count > 0) {
        sum /= static_cast<float>(count);
        sum.normalize();
        sum *= maxspeed;
        PVector steer = sum - velocity;
        steer.limit(maxforce);
        return steer;
      }
      else {
        return PVector(0.0f, 0.0f);
      }
    }

    // Cohesion
    // For the average location (i.e. center) of all nearby boids, calculate steering vector towards that location
    PVector cohesion(Boid boids [], uint8_t boidCount) {
      PVector sum = PVector(0.0f, 0.0f);   // Start with empty vector to accumulate all locations
      uint8_t count = 0;

      float neighborDistSq = neighbordist * neighbordist;

      for (uint8_t i = 0U; i < boidCount; i++) {
        const Boid& other = boids[i];
        if (!other.enabled)
          continue;

        PVector diff = location - other.location;
        float dSq = diff.magSq();

        if ((dSq > 0.0f) && (dSq < neighborDistSq)) {
          sum += other.location; // Add location
          count++;
        }
      }

      if (count > 0) {
        sum /= static_cast<float>(count);
        return seek(sum);  // Steer towards the location
      }
      else {
        return PVector(0.0f, 0.0f);
      }
    }

    // A method that calculates and applies a steering force towards a target
    // STEER = DESIRED MINUS VELOCITY
    PVector seek(const PVector& target) {
      PVector desired = target - location;  // A vector pointing from the location to the target

      if (desired.magSq() > 0.0f) {
        // Normalize desired and scale to maximum speed
        desired.normalize();
        desired *= maxspeed;

        // Steering = Desired minus Velocity
        PVector steer = desired - velocity;
        steer.limit(maxforce);  // Limit to maximum steering force
        return steer;
      }

      return PVector(0.0f, 0.0f);
    }

    // A method that calculates a steering force towards a target
    // STEER = DESIRED MINUS VELOCITY
    void arrive(const PVector& target) {
      PVector desired = target - location;  // A vector pointing from the location to the target

      float dSq = desired.magSq();
      // Защита от деления на ноль: если мы уже строго на месте, тормозим акселерацию
      if (dSq == 0.0f) {
        PVector steer = PVector(0.0f, 0.0f) - velocity;
        steer.limit(maxforce);
        applyForce(steer);
        return;
      }

      // Граница зоны торможения: 4 пикселя (в квадрате это 16.0f)
      if (dSq < 16.0f) {
        // Извлекаем корень ТОЛЬКО когда частица реально вошла в зону торможения
        float d = SQRT_VARIANT(dSq);

        // Normalize desired and scale with arbitrary damping within 100 pixels
        desired.normalize();

        // Быстрая float-замена функции map(d, 0, 100, 0, maxspeed)
        // Формула: d * (maxspeed / 100.0f). Множитель считается компилятором заранее!
        float m = d * (maxspeed * inv100);
        desired *= m;
      } else {
        // Если частица далеко, летим на полной скорости без извлечения корня
        desired.normalize();
        desired *= maxspeed;
      }

      // Steering = Desired minus Velocity
      PVector steer = desired - velocity;
      steer.limit(maxforce);  // Limit to maximum steering force
      applyForce(steer);
    }

    void wrapAroundBorders() {
      // Сохраняем дробную часть при перелете через левую/нижнюю границу
      if (location.x < 0.0f) {
        location.x += static_cast<float>(WIDTH);
      }
      if (location.y < 0.0f) {
        location.y += static_cast<float>(HEIGHT);
      }

      // Сохраняем дробную часть при перелете через правую/верхнюю границу
      if (location.x >= static_cast<float>(WIDTH)) {
        location.x -= static_cast<float>(WIDTH);
      }
      if (location.y >= static_cast<float>(HEIGHT)) {
        location.y -= static_cast<float>(HEIGHT);
      }
    }

    void avoidBorders() {
      PVector desired = velocity;
      bool changed = false; // Быстрый флаг вместо тяжелого сравнения векторов

      // Задаем ширину зоны предупреждения у стен (например, 2.0f пикселя)
      // Если WIDTH = 16, то 8 пикселей заставили бы boid паниковать прямо по центру экрана
      constexpr float padding = 2.0f;

      const float width_f  = static_cast<float>(WIDTH);
      const float height_f = static_cast<float>(HEIGHT);

      // --- Проверка левой и правой границ ---
      if (location.x < padding) {
        desired.x = maxspeed;
        changed = true;
      } else if (location.x >= (width_f - padding)) {
        desired.x = -maxspeed;
        changed = true;
      }

      // --- Проверка нижней и верхней границ ---
      if (location.y < padding) {
        desired.y = maxspeed;
        changed = true;
      } else if (location.y >= (height_f - padding)) {
        desired.y = -maxspeed;
        changed = true;
      }

      // Если траектория изменилась, плавно рулим в сторону от стены
      if (changed) {
        PVector steer = desired - velocity;
        steer.limit(maxforce);
        applyForce(steer);
      }

      // --- Жесткий Bounding Box (Защита от полного вылета) ---
      // Сохраняем дробную точность float: вместо сброса в WIDTH - 1 (15.0f),
      // останавливаем объект на честной границе (15.99f), чтобы он не дергался.
      if (location.x < 0.0f) {
        location.x = 0.0f;
      } else if (location.x >= width_f) {
        location.x = width_f - 0.01f;
      }

      if (location.y < 0.0f) {
        location.y = 0.0f;
      } else if (location.y >= height_f) {
        location.y = height_f - 0.01f;
      }
    }

    bool bounceOffBorders(float bounce) {
      bool bounced = false;

      const float width_f  = static_cast<float>(WIDTH);
      const float height_f = static_cast<float>(HEIGHT);

      // --- Отскок по оси X ---
      if (location.x >= width_f) {
        // Останавливаем строго на правой границе (15.99f для 16), сохраняя точность
        location.x = width_f - 0.01f;
        velocity.x *= -bounce;
        bounced = true;
      } else if (location.x < 0.0f) {
        location.x = 0.0f;
        velocity.x *= -bounce;
        bounced = true;
      }

      // --- Отскок по оси Y ---
      if (location.y >= height_f) {
        // Останавливаем строго на верхней границе, сохраняя точность
        location.y = height_f - 0.01f;
        velocity.y *= -bounce;
        bounced = true;
      } else if (location.y < 0.0f) {
        location.y = 0.0f;
        velocity.y *= -bounce;
        bounced = true;
      }

      return bounced;
    }

    void render() {
      // // Draw a triangle rotated in the direction of velocity
      // float theta = velocity.heading2D() + radians(90);
      // fill(175);
      // stroke(0);
      // pushMatrix();
      // translate(location.x,location.y);
      // rotate(theta);
      // beginShape(TRIANGLES);
      // vertex(0, -r*2);
      // vertex(-r, r*2);
      // vertex(r, r*2);
      // endShape();
      // popMatrix();
      // backgroundLayer.drawPixel(location.x, location.y, CRGB::Blue);
    }
};

constexpr uint8_t AVAILABLE_BOID_COUNT = 20U;
static Boid boids[AVAILABLE_BOID_COUNT];

#if defined(DEF_FLOCK) || defined(DEF_FLOCK_N_PR)
// ============= ЭФФЕКТ СТАЯ ===============
// https://github.com/pixelmatix/aurora/blob/master/PatternFlock.h
// Адаптация от (c) SottNick и @kDn
// Optimization by andrewjswan

// Flocking
// Daniel Shiffman <http://www.shiffman.net>
// The Nature of Code, Spring 2009

constexpr uint8_t boidCount = 10U;

static Boid predator;
static PVector wind;
static bool predatorPresent = true;

static void flockRoutine(bool predatorIs) {
  const uint8_t current_scale = modes[currentMode].Scale;
  const uint8_t current_speed = modes[currentMode].Speed;

  // Предрасчет коэффициента скорости для float-физики
  const float speed_factor = (float)current_speed / 127.0f;

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        //setModeSettings(random8(8U)*11U+1U + random8(11U), 1U + random8(255U));
        uint8_t tmp = random8(5U);  // 0, 1, 5, 6, 7 - остальные 4 палитры с чёрным цветом - стая будет исчезать периодически (2, 3, 4, 8)
        if (tmp > 1U) tmp += 3U;
        setModeSettings(tmp * 11U + 2U + random8(10U), 1U + random8(255U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette();

    for (int i = 0; i < boidCount; i++) {
      boids[i] = Boid(0.0f, 0.0f);  // WIDTH - 1U, HEIGHT - 1U);
      boids[i].maxspeed = 0.380f * speed_factor + 0.380f / 2.0f;
      boids[i].maxforce = 0.015f * speed_factor + 0.015f / 2.0f;
    }

    predatorPresent = predatorIs && random8(2U);
    predator = Boid(0, 0);          // WIDTH + WIDTH - 1, HEIGHT + HEIGHT - 1);
    predator.maxspeed = 0.385f * speed_factor + 0.385f / 2.0f;
    predator.maxforce = 0.020f * speed_factor + 0.020f / 2.0f;
    predator.neighbordist = 8.0f;   // было 16.0 и хищник гонял по одной линии всегда
    predator.desiredseparation = 0.0f;

    loadingFlag = false;
  }

  blurScreen(15); // @Palpalych советует делать размытие

  dimAll(255U - ((current_scale - 1U) % 11U * 3U));

  bool applyWind = random8() > 240U;
  if (applyWind) {
    wind.x = Boid::randomf() * 0.015f * speed_factor + 0.015f / 2.0f;
    wind.y = Boid::randomf() * 0.015f * speed_factor + 0.015f / 2.0f;
  }

  CRGB color = ColorFromPalette(*curPalette, hue);

  for (int i = 0; i < boidCount; i++) {
    Boid * boid = &boids[i];

    if (predatorPresent) {
      // flee from predator
      boid->repelForce(predator.location, 10);
    }

    boid->run(boids, boidCount);
    boid->wrapAroundBorders();
    PVector location = boid->location;
    drawPixelXYF(location.x, location.y, color);

    if (applyWind) {
      boid->applyForce(wind);
      applyWind = false;
    }
  }

  if (predatorPresent) {
    predator.run(boids, boidCount);
    predator.wrapAroundBorders();
    color = ColorFromPalette(*curPalette, hue + 128U);
    PVector location = predator.location;
    drawPixelXYF(location.x, location.y, color);
  }

  EVERY_N_MILLIS(333) {
    hue++;
  }

  EVERY_N_SECONDS(30) {
    predatorPresent = predatorIs && !predatorPresent;
  }
}
#endif


#if defined(DEF_WHIRL) || defined(DEF_WHIRL_MULTI)
// ============= ЭФФЕКТ ВИХРИ ===============
// https://github.com/pixelmatix/aurora/blob/master/PatternFlowField.h
// Адаптация (c) SottNick
// используются переменные эффекта Стая. Без него работать не будет.

// uint16_t ff_x; вынесены в общий пул
// uint16_t ff_y;
// uint16_t ff_z;

constexpr uint8_t ff_speed = 1U;  // чем выше этот параметр, тем короче переходы (градиенты) между цветами. 1 - это самое красивое
constexpr uint8_t ff_scale = 26U; // чем больше этот параметр, тем больше "языков пламени" или как-то так. 26 - это норм

static void whirlRoutine(bool oneColor) {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        if (oneColor)
          setModeSettings(random8(30U) ? 1U + random8(99U) : 100U, 221U + random8(32U));
        else{
          uint8_t tmp = random8(5U);
          if (tmp > 1U) tmp += 3U;
          setModeSettings(tmp * 11U + 3U, 221U + random8(32U));
        }
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette();

    ff_x = random16();
    ff_y = random16();
    ff_z = random16();

    for (uint8_t i = 0U; i < AVAILABLE_BOID_COUNT; i++) {
      boids[i] = Boid(random8(WIDTH), 0);
    }

    loadingFlag = false;
  }

  dimAll(240);

  const uint8_t current_scale = modes[currentMode].Scale;

  CRGB single_color;
  if (oneColor) {
    uint8_t calculated_hue = (current_scale == 100U) ? 0U : (uint8_t)((uint16_t)current_scale * 255U / 100U);  // цвет белый для .Scale = 100
    uint8_t calculated_sat = (current_scale == 100U) ? 0U : 255U;
    single_color = CHSV(calculated_hue, calculated_sat, 255U);
  }

  for (uint8_t i = 0U; i < AVAILABLE_BOID_COUNT; i++) {
    Boid * boid = &boids[i];

    int ioffset = ff_scale * boid->location.x;
    int joffset = ff_scale * boid->location.y;

    uint8_t angle = fastled_helper::perlin8(ff_x + ioffset, ff_y + joffset, ff_z);

    boid->velocity.x =   (float)sin8(angle) * inv128 - 1.0f;
    boid->velocity.y = -((float)cos8(angle) * inv128 - 1.0f);
    boid->update();

    if (oneColor) {
      drawPixelXYF(boid->location.x, boid->location.y, single_color);
    } else {
      drawPixelXYF(boid->location.x, boid->location.y, ColorFromPalette(*curPalette, angle + hue));  // + hue постепенно сдвигает палитру по кругу
    }

    if (boid->location.x < 0.0f || boid->location.x >= (float)WIDTH || boid->location.y < 0.0f || boid->location.y >= (float)HEIGHT) {
      boid->location.x = random8(WIDTH);
      boid->location.y = 0.0f;
    }
  }

  EVERY_N_MILLIS(200) {
    hue++;
  }

  ff_x += ff_speed;
  ff_y += ff_speed;
  ff_z += ff_speed;
}
#endif


#ifdef DEF_WAVES
// ============= ЭФФЕКТ ВОЛНЫ ===============
// https://github.com/pixelmatix/aurora/blob/master/PatternWave.h
// Адаптация от (c) SottNick

static uint8_t waveThetaUpdate = 0;
static uint8_t waveThetaUpdateFrequency = 0;
static uint8_t waveTheta = 0;

static uint8_t hueUpdate = 0;
static uint8_t hueUpdateFrequency = 0;
// uint8_t hue = 0; будем использовать сдвиг от эффектов Радуга

static uint8_t waveRotation = 0;
static uint8_t waveScale = 256 / WIDTH;
static uint8_t waveCount = 1;

static void WaveRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        uint8_t tmp = random8(5U);  // 0, 1, 5, 6, 7 - остальные 4 палитры с чёрным цветом - будет мерцать (2, 3, 4, 8)
        if (tmp > 1U) tmp += 3U;
        setModeSettings(tmp * 11U + 1U + random8(4U), 220U + random8(17U)*2U);
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette(); //а вот тут явно накосячено. палитры наложены на угол поворота несинхронно, но исправлять особого смысла нет

    waveRotation = (modes[currentMode].Scale % 11U) % 4U;  // (modes[currentMode].Scale - 1) / 25U;
    waveCount = modes[currentMode].Speed & 0x01;           // % 2;

    loadingFlag = false;
  }

  dimAll(254);

  int n = 0;

  switch (waveRotation) {
    case 0:
      for (uint8_t x = 0U; x < WIDTH; x++) {
        n = quadwave8((x << 1) + waveTheta) / waveScale;  // n = quadwave8(x * 2 + waveTheta) / waveScale;
        drawPixelXY(x, n, ColorFromPalette(*curPalette, hue + x));
        if (waveCount != 1) drawPixelXY(x, MAX_Y - n, ColorFromPalette(*curPalette, hue + x));
      }
      break;

    case 1:
      for (uint8_t y = 0U; y < HEIGHT; y++) {
        n = quadwave8((y << 1) + waveTheta) / waveScale;  // n = quadwave8(y * 2 + waveTheta) / waveScale;
        drawPixelXY(n, y, ColorFromPalette(*curPalette, hue + y));
        if (waveCount != 1) drawPixelXY(MAX_X - n, y, ColorFromPalette(*curPalette, hue + y));
      }
      break;

    case 2:
      for (uint8_t x = 0U; x < WIDTH; x++) {
        n = quadwave8((x << 1) - waveTheta) / waveScale;  // n = quadwave8(x * 2 - waveTheta) / waveScale;
        drawPixelXY(x, n, ColorFromPalette(*curPalette, hue + x));
        if (waveCount != 1) drawPixelXY(x, MAX_Y - n, ColorFromPalette(*curPalette, hue + x));
      }
      break;

    case 3:
      for (uint8_t y = 0U; y < HEIGHT; y++) {
        n = quadwave8((y << 1) - waveTheta) / waveScale;  // n = quadwave8(y * 2 - waveTheta) / waveScale;
        drawPixelXY(n, y, ColorFromPalette(*curPalette, hue + y));
        if (waveCount != 1) drawPixelXY(MAX_X - n, y, ColorFromPalette(*curPalette, hue + y));
      }
      break;
  }

  if (waveThetaUpdate >= waveThetaUpdateFrequency) {
    waveThetaUpdate = 0;
    waveTheta++;
  } else {
    waveThetaUpdate++;
  }

  if (hueUpdate >= hueUpdateFrequency) {
    hueUpdate = 0;
    hue++;
  } else {
    hueUpdate++;
  }

  blurScreen(20);  // @Palpalych советует делать размытие. вот в этом эффекте его явно не хватает...
}
#endif


#ifdef DEF_FIRE_2018
// ============= ЭФФЕКТ ОГОНЬ 2018 ===============
// https://gist.github.com/StefanPetrick/819e873492f344ebebac5bcd2fdd8aa8
// https://gist.github.com/StefanPetrick/1ba4584e534ba99ca259c1103754e4c5
// Адаптация от (c) SottNick
// Optimization by andrewjswan

// parameters and buffer for the noise array
// (вместо закомментированных строк используются массивы и переменные от эффекта Кометы для экономии памяти)
// define NUM_LAYERS 2 // менять бесполезно, так как в коде чётко использовано 2 слоя
// uint8_t noise3d[NUM_LAYERSMAX][WIDTH][HEIGHT];
// uint8_t fire18heat[NUM_LEDS]; будем использовать вместо него ledsbuff[NUM_LEDS].r
// this finds the right index within a serpentine matrix

static void Fire2018_2() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(1U + random8(50U), 195U + random8(44U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
  }

  const uint32_t current_ms = millis();
  const uint8_t current_scale = modes[currentMode].Scale;

  // some changing values
  uint16_t ctrl1 = fastled_helper::perlin16(11U * current_ms, 0, 0);
  uint16_t ctrl2 = fastled_helper::perlin16(13U * current_ms, 100000, 100000);
  uint16_t  ctrl = ((ctrl1 + ctrl2) >> 1);  // / 2

  // ================= LAYER 0 =================
  // parameters for the heatmap
  uint16_t speed = 25U;

  uint32_t base_noise_x = 3U * ctrl * speed;
  uint32_t base_noise_y = 20U * current_ms * speed;
  uint32_t base_noise_z = 5U * current_ms * speed;
  uint32_t scale_x      = ctrl1 >> 1;  // / 2
  uint32_t scale_y      = ctrl2 >> 1;  // / 2

  // calculate the noise data
  for (uint8_t i = 0U; i < WIDTH; i++) {
    uint32_t ioffset = scale_x * (i - CENTER_X_MAJOR);
    uint32_t current_x_noise = base_noise_x + ioffset;

    for (uint8_t j = 0U; j < HEIGHT; j++) {
      uint32_t joffset = scale_y * (j - CENTER_Y_MAJOR);
      uint16_t data = ((fastled_helper::perlin16(current_x_noise, base_noise_y + joffset, base_noise_z)) + 1U);
      noise3d[0][i][j] = (uint8_t)(data >> 8);
    }
  }

  // ================= LAYER 1 =================
  // parameters for te brightness mask
  speed = 20U;

  base_noise_x = 3U * ctrl * speed;
  base_noise_y = 20U * current_ms * speed;
  base_noise_z = 5U * current_ms * speed;
  scale_x      = ctrl1 >> 1;  // / 2;
  scale_y      = ctrl2 >> 1;  // / 2;

  // calculate the noise data
  for (uint8_t i = 0U; i < WIDTH; i++) {
    uint32_t ioffset = scale_x * (i - CENTER_X_MAJOR);
    uint32_t current_x_noise = base_noise_x + ioffset;

    for (uint8_t j = 0U; j < HEIGHT; j++) {
      uint32_t joffset = scale_y * (j - CENTER_Y_MAJOR);
      uint16_t data = ((fastled_helper::perlin16(current_x_noise, base_noise_y + joffset, base_noise_z)) + 1U);
      noise3d[1][i][j] = (uint8_t)(data >> 8);
    }
  }

  // draw lowest line - seed the fire
  const uint8_t lookup_y = CENTER_Y_MAJOR - 1U;  // хз, почему взято с середины. вожможно, нужно просто с 7 строки вне зависимости от высоты матрицы

  for (uint8_t x = 0U; x < WIDTH; x++) {
    ledsbuff[XY(x, MAX_Y)].r = noise3d[0][MAX_X - x][lookup_y];
  }

  // copy everything one line up
  for (uint8_t y = 0U; y < MAX_Y; y++) {
    for (uint8_t x = 0U; x < WIDTH; x++) {
      ledsbuff[XY(x, y)].r = ledsbuff[XY(x, y + 1U)].r;
    }
  }

  // dim
  for (uint8_t y = 0U; y < MAX_Y; y++) {
    for (uint8_t x = 0U; x < WIDTH; x++) {
      uint8_t dim = noise3d[0][x][y];

      // high value = high flames
      dim = ((uint16_t)dim * 150U) >> 8;  // dim / 1.7 : Умножаем на 150 и сдвигаем на 8 (аналог умножения на 0.585)
      dim = 255U - dim;

      uint16_t idx = XY(x, y);
      ledsbuff[idx].r = scale8(ledsbuff[idx].r, dim);
    }
  }

  for (uint8_t y = 0U; y < HEIGHT; y++) {
    uint8_t target_y = MAX_Y - y;

    for (uint8_t x = 0U; x < WIDTH; x++) {
      uint16_t idx = XY(x, y);
      uint8_t r_channel = ledsbuff[idx].r;
      uint8_t g_channel = ((uint16_t)r_channel * current_scale) / 100U;

      // map the colors based on heatmap
      uint16_t led_idx = XY(x, target_y);
      leds[led_idx] = CRGB(r_channel, g_channel, 0U);

      // dim the result based on 2nd noise layer
      leds[led_idx].nscale8(noise3d[1][x][y]);
    }
  }
}
#endif


#ifdef DEF_FIRE_2012
// ============= ЭФФЕКТ ОГОНЬ 2012 ===============
// там выше есть его копии для эффектов Водопад и Водопад 4 в 1
// по идее, надо бы объединить и оптимизировать, но мелких отличий довольно много
// based on FastLED example Fire2012WithPalette: https://github.com/FastLED/FastLED/blob/master/examples/Fire2012WithPalette/Fire2012WithPalette.ino

// База огня: максимум 6, но не меньше чем HEIGHT / 6 + 1
constexpr uint8_t FIRE_BASE = (HEIGHT / 6 > 6) ? 6U : (HEIGHT / 6 + 1U);
// Константы поведения огня
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
constexpr uint8_t cooling = 70U;
// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
constexpr uint8_t sparking = 130U;
// SMOOTHING; How much blending should be done between frames
// Lower = more blending and smoother flames. Higher = less blending and flickery flames
constexpr uint8_t fireSmoothing = 80U;

static void fire2012again()
{
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        uint8_t tmp = 17U + random8(55U);
        if (tmp>22) tmp += 28;
        setModeSettings(tmp, 185U + random8(50U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    if (modes[currentMode].Scale > 100U) modes[currentMode].Scale = 100U;
    if (modes[currentMode].Scale > 50U) {
      curPalette = firePalettes[(uint8_t)((modes[currentMode].Scale - 50U) / 50.0f * ((sizeof(firePalettes) / sizeof(TProgmemRGBPalette16 *)) - 0.01f))];
    } else {
      curPalette = palette_arr[(uint8_t)(modes[currentMode].Scale / 50.0f * ((sizeof(palette_arr) / sizeof(TProgmemRGBPalette16 *)) - 0.01f))];
    }

    loadingFlag = false;
  }

  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random8());

  const uint8_t cooling_limit = ((cooling * 10U) / HEIGHT) + 2U;

  // Loop for each column individually
  for (uint8_t x = 0U; x < WIDTH; x++) {
    // Step 1.  Cool down every cell a little
    for (uint8_t i = 0U; i < HEIGHT; i++) {
      noise3d[0][x][i] = qsub8(noise3d[0][x][i], random8(0, cooling_limit));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (uint8_t k = MAX_Y; k > 0; k--) { // fixed by SottNick
      noise3d[0][x][k] = ((uint16_t)noise3d[0][x][k - 1] + noise3d[0][x][k - 1] + noise3d[0][x][wrapY(k - 2)]) / 3U;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if (random8() < sparking) {
      uint8_t j = random8(FIRE_BASE);
      noise3d[0][x][j] = qadd8(noise3d[0][x][j], random8(160, 255));
    }

    // Step 4.  Map from heat cells to LED colors
    // Blend new data with previous frame. Average data between neighbouring pixels
    uint8_t next_x = wrapX(x + 1U);
    for (uint8_t y = 0U; y < HEIGHT; y++) {
      uint8_t blended_heat = scale8(noise3d[0][x][y], 179U) + scale8(noise3d[0][next_x][y], 76U);  // ((noise3d[0][x][y] * 0.7f) + (noise3d[0][next_x][y] * 0.3f))
      nblend(leds[XY(x, y)], ColorFromPalette(*curPalette, blended_heat), fireSmoothing);
    }
  }
}
#endif


#if defined(DEF_SIMPLE_RAIN) || defined(DEF_STORMY_RAIN) || defined(DEF_COLOR_RAIN)
// ============= ЭФФЕКТЫ ОСАДКИ / ТУЧКА В БАНКЕ / ГРОЗА В БАНКЕ ===============
// https://github.com/marcmerlin/FastLED_NeoMatrix_SmartMatrix_LEDMatrix_GFX_Demos/blob/master/FastLED/Sublime_Demos/Sublime_Demos.ino
// там по ссылке ещё остались эффекты с 3 по 9 (в SimplePatternList перечислены)

// прикольная процедура добавляет блеск почти к любому эффекту после его отрисовки https://www.youtube.com/watch?v=aobtR1gIyIo
// void addGlitter(uint8_t chanceOfGlitter){
//   if (random8() < chanceOfGlitter) leds[ random16(NUM_LEDS) ] += CRGB::White;
// }

// static uint8_t intensity = 42;  // будет бегунок масштаба

// Array of temp cells (used by fire, theMatrix, coloredRain, stormyRain)
// uint8_t **tempMatrix; = noise3d[0][WIDTH][HEIGHT]
// uint8_t *splashArray; = line[WIDTH] из эффекта Огонь

static CRGB solidRainColor = CRGB(60,80,90);

static void rain(uint8_t backgroundDepth, uint8_t maxBrightness, uint8_t spawnFreq, uint8_t tailLength, CRGB rainColor, bool splashes, bool clouds, bool storm) {
  ff_x = random16();
  ff_y = random16();
  ff_z = random16();

  CRGB lightningColor = CRGB(72, 72, 80);
  CRGBPalette16 rain_p(CRGB::Black, rainColor);

#ifdef SMARTMATRIX
  CRGBPalette16 rainClouds_p(CRGB::Black, CRGB(75, 84, 84), CRGB(49, 75, 75), CRGB::Black);
#else
  CRGBPalette16 rainClouds_p(CRGB::Black, CRGB(15, 24, 24), CRGB(9, 15, 15), CRGB::Black);
#endif

  dimAll(tailLength);

  // ledsbuff[].r - Канал молнии
  // ledsbuff[].b - Канал облаков
  memset(ledsbuff, 0, sizeof(ledsbuff));

  // Loop for each column individually
  for (uint8_t x = 0U; x < WIDTH; x++) {
    // Step 1.  Move each dot down one cell
    for (uint8_t i = 0U; i < HEIGHT; i++) {
      if (noise3d[0][x][i] >= backgroundDepth) {  // Don't move empty cells
        if (i > 0) {
          noise3d[0][x][wrapY(i-1)] = noise3d[0][x][i];
        }
        noise3d[0][x][i] = 0;
      }
    }

    // Step 2.  Randomly spawn new dots at top
    if (random8() < spawnFreq) {
      noise3d[0][x][MAX_Y] = random8(backgroundDepth, maxBrightness);
    }

    // Step 3. Map from tempMatrix cells to LED colors
    for (uint8_t y = 0U; y < HEIGHT; y++) {
      if (noise3d[0][x][y] >= backgroundDepth) {  // Don't write out empty cells
        leds[XY(x, y)] = ColorFromPalette(rain_p, noise3d[0][x][y]);
      }
    }

    // Step 4. Add splash if called for
    if (splashes) {
      // FIXME, this is broken
      uint8_t j = line[x];
      uint8_t v = noise3d[0][x][0];

      if (j >= backgroundDepth) {
        leds[XY(wrapX(x - 2), 0)] = ColorFromPalette(rain_p, j / 3U);
        leds[XY(wrapX(x + 2), 0)] = ColorFromPalette(rain_p, j / 3U);
        line[x] = 0;   // Reset splash
      }

      if (v >= backgroundDepth) {
        leds[XY(wrapX(x - 1), 1)] = ColorFromPalette(rain_p, v >> 1);  // / 2
        leds[XY(wrapX(x + 1), 1)] = ColorFromPalette(rain_p, v >> 1);  // / 2
        line[x] = v; // Prep splash for next frame
      }
    }

    // Step 5. Add lightning if called for
    if (storm) {
      if (random16() < 72U) {    // Odds of a lightning bolt
        constexpr uint16_t lightning_top_offset = (uint16_t)MAX_Y * WIDTH;

        uint16_t start_idx = scale8(random8(), MAX_X) + lightning_top_offset;
        ledsbuff[start_idx].r = 255U; // Random starting location

        for(uint8_t ly = MAX_Y; ly > 1; ly--) {
          for (uint8_t lx = 1; lx < MAX_X; lx++) {
            if (ledsbuff[lx + ly * WIDTH].r == 255U) {
              ledsbuff[lx + ly * WIDTH].r = 0U;

              uint8_t dir = random8(4U);
              switch (dir) {
                case 0:
                  leds[XY(lx + 1U, ly - 1U)] = lightningColor;
                  ledsbuff[(lx + 1U) + (ly - 1U) * WIDTH].r = 255U;  // move down and right
                  break;
                case 1:
                  leds[XY(lx, ly - 1U)] = CRGB(128, 128, 128);       // я без понятия, почему у верхней молнии один оттенок, а у остальных - другой
                  ledsbuff[lx + (ly - 1U) * WIDTH].r = 255U;         // move down
                  break;
                case 2:
                  leds[XY(lx - 1U, ly - 1U)] = CRGB(128, 128, 128);
                  ledsbuff[(lx - 1U) + (ly - 1U) * WIDTH].r = 255U;  // move down and left
                  break;
                case 3:
                  leds[XY(lx - 1U, ly - 1U)] = CRGB(128, 128, 128);
                  ledsbuff[(lx - 1U) + (ly - 1U) * WIDTH].r = 255U;  // fork down and left
                  leds[XY(lx - 1U, ly - 1U)] = CRGB(128, 128, 128);
                  ledsbuff[(lx + 1U) + (ly - 1U) * WIDTH].r = 255U;  // fork down and right
                  break;
              }
            }
          }
        }
      }
    }

    // Step 6. Add clouds if called for
    if (clouds) {
      constexpr uint16_t noiseScale = 250U;                                     // A value of 1 will be so zoomed in, you'll mostly see solid colors.
                                                                                // A value of 4011 will be very zoomed out and shimmery
      constexpr uint8_t cloudHeight = ((HEIGHT * 4U) / 10U) + 1U;               // это уже 40% c лишеним, но на высоких матрицах будет чуть меньше
      int xoffset = noiseScale * x + hue;

      for(uint8_t z = 0U; z < cloudHeight; z++) {
        int yoffset = noiseScale * z - hue;
        constexpr uint8_t dataSmoothing = 192;

        uint8_t noiseData = qsub8(fastled_helper::perlin8(ff_x + xoffset, ff_y + yoffset, ff_z), 16U);
        noiseData = qadd8(noiseData, scale8(noiseData, 39U));

        uint16_t buf_idx = (uint16_t)x * cloudHeight + z;
        ledsbuff[buf_idx].b = scale8(ledsbuff[buf_idx].b, dataSmoothing) + scale8(noiseData, 256U - dataSmoothing);
        nblend(leds[XY(x, MAX_Y - z)], ColorFromPalette(rainClouds_p, ledsbuff[buf_idx].b), (cloudHeight - z) * (250U / cloudHeight));
      }
      ff_z ++;
    }
  }
}

// Даёт масштабировать каждые 8 градаций (от 0 до 7) бегунка Масштаб в значения от 0 до 255 по типа синусоиде
static uint8_t myScale8(uint8_t x) {
  uint8_t x8 = x & 0x07U;  // то же самое, что x % 8U
  uint8_t x4 = x8 & 0x03U; // то же самое, что x8 % 4U

  if (x4 == 0U) {
    return (x8 == 0U) ? 0U : 255U; // Линейный быстрый тернарный оператор
  }
  if (x8 < 4U) {
    return (1U + x4 * 72U);        // Восхождение по синусоиде (всего 7шт по 36U + 3U лишних = 255U)
  }
  return (253U - x4 * 72U);        // Спуск по синусоиде (253U = 255U - 2U)
}

static void coloredRain() // внимание! этот эффект заточен на работу бегунка Масштаб в диапазоне от 0 до 255. пока что единственный.
{
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      uint8_t tmp = 1U + random8(255U);
      if ((tmp % 4U == 0U) && (tmp % 8U != 0U)) tmp--;
      setModeSettings(tmp, 165U + random8(76U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
  }

  const uint8_t spawn_frequency = map8(42U, 5U, 100U);
  const uint8_t current_scale = modes[currentMode].Scale;
  uint8_t tail_length = myScale8(current_scale);

  // я хз, как прикрутить а 1 регулятор и длину хвостов и цвет капель
  // ( Depth of dots, maximum brightness, frequency of new dots, length of tails, color, splashes, clouds, ligthening )
  if (current_scale > 247U) {
    rain(60U, 200U, spawn_frequency, tail_length, solidRainColor, false, false, false);
  } else {
    rain(60U, 200U, spawn_frequency, tail_length, CHSV(current_scale, 255U, 255U), false, false, false);
  }
}

static void simpleRain()
{
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      setModeSettings(random8(2U) ? 2U + random8(7U) : 9U + random8(70U), 220U + random8(22U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
  }

  // ( Depth of dots, maximum brightness, frequency of new dots, length of tails, color, splashes, clouds, ligthening )
  rain(60U, 180U, ((uint16_t)(modes[currentMode].Scale - 1U) * 258U) / 100U, 30U, solidRainColor, true, true, false);  //  2.58f
}

static void stormyRain()
{
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      setModeSettings(random8(2U) ? 2U + random8(15U) : 17U + random8(64U), 220U + random8(22U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
  }

  // ( Depth of dots, maximum brightness, frequency of new dots, length of tails, color, splashes, clouds, ligthening )
  rain(60U, 160U, ((uint16_t)(modes[currentMode].Scale - 1U) * 258U) / 100U, 30U, solidRainColor, true, true, true);  //  2.58f
}
#endif


#ifdef DEF_TWINKLES
// ------------------------------ ЭФФЕКТ МЕРЦАНИЕ ----------------------
// (c) SottNick

#define TWINKLES_SPEEDS (4U)     // всего 4 варианта скоростей мерцания
#define TWINKLES_MULTIPLIER (6U) // слишком медленно, если на самой медленной просто по единичке к яркости добавлять

static void twinklesRoutine()
{
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(random8(8U) * 11U + 2U + random8(9U), 180U + random8(69U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette();

    hue = 0U;
    deltaValue = (modes[currentMode].Scale - 1U) % 11U + 1U;      // вероятность пикселя загореться от 1/1 до 1/11
    for (uint32_t idx = 0U; idx < NUM_LEDS; idx++) {
      if (random8(deltaValue) == 0U) {
        ledsbuff[idx].r = random8();                               // оттенок пикселя
        ledsbuff[idx].g = random8(1U, TWINKLES_SPEEDS * 2U + 1U);  // скорость и направление (нарастает 1-4 или угасает 5-8)
        ledsbuff[idx].b = random8();                               // яркость
      } else {
        ledsbuff[idx] = 0U;                                        // всё выкл
      }
    }

    loadingFlag = false;
  }

  constexpr uint8_t speed_mult_sum = TWINKLES_SPEEDS + TWINKLES_MULTIPLIER;

  for (uint32_t idx = 0U; idx < NUM_LEDS; idx++) {
    CRGB &pixel = ledsbuff[idx];

    if (pixel.b == 0U) {
      if (hue > 0U && random8(deltaValue) == 0U) {                             // если пиксель ещё не горит, зажигаем каждый ХЗй
        pixel.r = random8();                                                   // оттенок пикселя
        pixel.g = random8(1U, TWINKLES_SPEEDS + 1U);                           // скорость и направление (нарастает 1-4, но не угасает 5-8)
        pixel.b = pixel.g;                                                     // яркость
        hue--;                                                                 // уменьшаем количество погасших пикселей
      }
    } else if (pixel.g <= TWINKLES_SPEEDS) {                                   // если нарастание яркости
      if (pixel.b > (uint8_t)(255U - pixel.g - TWINKLES_MULTIPLIER)) {         // если досигнут максимум
        pixel.b = 255U;
        pixel.g += TWINKLES_SPEEDS;
      } else {
        pixel.b += (pixel.g + TWINKLES_MULTIPLIER);
      }
    } else {                                                                   // если угасание яркости
      if (pixel.b <= (uint8_t)(pixel.g - speed_mult_sum)) {                    // если досигнут минимум
        pixel.b = 0U;                                                          // всё выкл
        hue++;                                                                 // считаем количество погасших пикселей
      } else {
        pixel.b -= (pixel.g - speed_mult_sum);
      }
    }

    if (pixel.b == 0U) {
      leds[idx] = CRGB::Black;
    } else {
      leds[idx] = ColorFromPalette(*curPalette, pixel.r, pixel.b);
    }
  }
}
#endif


#ifdef DEF_BALLS_BOUNCE
// ============= BOUNCE / ПРЫЖКИ / МЯЧИКИ БЕЗ ГРАНИЦ ===============
// Aurora : https://github.com/pixelmatix/aurora/blob/master/PatternBounce.h
// Copyright(c) 2014 Jason Coon
// v1.0 - Updating for GuverLamp v1.7 by Palpalych 14.04.2020

// #define e_bnc_COUNT (WIDTH) // теперь enlargedObjectNUM. хз, почему использовалась ширина матрицы тут, если по параметру идёт обращение к массиву boids, у которого может быть меньший размер
#define e_bnc_SIDEJUMP (true)

static PVector gravity = PVector(0, -0.0125f);

static void bounceRoutine()
{
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        setModeSettings(random8(9U) * 11U + 3U + random8(9U), random8(4U) ? 3U + random8(26U) : 255U);
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette();

    enlargedObjectNUM = (modes[currentMode].Scale - 1U) % 11U / 10.0f * (AVAILABLE_BOID_COUNT - 1U) + 1U;
    uint8_t colorWidth = 256U / enlargedObjectNUM;
    for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
      Boid boid = Boid(i % WIDTH, 0);
      boid.velocity.x = 0;
      boid.velocity.y = i * -0.01f;
      boid.colorIndex = colorWidth * i;
      boid.maxforce = 10;
      boid.maxspeed = 10;
      boids[i] = boid;
    }

    loadingFlag = false;
  }

  blurScreen(beatsin8(5U, 1U, 5U));
  dimAll(255U - modes[currentMode].Speed);

  for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
    Boid &boid = boids[i];

    boid.applyForce(gravity);
    boid.update();

    if (boid.location.x >= (float)WIDTH) boid.location.x -= (float)WIDTH; // это только
    else if (boid.location.x < 0.0f)     boid.location.x += (float)WIDTH; // для субпиксельной версии

    CRGB color = ColorFromPalette(*curPalette, boid.colorIndex);
    drawPixelXYF(boid.location.x, boid.location.y, color);

    if (boid.location.y <= 0.0f) {
      boid.location.y = 0.0f;
      boid.velocity.y = -boid.velocity.y;
      boid.velocity.x *= 0.9f;

      if (random8() == 0U || boid.velocity.y < 0.01f) {
#if e_bnc_SIDEJUMP
        boid.applyForce(PVector((float)random8(127U) * inv255 - 0.25f, (float)random8() * inv255));
#else
        boid.applyForce(PVector(0.0f, (float)random8() * inv255));
#endif
      }
    }
  }
}
#endif


#ifdef DEF_RINGS
// ------------------------------ ЭФФЕКТ КОЛЬЦА / КОДОВЫЙ ЗАМОК ----------------------
// (c) SottNick
// из-за повторного использоваия переменных от других эффектов теперь в этом коде невозможно что-то понять.
// поэтому для понимания придётся сперва заменить названия переменных на человеческие. но всё равно это песец, конечно.
// uint8_t deltaHue2;             // максимальне количество пикселей в кольце (толщина кольца) от 1 до CENTER_Y + 1
// uint8_t deltaHue;              // количество колец от 2 до HEIGHT
// uint8_t noise3d[1][1][HEIGHT]; // начальный оттенок каждого кольца (оттенка из палитры) 0-255
// uint8_t shiftValue[HEIGHT];    // местоположение начального оттенка кольца 0-WIDTH-1
// uint8_t shiftHue[HEIGHT];      // 4 бита на ringHueShift, 4 на ringHueShift2
// ringHueShift[ringsCount];      // шаг градиета оттенка внутри кольца -8 - +8 случайное число
// ringHueShift2[ringsCount];     // обычная скорость переливания оттенка всего кольца -8 - +8 случайное число
// uint8_t deltaValue;            // кольцо, которое в настоящий момент нужно провернуть
// uint8_t step;                  // оставшееся количество шагов, на которое нужно провернуть активное кольцо - случайное от WIDTH/5 до WIDTH-3
// uint8_t hue, hue2;             // количество пикселей в нижнем (hue) и верхнем (hue2) кольцах

static void ringsRoutine() {
  #define rings_count             deltaHue      // количество колец от 2 до HEIGHT
  #define ring_base_thickness     deltaHue2     // максимальне количество пикселей в кольце (толщина кольца) от 1 до CENTER_Y + 1
  #define ring_bottom_thickness   hue           // количество пикселей самого нижнего кольца
  #define ring_top_thickness      hue2          // количество пикселей самого верхнего кольца

  #define ring_base_hue           noise3d[0][0] // массив базовых оттенков для каждого кольца [rings_count]
  #define ring_hue_shifts         shiftHue      // 4 бита на градиент внутри кольца, 4 на скорость перелива [rings_count]
  #define ring_rotation_pos       shiftValue    // местоположение (сдвиг) начала кольца от 0 до WIDTH-1 [rings_count]

  #define active_ring_idx         deltaValue    // индекс кольца, которое в данный момент проворачивается
  #define active_ring_steps       step          // оставшееся количество шагов проворота активного кольца

  uint8_t h, x, y;

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(90U + random8(6U), 175U + random8(61U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette();

    // Математика вычисления толщины колец
    ring_base_thickness = (modes[currentMode].Scale - 1U) % 11U + 1U;                                // толщина кольца от 1 до 11 для каждой из палитр
    rings_count = HEIGHT / ring_base_thickness + ((HEIGHT % ring_base_thickness == 0U) ? 0U : 1U);   // количество колец
    ring_top_thickness = ring_base_thickness - (ring_base_thickness * rings_count - HEIGHT) / 2U;    // толщина верхнего кольца. может быть меньше нижнего
    ring_bottom_thickness = HEIGHT - ring_top_thickness - (rings_count - 2U) * ring_base_thickness;  // толщина нижнего кольца = всё оставшееся

    for (uint8_t i = 0U; i < rings_count; i++) {
      ring_base_hue[i] = random8(257U - CENTER_X);  // начальный оттенок кольца из палитры 0-255 за минусом длины кольца, делённой пополам
      ring_hue_shifts[i] = random8();
      ring_rotation_pos[i] = 0U;                    // random8(WIDTH); само прокрутится постепенно
      active_ring_steps = 0U;
      active_ring_idx = random8(rings_count);
    }

    loadingFlag = false;
  }

  for (uint8_t i = 0U; i < rings_count; i++) {
    if (i != active_ring_idx) {                                // если это не активное кольцо
      h = ring_hue_shifts[i] & 0x0FU;                          // младшие 4 бита: сдвигаем оттенок внутри кольца
      if (h > 8U)
        noise3d[0][0][i]--;
      else
        noise3d[0][0][i]++;
    } else {                                                   // если это активное проворачивающееся кольцо
      if (active_ring_steps  == 0U) {                          // если сдвиг активного кольца завершён, выбираем следующее
        active_ring_idx = random8(rings_count);
        do {
          active_ring_steps = WIDTH - 3U - random8((WIDTH - 3U) * 2U);
        } while (active_ring_steps < (WIDTH / 5U) || active_ring_steps > (255U - WIDTH / 5U));
      } else {
        if (active_ring_steps > 127U) {
          active_ring_steps++;
          ring_rotation_pos[i]++;
          if (ring_rotation_pos[i] >= WIDTH) ring_rotation_pos[i] = 0U;
        } else {
          active_ring_steps--;
          if (ring_rotation_pos[i] == 0U) ring_rotation_pos[i] = MAX_X;
          else                            ring_rotation_pos[i]--;
        }
      }
    }

    // отрисовываем кольца
    h = (ring_hue_shifts[i] >> 4) & 0x0FU; // старшие 4 бита: шаг для градиента внутри кольца
    if (h > 8U) h = 7U - h;

    // Определяем точную толщину текущего кольца (нижнее, верхнее или стандартное)
    uint8_t current_ring_thickness = (i == 0U) ? ring_bottom_thickness : ((i == rings_count - 1U) ? ring_top_thickness : ring_base_thickness);
    uint8_t y_offset = (i == 0U) ? 0U : (ring_base_thickness - ring_bottom_thickness);
    uint8_t current_base_hue = ring_base_hue[i];
    uint8_t current_rotation = ring_rotation_pos[i];

    for (uint8_t j = 0U; j < current_ring_thickness; j++) {  // от 0 до (толщина кольца - 1)
      y = i * ring_base_thickness + j - y_offset;

      // Вместо формул (current_rotation + k) % WIDTH мы запускаем бегущие указатели координат
      uint8_t left_x  = current_rotation; // Стартовая точка для первой половины полукольца

      // Старт для зеркальной половины. В целых числах: (WIDTH - 1 + current_rotation) % WIDTH.
      // Если current_rotation == 0, то (WIDTH - 1) % WIDTH = WIDTH - 1.
      // Иначе это просто current_rotation - 1.
      uint8_t right_x = (current_rotation == 0U) ? MAX_X : (current_rotation - 1U);

      for (uint8_t k = 0U; k < CENTER_X; k++) {
        // Отрисовка первой половины полукольца (сдвигается вправо)
        leds[XY(left_x, y)] = ColorFromPalette(*curPalette, current_base_hue + k * h);

        // Отрисовка второй половины полукольца (зеркально сдвигается влево)
        leds[XY(right_x, y)] = ColorFromPalette(*curPalette, current_base_hue + k * h);

        // Инкремент левой координаты с защитой от вылета за WIDTH за 1 такт процессора
        left_x++;
        if (left_x >= WIDTH) left_x = 0U;

        // Декремент правой координаты с защитой от ухода в минус
        if (right_x == 0U) right_x = MAX_X;
        else               right_x--;
      }

      // Оптимизация нечетной ширины: проверка constexpr вынесена из цикла по k
      if (WIDTH & 0x01U) {
        x = (current_rotation + CENTER_X);
        if (x >= WIDTH) x -= WIDTH; // Оптимизированная замена % WIDTH
        leds[XY(x, y)] = ColorFromPalette(*curPalette, current_base_hue + CENTER_X * h);
      }
    }
  }

  #undef rings_count
  #undef ring_base_thickness
  #undef ring_bottom_thickness
  #undef ring_top_thickness
  #undef ring_base_hue
  #undef ring_hue_shifts
  #undef ring_rotation_pos
  #undef active_ring_idx
  #undef active_ring_steps
}
#endif


#ifdef DEF_CUBE2D
// ------------------------------ ЭФФЕКТ КУБИК РУБИКА 2D ----------------------
// (c) SottNick

#define PAUSE_MAX (7U)                // пропустить 7 кадров после завершения анимации сдвига ячеек

// uint8_t noise3d[1][WIDTH][HEIGHT]; // тут используем только нулевую колонку и нулевую строку. просто для экономии памяти взяли существующий трёхмерный массив
// uint8_t hue2;                      // осталось шагов паузы
// uint8_t step;                      // текущий шаг сдвига (от 0 до deltaValue-1)
// uint8_t deltaValue;                // всего шагов сдвига (до razmer? до (razmer?+1)*shtuk?)
// uint8_t deltaHue, deltaHue2;       // глобальный X и глобальный Y нашего "кубика"
static uint8_t razmerX, razmerY;             // размеры ячеек по горизонтали / вертикали
static uint8_t shtukX, shtukY;               // количество ячеек по горизонтали / вертикали
static uint8_t poleX, poleY;                 // размер всего поля по горизонтали / вертикали (в том числе 1 дополнительная пустая дорожка-разделитель с какой-то из сторон)
static int8_t globalShiftX, globalShiftY;    // нужно ли сдвинуть всё поле по окончаии цикла и в каком из направлений (-1, 0, +1)
static bool seamlessX;                       // получилось ли сделать поле по Х бесшовным
static bool krutimVertikalno;                // направление вращения в данный момент

static void cube2dRoutine() {
  uint8_t x, y;
  uint8_t anim0;          // будем считать тут начальный пиксель для анимации сдвига строки/колонки
  int8_t shift, kudaVse;  // какое-то расчётное направление сдвига (-1, 0, +1)
  CRGB color, color2;

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        uint8_t tmp = random8(9U) * 11U + random8(8U);  // масштаб 1-7, палитры все 9
        if (tmp == 45U) tmp = 100U; // + белый цвет
        setModeSettings(tmp, 175U + random8(66U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette();

    ledsClear(); // esphome: FastLED.clear();

    razmerX = (modes[currentMode].Scale - 1U) % 11U + 1U;  // размер ячейки от 1 до 11 пикселей для каждой из 9 палитр
    razmerY = razmerX;

    if (modes[currentMode].Speed & 0x01)                   // по идее, ячейки не обязательно должны быть квадратными, поэтому можно тут поизвращаться
      razmerY = (razmerY << 1U) + 1U;

    shtukY = HEIGHT / (razmerY + 1U);
    if (shtukY < 2U) shtukY = 2U;
    y = HEIGHT / shtukY - 1U;
    if (razmerY > y) razmerY = y;
    poleY = (razmerY + 1U) * shtukY;

    shtukX = WIDTH / (razmerX + 1U);
    if (shtukX < 2U) shtukX = 2U;
    x = WIDTH / shtukX - 1U;
    if (razmerX > x) razmerX = x;
    poleX = (razmerX + 1U) * shtukX;

    seamlessX = (poleX == WIDTH);

    deltaHue = 0U;
    deltaHue2 = 0U;
    globalShiftX = 0;
    globalShiftY = 0;

    for (uint8_t j = 0U; j < shtukY; j++) {
      y = j * (razmerY + 1U);    // + deltaHue2 т.к. оно =0U
      for (uint8_t i = 0U; i < shtukX; i++) {
        x = i * (razmerX + 1U);  // + deltaHue т.к. оно =0U
        if (modes[currentMode].Scale == 100U)
          color = CHSV(45U, 0U, 128U + random8(128U));
        else
          color = ColorFromPalette(*curPalette, random8());

        for (uint8_t k = 0U; k < razmerY; k++)
          for (uint8_t m = 0U; m < razmerX; m++)
            leds[XY(x+m, y+k)] = color;
      }
    }

    step = 4U;       // текущий шаг сдвига первоначально с перебором (от 0 до deltaValue-1)
    deltaValue = 4U; // всего шагов сдвига (от razmer? до (razmer?+1) * shtuk?)
    hue2 = 0U;       // осталось шагов паузы

    loadingFlag = false;
  }

  // двигаем, что получилось...
  if (hue2 == 0U && step < deltaValue) {                                  // если пауза закончилась, а цикл вращения ещё не завершён
    step++;

    if (krutimVertikalno) {
      if (globalShiftY == 0U) {
        anim0 = (deltaHue2 == 0U) ? 0U : deltaHue2 - 1U;
      } else if (globalShiftY > 0U) {
        anim0 = deltaHue2;
      } else {
        anim0 = deltaHue2 - 1U;
      }

      const uint8_t step_x = razmerX + 1U;
      const uint16_t end_y = anim0 + poleY - 1U;

      for (uint8_t i = 0U; i < shtukX; i++) {
        x = (deltaHue + i * step_x) % WIDTH;

        if (noise3d[0][i][0] > 0) {                                       // в нулевой ячейке храним оставшееся количество ходов прокрутки
          noise3d[0][i][0]--;
          shift = noise3d[0][i][1] - 1;                                   // в первой ячейке храним направление прокрутки

          if (shift < 0) {                                                // если крутим столбец вниз
            color = leds[XY(x, anim0)];                                   // берём цвет от нижней строчки
            for (uint8_t k = anim0; k < end_y; k++) {
              color2 = leds[XY(x, k + 1U)];                               // берём цвет от строчки над нашей
              for (uint8_t m = x; m < x + razmerX; m++)
                leds[XY(m % WIDTH, k)] = color2;                          // копируем его на всю нашу строку
            }
            for (uint8_t m = x; m < x + razmerX; m++) {
              leds[XY(m % WIDTH, end_y)] = color;                         // цвет нижней строчки копируем на всю верхнюю
            }
          } else if (shift > 0) {                                         // если крутим столбец вверх
            color = leds[XY(x, end_y)];                                   // берём цвет от верхней строчки
            for (uint8_t k = end_y; k > anim0 ; k--) {
              color2 = leds[XY(x, k - 1U)];                               // берём цвет от строчки под нашей
              for (uint8_t m = x; m < x + razmerX; m++)
                leds[XY(m % WIDTH, k)] = color2;                          // копируем его на всю нашу строку
            }
            for (uint8_t m = x; m < x + razmerX; m++) {
              leds[XY(m % WIDTH, anim0)] = color;                         // цвет верхней строчки копируем на всю нижнюю
            }
          }
        }
      }
    } else {                                                              // Идём по вертикали, крутим по горизонтали (строки двигаются)
      if (seamlessX)
        anim0 = 0U;
      else if (globalShiftX == 0)
        anim0 = (deltaHue == 0U) ? 0U : deltaHue - 1U;
      else if (globalShiftX > 0)
        anim0 = deltaHue;
      else
        anim0 = deltaHue - 1U;

      const uint8_t step_y = razmerY + 1U;
      const uint16_t end_x = anim0 + poleX - 1U;

      for (uint8_t j = 0U; j < shtukY; j++) {
        y = deltaHue2 + j * step_y;

        if (noise3d[0][0][j] > 0) {                                       // в нулевой ячейке храним оставшееся количество ходов прокрутки
          noise3d[0][0][j]--;
          shift = noise3d[0][1][j] - 1;                                   // в первой ячейке храним направление прокрутки

          if (shift < 0) {                                                // если крутим строку влево
            color = leds[XY(anim0, y)];                                   // берём цвет от левой колонки (левого пикселя)
            for (uint8_t k = anim0; k < end_x; k++) {
              color2 = leds[XY(k + 1U, y)];                               // берём цвет от колонки (пикселя) правее
              for (uint8_t m = y; m < y + razmerY; m++) {
                leds[XY(k, m)] = color2;                                  // копируем его на всю нашу колонку
              }
            }
            for (uint8_t m = y; m < y + razmerY; m++) {
              leds[XY(end_x, m)] = color;                                 // цвет левой колонки копируем на всю правую
            }
          } else if (shift > 0) {                                         // если крутим столбец вверх
            color = leds[XY(end_x, y)];                                   // берём цвет от правой колонки
            for (uint8_t k = end_x; k > anim0 ; k--) {
              color2 = leds[XY(k - 1U, y)];                               // берём цвет от колонки левее
              for (uint8_t m = y; m < y + razmerY; m++) {
                leds[XY(k, m)] = color2;                                  // копируем его на всю нашу колонку
              }
            }
            for (uint8_t m = y; m < y + razmerY; m++) {
              leds[XY(anim0, m)] = color;                                 // цвет правой колонки копируем на всю левую
            }
          }
        }
      }
    }
  } else if (hue2 != 0U) {                                                // пропускаем кадры после прокрутки кубика (делаем паузу)
    hue2--;
  }

  if (step >= deltaValue) {                                               // если цикл вращения завершён, меняем местами соответствующие ячейки (цвет в них) и точку первой ячейки
    step = 0U;
    hue2 = PAUSE_MAX;

    deltaHue2 = deltaHue2 + globalShiftY;                                 // если часть ячеек двигалась на 1 пиксель, пододвигаем глобальные координаты начала
    globalShiftY = 0;

    deltaHue = (WIDTH + deltaHue + globalShiftX) % WIDTH;
    globalShiftX = 0;

    kudaVse = 0;                                                          // пришла пора выбрать следующие параметры вращения
    krutimVertikalno = random8(2U);

    if (krutimVertikalno) {                                               // идём по горизонтали, крутим по вертикали (столбцы двигаются)
      for (uint8_t i = 0U; i < shtukX; i++) {
        noise3d[0][i][1] = random8(3U);
        shift = noise3d[0][i][1] - 1;                                     // в первой ячейке храним направление прокрутки
        if (kudaVse == 0) {
          kudaVse = shift;
        } else if (shift != 0 && kudaVse != shift) {
          kudaVse = 50;
        }
      }
      int16_t check_y = (int16_t)deltaHue2 - kudaVse;                     // Защита от underflow: приведение к знаковому int16_t
      deltaValue = razmerY + ((check_y >= 0 && check_y + (int16_t)poleY < (int16_t)HEIGHT) ? random8(2U) : 1U);

      if (deltaValue == razmerY) {                                        // значит полюбому kudaVse было = (-1, 0, +1) - и для нуля в том числе мы двигаем весь куб на 1 пиксель
        globalShiftY = 1 - kudaVse;                                       // временно на единичку больше, чем надо
        for (uint8_t i = 0U; i < shtukX; i++) {
          if (noise3d[0][i][1] == 1U) {                                   // если ячейка никуда не планировала двигаться
            noise3d[0][i][1] = globalShiftY;
            noise3d[0][i][0] = 1U;                                        // в нулевой ячейке храним количество ходов сдвига
          } else {
            noise3d[0][i][0] = deltaValue;                                // в нулевой ячейке храним количество ходов сдвига
          }
        }
        globalShiftY--;
      } else {
        x = 0;
        for (uint8_t i = 0U; i < shtukX; i++) {
          if (noise3d[0][i][1] != 1U) {
            y = random8(shtukY);
            if (y > x) {
              x = y;
            }
            noise3d[0][i][0] = deltaValue * (x + 1U);                     // в нулевой ячейке храним количество ходов сдвига
          }
        }
        deltaValue = deltaValue * (x + 1U);
      }
    } else {                                                              // идём по вертикали, крутим по горизонтали (строки двигаются)
      for (uint8_t j = 0U; j < shtukY; j++) {
        noise3d[0][1][j] = random8(3);
        shift = noise3d[0][1][j] - 1;                                     // в первой ячейке храним направление прокрутки
        if (kudaVse == 0)
          kudaVse = shift;
        else if (shift != 0 && kudaVse != shift)
          kudaVse = 50;
      }
      if (seamlessX) {
        deltaValue = razmerX + ((kudaVse < 50) ? random8(2U) : 1U);
      } else {
        int16_t check_x = (int16_t)deltaHue - kudaVse;                    // Защита от underflow: приведение к знаковому int16_t
        deltaValue = razmerX + ((check_x >= 0 && check_x + (int16_t)poleX < (int16_t)WIDTH) ? random8(2U) : 1U);
      }

      if (deltaValue == razmerX) {                                        // значит полюбому kudaVse было = (-1, 0, +1) - и для нуля в том числе мы двигаем весь куб на 1 пиксель
        globalShiftX = 1 - kudaVse;                                       // временно на единичку больше, чем надо
        for (uint8_t j = 0U; j < shtukY; j++) {
          if (noise3d[0][1][j] == 1U) {                                   // если ячейка никуда не планировала двигаться
            noise3d[0][1][j] = globalShiftX;
            noise3d[0][0][j] = 1U;                                        // в нулевой ячейке храним количество ходов сдвига
          } else
            noise3d[0][0][j] = deltaValue;                                // в нулевой ячейке храним количество ходов сдвига
        }
        globalShiftX--;
      } else {
        y = 0;
        for (uint8_t j = 0U; j < shtukY; j++) {
          if (noise3d[0][1][j] != 1U) {
            x = random8(shtukX);
            if (x > y)
              y = x;
            noise3d[0][0][j] = deltaValue * (x + 1U);                     // в нулевой ячейке храним количество ходов сдвига
          }
        }
        deltaValue = deltaValue * (y + 1U);
      }
    }
  }
}
#endif


#if defined(DEF_SMOKE) || defined(DEF_SMOKE_COLOR)
// ------------------------------ ЭФФЕКТ ДЫМ ----------------------
// (c) SottNick

static void MultipleStreamSmoke(bool isColored){
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        uint8_t tmp = random8(9U);
        setModeSettings(isColored ? 1U + tmp * tmp : (random8(10U) ? 1U + random8(99U) : 100U), 145U + random8(56U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    hue2 = 0U;

    loadingFlag = false;
  }

  dimAll(254U);  // (255U - modes[currentMode].Scale * 2);

  deltaHue++;
  CRGB color;

  if (isColored) {
    if (hue2 == modes[currentMode].Scale) {
      hue2 = 0U;
      hue = random8();
    }
    if (deltaHue & 0x01) {
      hue2++;
    }

    hsv2rgb_spectrum(CHSV(hue, 255U, 127U), color);
  } else {
    hsv2rgb_spectrum(CHSV((modes[currentMode].Scale - 1U) * 2.6f, (modes[currentMode].Scale > 98U) ? 0U : 255U, 127U), color);
  }

  if (random8(WIDTH) != 0U) { // встречная спираль движется не всегда синхронно основной
    deltaHue2--;
  }

  for (uint8_t y = 0U; y < HEIGHT; y++) {
    uint8_t target_y = MAX_Y - y;

    leds[XY((deltaHue  + y + 1U) % WIDTH, target_y)] += color;
    leds[XY((deltaHue  + y     ) % WIDTH, target_y)] += color;
    leds[XY((deltaHue2 + y     ) % WIDTH,        y)] += color;
    leds[XY((deltaHue2 + y + 1U) % WIDTH,        y)] += color;
  }

  // Noise
  noise32_x[0] += 1500U;
  noise32_y[0] += 1500U;
  noise32_z[0] += 1500U;

  scale32_x[0] = 4000U;
  scale32_y[0] = 4000U;

  FillNoise(0);

  // допустимый отлёт зажжённого пикселя от изначально присвоенного местоположения (от 0 до указанного значения. дробное)
  MoveFractionalNoiseX(3);
  MoveFractionalNoiseY(3);

  blurScreen(20U); // без размытия как-то пиксельно, наверное...
}
#endif


#ifdef DEF_PICASSO
// ------------------------------ ЭФФЕКТЫ ПИКАССО ----------------------
// взято откуда-то by @obliterator или им написано
// https://github.com/DmytroKorniienko/FireLamp_JeeUI/blob/templ/src/effects.cpp

// вместо класса Particle будем повторно использовать переменные из эффекта мячики и мотыльки
// float   position_x = 0;
// float   trackingObjectPosX[enlargedOBJECT_MAX_COUNT];
// float   position_y = 0;
// float   trackingObjectPosY[enlargedOBJECT_MAX_COUNT];
// float   speed_x = 0;
// float   trackingObjectSpeedY[enlargedOBJECT_MAX_COUNT];                   // As time goes on the impact velocity will change, so make an array to store those values
// float   speed_y = 0;
// float   trackingObjectShift[enlargedOBJECT_MAX_COUNT];                    // Coefficient of Restitution (bounce damping)
// CHSV    color;
// uint8_t trackingObjectHue[enlargedOBJECT_MAX_COUNT];
// uint8_t hue_next = 0;
// uint8_t trackingObjectState[enlargedOBJECT_MAX_COUNT] ;                   // прикручено при адаптации для распределения мячиков по радиусу лампы
// int8_t  hue_step = 0;
// float   trackingObjectSpeedX[trackingOBJECT_MAX_COUNT];                   // The integer position of the dot on the strip (LED index)

static void PicassoGenerate(bool reset){
  if (loadingFlag) {
    enlargedObjectNUM = std::clamp(enlargedObjectNUM, (uint16_t)2U, (uint16_t)enlargedOBJECT_MAX_COUNT);

    constexpr float minSpeed = 0.2f;
    constexpr float maxSpeed = 0.8f;

    constexpr float speedDiv3 = -maxSpeed / 3.0f;
    constexpr float speedDiv2 = -maxSpeed / 2.0f;

    for (uint8_t i = 0 ; i < enlargedObjectNUM ; i++) {
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = random8(HEIGHT);

      trackingObjectHue[i] = random8();

      trackingObjectSpeedY[i] = speedDiv3 + (maxSpeed * (float)random8() / 255.0f);
      trackingObjectSpeedY[i] += (trackingObjectSpeedY[i] > 0.0f) ? minSpeed : -minSpeed;

      trackingObjectShift[i] = speedDiv2 + (maxSpeed * (float)random8() / 255.0f);
      trackingObjectShift[i] += (trackingObjectShift[i] > 0.0f) ? minSpeed : -minSpeed;

      trackingObjectState[i] = trackingObjectHue[i];
    }

    loadingFlag = false;
  }

  for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
    if (reset) {
      trackingObjectState[i] = random8();
      trackingObjectSpeedX[i] = (float)((int16_t)trackingObjectState[i] - (int16_t)trackingObjectHue[i]) / 25.0f;
    }
    if (trackingObjectState[i] != trackingObjectHue[i] && trackingObjectSpeedX[i] != 0.0f) {
      // trackingObjectHue[i] = (uint8_t)((float)trackingObjectHue[i] + trackingObjectSpeedX[i]);
      trackingObjectHue[i] = (uint8_t)std::round((float)trackingObjectHue[i] + trackingObjectSpeedX[i]);
    }
  }
}

static void PicassoPosition() {
  for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
    float nextX = trackingObjectPosX[i] + trackingObjectSpeedY[i];
    if (nextX > (float)WIDTH || nextX < 0.0f) {
      trackingObjectSpeedY[i] = -trackingObjectSpeedY[i];
    }

    float nextY = trackingObjectPosY[i] + trackingObjectShift[i];
    if (nextY > (float)HEIGHT || nextY < 0.0f) {
      trackingObjectShift[i] = -trackingObjectShift[i];
    }

    trackingObjectPosX[i] += trackingObjectSpeedY[i];
    trackingObjectPosY[i] += trackingObjectShift[i];
  }
}

static void PicassoRoutine(){
  #if false // defined(singleRANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      setModeSettings(17U+random8(64U) , 190U+random8(41U));
    }
  #endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

  PicassoGenerate(false);
  PicassoPosition();

  for (uint8_t i = 0U; i < enlargedObjectNUM - 2U; i += 2)
    DrawLine(trackingObjectPosX[i], trackingObjectPosY[i], trackingObjectPosX[i + 1U], trackingObjectPosY[i + 1U],
             CHSV(trackingObjectHue[i], 255U, 255U));

  EVERY_N_MILLIS(20000) {
    PicassoGenerate(true);
  }

  blurScreen(80);
}

static void PicassoRoutine2(){
  #if false // defined(singleRANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      setModeSettings(17U+random8(27U) , 185U+random8(46U));
    }
  #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

  PicassoGenerate(false);
  PicassoPosition();
  dimAll(180);

  for (uint8_t i = 0U; i < enlargedObjectNUM - 1U; i++)
    DrawLineF(trackingObjectPosX[i], trackingObjectPosY[i], trackingObjectPosX[i + 1U], trackingObjectPosY[i + 1U],
              CHSV(trackingObjectHue[i], 255U, 255U));

  EVERY_N_MILLIS(20000){
    PicassoGenerate(true);
  }

  blurScreen(80);

}

static void PicassoRoutine3(){
  #if false // defined(singleRANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      setModeSettings(19U+random8(31U) , 150U+random8(63U));
    }
  #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

  PicassoGenerate(false);
  PicassoPosition();
  dimAll(180);

  for (uint8_t i = 0U; i < enlargedObjectNUM - 2U; i += 2)
    drawCircleF(std::abs(trackingObjectPosX[i] - trackingObjectPosX[i + 1U]),
                std::abs(trackingObjectPosY[i] - trackingObjectPosX[i + 1U]),
                std::abs(trackingObjectPosX[i] - trackingObjectPosY[i]),
                CHSV(trackingObjectHue[i], 255U, 255U));

  EVERY_N_MILLIS(20000){
    PicassoGenerate(true);
  }

  blurScreen(80);

}

static void picassoSelector(){
  #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      uint8_t tmp = random8(3U);
      if (tmp == 2U)
        setModeSettings(4U+random8(42U) , 190U+random8(41U));
      else if (tmp)
        setModeSettings(39U+random8(8U) , 185U+random8(46U));
      else
        setModeSettings(73U+random8(10U) , 150U+random8(63U));
    }
  #endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)


  if (loadingFlag)
  {
    if (modes[currentMode].Scale < 34U)            // если масштаб до 34
      enlargedObjectNUM = (modes[currentMode].Scale - 1U) / 32.0f * (enlargedOBJECT_MAX_COUNT - 3U) + 3U;
    else if (modes[currentMode].Scale >= 68U)      // если масштаб больше 67
      enlargedObjectNUM = (modes[currentMode].Scale - 68U) / 32.0f * (enlargedOBJECT_MAX_COUNT - 3U) + 3U;
    else                                           // для масштабов посередине
      enlargedObjectNUM = (modes[currentMode].Scale - 34U) / 33.0f * (enlargedOBJECT_MAX_COUNT - 1U) + 1U;
  }

  if (modes[currentMode].Scale < 34U)              // если масштаб до 34
    PicassoRoutine();
  else if (modes[currentMode].Scale > 67U)         // если масштаб больше 67
    PicassoRoutine3();
  else                                             // для масштабов посередине
    PicassoRoutine2();
}
#endif

#ifdef DEF_LEAPERS
// ------------------------------ ЭФФЕКТ ПРЫГУНЫ ----------------------
// взято откуда-то by @obliterator
// https://github.com/DmytroKorniienko/FireLamp_JeeUI/blob/templ/src/effects.cpp

// Leaper leapers[20];
// вместо класса Leaper будем повторно использовать переменные из эффекта мячики и мотыльки
// float   x, y; будет:
// float   trackingObjectPosX[enlargedOBJECT_MAX_COUNT];
// float   trackingObjectPosY[enlargedOBJECT_MAX_COUNT];
// float   xd, yd; будет:
// float   trackingObjectSpeedX[enlargedOBJECT_MAX_COUNT];                   // As time goes on the impact velocity will change, so make an array to store those values
// float   trackingObjectSpeedY[enlargedOBJECT_MAX_COUNT];                   // Coefficient of Restitution (bounce damping)
// CHSV    color; будет:
// uint8_t trackingObjectHue[enlargedOBJECT_MAX_COUNT];

static void LeapersRestart_leaper(uint8_t l) {
  // leap up and to the side with some random component
  trackingObjectSpeedX[l] = (float)random8(1U, 100U) * inv100;
  trackingObjectSpeedY[l] = (float)random8(1U, 100U) * inv100 * 2.0f;

  // for variety, sometimes go 50% faster
  if (random8() < 12U) {
    trackingObjectSpeedX[l] *= 1.5f;
    trackingObjectSpeedY[l] *= 1.5f;
  }

  // leap towards the centre of the screen
  if (trackingObjectPosX[l] > CENTER_X) {
    trackingObjectSpeedX[l] = -trackingObjectSpeedX[l];
  }
}

static void LeapersMove_leaper(uint8_t l) {
  constexpr float GRAVITY           = 0.06f;
  constexpr float SETTLED_THRESHOLD = 0.1f;
  constexpr float WALL_FRICTION     = 0.95f;
  constexpr float WIND              = 0.95f;  // wind resistance

  trackingObjectPosX[l] += trackingObjectSpeedX[l];
  trackingObjectPosY[l] += trackingObjectSpeedY[l];

  // bounce off the floor and ceiling?
  if (trackingObjectPosY[l] < 0.0f || trackingObjectPosY[l] > MAX_Y) {
    trackingObjectSpeedY[l] = (-trackingObjectSpeedY[l] * WALL_FRICTION);
    trackingObjectSpeedX[l] = (trackingObjectSpeedX[l] * WALL_FRICTION);
    trackingObjectPosY[l] += trackingObjectSpeedY[l];

    if (trackingObjectPosY[l] < 0.0f) {
      trackingObjectPosY[l] = 0.0f; // settled on the floor?
    }
    if (trackingObjectPosY[l] <= SETTLED_THRESHOLD && std::abs(trackingObjectSpeedY[l]) <= SETTLED_THRESHOLD) {
      LeapersRestart_leaper(l);
    }
  }

  // bounce off the sides of the screen?
  if (trackingObjectPosX[l] <= 0.0f || trackingObjectPosX[l] >= MAX_X) {
    trackingObjectSpeedX[l] = (-trackingObjectSpeedX[l] * WALL_FRICTION);
    if (trackingObjectPosX[l] <= 0.0f) {
      trackingObjectPosX[l] = -trackingObjectPosX[l];
    } else {
      trackingObjectPosX[l] = (float)(WIDTH + WIDTH - 2U) - trackingObjectPosX[l];
    }
  }

  trackingObjectSpeedY[l] -= GRAVITY;
  trackingObjectSpeedX[l] *= WIND;
  trackingObjectSpeedY[l] *= WIND;
}


static void LeapersRoutine(){
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(random8(8U) * 11U + 5U + random8(7U) , 185U + random8(56U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette();

    enlargedObjectNUM = (float)((modes[currentMode].Scale - 1U) % 11U) * inv10 * (float)(enlargedOBJECT_MAX_COUNT - 1U) + 1U;
    if (enlargedObjectNUM > enlargedOBJECT_MAX_COUNT) {
      enlargedObjectNUM = enlargedOBJECT_MAX_COUNT;
    }

    for (uint8_t i = 0 ; i < enlargedObjectNUM ; i++) {
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = random8(HEIGHT);
      trackingObjectHue[i] = random8();
    }

    loadingFlag = false;
  }

  ledsClear(); // esphome: FastLED.clear();

  for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
    LeapersMove_leaper(i);
    drawPixelXYF(trackingObjectPosX[i], trackingObjectPosY[i], ColorFromPalette(*curPalette, trackingObjectHue[i]));
  };

  blurScreen(20U);
}
#endif


#ifdef DEF_LAVALAMP
// ------------------------------ ЭФФЕКТ ЛАВОВАЯ ЛАМПА ----------------------
// (c) SottNick

// float trackingObjectPosX[enlargedOBJECT_MAX_COUNT];                     // координата по Х
// float trackingObjectPosY[enlargedOBJECT_MAX_COUNT];                     // координата по Y
// float trackingObjectSpeedY[enlargedOBJECT_MAX_COUNT];                   // скорость движения пузыря
// float trackingObjectShift[enlargedOBJECT_MAX_COUNT];                    // радиус пузыря ... мог бы быть, если бы круги рисовались нормально

static void LavaLampGetspeed(uint8_t l) {
  trackingObjectSpeedY[l] = (float)random8(5U, 11U) / (257U - modes[currentMode].Speed) * inv4;  // / 4.0f // если скорость кадров фиксированная
}

static void drawBlob(uint8_t l, CRGB color) { //раз круги нарисовать не получается, будем попиксельно вырисовывать 2 варианта пузырей
  const float base_x = trackingObjectPosX[l] + (float)WIDTH;

  if ((uint8_t)trackingObjectShift[l] == 2U) {
    for (int8_t x = -2; x < 3; x++) {
      // Быстрый аналог fmod для зацикливания по ширине матрицы
      float target_x = base_x + (float)x;
      if (target_x >= (float)WIDTH) target_x -= (float)WIDTH;
      if (target_x < 0.0f) target_x += (float)WIDTH;

      for (int8_t y = -2; y < 3; y++) {
        if (std::abs(x) + std::abs(y) < 4U) {
          drawPixelXYF(target_x, trackingObjectPosY[l] + (float)y, color);
        }
      }
    }
  } else {
    for (int8_t x = -1; x < 3; x++) {
      float target_x = base_x + (float)x;
      if (target_x >= (float)WIDTH) target_x -= (float)WIDTH;
      if (target_x < 0.0f) target_x += (float)WIDTH;

      for (int8_t y = -1; y < 3; y++) {
        // Упростили логическое условие для исключения угловых пикселей
        if (!((x == -1 || x == 2) && (y == -1 || y == 2))) {
          drawPixelXYF(target_x, trackingObjectPosY[l] + (float)y, color);
        }
      }
    }
  }
}

static void LavaLampRoutine(){
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      setModeSettings(random8(30U) ? (random8(3U) ? 2U + random8(98U) : 1U) : 100U, 50U + random8(196U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    enlargedObjectNUM = CENTER_X - (MAX_X & 0x01);
    uint8_t shift = random8(2U);

    for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
      trackingObjectPosY[i] = 0.0f;
      trackingObjectPosX[i] = (float)(i * 2U + shift);
      LavaLampGetspeed(i);
      trackingObjectShift[i] = (float)random8(1U, 3U);  // присваивается случайный целочисленный радиус пузырям от 1 до 2
    }

    if (modes[currentMode].Scale != 1U) {
      hue = modes[currentMode].Scale * 2.57f;
    }

    loadingFlag = false;
  }

  if (modes[currentMode].Scale == 1U) {
    hue2++;
    if (hue2 % 16U == 0U) {  // 0x10
      hue++;
    }
  }

  CRGB color = CHSV(hue, (modes[currentMode].Scale < 100U) ? 255U : 0U, 255U);

  ledsClear(); // esphome: FastLED.clear();

  for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {       // двигаем по аналогии с https://jiwonk.im/lavalamp/
    float posY = trackingObjectPosY[i];
    float shiftY = trackingObjectShift[i];
    float speedY = trackingObjectSpeedY[i];

    // Движение и симуляция физики лавовой лампы у краев
    if (posY + shiftY >= MAX_Y) {
      posY += speedY * ((MAX_Y - posY) / shiftY + 0.005f);
    } else if (posY - shiftY <= 0.0f) {
      posY += speedY * (posY / shiftY + 0.005f);
    } else {
      posY += speedY;
    }

    // bounce off the floor and ceiling?
    if (posY < 0.01f) {                                   // почему-то при нуле появляется мерцание (один кадр, еле заметно)
      LavaLampGetspeed(i);
      posY = 0.01f;
    } else if (posY > HEIGHT - 1.01f) {                   // тоже на всякий пожарный
      LavaLampGetspeed(i);
      trackingObjectSpeedY[i] = -trackingObjectSpeedY[i];
      posY = HEIGHT - 1.01f;
    }

    trackingObjectPosY[i] = posY;
    drawBlob(i, color);                                  // рисуем попиксельно 2 размера пузырей
  };

  blurScreen(20U);
}
#endif


#ifdef DEF_SHADOWS
// ---------------------- SHADOWS -----------------------
// https://github.com/vvip-68/GyverPanelWiFi/blob/master/firmware/GyverPanelWiFi_v1.04/effects.ino
// (c) vvip-68
//

static void shadowsRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        setModeSettings(1U, 1U + random8(255U));
      }
    #endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
  }

  static uint16_t sPseudotime = 0U;
  static uint16_t sLastMillis = 0U;
  static uint16_t sHue16 = 0U;

  const uint8_t sat8 = beatsin88(87, 220, 250);
  const uint8_t brightdepth = beatsin88(341, 96, 224);
  const uint16_t brightnessthetainc16 = beatsin88(203, (25 * 256), (40 * 256));

  const uint8_t msmultiplier = beatsin88(map(modes[currentMode].Speed, 1, 255, 100, 255), 32, map(modes[currentMode].Speed, 1, 255, 60, 255));
  const uint16_t hueinc16 = beatsin88(113, 1, 3000);

  const uint16_t ms = millis();
  const uint16_t deltams = ms - sLastMillis ;

  const uint8_t effectBrightness = modes[currentMode].Scale * 2.55f;

  uint16_t hue16 = sHue16;

  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88(400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  const uint8_t inv_brightdepth = 255U - brightdepth;
  const long map_low = map(effectBrightness, 32, 255, 32, 125);
  const long map_high = map(effectBrightness, 32, 255, 125, 250);
  const uint16_t max_led_idx = NUM_LEDS - 1U;

  for(uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 >> 8U; // / 256

    brightnesstheta16 += brightnessthetainc16;
    uint16_t b16 = sin16(brightnesstheta16) + 32768U;

    uint16_t bri16 = ((uint32_t)b16 * b16) >> 16U;          // / 65536
    uint8_t bri8 = ((uint32_t)bri16 * brightdepth) >> 16U;  // / 65536

    bri8 += inv_brightdepth;

    CRGB newcolor = CHSV(hue8, sat8, map8(bri8, map_low, map_high));

    uint16_t pixelnumber = max_led_idx - i;
    nblend(leds[pixelnumber], newcolor, 64U);
  }
}
#endif


#ifdef DEF_DNA
// ----------- Эффект "ДНК"
// База https://pastebin.com/jwvC1sNF адаптация и доработки kostyamat
// нормальные копирайты:
// https://pastebin.com/jwvC1sNF
// 2 DNA spiral with subpixel
// 16x16 rgb led matrix demo
// Yaroslaw Turbin 04.09.2020
// https://vk.com/ldirko
// https://www.reddit.com/user/ldirko/
// https://www.reddit.com/r/FastLED/comments/gogs4n/i_made_7x11_matrix_for_my_ntp_clock_project_then/
// this is update for DNA procedure https://pastebin.com/Qa8A5NvW
// add subpixel render foк nice smooth look

static void DNARoutine()
{
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(1U + random8(100U), 1U + random8(200U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    step = map8(modes[currentMode].Speed, 10U, 60U);
    hue = modes[currentMode].Scale;
    deltaHue = hue > 50U;
    if (deltaHue)
      hue = 101U - hue;
    hue = 255U - map(51U - hue, 1U, 50U, 0, 255U);
  }

  constexpr uint16_t freq = 3000U;
  // float mn = 255.0/13.8; // Ниже...

  fadeToBlackBy(leds, NUM_LEDS, step);
  const uint16_t base_hue = millis() / 29U;

  if (deltaHue) {
    constexpr uint16_t w_step = 255U / MAX_X;
    constexpr uint16_t h_step = MAX_Y * 256U;

    for (uint8_t i = 0U; i < WIDTH; i++)
    {
      uint32_t x = beatsin16(step, 0, h_step, 0, i * freq);
      uint32_t y = (uint32_t)i << 8U;             // i * 256;
      uint32_t x1 = beatsin16(step, 0, h_step, 0, i * freq + 32768U);

      uint16_t i_mn = ((uint16_t)i * 425U) / 23U; // mn = 255.0 / 13.8. В целых числах это идеальная дробь 425 / 23 (дает 18.4782)
                                                  // Для i * mn мы будем писать: ((uint16_t)i * 425) / 23
      uint8_t w_hue = base_hue + (i * w_step);

      CRGB col = CHSV(w_hue, 255U, qadd8(hue, beatsin8(step, 60, 255U, 0, i_mn)));
      CRGB col1 = CHSV(w_hue + 128U, 255U, qadd8(hue, beatsin8(step, 60, 255U, 0, i_mn + 128)));

      wu_pixel (y , x, &col);
      wu_pixel (y , x1, &col1);
    }
  } else {
    constexpr uint16_t w_step = MAX_X * 256U;
    constexpr uint16_t h_step = 255U / MAX_Y;

    for (uint8_t i = 0U; i < HEIGHT; i++)
    {
      uint32_t x = beatsin16(step, 0, w_step, 0, i * freq);
      uint32_t y = (uint32_t)i << 8U;             // i * 256
      uint32_t x1 = beatsin16(step, 0, w_step, 0, i * freq + 32768U);

      uint16_t i_mn = ((uint16_t)i * 425U) / 23U; // mn = 255.0 / 13.8. В целых числах это идеальная дробь 425 / 23 (дает 18.4782)
                                                  // Для i * mn мы будем писать: ((uint16_t)i * 425) / 23
      uint8_t h_hue = base_hue + (i * h_step);

      CRGB col = CHSV(h_hue, 255U, qadd8(hue, beatsin8(step, 60, 255U, 0, i_mn)));
      CRGB col1 = CHSV(h_hue + 128U, 255U, qadd8(hue, beatsin8(step, 60, 255U, 0, i_mn + 128)));

      wu_pixel (x , y, &col);
      wu_pixel (x1 , y, &col1);
    }
  }

  blurScreen(16U);
}
#endif


#ifdef DEF_SNAKES
// ------------- Змейки --------------
// (c) SottNick

// #define enlargedOBJECT_MAX_COUNT            (WIDTH * 2)   // максимальное количество червяков
// uint8_t enlargedObjectNUM;                                // выбранное количество червяков
// long    enlargedObjectTime[enlargedOBJECT_MAX_COUNT] ;    // тут будет траектория тела червяка
// float   trackingObjectPosX[trackingOBJECT_MAX_COUNT];     // тут будет позиция головы
// float   trackingObjectPosY[trackingOBJECT_MAX_COUNT];     // тут будет позиция головы
// float   trackingObjectSpeedX[trackingOBJECT_MAX_COUNT];   // тут будет скорость червяка
// float   trackingObjectSpeedY[trackingOBJECT_MAX_COUNT];   // тут будет дробная часть позиции головы
// float   trackingObjectShift[trackingOBJECT_MAX_COUNT];    // не пригодилось пока что
// uint8_t trackingObjectHue[trackingOBJECT_MAX_COUNT];      // тут будет начальный цвет червяка
// uint8_t trackingObjectState[trackingOBJECT_MAX_COUNT];    // тут будет направление червяка

#define SNAKES_LENGTH (8U) // длина червяка от 2 до 15 (+ 1 пиксель голова/хвостик), ограничена размером переменной для хранения трактории тела червяка

static void snakesRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        uint8_t tmp = random8(8U);
        setModeSettings(8U + tmp * tmp, 20U + random8(120U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    speedfactor = (float)modes[currentMode].Speed / 555.0f + 0.001f;

    enlargedObjectNUM = (modes[currentMode].Scale - 1U) / 99.0f * (enlargedOBJECT_MAX_COUNT - 1U) + 1U;
    if (enlargedObjectNUM > enlargedOBJECT_MAX_COUNT) {
      enlargedObjectNUM = enlargedOBJECT_MAX_COUNT;
    }

    for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
      enlargedObjectTime[i] = 0;
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = random8(HEIGHT);
      trackingObjectSpeedX[i] = (255.0f + random8()) / 255.0f;
      trackingObjectSpeedY[i] = 0;
      // trackingObjectShift[i] = 0;
      trackingObjectHue[i] = random8();
      trackingObjectState[i] = random8(4); //     B00           направление головы змейки
                                           // B10     B11
                                           //     B01
    }

    loadingFlag = false;
  }

  ledsClear(); // esphome: FastLED.clear();

  int8_t dx = 0, dy = 0;

  for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
    trackingObjectSpeedY[i] += trackingObjectSpeedX[i] * speedfactor;

    if (trackingObjectSpeedY[i] >= 1.0f) {
      trackingObjectSpeedY[i] -= 1.0f; // Быстрое отсечение целой части

      if (random8(9U) == 0U) { // Вероятность поворота
        if (random8(2U)) {    // Поворот налево
          enlargedObjectTime[i] = (enlargedObjectTime[i] << 2) | 0b01;
          switch (trackingObjectState[i]) {
            case 0b10:
              trackingObjectState[i] = 0b01;
              if (trackingObjectPosY[i] == 0U) trackingObjectPosY[i] = MAX_Y; else trackingObjectPosY[i]--;
              break;
            case 0b11:
              trackingObjectState[i] = 0b00;
              if (trackingObjectPosY[i] >= MAX_Y) trackingObjectPosY[i] = 0U; else trackingObjectPosY[i]++;
              break;
            case 0b00:
              trackingObjectState[i] = 0b10;
              if (trackingObjectPosX[i] == 0U) trackingObjectPosX[i] = MAX_X; else trackingObjectPosX[i]--;
              break;
            case 0b01:
              trackingObjectState[i] = 0b11;
              if (trackingObjectPosX[i] >= MAX_X) trackingObjectPosX[i] = 0U; else trackingObjectPosX[i]++;
              break;
          }
        } else { // Поворот направо
          enlargedObjectTime[i] = (enlargedObjectTime[i] << 2) | 0b11;
          switch (trackingObjectState[i]) {
            case 0b11:
              trackingObjectState[i] = 0b01;
              if (trackingObjectPosY[i] == 0U) trackingObjectPosY[i] = MAX_Y; else trackingObjectPosY[i]--;
              break;
            case 0b10:
              trackingObjectState[i] = 0b00;
              if (trackingObjectPosY[i] >= MAX_Y) trackingObjectPosY[i] = 0U; else trackingObjectPosY[i]++;
              break;
            case 0b01:
              trackingObjectState[i] = 0b10;
              if (trackingObjectPosX[i] == 0U) trackingObjectPosX[i] = MAX_X; else trackingObjectPosX[i]--;
              break;
            case 0b00:
              trackingObjectState[i] = 0b11;
              if (trackingObjectPosX[i] >= MAX_X) trackingObjectPosX[i] = 0U; else trackingObjectPosX[i]++;
              break;
          }
        }
      } else { // Движение без поворота
        enlargedObjectTime[i] = (enlargedObjectTime[i] << 2);
        switch (trackingObjectState[i]) {
          case 0b01: if (trackingObjectPosY[i] == 0U) trackingObjectPosY[i] = MAX_Y; else trackingObjectPosY[i]--; break;
          case 0b00: if (trackingObjectPosY[i] >= MAX_Y) trackingObjectPosY[i] = 0U; else trackingObjectPosY[i]++; break;
          case 0b10: if (trackingObjectPosX[i] == 0U) trackingObjectPosX[i] = MAX_X; else trackingObjectPosX[i]--; break;
          case 0b11: if (trackingObjectPosX[i] >= MAX_X) trackingObjectPosX[i] = 0U; else trackingObjectPosX[i]++; break;
        }
      }
    }

    switch (trackingObjectState[i]) {
      case 0b01: dy = 1;  dx = 0;  break;
      case 0b00: dy = -1; dx = 0;  break;
      case 0b10: dy = 0;  dx = 1;  break;
      case 0b11: dy = 0;  dx = -1; break;
    }

    long temp = enlargedObjectTime[i];
    uint8_t x = trackingObjectPosX[i];
    uint8_t y = trackingObjectPosY[i];
    float speedY = trackingObjectSpeedY[i];

    // Голова
    leds[XY(x, y)] += CHSV(trackingObjectHue[i], 255U, (uint8_t)(speedY * 255.0f));

    // Тело
    for (uint8_t m = 0U; m < SNAKES_LENGTH; m++) { // 16 бит распаковываем, 14 ещё остаётся без дела в запасе, 2 на хвостик
      x += dx;
      if (x >= WIDTH) x = 0U; else if (x < 0) x = MAX_X;

      y += dy;
      if (y >= HEIGHT) y = 0U; else if (y < 0) y = MAX_Y;

      leds[XY(x, y)] += CHSV(trackingObjectHue[i] + (uint8_t)((float)m + speedY) * 4U, 255U, 255U);

      if (temp & 0b01) { // младший бит = поворот, старший = направо
        temp >>= 1;
        if (temp & 0b01) { // старший бит = направо
          if (dx == 0) { dx = -dy; dy = 0; } else { dy = dx; dx = 0; }
        } else { // иначе налево
          if (dx == 0) { dx = dy; dy = 0; } else { dy = -dx; dx = 0; }
        }
        temp >>= 1;
      } else { // если без поворота
        temp >>= 2;
      }
    }

    // Хвостик
    x += dx;
    if (x >= WIDTH) x = 0U; else if (x < 0) x = MAX_X;

    y += dy;
    if (y >= HEIGHT) y = 0U; else if (y < 0) y = MAX_Y;

    leds[XY(x, y)] += CHSV(trackingObjectHue[i] + (SNAKES_LENGTH + (uint8_t)speedY) * 4U, 255U, (uint8_t)((1.0f - speedY) * 255.0f));
  }
}
#endif


#if defined(DEF_LIQUIDLAMP) || defined(DEF_LIQUIDLAMP_AUTO) || defined(DEF_AURORA) || defined(DEF_SPECTRUM)

// генератор палитр для Жидкой лампы (c) SottNick
// генератор палитр для Северного сияния (c) SottNick
// static const uint8_t MBVioletColors_arr[5][4] PROGMEM = // та же палитра, но в формате CHSV

static CRGBPalette16 myPal;

// Передаем указатель на PROGMEM-массив как третий параметр (c) andrewjswan
static void fillMyPal16(uint8_t hue, bool isInvert, const uint8_t (*colors_arr)[4]) {
  int8_t lastSlotUsed = -1;
  uint8_t istart8, iend8;
  CRGB rgbstart, rgbend;

  // Строка 0 (индекс 0)
  const uint8_t h_offset0 = pgm_read_byte(&colors_arr[0][1]);
  const uint8_t sat0      = pgm_read_byte(&colors_arr[0][2]);
  const uint8_t val0      = pgm_read_byte(&colors_arr[0][3]);

  if (isInvert) {
    hsv2rgb_spectrum(CHSV((uint8_t)(256U + hue - h_offset0), sat0, val0), rgbstart);
  } else {
    hsv2rgb_spectrum(CHSV((uint8_t)(hue + h_offset0), sat0, val0), rgbstart);
  }

  uint8_t indexstart = 0U;
  for (uint8_t i = 1U; i < 5U; i++) {  // В палитре всего 5 строчек
    const uint8_t indexend = pgm_read_byte(&colors_arr[i][0]);
    const uint8_t h_offset = pgm_read_byte(&colors_arr[i][1]);
    const uint8_t sat      = pgm_read_byte(&colors_arr[i][2]);
    const uint8_t val      = pgm_read_byte(&colors_arr[i][3]);

    // Исправлен баг автора: теперь инверсия работает симметрично стартовой точке
    if (isInvert) {
      hsv2rgb_spectrum(CHSV((uint8_t)(256U + hue - h_offset), sat, val), rgbend);
    } else {
      hsv2rgb_spectrum(CHSV((uint8_t)(hue + h_offset), sat, val), rgbend);
    }

    // Быстрый побитовый сдвиг вместо деления на 16 (Правило 2)
    istart8 = indexstart >> 4U;
    iend8   = indexend   >> 4U;

    if ((istart8 <= (uint8_t)lastSlotUsed) && (lastSlotUsed < 15)) {
      istart8 = (uint8_t)lastSlotUsed + 1U;
      if (iend8 < istart8) {
        iend8 = istart8;
      }
    }

    lastSlotUsed = iend8;
    fill_gradient_RGB(myPal, istart8, rgbstart, iend8, rgbend);

    indexstart = indexend;
    rgbstart = rgbend;
  }
}
#endif


#if defined(DEF_LIQUIDLAMP) || defined(DEF_LIQUIDLAMP_AUTO)
// ----------------------------- Жидкая лампа ---------------------
// ----------- Эффект "Лавовая лампа" (c) obliterator
// https://github.com/DmytroKorniienko/FireLamp_JeeUI/commit/9bad25adc2c917fbf3dfa97f4c498769aaf76ebe
// с генератором палитр by SottNick

static float mapcurve(const float x, const float in_min, const float in_max, const float out_min, const float out_max, float (*curve)(float,float,float,float)) {
  if (x <= in_min) {
    return out_min;
  }
  if (x >= in_max) {
    return out_max;
  }
  return curve((x - in_min), out_min, (out_max - out_min), (in_max - in_min));
}

static float InQuad(float t, float b, float c, float d) {
  t /= d;
  return c * t * t + b;
}

static float OutQuart(float t, float b, float c, float d) {
  t = t / d - 1;
  return -c * (t * t * t * t - 1) + b;
}

static float InOutQuad(float t, float b, float c, float d) {
  t /= d / 2;
  if (t < 1) {
    return c / 2 * t * t + b;
  }
  --t;
  return -c / 2 * (t * (t - 2) - 1) + b;
}

// Масштаб: 16x16 — базовый размер.
constexpr float SCALE = MIN_SIDE / 16.0f;

// Масса пузырей: масштабируется с площадью матрицы
constexpr uint16_t MASS_MIN = std::max((uint16_t)5U, static_cast<uint16_t>(10.0f * SCALE));
constexpr uint16_t MASS_MAX = std::max((uint16_t)20U, static_cast<uint16_t>(50.0f * SCALE));

// Радиус пузыря: 12.5%-18.75% от меньшей стороны матрицы
constexpr float BASE_RADIUS_MIN = 0.125f * MIN_SIDE;
constexpr float BASE_RADIUS_MAX = 0.1875f * MIN_SIDE;

// Сила возмущения: масштабируется с размером
constexpr float BASE_FORCE_MIN = 40.0f * SCALE;
constexpr float BASE_FORCE_MAX = 80.0f * SCALE;

// Радиус возмущения: 37.5%-62.5% от меньшей стороны
constexpr float BASE_DISTURB_MIN = 0.375f * MIN_SIDE;
constexpr float BASE_DISTURB_MAX = 0.625f * MIN_SIDE;

// Отступ от границ для отключения физики: ~18.75% от высоты
constexpr float BOUNDARY_MARGIN = std::max(2.0f, 0.1875f * static_cast<float>(HEIGHT));

//массивы для метаболов (используем повторно всё подряд)
//uint8_t trackingObjectHue[enlargedOBJECT_MAX_COUNT];
//        float position_x = 0;
//float trackingObjectPosX[enlargedOBJECT_MAX_COUNT];
//        float position_y = 0;
//float trackingObjectPosY[enlargedOBJECT_MAX_COUNT];
//        float speed_x = 0;
//float trackingObjectSpeedX[enlargedOBJECT_MAX_COUNT];
//        float speed_y = 0;
//float trackingObjectSpeedY[enlargedOBJECT_MAX_COUNT];
//        float rad = 0;
//float trackingObjectShift[enlargedOBJECT_MAX_COUNT];
//        float hot = 0;
//float liquidLampHot[enlargedOBJECT_MAX_COUNT];
//        float spf = 0;
//float liquidLampSpf[enlargedOBJECT_MAX_COUNT];
//        int mass = 0;
//uint8_t trackingObjectState[enlargedOBJECT_MAX_COUNT];
//        unsigned mx = 0;
//unsigned liquidLampMX[enlargedOBJECT_MAX_COUNT];
//        unsigned sc = 0;
//unsigned liquidLampSC[enlargedOBJECT_MAX_COUNT];
//        unsigned tr = 0;
//unsigned liquidLampTR[enlargedOBJECT_MAX_COUNT];

static void LiquidLampPosition() {
  for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
    // Термический подъём: масштабируется с высотой матрицы
    liquidLampHot[i] += mapcurve(trackingObjectPosY[i], 0, MAX_Y, 5, -5, InOutQuad) * speedfactor;

    float heat = (liquidLampHot[i] / trackingObjectState[i]) - 1;
    if (heat > 0 && trackingObjectPosY[i] < MAX_Y) {
      trackingObjectSpeedY[i] += heat * liquidLampSpf[i];
    }

    // Гравитация: масштабируется с высотой для постоянной скорости падения
    if (trackingObjectPosY[i] > 0) {
      trackingObjectSpeedY[i] -= 0.07f * SCALE;
    }

    trackingObjectSpeedY[i] *= 0.85f;
    trackingObjectPosY[i] += trackingObjectSpeedY[i] * speedfactor;

    // Горизонтальное движение
    trackingObjectSpeedX[i] *= 0.7f;
    trackingObjectPosX[i] += trackingObjectSpeedX[i] * speedfactor;

    // Бесшовное зацикливание по X
    if (trackingObjectPosX[i] >= WIDTH) {
      trackingObjectPosX[i] -= WIDTH;
    } else if (trackingObjectPosX[i] < 0.0f) {
      trackingObjectPosX[i] += WIDTH;
    }

    // Ограничение по Y
    if (trackingObjectPosY[i] >= HEIGHT) {
      trackingObjectPosY[i] = MAX_Y;
    } else if (trackingObjectPosY[i] < 0.0f) {
      trackingObjectPosY[i] = 0.0f;
    }
  }
}

static void LiquidLampPhysic() {
  const float boundary_top = MAX_Y - BOUNDARY_MARGIN;

  for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
    float pi_y = trackingObjectPosY[i];

    // Отключаем физику у границ
    if (pi_y < BOUNDARY_MARGIN || pi_y > boundary_top) continue;

    const float pi_x = trackingObjectPosX[i];
    const float rad_i = trackingObjectShift[i];
    const float mass_i = trackingObjectState[i];

    for (uint8_t j = i + 1; j < enlargedObjectNUM; j++) {
      float pj_y = trackingObjectPosY[j];

      // Отключаем физику у границ
      if (pj_y < BOUNDARY_MARGIN || pj_y > boundary_top) continue;

      // Радиус взаимодействия масштабируется с размером матрицы
      float radius = (rad_i + trackingObjectShift[j]) * inv2;

      float abs_diff_x = std::abs(pi_x - trackingObjectPosX[j]);
      float abs_diff_y = std::abs(pi_y - pj_y);

      // Быстрая проверка коллизий
      if (abs_diff_x > radius * 2.0f || abs_diff_y > radius * 2.0f) {
        continue;
      }

      // Бесшовное расстояние по X
      float dx = min(abs_diff_x, (float)WIDTH - abs_diff_x);
      float dy = abs_diff_y;
      float dist = SQRT_VARIANT(dx * dx + dy * dy);

      if (dist <= radius && dist > 0.01f) {
        float nx = (trackingObjectPosX[j] - trackingObjectPosX[i]) / dist;
        float ny = (trackingObjectPosY[j] - trackingObjectPosY[i]) / dist;

        // Импульс с учётом массы
        float p = 2 * (trackingObjectSpeedX[i] * nx + trackingObjectSpeedY[i] * ny -
                       trackingObjectSpeedX[j] * nx - trackingObjectSpeedY[j] * ny) /
                  (trackingObjectState[i] + trackingObjectState[j]);

        float pnx = p * nx, pny = p * ny;

        trackingObjectSpeedX[i] -= pnx * trackingObjectState[i];
        trackingObjectSpeedY[i] -= pny * trackingObjectState[i];
        trackingObjectSpeedX[j] += pnx * trackingObjectState[j];
        trackingObjectSpeedY[j] += pny * trackingObjectState[j];
      }
    }
  }
}

static void LiquidLampRoutine(bool isColored){
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        uint8_t tmp = random8(28U);
        if (tmp > 9U) tmp += 21U;
        if (tmp > 38U) tmp += 7U;
        if (tmp > 48U) tmp += 44U;
        setModeSettings(isColored ? tmp : 27U + random8(54U), 30U + random8(170U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    speedfactor = ((float)modes[currentMode].Speed / 64.0f + 0.1f) / SCALE;

    if (isColored) {
      fillMyPal16((modes[currentMode].Scale - 1U) * 2.55f, !(modes[currentMode].Scale & 0x01), MBVioletColors_arr);
      // Количество объектов пропорционально площади матрицы
      enlargedObjectNUM = (uint8_t)(NUM_LEDS >> 1U) - 2U; // / 2
    } else {
      hue = random8();
      deltaHue = random8(2U);
      fillMyPal16(hue, deltaHue, MBVioletColors_arr);
      enlargedObjectNUM = (uint8_t)((float)(modes[currentMode].Scale - 1U) / 99.9f * (float)(enlargedOBJECT_MAX_COUNT - 1U) + 1U);
    }
    enlargedObjectNUM = clamp(enlargedObjectNUM, (uint16_t)2U, (uint16_t)enlargedOBJECT_MAX_COUNT);

    // Инициализация пузырей с масштабируемыми параметрами
    for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = 0.0f;

      // Масса: в диапазоне MASS_MIN..MASS_MAX
      trackingObjectState[i] = random16(MASS_MIN, MASS_MAX);

      // Скорость плавучести: обратно пропорциональна массе, с учётом масштаба
      liquidLampSpf[i] = remap(trackingObjectState[i],
                               (uint8_t)MASS_MIN, (uint8_t)MASS_MAX,
                               0.0015f / SCALE, 0.0005f / SCALE);

      // Радиус пузыря: в диапазоне BASE_RADIUS_MIN..BASE_RADIUS_MAX
      trackingObjectShift[i] = remap(trackingObjectState[i],
                                     (uint8_t)MASS_MIN, (uint8_t)MASS_MAX,
                                     BASE_RADIUS_MIN, BASE_RADIUS_MAX);

      // Сила возмущения поля в диапазоне BASE_FORCE_MIN..BASE_FORCE_MAX
      liquidLampMX[i] = (unsigned)remap(trackingObjectState[i],
                                        (uint8_t)MASS_MIN, (uint8_t)MASS_MAX,
                                        BASE_FORCE_MIN, BASE_FORCE_MAX);

      // Радиус возмущения в диапазоне BASE_DISTURB_MIN..BASE_DISTURB_MAX
      liquidLampSC[i] = (unsigned)remap(trackingObjectState[i],
                                        (uint8_t)MASS_MIN, (uint8_t)MASS_MAX,
                                        BASE_DISTURB_MIN, BASE_DISTURB_MAX);

      // Порог оптимизации (2/3 от радиуса возмущения)
      liquidLampTR[i] = (unsigned)((float)liquidLampSC[i] * 2.0f / 3.0f);
    }

    loadingFlag = false;
  }

  LiquidLampPosition();
  LiquidLampPhysic();

  // Анимация палитры для монохромного режима
  if (!isColored) {
    hue2++;
    if (hue2 % 16U == 0U) { // 0x10 = 16U
      hue++;
      fillMyPal16(hue, deltaHue, MBVioletColors_arr);
    }
  }

  // Рендеринг: расчёт влияния каждого пузыря на каждый пиксель
  for (uint8_t x = 0U; x < WIDTH; x++) {
    for (uint8_t y = 0U; y < HEIGHT; y++) {
      float sum = 0;

      for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
        const float obj_x = trackingObjectPosX[i];
        const float obj_y = trackingObjectPosY[i];
        const float tr_limit = (float)liquidLampTR[i];

        // Быстрое отсечение: если пиксель далеко от пузыря — пропускаем
        if (std::abs(x - obj_x) > tr_limit || std::abs(y - obj_y) > tr_limit) {
          continue;
        }

        // Бесшовное расстояние по X
        float abs_dx = std::abs(obj_x - (float)x);
        float dx = min(abs_dx, (float)WIDTH - abs_dx);
        float dy = std::abs(obj_y - (float)y);
        float d = SQRT_VARIANT(dx * dx + dy * dy);

        const float r_shift = trackingObjectShift[i];

        if (d < r_shift) {
          // Внутри пузыря: яркость растёт к центру
          sum += mapcurve(d, 0.0f, r_shift, 255.0f, (float)liquidLampMX[i], InQuad);
        } else {
          // В зоне возмущения: яркость спадает к краям
          const float r_sc = (float)liquidLampSC[i];
          if (d < r_sc) {
            sum += mapcurve(d, r_shift, r_sc, (float)liquidLampMX[i], 0.0f, OutQuart);
          }
        }

        if (sum >= 255.0f) {
          sum = 255.0f;
          break;
        }
      }

      // Минимальная яркость для избежания артефактов палитры
      if (sum < 16.0f) sum = 16.0f;

      leds[XY(x, y)] = ColorFromPalette(myPal, (uint8_t)sum);
    }
  }
}
#endif


#ifdef DEF_POPCORN
// ----------- Эффект "Попкорн"
// (C) Aaron Gotwalt (Soulmate)
// https://editor.soulmatelights.com/gallery/117
// переосмысление (c) SottNick

// uint8_t NUM_ROCKETS = 10;
// enlargedObjectNUM = (modes[currentMode].Scale - 1U) % 11U / 10.0 * (AVAILABLE_BOID_COUNT - 1U) + 1U;

// typedef struct
// {
//   int32_t x, y, xd, yd;
// } Rocket;
// float trackingObjectPosX[trackingOBJECT_MAX_COUNT];
// float trackingObjectPosY[trackingOBJECT_MAX_COUNT];
// float trackingObjectSpeedX[trackingOBJECT_MAX_COUNT];
// float trackingObjectSpeedY[trackingOBJECT_MAX_COUNT];

static void popcornRestart_rocket(uint8_t r) {
  // deltaHue = !deltaHue; // "Мальчик" <> "Девочка"
  constexpr int32_t range = NUM_LEDS + (WIDTH * 2U);

  trackingObjectSpeedX[r] = (float)random(-range, range) * inv256;  // * (deltaHue ? 1 : -1);  // Наклон. "Мальчики" налево, "девочки" направо. :)
  if ((trackingObjectPosX[r] < 0.0f && trackingObjectSpeedX[r] < 0.0f) ||
      (trackingObjectPosX[r] > MAX_X && trackingObjectSpeedX[r] > 0.0f)) {       // меняем направление только после выхода за пределы экрана
    trackingObjectSpeedX[r] = -trackingObjectSpeedX[r];
  }

  // controls the leap height
  trackingObjectSpeedY[r] = (float)(random8() * 8U + HEIGHT * 10U) * inv256;
  trackingObjectHue[r] = random8();
  trackingObjectPosX[r] = random8(WIDTH);
}

static void popcornRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        setModeSettings(random8(9U) * 11U + 3U + random8(9U), 5U + random8(67U) * 2U + (random8(4U) ? 0U : 1U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette();

    speedfactor = remap(modes[currentMode].Speed, (uint8_t)1U, (uint8_t)255U, 0.25f, 1.0f);

    enlargedObjectNUM = (float)((modes[currentMode].Scale - 1U) % 11U) * inv10 * (float)(enlargedOBJECT_MAX_COUNT - 1U) + 1U;
    if (enlargedObjectNUM > enlargedOBJECT_MAX_COUNT) {
      enlargedObjectNUM = enlargedOBJECT_MAX_COUNT;
    }

    for (uint8_t r = 0U; r < enlargedObjectNUM; r++) {
      trackingObjectPosX[r] = random8(WIDTH);
      trackingObjectPosY[r] = random8(HEIGHT);
      trackingObjectSpeedX[r] = 0.0f;
      trackingObjectSpeedY[r] = -1.0f;
      trackingObjectHue[r] = random8();
    }

    loadingFlag = false;
  }

  float popcornGravity = 0.1f * speedfactor;
  fadeToBlackBy(leds, NUM_LEDS, 60);

  const float double_h_minus_2 = (float)(HEIGHT + HEIGHT - 2U);
  const bool is_speed_odd = (modes[currentMode].Speed & 0x01);

  for (uint8_t r = 0U; r < enlargedObjectNUM; r++) {
    // add the X & Y velocities to the positions
    trackingObjectPosX[r] += trackingObjectSpeedX[r];

    if (trackingObjectPosX[r] > MAX_X) {
      trackingObjectPosX[r] -= MAX_X;
    }
    if (trackingObjectPosX[r] < 0.0f) {
      trackingObjectPosX[r] += MAX_X;
    }

    trackingObjectPosY[r] += trackingObjectSpeedY[r] * speedfactor;

    if (trackingObjectPosY[r] > MAX_Y) {
      trackingObjectPosY[r] = double_h_minus_2 - trackingObjectPosY[r];
      trackingObjectSpeedY[r] = -trackingObjectSpeedY[r];
    }

    // bounce off the floor?
    if (trackingObjectPosY[r] < 0.0f && trackingObjectSpeedY[r] < -0.7f) {  // 0.7 вычислено в экселе. скорость свободного падения ниже этой не падает. если ниже, значит ещё есть ускорение
      trackingObjectSpeedY[r] = (-trackingObjectSpeedY[r]) * 0.9375f;       // V * 0.9375 это 15/16. Можно сделать (V * 15) / 16, или (V * 240) >> 8
      trackingObjectPosY[r] = -trackingObjectPosY[r];
    }

    // settled on the floor?
    if (trackingObjectPosY[r] <= -1.0f) {
      popcornRestart_rocket(r);
    }

    // popcornGravity
    trackingObjectSpeedY[r] -= popcornGravity;

    // viscosity
    trackingObjectSpeedX[r] *= 0.875f;
    trackingObjectSpeedY[r] *= 0.875f;

    // make the acme gray, because why not
    CRGB color;
    if (trackingObjectSpeedY[r] < 0.004f && trackingObjectSpeedY[r] > -0.004f) {
      color = is_speed_odd ? ColorFromPalette(*curPalette, trackingObjectHue[r]) : CRGB::Pink;
    } else {
      color = is_speed_odd ? CRGB::Gray : ColorFromPalette(*curPalette, trackingObjectHue[r]);
    }

    drawPixelXYF(trackingObjectPosX[r], trackingObjectPosY[r], color);
  }
}
#endif


#ifdef DEF_OSCILLATING
// ============= Эффект Реакция Белоусова-Жаботинского (Осциллятор) ===============
// по наводке https://www.wikiwand.com/ru/%D0%9A%D0%BB%D0%B5%D1%82%D0%BE%D1%87%D0%BD%D1%8B%D0%B9_%D0%B0%D0%B2%D1%82%D0%BE%D0%BC%D0%B0%D1%82
// (c) SottNick

/*
class oscillatingCell {
public:
  uint8_t red; // значения 0 или 1
  uint8_t blue; // значения 0 или 1
  uint8_t green; // значения 0 или 1
  uint8_t color; // значения от 0 до 2
};
oscillatingCell oscillatingWorld[WIDTH][HEIGHT];

будем использовать вместо них всех имеющийся в прошивке массив
static uint8_t noise3d[2][WIDTH][HEIGHT];
*/

static uint8_t calcNeighbours(uint8_t x, uint8_t y, uint8_t n) {
  // Вычисляем индексы соседей по X с быстрым зацикливанием краев
  const uint8_t left  = (x == 0U) ? MAX_X : (x - 1U);
  const uint8_t right = (x == MAX_X) ? 0U : (x + 1U);

  // Вычисляем индексы соседей по Y с быстрым зацикливанием краев
  const uint8_t top    = (y == 0U) ? MAX_Y : (y - 1U);
  const uint8_t bottom = (y == MAX_Y) ? 0U : (y + 1U);

  // Кешируем указатели на три строки двумерного среза массива шума
  const uint8_t* row_current = noise3d[0][x];
  const uint8_t* row_top     = noise3d[0][left];
  const uint8_t* row_bottom  = noise3d[0][right];

  // Считаем совпадения
  return (row_bottom[y]      == n) +  // (x + 1, y)
         (row_current[bottom] == n) + // (x, y + 1)
         (row_top[y]         == n) +  // (x - 1, y)
         (row_current[top]    == n) + // (x, y - 1)
         (row_bottom[bottom] == n) +  // (x + 1, y + 1)
         (row_top[bottom]    == n) +  // (x - 1, y + 1)
         (row_top[top]       == n) +  // (x - 1, y - 1)
         (row_bottom[top]    == n);   // (x + 1, y - 1)
}

static void oscillatingRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        uint8_t tmp = random8(6U); // 4 палитры по 6? (0, 1, 6, 7) + цвет + смена цвета
        if (tmp < 4U){
          if (tmp > 1U) tmp += 4U;
          tmp = tmp * 6U + 1U;
        }
        else if (tmp == 4U)
          tmp = 51U + random8(49U);
        else
          tmp = 100U;
        setModeSettings(tmp, 185U+random8(40U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    step = 0U;
    if (modes[currentMode].Scale > 100U) modes[currentMode].Scale = 100U; // чтобы не было проблем при прошивке без очистки памяти
    if (modes[currentMode].Scale <= 50U)
      curPalette = palette_arr[(uint8_t)(modes[currentMode].Scale / 50.0f * ((sizeof(palette_arr) / sizeof(TProgmemRGBPalette16 *)) - 0.01f))];

    //случайное заполнение
    for (uint8_t i = 0U; i < WIDTH; i++) {
      for (uint8_t j = 0U; j < HEIGHT; j++) {
        noise3d[1][i][j] = random8(3);
        noise3d[0][i][j] = noise3d[1][i][j];
      }
    }

    loadingFlag = false;
  }

  hue++;
  CRGB currColors[3];

  if (modes[currentMode].Scale == 100U) {
    currColors[0U] = CHSV(hue, 255U, 255U);
    currColors[1U] = CHSV(hue, 128U, 255U);
    currColors[2U] = CHSV(hue, 255U, 128U);
  } else if (modes[currentMode].Scale > 50U) {
    uint8_t calc_hue = (modes[currentMode].Scale - 50U) * 5.1f;
    currColors[0U] = CHSV(calc_hue, 255U, 255U);
    currColors[1U] = CHSV(calc_hue, 128U, 255U);
    currColors[2U] = CHSV(calc_hue, 255U, 128U);
  } else {
    for (uint8_t c = 0U; c < 3; c++) {
      currColors[c] = ColorFromPalette(*curPalette, c * 85U + hue);
    }
  }

  ledsClear(); // esphome: FastLED.clear();

  // расчёт химической реакции и отрисовка мира
  uint16_t colorCount[3] = {0U, 0U, 0U};

  for (uint8_t x = 0U; x < WIDTH; x++) {
    for (uint8_t y = 0U; y < HEIGHT; y++) {
      uint8_t current_state = noise3d[0][x][y];

      if (current_state == 0U) {
        colorCount[0U]++;
        if (calcNeighbours(x, y, 1U) > 2U)
          noise3d[1][x][y] = 1U;
      } else if (current_state == 1U) {
        colorCount[1U]++;
        if (calcNeighbours(x, y, 2U) > 2U)
          noise3d[1][x][y] = 2U;
      } else { // if (current_state == 2U){
        colorCount[2U]++;
        if (calcNeighbours(x, y, 0U) > 2U)
          noise3d[1][x][y] = 0U;
      }

      leds[XY(x, y)] = currColors[noise3d[1][x][y]];
    }
  }

  // проверка зацикливания
  if (colorCount[0] == deltaHue && colorCount[1] == deltaHue2 && colorCount[2] == deltaValue) {
    step++;
    if (step > 10U) {
      if (colorCount[0] < colorCount[1]) step = 0U; else step = 1U;
      if (colorCount[2] < colorCount[step]) step = 2U;
      colorCount[step] = 0U;
      step = 0U;
    }
  } else {
    step = 0U;
  }

  // вброс хаоса
  if (hue == hue2) { // чтобы не каждый ход
    hue2 += random8(220U) + 36U;
    uint8_t tx = random8(WIDTH);
    deltaHue = noise3d[1][tx][0U] + 1U;
    if (deltaHue > 2U) deltaHue = 0U;
    noise3d[1][tx][0U] = deltaHue;
    noise3d[1][(tx + 1U) % WIDTH][0U] = deltaHue;
    noise3d[1][(tx + 2U) % WIDTH][0U] = deltaHue;
  }

  deltaHue = colorCount[0];
  deltaHue2 = colorCount[1];
  deltaValue = colorCount[2];

  // вброс исчезнувшего цвета
  for (uint8_t c = 0U; c < 3; c++) {
    if (colorCount[c] < 6U) {
      uint8_t tx = random8(WIDTH);
      uint8_t ty = random8(HEIGHT);
      if (random8(2U)) {
        noise3d[1][tx][ty] = c;
        noise3d[1][(tx + 1U) % WIDTH][ty] = c;
        noise3d[1][(tx + 2U) % WIDTH][ty] = c;
      } else {
        noise3d[1][tx][ty] = c;
        noise3d[1][tx][(ty + 1U) % HEIGHT] = c;
        noise3d[1][tx][(ty + 2U) % HEIGHT] = c;
      }
    }
  }

  // перенос на следующий цикл
  for (uint8_t x = 0U; x < WIDTH; x++) {
    for (uint8_t y = 0U; y < HEIGHT; y++) {
      noise3d[0][x][y] = noise3d[1][x][y];
    }
  }
}
#endif


#ifdef DEF_FIRE_2020
// ============= Огонь 2020 ===============
// (c) SottNick
//сильно по мотивам https://pastebin.com/RG0QGzfK
//Perlin noise fire procedure by Yaroslaw Turbin
//https://www.reddit.com/r/FastLED/comments/hgu16i/my_fire_effect_implementation_based_on_perlin/

constexpr uint8_t SPARKLES_NUM = OCTANT_X;   // не более чем  enlargedOBJECT_MAX_COUNT (WIDTH * 2)

// float   trackingObjectPosX[SPARKLES_NUM]; // это для искорок. по идее должны быть uint8_t, но были только такие
// float   trackingObjectPosY[SPARKLES_NUM];
// uint8_t shiftHue[HEIGHT];
// uint16_t ff_y, ff_z;                      // используем для сдвига нойза переменные из общих
// uint8_t deltaValue;

static void fire2020Routine2(){
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        setModeSettings(1U + random8(100U), 195U + random8(40U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    if (modes[currentMode].Scale > 100U) modes[currentMode].Scale = 100U;  // чтобы не было проблем при прошивке без очистки памяти
    deltaValue = modes[currentMode].Scale * 0.0899f;
    if (deltaValue == 3U ||deltaValue == 4U)
      curPalette =  palette_arr[deltaValue];
    else
      curPalette = firePalettes[deltaValue];

    deltaValue = (((modes[currentMode].Scale - 1U) % 11U + 1U) << 4U) - 8U; // ширина языков пламени (масштаб шума Перлина)
    deltaHue = map(deltaValue, 8U, 168U, 8U, 84U);                          // высота языков пламени должна уменьшаться не так быстро, как ширина
    step = map(255U - deltaValue, 87U, 247U, 4U, 32U);                      // вероятность смещения искорки по оси ИКС

    const float inv_max_h = 255.0f / MAX_Y;
    for (uint8_t j = 0U; j < HEIGHT; j++) {
      shiftHue[j] = (uint8_t)((float)(MAX_Y - j) * inv_max_h);              // init colorfade table
    }

    for (uint8_t i = 0U; i < SPARKLES_NUM; i++) {
      trackingObjectPosY[i] = random8(HEIGHT);
      trackingObjectPosX[i] = random8(WIDTH);
    }

    loadingFlag = false;
  }

  // Рендеринг пламени на базе шума Перлина
  for (uint8_t i = 0U; i < WIDTH; i++) {
    const uint16_t noise_x = i * deltaValue;

    for (uint8_t j = 0U; j < HEIGHT; j++) {
      uint16_t noise_y = (j + ff_y + random8(2U)) * deltaHue;

      uint8_t raw_noise = fastled_helper::perlin8(noise_x, noise_y, ff_z);
      uint8_t color_index = qsub8(raw_noise, shiftHue[j]);
      CRGB fire_color = ColorFromPalette(*curPalette, color_index, 255U);

      nblend(leds[XY(i, MAX_Y - j)], fire_color, 160U);
    }
  }

  // вставляем искорки из отдельного массива
  for (uint8_t i = 0U; i < SPARKLES_NUM; i++) {
    if (trackingObjectPosY[i] > 3U){
      leds[XY(trackingObjectPosX[i], trackingObjectPosY[i])] = leds[XY(trackingObjectPosX[i], 3U)];
      leds[XY(trackingObjectPosX[i], trackingObjectPosY[i])].fadeToBlackBy(trackingObjectPosY[i]*2U);
    }
    trackingObjectPosY[i]++;
    if (trackingObjectPosY[i] >= HEIGHT){
      trackingObjectPosY[i] = random8(4U);
      trackingObjectPosX[i] = random8(WIDTH);
    }
    if (!random8(step))
      trackingObjectPosX[i] = (WIDTH + (uint8_t)trackingObjectPosX[i] + 1U - random8(3U)) % WIDTH;
  }

  for (uint8_t i = 0U; i < SPARKLES_NUM; i++) {
    uint8_t px = (uint8_t)trackingObjectPosX[i];
    uint8_t py = (uint8_t)trackingObjectPosY[i];

    if (py > 3U) {
      leds[XY(px, py)] = leds[XY(px, 3U)];
      leds[XY(px, py)].fadeToBlackBy(py << 1U);
    }

    py++;
    if (py >= HEIGHT) {
      py = random8(4U);
      px = random8(WIDTH);
    } else {
      if (random8(step) == 0U) {
        uint8_t rnd = random8(3U);
        if (rnd == 0U) {
          if (px == 0U) px = MAX_X; else px--;
        } else if (rnd == 1U) {
          if (px >= MAX_X) px = 0U; else px++;
        }
        // при rnd == 2U координата px остается без изменений (+1 - 2 = -1, +1 - 1 = 0, +1 - 0 = +1)
      }
    }

    trackingObjectPosX[i] = px;
    trackingObjectPosY[i] = py;
  }

  ff_y++;
  if (ff_y & 0x01) {
    ff_z++;
  }
}
#endif


#ifdef DEF_LLAND
// ============= Эффект Кипение ===============
// (c) SottNick
// по мотивам LDIRKO Ленд - эффект номер 10
// ...ldir... Yaroslaw Turbin, 18.11.2020
// https://vk.com/ldirko
// https://www.reddit.com/user/ldirko/

static void LLandRoutine(){
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        uint8_t tmp = random8(6U);
        if (tmp > 1U) tmp += 3U;
        tmp = tmp * 11U + 4U + random8(8U);
        if (tmp > 97U) tmp = 94U;
        setModeSettings(tmp, 200U + random8(46U));  // масштаб 4-11, палитры 0, 1, 5, 6, 7, 8 (кроме 2, 3, 4)
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette();

    deltaValue = 10U * ((modes[currentMode].Scale - 1U) % 11U + 1U);  // значения от 1 до 11

    loadingFlag = false;
  }

  hue2 += 32U;
  if (hue2 < 32U)
    hue++;
  ff_y += 16U;

  const float inv_max_h = 255.0f / MAX_Y;

  for (uint8_t y = 0U; y < HEIGHT; y++) {
    const uint16_t noise_y = y * deltaValue - ff_y;
    const uint8_t height_fade = (uint8_t)((float)y * inv_max_h);

    for (uint16_t x = 0U; x < WIDTH; x++) {
      const uint16_t noise_x = x * deltaValue;

      // Генерация базового шума
      uint8_t raw_noise = fastled_helper::perlin8(noise_x, noise_y, ff_z);

      // Вычитание градиента высоты
      uint8_t value = raw_noise - height_fade;

      // Быстрая замена map(value, 0, 255, 205, 255) -> scale8(value, 50) + 205
      uint8_t color_index = scale8(value, 50U) + 205U + hue;

      drawPixelXY(x, y, ColorFromPalette(*curPalette, color_index, 255U));
    }
  }

  ff_z++;
}
#endif


#ifdef DEF_ATTRACT
// ============= ЭФФЕКТ ПРИТЯЖЕНИЕ ===============
// https://github.com/pixelmatix/aurora/blob/master/PatternAttract.h
// Адаптация (c) SottNick

// используются переменные эффекта Стая. Без него работать не будет.
// #define ASTEROIDS_NUM 5U // количество шариков не должно превышать AVAILABLE_BOID_COUNT = 20U;

static void attractRoutine() {
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        uint8_t tmp = random8(8U);
        if (tmp > 3U) tmp++;
        setModeSettings(tmp * 11U + 3U + random8(9U), 180U + random8(56U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette();

    enlargedObjectNUM = (modes[currentMode].Scale - 1U) % 11U + 1U;

    for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
      boids[i] = Boid(random8(WIDTH), random8(HEIGHT));
      boids[i].mass = (float)random8(33U, 134U) * inv100;
      boids[i].velocity.x = (float)random8(46U, 100U) * inv500; // 1 / 500
      if (random8(2U)) {
        boids[i].velocity.x = -boids[i].velocity.x;
      }
      boids[i].velocity.y = 0.0f;
      boids[i].colorIndex = random8();
    }

    loadingFlag = false;
  }

  dimAll(220);

  PVector attractLocation = PVector(CENTER_X_F, CENTER_Y_F);
  const float max_d = (float)HEIGHT * 2.0f;

  for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
    Boid &boid = boids[i];

    PVector force = attractLocation - boid.location;    // Calculate direction of force // и вкорячиваем сюда регулировку скорости
    float d = force.mag();                              // Distance between objects
    d = constrain(d, 5.0f, max_d);                      // Limiting the distance to eliminate "extreme" results for very close or very far objects
    force.normalize();                                  // Normalize vector (distance doesn't matter here, we just want this vector for direction)
    float strength = (5.0f * boid.mass) / (d * d);      // Calculate gravitional force magnitude 5.=attractG*attractMass
    force *= strength;                                  // Get force vector --> magnitude * direction

    boid.applyForce(force);
    boid.update();

    drawPixelXYF(boid.location.x, boid.location.y, ColorFromPalette(*curPalette, boid.colorIndex + hue));
  }

  EVERY_N_MILLIS(200) {
    hue++;
  }
}
#endif


#ifdef DEF_DROPS
// ============= ЭФФЕКТ Капли на стекле ===============
// https://github.com/DmytroKorniienko/FireLamp_JeeUI/blob/master/src/effects.cpp
static void newMatrixRoutine()
{
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(random8(30U) ? (random8(40U) ? 2U + random8(99U) : 1U) : 100U, 12U + random8(68U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette();

    enlargedObjectNUM = map(modes[currentMode].Speed, 1, 255, 1, trackingOBJECT_MAX_COUNT);
    speedfactor = 0.136f; // фиксируем хорошую скорость

    for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = random8(HEIGHT);
      trackingObjectSpeedY[i] = (float)random8(150U, 250U) * inv100;
      trackingObjectState[i] = random8(127U, 255U);
    }
    hue = modes[currentMode].Scale * 2.55f;

    loadingFlag = false;
  }

  dimAll(246); // для фиксированной скорости

  constexpr uint8_t GLUK = 20U; // Вероятность горизонтального сдвига капли

  CHSV color;
  const uint8_t spawn_min_y = HEIGHT - HEIGHT / 2U;

  for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
    trackingObjectPosY[i] -= trackingObjectSpeedY[i]*speedfactor;

    if (modes[currentMode].Scale == 100U) {
      color = rgb2hsv_approximate(CRGB::Gray);
      color.val = trackingObjectState[i];
    } else if (modes[currentMode].Scale == 1U) {
      color.hue = ++hue;
      color.sat = 255U;
      color.val = trackingObjectState[i];
    } else {
      color.hue = hue;
      color.sat = 255U;
      color.val = trackingObjectState[i];
    }

    drawPixelXYF(trackingObjectPosX[i], trackingObjectPosY[i], color);

    // Случайный горизонтальный сдвиг капли («глюк») без тяжелого оператора %
    if (random8() < GLUK) {
      uint8_t px = (uint8_t)trackingObjectPosX[i];
      uint8_t rnd = random8(3U);
      if (rnd == 0U) {
        if (px == 0U) px = MAX_X; else px--;
      } else if (rnd == 1U) {
        if (px >= MAX_X) px = 0U; else px++;
      }
      trackingObjectPosX[i] = px;
      trackingObjectState[i] = random8(196U, 255U);
    }

    // Возврат капли наверх матрицы, если она улетела вниз
    if (trackingObjectPosY[i] < -1.0f) {
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = random8(spawn_min_y, HEIGHT);
      trackingObjectSpeedY[i] = (float)random8(150U, 250U) * inv100;
      trackingObjectState[i] = random8(127U, 255U);
    }
  }
}
#endif


#ifdef DEF_SMOKEBALLS
//-------- Эффект Дымовые шашки ----------- aka "Детские сны"
// (c) Stepko
// https://editor.soulmatelights.com/gallery/505
// https://github.com/DmytroKorniienko/FireLamp_JeeUI/blob/master/src/effects.cpp

static void smokeballsRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(random8(9U) * 11U + 3U + random8(9U), 1U + random8(255U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette();

    enlargedObjectNUM = enlargedObjectNUM = (modes[currentMode].Scale - 1U) % 11U + 1U;
    speedfactor = remap(modes[currentMode].Speed, (uint8_t)1, (uint8_t)255, 0.02f, 0.1f); // попробовал разные способы управления скоростью. Этот максимально приемлемый, хотя и сильно тупой.

    for (uint8_t j = 0U; j < enlargedObjectNUM; j++) {
      trackingObjectShift[j]  = random16((WIDTH * 10) - (THIRD_X * 20));  // сумма trackingObjectState + trackingObjectShift не должна выскакивать за макс.Х
      trackingObjectSpeedX[j] = (float)random16(25U, 80 * WIDTH) * inv5;  // / 5.0f;
      trackingObjectState[j]  = random16(CENTER_X * 10, THIRD_X * 20);
      trackingObjectHue[j]    = random8();
      trackingObjectPosX[j]   = trackingObjectShift[j];
    }

    loadingFlag = false;
  }

  static float subPixelAccumulator = 0.0f;
  subPixelAccumulator += speedfactor;
  if (subPixelAccumulator >= 1.0f) {
    shiftUp();
    subPixelAccumulator -= 1.0f; // Сбрасываем накопленный пиксель
  }

  constexpr uint8_t fade_val = 128U / HEIGHT;
  fadeToBlackBy(leds, NUM_LEDS, fade_val);

  if (modes[currentMode].Speed & 0x01) {
    blurScreen(20);
  }

  for (uint8_t j = 0U; j < enlargedObjectNUM; j++) {
    uint16_t phase = (uint16_t)trackingObjectHue[j] << 8U;
    uint16_t b_speed = (uint16_t)(trackingObjectSpeedX[j] * (speedfactor * 5.0f));

    trackingObjectPosX[j] = beatsin16(b_speed, trackingObjectShift[j], trackingObjectState[j] + trackingObjectShift[j], phase, trackingObjectHue[j] * 8U);
    drawPixelXYF(trackingObjectPosX[j] * inv10, 0.05f, ColorFromPalette(*curPalette, trackingObjectHue[j]));
  }

  // Таймер мутации шаров
  EVERY_N_SECONDS(20U) {
    for (uint8_t j = 0U; j < enlargedObjectNUM; j++) {
      trackingObjectShift[j] += (int16_t)random8(40U) - 20;  // random(-20, 20);
      trackingObjectHue[j] += 28U;
    }
  }

  loadingFlag = (random8() > 253U);
}
#endif


#ifdef DEF_NEXUS
// ------------- Nexus --------------
// (c) kostyamat
// https://github.com/DmytroKorniienko/FireLamp_JeeUI/blob/master/src/effects.cpp

// #define enlargedOBJECT_MAX_COUNT            (WIDTH * 2)   // максимальное количество червяков
// uint8_t enlargedObjectNUM;                                // выбранное количество червяков
// float trackingObjectPosX[trackingOBJECT_MAX_COUNT];       // тут будет позиция головы
// float trackingObjectPosY[trackingOBJECT_MAX_COUNT];       // тут будет позиция головы
// float trackingObjectSpeedX[trackingOBJECT_MAX_COUNT];     // тут будет скорость червяка
// uint8_t trackingObjectHue[trackingOBJECT_MAX_COUNT];      // тут будет цвет червяка
// uint8_t trackingObjectState[trackingOBJECT_MAX_COUNT];    // тут будет направление червяка

static void nexusReset(uint8_t i) {
  constexpr float inv70 = 0.0142857f;  // 1.0f / 70.0f

  trackingObjectHue[i] = random8();
  trackingObjectState[i] = random8(4U);

  trackingObjectSpeedX[i] = (float)random8(5U, 11U) * inv70 + speedfactor;  // делаем частицам немного разное ускорение и сразу пересчитываем под общую скорость

  switch (trackingObjectState[i]) {
    case 0b01:
      trackingObjectPosY[i] = (float)HEIGHT;
      trackingObjectPosX[i] = random8(WIDTH);
      break;
    case 0b00:
      trackingObjectPosY[i] = -1.0f;
      trackingObjectPosX[i] = random8(WIDTH);
      break;
    case 0b10:
      trackingObjectPosX[i] = (float)WIDTH;
      trackingObjectPosY[i] = random8(HEIGHT);
      break;
    case 0b11:
      trackingObjectPosX[i] = -1.0f;
      trackingObjectPosY[i] = random8(HEIGHT);
      break;
  }
}

static void nexusRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        setModeSettings(random8(2U) ? 11U + random8(15U) : 26U + random8(55U), 1U + random8(161U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    speedfactor = remap(modes[currentMode].Speed, (uint8_t)1, (uint8_t)255, 0.1f, 0.33f);

    enlargedObjectNUM = (modes[currentMode].Scale - 1U) / 99.0f * (enlargedOBJECT_MAX_COUNT - 1U) + 1U;
    if (enlargedObjectNUM > enlargedOBJECT_MAX_COUNT) enlargedObjectNUM = enlargedOBJECT_MAX_COUNT;

    constexpr float inv70 = 0.0142857f;  // 1.0f / 70.0f
    for (uint8_t i = 0U; i < enlargedObjectNUM; i++){
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = random8(HEIGHT);
      trackingObjectSpeedX[i] = (float)random8(5, 11) * inv70 + speedfactor; // делаем частицам немного разное ускорение и сразу пересчитываем под общую скорость
      trackingObjectHue[i] = random8();
      trackingObjectState[i] = random8(4U); //     B00           // задаем направление
                                            // B10     B11
                                            //     B01
    }
    deltaValue = 255U - map(modes[currentMode].Speed, 1, 255, 11, 33);

    loadingFlag = false;
  }

  dimAll(deltaValue);

  CHSV color;
  color.sat = 255U;
  color.val = 255U;

  for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
    switch (trackingObjectState[i]) {
      case 0b01:
        trackingObjectPosY[i] -= trackingObjectSpeedX[i];
        if (trackingObjectPosY[i] <= -1.0f)
          nexusReset(i);
        break;
      case 0b00:
        trackingObjectPosY[i] += trackingObjectSpeedX[i];
        if (trackingObjectPosY[i] >= (float)HEIGHT)
          nexusReset(i);
        break;
      case 0b10:
        trackingObjectPosX[i] -= trackingObjectSpeedX[i];
        if (trackingObjectPosX[i] <= -1.0f)
          nexusReset(i);
        break;
      case 0b11:
        trackingObjectPosX[i] += trackingObjectSpeedX[i];
        if (trackingObjectPosX[i] >= (float)WIDTH)
          nexusReset(i);
        break;
    }

    color.hue = trackingObjectHue[i];
    drawPixelXYF(trackingObjectPosX[i], trackingObjectPosY[i], color);
  }
}
#endif


#ifdef DEF_PACIFIC
// ------------ Эффект "Тихий Океан"
//  "Pacifica" перенос кода kostyamat
//  Gentle, blue-green ocean waves.
//  December 2019, Mark Kriegsman and Mary Corey March.
//  For Dan.
// https://raw.githubusercontent.com/FastLED/FastLED/master/examples/Pacifica/Pacifica.ino
// https://github.com/DmytroKorniienko/FireLamp_JeeUI/blob/master/src/effects.cpp

// Add one layer of waves into the led array
// Добавление одного слоя волн в массив светодиодов
static void pacifica_one_layer(CRGB *leds_ptr, const TProgmemRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff) {
  uint16_t ci = cistart;
  uint16_t waveangle = ioff;

  const uint16_t wavescale_half = (wavescale >> 1U) + 20U;
  for(uint16_t i = 0U; i < NUM_LEDS; i++) {
    waveangle += 250U;
    uint16_t s16 = sin16(waveangle) + 32768U;
    uint16_t cs  = scale16(s16, wavescale_half) + wavescale_half;
    ci += cs;

    uint16_t sindex16 = sin16(ci) + 32768U;
    uint8_t  sindex8  = scale16(sindex16, 240U);

    leds_ptr[i] += ColorFromPalette(p, sindex8, bri, LINEARBLEND);
  }
}

// Add extra 'white' to areas where the four layers of light have lined up brightly
// Добавление белых гребней волн в местах сильной яркости
static void pacifica_add_whitecaps(CRGB *leds_ptr) {
  const uint8_t basethreshold = beatsin8(9U, 55U, 65U);
  uint8_t wave = beat8(7U);

  for(uint16_t i = 0U; i < NUM_LEDS; i++) {
    uint8_t threshold = scale8(sin8(wave), 20U) + basethreshold;
    wave += 7U;

    uint8_t l = leds_ptr[i].getAverageLight();
    if(l > threshold) {
      uint8_t overage = l - threshold;
      uint8_t overage2 = qadd8(overage, overage);

      leds_ptr[i].r = qadd8(leds_ptr[i].r, overage);
      leds_ptr[i].g = qadd8(leds_ptr[i].g, overage2);
      leds_ptr[i].b = qadd8(leds_ptr[i].b, qadd8(overage2, overage2));
    }
  }
}

// Deepen the blues and greens
// Углубление синих и зеленых оттенков океана
static void pacifica_deepen_colors(CRGB *leds_ptr) {
  for(uint16_t i = 0U; i < NUM_LEDS; i++) {
    leds_ptr[i].blue  = scale8(leds_ptr[i].blue,  145U);
    leds_ptr[i].green = scale8(leds_ptr[i].green, 200U);

    leds_ptr[i].r |= 2U;
    leds_ptr[i].g |= 5U;
    leds_ptr[i].b |= 7U;
  }
}

static void pacificRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        setModeSettings(100U, 1U + random8(255U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
  }

  // Increment the four "color index start" counters, one for each wave layer.
  // Each is incremented at a different speed, and the speeds vary over time.
  static uint16_t sCIStart1, sCIStart2, sCIStart3, sCIStart4;
  static uint32_t sLastms = 0U;

  const uint32_t ms = millis();
  const uint32_t deltams = ms - sLastms;
  sLastms = ms;

  const uint16_t speedfactor1 = beatsin16(3U, 179U, 269U);
  const uint16_t speedfactor2 = beatsin16(4U, 179U, 269U);

  const long speed_divider = map(modes[currentMode].Speed, 1, 255, 620, 60);
  const uint32_t deltams1 = (deltams * speedfactor1) / map(modes[currentMode].Speed, 1, 255, 620, 60);
  const uint32_t deltams2 = (deltams * speedfactor2) / map(modes[currentMode].Speed, 1, 255, 620, 60);
  const uint32_t deltams21 = (deltams1 + deltams2) / 2;

  sCIStart1 += (deltams1 * beatsin88(1011U, 10U, 13U));
  sCIStart2 -= (deltams21 * beatsin88(777U, 8U, 11U));
  sCIStart3 -= (deltams1 * beatsin88(501U, 5U, 7U));
  sCIStart4 -= (deltams2 * beatsin88(257U, 4U, 6U));

  // Clear out the LED array to a dim background blue-green
  fill_solid(leds, NUM_LEDS, CRGB(2U, 6U, 10U));

  // Render each of four layers, with different scales and speeds, that vary over time
  pacifica_one_layer(leds, pacifica_palette_1, sCIStart1, beatsin16(3U, 11 * 256, 14 * 256), beatsin8(10U, 70U, 130U), 0U - beat16(301U));
  pacifica_one_layer(leds, pacifica_palette_2, sCIStart2, beatsin16(4U,  6 * 256,  9 * 256), beatsin8(17U, 40U,  80U), beat16(401U));
  pacifica_one_layer(leds, pacifica_palette_3, sCIStart3, 6 * 256, beatsin8(9U, 10U, 38U), 0U - beat16(503U));
  pacifica_one_layer(leds, pacifica_palette_3, sCIStart4, 5 * 256, beatsin8(8U, 10U, 28U), beat16(601U));

  // Add brighter 'whitecaps' where the waves lines up more
  pacifica_add_whitecaps(leds);

  // Deepen the blues and greens a bit
  pacifica_deepen_colors(leds);

  blurScreen(20U);
}
#endif


#if defined(DEF_FOUNTAIN) || defined(DEF_FAIRY)
//-------- по мотивам Эффектов Particle System -------------------------
// https://github.com/fuse314/arduino-particle-sys
// https://github.com/giladaya/arduino-particle-sys
// https://www.youtube.com/watch?v=S6novCRlHV8&t=51s
// #include <ParticleSys.h>
// при попытке вытащить из этой библиотеки только минимально необходимое выяснилось, что там очередной (третий) вариант реализации субпиксельной графики.
// ну его нафиг. лучше будет повторить визуал имеющимися в прошивке средствами.

static void particlesUpdate2(uint8_t i) {
  // Плавный учет времени жизни (TTL) с использованием speedfactor
  trackingObjectShift[i] += speedfactor;
  if (trackingObjectShift[i] >= 1.0f) {
    uint8_t decrement = (uint8_t)trackingObjectShift[i];
    trackingObjectShift[i] -= (float)decrement;

    if (trackingObjectState[i] > decrement) {
      trackingObjectState[i] -= decrement;
    } else {
      trackingObjectState[i] = 0U;
    }
  }

  // Смещение координат с учетом скорости кадра
  trackingObjectPosX[i] += trackingObjectSpeedX[i] * speedfactor;
  trackingObjectPosY[i] += trackingObjectSpeedY[i] * speedfactor;

  if (trackingObjectState[i] == 0U ||
      trackingObjectPosX[i] < 0.0f || trackingObjectPosX[i] >= (float)WIDTH ||
      trackingObjectPosY[i] < 0.0f || trackingObjectPosY[i] >= (float)HEIGHT) {
    trackingObjectIsShift[i] = false;
  }
}
#endif


#ifdef DEF_FOUNTAIN
// ============= ЭФФЕКТ ИСТОЧНИК ===============
// (c) SottNick
// выглядит как https://github.com/fuse314/arduino-particle-sys/blob/master/examples/StarfieldFastLED/StarfieldFastLED.ino

static void starfield2Emit(uint8_t i) {
  if (hue++ & 0x01)
    hue2++;  //counter++;

  speedfactor = remap(modes[currentMode].Speed, (uint8_t)1U, (uint8_t)255U, 0.02f, 0.25f);

  trackingObjectPosX[i] = CENTER_X_F;  // CENTER_X_MINOR; // * RENDERER_RESOLUTION; // particle->x = source->x;
  trackingObjectPosY[i] = CENTER_Y_F;  // CENTER_Y_MINOR; // * RENDERER_RESOLUTION; // particle->y = source->y;

  constexpr float inv512 = 0.001953125f;
  trackingObjectSpeedX[i] = ((float)random8() - 127.0f) * inv512;                                      // random(_hVar) - _constVel; // particle->vx
  trackingObjectSpeedY[i] = SQRT_VARIANT(0.0626f - trackingObjectSpeedX[i] * trackingObjectSpeedX[i]); // SQRT_VARIANT(pow(_constVel, 2) - pow(trackingObjectSpeedX[i], 2));  // particle -> vy зависит от particle -> vx - не ошибка
  if(random8(2U)) {
    trackingObjectSpeedY[i]= -trackingObjectSpeedY[i];
  }
  trackingObjectState[i] = random8(50U, 250U);                                                         // random8(minLife, maxLife); // particle -> ttl
  if (modes[currentMode].Speed & 0x01)
    trackingObjectHue[i] = hue2;                                                                       // (counter / 2) % 255; // particle->hue
  else
    trackingObjectHue[i] = random8();
  trackingObjectIsShift[i] = true;                                                                     // particle -> isAlive
}

static void starfield2Routine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        setModeSettings(25U + random8(76U), 185U + random8(30U)*2U + (random8(6U) ? 0U : 1U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    // enlargedObjectNUM = (modes[currentMode].Scale - 1U) / 99.0f * (trackingOBJECT_MAX_COUNT - 1U) + 1U;
    enlargedObjectNUM = (float)(modes[currentMode].Scale - 1U) * inv99 * (float)(trackingOBJECT_MAX_COUNT - 1U) + 1U;
    if (enlargedObjectNUM > trackingOBJECT_MAX_COUNT) {
      enlargedObjectNUM = trackingOBJECT_MAX_COUNT;
    }

    deltaValue = enlargedObjectNUM / (SQRT_VARIANT(CENTER_X_MAJOR * CENTER_X_MAJOR + CENTER_Y_MAJOR * CENTER_Y_MAJOR) * 4U) + 1U;  // 4 - это потому что за 1 цикл частица пролетает ровно четверть расстояния между 2мя соседними пикселями
    for(uint8_t i = 0U; i < enlargedObjectNUM; i++) {
      trackingObjectIsShift[i] = false; // particle->isAlive
    }

    loadingFlag = false;
  }

  step = deltaValue; // счётчик количества частиц в очереди на зарождение в этом цикле
  dimAll(127);

  CHSV hsv_color;
  hsv_color.sat = 255U;
  hsv_color.val = 255U;

  // go over particles and update matrix cells on the way
  for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
    if (!trackingObjectIsShift[i] && step) {
      //emitter->emit(&particles[i], this->g);
      starfield2Emit(i);
      step--;
    }

    if (trackingObjectIsShift[i]){ // particle->isAlive
      particlesUpdate2(i);

      // generate RGB values for particle
      hsv_color.hue = trackingObjectHue[i];
      CRGB baseRGB;
      hsv2rgb_spectrum(hsv_color, baseRGB);

      // Плавное угасание яркости звезды в зависимости от её TTL
      baseRGB.nscale8(trackingObjectState[i]);

      drawPixelXYF(trackingObjectPosX[i], trackingObjectPosY[i], baseRGB);
    }
  }
}
#endif


#ifdef DEF_FAIRY
// ============= ЭФФЕКТ ФЕЯ ===============
// (c) SottNick

#define FAIRY_BEHAVIOR // типа сложное поведение

static void fairyEmit(uint8_t i) {
  if (deltaHue++ & 0x01) {
    if (hue++ & 0x01) {
      hue2++;
    }
  }

  trackingObjectPosX[i] = boids[0].location.x;
  trackingObjectPosY[i] = boids[0].location.y;

  constexpr float inv512 = 0.001953125f;  // 1.0f / 512.0f
  trackingObjectSpeedX[i] = ((float)random8() - 127.0f) * inv512;                                        // particle->vx

  // Круговая тригонометрия шлейфа
  trackingObjectSpeedY[i] = SQRT_VARIANT(0.0626f - trackingObjectSpeedX[i] * trackingObjectSpeedX[i]);   // particle->vy зависит от particle->vx - не ошибка
  if (random8(2U)) {
    trackingObjectSpeedY[i] = -trackingObjectSpeedY[i];
  }

  trackingObjectState[i] = random8(20U, 80U);                                                            // particle->ttl
  trackingObjectHue[i] = hue2;                                                                           // particle->hue
  trackingObjectIsShift[i] = true;                                                                       // particle->isAlive
}


static void fairyRoutine(){
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        setModeSettings(14U + random8(87U), 190U + random8(40U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    speedfactor = remap(modes[currentMode].Speed, (uint8_t)1U, (uint8_t)255U, 0.02f, 0.25f);

    deltaValue = 10U; // количество зарождающихся частиц за 1 цикл // perCycle = 1;

    // enlargedObjectNUM = (modes[currentMode].Scale - 1U) / 99.0f * (trackingOBJECT_MAX_COUNT - 1U) + 1U;
    enlargedObjectNUM = (float)(modes[currentMode].Scale - 1U) * inv99 * (float)(trackingOBJECT_MAX_COUNT - 1U) + 1U;
    if (enlargedObjectNUM > trackingOBJECT_MAX_COUNT) {
      enlargedObjectNUM = trackingOBJECT_MAX_COUNT;
    }

    for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
      trackingObjectIsShift[i] = false; // particle->isAlive
    }

    // лень было придумывать алгоритм для траектории феи, поэтому это будет нулевой "бойд" из эффекта Притяжение
    boids[0] = Boid(random8(WIDTH), random8(HEIGHT));
    boids[0].mass = 0.5f;  // сюда можно поставить регулятор разлёта. чем меньше число, тем дальше от центра будет вылет
    boids[0].velocity.x = (float)random8(46U, 100U) * inv500;
    if (random8(2U)) boids[0].velocity.x = -boids[0].velocity.x;
    boids[0].velocity.y = 0.0f;

    hue = random8();
    #ifdef FAIRY_BEHAVIOR
      deltaHue2 = 1U;
    #endif

    loadingFlag = false;
  }

  step = deltaValue; //счётчик количества частиц в очереди на зарождение в этом цикле

#ifdef FAIRY_BEHAVIOR
  if (!deltaHue && deltaHue2 && std::abs(boids[0].velocity.x) + std::abs(boids[0].velocity.y) < 0.15f) {
    deltaHue2 = 0U;

    // Быстрое умножение вместо делений (1/4080 и 1/2040)
    boids[1].velocity.x = ((float)random8() + 255.0f) * inv4080;  //  / 4080.0f;
    boids[1].velocity.y = ((float)random8() + 255.0f) * inv2040;  //  / 2040.0f;

    if (boids[0].location.x > CENTER_X_F) boids[1].velocity.x = -boids[1].velocity.x;
    if (boids[0].location.y > CENTER_Y_F) boids[1].velocity.y = -boids[1].velocity.y;
  }

  if (!deltaHue2) {
    step = 1U;

    // Умножаем смещение на speedfactor
    boids[0].location.x += boids[1].velocity.x * speedfactor;
    boids[0].location.y += boids[1].velocity.y * speedfactor;

    deltaHue2 = (boids[0].location.x <= 0.0f || boids[0].location.x >= MAX_X ||
                 boids[0].location.y <= 0.0f || boids[0].location.y >= MAX_Y);
  } else
#endif // FAIRY_BEHAVIOR
  {
    PVector attractLocation = PVector(CENTER_X_F, CENTER_Y_F);
    // float attractMass = 10;
    // float attractG = .5;
    // перемножаем и получаем 5.

    Boid &boid = boids[0];

    PVector force = attractLocation - boid.location;      // Calculate direction of force
    float d = force.mag();                                // Distance between objects
    d = constrain(d, 5.0f, (float)HEIGHT);                // Limiting the distance to eliminate "extreme" results for very close or very far objects
                                                          // видео снято на 5.0f

    force.normalize();                                    // Normalize vector (distance doesn't matter here, we just want this vector for direction)
    float strength = (5.0f * boid.mass) / (d * d);        // Calculate gravitional force magnitude 5.=attractG*attractMass
    force *= strength;                                    // Get force vector --> magnitude * direction

    boid.applyForce(force);
    boid.update();

    if (boid.location.x <= -1.0f) {
      boid.location.x = -boid.location.x;
    } else if (boid.location.x >= (float)WIDTH) {
      boid.location.x = -boid.location.x + (float)WIDTH + (float)WIDTH;
    }

    if (boid.location.y <= -1.0f) {
      boid.location.y = -boid.location.y;
    } else if (boid.location.y >= (float)HEIGHT) {
      boid.location.y = -boid.location.y + (float)HEIGHT + (float)HEIGHT;
    }

    if (!deltaHue) {
      if (random8(3U)) {
        d = ((random8(2U)) ? boid.velocity.x : boid.velocity.y) * ((random8(2U)) ? 0.2f : -0.2f);
        boid.velocity.x += d;
        boid.velocity.y -= d;
      } else {
        if (std::abs(boid.velocity.x) < 0.02f) {
          boid.velocity.x = -boid.velocity.x;
        } else if (std::abs(boid.velocity.y) < 0.02f) {
          boid.velocity.y = -boid.velocity.y;
        }
      }
    }
  }

  // dimAll(255 - 128 / 0.25f * speedfactor); очередной эффект, к которому нужно будет "подобрать коэффициенты"
  dimAll(127);

  CHSV hsv_color;
  hsv_color.sat = 255U;
  hsv_color.val = 255U;

  // go over particles and update matrix cells on the way
  for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
    if (!trackingObjectIsShift[i] && step) {
      fairyEmit(i);
      step--;
    }

    if (trackingObjectIsShift[i]) {  // particle->isAlive
      // Гравитация частиц с учетом скорости кадра
      if (modes[currentMode].Scale & 0x01 && trackingObjectSpeedY[i] > -1.0f) {
        trackingObjectSpeedY[i] -= 0.05f * speedfactor;  // apply acceleration
      }

      particlesUpdate2(i);

      // generate RGB values for particle
      hsv_color.hue = trackingObjectHue[i];
      CRGB baseRGB;
      hsv2rgb_spectrum(hsv_color, baseRGB);

      baseRGB.nscale8(trackingObjectState[i]);
      drawPixelXYF(trackingObjectPosX[i], trackingObjectPosY[i], baseRGB);
    }
  }

  // Отрисовка самой феи
  drawPixelXYF(boids[0].location.x, boids[0].location.y, CHSV(hue, 160U, 255U));
}
#endif


#ifdef DEF_SAND
// ============= Эффект Цветные драже ===============
// (c) SottNick
// по мотивам визуала эффекта by Yaroslaw Turbin 14.12.2020
// https://vk.com/ldirko программный код которого он запретил брать

static void sandRoutine(){
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(1U + random8(100U) , 140U + random8(100U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    pcnt = 0U; // = HEIGHT;

    loadingFlag = false;
  }

  // Разрушение/осыпание излишков песка (если насыпалось выше порога)
  // если насыпалось уже достаточно, бахаем рандомные песчинки
  uint8_t temp = map8(random8(), modes[currentMode].Scale * 2.55f, 255U);
  if (pcnt >= map8(temp, 2U, HEIGHT - 3U)) {
    temp = HEIGHT + 1U - pcnt;
    if (random8(4U) == 0U) { // Случайные обвалы песка до половины
      temp = random8(2U) ? 2U : 3U;
    }

    for (uint8_t y = 0U; y < pcnt; y++) {
      for (uint8_t x = 0U; x < WIDTH; x++) {
        if (random8(temp) == 0U) {
          leds[XY(x, y)] = 0U;
        }
      }
    }
  }

  pcnt = 0U;

  // осыпаем всё, что есть на экране
  for (uint8_t y = 1U; y < HEIGHT; y++) {
    const uint16_t idx_y_minus = y - 1U;

    for (uint8_t x = 0U; x < WIDTH; x++) {
      const uint16_t current_idx = XY(x, y);

      // Если в текущей ячейке есть песчинка
      if (leds[current_idx]) {
        const uint16_t bottom_idx = XY(x, idx_y_minus);

        // Если под нами строго пусто — падаем вниз
        if (!leds[bottom_idx]) {
          leds[bottom_idx] = leds[current_idx];
          leds[current_idx] = 0U;
        }
        // Под нами пик (свободно и слева-снизу, и справа-снизу)
        else if (x > 0U && !leds[XY(x - 1U, idx_y_minus)] && x < MAX_X && !leds[XY(x + 1U, idx_y_minus)]) {
          if (random8(2U) == 0U) {
            leds[XY(x - 1U, idx_y_minus)] = leds[current_idx];
          } else {
            leds[XY(x + 1U, idx_y_minus)] = leds[current_idx];
          }
          leds[current_idx] = 0U;
          pcnt = idx_y_minus;
        }
        // Под нами склон налево
        else if (x > 0U && !leds[XY(x - 1U, idx_y_minus)]) {
          leds[XY(x - 1U, idx_y_minus)] = leds[current_idx];
          leds[current_idx] = 0U;
          pcnt = idx_y_minus;
        }
        // Под нами склон направо
        else if (x < MAX_X && !leds[XY(x + 1U, idx_y_minus)]) {
          leds[XY(x + 1U, idx_y_minus)] = leds[current_idx];
          leds[current_idx] = 0U;
          pcnt = idx_y_minus;
        }
        // Под нами плоское плато из песка
        else {
          pcnt = y;
        }
      }
    }
  }

  // эмиттер новых песчинок
  constexpr uint8_t spawn_y = HEIGHT - 2U;
  if (!leds[XY(CENTER_X_MINOR, spawn_y)] && !leds[XY(CENTER_X_MAJOR, spawn_y)] && random8(3U) == 0U) {
    temp = random8(2U) ? CENTER_X_MINOR : CENTER_X_MAJOR;
    leds[XY(temp, MAX_Y)] = CHSV(random8(), 255U, 255U);
  }
}
#endif


#ifdef DEF_SPIDER
// ============= Эффект Плазменная лампа ===============
// эффект Паук (c) stepko
// плюс выбор палитры и багфикс (c) SottNick

static void spiderRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        uint8_t tmp = random8(5U);
        if (tmp > 1U) tmp += 3U;
        setModeSettings(tmp * 11U + 3U + random8(7U), 1U + random8(180U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette();

    pcnt = (modes[currentMode].Scale - 1U) % 11U + 1U; // количество линий от 1 до 11 для каждой из 9 палитр
    speedfactor = remap(modes[currentMode].Speed, (uint8_t)1U, (uint8_t)255U, 20.0f, 2.0f);

    deltaValue = 255U / pcnt;      // шаг смещения цвета для каждой линии
    emitterX = 1.0f / speedfactor; // сохраняем inv_speed в свободный float-буфер

    loadingFlag = false;
  }

  if (hue2++ & 0x01 && deltaHue++ & 0x01 && deltaHue2++ & 0x01) { // хз. как с 60ю кадрами в секунду скорость замедлять...
    hue++;
  }

  dimAll(205U);

  const float time_shift = (float)(millis() & 0x7FFFFFU) * emitterX;

  constexpr float inv12 = 1.0f / 12.0f;

  for (uint8_t c = 0U; c < pcnt; c++) {
    // Вращение Лиссажу на базе быстрых FastLED-функций sin8/cos8
    float xx = 2.0f + (float)sin8(time_shift + 6000.0f * (float)c) * inv12;
    float yy = 2.0f + (float)cos8(time_shift + 9000.0f * (float)c) * inv12;

    DrawLineF(xx, yy, MAX_X - xx, MAX_Y - yy, ColorFromPalette(*curPalette, hue + c * deltaValue));
  }
}
#endif


#ifdef DEF_AURORA
// --------- Эффект "Северное Сияние"
// (c) kostyamat 05.02.2021
// идеи подсмотрены тут https://www.reddit.com/r/FastLED/comments/jyly1e/challenge_fastled_sketch_that_fits_entirely_in_a/
// особая благодарность https://www.reddit.com/user/ldirko/ Yaroslaw Turbin aka ldirko

// вместо набора палитр в оригинальном эффекте сделан генератор палитр
#define AURORA_COLOR_RANGE (10U) // (+/-10 единиц оттенка) диапазон, в котором плавает цвет сияния относительно выбранного оттенка
#define AURORA_COLOR_PERIOD (2U) // (2 раза в минуту) частота, с которой происходит колебание выбранного оттенка в разрешённом диапазоне

// генератор палитр для Северного сияния (c) SottNick
// static const uint8_t MBAuroraColors_arr[5][4] PROGMEM = // палитра в формате CHSV
// CRGBPalette16 myPal; уже есть эта переменная в эффекте Жидкая лампа

static unsigned long polarTimer;
//float adjastHeight; // используем emitterX
//uint16_t adjScale; // используем ff_y

static void polarRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(random8(3U) ? 1U + random8(99U) : 100U, 1U + random8(170U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    ff_y = map(WIDTH, 8, 64, 310, 63);
    ff_z = ff_y;

    hue = (uint8_t)((modes[currentMode].Scale - 1U) * 2.55f);
    speedfactor = map(modes[currentMode].Speed, 1, 255, 128, 16);

    loadingFlag = false;
  }

  const bool invert_flag = (modes[currentMode].Scale & 0x01);

  if (modes[currentMode].Scale == 100U) {
    if (hue2++ & 0x01 && deltaHue++ & 0x01 && deltaHue2++ & 0x01) {
      hue++;
    }
    fillMyPal16(hue, invert_flag, MBAuroraColors_arr);
  } else {
    uint8_t wave_hue = hue + AURORA_COLOR_RANGE - beatsin8(AURORA_COLOR_PERIOD, 0U, AURORA_COLOR_RANGE * 2U);
    fillMyPal16(wave_hue, invert_flag, MBAuroraColors_arr);
  }

  constexpr float AURORA_EMITTER_X = 400.0f / (float)HEIGHT;  // а это - максимум без яркой засветки крайних рядов матрицы (сверху и снизу)

  for (uint8_t x = 0U; x < WIDTH; x++) {
    for (uint8_t y = 0U; y < HEIGHT; y++) {
      polarTimer++;

      const uint16_t noise_x = (polarTimer & 0x01U) + x * ff_z;
      const uint16_t noise_y = y * 16U + (polarTimer & 0x0FU);
      const uint32_t noise_z = polarTimer / (uint32_t)speedfactor;

      float diff_y = std::abs(CENTER_Y_F - (float)y);
      const uint8_t height_fade = (uint8_t)(diff_y * AURORA_EMITTER_X);

      uint8_t raw_noise = fastled_helper::perlin8(noise_x, noise_y, noise_z);
      uint8_t color_index = qsub8(raw_noise, height_fade);

      leds[XY(x, y)] = ColorFromPalette(myPal, color_index);
    }
  }
}
#endif


#ifdef DEF_SPHERES
// ----------- Эффект "Шары"
// (c) stepko and kostyamat https://wokwi.com/arduino/projects/289839434049782281
// 07.02.2021

static float randomf(float min, float max) {
  // Вычисляется на ПК при сборке. 4095 дает честный диапазон [0.0f, ~0.9997f]
  // constexpr float inv4095 = 1.0f / 4095.0f;
  // Вычисляется на ПК при сборке. 4096 дает честный диапазон [0.0f, 1.0f]
  constexpr float inv4096 = 1.0f / 4096.0f;

  // float k = (float)random16(4095U) * inv4096;
  float k = (float)random16(4096U) * inv4096;

  // Быстрый Lerp (линейная интерполяция)
  return min + k * (max - min);
}

static void ballsfill_circle(float cx, float cy, float radius, const CRGB& col) {
  radius -= 0.5f;
  for (int16_t y = -radius; y <= radius; y++) {
    for (int16_t x = -radius; x <= radius; x++) {
      if (x * x + y * y <= radius * radius) {
        drawPixelXYF(cx + x, cy + y, col);
      }
    }
  }
}

static void spheresRoutine() {
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(random8(8U) * 11U + 6U + random8(6U), 1U + random8(255U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette();

    speedfactor = remap(modes[currentMode].Speed, (uint8_t)1, (uint8_t)255, 0.15f, 0.5f);
    enlargedObjectNUM = (modes[currentMode].Scale - 1U) % 11U + 1U;
    emitterY = 0.5f + QUARTER_Y / (2.0f - 1.0f / (float)enlargedObjectNUM);  // radiusMax

    for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
      trackingObjectShift[i] = randomf(0.5f, emitterY);            // radius[i] = randomf(0.5, radiusMax);
      trackingObjectSpeedX[i] = randomf(0.5f, 1.1f) * speedfactor; // ball[i][2] =
      trackingObjectSpeedY[i] = randomf(0.5f, 1.1f) * speedfactor; // ball[i][3] =
      trackingObjectPosX[i] = random8(WIDTH);                      // ball[i][0] = random(0, WIDTH);
      trackingObjectPosY[i] = random8(HEIGHT);                     // ball[i][1] = random(0, HEIGHT);
      trackingObjectHue[i] = random8();                            // color[i] = random(0, 255);
    }

    loadingFlag = false;
  }

  dimAll(255U - map(modes[currentMode].Speed, 1, 255, 5, 20)); // fadeToBlackBy(leds, NUM_LEDS, map(speed, 1, 255, 5, 20));

  const float pulse_coeff = 0.1f * speedfactor;

  for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
    const float max_speed = std::max(std::abs(trackingObjectSpeedX[i]), std::abs(trackingObjectSpeedY[i]));
    const float radius_step = max_speed * pulse_coeff;

    if (trackingObjectIsShift[i]) {
      trackingObjectShift[i] += radius_step;
      if (trackingObjectShift[i] >= emitterY) {
        trackingObjectIsShift[i] = false;
      }
    } else {
      trackingObjectShift[i] -= radius_step;
      if (trackingObjectShift[i] < 1.0f) {
        trackingObjectIsShift[i] = true;
        trackingObjectHue[i] = random8();
      }
    }

    if (trackingObjectShift[i] > 1.0f) {
      ballsfill_circle(trackingObjectPosY[i], trackingObjectPosX[i], trackingObjectShift[i], ColorFromPalette(*curPalette, trackingObjectHue[i]));
    } else {
      drawPixelXYF(trackingObjectPosY[i], trackingObjectPosX[i], ColorFromPalette(*curPalette, trackingObjectHue[i]));
    }

    const float inv_radius = 1.0f / trackingObjectShift[i];

    // Симуляция движения по оси X (сглаживание у краев HEIGHT)
    if (trackingObjectPosX[i] + trackingObjectShift[i] >= MAX_Y) {
      trackingObjectPosX[i] += trackingObjectSpeedX[i] * ((MAX_Y - trackingObjectPosX[i]) * inv_radius + 0.005f);
    } else if (trackingObjectPosX[i] - trackingObjectShift[i] <= 0.0f) {
      trackingObjectPosX[i] += trackingObjectSpeedX[i] * (trackingObjectPosX[i] * inv_radius + 0.005f);
    } else {
      trackingObjectPosX[i] += trackingObjectSpeedX[i];
    }

    // Симуляция движения по оси Y (сглаживание у краев WIDTH)
    if (trackingObjectPosY[i] + trackingObjectShift[i] >= MAX_X) {
      trackingObjectPosY[i] += trackingObjectSpeedY[i] * ((MAX_X - trackingObjectPosY[i]) * inv_radius + 0.005f);
    } else if (trackingObjectPosY[i] - trackingObjectShift[i] <= 0.0f) {
      trackingObjectPosY[i] += trackingObjectSpeedY[i] * (trackingObjectPosY[i] * inv_radius + 0.005f);
    } else {
      trackingObjectPosY[i] += trackingObjectSpeedY[i];
    }

    // Идеальное сохранение физики жестких отскоков (Правило 5)
    if (trackingObjectPosX[i] < 0.01f) {
      trackingObjectSpeedX[i] = randomf(0.5f, 1.1f) * speedfactor;
      trackingObjectPosX[i] = 0.01f;
    }
    else if (trackingObjectPosX[i] > HEIGHT - 1.01f) {
      trackingObjectSpeedX[i] = -randomf(0.5f, 1.1f) * speedfactor;
      trackingObjectPosX[i] = HEIGHT - 1.01f;
    }

    if (trackingObjectPosY[i] < 0.01f) {
      trackingObjectSpeedY[i] = randomf(0.5f, 1.1f) * speedfactor;
      trackingObjectPosY[i] = 0.01f;
    }
    else if (trackingObjectPosY[i] > WIDTH - 1.01f) {
      trackingObjectSpeedY[i] = -randomf(0.5f, 1.1f) * speedfactor;
      trackingObjectPosY[i] = WIDTH - 1.01f;
    }
  }

  blurScreen(48U);
}
#endif


#ifdef DEF_MAGMA
// ============= Эффект Магма ===============
// (c) SottNick
// берём эффекты Огонь 2020 и Прыгуны:
// хуяк-хуяк - и в продакшен!

static void magmaRoutine(){
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        // палитры 0, 1, 5, 6, 7
        uint8_t tmp = random8(6U);
        if (tmp>1U) tmp+=3U;
        setModeSettings(tmp * 11U + 2U + random8(7U) , 185U + random8(48U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    deltaValue = modes[currentMode].Scale * 0.0899f;       // / 100.0F * ((sizeof(palette_arr) /sizeof(TProgmemRGBPalette16 *))-0.01F));
    if (deltaValue == 3U ||deltaValue == 4U)
      curPalette =  palette_arr[deltaValue];               // (uint8_t)(modes[currentMode].Scale/100.0F * ((sizeof(palette_arr) /sizeof(TProgmemRGBPalette16 *))-0.01F))];
    else
      curPalette = firePalettes[deltaValue];               // (uint8_t)(modes[currentMode].Scale/100.0F * ((sizeof(firePalettes)/sizeof(TProgmemRGBPalette16 *))-0.01F))];

    deltaValue = 12U;                                      // deltaValue = (((modes[currentMode].Scale - 1U) % 11U + 1U) << 4U) - 8U; // ширина языков пламени (масштаб шума Перлина)
    deltaHue = 10U;                                        // map(deltaValue, 8U, 168U, 8U, 84U); // высота языков пламени должна уменьшаться не так быстро, как ширина
    // step = map(255U - deltaValue, 87U, 247U, 4U, 32U);  // вероятность смещения искорки по оси ИКС

    constexpr float height_inv = 255.0f / MAX_Y;
    for (uint8_t j = 0U; j < HEIGHT; j++) {
      shiftHue[j] = (MAX_Y - j) * height_inv;        // init colorfade table
    }

    // ledsClear(); // esphome: FastLED.clear();
    // enlargedObjectNUM = (modes[currentMode].Scale - 1U) / 99.0 * (enlargedOBJECT_MAX_COUNT - 1U) + 1U;
    enlargedObjectNUM = (modes[currentMode].Scale - 1U) % 11U / 10.0f * (enlargedOBJECT_MAX_COUNT - 1U) + 1U;
    if (enlargedObjectNUM > enlargedOBJECT_MAX_COUNT) enlargedObjectNUM = enlargedOBJECT_MAX_COUNT;
    // if (enlargedObjectNUM < 2U) enlargedObjectNUM = 2U;

    for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = random8(HEIGHT);
      trackingObjectHue[i] = 50U;                          // random8();
    }

    loadingFlag = false;
  }

  // dimAll(255U - modes[currentMode].Scale * 2);
  // dimAll(255U - 44U * 2);
  dimAll(181U);

  for (uint8_t i = 0U; i < WIDTH; i++) {
    const uint16_t i_deltaValue = i * deltaValue;

    for (uint8_t j = 0U; j < HEIGHT; j++) {
      const uint16_t y_coord = (j + ff_y + (random8() & 0x01U)) * deltaHue;
      const uint8_t noise_val = fastled_helper::perlin8(i_deltaValue, y_coord, ff_z);
      const uint8_t palette_index = qsub8(noise_val, shiftHue[j]);

      drawPixelXYF(i, MAX_Y - j, ColorFromPalette(*curPalette, palette_index, 255U));
    }
  }

  for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
    LeapersMove_leaper(i);
    if (trackingObjectPosY[i] >= QUARTER_Y) {
      drawPixelXYF(trackingObjectPosX[i], trackingObjectPosY[i], ColorFromPalette(*curPalette, trackingObjectHue[i]));
    }
  }

  // blurScreen(20);

  ff_y++;
  if (ff_y & 0x01) {
    ff_z++;
  }
}
#endif


#ifdef DEF_FLAME
// ============= Эффект Пламя ===============
// (c) SottNick
// По мотивам https://goldenandy.blogspot.com/2021/05/ws2812.html
// by Андрей Локтев

// характеристики языков пламени
//  x, dx; => trackingObjectPosX, trackingObjectSpeedX;
//  y, dy; => trackingObjectPosY, trackingObjectSpeedY;
//  ttl; => trackingObjectState;
//  uint8_t hue; => float   trackingObjectShift
//  uint8_t saturation; => 255U
//  uint8_t value; => trackingObjectHue;

// характеристики изображения CHSV picture[WIDTH][HEIGHT]
//  uint8_t .hue; => noise3d[0][WIDTH][HEIGHT]
//  uint8_t .sat; => shiftValue[HEIGHT] (не хватило двухмерного массива на насыщенность)
//  uint8_t .val; => noise3d[1][WIDTH][HEIGHT]

#define FLAME_MAX_DY      (256U) // максимальная вертикальная скорость перемещения языков пламени за кадр.  имеется в виду 256/256 =   1 пиксель за кадр
#define FLAME_MIN_DY      (128U) // минимальная вертикальная скорость перемещения языков пламени за кадр.   имеется в виду 128/256 = 0.5 пикселя за кадр
#define FLAME_MAX_DX       (32U) // максимальная горизонтальная скорость перемещения языков пламени за кадр. имеется в виду 32/256 = 0.125 пикселя за кадр
#define FLAME_MIN_DX   (-FLAME_MAX_DX)
#define FLAME_MAX_VALUE   (255U) // максимальная начальная яркость языка пламени
#define FLAME_MIN_VALUE   (176U) // минимальная начальная яркость языка пламени

// пришлось изобрести очередную функцию субпиксельной графики. на этот раз бесшовная по ИКСу, работающая в цветовом пространстве HSV и без смешивания цветов
static void wu_pixel_maxV(int16_t item) {
  const int16_t base_x = trackingObjectPosX[item];
  const int16_t base_y = trackingObjectPosY[item];

  const uint8_t xx = (trackingObjectPosX[item] - base_x) * 255.0f;
  const uint8_t yy = (trackingObjectPosY[item] - base_y) * 255.0f;
  const uint8_t ix = 255U - xx;
  const uint8_t iy = 255U - yy;

  // calculate the intensities for each affected pixel
  const uint8_t wu[4] = {
    WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy),
    WU_WEIGHT(ix, yy), WU_WEIGHT(xx, yy)
  };

  const uint8_t obj_shift = trackingObjectShift[item];
  const uint8_t obj_hue   = trackingObjectHue[item];

  // multiply the intensities by the colour, and saturating-add them to the pixels
  for (uint8_t i = 0U; i < 4U; i++) {
    int16_t x1 = base_x + (i & 1U);
    if (x1 < 0) x1 += WIDTH;
    else if (x1 >= WIDTH) x1 -= WIDTH;
    const uint8_t y1 = base_y + ((i >> 1U) & 1U);
    const uint8_t val = (obj_hue * wu[i]) >> 8U;

    if (y1 < HEIGHT && val >= noise3d[1][x1][y1]) {
      noise3d[0][x1][y1] = obj_shift;
      shiftValue[y1] = 255U;
      noise3d[1][x1][y1] = val;
    }
  }
}

static void execStringsFlame() { // внимание! эффект заточен на бегунок Масштаб с диапазоном от 0 до 255
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        setModeSettings(1U + random8(255U), 20U + random8(236U)); // на свякий случай пусть будет от 1 до 255, а не от нуля
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    enlargedObjectNUM = (modes[currentMode].Speed - 1U) / 254.0f * (trackingOBJECT_MAX_COUNT - 1U) + 1U;
    if (enlargedObjectNUM > enlargedOBJECT_MAX_COUNT) enlargedObjectNUM = enlargedOBJECT_MAX_COUNT;

    hue = map8(myScale8(modes[currentMode].Scale + 3U), 3U, 10U);   // минимальная живучесть/высота языка пламени ...ttl
    hue2 = map8(myScale8(modes[currentMode].Scale + 3U), 6U, 31U);  // максимальная живучесть/высота языка пламени ...ttl

    for (uint16_t i = 0U; i < trackingOBJECT_MAX_COUNT; i++) {      // чистим массив объектов от того, что не похоже на языки пламени
      if (trackingObjectState[i] > 30U || trackingObjectPosY[i] >= HEIGHT || trackingObjectPosX[i] >= WIDTH || trackingObjectPosY[i] <= 0) {
        trackingObjectHue[i] = 0U;
        trackingObjectState[i] = random8(20U);
      }
    }

    for (uint8_t i = 0U; i < WIDTH; i++) {                          // заполняем массив изображения из массива leds обратным преобразованием, которое нихрена не работает
      for (uint8_t j = 0U; j < HEIGHT; j++) {
        CHSV tHSV = rgb2hsv_approximate(leds[XY(i, j)]);
        noise3d[0][i][j] = tHSV.hue;
        if (tHSV.val > 100U) {                                      // такая защита от пересвета более-менее достаточна
          shiftValue[j] = tHSV.sat;
          if (tHSV.sat < 100U) {                                    // для перехода с очень тусклых эффектов, использующих заливку белым или почти белым светом
            noise3d[1][i][j] = tHSV.val / 3U;
          } else {
            noise3d[1][i][j] = tHSV.val - 32U;
          }
        } else {
          noise3d[1][i][j] = 0U;
        }
      }
    }

    loadingFlag = false;
  }

  // угасание предыдущего кадра
  for (uint8_t i = 0U; i < WIDTH; i++) {
    for (uint8_t j = 0U; j < HEIGHT; j++) {
      noise3d[1][i][j] = ((uint16_t)(noise3d[1][i][j]) * 237U) >> 8U;
    }
  }

  constexpr uint8_t dx_diff = FLAME_MAX_DX - FLAME_MIN_DX;
  constexpr uint8_t dy_diff = FLAME_MAX_DY - FLAME_MIN_DY;
  constexpr uint8_t val_diff = FLAME_MAX_VALUE - FLAME_MIN_VALUE + 1U;

  // цикл перебора языков пламени
  for (uint16_t i = 0U; i < enlargedObjectNUM; i++) {
    const uint8_t state = trackingObjectState[i];

    if (state) { // если ещё не закончилась его жизнь
      wu_pixel_maxV(i);

      trackingObjectState[i]--;

      trackingObjectPosX[i] += trackingObjectSpeedX[i];
      trackingObjectPosY[i] += trackingObjectSpeedY[i];

      trackingObjectHue[i] = (trackingObjectState[i] * trackingObjectHue[i] + (state >> 1U)) / state;

      // если вышел за верхнюю границу или потух, то и жизнь закончилась
      if (trackingObjectPosY[i] >= HEIGHT || trackingObjectHue[i] < 2U) {
        trackingObjectState[i] = 0U;
      }

      // если вылез за край матрицы по горизонтали, перекинем на другую сторону
      if (trackingObjectPosX[i] < 0.0f) {
        trackingObjectPosX[i] += WIDTH;
      } else if (trackingObjectPosX[i] >= WIDTH) {
        trackingObjectPosX[i] -= WIDTH;
      }
    } else { // если жизнь закончилась, перезапускаем
      trackingObjectState[i] = random8(hue, hue2);
      trackingObjectShift[i] = (uint8_t)(254U + modes[currentMode].Scale + random8(20U));  // 254 - это шаг в обратную сторону от выбранного пользователем оттенка (стартовый оттенок диапазона)
                                                                                           // 20 - это диапазон из градиента цвета от выбранного пользователем оттенка (диапазон от 254 до 254+20)

      trackingObjectPosX[i] = (float)random16(WIDTH * 255U) * inv255;
      trackingObjectPosY[i] = -0.9f;

      trackingObjectSpeedX[i] = (FLAME_MIN_DX + random8(dx_diff)) * inv256;
      trackingObjectSpeedY[i] = (FLAME_MIN_DY + random8(dy_diff)) * inv256;
      trackingObjectHue[i]    = FLAME_MIN_VALUE + random8(val_diff);
    }
  }

  // выводим кадр на матрицу
  for (uint8_t i = 0U; i < WIDTH; i++) {
    for (uint8_t j = 0U; j < HEIGHT; j++) {
      hsv2rgb_spectrum(CHSV(noise3d[0][i][j], shiftValue[j], noise3d[1][i][j]), leds[XY(i, j)]);
    }
  }
}
#endif


#ifdef DEF_FIRE_2021
// ============= Эффект Огонь 2021 ===============
// (c) SottNick
// На основе алгоритма https://editor.soulmatelights.com/gallery/546-fire
// by Stepko

#define FIXED_SCALE_FOR_Y (4U) // менять нельзя. корректировка скорости ff_x =... подогнана под него

static void Fire2021Routine(){
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        uint8_t tmp = 1U + random8(89U); // пропускаем белую палитру
        if (tmp > 44U) tmp += 11U;
        setModeSettings(tmp, 42U + random8(155U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    if (modes[currentMode].Scale > 100U) modes[currentMode].Scale = 100U;

    deltaValue = modes[currentMode].Scale * 0.0899f;
    if (deltaValue == 3U || deltaValue == 4U)
      curPalette = palette_arr[deltaValue];
    else
      curPalette = firePalettes[deltaValue];

    deltaValue = (modes[currentMode].Scale - 1U) % 11U + 1U;

    if (modes[currentMode].Speed & 0x01U){
      ff_x = modes[currentMode].Speed;
      deltaHue2 = FIXED_SCALE_FOR_Y;
    } else {
      if (deltaValue > FIXED_SCALE_FOR_Y)
        speedfactor = 0.4f * (deltaValue - FIXED_SCALE_FOR_Y) + FIXED_SCALE_FOR_Y;
      else
        speedfactor = deltaValue;

      float poly = ((0.1686f * speedfactor - 1.162f) * speedfactor + 3.6694f) * speedfactor + 56.394f;   // Ааааа! это тупо подбор коррекции. очень приблизитеьный
      ff_x = roundf((modes[currentMode].Speed * 64.0f) / poly);
      deltaHue2 = deltaValue;
    }

    if (ff_x > 255U) ff_x = 255U;
    if (ff_x == 0U)  ff_x = 1U;

    step = map(ff_x * ff_x, 1U, 65025U, (deltaHue2 - 1U) / 2U + 1U, deltaHue2 * 18U + 44U);
    pcnt = map(step, 1U, 255U, 20U, 128U);                 // nblend 3th param

    deltaValue = 0.7f * deltaValue * deltaValue + 31.3f;   // ширина языков пламени (масштаб шума Перлина)
    deltaHue2 = 0.7f * deltaHue2 * deltaHue2 + 31.3f;      // высота языков пламени (масштаб шума Перлина)

    loadingFlag = false;
  }

  ff_y += step;

  constexpr uint16_t yStepFP = (255U * 256U) / HEIGHT;
  for (uint8_t x = 0U; x < WIDTH; x++) {
    const uint16_t x_deltaValue = x * deltaValue;

    for (uint8_t y = 0U; y < HEIGHT; y++) {
      const int16_t yOffset = (y * deltaHue2) - ff_y;
      const uint8_t yFade = (y * yStepFP) >> 8U;
      const int16_t Bri = fastled_helper::perlin8(x_deltaValue, yOffset, ff_z) - yFade;

      uint8_t finalBri = 0U;
      if (Bri > 0) {
        finalBri = 255U - (uint8_t)(Bri / 5U);
      }

      nblend(leds[XY(x, y)], ColorFromPalette(*curPalette, (uint8_t)Bri, finalBri), pcnt);
    }
  }

  if (!random8()) {
    ff_z++;
  }
}
#endif


#ifdef DEF_LUMENJER
// =============== Эффект Lumenjer ================
// (c) SottNick

constexpr uint8_t DIMSPEED = 254U - (500U / NUM_LEDS);

static void lumenjerRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      uint8_t tmp = random8(17U); //= random8(19U);
      if (tmp > 2U) tmp += 2U;
      tmp = (uint8_t)(tmp * 5.556f + 3.0f);
      if (tmp > 100U) tmp = 100U;
      setModeSettings(tmp, 190U + random8(56U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    if (modes[currentMode].Scale > 100U) {
      modes[currentMode].Scale = 100U;
    }

    constexpr float inv50 = 1.0f / 50.0f;
    if (modes[currentMode].Scale > 50U) {
      curPalette = firePalettes[(uint8_t)((modes[currentMode].Scale - 50U) * inv50 * ((sizeof(firePalettes) / sizeof(TProgmemRGBPalette16 *)) - 0.01f))];
    } else {
      curPalette = palette_arr[(uint8_t)(modes[currentMode].Scale * inv50 * ((sizeof(palette_arr) / sizeof(TProgmemRGBPalette16 *)) - 0.01f))];
    }

    deltaHue = -1;
    deltaHue2 = -1;
    dimAll(245U);

    loadingFlag = false;
  }

  dimAll(DIMSPEED);

  deltaHue = random8(3U) ? deltaHue : -deltaHue;
  deltaHue2 = random8(3U) ? deltaHue2 : -deltaHue2;

#if (((WIDTH & 0x01U) == 0U) && ((HEIGHT & 0x01U) == 0U))
  hue = (WIDTH + hue + (int8_t)deltaHue * (bool)random8(64U)) % WIDTH;
#else
  hue = (WIDTH + hue + (int8_t)deltaHue) % WIDTH;
#endif
  hue2 = (HEIGHT + hue2 + (int8_t)deltaHue2) % HEIGHT;

  if (modes[currentMode].Scale == 100U) {
    leds[XY(hue, hue2)] += CHSV(random8(), 255U, 255U);
  } else {
    leds[XY(hue, hue2)] += ColorFromPalette(*curPalette, step++);
  }
}
#endif


#ifdef DEF_CHRISTMAS_TREE
// =========== Christmas Tree ===========
//             © SlingMaster
//           EFF_CHRISTMAS_TREE
//            Новогодняя Елка
//---------------------------------------
static void VirtualSnow(uint8_t snow_type) {
  static int16_t deltaPos;

  constexpr uint8_t pre_max_h = HEIGHT - 2U;

  uint8_t posX = random8(MAX_X);
  uint8_t delta = (snow_type == 3) ? 0 : 1;

  for (uint8_t x = delta; x < WIDTH - delta; x++) {
    // заполняем случайно верхнюю строку
    if ((noise3d[0][x][pre_max_h] == 0U) && (posX == x) && (random8(0, 2) == 0U)) {
      noise3d[0][x][MAX_Y] = 1;
    } else {
      noise3d[0][x][MAX_Y] = 0;
    }

    for (uint8_t y = 0U; y < MAX_Y; y++) {
      switch (snow_type) {
        case 0:
          noise3d[0][x][y] = noise3d[0][x][y + 1U];
          deltaPos = 0;
          break;
        case 1:
        case 2:
          noise3d[0][x][y] = noise3d[0][x][y + 1U];
          deltaPos = 1 - random8(2U);
          break;
        default:
          deltaPos = -1;
          if ((x == 0U) && (y == 0U) && (random8(2U) == 0U)) {
            noise3d[0][MAX_X][random8(CENTER_Y_MAJOR / 2U, HEIGHT - CENTER_Y_MAJOR / 4U)] = 1U;
          }
          if (x > WIDTH - 2U) {
            noise3d[0][MAX_X][y] = 0U;
          }
          if (x < 1U) {
            noise3d[0][x][y] = noise3d[0][x][y + 1U];
          } else {
            noise3d[0][x - 1U][y] = noise3d[0][x][y + 1U];
          }
          break;
      }

      if (noise3d[0][x][y] > 0) {
        if (snow_type < 3) {
          if ((y & 0x01U) == 0U) {
            leds[XY(x - ((x > 0) ? deltaPos : 0), y)] = CHSV(160, 5U, random8(200U, 240U));
          } else {
            leds[XY(x + deltaPos, y)] = CHSV(160, 5U,  random8(200U, 240U));
          }
        } else {
          leds[XY(x, y)] = CHSV(160, 5U,  random8(200U, 240U));
        }
      }

      if (noise3d[0][x][y] > 0U) {
        const CRGB snowColor = CHSV(160U, 5U, random8(200U, 240U));
        if (snow_type < 3U) {
          if ((y & 0x01U) == 0U) {
            leds[XY(x - ((x > 0U) ? deltaPos : 0), y)] = snowColor;
          } else {
            leds[XY(x + deltaPos, y)] = snowColor;
          }
        } else {
          leds[XY(x, y)] = snowColor;
        }
      }
    }
  }
}

//---------------------------------------
static void GreenTree(uint8_t tree_h) {
  hue = step & 0xE0U;  // floor(step / 32) * 32. Оставляет значения кратные 32 (0, 32, 64...)

  for (uint8_t x = 0U; x <= WIDTH; x++) {
    if (x % 8U == 0U) {
      int16_t currentX = x - deltaValue; // Кэшируем для удобства и скорости

      if (modes[currentMode].Scale < 60) {
        // nature -----
        uint8_t h7  = (tree_h *  7U) * inv10;
        uint8_t h15 = (tree_h * 15U) * inv100;
        uint8_t h35 = (tree_h * 35U) * inv100;
        uint8_t h55 = (tree_h * 55U) * inv100;

        DrawLine(currentX - 1, h7,  currentX + 1,  h7, 0x002F00);
        DrawLine(currentX - 1, h55, currentX + 1, h55, 0x004F00);
        DrawLine(currentX - 2, h35, currentX + 2, h35, 0x005F00);
        DrawLine(currentX - 2, h15, currentX + 2, h15, 0x007F00);

        drawPixelXY(currentX - 3, h15, 0x001F00);
        drawPixelXY(currentX + 3, h15, 0x001F00);

        if (currentX >= 0) {
          gradientVertical(currentX, 0U, currentX, tree_h, 90U, 90U, 190U, 64U, 255U);
        }
      } else {
        // holiday -----
        uint8_t h60 = (tree_h *  6U) * inv10;
        uint8_t h25 = (tree_h * 25U) * inv100;

        drawPixelXY(currentX - 1, h60, CHSV(step, 255U, 128U + random8(128U)));
        drawPixelXY(currentX + 1, h60, CHSV(step, 255U, 128U + random8(128U)));
        drawPixelXY(currentX, (tree_h * 4U) * inv10, CHSV(step, 255U, 200U));
        drawPixelXY(currentX, (tree_h * 2U) * inv10, CHSV(step, 255U, 190U + random8(65U)));
        drawPixelXY(currentX - 2, h25, CHSV(step, 255U, 96U + random8(128U)));
        drawPixelXY(currentX + 2, h25, CHSV(step, 255U, 96U + random8(128U)));

        drawPixelXY(currentX - 2, 1U, CHSV(step, 255U, 200U));
        drawPixelXY(currentX,     0U, CHSV(step, 255U, 250U));
        drawPixelXY(currentX + 2, 1U, CHSV(step, 255U, 200U));

        if (currentX >= 0) {
          gradientVertical(currentX, (tree_h * 3U) / 4U, currentX, tree_h, hue, hue, 250U, 0U, 128U);
        }
      }
    }
  }
}

//---------------------------------------
inline constexpr uint8_t TREE_MAX_H = (HEIGHT > 16U) ? 16U : HEIGHT;

static void ChristmasTree() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), 10U + random8(128));
    }
#endif

    clearNoiseArr();

    deltaValue = 0;
    step = deltaValue;

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  if (HEIGHT > 16U && modes[currentMode].Scale < 60U) {
    gradientVertical(0, 0, WIDTH, HEIGHT, 160, 160, 64, 128, 255U);
  } else {
    ledsClear();
  }

  GreenTree(TREE_MAX_H);

  if (modes[currentMode].Scale < 60U) {
    VirtualSnow(1U);
  }
  if (modes[currentMode].Scale > 30U) {
    deltaValue++;
  }
  if (deltaValue >= 8U) {
    deltaValue = 0;
  }
  step++;
}
#endif


#ifdef DEF_BY_EFFECT
// ============== ByEffect ==============
//             © SlingMaster
//             EFF_BY_EFFECT
//            Побочный Эффект
// --------------------------------------
static void ByEffect() {
  // Вычисляем координаты один раз при компиляции
  constexpr uint8_t H_75 = (uint8_t)(HEIGHT * 0.75f);
  constexpr uint8_t H_70 = (uint8_t)(HEIGHT * 0.7f);
  constexpr uint8_t H_60 = (uint8_t)(HEIGHT * 0.6f);
  constexpr uint8_t H_40 = (uint8_t)(HEIGHT * 0.4f);
  constexpr uint8_t H_30 = (uint8_t)(HEIGHT * 0.3f);

  constexpr uint8_t saturation = 255U;

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed 210
      setModeSettings(random8(100U), random8(200U));
    }
    #endif

    deltaValue = 0;
    step = deltaValue;

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  // Заменяем floor(step / 32) * 32U на быстрый сдвиг вправо (>> 5 это деление на 32)
  hue = (step >> 5) * 32U;

  dimAll(180);

  // ------
  const bool scaleHigh = (modes[currentMode].Scale > 50U);

  uint8_t delta = 0U;

  for (uint8_t x = 0U; x <= WIDTH; x++) {
    if (x % 8U == 0U) {
      const int16_t currentX = x - deltaValue;

      gradientVertical(currentX, H_75, currentX + 1U, HEIGHT, hue, hue + 2U, 250U, 0U, 255U);

      if (scaleHigh) {
        delta = random8(200U);
      }
      drawPixelXY(currentX - 2U, H_70, CHSV(step, saturation - delta, 128U + random8(128U)));
      drawPixelXY(currentX + 2U, H_70, CHSV(step, saturation, 128U + random8(128U)));

      drawPixelXY(currentX, H_60, CHSV(hue, saturation, 190U + random8(65U)));

      if (scaleHigh) {
        delta = random8(200U);
      }
      drawPixelXY(currentX - 1U, CENTER_Y_MINOR, CHSV(step, saturation, 128U + random8(128U)));
      drawPixelXY(currentX + 1U, CENTER_Y_MINOR, CHSV(step, saturation - delta, 128U + random8(128U)));

      drawPixelXY(currentX, H_40, CHSV(hue, saturation, 200U));

      if (scaleHigh) {
        delta = random8(200U);
      }
      drawPixelXY(currentX - 2U, H_30, CHSV(step, saturation - delta, 96U + random8(128U)));
      drawPixelXY(currentX + 2U, H_30, CHSV(step, saturation, 96U + random8(128U)));

      gradientVertical(currentX, 0U, currentX + 1U, QUARTER_Y, hue + 2U, hue, 0U, 250U, 255U);

      if (scaleHigh) {
        drawPixelXY(currentX + 3U, HEIGHT - 3U, CHSV(step, saturation, 255U));
        drawPixelXY(currentX - 3U, CENTER_Y_MINOR, CHSV(step, saturation, 255U));
        drawPixelXY(currentX + 3U, 2U, CHSV(step, saturation, 255U));
      }
    }
  }

  // ------
  deltaValue++;
  if (deltaValue >= 8U) {
    deltaValue = 0U;
  }
  step++;
}
#endif


#ifdef DEF_COLOR_FRIZZLES
// =====================================
//            Цветные кудри
//           Color Frizzles
//             © Stepko
//       адаптация © SlingMaster
// =====================================
static void ColorFrizzles() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                        scale | speed
      setModeSettings(random(10U, 90U), 128);
    }
    #endif

    FPSdelay = 10U;
    deltaValue = 0;

    loadingFlag = false;
  }

  if (modes[currentMode].Scale > 50U) {
    if (FPSdelay > 48U) deltaValue = 0U;
    if (FPSdelay < 5U)  deltaValue = 1U;

    if (deltaValue == 1U) {
      FPSdelay++;
    } else {
      FPSdelay--;
    }
    blur2d(WIDTH, HEIGHT, 16U);
  } else {
    FPSdelay = 20U;
    dimAll(240U);
  }

  const uint8_t finalBri = 255U - (FPSdelay * 2U);
  const uint8_t baseHue = beatsin8(12U, 0U, 255U);

  for (uint8_t i = 8U; i--;) {
    leds[XY(beatsin8(12U + i, 0U, MAX_X), beatsin8(15U - i, 0U, MAX_Y))] = CHSV(baseHue, 255U, finalBri);
  }
}
#endif


#ifdef DEF_COLORED_PYTHON
// ============ Colored Python ============
//      base code WavingCell from © Stepko
//       Adaptation & modefed © alvikskor
//            Кольоровий Пітон
// --------------------------------------

static void Colored_Python() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
        //                     scale | speed
        setModeSettings(random8(100U), random8(1U, 255U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    colorChangeTime = millis();

    // Делитель времени
    deltaHue2 = 128U - (modes[currentMode].Speed >> 1U);  // / 2

    // Шаг палитры
    deltaHue = modes[currentMode].Speed % 10U;

    // Толщина змеек
    constexpr uint8_t thickness_map[] = {5U, 10U, 20U, 30U, 40U};
    deltaValue = thickness_map[modes[currentMode].Scale % 5U];

    // Выбора палитры
    const uint8_t palette_number = modes[currentMode].Scale / 10U;
    if (palette_number < 9U) {
      step = palette_number;
      pcnt = 0U; // Флаг: авто-переключения
    } else {
      step = 0U;
      pcnt = 1U;
    }

    loadingFlag = false;
  }

  if (pcnt == 1U) {
    if (millis() - colorChangeTime > 30000U) {
      colorChangeTime = millis();
      step++;
      if (step > 8U) step = 0U;
    }
  }

  switch (step) {
    case 0: currentPalette = CloudColors_p; break;
    case 1: currentPalette = AlcoholFireColors_p; break;
    case 2: currentPalette = OceanColors_p; break;
    case 3: currentPalette = ForestColors_p; break;
    case 4: currentPalette = RainbowColors_p; break;
    case 5: currentPalette = RainbowStripeColors_p; break;
    case 6: currentPalette = HeatColors_p; break;
    case 7: currentPalette = LavaColors_p; break;
    case 8: currentPalette = PartyColors_p;
  }

  const uint16_t t = millis() / deltaHue2;
  const uint16_t palette_motion = t * deltaHue;
  const uint16_t y_time_phase = t * 5U;

  for (uint8_t y = 0U; y < HEIGHT; y++) {
    const uint8_t y_cos = cos8(y * 10U);
    const uint8_t y_sin_phase = y * 5U + y_time_phase;

    for (uint8_t x = 0U; x < WIDTH; x++) {
      // deltaValue — толщина (thickness)
      const uint8_t index = ((sin8((x * deltaValue) + sin8(y_sin_phase)) + y_cos) + 1U) + palette_motion;
      leds[XY(x, y)] = ColorFromPalette(currentPalette, index);
    }
  }
}
#endif


#ifdef DEF_CONTACTS
// ================Contacts==============
//             © Yaroslaw Turbin
//        Adaptation © SlingMaster
//          modifed © alvikskor
//              Контакти
// =====================================

// static const uint8_t exp_gamma[256] PROGMEM = {

static void Contacts() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                         scale | speed
      setModeSettings(random8(25U, 91U), random8(5U, 251U));
    }
    #endif

    FPSdelay = 80U;

    deltaHue2 = map(modes[currentMode].Speed, 0U, 255U, 32U, 1U);
    hue = modes[currentMode].Scale / 14U;

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  const int32_t a = millis() / deltaHue2;
  const int32_t a_div2 = a >> 1U; // / 2
  const int32_t a_div3 = a / 3U;
  const int32_t a_div4 = a >> 2U; // / 4
  const int32_t a_div5 = a / 5U;

  for (uint8_t y = 0U; y < HEIGHT; y++) {
    const uint8_t y8_a2_cos  = cos8(y * 8U + a_div2);
    const uint8_t y8_a4_sin  = sin8(y * 8U + a_div4);
    const uint8_t y10_a3_cos = cos8((y * 10U) + a_div3) >> 2U; // / 4

    for (uint8_t x = 0U; x < WIDTH; x++) {
      const uint8_t color1 = pgm_read_byte(&exp_gamma[sin8(cos8(x * 7U + a_div5) - y10_a3_cos + a)]);
      const uint8_t color2 = pgm_read_byte(&exp_gamma[(uint8_t)(sin8(x * 16U + a_div3) + y8_a2_cos) >> 1U]);
      const uint8_t color3 = pgm_read_byte(&exp_gamma[sin8(cos8(x * 8U + a_div3) + y8_a4_sin + a)]);

      const uint16_t index = XY(x, y);

      switch (hue) {
        case 0U:
          leds[index].r = 0U;
          leds[index].g = color2;
          leds[index].b = color3 >> 2U;
          break;
        case 1U:
          leds[index].r = color3 >> 2U;
          leds[index].g = 0U;
          leds[index].b = color1;
          break;
        case 2U:
          leds[index].r = color3;
          leds[index].g = color1 >> 2U;
          leds[index].b = 0U;
          break;
        case 3U:
          leds[index].r = color3;
          leds[index].g = color2;
          leds[index].b = color1;
          break;
        case 5U:
          leds[index].r = color1;
          leds[index].g = color3;
          leds[index].b = color2;
          break;
        default: // Кейсы 4, 6
          leds[index].r = color2;
          leds[index].g = color1;
          leds[index].b = color3;
          break;
      }
    }
  }
}
#endif


#ifdef DEF_DROP_IN_WATER
// =====================================
//               DropInWater
//                © Stepko
//        Adaptation © SlingMaster
// =====================================

// CRGBPalette16 currentPalette(PartyColors_p);

inline constexpr uint8_t Sat = 255U;
inline constexpr uint8_t MaxRad = WIDTH + HEIGHT;
inline constexpr uint8_t DROP_COUNT = (((WIDTH + HEIGHT) / 8U) > 1U) ? ((WIDTH + HEIGHT) / 8U) : 2U;

static void DropInWater() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                         scale | speed
      setModeSettings(random8(0U, 101U), random8(160U, 216U));
    }
#endif

    hue = modes[currentMode].Scale * 2.55f;
    for (uint8_t i = 0U; i < DROP_COUNT; i++) {
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = random8(HEIGHT);

      // Для знаковых диапазонов, заходящих в минус, используем стандартный random
      trackingObjectShift[i] = random(-1, MaxRad);
    }

    loadingFlag = false;
  }

  fill_solid(currentPalette, 16U, CHSV(hue, Sat, 230U));
  currentPalette[10] = CHSV(hue, Sat - 60U, 255U);
  currentPalette[9] = CHSV(hue, 255U - Sat, 210U);
  currentPalette[8] = CHSV(hue, 255U - Sat, 210U);
  currentPalette[7] = CHSV(hue, Sat - 60U, 255U);
  fillAll(ColorFromPalette(currentPalette, 1U));

  for (int8_t i = (int8_t)(DROP_COUNT - 1U); i >= 0; i--) {
    const float current_rad = trackingObjectShift[i];

    constexpr float OUTER_RING_COLOR = (256.0f / 16.0f) * 8.5f;
    constexpr float INNER_RING_COLOR = (256.0f / 16.0f) * 7.5f;

    drawCircle(trackingObjectPosX[i], trackingObjectPosY[i], current_rad, ColorFromPalette(currentPalette, OUTER_RING_COLOR - current_rad));
    drawCircle(trackingObjectPosX[i], trackingObjectPosY[i], current_rad - 1.0f, ColorFromPalette(currentPalette, INNER_RING_COLOR - current_rad));

    if (current_rad >= MaxRad) {
      trackingObjectShift[i] = 0.0f;
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = random8(HEIGHT);
    } else {
      trackingObjectShift[i]++;
    }
  }

  if (modes[currentMode].Scale == 100U) {
    hue++;
  }

  blur2d(WIDTH, HEIGHT, 64U);
}
#endif


#ifdef DEF_FEATHER_CANDLE
// =========== FeatherCandle ============
//         адаптация © SottNick
//    github.com/mnemocron/FeatherCandle
//      modify & design © SlingMaster
//           EFF_FEATHER_CANDLE
//                Свеча
//---------------------------------------
// const uint8_t PROGMEM anim[] =                      // FeatherCandle animation data

constexpr uint8_t level     = 160U;
constexpr uint8_t low_level = 110U;
inline constexpr uint8_t  w = 7U;                      // image width
inline constexpr uint8_t  h = 15U;                     // image height

static const uint8_t *ptr  = anim;                     // Current pointer into animation data
static uint8_t        img[w * h];                      // Buffer for rendering image
// static uint8_t        deltaX = CENTER_X_MAJOR - 3;  // position img
// static uint8_t last_brightness;

static void FeatherCandleRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                         scale | speed
      setModeSettings(1U + random8(99U), 190U + random8(65U));
    }
    #endif

    hue = 0;

    trackingObjectState[0] = low_level;
    trackingObjectState[1] = low_level;
    trackingObjectState[2] = low_level;
    trackingObjectState[4] = CENTER_X_MAJOR;

    // dynamic flame scaling
    const float scale_w = (float)WIDTH / w;
    const float scale_h = (float)HEIGHT / h;
    const float scale = clamp((scale_w < scale_h) ? scale_w : scale_h, 0.4f, 4.0f);

    deltaHue  = (uint8_t)(w * scale + 0.5f);           // Сохраняем drawW
    deltaHue2 = (uint8_t)(h * scale + 0.5f);           // Сохраняем drawH

    const int8_t offX = (WIDTH - deltaHue) / 2U;
    const int8_t offY = (HEIGHT - deltaHue2) / 2U;
    pcnt       = (offX < 0) ? 0U : (uint8_t)offX;      // Сохраняем offsetX
    deltaValue = (offY < 0) ? 0U : (uint8_t)offY;      // Сохраняем offsetY

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  // Считывание кадра анимации из PROGMEM
  uint8_t a = pgm_read_byte(ptr++);     // New frame X1/Y1
  if (a >= 0x90U) {                     // EOD marker? (valid X1 never exceeds 8)
    ptr = anim;                         // Reset animation data pointer to start
    a   = pgm_read_byte(ptr++);         // and take first value
  }
  const uint8_t x1 = a >> 4U;           // X1 = high 4 bits
  const uint8_t y1 = a & 0x0FU;         // Y1 = low 4 bits

  a = pgm_read_byte(ptr++);             // New frame X2/Y2
  const uint8_t x2 = a >> 4U;           // X2 = high 4 bits
  const uint8_t y2 = a & 0x0FU;         // Y2 = low 4 bits

  // Read rectangle of data from anim[] into portion of img[] buffer
  for (uint8_t y = y1; y <= y2; y++) {
    const uint16_t y_w = y * w;
    for (uint8_t x = x1; x <= x2; x++) {
      img[y_w + x] = pgm_read_byte(ptr++);
    }
  }

  // Расчет базового цвета один раз за кадр
  const uint8_t color = (uint8_t)((modes[currentMode].Scale - 1U) * 2.57f);

  // Предрасчет шага интерполяции (Fixed Point 8.8) для полной ликвидации делений в цикле
  const uint16_t step_y = (uint16_t)((h << 8U) / deltaHue2);
  const uint16_t step_x = (uint16_t)((w << 8U) / deltaHue);

  // draw flame -------------------
  for (uint8_t dy = 0U; dy < deltaHue2; dy++) {
    const uint8_t sy = (dy * step_y) >> 8U;
    const uint16_t sy_w = sy * w;
    const uint8_t py = deltaValue + dy;

    if (py < HEIGHT) {
      for (uint8_t dx = 0U; dx < deltaHue; dx++) {
        const uint8_t sx = (dx * step_x) >> 8U;
        const uint8_t brightness = img[sy_w + sx];

        if (brightness > 0U) {
          const uint8_t px = pcnt + dx;
          if (px < WIDTH) {
            leds[XY(px, py)] = CHSV(brightness > 240U ? color : (uint8_t)(color - 10U), 255U, brightness);
          }
        }
      }
    }
  }

  // draw body FeatherCandle ------
  const uint8_t bodyH = (deltaHue2 >= 5U) ? (deltaHue2 / 5U) : 1U;
  if (bodyH > 0U && (deltaValue + bodyH) < HEIGHT) {
    gradientVertical(0U, deltaValue, WIDTH, deltaValue + bodyH, color, color, 48U, 128U, 20U);
  }

  // drops of wax move -------------
  switch (hue) {
    case 0U:
      if (trackingObjectState[0] + 3U < level) trackingObjectState[0] += 3U;
      else trackingObjectState[0] = level;
      break;
    case 1U:
      if (trackingObjectState[0] > low_level + 3U) trackingObjectState[0] -= 3U;
      else trackingObjectState[0] = low_level;

      if (trackingObjectState[1] + 3U < level) trackingObjectState[1] += 3U;
      else trackingObjectState[1] = level;
      break;
    case 2U:
      if (trackingObjectState[1] > low_level + 3U) trackingObjectState[1] -= 3U;
      else trackingObjectState[1] = low_level;

      if (trackingObjectState[2] + 3U < level) trackingObjectState[2] += 3U;
      else trackingObjectState[2] = level;
      break;
    case 3U:
      if (trackingObjectState[2] > low_level + 3U) trackingObjectState[2] -= 3U;
      else trackingObjectState[2] = low_level;

      if (trackingObjectState[2] == low_level) {
        hue++;
        trackingObjectState[4] = CENTER_X_MAJOR - 3U + random8(6U);
      }
      break;
  }

  if (hue > 3U) {
    hue++;
  } else {
    if (trackingObjectState[4] < WIDTH) {
      if (hue < 2U)
        leds[XY(trackingObjectState[4], 2U)] = CHSV(50U, 20U, trackingObjectState[0]);
      if (hue == 1U || hue == 2U)
        leds[XY(trackingObjectState[4], 1U)] = CHSV(50U, 15U, trackingObjectState[1]);
      if (hue > 1U)
        leds[XY(trackingObjectState[4], 0U)] = CHSV(50U, 5U, trackingObjectState[2]);
    }
  }

  // next -----------------
  if (trackingObjectState[0] >= level || trackingObjectState[1] >= level || trackingObjectState[2] >= level) {
    hue++;
  }
}
#endif


#ifdef DEF_FIREWORK
// =====================================
//               Фейерверк
//                Firework
//             © SlingMaster
// =====================================
static void VirtualExplosion(uint8_t f_type, int8_t timeline) {
  constexpr uint8_t DELAY_SECOND_EXPLOSION = QUARTER_Y;
  constexpr uint8_t horizont = 1U;  // HEIGHT * inv5;
  constexpr int8_t STEP = 255U / HEIGHT;
  constexpr uint8_t timeline_trigger = HEIGHT + DELAY_SECOND_EXPLOSION;

  constexpr uint8_t ACTION_TIME = ((uint16_t)HEIGHT * 7U) >> 2U; // HEIGHT * 1.75 через сдвиг
  constexpr uint8_t SECOND_EXPLOSION = ((uint16_t)(HEIGHT - DELAY_SECOND_EXPLOSION) * 7U) >> 2U;

  uint8_t firstColor  = random8();
  uint8_t secondColor = 0U;
  uint8_t saturation  = 255U;

  switch (f_type) {
    case 0U:
      secondColor = random8(50U, 255U);
      saturation  = random8(245U, 255U);
      break;
    case 1U: /* сакура */
      firstColor  = random8(210U, 230U);
      secondColor = random8(65U, 85U);
      break;
    case 2U: /* день Независимости */
      firstColor  = random8(160U, 170U);
      secondColor = random8(25U, 50U);
      break;
    default: /* хризантемы */
      firstColor  = random8(30U, 40U);
      secondColor = random8(25U, 50U);
      saturation  = random8(128U, 255U);
      break;
  }

  if ((timeline > MAX_Y) && (timeline < ACTION_TIME)) {
    for (uint8_t x = 0U; x < WIDTH; x++) {
      for (uint8_t y = horizont; y < MAX_Y; y++) {
        noise3d[0][x][y] = noise3d[0][x][y + 1U];
        const uint8_t bri = y * STEP;

        if (noise3d[0][x][y] > 0U) {
          if (timeline > timeline_trigger) {
            /* second explosion */
            drawPixelXY((uint8_t)(x - 2U + random8(4U)), y - 1U, CHSV((uint8_t)(secondColor + random8(16U)), saturation, bri));
          }
          if (timeline < SECOND_EXPLOSION) {
            /* first explosion */
            drawPixelXY(x, y, CHSV(firstColor, 255U, bri));
          }
        }
      }
    }

    const uint8_t posX = random8(WIDTH);
    const uint8_t step_odd = ((step & 0x01U) == 0U) ? 1U : 0U;
    for (uint8_t x = 0U; x < WIDTH; x++) {
      noise3d[0][x][MAX_Y] = (posX == x) ? step_odd : 0U;
    }
  }
}

// --------------------------------------
static void Firework() {
  constexpr uint8_t MAX_BRIGHTNESS = 40U;            /* sky brightness */
  constexpr uint8_t DOT_EXPLOSION = HEIGHT * 0.95f;
  constexpr uint8_t HORIZONT = QUARTER_Y;
  constexpr uint8_t DELTA = 1U;                      /* центровка по вертикали */
  constexpr float stepH = HEIGHT / 128.0f;
  constexpr uint8_t FPS_DELAY = 20U;
  // constexpr uint8_t STEP = 3U;
  constexpr uint8_t skyColor = 156U;

  constexpr uint8_t PHASE_V1     = (uint8_t)(HEIGHT  * 1.5f);
  constexpr uint8_t PHASE_V2     = (uint8_t)(HEIGHT  * 1.25f);
  constexpr uint16_t PHASE_LIMIT = (uint16_t)(HEIGHT * 2.5f);

  uint8_t sizeH;

  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                          scale | speed
      setModeSettings(1U + random8(100U), 1U + random8(250U));
    }
#endif

    deltaHue2 = 0U;
    FPSdelay = 255U;
    step = 0U;
    hue = 48U; // skyBright
    deltaHue2 = modes[currentMode].Scale / 26U;

    if (modes[currentMode].Speed > 85U) {
      sizeH = HORIZONT;
      FPSdelay = FPS_DELAY;
    }

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  if (FPSdelay > 128U) {
    /* вечерело */
    FPSdelay--;
    sizeH = (uint8_t)((FPSdelay - 128U) * stepH);
    // ESP_LOGD("Firework", PSTR("• [%03d] | %03d | %0.2f | \n"), FPSdelay, stepH, sizeH);
    dimAll(200);

    const uint8_t sky_bri = (uint8_t)(FPSdelay * 0.4545f);  // / 2.2f
    if ((ORIENTATION & 0x01U) == 0U) {
      gradientDownTop(0, CHSV(skyColor, 255U, sky_bri), sizeH, CHSV(skyColor, 255U, 2U));
    } else {
      gradientVertical(0, 0, WIDTH, sizeH, skyColor, skyColor, sky_bri, 2U, 255U);
    }

    if (sizeH > HORIZONT) return;
    if (sizeH == HORIZONT) FPSdelay = FPS_DELAY;
  }

  if (step > DOT_EXPLOSION) {
    blurScreen(beatsin8(3U, 64U, 80U));
  }
  if (step == (uint8_t)(DOT_EXPLOSION - 1U)) {
    /* включаем фазу затухания */
    FPSdelay = 70U;
  }
  dimAll((step > CENTER_Y_MAJOR) ? 140U : 100U);

  /* ============ draw sky =========== */
  if (modes[currentMode].Speed < 180U) {
    if ((ORIENTATION & 0x01U) == 0U) {
      gradientDownTop(0, CHSV(skyColor, 255U, hue), HORIZONT, CHSV(skyColor, 255U, 0U));
    } else {
      gradientVertical(0, 0, WIDTH, HORIZONT, skyColor, skyColor, (uint8_t)(hue + 1U), 0U, 255U);
    }
  }

  /* deltaHue2 - Firework type */
  VirtualExplosion(deltaHue2, step);

  if ((step > DOT_EXPLOSION) && (step < PHASE_V1)) {
    /* фаза взрыва */
    FPSdelay += 5U;
  }

  constexpr uint8_t rows = (uint8_t)((HEIGHT + 1U) / 3U);
  deltaHue = (modes[currentMode].Speed >> 6U) << 6U; // / 64) * 64;
  if (step > CENTER_Y_MAJOR) {
    bool dir = false;
    for (uint8_t y = 0U; y < rows; y++) {
      const uint8_t row_offset = y * 3U + DELTA;
      /* сдвигаем слои / эмитация разлета */
      for (uint8_t x = 0U; x < WIDTH; x++) {
        if (dir) {  // <==
          drawPixelXY((int16_t)(x - 1U), row_offset, getPixColorXY(x, row_offset));
        } else {    // ==>
          drawPixelXY((int16_t)(WIDTH - x), row_offset, getPixColorXY((int16_t)(WIDTH - x - 1U), row_offset));
        }
      }
      dir = !dir;
      /* --------------------------------- */
    }
  }

  /* ========== фаза полета ========== */
  if (step < DOT_EXPLOSION) {
    FPSdelay++;
    if (HEIGHT < 20U) {
      FPSdelay++;
    }
    /* закоментируйте следующие две строки если плоская лампа
      подсветка заднего фона */
    if (custom_eff == 1) {
      const uint8_t line_h = HEIGHT - step;
      DrawLine(0U, 0U, 0U, line_h, CHSV(skyColor, 255U, 32U));
      DrawLine(MAX_X, 0U, MAX_X, line_h, CHSV(skyColor, 255U, 32U));
    }
    /* ------------------------------------------------------ */

    const uint8_t saturation = (step > (uint8_t)(DOT_EXPLOSION - 2U)) ? 192U : 20U;
    drawPixelXY((int16_t)(CENTER_X_MINOR + deltaHue2), step, CHSV(50U, saturation, 80U));
    drawPixelXY((int16_t)(CENTER_X_MAJOR - deltaHue2), (int16_t)(step - HORIZONT), CHSV(50U, saturation, 80U));

    /* sky brightness */
    if (hue > 2U) {
      hue -= 1U;
    }
  }

  if (step > PHASE_V2) {
    /* sky brightness */
    if (hue < MAX_BRIGHTNESS) {
      hue += 2U;
    }
  }

  if (step >= PHASE_LIMIT) {
    step = 0U;
    FPSdelay = FPS_DELAY;
    if (modes[currentMode].Scale <= 1U) {
      deltaHue2++;
    }
    if (deltaHue2 >= 4U) deltaHue2 = 0U;    // next Firework type
  }

  // ESP_LOGD("Firework", PSTR("• [%03d] | %03d | sky Bright • [%03d]\n"), step, FPSdelay, hue);
  step ++;
}
#endif


#ifdef DEF_FIREWORK_2
//---------- Эффект "Фейерверк" Салют ---
// адаптация и переписал - kostyamat
// https://gist.github.com/jasoncoon/0cccc5ba7ab108c0a373
// https://github.com/marcmerlin/FastLED_NeoMatrix_SmartMatrix_LEDMatrix_GFX_Demos/blob/master/FastLED/FireWorks2/FireWorks2.ino

constexpr uint8_t  MODEL_BORDER = HEIGHT - 4U;                              // как далеко за экран может вылетить снаряд, если снаряд вылетает за экран, то всышка белого света (не особо логично)
constexpr uint16_t MODEL_WIDTH = MODEL_BORDER + WIDTH  + MODEL_BORDER;      // не трогать, - матиматика
constexpr uint16_t MODEL_HEIGHT = MODEL_BORDER + HEIGHT + MODEL_BORDER;     // -//-
constexpr uint16_t PIXEL_X_OFFSET = (MODEL_WIDTH  - WIDTH) / 2U;            // -//-
constexpr uint16_t PIXEL_Y_OFFSET = (MODEL_HEIGHT - HEIGHT) / 2U;           // -//-

constexpr uint8_t SPARK = 8U;                                               // максимальное количество снарядов
constexpr uint8_t NUM_SPARKS = WIDTH;                                       // количество разлетающихся петард (частей снаряда)

constexpr saccum78 gGravity = 10;
constexpr fract8  gBounce = 127;
constexpr fract8  gDrag = 255;

constexpr uint8_t SCALE_M_W = 256U / MODEL_WIDTH;
constexpr uint8_t SCALE_M_H = 256U / MODEL_HEIGHT;
constexpr uint8_t SCALE_ERR_W = 255U / MODEL_WIDTH;
constexpr uint8_t SCALE_ERR_H = 255U / MODEL_HEIGHT;

typedef struct _DOTS_STORE {
  accum88 gBurstx;
  accum88 gBursty;
  saccum78 gBurstxv;
  saccum78 gBurstyv;
  CRGB gBurstcolor;
  bool gSkyburst = false;
} DOTS_STORE;
static DOTS_STORE store[SPARK];

static CRGB overrun;
static CRGB& piXY(uint8_t x, uint8_t y) {
  const int16_t real_x = x - PIXEL_X_OFFSET;
  const int16_t real_y = y - PIXEL_Y_OFFSET;

  if ((uint16_t)real_x < WIDTH && (uint16_t)real_y < HEIGHT) {
    return leds[XY(real_x, real_y)];
  }
  return overrun;
}

class Dot {    // класс для создания снарядов и питард
  public:
    uint8_t  show;
    uint8_t  theType;
    accum88  x;
    accum88  y;
    saccum78 xv;
    saccum78 yv;
    accum88  r;
    CRGB     color;

    Dot() {
      show = 0;
      theType = 0;
      x = 0; y =  0;
      xv = 0; yv = 0;
      r  = 0;
      color.setRGB(0, 0, 0);
    }

    void Draw() {
      if(!show) return;

      // Оптимизированный screenscale для X
      const uint8_t ia_x = x >> 8U;
      const uint8_t ix = scale8(ia_x, MODEL_WIDTH);
      const uint8_t xc = 255U - ((ia_x - (ix * SCALE_M_W)) * SCALE_ERR_W);

      // Оптимизированный screenscale для Y
      const uint8_t ia_y = y >> 8U;
      const uint8_t iy = scale8(ia_y, MODEL_HEIGHT);
      const uint8_t yc = 255U - ((ia_y - (iy * SCALE_M_H)) * SCALE_ERR_H);

      const uint8_t ye = 255U - yc;
      const uint8_t xe = 255U - xc;

      piXY(ix, iy) += CRGB(
        dim8_video(scale8(scale8(color.r, yc), xc)),
        dim8_video(scale8(scale8(color.g, yc), xc)),
        dim8_video(scale8(scale8(color.b, yc), xc))
      );
      piXY(ix, iy + 1U) += CRGB(
        dim8_video(scale8(scale8(color.r, ye), xc)),
        dim8_video(scale8(scale8(color.g, ye), xc)),
        dim8_video(scale8(scale8(color.b, ye), xc))
      );
      piXY(ix + 1U, iy) += CRGB(
        dim8_video(scale8(scale8(color.r, yc), xe)),
        dim8_video(scale8(scale8(color.g, yc), xe)),
        dim8_video(scale8(scale8(color.b, yc), xe))
      );
      piXY(ix + 1U, iy + 1U) += CRGB(
        dim8_video(scale8(scale8(color.r, ye), xe)),
        dim8_video(scale8(scale8(color.g, ye), xe)),
        dim8_video(scale8(scale8(color.b, ye), xe))
      );
    }

    void Move(uint8_t num, bool Flashing) {
      if(!show) return;

      yv -= gGravity;

      xv = (int16_t)((int32_t)(xv * gDrag) >> 8U);
      yv = (int16_t)((int32_t)(yv * gDrag) >> 8U);

      if (theType == 2U) {
        xv = (int16_t)((int32_t)(xv * gDrag) >> 8U);
        yv = (int16_t)((int32_t)(yv * gDrag) >> 8U);
        color.nscale8(255U);
        if (!color) show = 0U;
      }

      // if we'd hit the ground, bounce
      if (yv < 0 && (y < (-yv))) {
        if (theType == 2U) {
          show = 0U;
        } else {
          yv = -yv;
          yv = (int16_t)((int32_t)(yv * gBounce) >> 8U);
          if (yv < 500) show = 0U;
        }
      }

      if (yv < -300) {
        // pinnacle
        if (theType == 1U) {
          if ((y > (uint16_t)0x8000U) && (random8() < 32U) && Flashing) {
            ledsClear();
          }

          show = 0U;
          store[num].gSkyburst = true;
          store[num].gBurstx = x;
          store[num].gBursty = y;
          store[num].gBurstxv = xv;
          store[num].gBurstyv = yv;
          store[num].gBurstcolor = CRGB(random8(), random8(), random8());
        }
      }

      if (theType == 2U) {
        if (((xv > 0) && (x > xv)) || ((xv < 0) && (x < (uint16_t)(0xFFFFU + xv)))) {
          x += xv;
        } else {
          show = 0U;
        }
      } else {
        x += xv;
      }
      y += yv;
    }

    void GroundLaunch() {
      yv = 600 + random16(400U + (25U * HEIGHT));
      if (yv > 1200) yv = 1200;
      xv = (int16_t)random16(600U) - 300;
      y = 0;
      x = 0x8000U;
      color = CHSV(0U, 0U, 130U); // цвет запускаемого снаряда
      show = 1U;
    }

    void Skyburst(accum88 basex, accum88 basey, saccum78 basedv, const CRGB& basecolor, uint8_t dim) {
      yv = (int16_t)random16(1500U) - 500;
      xv = basedv + (int16_t)random16(2000U) - 1000;
      y = basey;
      x = basex;
      color = basecolor;
      color *= dim;
      theType = 2U;
      show = 1U;
    }
};

static uint16_t launchcountdown[SPARK];
static Dot gDot[SPARK];
static Dot gSparks[NUM_SPARKS];

// bool flashing = true; // нахрен эти вспышки прямо в коде false напишу

static void sparkGen() {
  const uint8_t speed_factor = modes[currentMode].Speed * 4U;

  for (uint8_t c = 0U; c < enlargedObjectNUM; c++) {
    if (gDot[c].show == 0U) {
      if (launchcountdown[c] == 0U) {
        gDot[c].GroundLaunch();
        gDot[c].theType = 1U;
        launchcountdown[c] = random16(1200U - speed_factor) + 1U;
      } else {
        launchcountdown[c]--;
      }
    }

    if (store[c].gSkyburst) {
      store[c].gBurstcolor = CHSV(random8(), 200U, 100U);
      store[c].gSkyburst = false;

      const uint8_t nsparks = random8(NUM_SPARKS / 2U, NUM_SPARKS + 1U);
      for (uint8_t b = 0U; b < nsparks; b++) {
        gSparks[b].Skyburst(store[c].gBurstx, store[c].gBursty, store[c].gBurstyv, store[c].gBurstcolor, pcnt);
      }
    }
  }
}

static void fireworksRoutine()
{
  if (loadingFlag) {
    constexpr float inv99 = 1.0f / 99.0f;
    enlargedObjectNUM = (modes[currentMode].Scale - 1U) * inv99 * (SPARK - 1U) + 1U;
    if (enlargedObjectNUM > SPARK) enlargedObjectNUM = SPARK;

    for (uint8_t c = 0U; c < SPARK; c++) {
      launchcountdown[c] = 0U;
    }

    loadingFlag = false;
  }

  // random16_add_entropy(analogRead(A0));
  pcnt = beatsin8(100U, 20U, 100U);

  if (hue++ % 10U == 0U) {
    deltaValue = random8(25U, 50U);
  }

  fadeToBlackBy(leds, NUM_LEDS, deltaValue);
  sparkGen();

  for (uint8_t a = 0U; a < enlargedObjectNUM; a++) {
    gDot[a].Move(a, false);     // flashing);
    gDot[a].Draw();
  }
  for (uint8_t b = 0U; b < NUM_SPARKS; b++) {
    gSparks[b].Move(0U, false); // flashing);
    gSparks[b].Draw();
  }
}
#endif


#ifdef DEF_HOURGLASS
// ============= Hourglass ==============
//             © SlingMaster
//             EFF_HOURGLASS
//             Песочные Часы
//---------------------------------------

static void Hourglass() {
  constexpr float SIZE = 0.4f;
  constexpr uint8_t h = (uint8_t)(SIZE * HEIGHT);
  constexpr uint8_t route = HEIGHT - h - 1U;
  constexpr uint8_t STEP = 18U;

  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                           scale | speed 210
      setModeSettings(15U + random8(225U), random8(255U));
    }
#endif

    pcnt = 0U;
    deltaHue2 = 0U;
    hue2 = 0U;

    hue = modes[currentMode].Scale * 2.55f;
    for (uint8_t x = 0U; x < CENTER_X; x++) {
      const uint8_t bri = 255U - (x * STEP);
      for (uint8_t y = 0U; y < h; y++) {
        drawPixelXY(CENTER_X_MINOR - x, MAX_Y - y, CHSV(hue, 255U, bri));
        drawPixelXY(CENTER_X_MAJOR + x, MAX_Y - y, CHSV(hue, 255U, bri));
      }
    }

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  uint8_t posX = 0;

  if (hue2 == 0U) {
    posX = pcnt / 2U;
    const uint8_t posY = HEIGHT - h - pcnt;
    // ESP_LOGD(PSTR("Hourglass", "• [%03d] | posX %03d | deltaHue2 %03d | \n"), step, posX, deltaHue2);

    /* move sand -------- */
    if ((posY < (uint8_t)(HEIGHT - h - 2U)) && (posY > deltaHue2)) {
      const CRGB sand_col = CHSV(hue, 255U, 255U);
      drawPixelXY(CENTER_X_MAJOR, posY, sand_col);
      drawPixelXY(CENTER_X_MAJOR, posY - 2U, sand_col);
      drawPixelXY(CENTER_X_MAJOR, posY - 4U, sand_col);

      if (posY < (uint8_t)(HEIGHT - h - 3U)) {
        drawPixelXY(CENTER_X_MAJOR, posY + 1U, CHSV(hue, 255U, 0U));
      }
    }

    /* draw body hourglass */
    const uint8_t sand_bri = 255U - (posX * STEP);
    if ((pcnt & 0x01U) == 0U) {
      drawPixelXY(CENTER_X_MAJOR - posX, HEIGHT - deltaHue2 - 1U, CHSV(hue, 255U, 0U));
      drawPixelXY(CENTER_X_MAJOR - posX, deltaHue2, CHSV(hue, 255U, sand_bri));
    } else {
      drawPixelXY(CENTER_X_MAJOR + posX, HEIGHT - deltaHue2 - 1U, CHSV(hue, 255U, 0U));
      drawPixelXY(CENTER_X_MAJOR + posX, deltaHue2, CHSV(hue, 255U, sand_bri));
    }

    if (pcnt > MAX_X) {
      deltaHue2++;
      pcnt = 0U;
      if (modes[currentMode].Scale > 95U) {
        hue += 4U;
      }
    }

    pcnt++;
    if (deltaHue2 > h) {
      deltaHue2 = 0U;
      hue2 = 1U;
    }
  }

  // имитация переворота песочных часов
  if (hue2 > 0U) {
    for (uint8_t x = 0U; x < WIDTH; x++) {
      for (uint8_t y = MAX_Y; y > 0U; y--) {
        drawPixelXY(x, y, getPixColorXY(x, y - 1U));
      }
      drawPixelXY(x, 0U, 0x000000);
    }
    hue2++;
    if (hue2 > route) {
      hue2 = 0U;
    }
  }
}
#endif


#ifdef DEF_FLOWERRUTA
// =====================================
//            Flower Ruta
//    © Stepko and © Sutaburosu
//     Adaptation © SlingMaster
//       Modifed © alvikskor
// =====================================

static void FlowerRuta() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                         scale | speed
      setModeSettings(random8(11U, 69U), random8(150U, 255U));
    }
#endif

    for (int16_t x = -CENTER_X_MAJOR; x < CENTER_X_MAJOR; x++) {
      for (int16_t y = -CENTER_Y_MAJOR; y < CENTER_Y_MAJOR; y++) {
        noise3d[0U][x + CENTER_X_MAJOR][y + CENTER_Y_MAJOR] = (atan2(x, y) * invPI) * 128.0f + 127.0f;  // thanks ldirko
        noise3d[1U][x + CENTER_X_MAJOR][y + CENTER_Y_MAJOR] = hypot(x, y);                              // thanks Sutaburosu
      }
    }

    deltaValue = modes[currentMode].Scale / 10U; // deltaValue - Petals
    deltaHue = modes[currentMode].Scale % 10U;

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  if (deltaHue < 5U) {
    ff_x = scale / (3U - (deltaHue >> 1U)); // ff_x - color_speed
  } else {
    ff_x = scale * ((deltaHue >> 1U) - 1U);
  }

  scale++;

  constexpr uint8_t rad_step = 255U / WIDTH;
  for (uint8_t x = 0U; x < WIDTH; x++) {
    for (uint8_t y = 0U; y < HEIGHT; y++) {
      const uint8_t angle = noise3d[0U][x][y];
      const uint8_t radius = noise3d[1U][x][y];

      const uint8_t rad_offset = radius * rad_step;
      const uint8_t angle_petals = angle * deltaValue;

      const uint8_t index = sin8(sin8((uint8_t)(scale + angle_petals + rad_offset)) + (uint8_t)(scale * 4U) + sin8((uint8_t)(scale * 4U - rad_offset)) + angle_petals);

      leds[XY(x, y)] = CHSV((uint8_t)(ff_x + rad_offset), 255U, index);
    }
  }

}
#endif


#ifdef DEF_MAGIC_LANTERN
// ============ Magic Lantern ===========
//             © SlingMaster
//            Чарівний Ліхтар
// --------------------------------------
static void MagicLantern() {
  constexpr uint8_t PADDING = QUARTER_Y;
  constexpr uint8_t WARM_LIGHT = 55U;
  constexpr uint8_t STEP = 4U;

  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed 210
      setModeSettings(random8(100U), random8(40, 200U));
    }
#endif

    deltaValue = 0;
    step = deltaValue;

    // deltaHue2 = brightness, deltaHue = low_br, pcnt = saturation
    if (modes[currentMode].Speed > 52U) {
      deltaHue2 = map(modes[currentMode].Speed, 1U, 255U, 50U, 250U);
      deltaHue = 50U;
    } else {
      deltaHue2 = 0U;
      deltaHue = 0U;
    }
    pcnt = (modes[currentMode].Scale > 50U) ? 64U : 0U;
    if (std::abs(70 - (int16_t)modes[currentMode].Scale) <= 5) {
      pcnt = 170U;
    }

    hue2 = (modes[currentMode].Speed < 25U) ? 1U : 0U;

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  dimAll(170);

  hue = (modes[currentMode].Scale > 95U) ? (uint8_t)((step >> 5U) * 32U) : (uint8_t)(modes[currentMode].Scale * 2.55f);

  // ------
  for (uint8_t x = 0U; x <= WIDTH; x++) {
    const int16_t x_offset = x - deltaValue;
    const int16_t x_inv_offset = WIDTH - x + deltaValue;

    // light ---
    if (deltaHue > 0U) {
      gradientVertical(x_offset, CENTER_Y_MAJOR, x_offset + 1U, HEIGHT - PADDING - 1U, WARM_LIGHT, WARM_LIGHT, deltaHue2, deltaHue, pcnt);
      gradientVertical(x_inv_offset, CENTER_Y_MAJOR, x_inv_offset + 1U, HEIGHT - PADDING - 1U, WARM_LIGHT, WARM_LIGHT, deltaHue2, deltaHue, pcnt);
      gradientVertical(x_offset, PADDING + 1U, x_offset + 1U, CENTER_Y_MAJOR, WARM_LIGHT, WARM_LIGHT, (uint8_t)(deltaHue + 10U), deltaHue2, pcnt);
      gradientVertical(x_inv_offset, PADDING + 1U, x_inv_offset + 1U, CENTER_Y_MAJOR, WARM_LIGHT, WARM_LIGHT, (uint8_t)(deltaHue + 10U), deltaHue2, pcnt);
    } else {
      if (x % (STEP + 1U) == 0U) {
        leds[XY(random8(WIDTH), random8((uint8_t)(PADDING + 2U), (uint8_t)(HEIGHT - PADDING - 2U)))] = CHSV((uint8_t)(step - 32U), random8(128U, 255U), 255U);
      }
      if ((hue2 == 1U) && (deltaHue == 0U)) {
        deltaValue = 0U;
        if ((x % 2U) != 0U) {
          gradientVertical(x_offset, HEIGHT - PADDING, x_offset + 1U, HEIGHT, hue, (uint8_t)(hue + 2U), 64U, 20U, 255U);
          gradientVertical(x_inv_offset, 0U, x_inv_offset + 1U, PADDING, hue, hue, 42U, 64U, 255U);
        }
      }
    }

    if (x % STEP == 0U) {
      // body --
      gradientVertical(x_offset, HEIGHT - PADDING, x_offset + 1U, HEIGHT, hue, (uint8_t)(hue + 2U), 255U, 20U, 255U);
      gradientVertical(x_inv_offset, 0U, x_inv_offset + 1U, PADDING, hue, hue, 42U, 255U, 255U);
    }
  }
  // ------

  deltaValue++;
  if (deltaValue >= STEP) {
    deltaValue = 0U;
  }

  step++;
}
#endif


#ifdef DEF_MOSAIC
// ----------------------------- Эффект Мозайка / Кафель ------------------------------
// (c) SottNick
// на основе идеи Idir
// https://editor.soulmatelights.com/gallery/843-squares-and-dots

static void squaresNdotsRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
        //                          scale | speed
        setModeSettings(1U + random8(100U), 1U + random8(255U));
      }
    #endif

    shtukX = THIRD_X + 1U;
    shtukY = THIRD_Y + 1U;
    poleX = modes[currentMode].Speed % 3U;
    poleY = (modes[currentMode].Speed / 3U) % 3U;

    deltaHue = (modes[currentMode].Scale & 0x01U) ? 1U : 0U;
    deltaValue = modes[currentMode].Scale * 2.55f;

    for (uint8_t i = 0U; i < shtukX; i++)
      line[i] = random8(3U);
    for (uint8_t i = 0U; i < shtukY; i++)
      shiftValue[i] = random8(3U);

    loadingFlag = false;
  }

  const bool type = random8() & 0x01U;  // random8(2U);
  const CRGB color = CHSV(random8(), (uint8_t)(255U - random8(deltaValue)), 255U);

  const uint8_t i = random8(shtukX);
  const uint8_t j = random8(shtukY);

  const int16_t x0 = i * 3U + (poleX + (deltaHue ? line[j] : 0U)) % 3U - 2;
  const int16_t y0 = j * 3U + (poleY + (deltaHue ? 0U : shiftValue[i])) % 3U - 2;
  uint8_t hole = 0U;

  for (int16_t x = x0; x < x0 + 3; x++) {
    for (int16_t y = y0; y < y0 + 3; y++) {
      drawPixelXY(x, y, ((hole == 4U) ^ type) ? CRGB::Black : color);
      hole++;
    }
  }
}
#endif


#ifdef DEF_OCTOPUS
// ============ Octopus ===========
//        © Stepko and Sutaburosu
//    Adapted and modifed © alvikskor
//             Восьминіг
// --------------------------------------
// Idea from https://www.youtube.com/watch?v=HsA-6KIbgto&ab_channel=GreatScott%21

static void Octopus() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                          scale | speed
      setModeSettings(random8(10U, 101U), random8(150U, 255U));
    }
    #endif

    for (int16_t x = -CENTER_X_MAJOR; x < CENTER_X_MAJOR + ((int8_t)WIDTH % 2); x++) {
      for (int16_t y = -CENTER_Y_MAJOR; y < CENTER_Y_MAJOR + ((int8_t)HEIGHT % 2); y++) {
        // ИСПРАВЛЕНО: Запись строго в свои слои 0U и 1U трехмерного массива noise3d
        noise3d[0U][x + CENTER_X_MAJOR][y + CENTER_Y_MAJOR] = (atan2(x, y) * invPI) * 128.0f + 127.0f;  // thanks ldirko
        noise3d[1U][x + CENTER_X_MAJOR][y + CENTER_Y_MAJOR] = hypot(x, y);                              // thanks Sutaburosu
      }
    }

    deltaValue = modes[currentMode].Scale / 10U; // deltaValue - legs
    deltaHue = modes[currentMode].Scale % 10U;   // deltaHue - step

    loadingFlag = false;
  }

  if (deltaHue < 5U) {
    ff_x = scale / (3U - (deltaHue >> 1U));  // ff_x - color_speed
  } else {
    ff_x = scale * ((deltaHue >> 1U) - 1U);
  }

  scale++;

  constexpr uint8_t rad_step = 255U / WIDTH;
  for (uint8_t x = 0U; x < WIDTH; x++) {
    for (uint8_t y = 0U; y < HEIGHT; y++) {
      const uint8_t angle = noise3d[0U][x][y];
      const uint8_t radius = noise3d[1U][x][y];

      const uint8_t rad_offset = radius * rad_step;
      const uint8_t angle_legs = angle * deltaValue;

      const uint8_t index = sin8(sin8((uint8_t)(((angle * 4U) - rad_offset) >> 2U) + scale) + rad_offset - (uint8_t)(scale * 2U) + angle_legs);

      leds[XY(x, y)] = CHSV((uint8_t)(ff_x - rad_offset), 255U, index);
    }
  }
}
#endif


#ifdef DEF_PAINTS
// ============ Oil Paints ==============
//      © SlingMaster | by Alex Dovby
//              EFF_PAINT
//           Масляные Краски
//---------------------------------------
static void OilPaints() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                          scale | speed 210
      setModeSettings(1U + random8(252U), 1 + random8(219U));
    }
    #endif

    deltaValue = 255U - modes[currentMode].Speed + 1U;
    step = deltaValue;                                                        // чтообы при старте эффекта сразу покрасить лампу
    hue = (random8(11U) + 1U) * 21.25f;                                       // next color
    deltaHue = hue - 22U;                                                     // last color
    deltaHue2 = 80U - (uint8_t)(logf(modes[currentMode].Brightness) * 6.0f);  // min bright
    uint32_t calc_max = (1U << WIDTH) - 1U;
    ff_z = (calc_max > 65535U) ? 65535U : (uint16_t)calc_max;                 // ff_z - max_val

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  if (step >= deltaValue) {
    step = 0U;
    // ESP_LOGD("OilPaints", PSTR("%03d | log: %f | val: %03d\n\r"), modes[currentMode].Brightness, log(modes[currentMode].Brightness), deltaHue2);
  }

  // Create Oil Paints --------------
  // выбираем краски  ---------------
  if (step % CENTER_Y_MINOR == 0) {
    const uint8_t divider = (modes[currentMode].Scale - 1U) / 10U;    // маштаб задает диапазон изменения цвета
    deltaHue = hue;                                                   // set last color
    hue += 6U * divider;                                              // new color
    hue2 = 255U;                                                      // restore brightness

    const uint8_t entry_point = random8(WIDTH);                       // start X position
    trackingObjectHue[entry_point] = hue;                             // set start position

    drawPixelXY(entry_point, HEIGHT - 2U, CHSV(hue, 255U, 255U));
    // !!! ********
    if (custom_eff == 1) {
      drawPixelXY((uint8_t)(entry_point + 1U), HEIGHT - 3U, CHSV((uint8_t)(hue + 30U), 255U, 255U));
    }
    // ************
    // ESP_LOGD("OilPaints", PSTR("BR %03d | SP %03d | SC %03d | hue %03d\n\r"), modes[currentMode].Brightness, modes[currentMode].Speed, modes[currentMode].Scale, hue);
  }

  // формируем форму краски, плавно расширяя струю ----
  if (random8(3U) == 1U) {
    // ESP_LOGD("OilPaints", "<--");
    for (uint8_t x = 1U; x < WIDTH; x++) {
      if (trackingObjectHue[x] == hue) {
        trackingObjectHue[x - 1U] = hue;
        break;
      }
    }
  } else {
    // ESP_LOGD("OilPaints", "-->");
    for (uint8_t x = (uint8_t)(MAX_X - 1U); x > 0U; x--) {
      if (trackingObjectHue[x] == hue) {
        trackingObjectHue[x + 1U] = hue;
        break;
      }
      // ESP_LOGD("OilPaints", PSTR("x = %02d | value = %03d | hue = %03d \n\r"), x, trackingObjectHue[x], hue);
    }
  }
  // ESP_LOGD("OilPaints", "------------------------------------");

  // выводим сформированную строку --------------------- максимально яркую в момент смены цвета
  for (uint8_t x = 0U; x < WIDTH; x++) {
    const uint8_t current_bri = (trackingObjectHue[x] == hue) ? hue2 : deltaHue2;
    drawPixelXY(x, MAX_Y, CHSV(trackingObjectHue[x], 255U, current_bri));
  }

  // уменьшаем яркость для следующих строк
  if (hue2 > (uint8_t)(deltaHue2 + 16U)) {
    hue2 -= 16U;
  }

  // сдвигаем неравномерно поток вниз ---
  const uint16_t mask_value = random16(ff_z);
  // ESP_LOGD("OilPaints", PSTR("value = %06d | "), mask_value);
  for (uint8_t x = 0U; x < WIDTH; x++) {
    if (bitRead(mask_value, x) == 0U) {
      // ESP_LOGD("OilPaints", " X");
      for (uint8_t y = 0U; y < MAX_Y; y++) {
        drawPixelXY(x, y, getPixColorXY(x, y + 1U));
      }
    }
  }
  // ESP_LOGD("OilPaints", PSTR("%02d | hue2 = %03d | min = %03d \n\r"), step, hue2, deltaHue2);
  // -------------------------------------

  step++;
}
#endif


#ifdef DEF_PLASMA_WAVES
// ============ Plasma Waves ============
//              © Stepko
//        Adaptation © alvikskor
//             Плазмові Хвилі
// --------------------------------------

static void Plasma_Waves() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), random8(40, 200U));
    }
#endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    hue = modes[currentMode].Scale / 10U;

    ff_z = 0U; // Сбрасываем глобальный счетчик - frameCount
    deltaValue = 132U - (modes[currentMode].Speed >> 1U);

    loadingFlag = false;
  }

  FPSdelay = 1U;  // 64 - modes[currentMode].Speed / 4;

  ff_z++;

  const uint8_t t1 = cos8((42U * ff_z) / deltaValue);
  const uint8_t t2 = cos8((35U * ff_z) / deltaValue);
  const uint8_t t3 = cos8((38U * ff_z) / deltaValue);

  const uint8_t t1_div2 = t1 >> 1U;
  const uint8_t t3_div2 = t3 >> 2U;

  const uint8_t current_scale = modes[currentMode].Scale;

  for (uint8_t y = 0U; y < HEIGHT; y++) {
    const uint8_t y8 = y << 3U;
    const uint8_t g_y_phase = y8 + t1 + current_scale;
    const uint8_t b_y_phase = y8 + t2;

    for (uint8_t x = 0U; x < WIDTH; x++) {
      // Calculate 3 seperate plasma waves, one for each color channel
      uint8_t r = cos8((x << 3U) + t1_div2 + cos8((uint8_t)(t2 + y8 + current_scale)));
      uint8_t g = cos8(g_y_phase + cos8((uint8_t)(t3_div2 + (x << 3U))));
      uint8_t b = cos8(b_y_phase + cos8((uint8_t)(t1 + x + (g >> 2U) + current_scale)));

      switch (hue) {
        case 0U:
          r = pgm_read_byte(&exp_gamma[r]);
          g = pgm_read_byte(&exp_gamma[g]);
          b = pgm_read_byte(&exp_gamma[b]);
          break;
        case 1U:
          r = pgm_read_byte(&exp_gamma[r]);
          b = pgm_read_byte(&exp_gamma[g]);
          g = pgm_read_byte(&exp_gamma[b]);
          break;
        case 2U:
          g = pgm_read_byte(&exp_gamma[r]);
          r = pgm_read_byte(&exp_gamma[g]);
          b = pgm_read_byte(&exp_gamma[b]);
          break;
        case 3U:
          r = pgm_read_byte(&exp_gamma[r]) >> 1U;
          g = pgm_read_byte(&exp_gamma[g]);
          b = pgm_read_byte(&exp_gamma[b]);
          break;
        case 4U:
          r = pgm_read_byte(&exp_gamma[r]);
          g = pgm_read_byte(&exp_gamma[g]) >> 1U;
          b = pgm_read_byte(&exp_gamma[b]);
          break;
        case 5U:
          r = pgm_read_byte(&exp_gamma[r]);
          g = pgm_read_byte(&exp_gamma[g]);
          b = pgm_read_byte(&exp_gamma[b]) >> 1U;
          break;
        case 6U:
          r = (uint8_t)(pgm_read_byte(&exp_gamma[r]) * 3U);
          g = pgm_read_byte(&exp_gamma[g]);
          b = pgm_read_byte(&exp_gamma[b]);
          break;
        case 7U:
          r = pgm_read_byte(&exp_gamma[r]);
          g = (uint8_t)(pgm_read_byte(&exp_gamma[g]) * 3U);
          b = pgm_read_byte(&exp_gamma[b]);
          break;
        default: // 8U и все остальные
          r = pgm_read_byte(&exp_gamma[r]);
          g = pgm_read_byte(&exp_gamma[g]);
          b = (uint8_t)(pgm_read_byte(&exp_gamma[b]) * 3U);
          break;
      }

      leds[XY(x, y)] = CRGB(r, g, b);
    }
  }
}
#endif


#ifdef DEF_RADIAL_WAVE
// =====================================
//              RadialWave
//            Радіальна хвиля
//               © Stepko
// =====================================

static void RadialWave() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                         scale | speed
      setModeSettings(random(10U, 101U), random(150U, 255U));
    }
    #endif

    for (int16_t x = -CENTER_X_MAJOR; x < CENTER_X_MAJOR + ((int8_t)WIDTH % 2); x++) {
      for (int16_t y = -CENTER_Y_MAJOR; y < CENTER_Y_MAJOR + ((int8_t)HEIGHT % 2); y++) {
        noise3d[0U][x + CENTER_X_MAJOR][y + CENTER_Y_MAJOR] = (atan2(x, y) * invPI) * 128.0f + 127.0f;  // thanks ldirko
        noise3d[1U][x + CENTER_X_MAJOR][y + CENTER_Y_MAJOR] = hypot(x, y);                              // thanks Sutaburosu
      }
    }

    deltaValue = modes[currentMode].Scale / 10U; // legs
    deltaHue = modes[currentMode].Scale % 10U;   // Временный шаг

    loadingFlag = false;
  }

  if (deltaHue < 5U) {
    ff_x = scale / (3U - (deltaHue >> 1U));
  } else {
    ff_x = scale * ((deltaHue >> 1U) - 1U);
  }

  scale++;

  constexpr uint8_t rad_step = 255U / WIDTH;
  for (uint8_t x = 0U; x < WIDTH; x++) {
    for (uint8_t y = 0U; y < HEIGHT; y++) {
      const uint8_t angle = noise3d[0U][x][y];
      const uint8_t radius = noise3d[1U][x][y];

      const uint8_t rad_offset = radius * rad_step;
      const uint8_t angle_legs = angle * deltaValue;

      const uint8_t index = sin8((uint8_t)(scale * 4U) + sin8((uint8_t)(scale * 4U - rad_offset)) + angle_legs);

      leds[XY(x, y)] = CHSV((uint8_t)(ff_x + rad_offset), 255U, index);
    }
  }
}
#endif


#ifdef DEF_RIVERS
// ========== Botswana Rivers ===========
//      © SlingMaster | by Alex Dovby
//              EFF_RIVERS
//            Реки Ботсваны
//---------------------------------------

static void flora() {
  constexpr uint32_t FLORA_COLOR = 0x2F1F00;
  constexpr uint8_t posX = (CENTER_X_MINOR > THIRD_X) ? (CENTER_X_MINOR - THIRD_X) : 0U;

  uint8_t h = random8((uint8_t)(HEIGHT - 6U)) + 4U;

  DrawLine(posX + 1U, 1U, posX + 1U, h - 1U, 0x000000);
  DrawLine(posX + 2U, 1U, posX + 2U, h, FLORA_COLOR);

  drawPixelXY(posX + 2U, h - random8((uint8_t)(h / 2U)), random8(2U) == 1U ? 0xFF00E0 : random8(2U) == 1U ? 0xFFFF00 : 0x00FF00);
  drawPixelXY(posX + 1U, h - random8((uint8_t)(h / 4U)), random8(2U) == 1U ? 0xFF00E0 : 0xFFFF00);

  if (random8(2U) == 1U) {
    drawPixelXY(posX + 1U, h / 2U, random8(2U) == 1U ? 0xEF001F : 0x9FFF00);
  }

  h = ((uint16_t)h * 166U) >> 8U;  // h = floor(h * 0.65); // Быстрое умножение на 0.65 через сдвиг

  if (WIDTH > 8U) {
    DrawLine(posX - 1U, 1U, posX - 1U, h - 1U, 0x000000);
  }

  DrawLine(posX, 1U, posX, h, FLORA_COLOR);
  drawPixelXY(posX, h - random8((uint8_t)(h / 2U)), random8(2U) == 1U ? 0xFF00E0 : 0xFFFF00);
}

//---------------------------------------
static void animeBobbles() {
  constexpr uint32_t BOBBLE_COLOR = 0xFFFFF7;

  // сдвигаем всё вверх ----
  for (uint8_t x = CENTER_X_MAJOR; x < WIDTH; x++) {
    for (uint8_t y = MAX_Y; y > 0U; y--) {
      if (getPixColorXY(x, y - 1U) == BOBBLE_COLOR) {
        drawPixelXY(x, y, BOBBLE_COLOR);
        drawPixelXY(x, y - 1U, getPixColorXY(0U, y - 1U));
      }
    }
  }

  if (step % 4U == 0U) {
    drawPixelXY((uint8_t)(CENTER_X_MAJOR + random8(5U)), 0U, BOBBLE_COLOR);
    if (step % 12U == 0U) {
      drawPixelXY((uint8_t)(CENTER_X_MAJOR + 2U + random8(3U)), 0U, BOBBLE_COLOR);
    }
  }
}

//---------------------------------------
static void createScene(uint8_t idx) {
  const uint8_t MID = MAX_Y / 2U;                     // floor((H - 1) * 0.5)
  const uint8_t H3  = (uint16_t)(HEIGHT * 3U) / 10U;  // floor(H * 0.3)

  switch (idx) {
    case 0:     // blue green ------
      gradientDownTop(MID, CHSV(96, 255, 100), HEIGHT, CHSV(160, 255, 255));
      gradientDownTop(0, CHSV(96, 255, 255), CENTER_Y_MINOR, CHSV(96, 255, 100));
      break;
    case 1:     // aquamarine green
      gradientDownTop(H3, CHSV(96, 255, 100), HEIGHT, CHSV(130, 255, 220));
      gradientDownTop(0, CHSV(96, 255, 255), H3, CHSV(96, 255, 100));
      break;
    case 2:     // blue aquamarine -
      gradientDownTop(MID, CHSV(170, 255, 100), HEIGHT, CHSV(160, 255, 200));
      gradientDownTop(0, CHSV(100, 255, 255), CENTER_Y_MINOR, CHSV(170, 255, 100));
      break;
    case 3:     // yellow green ----
      gradientDownTop(MID, CHSV(95, 255, 55), HEIGHT, CHSV(70, 255, 200));
      gradientDownTop(0, CHSV(95, 255, 255), CENTER_Y_MINOR, CHSV(100, 255, 55));
      break;
    case 4:     // sea green -------
      gradientDownTop(H3, CHSV(120, 255, 55), HEIGHT, CHSV(175, 255, 200));
      gradientDownTop(0, CHSV(120, 255, 255), H3, CHSV(120, 255, 55));
      break;
    default:
      gradientDownTop(QUARTER_Y, CHSV(180, 255, 85), HEIGHT, CHSV(160, 255, 200));
      gradientDownTop(0, CHSV(80, 255, 255), QUARTER_Y, CHSV(180, 255, 85));
      break;
  }
  flora();
}

//---------------------------------------
static void createSceneM(uint8_t idx) {
  const uint8_t H3 = (uint16_t)(HEIGHT * 3U) / 10U; // замена floor(HEIGHT * 0.3)

  switch (idx) {
    case 0:     // blue green ------
      gradientVertical(0, CENTER_Y_MINOR, WIDTH, HEIGHT, 96, 150, 100, 255, 255U);
      gradientVertical(0, 0, WIDTH, CENTER_Y_MINOR, 96, 96, 255, 100, 255U);
      break;
    case 1:     // aquamarine green
      gradientVertical(0, H3, WIDTH, HEIGHT, 96, 120, 100, 220, 255U);
      gradientVertical(0, 0, WIDTH, H3, 96, 96, 255, 100, 255U);
      break;
    case 2:     // blue aquamarine -
      gradientVertical(0, CENTER_Y_MINOR, WIDTH, HEIGHT, 170, 160, 100, 200, 255U);
      gradientVertical(0, 0, WIDTH, CENTER_Y_MINOR, 100, 170, 255, 100, 255U);
      break;
    case 3:     // yellow green ----
      gradientVertical(0, CENTER_Y_MINOR, WIDTH, HEIGHT, 95, 65, 55, 200, 255U);
      gradientVertical(0, 0, WIDTH, CENTER_Y_MINOR, 95, 100, 255, 55, 255U);
      break;
    case 4:     // sea green -------
      gradientVertical(0, H3, WIDTH, HEIGHT, 120, 160, 55, 200, 255U);
      gradientVertical(0, 0, WIDTH, H3, 120, 120, 255, 55, 255U);
      break;
    default:
      drawRec(0, 0, WIDTH, HEIGHT, 0x000050);
      break;
  }
  flora();
}

//---------------------------------------
static void BotswanaRivers() {
  // альтернативный градиент для ламп собраных из лент с вертикальной компоновкой
  // для корректной работы ALT_GRADIENT = true
  // для ламп из лент с горизонтальной компоновкой и матриц ALT_GRADIENT = false
  // ALT_GRADIENT = false более производительный и более плавная растяжка
  //------------------------------------------------------------------------------
  // static const bool ALT_GRADIENT = true;

  #define ALT_GRADIENT (0U)

  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                          scale | speed 210
      setModeSettings(1U + random8(252U), 20 + random8(180U));
    }
#endif

    deltaValue = 255U - modes[currentMode].Speed + 1U;
    step = deltaValue;                                          // чтообы при старте эффекта сразу покрасить лампу
    hue = (modes[currentMode].Scale - 1U) / 20U;                // divider - маштаб задает смену палитры воды

    #if (ALT_GRADIENT == 1U)
      createSceneM(hue);
    #else
      createScene(hue);
    #endif

    loadingFlag = false;
  }

  if (step >= deltaValue) {
    step = 0U;
  }

  // restore scene after power on ---------
  if (getPixColorXY(0U, HEIGHT - 2U) == CRGB::Black) {
    #if (ALT_GRADIENT == 1U)
      createSceneM(hue);
    #else
      createScene(hue);
    #endif
  }

  // light at the bottom ------------------
  #if (ALT_GRADIENT == 0U)
    if ((step & 0x01U) == 0U) {
      if (random8(6U) == 1U) {
        if (ORIENTATION < 3U || ORIENTATION == 7U) {
          fill_gradient(leds, 0, CHSV(96U, 255U, 190U), random8((uint8_t)(WIDTH + random8(6U))), CHSV(90U, 200U, 255U), fl::SHORTEST_HUES);
        } else {
          fill_gradient(leds, NUM_LEDS - random8((uint8_t)(WIDTH + random8(6U))), CHSV(96U, 255U, 190U), NUM_LEDS, CHSV(90U, 200U, 255U), fl::SHORTEST_HUES);
        }
      } else {
        if (ORIENTATION < 3U || ORIENTATION == 7U) {
          fill_gradient(leds, 0, CHSV(85U, 128U, 255U), random8(WIDTH), CHSV(90U, 255U, 180U), fl::SHORTEST_HUES);
        } else {
          fill_gradient(leds, NUM_LEDS - random8(WIDTH), CHSV(85U, 128U, 255U), NUM_LEDS, CHSV(90U, 255U, 180U), fl::SHORTEST_HUES);
        }
      }
    }
  #endif

  // LOG.printf_P(PSTR("%02d | hue2 = %03d | min = %03d \n\r"), step, hue2, deltaHue2);
  // -------------------------------------
  animeBobbles();

  if (custom_eff == 1) {
    blurRows(WIDTH, 3U, 10U);
  }

  step++;
}
#endif


#ifdef DEF_SPECTRUM
// ============ Spectrum New ==============
//             © SlingMaster
//         source code © kostyamat
//                Spectrum
//---------------------------------------
static void  Spectrum() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                        scale | speed
      setModeSettings(random8(1, 100U), random8(215, 255U));
    }
#endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    ff_y = map(WIDTH, 8U, 64U, 310U, 63U);
    ff_z = ff_y;
    speedfactor = map(modes[currentMode].Speed, 1U, 255U, 32U, 4U);
    pcnt = (uint8_t)((modes[currentMode].Scale - 1U) * 2.55f);  // customHue

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  uint8_t color = pcnt + hue;
  const uint8_t scale_odd = modes[currentMode].Scale & 0x01U;

  if (modes[currentMode].Scale >= 99U) {
    hue2++; deltaHue++; deltaHue2++;
    if ((hue2 & 0x01U) && (deltaHue & 0x01U) && (deltaHue2 & 0x01U)) {
      hue += 8U;
    }
    fillMyPal16((uint8_t)(pcnt + hue), scale_odd, MBAuroraColors_arr);
  } else {
    color = pcnt;
    fillMyPal16((uint8_t)(pcnt + AURORA_COLOR_RANGE - beatsin8(AURORA_COLOR_PERIOD, 0U, (uint8_t)(AURORA_COLOR_RANGE * 2U))), scale_odd, MBAuroraColors_arr);
  }

  const float emitterX = (((random8() & 0x01U) == 0U) ? 545.0f : 390.0f) / HEIGHT;
  const uint32_t timer_div_speed = polarTimer / speedfactor;

  constexpr float inv_height = 1.0f / HEIGHT;

  for (uint8_t x = 0U; x < WIDTH; x++) {
    if ((x & 0x01U) == 0U) {
      leds[XY(x, 0U)] = CHSV(color, 255U, 128U);
    }

    const uint16_t x_phase = x * ff_z;
    const float emitterX = (((random8() & 0x01U) == 0U) ? 545.0f : 390.0f) * inv_height;

    for (uint8_t y = 2U; y < MAX_Y; y++) {
      polarTimer++;

      const uint8_t noise = fastled_helper::perlin8(
        (uint16_t)((polarTimer & 0x01U) + x_phase),
        (uint8_t)((y << 4U) + (polarTimer & 0x0FU)),
        timer_div_speed
      );
      const uint8_t fade_y = std::abs(CENTER_Y_F - (float)y) * emitterX;

      leds[XY(x, y)] = ColorFromPalette(myPal, qsub8(noise, fade_y));
    }
  }
}
#endif


#ifdef DEF_STROBE
// =====================================
//            Строб Хаос Дифузия
//          Strobe Haos Diffusion
//             © SlingMaster
// =====================================
/* должен быть перед эффектом Матрицf бегунок Скорость не регулирует задержку между кадрами,
   но меняет частоту строба */
static void StrobeAndDiffusion() {
  // constexpr uint8_t SIZE = 3U;
  constexpr uint8_t DELTA = 1U;         // центровка по вертикали

  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                          scale | speed
      setModeSettings(1U + random8(100U), 1U + random8(150U));
    }
#endif

    FPSdelay = 25U; // LOW_DELAY;
    hue2 = 1;

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  const uint8_t speed_val = modes[currentMode].Speed;
  const uint8_t scale_val = modes[currentMode].Scale;

  uint8_t STEP = ((uint8_t)(255U - speed_val) / 64U) + 1U; // for strob

  if (scale_val > 50U) {
    // diffusion ---
    blurScreen(beatsin8(3U, 64U, 80U));
    FPSdelay = LOW_DELAY;
    STEP = 1U;
    if (scale_val < 75U) {
      // chaos ---
      FPSdelay = 30U;
      VirtualSnow(1U);
    }
  } else {
    // strob -------
    if (scale_val > 25U) {
      dimAll(200U);
      FPSdelay = 30U;
    } else {
      dimAll(240U);
      FPSdelay = 40U;
    }
  }

  constexpr uint8_t rows = (HEIGHT + 1) / 3U;
  deltaHue = (speed_val >> 6U) << 6U;  // modes[currentMode].Speed / 64 * 64

  bool dir = false;
  for (uint8_t y = 0U; y < rows; y++) {
    const uint8_t target_y = y * 3U + DELTA;

    if (dir) { // <==
      if ((step % STEP) == 0U) {  // small layers
        drawPixelXY(MAX_X, target_y, CHSV(step, 255U, 255U));
      } else {
        drawPixelXY(MAX_X, target_y, CHSV(170U, 255U, 1U));
      }

      for (uint8_t x = 1U; x < WIDTH; x++) {
        drawPixelXY((int16_t)(x - 1U), target_y, getPixColorXY(x, target_y));
      }
    } else { // ==>
      if ((step % STEP) == 0U) {  // big layers
        drawPixelXY(0U, target_y, CHSV((uint8_t)(step + deltaHue), 255U, 255U));
      } else {
        drawPixelXY(0U, target_y, CHSV(0U, 255U, 0U));
      }

      for (uint8_t x = 1U; x < WIDTH; x++) {
        const int16_t inv_x = WIDTH - x;
        drawPixelXY(inv_x, target_y, getPixColorXY((int16_t)(inv_x - 1U), target_y));
      }
    }

    dir = !dir;
  }

  if (hue2 == 1U) {
    step++;
    if (step >= 254U) hue2 = 0U;
  } else {
    step--;
    if (step < 1U) hue2 = 1U;
  }
}
#endif


#ifdef DEF_SPINDLE
// ============== Spindle ==============
//             © SlingMaster
//          adapted © alvikskor
//               Веретено
// =====================================
static void Spindle() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                         scale | speed
      setModeSettings(random8(1U, 100U), random8(100U, 255U));
    }
#endif

    hue = random8(8U) * 32U; // modes[currentMode].Scale;
    hue2 = 255U;

    loadingFlag = false;
  }

  const uint8_t scale_val = modes[currentMode].Scale;

  if (scale_val < 81U) {
    blurScreen(128U);
  } else if (scale_val < 86U) {
    blurScreen(96U);
  } else if (scale_val < 91U) {
    blurScreen(64U);
  } else if (scale_val < 96U) {
    blurScreen(32U);
  }

  CHSV color;
  color.hue = hue;

  // <==== scroll =====
  for (uint8_t y = 0U ; y < HEIGHT; y++) {
    for (uint8_t x = 0U ; x < MAX_X; x++) {
      hue2--;

      if (scale_val < 76U) {  // black delimiter -----
        color.sat = 255U;
        color.val = hue2;
      } else {               // white delimiter -----
        color.sat = 64U + (hue2 >> 1U);   // / 2U
        color.val = 255U - (hue2 >> 2U);  // / 4U
      }

      drawPixelXY(MAX_X, y, color);
      drawPixelXY(x, y, getPixColorXY(x + 1U, y));
    }
  }

  if (scale_val < 56U) {
    return;
  }

  if (scale_val < 61U) {
    hue += 1U;
  } else if (scale_val < 66U) {
    hue += 2U;
  } else if (scale_val < 71U) {
    hue += 3U;
  } else if (scale_val < 76U) {
    hue += 4U;
  } else {
    hue += 3U;
  }
}
#endif


#ifdef DEF_SWIRL
// ============== Swirl ================
//    © SlingMaster | by Alex Dovby
//              EFF_SWIRL
//--------------------------------------
static void Swirl() {
  static const uint32_t colors[5][6] PROGMEM = {
    {CRGB::Blue, CRGB::DarkRed, CRGB::Aqua, CRGB::Magenta, CRGB::Gold, CRGB::Green },
    {CRGB::Yellow, CRGB::LemonChiffon, CRGB::LightYellow, CRGB::Gold, CRGB::Chocolate, CRGB::Goldenrod},
    {CRGB::Green, CRGB::DarkGreen, CRGB::LawnGreen, CRGB::SpringGreen, CRGB::Cyan, CRGB::Black },
    {CRGB::Blue, CRGB::DarkBlue, CRGB::MidnightBlue, CRGB::MediumSeaGreen, CRGB::MediumBlue, CRGB::DeepSkyBlue },
    {CRGB::Magenta, CRGB::Red, CRGB::DarkMagenta, CRGB::IndianRed, CRGB::Gold, CRGB::MediumVioletRed }
  };

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                           scale | speed
      setModeSettings(50U + random8(190U), 250U);
    }
    #endif

    deltaValue = 255U - modes[currentMode].Speed + 1U;
    step = deltaValue;                                // чтообы при старте эффекта сразу покрасить лампу

    deltaHue2 = 0U;                                   // Координата Y вихря
    hue2 = 0U;                                        // Координата X вихря
    hue = 0U;                                         // Индекс текущего цвета
    pcnt = 0U;                                        // lastHue

    deltaHue = (modes[currentMode].Scale - 1U) / 20U; // маштаб задает смену палитры

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  if (step >= deltaValue) {
    step = 0U;
  }

  // задаем цвет и рисуем завиток --------
  const uint32_t color = pgm_read_dword(&(colors[deltaHue][hue]));
  drawPixelXY(hue2, deltaHue2, color);

  hue2++;                     // x
  // два варианта custom_eff задается в сетапе лампы ----
  if (custom_eff == 1) {
    deltaHue2++;              // y
  } else {
    if ((hue2 & 0x01U) == 0U) {
      deltaHue2++;            // y
    }
  }
  // -------------------------------------

  if (hue2 >= WIDTH) {
    hue2 = 0U;
  }

  if (deltaHue2 >= HEIGHT) {
    deltaHue2 = 0U;

    // new swirl ------------
    hue2 = random8((uint8_t)(WIDTH - 2U));

    // select new color -----
    hue = random8(6U);

    if (pcnt == hue) {
      hue++;
      if (hue >= 6U) {
        hue = 0U;
      }
    }
    pcnt = hue;
  }

  blurScreen((uint8_t)(4U + random8(8U)));

  step++;
}
#endif


#ifdef DEF_TORNADO
// =====================================
//           Rainbow Tornado
//  base code © Stepko, © Sutaburosu
//        and © SlingMaster
//   adapted and modifed © alvikskor
//              Торнадо
// =====================================

constexpr uint8_t OFFSET = 1U;
constexpr uint8_t H = HEIGHT - OFFSET;

static void Tornado() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                           scale | speed
      setModeSettings(random8(100U, 255U), random8(20U, 100U));
    }
#endif

    for (int16_t x = -CENTER_X_MAJOR; x < CENTER_X_MAJOR; x++) {
      for (int16_t y = -OFFSET; y < H; y++) {
        noise3d[0U][x + CENTER_X_MAJOR][y + OFFSET] = 128.0f * (atan2(y, x) * invPI);
        noise3d[1U][x + CENTER_X_MAJOR][y + OFFSET] = hypot(x, y);  // thanks Sutaburosu
      }
    }

    deltaValue = modes[currentMode].Scale / 10U;
    deltaHue = modes[currentMode].Speed / 10U;

    loadingFlag = false;
  }

  scale += deltaHue;

  constexpr uint16_t sat_step = 512U / HEIGHT;
  constexpr uint8_t edge_y = HEIGHT / 8U;

  for (uint8_t x = 0U; x < WIDTH; x++) {
    for (uint8_t y = 0U; y < HEIGHT; y++) {
      uint8_t angle = noise3d[0][x][y];
      uint8_t radius = noise3d[1][x][y];
      leds[XY(x, y)] = CHSV((angle * modes[currentMode].Scale / 10) - scale + (radius * modes[currentMode].Scale / 10), min(((uint16_t)y*512U/(uint16_t)HEIGHT),255U), (y < (HEIGHT/8) ? 255 - (((HEIGHT/8) - y) * 16) : 255));
    }
  }

  for (uint8_t y = 0U; y < HEIGHT; y++) {
    const uint16_t sat_calc = y * sat_step;
    const uint8_t saturation = (sat_calc > 255U) ? 255U : (uint8_t)sat_calc;

    const uint8_t brightness = (y < edge_y) ? (uint8_t)(255U - ((edge_y - y) << 4U)) : 255U;

    for (uint8_t x = 0U; x < WIDTH; x++) {
      const uint8_t angle = noise3d[0U][x][y];
      const uint8_t radius = noise3d[1U][x][y];

      const uint8_t hue_index = (angle * deltaValue) - scale + (radius * deltaValue);

      leds[XY(x, y)] = CHSV(hue_index, saturation, brightness);
    }
  }
}
#endif


#ifdef DEF_WATERCOLOR
// ============ Watercolor ==============
//      © SlingMaster | by Alex Dovby
//            EFF_WATERCOLOR
//               Акварель
//---------------------------------------
static void SmearPaint(uint8_t *obj) {
  static const uint32_t colors[6][8] PROGMEM = {
    {0x2F0000,  0xFF4040, 0x6F0000, 0xAF0000, 0xff5f00, CRGB::Red, 0x480000, 0xFF0030},
    {0x002F00, CRGB::LawnGreen, 0x006F00, 0x00AF00, CRGB::DarkMagenta, 0x00FF00, 0x004800, 0x00FF30},
    {0x002F1F, CRGB::DarkCyan, 0x00FF7F, 0x007FFF, 0x20FF5F, CRGB::Cyan, 0x004848, 0x7FCFCF },
    {0x00002F, 0x5030FF, 0x00006F, 0x0000AF, CRGB::DarkCyan, 0x0000FF, 0x000048, 0x5F5FFF},
    {0x2F002F, 0xFF4040, 0x6F004A, 0xFF0030, CRGB::DarkMagenta, CRGB::Magenta, 0x480048, 0x3F00FF},
    {CRGB::Blue, CRGB::Red, CRGB::Gold, CRGB::Green, CRGB::DarkCyan, CRGB::DarkMagenta, 0x000000, 0xFF7F00 }
  };

  if (trackingObjectHue[5] == 1U) {  // direction >>>
    obj[1]++;
    if (obj[1] >= obj[2]) {
      trackingObjectHue[5] = 0U;     // swap direction
      obj[3]--;                      // new line
      if ((step & 0x01U) == 0U) {
        obj[1]++;
      } else {
        obj[1]--;
      }
      obj[0]--;
    }
  } else {                           // direction <<<
    obj[1]--;
    if (obj[1] <= (obj[2] - obj[0])) {
      trackingObjectHue[5] = 1U;     // swap direction
      obj[3]--;                      // new line
      if (obj[0] >= 1U) {
        obj[0] = (obj[0] > 1U) ? (uint8_t)(obj[0] - 1U) : 0U;
        obj[1]++;
      }
    }
  }

  if (obj[3] == 255U) {
    deltaHue = 255U;
  }

  if ((obj[1] >= WIDTH) || (obj[3] == obj[4])) {
    // deltaHue value == 255 activate -------
    // set new parameter for new smear ------
    deltaHue = 255U;
  }

  // divider = floor((modes[currentMode].Scale - 1) / 16.7f);
  // drawPixelXY(obj[1], obj[3], colors[divider][hue]);
  const uint32_t color = pgm_read_dword(&(colors[deltaHue2][hue]));
  drawPixelXY(obj[1], obj[3], color);

  // alternative variant without dimmer effect
  // uint8_t h = obj[3] - obj[4];
  // uint8_t br = 266U - 12U * h;
  // if (h > 0U) {
  // drawPixelXY(obj[1], obj[3], makeDarker(color, br));
  // } else {
  // drawPixelXY(obj[1], obj[3], makeDarker(color, 240U));
  // }
}

//---------------------------------------
static void Watercolor() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                          scale | speed 250
      setModeSettings(1U + random8(252U), 1 + random8(250U));
    }
#endif

    deltaValue = 255U - modes[currentMode].Speed + 1U;
    step = deltaValue;                    // Чтобы при старте эффекта сразу покрасить лампу
    deltaHue = 255U;                      // last color
    hue = 0U;

    trackingObjectHue[1] = QUARTER_X;
    trackingObjectHue[3] = QUARTER_Y;

    deltaHue2 = (uint8_t)((modes[currentMode].Scale - 1U) * 0.05988f);
    if (deltaHue2 > 5U) deltaHue2 = 5U;

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  if (step >= deltaValue) {
    step = 0U;
    // LOG.printf_P(PSTR("%03d | log: %f | val: %03d | divider: %d \n\r"), modes[currentMode].Brightness, log(modes[currentMode].Brightness), deltaHue2, divider);
  }

  // ******************************
  // set random parameter for smear
  // ******************************
  if (deltaHue == 255U) {
    trackingObjectHue[0] = 4U + random8(QUARTER_X);                             // width

    trackingObjectHue[1] = random8((uint8_t)(WIDTH - trackingObjectHue[0]));    // x
    uint8_t temp = trackingObjectHue[1] + trackingObjectHue[0];
    if (temp >= MAX_X) {
      temp = MAX_X;
      if (trackingObjectHue[1] > 1U) {
        trackingObjectHue[1]--;
      } else {
        trackingObjectHue[1]++;
      }
    }
    trackingObjectHue[2] = temp;                                                // x end

    trackingObjectHue[3] = 3U + random8((uint8_t)(HEIGHT - 4U));                // y
    const uint8_t r_sub = random8(3U) + 3U;
    trackingObjectHue[4] = (trackingObjectHue[3] > r_sub) ?
                           (uint8_t)(trackingObjectHue[3] - r_sub) : 0U;        // y end

    trackingObjectHue[5] = 1U;
    //divider = floor((modes[currentMode].Scale - 1) / 16.7);                   // маштаб задает смену палитры
    hue = random8(8);
    // if (step % 127 == 0) {
    //   ESP_LOGD("Watercolor", "BR %03d | SP %03d | SC %03d | divider %d | [ %d ]", modes[currentMode].Brightness, modes[currentMode].Speed, modes[currentMode].Scale, divider, hue);
    // }
    hue2 = 255U;
    deltaHue = 0U;
  }
  // ******************************

  SmearPaint(trackingObjectHue);

  // LOG.printf_P(PSTR("%02d | hue2 = %03d | min = %03d \n\r"), step, hue2, deltaHue2);
  // -------------------------------------
  //  if (custom_eff == 1) {
  // dimAll(DIMSPEED);
  if ((step & 0x01U) == 0U) {
    blurScreen(beatsin8(1U, 1U, 6U));
    // blurRows(WIDTH, 3U, 10U);
  }
  //  }

  step++;
}
#endif


#ifdef DEF_WEB_TOOLS
// =====================================
//             Мечта Дизайнера
//                WebTools
//             © SlingMaster
// =====================================
/* --------------------------------- */
static int16_t getRandomPos(uint8_t STEP) {
  const uint8_t max_rnd = (uint8_t)(STEP * 16U - MAX_X);
  const uint8_t val = (random8(max_rnd) / STEP) * STEP;
  return -(int16_t)val;
}

/* --------------------------------- */
static uint8_t  getHue(uint8_t x, uint8_t y) {
  return (uint8_t)(x * 32U + y * 24U);
}

/* --------------------------------- */
static uint8_t getSaturationStep() {
  // return (modes[currentMode].Speed > 170U) ? ((HEIGHT > 24) ? 12 : 24) : 0;
  return pcnt;
}

/* --------------------------------- */
static uint8_t getBrightnessStep() {
  // return (modes[currentMode].Speed < 85U) ? ((HEIGHT > 24) ? 16 : 24) : 0;
  return deltaValue;
}

/* --------------------------------- */
static void drawPalette(int16_t posX, int16_t posY, uint8_t STEP) {
  const uint8_t SZ = STEP - 1U;
  const uint8_t maxY = HEIGHT / SZ;

  const uint8_t sat = getSaturationStep();
  const uint8_t br  = getBrightnessStep();

  ledsClear(); // esphome: FastLED.clear();

  for (uint8_t y = 0U; y < maxY; y++) {
    const int16_t PY = y * STEP;

    if (PY >= -(int16_t)STEP && PY < HEIGHT) {
      const uint8_t current_sat = (uint8_t)(255U - sat * y);
      const uint8_t current_br  = (uint8_t)(240U - br * y);

      for (uint8_t x = 0U; x < 16U; x++) {
        const int16_t PX = posX + x * STEP;
        if (PX >= -(int16_t)STEP && PX < WIDTH) {
          // LOG.printf_P(PSTR("y: %03d | br • %03d | sat • %03d\n"), y, (240U - br * y), sat);
          drawRecCHSV(PX, PY, PX + SZ, PY + SZ, CHSV(getHue(x, y), current_sat, current_br));
        }
      }
    }
  }
}

/* --------------------------------- */
static void selectColor(uint8_t sc) {
  constexpr uint8_t offset = (WIDTH >= 16U) ? QUARTER_X : 0U;

  hue = getHue(offset + random8((uint8_t)(WIDTH - (offset << 1U))), random8(HEIGHT));

  const uint8_t sat = getSaturationStep();
  const uint8_t br  = getBrightnessStep();

  for (uint8_t y = 0U; y < HEIGHT; y++) {
    const uint8_t current_sat = (uint8_t)(255U - sat * y);
    const uint8_t current_br  = (uint8_t)(240U - br * y);

    for (uint8_t x = offset; x < (uint8_t)(WIDTH - offset); x++) {
      const CHSV curColor = CHSV(hue, current_sat, current_br);

      if (CRGB(curColor) == getPixColorXY(x, y)) {
        /* show select color */
        drawRecCHSV(x, y, x + sc, y + sc, CHSV(hue, 64U, 255U));
        drawRecCHSV(x, y, x + sc, y + sc, CHSV(hue, 255U, 255U));
        return;
      }
    }
  }
}

/* --------------------------------- */
static void WebTools() {
  constexpr uint8_t FPS_D = 24U;
  const uint8_t speed = (modes[currentMode].Speed > 65U) ? modes[currentMode].Speed : 65U;

  // static uint8_t STEP = 3U;           // deltaHue             => Смысловой шаг STEP
  // static int posX = -STEP;            // trackingObjectPosX   => Координата posX (приводим к int16_t для знака)
  // static int posY = 0;                // trackingObjectPosY   => Координата posY (приводим к int16_t для знака)
  // static int nextX = -STEP * 2;       // trackingObjectSpeedX => Целевая координата nextX (приводим к int16_t для знака)
  // static bool stop_moving = true;     // trackingObjectState  => Флаг stop_moving (1U - стоим, 0U - движемся)

  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
       //                        scale | speed
       setModeSettings(random(10U, 90U), random(10U, 255U));
    }
#endif

    FPSdelay = 1U;
    step = 0;

    deltaHue = 2U + (modes[currentMode].Scale / 35U);

    trackingObjectPosX[0] = 0.0f;
    trackingObjectPosY[0] = 0.0f;

    trackingObjectSpeedX[0] = -(int16_t)(deltaHue * 2U);
    trackingObjectState[0] = 1U;                                                        // stop_moving = true

    pcnt = (modes[currentMode].Speed > 170U) ? ((HEIGHT > 24U) ? 12U : 24U) : 0U;       // sat_step
    deltaValue = (modes[currentMode].Speed < 85U) ? ((HEIGHT > 24U) ? 16U : 24U) : 0U;  // br_step

    drawPalette((int16_t)trackingObjectPosX[0], (int16_t)trackingObjectPosY[0], deltaHue);

    loadingFlag = false;
  }

  int16_t posX = (int16_t)trackingObjectPosX[0];
  int16_t nextX = (int16_t)trackingObjectSpeedX[0];

  /* auto scenario */
  if (step == 0U) {                                  /* restart ----------     */
    nextX = 0;
    FPSdelay = FPS_D;
  } else if (step == (uint8_t)(speed / 16U + 1U)) {  /* start move -------  16 */
    nextX = getRandomPos(deltaHue);
    FPSdelay = FPS_D;
  } else if (step == (uint8_t)(speed / 10U + 1U)) {  /* find ------------- 100 */
    nextX = getRandomPos(deltaHue);
    FPSdelay = FPS_D;
  } else if (step == (uint8_t)(speed / 7U + 1U)) {   /* find 2 ----------- 150 */
    nextX = getRandomPos(deltaHue);
    FPSdelay = FPS_D;
  } else if (step == (uint8_t)(speed / 6U + 1U)) {   /* find 3 ----------- 200 */
    nextX = -(int16_t)(deltaHue * random8(4U, 8U));
    FPSdelay = FPS_D;
  } else if (step == (uint8_t)(speed / 5U + 1U)) {   /* select color ----- 220 */
    FPSdelay = 200U;
    selectColor((uint8_t)(deltaHue - 1U));
  } else if (step == (uint8_t)(speed / 4U + 1U)) {   /* show color ------- 222 */
    FPSdelay = FPS_D;
    nextX = WIDTH;
  } else if (step == (uint8_t)(speed / 4U + 3U)) {
    step = 252U;
  }

  if (posX < nextX) posX++;
  if (posX > nextX) posX--;

  trackingObjectPosX[0] = posX;
  trackingObjectSpeedX[0] = nextX;

  if (trackingObjectState[0] == 1U) {  // stop_moving
    FPSdelay = 80U;
    step++;
  } else {
    drawPalette(posX, (int16_t)trackingObjectPosY[0], deltaHue);

    if ((nextX == (int16_t)WIDTH) || (nextX == 0)) {
      /* show select color bar gradient */
      // LOG.printf_P(PSTR("step: %03d | Next x: %03d • %03d | fps %03d\n"), step, nextX, posX, FPSdelay);
      if (posX > 1) {
        gradientHorizontal(0, 0, (uint8_t)(posX - 1), HEIGHT, hue, hue, 255U, 96U, 255U);
      }
      if (posX > 3) {
        DrawLine((uint8_t)(posX - 3), CENTER_Y_MINOR, (uint8_t)(posX - 3), CENTER_Y_MAJOR, CHSV(hue, 192U, 255U));
      }
    }
  }

  trackingObjectState[0] = (posX == nextX) ? 1U : 0U;  // stop_moving = (posX == nextX);
}
#endif


#ifdef DEF_WINE
// =============== Wine ================
//    © SlingMaster | by Alex Dovby
//               EFF_WINE
//--------------------------------------

static void colorsWine() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                           scale | speed
      setModeSettings(20U + random8(200U), 200U);
    }
#endif

    fillAll(CHSV(55U, 255U, 65U));

    step = 255U - modes[currentMode].Speed + 1U;

    // minspeed 230 maxspeed 250 ============
    // minscale  40 maxscale  75 ============
    // красное вино hue > 0 & <=10
    // розовое вино hue > 10 & <=20
    // белое вино   hue > 20U & <= 40
    // шампанское   hue > 40U & <= 60

    deltaHue2 = 0U;                                             // count для замедления смены цвета
    deltaHue = 1U;                                              // direction | 0 hue-- | 1 hue++ |
    hue = 55U;                                                  // Start Color
    hue2 = 65U;                                                 // Brightness
    pcnt = 0U;                                                  // Координата пузырька перляжа

    deltaValue = 5U - ((modes[currentMode].Scale - 1U) / 20U);  // маштаб задает скорость изменения цвета 5 уровней

    loadingFlag = false;
  }

  deltaHue2++;

  // возвращаем яркость для перехода к белому
  if (hue >= 10U && hue2 < 100U) {
    hue2++;
  }

  // уменьшаем яркость для красного вина
  if (hue < 10U && hue2 > 40U) {
    hue2--;
  }

  // изменение цвета вина -----
  if (deltaHue == 1U) {
    if (deltaHue2 % deltaValue == 0U) {
      hue++;
    }
  } else {
    if (deltaHue2 % deltaValue == 0U) {
      hue--;
    }
  }
  // --------

  // LOG.printf_P(PSTR("Wine | hue = %03d | Dir = %d | Brightness %03d | deltaHue2 %03d | divider %d | %d\n"), hue, deltaHue, hue2, deltaHue2, divider, step);

  // сдвигаем всё вверх -----------
  for (uint8_t x = 0U; x < WIDTH; x++) {
    for (uint8_t y = MAX_Y; y > 0U; y--) {
      drawPixelXY(x, y, getPixColorXY(x, y - 1U));
    }
  }

  // Добавляем перляж (пузырьки) исключительно для шампанского (hue > 40)
  if (hue > 40U) {
    pcnt = random8(WIDTH);
  } else {
    pcnt = 0U;
  }

  // заполняем нижнюю строку с учетом перляжа
  for (uint8_t x = 0U; x < WIDTH; x++) {
    if ((x == pcnt) && (pcnt > 0U)) {
      // Пузырек шампанского (чуть светлее и прозрачнее)
      drawPixelXY(x, 0U, CHSV(hue, 150U, (uint8_t)(hue2 + 20U + random8(50U))));
    } else {
      // Стандартная толща вина
      drawPixelXY(x, 0U, CHSV(hue, 255U, hue2));
    }
  }

  // меняем направление изменения цвета вина от красного к шампанскому и обратно
  // в диапазоне шкалы HUE |0-60|
  if (hue == 0U) {
    deltaHue = 1U;
  }
  if (hue == 60U) {
    deltaHue = 0U;
  }
}
#endif


#ifdef DEF_UKRAINE
// ============== Ukraine ==============
//      © SlingMaster | by Alex Dovby
//              EFF_UKRAINE
//--------------------------------------

// -------------------------------------------
// for effect Ukraine
// -------------------------------------------
static void drawCrest() {
  static const uint32_t data[9][5] PROGMEM = {
    {0x000000, 0x000000, 0xFFD700, 0x000000, 0x000000 },
    {0xFFD700, 0x000000, 0xFFD700, 0x000000, 0xFFD700 },
    {0xFFD700, 0x000000, 0xFFD700, 0x000000, 0xFFD700 },
    {0xFFD700, 0x000000, 0xFFD700, 0x000000, 0xFFD700 },
    {0xFFD700, 0x000000, 0xFFD700, 0x000000, 0xFFD700 },
    {0xFFD700, 0xFFD700, 0xFFD700, 0xFFD700, 0xFFD700 },
    {0xFFD700, 0x000000, 0xFFD700, 0x000000, 0xFFD700 },
    {0x000000, 0xFFD700, 0xFFD700, 0xFFD700, 0x000000 },
    {0x000000, 0x000000, 0xFFD700, 0x000000, 0x000000 }
  };

  constexpr uint8_t posX = CENTER_X_MAJOR - 3U;
  constexpr uint8_t posY = (HEIGHT > 16U) ? (uint8_t)(CENTER_Y_MINOR - 1U) : 9U;

  ledsClear(); // esphome: FastLED.clear();

  for (uint8_t y = 0U; y < 9U; y++) {
    for (uint8_t x = 0U; x < 5U; x++) {
      const uint32_t color = pgm_read_dword(&(data[y][x]));
      drawPixelXY((uint8_t)(posX + x), (uint8_t)(posY - y), color);
    }
  }
}

static void Ukraine() {
  constexpr uint8_t timeout = 100U;

  static const uint32_t colors[2][5] = {
    {CRGB::Blue, CRGB::MediumBlue, 0x0F004F, 0x02002F, 0x1F2FFF },
    {CRGB::Yellow, CRGB::Gold, 0x4E4000, 0xFF6F00, 0xFFFF2F }
  };

  // deltaHue = предрассчитанный индекс режима рестарта (divider)
  // pcnt     = динамический предел таймаута (tMAX)

  // Initialization =========================
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(250U), 200U + random8(50U));
    }
#endif

    drawCrest();

    // minspeed 200 maxspeed 250 ============
    // minscale   0 maxscale 100 ============
    deltaValue = 255U - modes[currentMode].Speed + 1U;

    deltaHue2 = 0U;    // Координата Y текущего мазка флага
    hue2 = 0U;         // Координата X текущего мазка флага
    hue = 0U;          // Текущий цветовой индекс палитры флага
    ff_x = 1U;         // Общий счетчик жизненного цикла сцены

    deltaHue = (modes[currentMode].Scale - 1U) / 10U;
    pcnt = timeout + 100U * deltaHue;

    loadingFlag = false;
  }

  // Фаза плавного размытия Тризуба перед началом прорисовки флага
  if ((ff_x > (uint16_t)(timeout - 10U)) && (ff_x < timeout)) {   // таймаут блокировки отрисовки флага
    if (ff_x < (uint16_t)(timeout - 5U)) {                        // размытие тризуба
      blurScreen(beatsin8(5U, 60U, 5U));
    } else {
      blurScreen((uint8_t)(210U - ff_x));
    }
  }

  // Проверка наступления фазы автоматического рестарта сцены
  if (ff_x > pcnt) {
    if (deltaHue == 0U) {                                         // Режим 0: Тризуб рисуется только один раз при включении
      ff_x = 0U;
      pcnt += 20U;
    } else {
      if (ff_x > (uint16_t)(pcnt + 100U * deltaHue)) {            // Режим >0: Циклический перезапуск эффекта
        drawCrest();
        ff_x = 1U;
      }
    }
  }

  if ((ff_x != 0U) || (deltaHue > 0U)) {
    ff_x++;
  }

  // Flag Draw =============================
  if ((ff_x > timeout) || (ff_x == 0U)) {                         // отрисовка флага
    if (step >= deltaValue) {
      step = 0U;
      hue2 = random8((uint8_t)(WIDTH - 2U));                      // Случайное начальное смещение мазка по X
      hue = random8(5U);                                          // flag color
    }

    if ((step % 8U == 0U) && (modes[currentMode].Speed > 230U)) {
      blurScreen(beatsin8(5U, 5U, 72U));
    }

    hue2++;                                                       // Движение мазка по горизонтали (X)
    deltaHue2++;                                                  // Движение мазка по вертикали (Y)

    if (hue2 >= WIDTH) {
      if (deltaHue2 > (uint8_t)(HEIGHT - 2U)) {                   // если матрица высокая дорисовываем остальные мазки
        deltaHue2 = random8(5U);                                  // Изменяем вертикальное (Y) положение только отрисовав весь флаг
      }

      if ((step & 0x01U) == 0U) {
        hue2 = 0U;
      } else {
        hue2 = random8(WIDTH);                                    // Случайный сдвиг следующего мазка по оси X
      }
    }

    if (deltaHue2 >= HEIGHT) {
      deltaHue2 = 0U;
      if (deltaValue > 200U) {
        hue = random8(5U);                                        // На низких скоростях меняем оттенок палитры после каждого витка
      }
    }

    // меняем цвет для разных частей флага
    const uint32_t final_color = (deltaHue2 > (uint8_t)(CENTER_Y - 1U)) ? colors[0U][hue] : colors[1U][hue];

    // LOG.printf_P(PSTR("color = %08d | hue2 = %d | speed = %03d | custom_eff = %d\n"), color, hue2, deltaValue, custom_eff);
    drawPixelXY(hue2, deltaHue2, final_color);
    // ----------------------------------

    step++;
  }
}
#endif


#ifdef DEF_BAMBOO
// =============== Bamboo ===============
//             © SlingMaster
//                 Бамбук
// --------------------------------------
static uint8_t nextColor(uint8_t posY, uint8_t base, uint8_t next) {
  constexpr uint8_t posLine = (HEIGHT > 16U) ? 4U : 3U;

  if (((uint8_t)(posY + 1U) == posLine) || (posY == posLine)) {
    return next;
  }
  return base;
}

// --------------------------------------
static void Bamboo() {
  static constexpr uint8_t gamma[7] = {0, 32, 144, 160, 196, 208, 230};

  constexpr uint8_t DELTA = 4U;
  constexpr uint8_t VG_STEP = 64U;
  constexpr uint8_t V_STEP = 32U;
  constexpr uint8_t posLine = (HEIGHT > 16) ? 4 : 3;
  constexpr uint8_t SX = 5;
  constexpr uint8_t SY = 10;
  constexpr float STP = 0.2f;

  // trackingObjectPosX[0] => субпиксельный сдвиг index
  // trackingObjectPosY[0] => горизонтальный люфт ветра deltaX
  // trackingObjectState[0] => флаг направления ветра direct (1U - влево, 0U - вправо)
  // static uint8_t colLine  => переносим в пуловый свободный регистр hue

  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), random8(128, 255U));
    }
#endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    trackingObjectPosX[0] = STP;
    trackingObjectPosY[0] = 0.0f;
    trackingObjectState[0] = 0U; // direct = false

    hue = gamma[map(modes[currentMode].Scale, 5U, 95U, 0U, 6U)];

    step = 0U;

    loadingFlag = false;
  }

  float& index = trackingObjectPosX[0];
  float& deltaX = trackingObjectPosY[0];

  bool direct = (trackingObjectState[0] == 1U);

  const bool wind_enabled = (modes[currentMode].Scale < 50U);

  // *** ---
  for (uint8_t y = 0U; y < (uint8_t)(HEIGHT + SY); y++) {
    if (wind_enabled) {
      if (step % 128U == 0U) {
        deltaX += STP * (direct ? -1.0f : 1.0f);
        if ((deltaX > 1.0f) || (deltaX < -1.0f)) {
          direct = !direct;
        }
      }
    } else {
      deltaX = 0.0f;
    }

    // Горизонтальный рендеринг бамбука
    for (uint8_t x = 0U; x < (uint8_t)(WIDTH + SX); x++) {
      if (y == posLine) {
        drawPixelXYF(x, y - 1U, CHSV(hue, 255U, 128U));
        drawPixelXYF(x, y, CHSV(hue, 255U, 96U));
        if (HEIGHT > 16U) {
          drawPixelXYF(x, y - 2U, CHSV(hue, 10U, 64U));
        }
      }

      if (((x % SX) == 0U) && ((y % SY) == 0U)) {
        const float x_wind = x + deltaX;

        for (uint8_t i = 1U; i < (uint8_t)(SY - 3U); i++) {
          const uint8_t v_fade = (uint8_t)(255U - V_STEP * i);

          if (i < 3U) {
            uint8_t posY1 = y - i + 1U - DELTA + index;
            drawPixelXYF(x_wind - 3.0f, posY1, CHSV(nextColor(posY1, 96U, hue), 255U, v_fade));

            uint8_t posY2 = y - i + index;
            drawPixelXYF(x_wind, posY2, CHSV(nextColor(posY2, 96U, hue), 255U, (uint8_t)(255U - VG_STEP * i)));
          }

          uint8_t posY3 = y - i - DELTA + index;
          drawPixelXYF(x_wind - 4.0f, posY3, CHSV(nextColor(posY3, 180U, hue), 180U, v_fade));

          uint8_t posY4 = y - i + 1U + index;
          drawPixelXYF(x_wind - 1.0f, posY4, CHSV(nextColor(posY4, ((i == 1U) ? 96U : 80U), hue), 255U, v_fade));
        }
      }
    }
    step++;
  }

  fadeToBlackBy(leds, NUM_LEDS, 60);

  if (index >= SY) {
    index = 0.0f;
  }
  index += STP;

  trackingObjectState[0] = direct ? 1U : 0U;
}
#endif


#ifdef DEF_BALLROUTINE
// =====================================
//          Блуждающий кубик
// =====================================

#define RANDOM_COLOR          (1U)                          // случайный цвет при отскоке

static int16_t coordB[2U];
static int8_t vectorB[2U];
static CHSV _pulse_color;
static CRGB ballColor;

static void ballRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      setModeSettings(13U + random8(88U) , 155U + random8(46U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    constexpr int16_t start_coord = CENTER_X * 10;
    for (uint8_t i = 0U; i < 2U; i++) {
      coordB[i] = start_coord;
      vectorB[i] = random8(8U, 20U);
    }

    // Расчет размера мячика (ballSize)
    deltaValue = map((uint8_t)(modes[currentMode].Scale * 2.55f), 0U, 255U, 2U, (uint8_t)max(MIN_SIDE / 3, 4));

    // Генерация стартовых цветов
    ballColor = CHSV((uint8_t)(random8(9U) * 28U), 255U, 255U);
    _pulse_color = CHSV((uint8_t)(random8(9U) * 28U), 255U, 255U);

    loadingFlag = false;
  }

  ledsClear(); // esphome: FastLED.clear();

  const uint8_t size = deltaValue;
  const bool has_pulse = (modes[currentMode].Scale & 0x01U);

  const int16_t ball_x = coordB[0U] / 10;
  const int16_t ball_y = coordB[1U] / 10;

  // Отрисовка следа импульса (если Scale нечетный)
  if (has_pulse) {
    for (uint8_t i = 0U; i < size; i++) {
      const int16_t px = ball_x + i;
      for (uint8_t j = 0U; j < size; j++) {
        leds[XY(px, ball_y + j)] = _pulse_color;
      }
    }
  }

  // Обсчет физики шага движения по осям X и Y
  for (uint8_t i = 0U; i < 2U; i++) {
    coordB[i] += vectorB[i];
    if (coordB[i] < 0) {
      coordB[i] = 0;
      vectorB[i] = -vectorB[i];
      #if (RANDOM_COLOR == 1U)
        ballColor = CHSV((uint8_t)(random8(9U) * 28U), 255U, 255U);
      #endif
    }
  }

  // Проверка правого края матрицы по оси X с учетом размера мячика
  const int16_t max_x_limit = (WIDTH - size) * 10;
  if (coordB[0U] > max_x_limit) {
    coordB[0U] = max_x_limit;
    vectorB[0U] = -vectorB[0U];
    #if (RANDOM_COLOR == 1U)
      ballColor = CHSV((uint8_t)(random8(9U) * 28U), 255U, 255U);
    #endif
  }

  // Проверка верхнего края матрицы по оси Y с учетом размера мячика
  const int16_t max_y_limit = (HEIGHT - size) * 10;
  if (coordB[1U] > max_y_limit) {
    coordB[1U] = max_y_limit;
    vectorB[1U] = -vectorB[1U];
    #if (RANDOM_COLOR == 1U)
      ballColor = CHSV((uint8_t)(random8(9U) * 28U), 255U, 255U);
    #endif
  }

  const int16_t final_x = coordB[0U] / 10;
  const int16_t final_y = coordB[1U] / 10;

  for (uint8_t i = 0U; i < size; i++) {
    const int16_t px = final_x + i;
    for (uint8_t j = 0U; j < size; j++) {
      leds[XY(px, final_y + j)] = ballColor;
    }
  }
}
#endif


#ifdef DEF_STARS
// =====================================
//                Stars
//     © SottNick and  © Stepko
//      Adaptation © SlingMaster
//                Звезды
// =====================================
static void drawStar(float xlocl, float ylocl, float biggy, float little, int16_t points, float dangle, uint8_t koler) { // random multipoint star
  const float radius2 = 255.0f / (float)points;
  constexpr float inv128 = 1.0f / 128.0f;

   for (uint8_t i = 0U; i < points; i++) {
    const float i_rad2 = i * radius2;
    const float i_rad2_dangle = i_rad2 - dangle;
    const float half_rad2 = radius2 * inv2;

    const float sin_big = (sin8(i_rad2_dangle) - 128.0f) * inv128;
    const float cos_big = (cos8(i_rad2_dangle) - 128.0f) * inv128;
    const float x_big = xlocl + (biggy * sin_big);
    const float y_big = ylocl + (biggy * cos_big);

    const float phase_plus = i_rad2 + half_rad2 - dangle;
    const float x_lit1 = xlocl + (little * (sin8(phase_plus) - 128.0f) * inv128);
    const float y_lit1 = ylocl + (little * (cos8(phase_plus) - 128.0f) * inv128);

    const float phase_minus = i_rad2 - half_rad2 - dangle;
    const float x_lit2 = xlocl + (little * (sin8(phase_minus) - 128.0f) * inv128);
    const float y_lit2 = ylocl + (little * (cos8(phase_minus) - 128.0f) * inv128);

    const CRGB star_color = ColorFromPalette(*curPalette, koler);

    DrawLine(x_lit1, y_lit1, x_big, y_big, star_color);
    DrawLine(x_lit2, y_lit2, x_big, y_big, star_color);
  }
}

// --------------------------------------
static void EffectStars() {
  constexpr uint8_t STARS_NUM = 8U;
  constexpr uint8_t STAR_BLENDER = 128U;
  constexpr uint8_t CENTER_DRIFT_SPEED = 6U;

  constexpr float inv128 = 1.0f / 128.0f;

  // trackingObjectPosX[0] => driftx,  trackingObjectPosY[0] => drifty
  // trackingObjectPosX[1] => cangle,  trackingObjectPosY[1] => sangle
  // trackingObjectPosX[2] => counter
  // Массив кодов цвета => trackingObjectHue
  // Массив задержек    => trackingObjectShift
  // Массив лучей звезд => trackingObjectSpeedX
  // static uint8_t spd => переносим в глобальный пул в регистр ff_x
  // static uint8_t stars_count => переносим в глобальный пул в регистр ff_y
  // static uint8_t blur => переносим в глобальный пул в регистр ff_z

  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), random8(80U, 255U));
    }
#endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    trackingObjectPosX[0U] = CENTER_X_F;                                           // Стартуем строго с центра матрицы
    trackingObjectPosY[0U] = CENTER_Y_F;

    trackingObjectPosX[1U] = (float)(sin8(random8(25U, 220U)) - 128.0f) * inv128;  // angle of movement for the center of animation gives a float value between -1 and 1
    trackingObjectPosY[1U] = (float)(sin8(random8(25U, 220U)) - 128.0f) * inv128;  // angle of movement for the center of animation in the y direction gives a float value between -1 and 1

    ff_x = modes[currentMode].Speed;                                               // spd
    ff_z = modes[currentMode].Scale >> 1U;                                         // blur
    ff_y = CENTER_X;                                                               // stars_count
    if (ff_y > STARS_NUM) ff_y = STARS_NUM;

    trackingObjectPosX[2U] = (float)(ff_x / 5U + 3U);                              // counter

    for (uint8_t num = 0U; num < ff_y; num++) {
      trackingObjectSpeedX[num] = map(modes[currentMode].Scale, 1U, 255U, 3U, 7U); // количество углов в звезде
      trackingObjectShift[num] = ff_x / 5U + (num << 2U) + 2U;                     // задержка следующего пуска звезды
      trackingObjectHue[num] = random8();
    }

    loadingFlag = false;
  }

  fadeToBlackBy(leds, NUM_LEDS, 165);

  float& driftx = trackingObjectPosX[0U];
  float& drifty = trackingObjectPosY[0U];
  float& cangle = trackingObjectPosX[1U];
  float& sangle = trackingObjectPosY[1U];
  float& counter = trackingObjectPosX[2U];

  const float speedFactor = ((float)ff_x * inv380 + 0.05f);                     // ((float)spd / 380.0f + 0.05f)
  counter += speedFactor;                                                       // определяет то, с какой скоростью будет приближаться звезда

  if (driftx > (float)(WIDTH - spirocenterX / 2U))                              // change directin of drift if you get near the right 1/4 of the screen
    cangle = 0.0f - std::abs(cangle);
  if (driftx < (float)(spirocenterX / 2U))                                      // change directin of drift if you get near the right 1/4 of the screen
    cangle = std::abs(cangle);

  const uint16_t int_counter = (uint16_t)counter;
  if (int_counter % CENTER_DRIFT_SPEED == 0U) {                                 // move the x center every so often
    driftx += (cangle * speedFactor);
  }

  if (drifty > (float)(HEIGHT - spirocenterY / 2U))                             // if y gets too big, reverse
    sangle = 0.0f - std::abs(sangle);
  if (drifty < (float)(spirocenterY / 2U))                                      // if y gets too small reverse
    sangle = std::abs(sangle);

  if (int_counter % CENTER_DRIFT_SPEED == 0U) {                                 // move the y center every so often
    drifty += (sangle * speedFactor);
  }

  for (uint8_t num = 0U; num < ff_y; num++) {
    const float current_delay = trackingObjectShift[num];
    if (counter >= current_delay) {
      const float star_age = counter - current_delay;

      if (star_age <= (float)(WIDTH + 5U)) {
        drawStar(driftx, drifty, star_age * 2.0f, star_age, (uint8_t)trackingObjectSpeedX[num], STAR_BLENDER + trackingObjectHue[num], (uint8_t)trackingObjectHue[num]);
        trackingObjectHue[num] += speedFactor;                                  // в зависимости от знака - направление вращения
      } else {
        trackingObjectShift[num] = counter + (ff_y << 1U) + 1U;                 // задержка следующего пуска звезды
      }
    }
  }

  blur2d(WIDTH, HEIGHT, ff_z);
}
#endif


#ifdef DEF_TIXYLAND
// ============= Tixy Land ==============
//        © Martin Kleppe @aemkei
//github.com/owenmcateer/tixy.land-display
//      Create Script Change Effects
//             © SlingMaster
// ======================================
//   набор мат. функций и примитивов для
//            обсчета эффектов
//       © Dmytro Korniienko (kDn)
// ======================================
//   float optimization © andrewjswan
// ======================================

#ifndef M_PI_2
  #define M_PI_2 (1.57079632679489661923f)
#endif

static const PROGMEM float LUT[102] = {
  0,           0.0099996664, 0.019997334, 0.029991005, 0.039978687,
  0.049958397, 0.059928156,  0.069885999, 0.079829983, 0.089758173,
  0.099668652, 0.10955953,   0.11942893,  0.12927501,  0.13909595,
  0.14888994,  0.15865526,   0.16839015,  0.17809294,  0.18776195,
  0.19739556,  0.20699219,   0.21655031,  0.22606839,  0.23554498,
  0.24497867,  0.25436807,   0.26371184,  0.27300870,  0.28225741,
  0.29145679,  0.30060568,   0.30970293,  0.31874755,  0.32773849,
  0.33667481,  0.34555557,   0.35437992,  0.36314702,  0.37185606,
  0.38050637,  0.38909724,   0.39762798,  0.40609807,  0.41450688,
  0.42285392,  0.43113875,   0.43936089,  0.44751999,  0.45561564,
  0.46364760,  0.47161558,   0.47951928,  0.48735857,  0.49513325,
  0.50284320,  0.51048833,   0.51806855,  0.52558380,  0.53303409,
  0.54041952,  0.54774004,   0.55499572,  0.56218672,  0.56931317,
  0.57637525,  0.58337301,   0.59030676,  0.59717667,  0.60398299,
  0.61072594,  0.61740589,   0.62402308,  0.63057774,  0.63707036,
  0.64350110,  0.64987046,   0.65617871,  0.66242629,  0.66861355,
  0.67474097,  0.68080884,   0.68681765,  0.69276786,  0.69865984,
  0.70449406,  0.71027100,   0.71599114,  0.72165483,  0.72726268,
  0.73281509,  0.73831260,   0.74375558,  0.74914461,  0.75448018,
  0.75976276,  0.76499283,   0.77017093,  0.77529752,  0.78037310,
  0.78539819,  0.79037325
};

// --------------------------------------
static float atan2_fast(float y, float x) {
  // http://pubs.opengroup.org/onlinepubs/009695399/functions/atan2.html
  // Volkan SALMA

  // Защита от неопределенности 0/0: по стандарту atan2(0,0) равен 0
  if (x == 0.0f && y == 0.0f) return 0.0f;

  constexpr float ONEQTR_PI = PI / 4.0f;
  constexpr float THRQTR_PI = 3.0f * PI / 4.0f;

  float r, angle;
  float abs_y = std::abs(y);

  if (x < 0.0f) {
    // Ветка для II и III квадрантов
    r = (x + abs_y) / (abs_y - x);
    angle = THRQTR_PI;
  } else {
    // Ветка для I и IV квадрантов (включая ось Y, когда x == 0)
    r = (x - abs_y) / (x + abs_y);
    angle = ONEQTR_PI;
  }

  // Оптимизация полинома по схеме Горнера (минус одно умножение)
  angle += ((0.1963f * r) * r - 0.9817f) * r;

  // Отрицательный угол для III и IV квадрантов
  return (y < 0.0f) ? -angle : angle;
}

// --------------------------------------
static float atan_fast(float x) {
  if (std::isinf(x)) {
    return (x > 0.0f) ? M_PI_2 : -M_PI_2;
  }

  /* A fast look-up method with enough accuracy */
  if (x > 0) {
    if (x <= 1) {
      uint8_t index = std::clamp((int32_t)std::round(x * 100.0f), (int32_t)0, (int32_t)101);
      return LUT[index];
    } else {
      float re_x = 1 / x;
      uint8_t index = std::clamp((int32_t)std::round(re_x * 100.0f), (int32_t)0, (int32_t)101);
      return (M_PI_2 - LUT[index]);
    }
  } else {
    if (x >= -1) {
      float abs_x = -x;
      uint8_t index = std::clamp((int32_t)std::round(abs_x * 100.0f), (int32_t)0, (int32_t)101);
      return -(LUT[index]);
    } else {
      float re_x = 1 / (-x);
      uint8_t index = std::clamp((int32_t)std::round(re_x * 100.0f), (int32_t)0, (int32_t)101);
      return (LUT[index] - M_PI_2);
    }
  }
}

// --------------------------------------
static float tan2pi_fast(float x) {
  float y = (1 - x * x);
  return x * (((-0.000221184f * y + 0.0024971104f) * y - 0.02301937096f) * y + 0.3182994604f + 1.2732402998f / y);
}

// --------------------------------------
static float code(float t, float i, float x, float y) {
  switch (pcnt) {
    /** © Motus Art @motus_art */
    case 1: /* Plasma */
      hue = 96U; hue2 = 224U;
      return (sin16((x + t) * 8192.0f) * inv2 + sin16((y + t) * 8192.0f) * inv2 + sin16((x + y + t) * 8192.0f) * inv3) / 32767.0f;
      break;

    case 2: /* Up & Down */
      //return sin(cos(x) * y / 8 + t);
      hue = 255U; hue2 = 160U;
      return sin16((cos16(x * 8192.0f) / 32767.0f * y / CENTER_Y_F + t) * 8192.0f) / 32767.0f;
      break;

    case 3:
      hue = 255U; hue2 = 96U;
      return sin16((atan_fast(y / x) + t) * 8192.0f) / 32767.0f;
      break;

    /** © tixy.land website */
    case 4: /* Emitting rings */
      hue = 255U; hue2 = 0U;
      return sin16((t - SQRT_VARIANT((x - CENTER_X_F) * (x - CENTER_X_F) + (y - CENTER_Y_F) * (y - CENTER_Y_F))) * 8192.0f) / 32767.0f;
      break;

    case 5: /* Rotation  */
      hue = 136U; hue2 = 48U;
      return sin16((PI * 2.5f * atan_fast((y - CENTER_Y_F) / (x - CENTER_X_F)) + 5.0f * t) * 8192.0f) / 32767.0f;
      break;

    case 6: /* Vertical fade */
      hue = 160U; hue2 = 0U;
      return sin16((y * inv8 + t) * 8192.0f) / 32767.0f;  // y / 8 -> y * inv8
      break;

    case 7: /* Waves */
      hue = 48U; hue2 = 160U;
      return (sin16(x * 4096.0f) - sin16((x - t) * 8192.0f)) / 32767.0f - y + CENTER_Y_F;
      break;

    case 8: /* Drop */
      hue = 136U; hue2 = 160U;
      return std::fmod(8.0f * t, 13.0f) - SQRT_VARIANT((x - CENTER_X_F) * (x - CENTER_X_F) + (y - CENTER_Y_F) * (y - CENTER_Y_F));
      break;

    case 9: /* Ripples @thespite */
      hue = 96U; hue2 = 224U;
      return sin16((t - SQRT_VARIANT(x * x + y * y)) * 8192.0f) / 32767.0f;
      break;

    case 10: /* Bloop bloop bloop @v21 */
      hue = 136U; hue2 = 160U;
      return (x - CENTER_X_F) * (y - CENTER_Y_F) - sin16(t * 4096.0f) / 512.0f;
      break;

    case 11: /* SN0WFAKER */
      // https://www.reddit.com/r/programming/comments/jpqbux/minimal_16x16_dots_coding_environment/gbgk7c0/
      hue = 96U; hue2 = 160U;
      return sin16((atan_fast((y - CENTER_Y_F) / (x - CENTER_X_F)) + t) * 8192.0f) / 32767.0f;
      break;

    case 12: /* detunized */
      // https://www.reddit.com/r/programming/comments/jpqbux/minimal_16x16_dots_coding_environment/gbgk30l/
      hue = 136U; hue2 = 160U;
      return sin16((y / CENTER_Y_F + t * inv2) * 8192.0f) / 32767.0f + x * inv16 - 0.5f;  // x / 16 -> x * inv16
      break;

    /** © @akella | https://twitter.com/akella/status/1323549082552619008 */
    case 13:
      hue = 255U; hue2 = 0U;
      return sin16((6.0f * atan2_fast(y - CENTER_Y_F, x) + t) * 8192.0f) / 32767.0f;
      break;

    case 14:
      hue = 32U; hue2 = 160U;
      return sin16((i * inv5 + t) * 16384.0f) / 32767.0f;  // i / 5 -> i * inv5
      break;

    case 15: /* Burst */
      // https://twitter.com/P_Malin/status/1323605999274594304
      hue = 136U; hue2 = 160U;
      {
        float denom = (x - CENTER_X_F) * (x - CENTER_X_F) + (y - CENTER_Y_F) * (y - CENTER_Y_F) - std::fmod(t * 0.3f, 0.7f) * 200.0f;
        if (std::abs(denom) < 1e-5f) denom = 1e-5f; // Защита от деления на 0
        return -10.0f / denom;
      }
      break;

    case 16: /* Rays */
      hue = 255U; hue2 = 0U;
      return sin16((atan2_fast(x, y) * 5.0f + t * 2.0f) * 8192.0f) / 32767.0f;
      break;

    case 17: /* Starfield */
      // org | https://twitter.com/P_Malin/status/1323702220320313346
      hue = 255U; hue2 = 160U;
      {
        float denom = std::fmod(y * y, 5.9f) + 1.0f;
        int32_t val = (int32_t)(x + t * 50.0f / denom);
        return !(val & 15) / denom;
      }
      break;

    case 18:
      hue = 255U; hue2 = 0U;
      {
        float offset = sin16(t * 8192.0f) * 0.00006f;
        return sin16((3.5f * atan2_fast(y - CENTER_Y_F + offset, x - CENTER_X_F + offset) + t * 1.5f + 5.0f) * 8192.0f) / 32767.0f;
      }
      break;

    case 19:
      hue = 255U; hue2 = 224U;
      return (y - 8.0f) * inv3 - tan2pi_fast((x * inv6 + 1.87f) * inv2PI) * sin16(t * 16834.0f) / 32767.0f;  // / 3 -> * inv3, / PI*2 -> * inv2PI
      break;

    case 20:
      hue = 136U; hue2 = 160U;
      return (y - 8.0f) * inv3 - (sin16((x * inv4 + t * 2.0f) * 8192.0f) / 32767.0f);  // / 3 -> * inv3
      break;

    case 21:
      hue = 72U; hue2 = 96U;
      return std::cos(sin16(x * t * 819.2f) / 32767.0f * PI) + cos16((sin16((y * t * inv10 + SQRT_VARIANT(std::abs(cos16(x * t * 8192.0f) / 32767.0f))) * 8192.0f) / 32767.0f * PI) * 8192.0f) / 32767.0f;
      break;

    case 22: /* bambuk */
      hue = 96U; hue2 = 80U;
      return sin16(x * inv3 * sin16(t * 2730.6667f) * inv2) / 32767.0f + cos16(y * inv4 * sin16(t * 4096.0f) * inv2) / 32767.0f;
      break;

    case 23:
      hue = 0U; hue2 = 224U;
      {
        float _x = x - std::fmod(t, (float)WIDTH);
        float _y = y - std::fmod(t, (float)HEIGHT);
        float denom = SQRT_VARIANT(_x * _x + _y * _y) - std::fmod(t, 2.0f) * 9.0f;
        if (std::abs(denom) < 1e-5f) denom = 1e-5f;
        return -0.4f / denom;
      }
      break;

    case 24: /* honey */
      hue = 255U; hue2 = 40U;
      return sin16(y * t * 2048.0f) / 32767.0f * cos16(x * t * 2048.0f) / 32767.0f;
      break;

    case 25:
      hue = 96U; hue2 = 160U;
      return atan_fast((x - CENTER_X_F) * (y - CENTER_Y_F)) - 2.5f * sin16(t * 8192.0f) / 32767.0f;
      break;

    default:
      if (pcnt > 25) {
        deltaHue2 += 32;
      }
      pcnt = 1;
      hue = 96U; hue2 = 0U;
      return sin16(t * 8192.0f) / 32767.0f;
      break;
  }
}

// --------------------------------------
static void TixyLand() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), random8(255U));
    }
#endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    deltaHue = 0U;
    pcnt = map(modes[currentMode].Speed, 5U, 250U, 1U, 25U);
    FPSdelay = 1U;

    deltaHue2 = (modes[currentMode].Scale * 255U) / 100U;
    if (modes[currentMode].Scale < 5U) {
      deltaHue2 = 0U;
    }

    hue = 255U;
    hue2 = 0U;

    loadingFlag = false;
  }

  constexpr float inv1000 = 1.0f / 1000.0f;
  const float t = (float)millis() * inv1000;

  EVERY_N_SECONDS(20) {
    if ((modes[currentMode].Speed < 5U) || (modes[currentMode].Speed > 250U)) {
      pcnt++;
    }
  }

  const uint8_t current_hue = hue;
  const uint8_t current_hue2 = hue2 + deltaHue2;
  const bool is_white_mode = (current_hue == 255U);

  for (uint8_t x = 0U; x < WIDTH; x++) {
    const float fx = (float)x;

    for (uint8_t y = 0U; y < HEIGHT; y++) {
      const float fy = (float)y;
      const float i = (fy * (float)WIDTH) + fx;

      const float frame = clamp(code(t, i, fx, fy), -1.0f, 1.0f) * 255.0f;

      if (frame > 0.0f) {
        const uint8_t u8f = (uint8_t)frame;
        if (is_white_mode) {
          drawPixelXY(x, y, CRGB(u8f, u8f, u8f));
        } else {
          drawPixelXY(x, y, CHSV(current_hue, u8f, u8f));
        }
      } else if (frame < 0.0f) {
        drawPixelXY(x, y, CHSV(current_hue2, (uint8_t)(-frame), (uint8_t)(-frame)));
      } else {
        drawPixelXY(x, y, 0x000000);
      }
    }
  }
}
#endif


#ifdef DEF_FIRESPARKS
// ============  FireSparks =============
//               © Stepko
//    updated with Sparks © kostyamat
//             EFF_FIRE_SPARK
//            Fire with Sparks
//---------------------------------------
static uint16_t RGBweight(uint16_t idx) {
  return (uint16_t)(leds[idx].r + leds[idx].g + leds[idx].b);
}

class Spark {
  private:
    CRGB color;
    uint8_t Bri;
    uint8_t Hue;
    float x, y, speedy = (float)random8(5U, 31U) * inv10;

  public:
    void addXY(float nx, float ny) {
      x += nx;
      y += ny * speedy;
    }

    float getY() {
      return y;
    }

    void reset() {
      uint32_t peak = 0;
      speedy = (float)random8(5U, 31U) * inv10;
      y = random8(QUARTER_Y, CENTER_Y);

      color = leds[XY(x, y)];
      for (uint8_t i = 0U; i < WIDTH; i++) {
        const uint32_t temp = RGBweight(XY(i, (uint8_t)y));
        if (temp > peak) {
          x = i;
          peak = temp;
        }
      }

      color = leds[XY((uint8_t)x, (uint8_t)y)];
    }

    void draw() {
      // color.fadeLightBy(256 / (HEIGHT * 0.75f));
      // 0.75 это 3/4.
      // Чтобы получить (256 / (HEIGHT * 3/4)),
      // переворачиваем дробь: (256 * 4) / (HEIGHT * 3)
      // 256 * 4 = 1024

      constexpr uint16_t fade_amount = 1024U / (HEIGHT * 3U);
      color.fadeLightBy(fade_amount);
      drawPixelXYF(x, y, color);
    }
};

constexpr uint8_t sparksCount = QUARTER_X;
static Spark sparks[sparksCount];

//---------------------------------------
static void  FireSparks() {
  constexpr uint8_t spacer = QUARTER_Y;
  constexpr uint8_t scale = 50U;

  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                       scale | speed
      setModeSettings(random(0U, 99U), random(20U, 100U));
    }
#endif

    FPSdelay = DYNAMIC;
    lastUpdateTime = 0U;

    for (uint8_t i = 0U; i < sparksCount; i++) {
      sparks[i].reset();
    }

    loadingFlag = false;
  }

  const bool withSparks = (modes[currentMode].Scale >= 50U);
  const uint8_t current_speed = modes[currentMode].Speed;
  lastUpdateTime += current_speed;

  // Обсчет и отрисовка физики искр
  if (withSparks) {
    for (uint8_t i = 0U; i < sparksCount; i++) {
      const float nx_drift = (float)((int16_t)random8(3U) - 1) * inv2;
      sparks[i].addXY(nx_drift, 0.75f);

      if (sparks[i].getY() >= (float)HEIGHT && random8(50U) == 0U) {
        sparks[i].reset();
      } else {
        sparks[i].draw();
      }
    }
  }

  // Предрассчитаем константу затухания по вертикали
  constexpr uint8_t fadeStep = 255U / HEIGHT;

  for (uint8_t y = 0U; y < HEIGHT; y++) {
    const int16_t yComp = (int16_t)(y * scale) - lastUpdateTime;
    const uint8_t yFade = (withSparks ? (uint8_t)(y + spacer) : y) * fadeStep;

    for (uint8_t x = 0U; x < WIDTH; x++) {
      const int16_t Bri = fastled_helper::perlin8(x * scale, yComp, 0) - yFade;
      const uint8_t Col = (uint8_t)Bri;
      uint8_t finalBri = 0U;

      if (Bri > 0) {
        finalBri = 255U - (uint8_t)(Bri / 5U);
      }

      nblend(leds[XY(x, y)], ColorFromPalette(HeatColors_p, Col, finalBri), current_speed);
    }
  }
}
#endif


#ifdef DEF_DANDELIONS
// =====================================
//     Multicolored Dandelions
//      Base Code © Less Lam
//          © SlingMaster
//       Разноцветные одуванчики
// https://editor.soulmatelights.com/gallery/2007-amber-rain
// =====================================
class Circle {
  public:
    float thickness = 3.0f;
    uint32_t startTime;
    uint16_t offset;
    int16_t centerX;
    int16_t centerY;
    uint8_t hue;

    void move() {
      centerX = random8(WIDTH);
      centerY = random8(HEIGHT);
    }

    void scroll() {
      centerX--;
      if (centerX < 1U) {
        centerX = MAX_X;
      }
      centerY++;
      if (centerY > (int16_t)HEIGHT) {
        centerY = 0U;
      }
    }

    void reset() {
      startTime = millis();
      centerX = random8(WIDTH);
      centerY = random8(HEIGHT);
      hue = random8();
      offset = random16(6000U);
    }

    float radius() const {
      const uint16_t bpm = ((uint16_t)modes[currentMode].Speed * 2U) / 5U;
      return (float)beatsin16(bpm, 0U, 500U, offset) * inv100;
    }
};

// -----------------------------------
namespace Circles {
constexpr uint8_t NUMBER_OF_CIRCLES = CENTER_X;
static Circle circles[NUMBER_OF_CIRCLES] = {};

static void drawCircle(const Circle& circle, float radius) {
  const int16_t centerX = circle.centerX;
  const int16_t centerY = circle.centerY;
  const uint8_t hue = circle.hue;

  const int16_t r_ceil = (int16_t)(radius + 0.999f); // ceil(radius)
  const int16_t startX = centerX - r_ceil;
  const int16_t endX = centerX + r_ceil;
  const int16_t startY = centerY - r_ceil;
  const int16_t endY = centerY + r_ceil;

  const float radiusSq = radius * radius;

  const float inv_radius = 1.0f / radius;

  for (int16_t x = startX; x <= endX; x++) {
    int16_t dx = x - centerX;
    int16_t dxSq = dx * dx;

    for (int16_t y = startY; y <= endY; y++) {
      int16_t dy = y - centerY;
      int16_t dySq = dy * dy;

      // Быстрая фильтрация пикселей вне круга по квадрату расстояния без корня
      if ((dxSq + dySq) > radiusSq)
        continue;

      uint16_t index = XY(x, y);
      if (index >= NUM_LEDS)
        continue;

      uint16_t brightness;
      uint8_t current_sat;

      if (radius < 1.0f) {  // Маленькая точка (центр одуванчика)
        current_sat = 20U;
        brightness = 180U;
      } else {
        current_sat = 200U;
        const float distance = SQRT_VARIANT((float)(dxSq + dySq));
        const float fraction = (radius - distance) * inv_radius;
        brightness = (uint16_t)(255.0f * fraction);
      }

      leds[index] += CHSV(hue, current_sat, brightness);
    }
  }
}

// -----------------------------
static void draw(bool setup) {
  fadeToBlackBy(leds, NUM_LEDS, 100U);

  for (uint8_t i = 0U; i < NUMBER_OF_CIRCLES; i++) {
    if (setup) {
      circles[i].reset();
    }

    const float current_radius = circles[i].radius();
    if (!setup && (current_radius < 0.5f)) {
      circles[i].scroll();
    }

    drawCircle(circles[i], current_radius);
  }
}
}; // namespace Circles

// ==============
static void Dandelions() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                         scale | speed
      setModeSettings(random8(1U, 100U), random8(10U, 255U));
    }
#endif

    ledsClear(); // esphome: FastLED.clear();

    Circles::draw(true);

    loadingFlag = false;
  }

  // FPSdelay = SOFT_DELAY;
  Circles::draw(false);
}
#endif


#ifdef DEF_SERPENTINE
// ============ Serpentine =============
//             © SlingMaster
//              Серпантин
// =====================================
static void Serpentine() {
  constexpr uint8_t PADDING = QUARTER_Y;
  constexpr uint8_t BR_INTERWAL = 64U / HEIGHT;
  constexpr uint8_t DELTA = QUARTER_X;
  constexpr float freq = 3000.0f;

  // ---------------------
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      setModeSettings(random8(4, 50), random8(4, 254U));
    }
#endif

    deltaValue = 0U;
    hue = 0U;

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }
  // ---------------------

  const uint8_t step1 = map8(modes[currentMode].Speed, 10U, 60U);
  const uint16_t ms = millis();
  const uint8_t fade = 180U - std::abs(128 - step);

  fadeToBlackBy(leds, NUM_LEDS, fade);

  constexpr float inv_max_y = 1.0f / MAX_Y;
  const uint16_t ms_div29 = ms / 29U;

  // -----------------
  for (uint16_t y = 0U; y < HEIGHT; y++) {
    const uint32_t yy = (uint32_t)(y << 8U);                                                                           // y * 256
    const uint32_t x1 = beatsin16(step1, WIDTH, (uint16_t)(MAX_Y << 8U), WIDTH, (uint16_t)(y * freq + 32768U)) >> 1U;  // / 2

    const uint8_t bri = 255U - (uint8_t)((HEIGHT - y) * BR_INTERWAL);
    const uint8_t base_hue = ms_div29 + (uint8_t)((y << 8U) * inv_max_y);

    // change color --------
    CRGB col1 = CHSV((uint8_t)(base_hue + 128U), 255U, bri);
    CRGB col2 = CHSV(base_hue, 255U, bri);

    const uint32_t x_offset = x1 + (hue * DELTA);
    wu_pixel(x_offset, (uint32_t)(yy - PADDING * (255U - hue)), &col1);

    const int32_t inv_x = (int32_t)(MAX_X << 8U) - (int32_t)x_offset;
    wu_pixel((uint32_t)std::abs(inv_x), (uint32_t)(yy - PADDING * hue), &col2);
  }

  step++;

  if ((step % 64U) == 0U) {
    if (deltaValue == 0U) {
      hue++;
      if (hue >= 255U) {
        deltaValue = 1U;
      }
    } else {
      hue--;
      if (hue < 1U) {
        deltaValue = 0U;
      }
    }
  }
}
#endif


#ifdef DEF_TURBULENCE
// ======== Digital Тurbulence =========
//             © SlingMaster
//        Цифрова Турбулентність
// =====================================
static void drawRandomCol(uint8_t x, uint8_t y, uint8_t offset, uint8_t count) {
  constexpr uint8_t STEP = 32U;
  constexpr uint8_t D = OCTANT_Y;

  const uint8_t color = (y / D) * STEP + offset;  // floor(y / D) * STEP + offset;

  if (count == 0U) {
    const uint8_t bri = (random8(8U) == 0U) ? ((step & 0x01U) ? 0U : 255U) : 0U;
    drawPixelXY(x, y, CHSV(color, 255U, bri));
  } else {
    const uint8_t bri = (bitRead(count, y) == 1U) ? ((step % 5U) ? 0U : 255U) : 0U;
    drawPixelXY(x, y, CHSV(color, 255U, bri));
  }
}

//---------------------------------------
static void Turbulence() {
  constexpr uint8_t STEP_OBJ = 8U;
  constexpr uint8_t DEPTH = 2U;

  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), random8(1, 255U));
    }
#endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    step = 0U;
    deltaValue = 0U;
    hue = 0U;

    pcnt = 0U;

    if (modes[currentMode].Speed < 20U) {
      FPSdelay = SpeedFactor(30U);
    }

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  deltaValue++;     /* size morph  */

  /* <==== scroll =====> */
  for (uint8_t y = HEIGHT; y > 0U; y--) {
    const uint8_t current_y = y - 1U;

    drawRandomCol(0U, current_y, hue, pcnt);
    drawRandomCol(MAX_X, current_y, (uint8_t)(hue + 128U), pcnt);

    // left -----
    for (uint8_t x = (uint8_t)(CENTER_X_MAJOR - 1U); x > 0U; x--) {
      /* ---> */
      const uint32_t curColor = getPixColorXY((uint8_t)(x - 1U), current_y);
      if (x < (uint8_t)(CENTER_X_MAJOR - DEPTH / 2U)) {
        drawPixelXY(x, current_y, curColor);
      } else {
        if (curColor != 0U) drawPixelXY(x, current_y, curColor);
      }
    }

    // right -----
    for (uint8_t x = (uint8_t)(CENTER_X_MAJOR + 1U); x < WIDTH; x++) {
      /* <---  */
      const uint32_t curColor = getPixColorXY(x, current_y);
      if (x > (uint8_t)(CENTER_X_MAJOR + DEPTH / 2U)) {
        drawPixelXY((uint8_t)(x - 1U), current_y, curColor);
      } else {
        if (curColor != 0U) drawPixelXY((uint8_t)(x - 1U), current_y, curColor);
      }
    }

    /* scroll center up ---- */
    for (uint8_t x = (uint8_t)(CENTER_X_MAJOR - DEPTH); x < (uint8_t)(CENTER_X_MAJOR + DEPTH); x++) {
      drawPixelXY(x, y, makeDarker(getPixColorXY(x, current_y), (uint8_t)(128U / y)));
      if (y == 1U) {
        drawPixelXY(x, 0U, 0x000000);
      }
    }
    /* --------------------- */
  }

  if (modes[currentMode].Scale > 50U) {
    pcnt++;
    if (pcnt == 0U) {
      hue += 16U;
    }
  }

  step++;
}
#endif


#ifdef DEF_ARROWS
// ============== СТРЕЛКИ ==============
//
// =====================================
static int8_t arrow_x[4], arrow_y[4], stop_x[4], stop_y[4];
static uint8_t arrow_direction;                 // 0x01 - слева направо; 0x02 - снизу вверх; 0х04 -
                                                // справа налево; 0х08 - сверху вниз
static uint8_t arrow_mode, arrow_mode_orig;     // 0 - по очереди все варианты
                                                // 1 - по очереди от края до края экрана;
                                                // 2 - одновременно по горизонтали навстречу к ентру, затем одновременно по
                                                // вертикали навстречу к центру 3 - одновременно все к центру 4 - по два
                                                // (горизонталь / вертикаль) все от своего края к противоположному, стрелки
                                                // смещены от центра на 1/3 5 - одновременно все от своего края к
                                                // противоположному, стрелки смещены от центра на 1/3
static bool arrow_complete, arrow_change_mode;
static uint8_t arrow_hue[4];
static uint8_t arrow_play_mode_count[6];        // Сколько раз проигрывать полностью каждый
                                                // режим если вариант 0 - текущий счетчик
static uint8_t arrow_play_mode_count_orig[6];   // Сколько раз проигрывать полностью каждый
                                                // режим если вариант 0 - исходные настройки

static void arrowSetup_mode1() {
  // Слева направо
  if ((arrow_direction & 0x01U) != 0U) {
    arrow_hue[0] = random8();
    arrow_x[0]   = 0;
    arrow_y[0]   = (int8_t)CENTER_Y;
    stop_x[0]    = (int8_t)(WIDTH + 7U);        // скрывается за экраном на 7 пикселей
    stop_y[0]    = 0;                           // неприменимо
  }
  // снизу вверх
  if ((arrow_direction & 0x02U) != 0U) {
    arrow_hue[1] = random8();
    arrow_y[1]   = 0;
    arrow_x[1]   = (int8_t)CENTER_X;
    stop_y[1]    = (int8_t)(HEIGHT + 7U);       // скрывается за экраном на 7 пикселей
    stop_x[1]    = 0;                           // неприменимо
  }
  // справа налево
  if ((arrow_direction & 0x04U) != 0U) {
    arrow_hue[2] = random8();
    arrow_x[2]   = (int8_t)MAX_X;
    arrow_y[2]   = (int8_t)CENTER_Y;
    stop_x[2]    = -7;                          // скрывается за экраном на 7 пикселей
    stop_y[2]    = 0;                           // неприменимо
  }
  // сверху вниз
  if ((arrow_direction & 0x08U) != 0U) {
    arrow_hue[3] = random8();
    arrow_y[3]   = (int8_t)MAX_Y;
    arrow_x[3]   = (int8_t)CENTER_X;
    stop_y[3]    = -7;                          // скрывается за экраном на 7 пикселей
    stop_x[3]    = 0;                           // неприменимо
  }
}

static void arrowSetup_mode2() {
  // Слева направо до половины экрана
  if ((arrow_direction & 0x01U) != 0U) {
    arrow_hue[0] = random8();
    arrow_x[0]   = 0;
    arrow_y[0]   = (int8_t)CENTER_Y;
    stop_x[0]    = (int8_t)(CENTER_X - 1U);     // до центра экрана
    stop_y[0]    = 0;                           // неприменимо
  }
  // снизу вверх до половины экрана
   if ((arrow_direction & 0x02U) != 0U) {
    arrow_hue[1] = random8();
    arrow_y[1]   = 0;
    arrow_x[1]   = (int8_t)CENTER_X;
    stop_y[1]    = (int8_t)CENTER_Y - 1;        // до центра экрана
    stop_x[1]    = 0;                           // неприменимо
  }
  // справа налево до половины экрана
  if ((arrow_direction & 0x04U) != 0U) {
    arrow_hue[2] = random8();
    arrow_x[2]   = (int8_t)MAX_X;
    arrow_y[2]   = (int8_t)CENTER_Y;
    stop_x[2]    = (int8_t)CENTER_X;            // до центра экрана
    stop_y[2]    = 0;                           // неприменимо
  }
  // сверху вниз до половины экрана
  if ((arrow_direction & 0x08U) != 0U) {
    arrow_hue[3] = random8();
    arrow_y[3]   = (int8_t)MAX_Y;
    arrow_x[3]   = (int8_t)CENTER_X;
    stop_y[3]    = (int8_t)CENTER_Y;            // до центра экрана
    stop_x[3]    = 0;                           // неприменимо
  }
}

static void arrowSetup_mode4() {
  // Слева направо
  if ((arrow_direction & 0x01U) != 0U) {
    arrow_hue[0] = random8();
    arrow_x[0]   = 0;
    arrow_y[0]   = (int8_t)(THIRD_Y * 2U);
    stop_x[0]    = (int8_t)(WIDTH + 7U);        // скрывается за экраном на 7 пикселей
    stop_y[0]    = 0;                           // неприменимо
  }
  // снизу вверх
  if ((arrow_direction & 0x02U) != 0U) {
    arrow_hue[1] = random8();
    arrow_y[1]   = 0;
    arrow_x[1]   = (int8_t)(THIRD_X * 2U);
    stop_y[1]    = (int8_t)(HEIGHT + 7U);       // скрывается за экраном на 7 пикселей
    stop_x[1]    = 0;                           // неприменимо
  }
  // справа налево
  if ((arrow_direction & 0x04U) != 0U) {
    arrow_hue[2] = random8();
    arrow_x[2]   = (int8_t)MAX_X;
    arrow_y[2]   = (int8_t)THIRD_Y;
    stop_x[2]    = -7;                          // скрывается за экраном на 7 пикселей
    stop_y[2]    = 0;                           // неприменимо
  }
  // сверху вниз
  if ((arrow_direction & 0x08U) != 0U) {
    arrow_hue[3] = random8();
    arrow_y[3]   = (int8_t)MAX_Y;
    arrow_x[3]   = (int8_t)THIRD_X;
    stop_y[3]    = -7;                           // скрывается за экраном на 7 пикселей
    stop_x[3]    = 0;                            // неприменимо
  }
}

static void arrowSetupForMode(uint8_t mode, bool change) {
  switch (mode) {
    case 1U:
      if (change) arrow_direction = 1U;
      arrowSetup_mode1();                        // От края матрицы к краю, по центру гориз и верт
      break;
    case 2U:
      if (change) arrow_direction = 5U;
      arrowSetup_mode2();                        // По центру матрицы (гориз / верт) - ограничение -
                                                 // центр матрицы
      break;
    case 3U:
      if (change) arrow_direction = 15U;
      arrowSetup_mode2();                        // как и в режиме 2 - по центру матрицы (гориз / верт) -
                                                 // ограничение - центр матрицы
      break;
    case 4U:
      if (change) arrow_direction = 5U;
      arrowSetup_mode4();                        // От края матрицы к краю, верт / гориз
      break;
    case 5U:
      if (change) arrow_direction = 15U;
      arrowSetup_mode4();                        // как и в режиме 4 от края матрицы к краю, на 1/3
      break;
  }
}

static void arrowsRoutine() {
  if (loadingFlag) {
    arrow_complete = false;
    arrow_mode = (arrow_mode_orig == 0U || arrow_mode_orig > 5U)
                     ? random8(1U, 5U)
                     : arrow_mode_orig;

    arrow_play_mode_count_orig[0U] = 0;
    arrow_play_mode_count_orig[1U] = 4; // 4 фазы - все стрелки показаны по кругу
                                        // один раз - переходить к следующему ->
    arrow_play_mode_count_orig[2U] = 4; // 2 фазы - гориз к центру (1), затем верт к центру (2) - обе фазы
                                        // повторить по 2 раза -> 4
    arrow_play_mode_count_orig[3U] = 4; // 1 фаза - все к центру (1) повторить по 4 раза -> 4
    arrow_play_mode_count_orig[4U] = 4; // 2 фазы - гориз к центру (1), затем верт к центру (2) - обе фазы
                                        // повторить по 2 раза -> 4
    arrow_play_mode_count_orig[5U] = 4; // 1 фаза - все сразу (1) повторить по 4 раза -> 4

    for (uint8_t i = 0U; i < 6U; i++) {
      arrow_play_mode_count[i] = arrow_play_mode_count_orig[i];
    }
    arrowSetupForMode(arrow_mode, true);

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
}

  dimAll(160U);
  CHSV color;

  constexpr CHSV black_color = CHSV(0U, 0U, 0U);

  const uint8_t current_bri = modes[currentMode].Brightness;
  const int8_t w_limit = (int8_t)WIDTH;
  const int8_t h_limit = (int8_t)HEIGHT;

  // движение стрелки - cлева направо
  if ((arrow_direction & 0x01U) != 0U) {
    color = CHSV(arrow_hue[0U], 255U, current_bri);
    const int8_t base_x = arrow_x[0U];
    const int8_t base_y = arrow_y[0U];
    const int8_t stop_val_x = stop_x[0U];

    for (int8_t x = 0; x <= 4; x++) {
      const int16_t target_x = base_x - x;
      if (target_x >= 0 && target_x <= stop_val_x) {
        for (int8_t y = 0; y <= x; y++) {
          const CHSV clr = (x < 4 || (x == 4 && y < 2)) ? color : black_color;
          drawPixelXY(target_x, base_y - y, clr);
          drawPixelXY(target_x, base_y + y, clr);
        }
      }
    }
    arrow_x[0U]++;
  }

  // движение стрелки - cнизу вверх
  if ((arrow_direction & 0x02U) != 0U) {
    color = CHSV(arrow_hue[1U], 255U, current_bri);
    const int8_t base_x = arrow_x[1U];
    const int8_t base_y = arrow_y[1U];
    const int8_t stop_val_y = stop_y[1U];

    for (int8_t y = 0; y <= 4; y++) {
      const int16_t target_y = base_y - y;
      if (target_y >= 0 && target_y <= stop_val_y) {
        for (int8_t x = 0; x <= y; x++) {
          const CHSV clr = (y < 4 || (y == 4 && x < 2)) ? color : black_color;
          drawPixelXY(base_x - x, target_y, clr);
          drawPixelXY(base_x + x, target_y, clr);
        }
      }
    }
    arrow_y[1U]++;
  }

  // движение стрелки - cправа налево
  if ((arrow_direction & 0x04U) != 0U) {
    color = CHSV(arrow_hue[2U], 255U, current_bri);
    const int8_t base_x = arrow_x[2U];
    const int8_t base_y = arrow_y[2U];
    const int8_t stop_val_x = stop_x[2U];

    for (int8_t x = 0; x <= 4; x++) {
      const int16_t target_x = base_x + x;
      if (target_x >= stop_val_x && target_x < w_limit) {
        for (int8_t y = 0; y <= x; y++) {
          const CHSV clr = (x < 4 || (x == 4 && y < 2)) ? color : black_color;
          drawPixelXY(target_x, base_y - y, clr);
          drawPixelXY(target_x, base_y + y, clr);
        }
      }
    }
    arrow_x[2U]--;
  }

  // движение стрелки - cверху вниз
  if ((arrow_direction & 0x08U) != 0U) {
    color = CHSV(arrow_hue[3U], 255U, current_bri);
    const int8_t base_x = arrow_x[3U];
    const int8_t base_y = arrow_y[3U];
    const int8_t stop_val_y = stop_y[3U];

    for (int8_t y = 0; y <= 4; y++) {
      const int16_t target_y = base_y + y;
      if (target_y >= stop_val_y && target_y < h_limit) {
        for (int8_t x = 0; x <= y; x++) {
          const CHSV clr = (y < 4 || (y == 4 && x < 2)) ? color : black_color;
          drawPixelXY(base_x - x, target_y, clr);
          drawPixelXY(base_x + x, target_y, clr);
        }
      }
    }
    arrow_y[3U]--;
  }

  // Проверка завершения движения стрелки, переход к следующей фазе или режиму

  switch (arrow_mode) {

  case 1:
    // Последовательно - слева-направо -> снизу вверх -> справа налево -> сверху
    // вниз и далее по циклу В каждый сомент времени сктивна только одна
    // стрелка, если она дошла до края - переключиться на следующую и задать ее
    // начальные координаты
    arrow_complete = false;
    switch (arrow_direction) {
      case 1U: arrow_complete = (arrow_x[0U] > stop_x[0U]); break;
      case 2U: arrow_complete = (arrow_y[1U] > stop_y[1U]); break;
      case 4U: arrow_complete = (arrow_x[2U] < stop_x[2U]); break;
      case 8U: arrow_complete = (arrow_y[3U] < stop_y[3U]); break;
    }

    arrow_change_mode = false;
    if (arrow_complete) {
      arrow_direction = (arrow_direction << 1U) & 0x0FU;
      if (arrow_direction == 0U) {
        arrow_direction = 1U;
      }
      if (arrow_mode_orig == 0U) {
        arrow_play_mode_count[1U]--;
        if (arrow_play_mode_count[1U] == 0U) {
          arrow_play_mode_count[1U] = arrow_play_mode_count_orig[1U];
          arrow_mode = random8(1U, 5U);
          arrow_change_mode = true;
        }
      }
      arrowSetupForMode(arrow_mode, arrow_change_mode);
    }
    break;

  case 2:
    // Одновременно горизонтальные навстречу до половины экрана
    // Затем одновременно вертикальные до половины экрана. Далее - повторять
    arrow_complete = false;
    switch (arrow_direction) {
      case 5U:  arrow_complete = (arrow_x[0U] > stop_x[0U]);  break; // Стрелка слева и справа встречаются в центре одновременно - проверять только стрелку слева
      case 10U: arrow_complete = (arrow_y[1U] > stop_y[1U]);  break; // Стрелка снизу и сверху встречаются в центре одновременно - проверять только стрелку снизу
    }

    arrow_change_mode = false;
    if (arrow_complete) {
      arrow_direction = (arrow_direction == 5U) ? 10U : 5U;
      if (arrow_mode_orig == 0U) {
        arrow_play_mode_count[2U]--;
        if (arrow_play_mode_count[2U] == 0U) {
          arrow_play_mode_count[2U] = arrow_play_mode_count_orig[2U];
          arrow_mode = random8(1U, 5U);
          arrow_change_mode = true;
        }
      }
      arrowSetupForMode(arrow_mode, arrow_change_mode);
    }
    break;

  case 3:
    // Одновременно со всех сторон к центру
    // Завершение кадра режима - когда все стрелки собрались в центре.
    // Проверять стрелки по самой длинной стороне
    arrow_complete = (WIDTH >= HEIGHT) ? (arrow_x[0U] > stop_x[0U]) : (arrow_y[1U] > stop_y[1U]);

    arrow_change_mode = false;
    if (arrow_complete) {
      if (arrow_mode_orig == 0U) {
        arrow_play_mode_count[3U]--;
        if (arrow_play_mode_count[3U] == 0U) {
          arrow_play_mode_count[3U] = arrow_play_mode_count_orig[3U];
          arrow_mode = random8(1U, 5U);
          arrow_change_mode = true;
        }
      }
      arrowSetupForMode(arrow_mode, arrow_change_mode);
    }
    break;

  case 4:
    // Одновременно слева/справа от края до края со смещением горизонтальной оси
    // на 1/3 высоты, далее одновременно снизу/сверху от края до края со
    // смещением вертикальной оси на 1/3 ширины Завершение кадра режима - когда
    // все стрелки собрались в центре. Проверять стрелки по самой длинной
    // стороне
    arrow_complete = false;
    switch (arrow_direction) {
      case 5U:  arrow_complete = (arrow_x[0U] > stop_x[0U]);  break; // Стрелка слева и справа движутся и достигают края одновременно - проверять только стрелку слева
      case 10U: arrow_complete = (arrow_y[1U] > stop_y[1U]);  break; // Стрелка снизу и сверху движутся и достигают края одновременно - проверять только стрелку снизу
    }

    arrow_change_mode = false;
    if (arrow_complete) {
      arrow_direction = (arrow_direction == 5U) ? 10U : 5U;
      if (arrow_mode_orig == 0U) {
        arrow_play_mode_count[4U]--;
        if (arrow_play_mode_count[4U] == 0U) {
          arrow_play_mode_count[4U] = arrow_play_mode_count_orig[4U];
          arrow_mode = random8(1U, 5U);
          arrow_change_mode = true;
        }
      }
      arrowSetupForMode(arrow_mode, arrow_change_mode);
    }
    break;

  case 5:
    // Одновременно со всех сторон от края до края со смещением горизонтальной
    // оси на 1/3 высоты, далее Проверять стрелки по самой длинной стороне
    arrow_complete = (WIDTH >= HEIGHT) ? (arrow_x[0U] > stop_x[0U]) : (arrow_y[1U] > stop_y[1U]);

    arrow_change_mode = false;
    if (arrow_complete) {
      if (arrow_mode_orig == 0U) {
        arrow_play_mode_count[5U]--;
        if (arrow_play_mode_count[5U] == 0U) {
          arrow_play_mode_count[5U] = arrow_play_mode_count_orig[5U];
          arrow_mode = random8(1U, 5U);
          arrow_change_mode = true;
        }
      }
      arrowSetupForMode(arrow_mode, arrow_change_mode);
    }
    break;
  }
}
#endif


#ifdef DEF_AVRORA
// ============== Avrora ===============
//             © SlingMaster
//                Аврора
// =====================================
static void Avrora() {
  constexpr uint8_t PADDING = QUARTER_Y;
  constexpr float freq = 3000.0f;
  constexpr uint8_t fade = 30U;

  // ---------------------
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //           scale | speed
      setModeSettings(50U, random8(2U, 254U));
    }
    #endif

    deltaValue = 0U;
    hue = 0U;

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }
  // ---------------------

  const uint8_t step1 = map8(modes[currentMode].Speed, 10U, 60U);
  const uint16_t ms = millis();

  fadeToBlackBy(leds, NUM_LEDS, fade);

  constexpr float inv_height = 1.0f / HEIGHT;
  const uint16_t ms_div29 = ms / 29U;

  // -----------------
  for (uint16_t y = 0U; y < HEIGHT; y++) {
    const uint32_t yy = (uint32_t)(y << 8U); // y * 256

    // Оптимизация: деление на 1.5f заменено умножением на 0.6666667f
    const uint32_t x1 = (uint32_t)(beatsin16(step1, WIDTH, (uint16_t)(MAX_Y << 8U), WIDTH, (uint16_t)(y * freq + 32768U)) * 0.6666667f);

    /* change color -------- */
    const uint8_t cur_color = ms_div29 + (uint8_t)((y << 8U) * inv_height);

    CRGB color = CHSV(cur_color, 255U, (uint8_t)(255U - y * OCTANT_Y));

    // Оптимизация: заменяем деление на 5 умножением на 0.2f
    const int16_t calc_br = 255 - (int16_t)((y * HEIGHT) * inv5);
    const uint8_t br = (calc_br < 0) ? 0U : ((calc_br > 200) ? 200U : (uint8_t)calc_br);

    CRGB color2 = CHSV((uint8_t)(cur_color - 32U), (uint8_t)(255U - y * QUARTER_Y), br);

    const uint32_t x_offset = x1 + hue;
    wu_pixel((uint32_t)(x_offset + ((PADDING * hue) >> 1U)), yy, &color); // Сдвиг >> 1U вместо / 2

    const int32_t inv_x = (int32_t)(MAX_X << 8U) - (int32_t)x_offset;
    wu_pixel((uint32_t)std::abs(inv_x), (uint32_t)(yy - PADDING * hue), &color2);
  }

  step++;

  if ((step % 64U) == 0U) {
    if (deltaValue == 1U) {
      hue++;
      if (hue >= 255U) {
        deltaValue = 0U;
      }
    } else {
      hue--;
      if (hue < 1U) {
        deltaValue = 1U;
      }
    }
  }
}
#endif


#ifdef DEF_LOTUS
// ============ Lotus Flower ============
//             © SlingMaster
//             Цветок Лотоса
//---------------------------------------
static void drawLotusFlowerFragment(uint8_t posX, uint8_t line) {
  constexpr uint8_t h = (HEIGHT > 24U) ? (uint8_t)((uint16_t)(HEIGHT * 9U) / 10U) : HEIGHT;
  constexpr uint8_t lowBri = 112U;

  const uint8_t flover_color = 128U + std::abs(128 - (int16_t)hue);             // 128 -- 255
  const uint8_t gleam = 255U - std::abs(128 - (int16_t)hue2);                   // 255 -- 128

  // Вместо f_size считаем сразу смещение по высоте в целых числах:
  // f_size был (128 - abs(128 - deltaValue)) / 150.0f                          // 1.0 -- 0.0
  // Значит h * f_size = (h * (128 - abs(128 - deltaValue))) / 150
  const uint8_t h_f_size = (uint8_t)(((uint16_t)h * (128 - std::abs(128 - (int16_t)deltaValue))) / 150U);

  // clear: h * 1.1 это h + h/10
  DrawLine(posX, 0U, posX, (uint8_t)(h + (h / 10U)), 0x000000);

  switch (line) {
    case 0U:
      gradientVertical(posX, 0U, (uint8_t)(posX + 1U), (uint8_t)((h * 22U) / 100U), 96U, 96U, 32U, 255U, 255U);                             // 0.22        green leaf c
      gradientVertical(posX, (uint8_t)((h * 9U) / 10U), (uint8_t)(posX + 1U), (uint8_t)(h + (h / 10U)), 64U, 48U, 64U, 205U, gleam);        // 0.9 и 1.1   pestle
      gradientVertical(posX, 8U, (uint8_t)(posX + 1U), (uint8_t)((h * 6U) / 10U), flover_color, flover_color, 128U, lowBri, 255U);          // 0.6         ---
      break;
    case 2U:
    case 6U:
      gradientVertical(posX, (uint8_t)(h / 5U), (uint8_t)(posX + 1U), (uint8_t)(h - 4U), flover_color, flover_color, lowBri, 255U, gleam);  // 0.2         -->
      gradientVertical(posX, (uint8_t)(h / 20U), (uint8_t)(posX + 1U), (uint8_t)((h * 15U) / 100U), 96U, 96U, 32U, 255U, 255U);             // 0.05 и 0.15 green leaf
      break;
    case 3U:
    case 5U:
      gradientVertical(posX, (uint8_t)(h / 2U), (uint8_t)(posX + 1U), (uint8_t)(h - 2U), flover_color, flover_color, lowBri, 255U, 255U);   // 0.5         ---->
      break;
    case 4U:
      gradientVertical(posX, (uint8_t)(1U + h_f_size), (uint8_t)(posX + 1U), h, flover_color, flover_color, lowBri, 255U, gleam);           //             ------>
      break;
    default:
      gradientVertical(posX, (uint8_t)(h / 20U), (uint8_t)(posX + 1U), (uint8_t)(h / 5U), 80U, 96U, 160U, 64U, 255U);                       // 0.05 и 0.2 green leaf m
      break;
  }
}

//---------------------------------------
static void LotusFlower() {
  constexpr uint8_t STEP_OBJ = 8;

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), random8(1, 255U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    step = 0U;
    hue2 = 128U;
    deltaValue = 0U;
    hue = 224U;
    FPSdelay = SpeedFactor(160U);

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  const uint8_t current_speed = modes[currentMode].Speed;
  const uint8_t current_scale = modes[currentMode].Scale;

  if (current_speed > 128U) {
    if (current_scale > 50U) {
      const uint8_t deltaSpeed = 80U + (uint8_t)((128U - std::abs(128 - (int16_t)deltaValue)) * 0.8f);
      FPSdelay = SpeedFactor(deltaSpeed);

      if (step == 0U) {  /* color morph */
        hue += 32U;
      }
    } else {
      FPSdelay = SpeedFactor(160U);
      hue = 28U;
    }
    deltaValue++;        /* size morph */

    /* <==== scroll ===== */
    drawLotusFlowerFragment(MAX_X, (uint8_t)(step % STEP_OBJ));
    for (uint8_t y = 0U; y < HEIGHT; y++) {
      for (uint8_t x = 0U; x < WIDTH; x++) {
        drawPixelXY((int16_t)(x - 1U), y, getPixColorXY(x, y));
      }
    }
  } else {
    /* <==== morph ===== */
    for (uint8_t x = 0U; x < WIDTH; x++) {
      drawLotusFlowerFragment(x, (uint8_t)(x % STEP_OBJ));
      if ((x & 0x01U) != 0U) {
        hue2++;          /* gleam morph */
      }
    }
    deltaValue++;        /* size morph  */

    if (current_scale > 50U) {
      hue += 8U;         /* color morph */
    } else {
      hue = 28U;
    }
  }

  step++;
}
#endif


#ifdef DEF_FONTAN
// ============== Fountain =============
//             © SlingMaster
//                Фонтан
// =====================================
static void Fountain() {
  static constexpr uint8_t gamma[] = {0U, 96U, 128U, 160U, 240U, 112U};
  constexpr uint8_t PADDING = (uint8_t)(OCTANT_Y + 0.5f);

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), random8(2U, 254U));
    }
    #endif

    const uint8_t idx = constrain((uint8_t)(modes[currentMode].Scale / 20U), 0U, 4U);
    hue = gamma[idx];        // hue — базовый цвет
    hue2 = gamma[idx + 1U];  // hue2 — соседний цвет

    emitterY = 0.0f;
    pcnt = 0U;
    step = 0U;

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  const float radius = std::abs(128 - (int16_t)step) * inv127 * (float)CENTER_Y_MINOR;  // / 127.0f

  // Предрасчет шага базовой яркости
  const uint8_t br_div = 255U / ((uint8_t)emitterY + 1U);

  const uint8_t ceilRadius = (uint8_t)radius + (radius > (float)((uint8_t)radius) ? 1U : 0U);
  const uint8_t delta_val = (uint8_t)(((uint16_t)emitterY * 7U + (uint16_t)radius * 10U + 9U) / 10U);

  const uint8_t right_target_hue = hue2 - ceilRadius;

  for (uint8_t y = 0U; y < HEIGHT; y++) {
    const int16_t calc_br = br_div * y;
    const uint8_t br = (calc_br < 48) ? 48U : ((calc_br > 255) ? 255U : (uint8_t)calc_br);

    const int16_t boundaryLow = (int16_t)(emitterY - radius + 0.99f);
    const int16_t boundaryMid = (int16_t)(emitterY - (radius * inv2) + 0.99f);

    const float fy_plus = (float)y + 0.5f;
    const float fy_minus = (float)y - 0.5f;

    for (uint8_t x = 0U; x < WIDTH; x++) {
      if ((x & 0x01U) == 0U) { // Четные столбцы (активные струи фонтана)
        if ((x % 4U) == 0U) {
          if (y == (uint8_t)(boundaryLow + random8(1U, 4U))) {
            if ((step & 0x01U) == 0U) {
              drawPixelXYF((float)x, fy_plus, CHSV(hue, 200U, 255U));
            } else {
              drawPixelXY(x, y, CHSV(hue, 200U, 255U));
            }
          } else {
            drawPixelXY(x, y, CHSV(hue, 255U, (y > boundaryMid) ? 0U : br));
          }
        } else {
          if (y == (uint8_t)(delta_val + random8(3U))) {
            drawPixelXYF((float)x, fy_minus, CHSV(right_target_hue, 160U, 255U));
          } else {
            drawPixelXY(x, y, CHSV(right_target_hue, 255U, (y > delta_val) ? 0U : br));
          }
        }
      } else { // Нечетные столбцы (зона размытия и гашения)
        if (pcnt > (uint8_t)(PADDING + 2U)) {
          drawPixelXY(x, y, 0x000000);
        }
      }
    }
  }

  // Обсчет физики плавного движения оригинального float эмиттера
  if ((emitterY <= (float)(PADDING * 2U)) || (emitterY > (float)(HEIGHT - PADDING - 1U))) {
    blurScreen(32U);
  }

  if (emitterY > (float)pcnt) {
    emitterY -= 0.5f;
    if (std::abs((float)pcnt - emitterY) < (float)PADDING) {
      if (emitterY > (float)pcnt) {
        emitterY -= 0.5f;
      }
    }
  } else {
    if (emitterY < (float)pcnt) {
      emitterY += 3.0f;
    } else {
      pcnt = random8(2U, (uint8_t)(HEIGHT - PADDING - 1U));
    }
  }

  step++;
}
#endif


#ifdef DEF_NIGHTCITY
// ============ Night City =============
//             © SlingMaster
//              Ночной Город
// =====================================
static void NightCity() {
  constexpr uint8_t PADDING = (uint8_t)(HEIGHT * 0.13f);

  // ---------------------
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //          scale | speed
      setModeSettings(50, random8(2, 254U));
    }
    #endif

    hue = 64U;
    for (uint8_t i = 0U; i < WIDTH; i++) {
      for (uint8_t j = 0U; j < HEIGHT; j++) {
        noise3d[0U][i][j] = 0U;
      }
      noise3d[0U][i][0U] = PADDING + 2U; // Текущий этаж лифта
      noise3d[0U][i][1U] = PADDING + 3U; // Целевой этаж лифта
    }

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }
  // ---------------------

  const uint8_t xx = random8(WIDTH);
  const uint8_t yy = random8(HEIGHT);
  constexpr uint8_t fade = 80U;

  fadeToBlackBy(leds, NUM_LEDS, fade);

  const uint8_t speed_val = modes[currentMode].Speed;
  const uint8_t scale_val = modes[currentMode].Scale;
  const bool flag_scale = (scale_val > 50U);

  // -----------------
  for (uint16_t y = 0U; y < HEIGHT; y++) {
    if (y > PADDING) {
      const bool y_even = ((y & 0x01U) == 0U);  // % 2 == 0U

      for (uint16_t x = 0U; x < WIDTH; x++) {
        if (x % 6U == 0U) {
          /* draw Elevator */
          leds[XY(x, noise3d[0U][x][1U])] = CHSV(160U, 255U, 255U);
        } else {
          /* draw light ------- */
          const bool flag = flag_scale ? true : ((x & 0x01U) == 0U);

          if (flag && y_even) {
            if ((x == xx) && (y == yy)) {
              /* change light */
              if (noise3d[0U][x][y] == 0U) {
                noise3d[0U][x][y] = random8(1U, 5U);
                if (speed_val > 80U) {
                  noise3d[0U][random8(WIDTH)][random8((uint8_t)(PADDING + 1U), MAX_Y)] = 6U;
                }
                if (speed_val > 160U) {
                  noise3d[0U][random8(WIDTH)][random8((uint8_t)(PADDING + 1U), MAX_Y)] = 6U;
                }
              } else {
                noise3d[0U][x][y] = 0U;
              }
            }

            if (speed_val > 250U) {
              noise3d[0U][x][y] = 2U;
            }

            /* draw light ----- */
            const uint8_t window_state = noise3d[0U][x][y];
            if (window_state > 0U) {
              if (window_state == 1U) {
                leds[XY(x, y)] = CHSV(32U, 200U, 255U);
              } else {
                leds[XY(x, y)] = CHSV(128U, 32U, 255U);
              }
            }
          }
        }
      }
    } else {
      /* draw the lower floors */
      const CRGB floor_color = (y == PADDING) ? CHSV(hue, 255U, 255U) : CHSV(96U, 128U, (uint8_t)(80U + y * 32U));
      for (uint8_t x = 0U; x < WIDTH; x++) {
        leds[XY(x, y)] = floor_color;
      }
    }
  }

  /* change elevators position */
  /* 1 current floor */
  if ((step & 0x03U) == 0U) { // step % 4U == 0U
    for (uint8_t i = 0U; i < WIDTH; i += 6U) {
      const uint8_t current_fl = noise3d[0U][i][1U];
      const uint8_t target_fl = noise3d[0U][i][0U];
      if (target_fl > current_fl) noise3d[0U][i][1U]++;
      if (target_fl < current_fl) noise3d[0U][i][1U]--;
    }
  }

  /* 0 target floor ----- */
  if (step % 128U == 0U) {
    for (uint8_t i = 0U; i < WIDTH; i += 6U) {
      uint8_t target_floor = random8((uint8_t)(PADDING + 1U), MAX_Y);
      if ((target_floor & 0x01U) != 0U) {
        target_floor++; // Округление до четного этажа
      }
      noise3d[0U][i][0U] = target_floor;
    }
  }

  hue++;
  step++;
}
#endif


#ifdef DEF_RAIN
// =============== Rain ================
//             © @Shaitan
//            ЭФФЕКТ ДОЖДЬ
// =====================================
static void RainRoutine()
{
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                                             scale | speed
      setModeSettings(random8(10U) ? 2U + random8(99U) : 1U, 185U + random8(52U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    const uint8_t scale_val = modes[currentMode].Scale;
    // ПРЕДРАСЧЕТ ПОДРЕЖИМА ОСАДКОВ
    if (scale_val == 1U) {
      deltaValue = 0U; // Радужный дождь
    } else if (scale_val == 100U) {
      deltaValue = 1U; // Снег
    } else {
      deltaValue = 2U; // Цветной дождь
    }

    // ПОЛНЫЙ ПРЕДРАСЧЕТ БАЗОВОГО ОТТЕНКА
    deltaHue = (uint8_t)(scale_val * 2.4f);

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  // Обсчет и генерация капель на самой верхней строке матрицы
  for (uint8_t x = 0U; x < WIDTH; x++) {
    if (getPixColorXY(x, MAX_Y) == 0U) {
      if (random8(50U) == 0U) {
        if (deltaValue == 0U) {
          // Радужный дождь
          drawPixelXY(x, MAX_Y, CHSV((uint8_t)(random8(9U) * 28U), 255U, 255U));
        } else if (deltaValue == 1U) {
          // Снег
          drawPixelXY(x, MAX_Y, (uint32_t)(0xE0FFFF - 0x101010 * random8(4U)));
        } else {
          // Цветной дождь
          drawPixelXY(x, MAX_Y, CHSV((uint8_t)(deltaHue + random8(16U)), 255U, 255U));
        }

      }
    } else {
      // Постепенное угасание капель, задержавшихся на верхней строчке
      leds[XY(x, MAX_Y)] -= CHSV(0U, 0U, random8(96U, 128U));
    }
  }

  // сдвигаем всё вниз
  for (uint8_t x = 0U; x < WIDTH; x++) {
    for (uint8_t y = 0U; y < MAX_Y; y++) {
      drawPixelXY(x, y, getPixColorXY(x, (uint8_t)(y + 1U)));
    }
  }
}
#endif


#ifdef DEF_SCANNER
// ============== Scanner ==============
//             © SlingMaster
//                Сканер
// =====================================
static void Scanner() {
  constexpr bool v_scanner = (HEIGHT >= WIDTH);

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                         scale | speed
      setModeSettings(random8(0U, 100U), random8(128U, 255U));
    }
    deltaValue = 0U;
    #endif

    hue = modes[currentMode].Scale * 2.55f;
    deltaHue = modes[currentMode].Scale;
    hue2 = 5U;

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  if ((step & 0x01U) == 0U) { // step % 2U
    if (deltaValue == 0U) {
      hue2++;
    } else {
      hue2--;
    }
    if (deltaHue == 0U) {
      hue++;
    }
  }

  if (hue2 > 250U) {
    hue2 = 0U;
    deltaValue = 0U;
  }

  fadeToBlackBy(leds, NUM_LEDS, v_scanner ? 50U : 30U);

  if (v_scanner) {
    /* vertical scanner */
    if (hue2 >= MAX_Y) {
      deltaValue = 1U;
    }

    if (((hue2 & 0x01U) == 0U)) {
      const uint8_t spark_trigger_x = hue2 >> 1U; // hue2 / 2.0f

      for (uint8_t x = 0U; x < WIDTH; x++) {
        leds[XY(x, hue2)] = CHSV(hue, 255U, 180U);

        if (x == spark_trigger_x) {
          if (deltaValue == 0U) {
            const float rnd_offset = random8(2U) ? 1.5f : 1.0f;
            drawPixelXYF((float)random8(WIDTH) - rnd_offset, (float)hue2 * 0.9f, CHSV(hue, 16U, 255U));
          } else {
            drawPixelXYF((float)random8(WIDTH) - 1.5f, (float)hue2 * 1.1f, CHSV(hue, 16U, 255U));
          }
        }
      }
    } else {
      for (uint8_t x = 0U; x < WIDTH; x++) {
        leds[XY(x, hue2)] = CHSV(hue, 255U, 180U);
      }
    }
  } else {
    /* horizontal scanner */
    if (hue2 >= MAX_X) {
      deltaValue = 1U;
    }

    if (((hue2 & 0x01U) == 0U)) {
      const uint8_t spark_trigger_y = hue2 >> 1U; // hue2 / 2.0f через быстрый сдвиг

      for (uint8_t y = 0U; y < HEIGHT; y++) {
        leds[XY(hue2, y)] = CHSV(hue, 255U, 180U);

        if (y == spark_trigger_y) {
          if (deltaValue == 0U) {
            const float rnd_offset = random8(2U) ? 1.5f : 1.0f;
            drawPixelXYF((float)hue2 * 0.9f, (float)random8(HEIGHT) - rnd_offset, CHSV(hue, 16U, 255U));
          } else {
            drawPixelXYF((float)hue2 * 1.1f, (float)random8(HEIGHT) - 1.5f, CHSV(hue, 16U, 255U));
          }
        }
      }
    } else {
      for (uint8_t y = 0U; y < HEIGHT; y++) {
        leds[XY(hue2, y)] = CHSV(hue, 255U, 180U);
      }
    }
  }

  step++;
}
#endif


#ifdef DEF_MIRAGE
// =====================================
//                Mirage
//               © Stepko
//                Міраж
// =====================================
static uint8_t buff[WIDTH + 2U][HEIGHT + 2U];

// -------------------------------------
static void blur() {
  uint16_t sum;
  for (uint8_t y = 1U; y < (uint8_t)(HEIGHT + 1U); y++) {
    for (uint8_t x = 1U; x < (uint8_t)(WIDTH + 1U); x++) {
      sum = buff[x][y];
      sum += buff[x + 1U][y];
      sum += buff[x][y - 1U];
      sum += buff[x][y + 1U];
      sum += buff[x - 1U][y];
      sum /= 5U;
      buff[x][y] = (uint8_t)sum;
    }
  }
}

// -------------------------------------
static void drawDot(float x, float y, uint8_t a) {
  const uint8_t xx = (uint8_t)((x - (float)((int16_t)x)) * 255.0f);
  const uint8_t yy = (uint8_t)((y - (float)((int16_t)y)) * 255.0f);
  const uint8_t ix = 255U - xx;
  const uint8_t iy = 255U - yy;

  const uint8_t wu[4] = {
    (uint8_t)WU_WEIGHT(ix, iy),
    (uint8_t)WU_WEIGHT(xx, iy),
    (uint8_t)WU_WEIGHT(ix, yy),
    (uint8_t)WU_WEIGHT(xx, yy)
  };

  // multiply the intensities by the colour, and saturating-add them to the pixels
  for (uint8_t i = 0U; i < 4U; i++) {
    const int16_t xn = (int16_t)x + (i & 0x01U);
    const int16_t yn = (int16_t)y + ((i >> 1U) & 0x01U);

    buff[xn][yn] = qadd8(buff[xn][yn], (uint8_t)((a * wu[i]) >> 8U));
  }
}

// -------------------------------------
static void Mirage() {
  constexpr uint8_t divider = 4U;
  constexpr uint8_t val = 255U;

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), random8(80U, 255U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    hue = 70U;

    loadingFlag = false;
  }

  blur();

  const uint16_t speed_val = modes[currentMode].Speed;
  const uint16_t spd15 = 15U * speed_val;
  const uint16_t spd20 = 20U * speed_val;
  const uint16_t spd16 = 16U * speed_val;
  const uint16_t spd14 = 14U * speed_val;
  const uint16_t spd12 = 12U * speed_val;

  const float x1 = (float)beatsin88(spd15, divider, (uint16_t)(WIDTH * divider)) / (float)divider;
  const float y1 = (float)beatsin88(spd20, divider, (uint16_t)(HEIGHT * divider)) / (float)divider;
  const float x2 = (float)beatsin88(spd16, divider, (uint16_t)(MAX_X * divider)) / (float)divider;
  const float y2 = (float)beatsin88(spd14, divider, (uint16_t)(HEIGHT * divider)) / (float)divider;
  const float x3 = (float)beatsin88(spd12, divider, (uint16_t)(MAX_X * divider)) / (float)divider;
  const float y3 = (float)beatsin88(spd16, divider, (uint16_t)(HEIGHT * divider)) / (float)divider;

  drawDot(x1, y1, val);
  drawDot(x1 + 1.0f, y1, val);
  drawDot(x2, y2, val);
  drawDot(x2 + 1.0f, y2, val);
  drawDot(x3, y3, val);
  drawDot(x3 + 1.0f, y3, val);

  hue++;

  for (uint8_t y = 1U; y < (uint8_t)(HEIGHT + 1U); y++) {
    const uint8_t target_y = y - 1U;

    for (uint8_t x = 1U; x < (uint8_t)(WIDTH + 1U); x++) {
      leds[XY((uint8_t)(x - 1U), target_y)] = CHSV(hue, buff[x][y], 255U);
    }
  }
}
#endif


#ifdef DEF_HANDFAN
// ============== Hand Fan ==============
//           на основі коду від
//          © mastercat42@gmail.com
//             © SlingMaster
//                Опахало
// --------------------------------------

static void HandFan() {
  constexpr uint8_t V_STEP = 255U / (HEIGHT + 9U);

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), random8(210, 255U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    hue = (uint8_t)(modes[currentMode].Scale * 2.55f);
    deltaHue = map(modes[currentMode].Speed, 1U, 255U, 200U, 255U);

    loadingFlag = false;
  }

  nscale8_video(leds, NUM_LEDS, deltaHue);

  const bool scale_flag = (modes[currentMode].Scale > 95U);

  for (uint8_t i = 0U; i < HEIGHT; i++) {
    const uint8_t sin_val = sin8((uint8_t)(i + (millis() >> 4U)));
    const uint8_t tmp = map8(sin_val, 2U, (uint8_t)(WIDTH - 2U));

    const uint8_t center_x = WIDTH - tmp;
    const uint8_t side_bri = (uint8_t)(255U - V_STEP * i);

    leds[XY(center_x, i)]                 = CHSV(hue, (uint8_t)(V_STEP * i + 32U), 205U);
    leds[XY((int16_t)(center_x - 1), i)]  = CHSV(hue, 255U, side_bri);
    leds[XY((uint8_t)(center_x + 1U), i)] = CHSV(hue, 255U, side_bri);

    if ((i % 6U == 0U) && scale_flag) {
      hue++;
    }
  }
}
#endif


#ifdef DEF_LIGHTFILTER
// ============ Light Filter ============
//             © SlingMaster
//              Cвітлофільтр
// --------------------------------------
static void LightFilter() {
  constexpr uint8_t END = MAX_X;

  // static int64_t frameCount =  0;
  // static uint8_t dX;
  // static bool direct;
  // static uint8_t divider;
  // static uint8_t deltaValue = 0;

  // step                 => Используется напрямую как 8-битный frameCount!
  // hue2                 => Координата dX разделительной линии
  // deltaHue             => Значение divider масштаба
  // deltaHue2            => Текущий режим цветового фильтра (deltaValue)
  // hue                  => Направление движения линии direct (1U - вперед, 0U - назад)
  // pcnt                 => Внутренний таймер изменения направления

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), random8(40, 160U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    deltaHue = modes[currentMode].Scale / 25U; // divider
    hue = 1U;                                  // direct = true
    hue2 = 1U;                                 // dX = 1
    pcnt = 0U;
    deltaHue2 = 0U;                            // deltaValue = 0
    step = 0U;                                 // Наш 8-битный frameCount

    clearNoiseArr();

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  pcnt++;

  const uint8_t t1 = cos8((uint16_t)((42U * step) * inv30));
  const uint8_t t2 = cos8((uint16_t)((35U * step) * inv30));
  const uint8_t t3 = cos8((uint16_t)((38U * step) * inv30));

  uint8_t dX = hue2;
  uint8_t divider = deltaHue;
  uint8_t current_filter = deltaHue2;

  bool direct = (hue == 1U);
  if (direct) {
    if (dX < END) dX++;
  } else {
    if (dX > 0U) dX--;
  }

  if (pcnt > 128U) {
    pcnt = 0U;
    direct = !direct;
    if (divider > 2U) {
      if (dX == 0U) {
        current_filter++;
        if (current_filter > 2U) {
          current_filter = 0U;
        }
      }
    } else {
      current_filter = divider;
    }
  }

  hue2 = dX;
  deltaHue2 = current_filter;
  hue = direct ? 1U : 0U;

  const uint8_t val = dX << 3U;      // dX * 8
  const uint8_t val_div2 = dX << 2U; // val / 2 эквивалентно dX * 4
  const uint8_t line_gold_x = END - dX;

  const uint8_t t1_div2 = t1 >> 1U;
  const uint8_t t3_div2 = t3 >> 2U;

  for (uint16_t y = 0U; y < HEIGHT; y++) {
    for (uint16_t x = 0U; x < WIDTH; x++) {
      if (x != line_gold_x) {
        // Базовая плазма
        uint8_t r = cos8((y << 3) + (t1 >> 1) + cos8(t2 + (x << 3)));
        uint8_t g = cos8((y << 3) + t1 + cos8((t3 >> 2) + (x << 3)));
        uint8_t b = cos8((y << 3) + t2 + cos8(t1 + x + (g >> 2)));

        switch (current_filter) {
          case 0U:
            r = (r > val) ? (uint8_t)(r - val) : 0U;
            g = (g > val_div2) ? (uint8_t)(g - val_div2) : 0U;
            break;
          case 1U:
            g = (g > val) ? (uint8_t)(g - val) : 0U;
            b = (b > val_div2) ? (uint8_t)(b - val_div2) : 0U;
            break;
          case 2U:
            b = (b > val) ? (uint8_t)(b - val) : 0U;
            r = (r > val_div2) ? (uint8_t)(r - val_div2) : 0U;
            break;
        }

        leds[XY(x, y)] = CRGB(exp_gamma[r], exp_gamma[g], exp_gamma[b]);
      } else {
        // Золотая разделительная линия
        leds[XY(x, y)] = CRGB(255U, 255U, 255U);
      }
    }
  }
  hue++;
}
#endif


#ifdef DEF_RAINBOW_SPOT
// =========== Rainbow Spot ============
//             © SlingMaster
//            Веселкова Пляма
// =====================================
static void RainbowSpot() {
  constexpr uint8_t STEP = 255U / CENTER_X_MINOR;

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      setModeSettings(random8(100), random8(2, 254U));
    }
    #endif

    deltaValue = modes[currentMode].Scale;
    hue = 96U;
    emitterY = 0.0f;
    pcnt = 0U;

    ledsClear(); // esphome: FastLED.clear();

    loadingFlag = false;
  }

  // Calculate the radius based on the sound value --
  // Заменили деление на 127.0f быстрым умножением на инвариант (1.0f / 127.0f ≈ 0.007874f)
  const float radius = std::abs(128 - (int16_t)step) * inv127 * (float)max(CENTER_X_MINOR, CENTER_Y_MINOR);
  const float radiusSq = radius * radius;

  const uint8_t scale_val = modes[currentMode].Scale;
  const bool scale_gt50 = (scale_val > 50U);
  const bool scale_gt75 = (scale_val > 75U);

  const float center_x_offset = (float)CENTER_X_MINOR + 1.0f;
  const float center_y_offset = (float)CENTER_Y_MINOR + emitterY;

  // Loop through all matrix points -----------------
  for (uint8_t x = 0U; x < WIDTH; x++) {
    const float dx = (float)x - center_x_offset;
    const float dxSq = dx * dx;

    for (uint8_t y = 0U; y < HEIGHT; y++) {
      const float dy = (float)y - center_y_offset;
      const float dySq = dy * dy;

      const float y_anim_offset = (float)y - (float)(CENTER_Y_MINOR >> 1U) + emitterY; // CENTER_Y_MINOR / 2

      // Calculate the distance from the center to the current point
      const float distance = SQRT_VARIANT(dxSq + dySq);

      const uint8_t current_hue = step + (uint8_t)(distance * radius);

      // Check if the point is inside the radius ----
      const uint8_t current_delta = (uint8_t)(200U - (STEP * distance * inv4));
      const uint8_t inv_dist = (uint8_t)(255U - distance);

      if (distance < radius) {
        if (scale_gt50) {
          if ((x & 0x01U) && (y & 0x01U)) {
            drawPixelXYF((float)x, y_anim_offset, CHSV(current_hue, 255U, 64U));
          } else {
            leds[XY(x, y)] = CHSV((uint8_t)(current_hue + 32U), inv_dist, current_delta);
          }
        } else {
          leds[XY(x, y)] = CHSV(current_hue, inv_dist, 255U);
        }
      } else {
        const uint8_t final_hue = scale_gt75 ? (uint8_t)(current_hue + 96U) : current_hue;
        leds[XY(x, y)] = CHSV(final_hue, 255U, current_delta);
      }
    }
  }

  if (scale_gt50) {
    if (emitterY > (float)pcnt) {
      emitterY -= 0.25f;
    } else {
      if (emitterY < (float)pcnt) {
        emitterY += 0.25f;
      } else {
        pcnt = random8(CENTER_Y_MINOR);
      }
    }
  } else {
    emitterY = 0.0f;
  }

  blurScreen(48U);
  step++;
}
#endif

#ifdef DEF_RAINBOW_RINGS
// =========== Rainbow Rings ===========
//    base code © Martin Kleppe @aemkei
//             © SlingMaster
//            Радужные кольца
// =====================================

#define MAX_ACTIVE_RINGS (5U)

static void RainbowRings() {
  // static float ringRadii[MAX_ACTIVE_RINGS];  -> trackingObjectPosX[trackingOBJECT_MAX_COUNT];
  // static float ringFades[MAX_ACTIVE_RINGS];  -> trackingObjectPosY[trackingOBJECT_MAX_COUNT];
  // static uint8_t ringHues[MAX_ACTIVE_RINGS]; -> trackingObjectHue[trackingOBJECT_MAX_COUNT];
  // static uint8_t baseHue = 0;                -> hue;

  // trackingObjectPosX => Радиусы колец
  // trackingObjectPosY => Коэффициенты затухания колец
  // trackingObjectHue  => Оттенки колец
  // hue                => Базовый оттенок палитры (baseHue)

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                          scale | speed
      setModeSettings(10U + random8(90U), 100U + random8(100U));
    }
    #endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette();

    dimAll(0U);

    lastUpdateTime  = millis();
    colorChangeTime = millis();

    hue = map(modes[currentMode].Scale, 1U, 100U, 0U, 255U);

    constexpr float spacing = (float)max(CENTER_X_MAJOR, CENTER_Y_MAJOR) * 2.0f / (float)MAX_ACTIVE_RINGS;
    constexpr uint8_t hue_step = 256U / MAX_ACTIVE_RINGS;

    for (uint8_t i = 0U; i < MAX_ACTIVE_RINGS; i++) {
      trackingObjectPosX[i] = (float)i * spacing;
      trackingObjectHue[i]  = hue + (i * hue_step);
      trackingObjectPosY[i] = 1.0f;
    }

    loadingFlag = false;
  }

  const float speedFactor = (float)modes[currentMode].Speed * inv255;
  const uint32_t colorInterval = 300U - (uint32_t)(speedFactor * 200.0f);
  const uint32_t currentTime = millis();

  constexpr uint8_t hue_step = 256U / MAX_ACTIVE_RINGS;

  if (currentTime - colorChangeTime > colorInterval) {
    hue += 2U + (uint8_t)(speedFactor * 5.0f);

    for (uint8_t i = 0U; i < MAX_ACTIVE_RINGS; i++) {
      trackingObjectHue[i] = hue + (i * hue_step);
    }
    colorChangeTime = currentTime;
  }

  const uint8_t dimValue = map(modes[currentMode].Scale, 1U, 100U, 240U, 255U);
  dimAll(dimValue);

  const float ringSpeed = 0.6f + speedFactor * 2.4f;
  const float deltaTime = (float)(currentTime - lastUpdateTime) * inv1000;  // / 1000.0f

  // Обсчет физики расширения колец
  const float max_radius_limit = (float)max(CENTER_X_MAJOR, CENTER_Y_MAJOR) * 2.0f;
  for (uint8_t i = 0U; i < MAX_ACTIVE_RINGS; i++) {
    trackingObjectPosX[i] += ringSpeed * deltaTime;
    if (trackingObjectPosX[i] >= max_radius_limit) {
      trackingObjectPosX[i] = 0.0f;
      trackingObjectHue[i] = hue + (i * hue_step);
      trackingObjectPosY[i] = 1.0f;
    }
  }

  // Инварианты времени для тригонометрии волн
  const float t_seconds = (float)currentTime * inv1000;
  const float t_double = t_seconds * 2.0f;

  for (uint8_t y = 0U; y < HEIGHT; y++) {
    const float dy = (float)y - (float)CENTER_Y_MAJOR;
    const float dySq = dy * dy;

    for (uint8_t x = 0U; x < WIDTH; x++) {
      const float dx = (float)x - (float)CENTER_X_MAJOR;
      const float dxSq = dx * dx;

      const float distance = SQRT_VARIANT(dxSq + dySq);

      for (uint8_t i = 0U; i < MAX_ACTIVE_RINGS; i++) {
        const float radius = trackingObjectPosX[i];

        if (std::abs(distance - radius) < 2.0f) {
          const float wave = (float)sin16((int32_t)((t_double - distance + radius) * 8192.0f)) * inv32768; // 1.0f / 32767.0f ≈ 0.000030518f
          const float fraction = ((wave + 1.0f) * 0.35f) * trackingObjectPosY[i]; // 0.7f / 2.0f = 0.35f

          if (fraction > 0.01f) {
            const uint8_t brightness = (uint8_t)(fraction * 255.0f);
            drawPixelXY(x, y, ColorFromPalette(*curPalette, (uint8_t)trackingObjectHue[i], brightness));
          } else {
            drawPixelXY(x, y, 0x000000);
          }

        }
      }
    }
  }

  lastUpdateTime = currentTime;
}
#endif

#ifdef DEF_VYSHYVANKA
// ============ Vyshyvanka =============
//       (с) проект Aurora "Munch"
//     adopted/updated by kostyamat
//        updated by andrewjswan
//          Эффект "Вышиванка"
//          Munching Squares
// =====================================

static void munchRoutine() {
  constexpr uint8_t min_dim = (MAX_SIDE > 32U) ? 32U : 16U;
  constexpr uint8_t dim_sub = min_dim >> 1U;

  // trackingObjectPosX[0U] => Знаковый счетчик count (приводим к int8_t)
  // trackingObjectPosY[0U] => Знаковый шаг направления dir (приводим к int8_t)
  // deltaHue2             => Фаза инверсии паттерна flip
  // deltaHue              => Счетчик поколений фрактала generation
  // trackingObjectShift[0U] => Ячейка mic[0]
  // trackingObjectShift[1U] => Ячейка mic[1]

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                         scale | speed
      setModeSettings(1U + random8(90U), 140U + random8(100U));
    }
    #endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette();

    hue = 0U;              // generation = 0
    hue2 = 0U;             // flip = 0

    emitterX = 0.0f;       // count = 0
    emitterY = 1.0f;       // dir = 1

    // Инициализируем лимиты шагов фрактала
    pcnt = min_dim;        // mic
    deltaValue = min_dim;  // mic

    loadingFlag = false;
  }

  const uint8_t current_bri = modes[currentMode].Brightness;

  for (uint8_t x = 0U; x < min_dim; x++) {
    const uint8_t x_min_dim = x + min_dim;
    const bool x_in_width = (x < WIDTH);
    const bool x_dim_in_width = (x_min_dim < WIDTH);

    for (uint8_t y = 0U; y < min_dim; y++) {
      const uint8_t x_xor_y = x ^ y;
      const uint8_t y_min_dim = y + min_dim;
      const bool y_in_height = (y < HEIGHT);
      const bool y_dim_in_height = (y_min_dim < HEIGHT);

      CRGB color;
      if ((int8_t)(x_xor_y ^ hue2) < (int8_t)emitterX) {
        color = ColorFromPalette(*curPalette, (uint8_t)((x_xor_y << 4U) + hue), current_bri);
      } else {
        color = leds[XY(x, y)].subtractFromRGB(dim_sub);
      }

      // Тиражирование посчитанного фрактального блока по 4 квадрантам матрицы
      if (x_in_width && y_in_height) {
        leds[XY(x, y)] = color;
      }
      if (x_dim_in_width && y_in_height) {
        leds[XY(x_min_dim, y)] = color;
      }
      if (y_dim_in_height && x_in_width) {
        leds[XY(x, y_min_dim)] = color;
      }
      if (x_dim_in_width && y_dim_in_height) {
        leds[XY(x_min_dim, y_min_dim)] = color;
      }
    }
  }

  // Обсчет физики шага автомата состояний Munching Squares
  emitterX += emitterY; // count += dir

  if ((int8_t)emitterX <= 0 || (int8_t)emitterX >= (int8_t)pcnt) {
    emitterY = -emitterY; // dir = -dir

    if ((int8_t)emitterX <= 0) {
      pcnt = deltaValue; // mic[0] = mic[1]
      // Прямая инверсия фазы фрактала в регистре hue2 (flip)
      hue2 = (hue2 == 0U) ? (uint8_t)(deltaValue - 1U) : 0U;
    }
  }

  hue++;                // generation++
  deltaValue = min_dim; // mic[1] = minDimLocal
}
#endif

#ifdef DEF_INCREMENTAL_DRIFT
// ============ Incremental Drift =============
// The name "Incremental Drift" was coined by the
// late computer animation pioneer John Whitney,
// in his 1980 book, Digital Harmony. He describes
// a system of abstract motion graphics in which
// each particle moves successively faster, obeying
// simple ratios in accord with musical harmonics
// of 1/1, 1/2, 1/3, 1/4 and so on.
// =====================================
static void IncrementalDriftRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                          scale | speed
      setModeSettings(1U + random8(100U), 140U + random8(100U));
    }
    #endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    const uint8_t scale_val = modes[currentMode].Scale;
    if (scale_val < 20U) {
      curPalette = &RainbowColors_p;
    } else if (scale_val < 40U) {
      curPalette = &PartyColors_p;
    } else if (scale_val < 60U) {
      curPalette = &CloudColors_p;
    } else if (scale_val < 80U) {
      curPalette = &LavaColors_p;
    } else {
      curPalette = &ForestColors_p;
    }

    loadingFlag = false;
  }

  const uint8_t dim = beatsin8(2U, 170U, 250U);
  dimAll(dim);

  const bool mirror_flag = (modes[currentMode].Brightness > 128U);

  for (uint8_t i = 0U; i < WIDTH; i++)
  {
    CRGB color;
    uint8_t x = 0;
    uint8_t y = 0;

    // Симметричный расчет траекторий частиц относительно центра X
    if (i < CENTER_X) {
      const uint8_t freq = (uint8_t)((i - 1U) << 1U); // (i - 1) * 2
      x = beatcos8(freq, i, (uint8_t)(WIDTH - i - 1U));
      y = beatsin8(freq, i, (uint8_t)(HEIGHT - i - 1U));
      color = ColorFromPalette(*curPalette, (uint8_t)(i * 14U));
    }
    else
    {
      const uint8_t freq_w = (uint8_t)((WIDTH - i) << 1U);
      const uint8_t freq_h = (uint8_t)((HEIGHT - i) << 1U);
      x = beatsin8(freq_w, (uint8_t)(WIDTH - i - 1U), i);
      y = beatcos8(freq_h, (uint8_t)(HEIGHT - i - 1U), i);
      color = ColorFromPalette(*curPalette, (uint8_t)((MAX_X - i) * 14U));
    }

    drawPixelXY(x, y, color);

    if (mirror_flag) {
      drawPixelXY((uint8_t)(WIDTH - x), (uint8_t)(HEIGHT - y), color);
    }
  }
}
#endif

#ifdef DEF_FIRE_2025
// ======== Fire 2025 / Parus =========
//            (с) Parus
//        updated by andrewjswan
//        matrix-lamp/issues/86
// =====================================

// Фиксированные параметры, можно менять или привязать к слайдерам
constexpr uint8_t COAL_ZONE_HEIGHT = 1U;        // Высота зоны углей (строк снизу)
constexpr uint8_t SPARK_BRIGHT_MIN = 100U;      // Мин. яркость искр было 150
constexpr uint8_t SPARK_BRIGHT_MAX = 225U;      // Макс. яркость искр было 255
constexpr uint8_t spark_gap_probability = 20U;  // Вероятность промежутка между искрами (0–100 %)

// static uint8_t COOLING_RAW;  // 0–255
// static uint8_t COOLING;      // минимум 20!
// static uint8_t SPARKING;     // 0–255
// static uint8_t FIRE_SPEED;   // 0–100

// static uint8_t heat[WIDTH][HEIGHT]; --> noise3d[0][WIDTH][HEIGHT]

static void fire2025Routine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                          scale | speed
      setModeSettings(1U + random8(100U), 195U+random8(40U));
    }
    #endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    // Быстрая послойная очистка тепловой карты
    for (uint8_t x = 0U; x < WIDTH; x++) {
      for (uint8_t y = 0U; y < HEIGHT; y++) {
        noise3d[0U][x][y] = 0U;
      }
    }

    // Получаем значения со слайдеров
    deltaValue = 255U - modes[currentMode].Brightness;  // COOLING_RAW = 255 - static_cast<uint8_t>(modes[currentMode].Brightness );  // 0–255
    deltaHue = (deltaValue < 20U) ? 20U : deltaValue;   // COOLING = std::max(static_cast<uint8_t>(20), COOLING_RAW);                 // минимум 20!
    hue = modes[currentMode].Speed;                     // SPARKING = static_cast<uint8_t>(modes[currentMode].Speed);                 // 0–255
    hue2 = modes[currentMode].Scale;                    // FIRE_SPEED = static_cast<uint8_t>(modes[currentMode].Scale);               // 0–100
    emitterY = 0.0f;                                    // Текущая высота эмиттера физики
    pcnt = 0U;

    deltaHue2 = 10U + ((uint16_t)hue * 30U) / 255U;     // spark_value_pre
    ff_x = (uint16_t)(hue * 0.95f);                     // spark_chance_1
    ff_y = hue + (deltaValue >> 2U);

    loadingFlag = false;
  }

  const uint8_t cooling_raw = deltaValue;
  const uint8_t cooling_val = deltaHue;
  const uint8_t sparking_val = hue;
  const uint8_t fire_speed_val = hue2;
  const uint8_t spark_val = deltaHue2;

  const uint8_t extra_cooling = (fire_speed_val > 70U) ? (uint8_t)(fire_speed_val - 70U) : 0U;
  constexpr float inv_height = 1.0f / (float)HEIGHT;

  // Остывание
  for (uint8_t y = 0U; y < HEIGHT; y++) {
    // Остывание сильнее сверху, слабее снизу
    const uint8_t cooling_factor = (y < CENTER_Y) ? (uint8_t)(cooling_val >> 1U) : cooling_val;
    const uint8_t cooling_limit = (uint8_t)(((uint16_t)cooling_factor * 10U) / HEIGHT) + 2U + extra_cooling;
    const bool is_tip_zone = (y > THIRD_Y);

    for (uint8_t x = 0U; x < WIDTH; x++) {
      uint8_t heat_val = noise3d[0U][x][y];
      heat_val = qsub8(heat_val, random8(cooling_limit));

      // Мягкое тление кончиков пламени для слабых языков
      if (is_tip_zone && (heat_val < 50U)) {
        heat_val = qadd8(heat_val, random8(spark_val));
      }
      noise3d[0U][x][y] = heat_val;
    }
  }

  // Распространение тепла снизу вверх (с учётом FIRE_SPEED)
  const int16_t speed_boost = (fire_speed_val > 50U) ? (int16_t)(fire_speed_val - 50U) : 0;
  const uint16_t height_x2 = HEIGHT << 1U;

  for (uint8_t y = MAX_Y; y > 0U; y--) {
    const int16_t boost_factor = (speed_boost * (HEIGHT - y)) / height_x2;
    const uint8_t natural_decay = (uint8_t)(((uint16_t)y * 10U) / HEIGHT);
    const uint8_t current_y = y;

    for (uint8_t x = 0U; x < WIDTH; x++) {
      // Подъем тепла
      int16_t new_heat = (noise3d[0U][x][current_y] + noise3d[0U][x][current_y - 1U]) >> 1U;
      if (speed_boost > 0) {
        new_heat += boost_factor;
        if (new_heat > 220) new_heat = 220;
      }

      // Накладываем естественное затухание по высоте
      noise3d[0U][x][current_y] = qsub8((uint8_t)new_heat, natural_decay);
    }
  }

  // Накладываем затухание для самой нижней строки (индекс 0)
  for (uint8_t x = 0U; x < WIDTH; x++) {
    noise3d[0U][x][0U] = qsub8(noise3d[0U][x][0U], 0U);
  }

  // Искры в зоне углей
  const uint8_t spark_height_limit = COAL_ZONE_HEIGHT + (sparking_val / 100U);

  for (uint8_t x = 0U; x < WIDTH; x++) {
    // Естественные искры углей
    if (random8() < ff_x) {
      const uint8_t spark_y = random8(spark_height_limit);
      if (random8() > spark_gap_probability) {
        noise3d[0U][x][spark_y] = qadd8(noise3d[0U][x][spark_y], random8(SPARK_BRIGHT_MIN, SPARK_BRIGHT_MAX));
      }
    }
    // Вспомогательные искры от интенсивности охлаждения
    if (random8() < ff_y) {
      const uint8_t spark_y = random8(COAL_ZONE_HEIGHT);
      noise3d[0U][x][spark_y] = qadd8(noise3d[0U][x][spark_y], random8(SPARK_BRIGHT_MIN, SPARK_BRIGHT_MAX));
    }
  }

  // Вывод
  const uint8_t dark_gap_threshold = (HEIGHT * 2U) / 5U;

  // Предрассчитанный коэффициент влияния масштаба скорости на красный ореол
  const float scale_speed_factor = 1.0f + ((float)(fire_speed_val * 2U) * inv100);

  for (uint8_t y = 0U; y < HEIGHT; y++) {
    // ВЫНОС ИНВАРИАНТОВ СТРОКИ РЕНДЕРИНГА
    const bool is_dark_gap_zone = (y > dark_gap_threshold);
    const bool is_coal_zone = (y < COAL_ZONE_HEIGHT);

    // Расчет красного ореола
    const float red_boost = 1.0f + 0.3f * (1.0f - (float)y * inv_height) * scale_speed_factor;
    const uint16_t red_boost_fixed = (uint16_t)(red_boost * 256.0f);

    for (uint8_t x = 0U; x < WIDTH; x++) {
      const uint8_t temp = noise3d[0U][x][y];
      const uint8_t colorindex = scale8(temp, 240U);

      uint16_t r = 0U;
      uint16_t g = 0U;
      uint16_t b = 0U;

      if (colorindex < 85U) {
        // Оптимизация: Перевели мерцание капли и форму волны В ЦЕЛЫЕ ЧИСЛА БЕЗ FLOAT!
        const uint16_t flicker_fixed = 179U + (random8(50U) * 218U) / 255U;
        const uint16_t shape_fixed = 205U + (sin8((uint8_t)(y * 5U + x * 3U)) * 51U) / 255U;

        uint32_t r_calc = ((uint32_t)colorindex * 640U * flicker_fixed) >> 16U;
        r_calc = (r_calc * shape_fixed) >> 8U;
        r = (r_calc > 255) ? 255U : (uint16_t)r_calc;

        g = ((uint32_t)colorindex * 25U * flicker_fixed) >> 16U;
      }
      else if (colorindex < 200U) {
        r = 255U;
        g = (uint16_t)((colorindex - 85U) << 1U);
      }
      else {
        r = 255U;
        g = 255U;
        b = (uint16_t)((colorindex - 200U) * 3U);
      }

      // Наложение темных дымовых промежутков в верхней части пламени
      if (is_dark_gap_zone) {
        r = (r * 179U) >> 8U;
        g = (g * 77U) >> 8U;
      }

      // Физика цвета горящих угольков нижней зоны
      if (is_coal_zone) {
        if (temp > 220U)      { r = 255U; g = 230U; b = 150U; }
        else if (temp > 180U) { r = 255U; g = 200U; b = 50U;  }
        else if (temp > 100U) { r = 255U; g = 100U; b = 0U;   }
        else if (temp > 50U)  { r = 200U; g = 50U;  b = 0U;   }
        else {
          r = (uint16_t)((uint16_t)temp * 384U) >> 8U;
          g = (uint16_t)((uint16_t)temp * 77U) >> 8U;
          b = 0U;
        }
      }

      // Применение динамического красного ореола
      uint32_t final_r = ((uint32_t)r * red_boost_fixed) >> 8U;
      if (final_r > 255) final_r = 255U;

      drawPixelXY(x, y, CRGB((uint8_t)final_r, (uint8_t)g, (uint8_t)b));
    }
  }
}
#endif

#ifdef DEF_BUTTERFLY
// ============= Butterfly ============
//           (c) FieryLedLamp
//         adopted by andrewjswan
//                БАБОЧКА
// =====================================
static void butterflyRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                          scale | speed
      setModeSettings(10U + random8(90U), 100U + random8(100U));
    }
    #endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette();

    dimAll(0U);

    colorChangeTime = 0U;

    const uint8_t max_allowed = (enlargedOBJECT_MAX_COUNT < 5U) ? (uint8_t)enlargedOBJECT_MAX_COUNT : 5U;
    enlargedObjectNUM = map(modes[currentMode].Scale, 1U, 100U, 1U, max_allowed);
    hue = map(modes[currentMode].Scale, 1U, 100U, 0U, 255U);

    const uint8_t hue_step = 256U / enlargedObjectNUM;

    for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = random8(HEIGHT);

      trackingObjectSpeedX[i] = (float)random8(10U, 20U) * inv10 * (random8(2U) ? 1.0f : -1.0f);
      trackingObjectSpeedY[i] = (float)random8(10U, 20U) * inv10 * (random8(2U) ? 1.0f : -1.0f);
      trackingObjectHue[i] = hue + (i * hue_step);
      trackingObjectState[i] = 0U;
      trackingObjectIsShift[i] = true;
      enlargedObjectTime[i] = millis();
    }

    loadingFlag = false;
  }

  const float current_speed_factor = (float)modes[currentMode].Speed * inv255;
  const uint32_t colorInterval = (300U - (uint32_t)(current_speed_factor * 200.0f));
  const uint8_t hue_step = 256U / enlargedObjectNUM;
  const uint32_t current_time = millis();

  if (current_time - colorChangeTime > colorInterval) {
    hue += 2U + (uint8_t)(current_speed_factor * 5.0f);
    for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
      trackingObjectHue[i] = hue + (i * hue_step);
    }
    colorChangeTime = current_time;
  }

  dimAll(230U);

  for (uint8_t i = 0U; i < enlargedObjectNUM; i++) {
    if (!trackingObjectIsShift[i]) {
      continue;
    }

    trackingObjectPosX[i] += trackingObjectSpeedX[i] * current_speed_factor;
    trackingObjectPosY[i] += trackingObjectSpeedY[i] * current_speed_factor;

    if (trackingObjectPosX[i] < 0.0f || trackingObjectPosX[i] >= (float)WIDTH) {
      trackingObjectSpeedX[i] = -trackingObjectSpeedX[i];
      trackingObjectPosX[i] = constrain(trackingObjectPosX[i], 0.0f, (float)MAX_X);
    }
    if (trackingObjectPosY[i] < 0.0f || trackingObjectPosY[i] >= (float)HEIGHT) {
      trackingObjectSpeedY[i] = -trackingObjectSpeedY[i];
      trackingObjectPosY[i] = constrain(trackingObjectPosY[i], 0.0f, (float)MAX_Y);
    }

    const uint8_t wingPhase = (current_time - enlargedObjectTime[i]) / 100U;
    const float wingSize = 1.0f + 0.5f * sin((float)wingPhase * M_PI / 8.0f);

    const CRGB color = ColorFromPalette(*curPalette, trackingObjectHue[i]);
    const CRGB wing_color = makeDarker(color, 50U);

    drawPixelXYF(trackingObjectPosX[i], trackingObjectPosY[i], color);
    drawPixelXYF(trackingObjectPosX[i] + wingSize, trackingObjectPosY[i] + wingSize, wing_color);
    drawPixelXYF(trackingObjectPosX[i] - wingSize, trackingObjectPosY[i] + wingSize, wing_color);
    drawPixelXYF(trackingObjectPosX[i] + wingSize, trackingObjectPosY[i] - wingSize, wing_color);
    drawPixelXYF(trackingObjectPosX[i] - wingSize, trackingObjectPosY[i] - wingSize, wing_color);
  }
}
#endif

#ifdef DEF_STARS_NIGHT
// ============= Stars Night ============
//           (c) FieryLedLamp
//          adopted by andrewjswan
//              НОВЫЕ ЗВЁЗДЫ
// ======================================

#define MAX_STARS (30U)

static void StarsEffect() {
  // trackingObjectPosX    => Координата X звезды
  // trackingObjectPosY    => Координата Y звезды
  // trackingObjectSpeedX  => Скорость мерцания speed
  // trackingObjectSpeedY  => Размер звезды size (1.0f или 2.0f)
  // trackingObjectShift   => Оставшееся время жизни lifetime
  // trackingObjectHue     => Оттенок hue
  // trackingObjectState   => Фаза яркости brightness (упакована в байт 0..255)
  // trackingObjectIsShift => Флаг активности active (true / false)
  // deltaHue2             => Количество активных звезд activeStars
  // deltaValue            => Значение затухания dimValue
  // deltaHue              => Целевое количество звезд desiredStars

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                          scale | speed
      setModeSettings(10U + random8(90U), 100U + random8(100U));
    }
    #endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    setCurrentPalette();
    dimAll(0U);

    lastUpdateTime = millis();
    hue = map(modes[currentMode].Scale, 1U, 100U, 0U, 255U);

    deltaValue = map(modes[currentMode].Scale, 1U, 100U, 225U, 240U);   // dimValue
    deltaHue = map(modes[currentMode].Scale, 1U, 100U, 3U, MAX_STARS);  // desiredStars
    deltaHue2 = 0U;                                                     // activeStars = 0

    for (uint8_t i = 0U; i < MAX_STARS; i++) {
      trackingObjectIsShift[i] = false;                                 // active = false
    }

    loadingFlag = false;
  }

  dimAll(deltaValue);

  const uint32_t currentTime = millis();
  const uint8_t desiredStars = deltaHue;

  const float deltaTime = (float)(currentTime - lastUpdateTime) * inv1000;
  const float speedFactor = (float)modes[currentMode].Speed * inv255;

  // Предрассчитанный коэффициент скорости мерцания на текущий кадр
  const float speed_multiplier = deltaTime * (0.8f + speedFactor * 2.5f);

  const int8_t w_limit = (int8_t)WIDTH;
  const int8_t h_limit = (int8_t)HEIGHT;

  // 1. Обсчет физики и рендеринг существующих звезд
  for (uint8_t i = 0U; i < MAX_STARS; i++) {
    if (trackingObjectIsShift[i]) {                                      // Проверка active

      // Накапливаем фазу яркости: скорость (speedX) * коэффициент кадра.
      // Переводим шаг во встроенный байтовый масштаб (сдвиг радианов 0..TWO_PI в 0..255)
      // В радианах шаг был speed * speed_multiplier. В байтах это умножается на (255.0f / TWO_PI) ≈ 40.584f
      float step_phase = trackingObjectSpeedX[i] * speed_multiplier * 40.584f;
      float next_phase = (float)trackingObjectState[i] + step_phase;

      // Аппаратное циклическое зацикливание байта 0..255 срабатывает само при кастинге!
      trackingObjectState[i] = (uint8_t)next_phase;

      // Расчет дыхания звезд через быструю табличную sin8()!
      const uint8_t phase1 = trackingObjectState[i];
      // Вторая гармоника на частоте 1.5x: phase * 1.5 -> phase + phase / 2
      const uint8_t phase2 = (uint8_t)(phase1 + (phase1 >> 1U));

      // sin8 выдает диапазон 0..255. Перемножаем амплитуды
      uint16_t bright_calc = ((uint16_t)sin8(phase1) * sin8(phase2)) >> 8U;
      // Масштабируем до пиковых значений (bright_calc * 200 / 255) -> примерно * 0.784
      uint8_t pixelBright = (uint8_t)((bright_calc * 200U) >> 8U);

      trackingObjectShift[i] -= deltaTime;                               // Уменьшаем lifetime

      const uint8_t x = (uint8_t)trackingObjectPosX[i];
      const uint8_t y = (uint8_t)trackingObjectPosY[i];

      if (pixelBright > 5U && trackingObjectShift[i] > 0.0f) {
        const CRGB color = CHSV((uint8_t)trackingObjectHue[i], 200U, pixelBright);

        if (trackingObjectSpeedY[i] <= 1.0f) {                           // Проверка size
          drawPixelXY(x, y, color);
        } else {
          drawPixelXY(x, y, color);
          if ((int8_t)(x + 1U) < w_limit) drawPixelXY((uint8_t)(x + 1U), y, color);
          if ((int8_t)(y + 1U) < h_limit) drawPixelXY(x, (uint8_t)(y + 1U), color);
          if (((int8_t)(x + 1U) < w_limit) && ((int8_t)(y + 1U) < h_limit)) drawPixelXY((uint8_t)(x + 1U), (uint8_t)(y + 1U), color);
        }
      } else {
        // Гашение звезды при окончании времени жизни или яркости
        if (trackingObjectSpeedY[i] <= 1.0f) {
          drawPixelXY(x, y, 0x000000);
        } else {
          drawPixelXY(x, y, 0x000000);
          if ((int8_t)(x + 1U) < w_limit) drawPixelXY((uint8_t)(x + 1U), y, 0x000000);
          if ((int8_t)(y + 1U) < h_limit) drawPixelXY(x, (uint8_t)(y + 1U), 0x000000);
          if (((int8_t)(x + 1U) < w_limit) && ((int8_t)(y + 1U) < h_limit)) drawPixelXY((uint8_t)(x + 1U), (uint8_t)(y + 1U), 0x000000);
        }
        trackingObjectIsShift[i] = false; // active = false
        deltaHue2--;                                                     // activeStars--
      }
    }
  }

  // 2. Спавн и рождение новых звезд при нехватке лимита
  if (deltaHue2 < desiredStars) {
    const uint8_t spawnChance = 10U + (uint8_t)(speedFactor * 15.0f);
    if (random8(100U) < spawnChance) {
      for (uint8_t i = 0U; i < MAX_STARS; i++) {
        if (!trackingObjectIsShift[i]) {
          trackingObjectPosX[i] = (float)random8(WIDTH);
          trackingObjectPosY[i] = (float)random8(HEIGHT);
          trackingObjectHue[i]  = hue + random8(32U);
          trackingObjectState[i] = random8();

          trackingObjectSpeedX[i] = (float)random8(6U, 19U) * inv10;     // random(600U, 1800U) / 1000.0f
          trackingObjectSpeedY[i] = (random8(100U) < 20U) ? 2.0f : 1.0f; // size
          trackingObjectShift[i]  = (float)random8(2U, 6U);              // random(2000U, 5000U) / 1000.0f lifetime

          trackingObjectIsShift[i] = true;                               // active = true
          deltaHue2++;                                                   // activeStars++
          break;
        }
      }
    }
  }

  lastUpdateTime = currentTime;
}
#endif

#ifdef DEF_TETRIX
// =============== Tetrix ===============
//                (c) WLED
//          adopted by andrewjswan
//                Тетрикс
// ======================================

constexpr uint8_t MAX_FALLING_LINES = (WIDTH > 4U) ? ((WIDTH * 4U) / 10U) : 2U;
constexpr uint8_t MAX_BRICK_HEIGHT = (HEIGHT / 3U < 2U) ? 2U : (HEIGHT / 3U);
constexpr uint8_t START_PROBABILITY = (320U / WIDTH < 5U) ? 5U : (320U / WIDTH);

constexpr float SPEED_FACTOR_MIN = 250.0f;   // 500.0f
constexpr float SPEED_FACTOR_MAX = 5000.0f;  // 8000.0f
constexpr float SPEED_FACTOR_RANGE = SPEED_FACTOR_MAX - SPEED_FACTOR_MIN;

static void tetrixRoutine() {
  const uint32_t currentMillis = millis();
  const uint8_t current_speed = modes[currentMode].Speed;
  const uint8_t current_scale = modes[currentMode].Scale;

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                          scale | speed
      setModeSettings(1U + random8(255U), 1U + random8(255U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    dimAll(0U);

    for (uint8_t x = 0U; x < WIDTH; x++) {
      enlargedObjectTime[x] = (long)(currentMillis + random16(0U, 4000U));
      trackingObjectState[x] = 3U;    // Состояние 3: стартовая очистка/затухание
      trackingObjectPosY[x] = 0.0f;   // Обнуляем высоту стакана для каждой колонки
    }

    // Выбор цветовой схемы на основе ползунка Масштаб (Scale)
    if (current_scale <= 42U) {
      currentPalette = RainbowColors_p;
    } else if (current_scale <= 84U) {
      currentPalette = PartyColors_p;
    } else if (current_scale <= 126U) {
      currentPalette = OceanColors_p;
    } else if (current_scale <= 168U) {
      currentPalette = CloudColors_p;
    } else if (current_scale <= 210U) {
      currentPalette = ForestColors_p;
    } // else (211 - 255) - HSV спектр

    loadingFlag = false;
  }

  const float speedFactor = SPEED_FACTOR_MIN + SPEED_FACTOR_RANGE * (1.0f - (float)current_speed * inv255);
  const float baseSpeed = ((float)HEIGHT * 30.0f) / speedFactor;

  uint8_t currentlyFallingCount = 0U;
  for (uint8_t x = 0U; x < WIDTH; x++) {
    if (trackingObjectState[x] == 2U) {
      currentlyFallingCount++;
    }
  }

  bool brickStartedThisFrame = false;

  hue += 1U;

  // Основной цикл обработки колонок матрицы
  for (uint8_t x = 0U; x < WIDTH; x++) {
    const uint8_t currentStepState = trackingObjectState[x];

    // --- Состояние 0: Инициализация нового кирпичика ---
    if (currentStepState == 0U) {
      if ((uint8_t)trackingObjectPosY[x] >= HEIGHT - 1U) {
        enlargedObjectTime[x] = (long)(currentMillis + 2000U);
        trackingObjectState[x] = 3U; // Переходим в режим угасания
        continue;
      }

      trackingObjectSpeedY[x] = baseSpeed * (1.0f + (float)random8(0, 4) * 0.1f);
      trackingObjectPosX[x]   = (float)HEIGHT; // Старт из-за верхней границы матрицы

      // Привязываем оттенок кирпича к hue
      trackingObjectHue[x] = hue + (x * 4U);

      // Вычисляем размер кирпича на основе масштаба
      const uint8_t brickSize = random8(1U, MAX_BRICK_HEIGHT);

      trackingObjectShift[x] = (float)brickSize;
      trackingObjectState[x] = 1U;
      continue;
    }

    // --- Состояние 1: Ожидание случайного старта падения ---
    if (currentStepState == 1U) {
      // Разрешаем старт падения только если лимит не превышен и в этом кадре еще никто не зародился
      if (!brickStartedThisFrame && currentlyFallingCount < MAX_FALLING_LINES) {
        if (random8() < START_PROBABILITY) {
          trackingObjectState[x] = 2U;
          currentlyFallingCount++;            // Учитываем взлетевший кирпич
          brickStartedThisFrame = true;       // Блокируем запуск других линий в этом кадре
        }
      }

      // Зачищаем фон над стаканом, пока кирпич ожидает вылета
      const uint8_t stackHeight = (uint8_t)trackingObjectPosY[x];
      for (uint8_t y = stackHeight; y < HEIGHT; y++) {
        leds[XY(x, y)] = CRGB::Black;
      }
      continue;
    }

    // --- Состояние 2: Процесс падения кирпичика ---
    if (currentStepState == 2U) {
      const float stackPos = trackingObjectPosY[x];  // Высота стакана из trackingObjectPosY
      float currentPos = trackingObjectPosX[x];

      if (currentPos > stackPos) {
        currentPos -= trackingObjectSpeedY[x];       // Падаем вниз
        if (currentPos < stackPos) {
          currentPos = stackPos;
        }
        trackingObjectPosX[x] = currentPos;

        const uint8_t intPos = (uint8_t)currentPos;
        const uint8_t brickTop = intPos + (uint8_t)trackingObjectShift[x];
        const uint8_t currentHue = trackingObjectHue[x];

        // Выбор цветовой схемы на основе ползунка Масштаб (Scale)
        CRGB brickColor;
        if (current_scale <= 210U) {
          brickColor = ColorFromPalette(currentPalette, currentHue);
        } else {
          brickColor = CHSV(currentHue, 255U, 255U);
        }

        for (uint8_t y = 0U; y < HEIGHT; y++) {
          if (y < (uint8_t)stackPos) {
            // Стакан внизу не трогаем, сохраняя его старые цвета
            continue;
          }
          else if (y >= intPos && y < brickTop) {
            leds[XY(x, y)] = brickColor;
          } else {
            leds[XY(x, y)] = CRGB::Black;
          }
        }
      } else {
        // Фиксация кирпичика на стаке: увеличиваем высоту стакана
        trackingObjectPosY[x] += trackingObjectShift[x];

        if ((uint8_t)trackingObjectPosY[x] >= HEIGHT) {
          enlargedObjectTime[x] = (long)(currentMillis + 2000U);
          trackingObjectState[x] = 3U;               // Переходим в режим угасания
        } else {
          // После падения отправляем колонку на небольшую паузу отдыха перед новым циклом
          enlargedObjectTime[x] = (long)(currentMillis + random16(200U, 1500U));
          trackingObjectState[x] = 4U;               // Переходим в режим паузы
        }
      }
      continue;
    }

    // --- Состояние 3: Плавное угасание стакана при заполнении ---
    if (currentStepState == 3U) {
      if (enlargedObjectTime[x] > (long)currentMillis) {
        // Гашение пикселей столбца X
        for (uint8_t y = 0U; y < HEIGHT; y++) {
          leds[XY(x, y)].nscale8(220U);
        }
      } else {
        // Таймер угасания истек: сброс стакана строго в существующем массиве
        trackingObjectPosY[x] = 0.0f;
        trackingObjectState[x] = 0U;
      }
      continue; // Переход на следующую итерацию
    }

    // --- Состояние 4: Искусственная пауза отдыха между кирпичами в одной колонке ---
    if (currentStepState == 4U) {
      if ((long)currentMillis >= enlargedObjectTime[x]) {
        trackingObjectState[x] = 0U; // Пауза прошла, колонка снова готова создавать кирпич
      }
      // Пока колонка отдыхает, удерживаем стакан и фон в стабильном состоянии
      const uint8_t stackHeight = (uint8_t)trackingObjectPosY[x];
      for (uint8_t y = stackHeight; y < HEIGHT; y++) {
        leds[XY(x, y)] = CRGB::Black;
      }
      continue; // Переход на следующую итерацию
    }
  }
}
#endif

}  // namespace esphome::matrix_lamp
