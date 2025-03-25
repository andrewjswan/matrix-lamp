#pragma once

#define FASTLED_INTERNAL // remove annoying pragma messages

#include "FastLED.h"
#include "esphome.h"
#include "common.h"
#include "constants.h"
#include "effect_data.h"
#include "effect_palette.h"
#include "utility.h"

#define SQRT_VARIANT sqrt3                         // выбор основной функции для вычисления квадратного корня sqrtf или sqrt3 для ускорения

namespace esphome {
namespace matrix_lamp {

// ============= ЭФФЕКТЫ ===============
// несколько общих переменных и буферов, которые могут использоваться в любом эффекте
#define NUM_LAYERSMAX 2

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

static const uint8_t maxDim = max(WIDTH, HEIGHT);

//массивы состояния объектов, которые могут использоваться в любом эффекте
#define trackingOBJECT_MAX_COUNT                         (100U)              // максимальное количество отслеживаемых объектов (очень влияет на расход памяти)
static float    trackingObjectPosX[trackingOBJECT_MAX_COUNT];
static float    trackingObjectPosY[trackingOBJECT_MAX_COUNT];
static float    trackingObjectSpeedX[trackingOBJECT_MAX_COUNT];
static float    trackingObjectSpeedY[trackingOBJECT_MAX_COUNT];
static float    trackingObjectShift[trackingOBJECT_MAX_COUNT];
static uint8_t  trackingObjectHue[trackingOBJECT_MAX_COUNT];
static uint8_t  trackingObjectState[trackingOBJECT_MAX_COUNT];
static bool     trackingObjectIsShift[trackingOBJECT_MAX_COUNT];

#define enlargedOBJECT_MAX_COUNT                     (WIDTH * 2)            // максимальное количество сложных отслеживаемых объектов (меньше, чем trackingOBJECT_MAX_COUNT)
static uint8_t  enlargedObjectNUM;                                          // используемое в эффекте количество объектов
static long     enlargedObjectTime[enlargedOBJECT_MAX_COUNT];
static float    liquidLampHot[enlargedOBJECT_MAX_COUNT];        
static float    liquidLampSpf[enlargedOBJECT_MAX_COUNT];        
static unsigned liquidLampMX[enlargedOBJECT_MAX_COUNT];        
static unsigned liquidLampSC[enlargedOBJECT_MAX_COUNT];        
static unsigned liquidLampTR[enlargedOBJECT_MAX_COUNT];        

static uint8_t custom_eff = 0;

//константы размера матрицы вычисляется только здесь и не меняется в эффектах
static const uint8_t CENTER_X_MINOR =  (WIDTH / 2) - ((WIDTH  - 1) & 0x01); // центр матрицы по ИКСУ, сдвинутый в меньшую сторону, если ширина чётная
static const uint8_t CENTER_Y_MINOR = (HEIGHT / 2) - ((HEIGHT - 1) & 0x01); // центр матрицы по ИГРЕКУ, сдвинутый в меньшую сторону, если высота чётная
static const uint8_t CENTER_X_MAJOR =   WIDTH / 2  + (WIDTH  % 2);          // центр матрицы по ИКСУ, сдвинутый в большую сторону, если ширина чётная
static const uint8_t CENTER_Y_MAJOR =  HEIGHT / 2  + (HEIGHT % 2);          // центр матрицы по ИГРЕКУ, сдвинутый в большую сторону, если высота чётная

// --------------------------------------------------------------------------------------

#ifdef DEF_SPARKLES
// ------------- конфетти --------------
#define FADE_OUT_SPEED        (70U)                         // скорость затухания
static void sparklesRoutine()
{
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(4U+random8(97U), 99U+random8(125U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    for (uint16_t i = 0; i < NUM_LEDS; i++)
      if (random8(3U))
        leds[i].nscale8(random8());
      else
        leds[i] = 0U;
  }
  
  for (uint8_t i = 0; i < modes[currentMode].Scale; i++)
  {
    uint8_t x = random8(WIDTH);
    uint8_t y = random8(HEIGHT);
    if (getPixColorXY(x, y) == 0U)
    {
      leds[XY(x, y)] = CHSV(random8(), 255U, 255U);
    }
  }
  //fader(FADE_OUT_SPEED);
  dimAll(256U - FADE_OUT_SPEED);
}
#endif


#ifdef DEF_WATERFALL
// =============- новый огонь / водопад -===============
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 55, suggested range 20-100
#define COOLINGNEW 32
// 8  практически сплошной поток красивой подсвеченной воды ровным потоком сверху донизу. будто бы на столе стоит маленький "родничок"
// 20 ровный водопад с верщиной на свету, где потоки летящей воды наверху разбиваются ветром в белую пену
// 32 уже не ровный водопад, у которого струи воды долетают до земли неравномерно
// чем больше параметр, тем больше тени снизу
// 55 такое, как на видео

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKINGNEW 80 // 30 // 120 // 90 // 60
// 80 почти все белые струи сверху будут долетать до низа - хорошо при выбранном ползунке Масштаб = 100 (белая вода без подкрашивания)
// 50 чуть больше половины будет долетать. для цветных вариантов жидкости так более эффектно

static void fire2012WithPalette() {
  #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      setModeSettings(random8(7U) ? 46U+random8(26U) : 100U, 195U+random8(40U));
    }
  #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

  for (uint8_t x = 0; x < WIDTH; x++)
  {
    // Step 1.  Cool down every cell a little
    for (uint8_t i = 0; i < HEIGHT; i++)
    {
      noise3d[0][x][i] = qsub8(noise3d[0][x][i], random8(0, ((COOLINGNEW * 10) / HEIGHT) + 2));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (uint8_t k = HEIGHT - 1; k >= 2; k--)
    {
      noise3d[0][x][k] = (noise3d[0][x][k - 1] + noise3d[0][x][k - 2] + noise3d[0][x][k - 2]) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if (random8() < SPARKINGNEW)
    {
      uint8_t y = random8(2);
      noise3d[0][x][y] = qadd8(noise3d[0][x][y], random8(160, 255));
    }

    // Step 4.  Map from heat cells to LED colors
    for (uint8_t j = 0; j < HEIGHT; j++)
    {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      byte colorindex = scale8(noise3d[0][x][j], 240);
      if (modes[currentMode].Scale == 100)
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(WaterfallColors_p, colorindex);
      else
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(CRGBPalette16( CRGB::Black, CHSV(modes[currentMode].Scale * 2.57, 255U, 255U) , CHSV(modes[currentMode].Scale * 2.57, 128U, 255U) , CRGB::White), colorindex);// 2.57 вместо 2.55, потому что 100 для белого цвета
    }
  }
}
#endif


#ifdef DEF_FIRE
// ------------- Огонь -----------------
#define SPARKLES              (1U)                       // вылетающие угольки вкл выкл
#define UNIVERSE_FIRE                                    // универсальный огонь 2-в-1 Цветной+Белый
 
//uint8_t pcnt = 0U;                                     // внутренний делитель кадров для поднимающегося пламени - переменная вынесена в общий пул, чтобы использовать повторно
//uint8_t deltaHue = 16U;                                // текущее смещение пламени (hueMask) - переменная вынесена в общий пул, чтобы использовать повторно
//uint8_t shiftHue[HEIGHT];                              // массив дороожки горизонтального смещения пламени (hueMask) - вынесен в общий пул массивов переменных
//uint8_t deltaValue = 16U;                              // текущее смещение пламени (hueValue) - переменная вынесена в общий пул, чтобы использовать повторно
//uint8_t shiftValue[HEIGHT];                            // массив дороожки горизонтального смещения пламени (hueValue) - вынесен в общий пул массивов переменных

// these values are substracetd from the generated values to give a shape to the animation
// static const uint8_t valueMask[8][16] PROGMEM =

// these are the hues for the fire,
// should be between 0 (red) to about 25 (yellow)
// static const uint8_t hueMask[8][16] PROGMEM =

static unsigned char matrixValue[8][16];                 // это массив для эффекта Огонь

static void generateLine();
static void shiftUp();
static void drawFrame(uint8_t pcnt, bool isColored);

static void fireRoutine(bool isColored)
{
  if (loadingFlag) {
    memset(matrixValue, 0, sizeof(matrixValue)); // это массив для эффекта Огонь. странно, что его нужно залить нулями
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(random8(30U) ? 1U+random8(100U) : 100U, 200U+random8(35U));
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
    line[x] = random(127, 255);                             // заполнение случайным образом нижней линии (127, 255) - менее контрастное, (64, 255) - оригинал
  }
}

//---------------------------------------
static void shiftUp() {                                     // подъем кадра
  for (uint8_t y = HEIGHT - 1U; y > 0U; y--) {
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
  uint8_t baseHue = (float)(modes[currentMode].Scale - 1U) * 2.6;
#else
  uint8_t baseHue = isColored ? 255U : 0U;
#endif
  uint8_t baseSat = (modes[currentMode].Scale < 100) ? 255U : 0U;  // вычисление базового оттенка


  //first row interpolates with the "next" line
  deltaHue = random(0U, 2U) ? constrain (shiftHue[0] + random(0U, 2U) - random(0U, 2U), 15U, 17U) : shiftHue[0]; 
  // random(0U, 2U)= скорость смещения языков чем больше 2U - тем медленнее
  // 15U, 17U - амплитуда качания -1...+1 относительно 16U
  // высчитываем плавную дорожку смещения всполохов для нижней строки
  // так как в последствии координаты точки будут исчисляться из остатка, то за базу можем принять кратную ширину матрицы hueMask
  // ширина матрицы hueMask = 16, поэтому нам нужно получить диапазон чисел от 15 до 17
  // далее к предыдущему значению прибавляем случайную 1 и отнимаем случайную 1 - это позволит плавным образом менять значение смещения
  shiftHue[0] = deltaHue; // заносим это значение в стэк

  deltaValue = random(0U, 3U) ? constrain (shiftValue[0] + random(0U, 2U) - random(0U, 2U), 15U, 17U) : shiftValue[0];
  // random(0U, 3U)= скорость смещения очага чем больше 3U - тем медленнее
  // 15U, 17U - амплитуда качания -1...+1 относительно 16U
  shiftValue[0] = deltaValue;


  for (uint8_t x = 0U; x < WIDTH; x++) {                                                    // прорисовка нижней строки (сначала делаем ее, так как потом будем пользоваться ее значением смещения)
    uint8_t newX = x % 16;                                                                  // сократил формулу без доп. проверок
    nextv =                                                                                 // расчет значения яркости относительно valueMask и нижерасположенной строки.
      (((100.0 - pcnt) * matrixValue[0][newX] + pcnt * line[newX]) / 100.0)
      - pgm_read_byte(&valueMask[0][(x + deltaValue) % 16U]);
    CRGB color = CHSV(                                                                      // вычисление цвета и яркости пикселя
                   baseHue + pgm_read_byte(&hueMask[0][(x + deltaHue) % 16U]),              // H - смещение всполохов
                   baseSat,                                                                 // S - когда колесо масштаба =100 - белый огонь (экономим на 1 эффекте)
                   (uint8_t)max((int32_t)0, nextv)                                          // V
                 );
    leds[XY(x, 0)] = color;                                                                 // прорисовка цвета очага
  }

  // Each row interpolates with the one before it
  for (uint8_t y = HEIGHT - 1U; y > 0U; y--) {                                              // прорисовка остальных строк с учетом значения низлежащих
    deltaHue = shiftHue[y];                                                                 // извлекаем положение
    shiftHue[y] = shiftHue[y - 1];                                                          // подготавлеваем значение смешения для следующего кадра основываясь на предыдущем
    deltaValue = shiftValue[y];                                                             // извлекаем положение
    shiftValue[y] = shiftValue[y - 1];                                                      // подготавлеваем значение смешения для следующего кадра основываясь на предыдущем


    if (y > 8U) {                                                                           // цикл стирания текущей строоки для искр
      for (uint8_t _x = 0U; _x < WIDTH; _x++) {                                             // стираем строчку с искрами (очень не оптимально)
        drawPixelXY(_x, y, 0U);
      }
    }
    for (uint8_t x = 0U; x < WIDTH; x++) {                                                  // пересчет координаты x для текущей строки
      uint8_t newX = x % 16U;                                                               // функция поиска позиции значения яркости для матрицы valueMask
      if (y < 8U) {                                                                         // если строка представляет очаг
        nextv =                                                                             // расчет значения яркости относительно valueMask и нижерасположенной строки.
          (((100.0 - pcnt) * matrixValue[y][newX]
            + pcnt * matrixValue[y - 1][newX]) / 100.0)
          - pgm_read_byte(&valueMask[y][(x + deltaValue) % 16U]);

        CRGB color = CHSV(                                                                  // определение цвета пикселя
                       baseHue + pgm_read_byte(&hueMask[y][(x + deltaHue) % 16U ]),         // H - смещение всполохов
                       baseSat,                                                             // S - когда колесо масштаба =100 - белый огонь (экономим на 1 эффекте)
                       (uint8_t)max((int32_t)0, nextv)                                      // V
                     );
        leds[XY(x, y)] = color;
      }
      else if (y == 8U && SPARKLES) {                                                       // если это самая нижняя строка искр - формитуем искорку из пламени
        if (random(0, 20) == 0 && getPixColorXY(x, y - 1U) != 0U)
          drawPixelXY(x, y, getPixColorXY(x, y - 2U));                                      // 20 = обратная величина количества искр
        else
          drawPixelXY(x, y, 0U);
      }
      else if (SPARKLES) {                                                                  // если это не самая нижняя строка искр - перемещаем искорку выше
        // старая версия для яркости
        newX = (random(0, 4)) ? x : (x + WIDTH + random(0U, 2U) - random(0U, 2U)) % WIDTH ; // с вероятностью 1/3 смещаем искорку влево или вправо
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
  for (uint8_t i = 0U; i < (isVertical?WIDTH:HEIGHT); i++) {
    CHSV thisColor = CHSV((uint8_t)(hue + i * (modes[currentMode].Scale % 67U) * 2U), 255U, 255U);

    for (uint8_t j = 0U; j < (isVertical?HEIGHT:WIDTH); j++)
      drawPixelXY((isVertical?i:j), (isVertical?j:i), thisColor);
  }
}

static void rainbowRoutine() {
  #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      uint8_t tmp = 7U+random8(50U);
      if (tmp>14) tmp += 19U;
      if (tmp>67) tmp += 6U;
      setModeSettings(tmp , 150U+random8(86U));
    }
  #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

  hue += 4U;
  if (modes[currentMode].Scale < 34U)                                        // если масштаб до 34
    rainbowHorVertRoutine(false);
  else if (modes[currentMode].Scale > 67U)                                   // если масштаб больше 67
    rainbowHorVertRoutine(true);
  else                                                                       // для масштабов посередине
    for (uint8_t i = 0U; i < WIDTH; i++)
      for (uint8_t j = 0U; j < HEIGHT; j++)
      {
        float twirlFactor = 9.0F * ((modes[currentMode].Scale-33) / 100.0F); // на сколько оборотов будет закручена матрица, [0..3]
        CRGB thisColor = CHSV((uint8_t)(hue + ((float)WIDTH / (float)HEIGHT * i + j * twirlFactor) * ((float)255 / (float)maxDim)), 255U, 255U);
        drawPixelXY(i, j, thisColor);
      }
}
#endif


#if defined(DEF_PULSE) || defined(DEF_PULSE_RAINBOW) || defined(DEF_PULSE_WHITE)
// -------------- эффект пульс ------------
// Stefan Petrick's PULSE Effect mod by PalPalych for GyverLamp
static void drawCircle(int x0, int y0, int radius, const CRGB &color){
  int a = radius, b = 0;
  int radiusError = 1 - a;

  if (radius == 0) {
    drawPixelXY(x0, y0, color);
    return;
  }

  while (a >= b)  {
    drawPixelXY(a + x0, b + y0, color);
    drawPixelXY(b + x0, a + y0, color);
    drawPixelXY(-a + x0, b + y0, color);
    drawPixelXY(-b + x0, a + y0, color);
    drawPixelXY(-a + x0, -b + y0, color);
    drawPixelXY(-b + x0, -a + y0, color);
    drawPixelXY(a + x0, -b + y0, color);
    drawPixelXY(b + x0, -a + y0, color);
    b++;
    if (radiusError < 0)
      radiusError += 2 * b + 1;
    else
    {
      a--;
      radiusError += 2 * (b - a + 1);
    }
  }
}

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
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(1U+random8(100U), 170U+random8(62U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    CRGB _pulse_color;
  
    dimAll(248U);

    uint8_t _sat;
    if (step <= pcnt) {
      for (uint8_t i = 0; i < step; i++ ) {
        uint8_t _dark = qmul8( 2U, cos8 (128U / (step + 1U) * (i + 1U))) ;
        switch (PMode) {
          case 1U:                    // 1 - случайные диски
            deltaHue = hue;
            _pulse_color = CHSV(deltaHue, 255U, _dark);
            break;
          case 2U:                    // 2...17 - перелив цвета дисков
            deltaHue2 = modes[currentMode].Scale;
            _pulse_color = CHSV(hue2, 255U, _dark);
            break;
          case 3U:                    // 18...33 - выбор цвета дисков
            deltaHue = modes[currentMode].Scale * 2.55;
            _pulse_color = CHSV(deltaHue, 255U, _dark);
            break;
          case 4U:                    // 34...50 - дискоцветы
            deltaHue += modes[currentMode].Scale;
            _pulse_color = CHSV(deltaHue, 255U, _dark);
            break;
          case 5U:                    // 51...67 - пузыри цветы
            _sat =  qsub8( 255U, cos8 (128U / (step + 1U) * (i + 1U))) ;
            deltaHue += modes[currentMode].Scale;
            _pulse_color = CHSV(deltaHue, _sat, _dark);
            break;
          case 6U:                    // 68...83 - выбор цвета пузырей
            _sat =  qsub8( 255U, cos8 (128U / (step + 1U) * (i + 1U))) ;
            deltaHue = modes[currentMode].Scale * 2.55;
            _pulse_color = CHSV(deltaHue, _sat, _dark);
            break;
          case 7U:                    // 84...99 - перелив цвета пузырей
            _sat =  qsub8( 255U, cos8 (128U / (step + 1U) * (i + 1U))) ;
            deltaHue2 = modes[currentMode].Scale;
            _pulse_color = CHSV(hue2, _sat, _dark);
            break;
          case 8U:                    // 100 - случайные пузыри
            _sat =  qsub8( 255U, cos8 (128U / (step + 1U) * (i + 1U))) ;
            deltaHue2 = modes[currentMode].Scale;
            _pulse_color = CHSV(hue2, _sat, _dark);
            break;
        }
        drawCircle(emitterX, emitterY, i, _pulse_color  );
      }
    }
    else
    {
      emitterX = random8(WIDTH - 5U) + 3U;
      emitterY = random8(HEIGHT - 5U) + 3U;
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
        setModeSettings(47U+random8(28U), 201U+random8(38U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    hue = modes[currentMode].Scale * 2.55;
    fillAll(CHSV(hue, 255U, 255U));
    deltaHue = 0U;
    deltaHue2 = 0U;
  }

  if (modes[currentMode].Speed != 255U) // если регулятор скорости на максимуме, то будет работать старый эффект "цвет" (без анимации бликов воды)
  {
    if (step > 24U)                     // количество кадров в анимации -1 (отсчёт с нуля)
      step = 0U;
    if (step > 0U && step < 3U)         // пару раз за цикл анимации двигаем текстуру по радиусу лампы. а может и не двигаем. как повезёт
    {
      if (random(2U) == 0U)
      {
        deltaHue++;
        if (deltaHue > 31U) deltaHue = 0U;
      }
    }
    if (step > 11U && step < 14U)       // пару раз за цикл анимации двигаем текстуру по вертикали. а может и не двигаем. как повезёт
    {
      if (random(2U) == 0U)
      {
        deltaHue2++;
        if (deltaHue2 > 31U) deltaHue2 = 0U;
      }
    }

    for (uint8_t x = 0U; x < WIDTH ; x++) {
      for (uint8_t y = 0U; y < HEIGHT; y++) {
        // y%32, x%32 - это для масштабирования эффекта на лампы размером большим, чем размер анимации 32х32, а также для произвольного сдвига текстуры
        leds[XY(x, y)] = CHSV(hue, 255U - pgm_read_byte(&aquariumGIF[step][(y + deltaHue2) % 32U][(x + deltaHue) % 32U]) * CAUSTICS_BR / 100U, 255U);
        // чтобы регулятор Масштаб начал вместо цвета регулировать яркость бликов, нужно закомментировать предыдущую строчку и раскоментировать следующую
        // leds[XY(x, y)] = CHSV(158U, 255U - pgm_read_byte(&aquariumGIF[step][(y+deltaHue2)%32U][(x+deltaHue)%32U]) * modes[currentMode].Scale / 100U, 255U);
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
        setModeSettings(1U+random8(255U), 210U+random8(46U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    deltaValue = 255U - modes[currentMode].Speed + 1U;
    step = deltaValue;                                                   // чтообы при старте эффекта сразу покрасить лампу (для бугунка Масштаб от 246 до 9)
    deltaHue = 1U;                                                       // чтообы при старте эффекта сразу покрасить лампу (для бегунка Масштаб от 10 до 245)
    hue2 = 0U;
  }

  if (modes[currentMode].Scale < 10U || modes[currentMode].Scale > 245U) // если Масштаб небольшой, меняем цвет на это значение регулярно (каждый цикл кратный значению Скорость)
    if (step >= deltaValue){
      hue += modes[currentMode].Scale;
      step = 0U;
      fillAll(CHSV(hue, 255U, 255U));
    }
    else
      step++;
  else                                                                   // если Масштаб большой, тогда смену цвета делаем как бы пульсацией (поменяли, пауза, поменяли, пауза)
    if (deltaHue != 0){
      if (deltaHue > 127U){
        hue--;
        deltaHue++;
      }
      else {
        hue++;
        deltaHue--;
      }
      fillAll(CHSV(hue, 255U, 255U));
    }
    else
      if (step >= deltaValue){
        deltaHue = modes[currentMode].Scale;
        step = 0U;
      }
      else
        if (hue2 >= DELAY_MULTIPLIER) {
          step++;
          hue2 = 0U;
        }
        else 
          hue2++;
}
#endif


#ifdef DEF_SNOW
// ------------- снегопад ----------
static void snowRoutine()
{
  #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      setModeSettings(88U+random8(9U), 170U+random8(36U));
    }
  #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
  
  // сдвигаем всё вниз
  for (uint8_t x = 0U; x < WIDTH; x++)
  {
    for (uint8_t y = 0U; y < HEIGHT - 1; y++)
    {
      drawPixelXY(x, y, getPixColorXY(x, y + 1U));
    }
  }

  for (uint8_t x = 0U; x < WIDTH; x++)
  {
    // заполняем случайно верхнюю строку
    // а также не даём двум блокам по вертикали вместе быть
    if (getPixColorXY(x, HEIGHT - 2U) == 0U && (random(0, 100 - modes[currentMode].Scale) == 0U))
      drawPixelXY(x, HEIGHT - 1U, 0xE0FFFF - 0x101010 * random(0, 4));
    else
      drawPixelXY(x, HEIGHT - 1U, 0x000000);
  }
}
#endif


#ifdef DEF_STARFALL
// ------------- метель - 2 -------------
//SNOWSTORM / МЕТЕЛЬ # STARFALL / ЗВЕЗДОПАД ***** V1.2
// v1.0 - Updating for GuverLamp v1.7 by PalPalych 12.03.2020
// v1.1 - Fix wrong math & full screen drawing by PalPalych 14.03.2020
// v1.2 - Code optimisation + pseudo 3d by PalPalych 21.04.2020
#define e_sns_DENSE (32U) // плотность снега - меньше = плотнее

static void stormRoutine2()// (bool isColored) // сворачиваем 2 эффекта в 1
{
  #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      uint8_t tmp = 175U+random8(39U);
      if (tmp & 0x01)
        setModeSettings(50U+random8(51U), tmp);
      else
        setModeSettings(50U+random8(24U), tmp);
    }
  #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

bool isColored = modes[currentMode].Speed & 0x01; // сворачиваем 2 эффекта в 1
  // заполняем головами комет
  uint8_t Saturation = 0U;    // цвет хвостов
  uint8_t e_TAIL_STEP = 127U; // длина хвоста
  if (isColored)
    Saturation = modes[currentMode].Scale * 2.55;
  else
  {
    e_TAIL_STEP = 255U - modes[currentMode].Scale * 2.5;
  }
  for (uint8_t x = 0U; x < WIDTH - 1U; x++) // fix error i != 0U
  {
    if (!random8(e_sns_DENSE) &&
        !getPixColorXY(wrapX(x), HEIGHT - 1U) &&
        !getPixColorXY(wrapX(x + 1U), HEIGHT - 1U) &&
        !getPixColorXY(wrapX(x - 1U), HEIGHT - 1U))
    {
      drawPixelXY(x, HEIGHT - 1U, CHSV(random8(), Saturation, random8(64U, 255U)));
    }
  }

  // сдвигаем по диагонали
  for (uint8_t y = 0U; y < HEIGHT - 1U; y++)
  {
    for (uint8_t x = 0; x < WIDTH; x++)
    {
      drawPixelXY(wrapX(x + 1U), y, getPixColorXY(x, y + 1U));
    }
  }

  // уменьшаем яркость верхней линии, формируем "хвосты"
  for (uint8_t i = 0U; i < WIDTH; i++)
  {
    fadePixel(i, HEIGHT - 1U, e_TAIL_STEP);
  }
}
#endif


#ifdef DEF_MATRIX
// ------------- матрица ---------------
static void matrixRoutine()
{
  #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      setModeSettings(1U+random8(90U), 165U+random8(66U));
    }
  #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

  for (uint8_t x = 0U; x < WIDTH; x++)
  {
    // обрабатываем нашу матрицу снизу вверх до второй сверху строчки
    for (uint8_t y = 0U; y < HEIGHT - 1U; y++)
    {
      uint32_t thisColor  = getPixColorXY(x, y);                                                 // берём цвет нашего пикселя
      uint32_t upperColor = getPixColorXY(x, y + 1U);                                            // берём цвет пикселя над нашим
      if (upperColor >= 0x900000 && random(7 * HEIGHT) != 0U)                                    // если выше нас максимальная яркость, игнорим этот факт с некой вероятностью или опускаем цепочку ниже
        drawPixelXY(x, y, upperColor);
      else if (thisColor == 0U && random((100 - modes[currentMode].Scale) * HEIGHT) == 0U)       // если наш пиксель ещё не горит, иногда зажигаем новые цепочки
        //else if (thisColor == 0U && random((100 - modes[currentMode].Scale) * HEIGHT*3) == 0U) // для длинных хвостов
        drawPixelXY(x, y, 0x9bf800);
      else if (thisColor <= 0x050800)                                                            // если наш пиксель почти погас, стараемся сделать затухание медленней
      {
        if (thisColor >= 0x030000)
          drawPixelXY(x, y, 0x020300);
        else if (thisColor != 0U)
          drawPixelXY(x, y, 0U);
      }
      else if (thisColor >= 0x900000)                                                            // если наш пиксель максимальной яркости, резко снижаем яркость
        drawPixelXY(x, y, 0x558800);
      else
        drawPixelXY(x, y, thisColor - 0x0a1000);                                                 // в остальных случаях снижаем яркость на 1 уровень
        // drawPixelXY(x, y, thisColor - 0x050800);                                              // для длинных хвостов
    }

    // аналогично обрабатываем верхний ряд пикселей матрицы
    uint32_t thisColor = getPixColorXY(x, HEIGHT - 1U);
    if (thisColor == 0U)                                                                         // если наш верхний пиксель не горит, заполняем его с вероятностью .Scale
    {
      if (random(100 - modes[currentMode].Scale) == 0U)
        drawPixelXY(x, HEIGHT - 1U, 0x9bf800);
    }
    else if (thisColor <= 0x050800)                                                             // если наш верхний пиксель почти погас, стараемся сделать затухание медленней
    {
      if (thisColor >= 0x030000)
        drawPixelXY(x, HEIGHT - 1U, 0x020300);
      else
        drawPixelXY(x, HEIGHT - 1U, 0U);
    }
    else if (thisColor >= 0x900000)                                                             // если наш верхний пиксель максимальной яркости, резко снижаем яркость
      drawPixelXY(x, HEIGHT - 1U, 0x558800);
    else
      drawPixelXY(x, HEIGHT - 1U, thisColor - 0x0a1000);                                        // в остальных случаях снижаем яркость на 1 уровень
      // drawPixelXY(x, HEIGHT - 1U, thisColor - 0x050800);                                     // для длинных хвостов
  }
}
#endif


#if defined(DEF_BUTTERFLYS) || defined(DEF_BUTTERFLYS_LAMP)
// ------------- Светлячки 2 - Светлячки в банке - Мотыльки - Лампа с мотыльками --------------
// (c) SottNick

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
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        if (isColored){
          uint8_t tmp = 66U+random8(83U);
          setModeSettings((tmp & 0x01) ? 65U+random8(36U) : 15U+random8(26U), tmp);
        }
        else
          setModeSettings(random8(21U) ? (random8(3U) ? 2U+random8(98U) : 1U) : 100U, 20U+random8(155U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

  bool isWings = modes[currentMode].Speed & 0x01;
  if (loadingFlag)
  {
    loadingFlag = false;
    speedfactor = (float)modes[currentMode].Speed / 2048.0f + 0.001f;
    if (isColored)                         // для режима смены цвета фона фиксируем количество мотыльков
      deltaValue = (modes[currentMode].Scale > trackingOBJECT_MAX_COUNT) ? trackingOBJECT_MAX_COUNT : modes[currentMode].Scale; 
    else
      deltaValue = BUTTERFLY_FIX_COUNT;
    for (uint8_t i = 0U; i < trackingOBJECT_MAX_COUNT; i++)
    {
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = random8(HEIGHT);
      trackingObjectSpeedX[i] = 0;
      trackingObjectSpeedY[i] = 0;
      trackingObjectShift[i] = 0;
      trackingObjectHue[i] = (isColored) ? random8() : 255U;
      trackingObjectState[i] = 255U;
    }
    //для инверсии, чтобы сто раз не пересчитывать
    if (modes[currentMode].Scale != 1U)
      hue = (float)(modes[currentMode].Scale - 1U) * 2.6;
    else
      hue = random8();
    if (modes[currentMode].Scale == 100U){ // вместо белого будет желтоватая лампа
      hue2 = 170U;
      hue = 31U;
    }
    else
     hue2 = 255U;
  }
  if (isWings && isColored)
    dimAll(35U);                           // для крылышков
  else
    ledsClear();                           // esphome: FastLED.clear();

  float maxspeed;
  uint8_t tmp;
  if (++step >= deltaValue)
    step = 0U;
  for (uint8_t i = 0U; i < deltaValue; i++)
  {
    trackingObjectPosX[i] += trackingObjectSpeedX[i]*speedfactor;
    trackingObjectPosY[i] += trackingObjectSpeedY[i]*speedfactor;

    if (trackingObjectPosX[i] < 0)
      trackingObjectPosX[i] = (float)(WIDTH - 1) + trackingObjectPosX[i];
    if (trackingObjectPosX[i] > WIDTH - 1)
      trackingObjectPosX[i] = trackingObjectPosX[i] + 1 - WIDTH;

    if (trackingObjectPosY[i] < 0)
    {
      trackingObjectPosY[i] = -trackingObjectPosY[i];
      trackingObjectSpeedY[i] = -trackingObjectSpeedY[i];
    }
    if (trackingObjectPosY[i] > HEIGHT - 1U)
    {
      trackingObjectPosY[i] = (HEIGHT << 1U) - 2U - trackingObjectPosY[i];
      trackingObjectSpeedY[i] = -trackingObjectSpeedY[i];
    }

    // проворот траектории
    maxspeed = fabs(trackingObjectSpeedX[i])+fabs(trackingObjectSpeedY[i]);  // максимальная суммарная скорость
    if (maxspeed == fabs(trackingObjectSpeedX[i] + trackingObjectSpeedY[i]))
    {
      if (trackingObjectSpeedX[i] > 0)                                       // правый верхний сектор вектора
      {
        trackingObjectSpeedX[i] += trackingObjectShift[i];
        if (trackingObjectSpeedX[i] > maxspeed)                              // если вектор переехал вниз
        {
          trackingObjectSpeedX[i] = maxspeed + maxspeed - trackingObjectSpeedX[i];
          trackingObjectSpeedY[i] = trackingObjectSpeedX[i] - maxspeed;
        }
        else
          trackingObjectSpeedY[i] = maxspeed - fabs(trackingObjectSpeedX[i]);
      }
      else                                                                   // левый нижний сектор
      {
        trackingObjectSpeedX[i] -= trackingObjectShift[i];
        if (trackingObjectSpeedX[i] + maxspeed < 0)                          // если вектор переехал вверх
        {
          trackingObjectSpeedX[i] = 0 - trackingObjectSpeedX[i] - maxspeed - maxspeed;
          trackingObjectSpeedY[i] = maxspeed - fabs(trackingObjectSpeedX[i]);
        }
        else
          trackingObjectSpeedY[i] = fabs(trackingObjectSpeedX[i]) - maxspeed;
      }
    }
    else //л евый верхний и правый нижний секторы вектора
    {
      if (trackingObjectSpeedX[i] > 0)                                       // правый нижний сектор
      {
        trackingObjectSpeedX[i] -= trackingObjectShift[i];
        if (trackingObjectSpeedX[i] > maxspeed)                              // если вектор переехал наверх
        {
          trackingObjectSpeedX[i] = maxspeed + maxspeed - trackingObjectSpeedX[i];
          trackingObjectSpeedY[i] = maxspeed - trackingObjectSpeedX[i];
        }
        else
          trackingObjectSpeedY[i] = fabs(trackingObjectSpeedX[i]) - maxspeed;
      }
      else                                                                   // левый верхний сектор
      {
        trackingObjectSpeedX[i] += trackingObjectShift[i];
        if (trackingObjectSpeedX[i] + maxspeed < 0)                          // если вектор переехал вниз
        {
          trackingObjectSpeedX[i] = 0 - trackingObjectSpeedX[i] - maxspeed - maxspeed;
          trackingObjectSpeedY[i] = 0 - trackingObjectSpeedX[i] - maxspeed;
        }
        else
          trackingObjectSpeedY[i] = maxspeed - fabs(trackingObjectSpeedX[i]);
      }
    }
    
    if (trackingObjectState[i] == 255U)
    {
      if (step == i && random8(2U) == 0U)
      {
        trackingObjectState[i] = random8(220U,244U);
        trackingObjectSpeedX[i] = (float)random8(101U) / 20.0f + 1.0f;
        if (random8(2U) == 0U) trackingObjectSpeedX[i] = -trackingObjectSpeedX[i];
        trackingObjectSpeedY[i] = (float)random8(101U) / 20.0f + 1.0f;
        if (random8(2U) == 0U) trackingObjectSpeedY[i] = -trackingObjectSpeedY[i];
        // проворот траектории
        trackingObjectShift[i] = (float)random8((fabs(trackingObjectSpeedX[i])+fabs(trackingObjectSpeedY[i]))*20.0f+2.0f) / 200.0f;
        if (random8(2U) == 0U) trackingObjectShift[i] = -trackingObjectShift[i];
      }
    }
    else 
    {
      if (step == i)
        trackingObjectState[i]++;
      tmp = 255U - trackingObjectState[i];
      if (tmp == 0U || ((uint16_t)(trackingObjectPosX[i] * tmp) % tmp == 0U && (uint16_t)(trackingObjectPosY[i] * tmp) % tmp == 0U))
      {
        trackingObjectPosX[i] = round(trackingObjectPosX[i]);
        trackingObjectPosY[i] = round(trackingObjectPosY[i]);
        trackingObjectSpeedX[i] = 0;
        trackingObjectSpeedY[i] = 0;
        trackingObjectShift[i] = 0;
        trackingObjectState[i] = 255U;
      }
    }

    if (isWings)
      // это процедура рисования с нецелочисленными координатами. ищите её в прошивке
      drawPixelXYF(trackingObjectPosX[i], trackingObjectPosY[i], CHSV(trackingObjectHue[i], 255U, (trackingObjectState[i] == 255U) ? 255U : 128U + random8(2U) * 111U));
    else
      // это процедура рисования с нецелочисленными координатами. ищите её в прошивке
      drawPixelXYF(trackingObjectPosX[i], trackingObjectPosY[i], CHSV(trackingObjectHue[i], 255U, trackingObjectState[i]));
  }

  // постобработка кадра
  if (isColored){
    for (uint8_t i = 0U; i < deltaValue; i++) // ещё раз рисуем всех Мотыльков, которые "сидят на стекле"
      if (trackingObjectState[i] == 255U)
        drawPixelXY(trackingObjectPosX[i], trackingObjectPosY[i], CHSV(trackingObjectHue[i], 255U, trackingObjectState[i]));
  }
  else {
    //теперь инверсия всей матрицы
    if (modes[currentMode].Scale == 1U)
      if (++deltaHue == 0U) hue++;
    for (uint16_t i = 0U; i < NUM_LEDS; i++)
      leds[i] = CHSV(hue, hue2, 255U - leds[i].r);
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
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(14U+random8(43U), 100U+random8(81U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;

    if (modes[currentMode].Scale > trackingOBJECT_MAX_COUNT) modes[currentMode].Scale = trackingOBJECT_MAX_COUNT;
    for (uint8_t i = 0U; i < trackingOBJECT_MAX_COUNT; i++)
    {
      trackingObjectPosX[i]   = random(0, WIDTH * 10);
      trackingObjectPosY[i]   = random(0, HEIGHT * 10);
      trackingObjectSpeedX[i] = random(-10, 10);
      trackingObjectSpeedY[i] = random(-10, 10);
      trackingObjectHue[i]    = random8();
    }
  }

  ledsClear(); // esphome: FastLED.clear();
  
  if (++step > 20U) step = 0U;
  for (uint8_t i = 0U; i < modes[currentMode].Scale; i++)
  {
    if (step == 0U) // меняем скорость каждые 255 отрисовок
    {
      trackingObjectSpeedX[i] += random(-3, 4);
      trackingObjectSpeedY[i] += random(-3, 4);
      trackingObjectSpeedX[i] = constrain(trackingObjectSpeedX[i], -20, 20);
      trackingObjectSpeedY[i] = constrain(trackingObjectSpeedY[i], -20, 20);
    }

    trackingObjectPosX[i] += trackingObjectSpeedX[i];
    trackingObjectPosY[i] += trackingObjectSpeedY[i];

    if (trackingObjectPosX[i] < 0) trackingObjectPosX[i] = (WIDTH - 1) * 10;
    if (trackingObjectPosX[i] >= (int32_t)(WIDTH * 10)) trackingObjectPosX[i] = 0;

    if (trackingObjectPosY[i] < 0)
    {
      trackingObjectPosY[i] = 0;
      trackingObjectSpeedY[i] = -trackingObjectSpeedY[i];
    }
    if (trackingObjectPosY[i] >= (int32_t)(HEIGHT - 1) * 10)
    {
      trackingObjectPosY[i] = (HEIGHT - 1U) * 10;
      trackingObjectSpeedY[i] = -trackingObjectSpeedY[i];
    }
    drawPixelXY(trackingObjectPosX[i] / 10, trackingObjectPosY[i] / 10, CHSV(trackingObjectHue[i], 255U, 255U));
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
      setModeSettings(1U+random8(100U) , 190U+random8(31U));
    }
    #endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;

    for (uint8_t j = 0U; j < BALLS_AMOUNT; j++)
    {
      int8_t sign;
      // забиваем случайными данными
      coord[j][0U] = WIDTH / 2 * 10;
      random(0, 2) ? sign = 1 : sign = -1;
      vector[j][0U] = random(4, 15) * sign;
      coord[j][1U] = HEIGHT / 2 * 10;
      random(0, 2) ? sign = 1 : sign = -1;
      vector[j][1U] = random(4, 15) * sign;

      // цвет зависит от масштаба
      ballColors[j] = CHSV((modes[currentMode].Scale * (j + 1)) % 256U, 255U, 255U);
    }
  }

  if (!BALL_TRACK)                                          // режим без следов шариков
  {
    ledsClear(); // esphome: FastLED.clear();
  }
  else                                                      // режим со следами
  {
    dimAll(256U - TRACK_STEP);
  }

  // движение шариков
  for (uint8_t j = 0U; j < BALLS_AMOUNT; j++)
  {
    // движение шариков
    for (uint8_t i = 0U; i < 2U; i++)
    {
      coord[j][i] += vector[j][i];
      if (coord[j][i] < 0)
      {
        coord[j][i] = 0;
        vector[j][i] = -vector[j][i];
      }
    }

    if (coord[j][0U] > (int16_t)((WIDTH - 1) * 10))
    {
      coord[j][0U] = (WIDTH - 1) * 10;
      vector[j][0U] = -vector[j][0U];
    }
    if (coord[j][1U] > (int16_t)((HEIGHT - 1) * 10))
    {
      coord[j][1U] = (HEIGHT - 1) * 10;
      vector[j][1U] = -vector[j][1U];
    }
    drawPixelXYF(coord[j][0U] / 10., coord[j][1U] / 10., ballColors[j]);
  }
}
#endif


#ifdef DEF_PAINTBALL
// ------------- пейнтбол -------------
#define BORDERTHICKNESS (1U) // глубина бордюра для размытия яркой частицы: 0U - без границы (резкие края); 1U - 1 пиксель (среднее размытие) ; 2U - 2 пикселя (глубокое размытие)
const uint8_t paintWidth = WIDTH - BORDERTHICKNESS * 2;
const uint8_t paintHeight = HEIGHT - BORDERTHICKNESS * 2;

static void lightBallsRoutine()
{
  #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      setModeSettings(1U+random8(100U) , 230U+random8(16U));
    }
  #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

  // Apply some blurring to whatever's already on the matrix
  // Note that we never actually clear the matrix, we just constantly
  // blur it repeatedly. Since the blurring is 'lossy', there's
  // an automatic trend toward black -- by design.
  // uint8_t blurAmount = dim8_raw(beatsin8(3, 64, 100));
  // blur2d(leds, WIDTH, HEIGHT, blurAmount);
  blurScreen(dim8_raw(beatsin8(3, 64, 100)));

  // Use two out-of-sync sine waves
  uint16_t i = beatsin16( 79, 0, 255); //91
  uint16_t j = beatsin16( 67, 0, 255); //109
  uint16_t k = beatsin16( 53, 0, 255); //73
  uint16_t m = beatsin16( 97, 0, 255); //123

  // The color of each point shifts over time, each at a different speed.
  uint32_t ms = millis() / (modes[currentMode].Scale / 4 + 1);
  leds[XY( highByte(i * paintWidth) + BORDERTHICKNESS, highByte(j * paintHeight) + BORDERTHICKNESS)] += CHSV( ms / 29, 200U, 255U);
  leds[XY( highByte(j * paintWidth) + BORDERTHICKNESS, highByte(k * paintHeight) + BORDERTHICKNESS)] += CHSV( ms / 41, 200U, 255U);
  leds[XY( highByte(k * paintWidth) + BORDERTHICKNESS, highByte(m * paintHeight) + BORDERTHICKNESS)] += CHSV( ms / 37, 200U, 255U);
  leds[XY( highByte(m * paintWidth) + BORDERTHICKNESS, highByte(i * paintHeight) + BORDERTHICKNESS)] += CHSV( ms / 53, 200U, 255U);
}
#endif


#ifdef DEF_WHITE_COLOR
// ------------- ещё более белый свет (с вертикальным вариантом) -------------
// (c) SottNick
#define BORDERLAND   2 // две дополнительные единицы бегунка Масштаб на границе вертикального и горизонтального варианта эффекта (с каждой стороны границы) будут для света всеми светодиодами в полную силу
static void whiteColorStripeRoutine()
{
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(11U+random8(83U), 1U + random8(255U / WIDTH + 1U) * WIDTH);
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    ledsClear(); // esphome: FastLED.clear();

    uint8_t thisSize = HEIGHT;
    uint8_t halfScale = modes[currentMode].Scale;
    if (halfScale > 50U)
    {
      thisSize = WIDTH;
      halfScale = 101U - halfScale;
    }
    halfScale = constrain(halfScale, 0U, 50U - BORDERLAND);

    uint8_t center =  (uint8_t)round(thisSize / 2.0F) - 1U;
    uint8_t offset = (uint8_t)(!(thisSize & 0x01));
    
    uint8_t fullFill =  center / (50.0 - BORDERLAND) * halfScale;
    uint8_t iPol = (center / (50.0 - BORDERLAND) * halfScale - fullFill) * 255;
    
    for (int16_t i = center; i >= 0; i--)
    {
      CRGB color = CHSV(
                     45U,                                                                              // определяем тон
                     map(modes[currentMode].Speed, 0U, 255U, 0U, 170U),                                // определяем насыщенность
                     i > (center - fullFill - 1)                                                       // определяем яркость
                     ? 255U                                                                            // для центральных горизонтальных полос
                     : iPol * (i > center - fullFill - 2));                                            // для остальных горизонтальных полос яркость равна либо 255, либо 0 в зависимости от масштаба

      if (modes[currentMode].Scale <= 50U)
        for (uint8_t x = 0U; x < WIDTH; x++)
        {
          drawPixelXY(x, i, color);                                                                    // при чётной высоте матрицы максимально яркими отрисуются 2 центральных горизонтальных полосы
          drawPixelXY(x, HEIGHT + offset - i - 2U, color);                                             // при нечётной - одна, но дважды
        }
      else
        for (uint8_t y = 0U; y < HEIGHT; y++)
        {
          drawPixelXY((i + modes[currentMode].Speed - 1U) % WIDTH, y, color);                          // при чётной ширине матрицы максимально яркими отрисуются 2 центральных вертикальных полосы
          drawPixelXY((WIDTH + offset - i + modes[currentMode].Speed - 3U) % WIDTH, y, color);         // при нечётной - одна, но дважды
        }
    }
  }
}
#endif

// --------------------------- эффект кометы ----------------------

// далее идут общие процедуры для эффектов от Stefan Petrick, а непосредственно Комета - в самом низу
//const uint8_t CENTER_X_MINOR =  (WIDTH / 2) -  ((WIDTH - 1) & 0x01);
//const uint8_t CENTER_Y_MINOR = (HEIGHT / 2) - ((HEIGHT - 1) & 0x01);
static int8_t zD;
static int8_t zF;
// The coordinates for 3 16-bit noise spaces.
#define NUM_LAYERS 1 // в кометах используется 1 слой, но для огня 2018 нужно 2

static uint32_t noise32_x[NUM_LAYERSMAX];
static uint32_t noise32_y[NUM_LAYERSMAX];
static uint32_t noise32_z[NUM_LAYERSMAX];
static uint32_t scale32_x[NUM_LAYERSMAX];
static uint32_t scale32_y[NUM_LAYERSMAX];

static uint8_t noisesmooth;
static bool eNs_isSetupped;

static void eNs_setup() {
  noisesmooth = 200;
  for (uint8_t i = 0; i < NUM_LAYERS; i++) {
    noise32_x[i] = random16();
    noise32_y[i] = random16();
    noise32_z[i] = random16();
    scale32_x[i] = 6000;
    scale32_y[i] = 6000;
  }
  eNs_isSetupped = true;
}

static void FillNoise(int8_t layer) {
  for (uint8_t i = 0; i < WIDTH; i++) {
    int32_t ioffset = scale32_x[layer] * (i - CENTER_X_MINOR);
    for (uint8_t j = 0; j < HEIGHT; j++) {
      int32_t joffset = scale32_y[layer] * (j - CENTER_Y_MINOR);
      int8_t data = inoise16(noise32_x[layer] + ioffset, noise32_y[layer] + joffset, noise32_z[layer]) >> 8;
      int8_t olddata = noise3d[layer][i][j];
      int8_t newdata = scale8( olddata, noisesmooth ) + scale8( data, 255 - noisesmooth );
      data = newdata;
      noise3d[layer][i][j] = data;
    }
  }
}

/* эти функции в данных эффектах не используются, но на всякий случай уже адаптированы
static void MoveX(int8_t delta) {
  //CLS2();
  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH - delta; x++) {
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
  //for (uint8_t y = 0; y < HEIGHT; y++) {
  //  for (uint8_t x = 0; x < WIDTH; x++) {
  //    leds[XY(x, y)] = ledsbuff[XY(x, y)];
  //  }
  //}
}

static void MoveY(int8_t delta) {
  //CLS2();
  for (uint8_t x = 0; x < WIDTH; x++) {
    for (uint8_t y = 0; y < HEIGHT - delta; y++) {
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
  //for (uint8_t y = 0; y < HEIGHT; y++) {
  //  for (uint8_t x = 0; x < WIDTH; x++) {
  //    leds[XY(x, y)] = ledsbuff[XY(x, y)];
  //  }
  //}
}
*/

static void MoveFractionalNoiseX(int8_t amplitude = 1, float shift = 0) {
  for (uint8_t y = 0; y < HEIGHT; y++) {
    int16_t amount = ((int16_t)noise3d[0][0][y] - 128) * 2 * amplitude + shift * 256  ;
    int8_t delta = abs(amount) >> 8 ;
    int8_t fraction = abs(amount) & 255;
    for (uint8_t x = 0 ; x < WIDTH; x++) {
      if (amount < 0) {
        zD = x - delta; zF = zD - 1;
      } else {
        zD = x + delta; zF = zD + 1;
      }
      CRGB PixelA = CRGB::Black  ;
      if ((zD >= 0) && (zD < WIDTH)) PixelA = leds[XY(zD, y)];
      CRGB PixelB = CRGB::Black ;
      if ((zF >= 0) && (zF < WIDTH)) PixelB = leds[XY(zF, y)];
      ledsbuff[XY(x, y)] = (PixelA.nscale8(ease8InOutApprox(255 - fraction))) + (PixelB.nscale8(ease8InOutApprox(fraction)));   // lerp8by8(PixelA, PixelB, fraction );
    }
  }
  memcpy(leds, ledsbuff, sizeof(CRGB)* NUM_LEDS);
}

static void MoveFractionalNoiseY(int8_t amplitude = 1, float shift = 0) {
  for (uint8_t x = 0; x < WIDTH; x++) {
    int16_t amount = ((int16_t)noise3d[0][x][0] - 128) * 2 * amplitude + shift * 256 ;
    int8_t delta = abs(amount) >> 8 ;
    int8_t fraction = abs(amount) & 255;
    for (uint8_t y = 0 ; y < HEIGHT; y++) {
      if (amount < 0) {
        zD = y - delta; zF = zD - 1;
      } else {
        zD = y + delta; zF = zD + 1;
      }
      CRGB PixelA = CRGB::Black ;
      if ((zD >= 0) && (zD < HEIGHT)) PixelA = leds[XY(x, zD)];
      CRGB PixelB = CRGB::Black ;
      if ((zF >= 0) && (zF < HEIGHT)) PixelB = leds[XY(x, zF)];
      ledsbuff[XY(x, y)] = (PixelA.nscale8(ease8InOutApprox(255 - fraction))) + (PixelB.nscale8(ease8InOutApprox(fraction)));
    }
  }
  memcpy(leds, ledsbuff, sizeof(CRGB)* NUM_LEDS);
}

#ifdef DEF_COMET_TWO
// NoiseSmearing(by StefanPetrick) Effect mod for GyverLamp by PalPalych
static void MultipleStream() { // 2 comets
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        hue = random8();
        hue2 = hue + 85U;
        setModeSettings(1U+random8(25U), 185U+random8(36U));
      }
      else{
        hue = 0U;   // 0xFF0000
        hue2 = 43U; // 0xFFFF00
      }
    #else
      hue = 0U;     // 0xFF0000
      hue2 = 43U;   // 0xFFFF00
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    trackingObjectState[0] = WIDTH / 8;
    trackingObjectState[1] = HEIGHT / 8;
    trackingObjectShift[0] = 255./(WIDTH-1.-trackingObjectState[0]-trackingObjectState[0]);
    trackingObjectShift[1] = 255./(HEIGHT-1.-trackingObjectState[1]-trackingObjectState[1]);
    trackingObjectState[2] = WIDTH / 4;
    trackingObjectState[3] = HEIGHT / 4;
    trackingObjectShift[2] = 255./(WIDTH-1.-trackingObjectState[2]-trackingObjectState[2]);  // ((WIDTH>10)?9.:5.));
    trackingObjectShift[3] = 255./(HEIGHT-1.-trackingObjectState[3]-trackingObjectState[3]); //- ((HEIGHT>10)?9.:5.));
  }
  
  //dimAll(192); // < -- затухание эффекта для последующего кадрв
  dimAll(255U - modes[currentMode].Scale * 2);

  // gelb im Kreis
  byte xx = trackingObjectState[0] + sin8( millis() / 10) / trackingObjectShift[0]; // / 22;
  byte yy = trackingObjectState[1] + cos8( millis() / 10) / trackingObjectShift[1]; // / 22;
  if (xx < WIDTH && yy < HEIGHT)
    leds[XY( xx, yy)] = CHSV(hue2 , 255, 255);                                      // 0xFFFF00;

  // rot in einer Acht
  xx = trackingObjectState[2] + sin8( millis() / 46) / trackingObjectShift[2];      // / 32;
  yy = trackingObjectState[3] + cos8( millis() / 15) / trackingObjectShift[3];      // / 32;
  if (xx < WIDTH && yy < HEIGHT)
    leds[XY( xx, yy)] = CHSV(hue , 255, 255);                                       // 0xFF0000;

  // Noise
  noise32_x[0] += 3000;
  noise32_y[0] += 3000;
  noise32_z[0] += 3000;
  scale32_x[0] = 8000;
  scale32_y[0] = 8000;
  FillNoise(0);
  MoveFractionalNoiseX(3, 0.33);
  MoveFractionalNoiseY(3);
}
#endif


#ifdef DEF_COMET_THREE
static void MultipleStream2() { // 3 comets
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        hue = random8();
        hue2 = hue + 85U;
        deltaHue = hue2 + 85U;
        setModeSettings(1U+random8(25U), 185U+random8(36U));
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

    loadingFlag = false;
    trackingObjectState[0] = WIDTH / 8;
    trackingObjectState[1] = HEIGHT / 8;
    trackingObjectShift[0] = 255./(WIDTH-1.-trackingObjectState[0]-trackingObjectState[0]);
    trackingObjectShift[1] = 255./(HEIGHT-1.-trackingObjectState[1]-trackingObjectState[1]);
    trackingObjectState[2] = WIDTH / 4;
    trackingObjectState[3] = HEIGHT / 4;
    trackingObjectShift[2] = 255./(WIDTH-1.-trackingObjectState[2]-trackingObjectState[2]);  // ((WIDTH>10)?9.:5.));
    trackingObjectShift[3] = 255./(HEIGHT-1.-trackingObjectState[3]-trackingObjectState[3]); //- ((HEIGHT>10)?9.:5.));
  }
    //dimAll(220); // < -- затухание эффекта для последующего кадрв
  dimAll(255U - modes[currentMode].Scale * 2);

  //byte xx = 2 + sin8( millis() / 10) / 22;
  //byte yy = 2 + cos8( millis() / 9) / 22;
  byte xx = trackingObjectState[0] + sin8( millis() / 10) / trackingObjectShift[0]; // / 22;
  byte yy = trackingObjectState[1] + cos8( millis() / 9) / trackingObjectShift[1];  // / 22;

  if (xx < WIDTH && yy < HEIGHT)
    leds[XY( xx, yy)] += CHSV(deltaHue , 255, 255);//0x0000FF;

  //xx = 4 + sin8( millis() / 10) / 32;
  //yy = 4 + cos8( millis() / 7) / 32;
  xx = trackingObjectState[2] + sin8( millis() / 10) / trackingObjectShift[2];     // / 32;
  yy = trackingObjectState[3] + cos8( millis() / 7) / trackingObjectShift[3];      // / 32;
  if (xx < WIDTH && yy < HEIGHT)
    leds[XY( xx, yy)] += CHSV(hue , 255, 255);                                     // 0xFF0000;
  leds[XY( CENTER_X_MINOR, CENTER_Y_MINOR)] += CHSV(hue2 , 255, 255);              // 0xFFFF00;

  noise32_x[0] += 3000;
  noise32_y[0] += 3000;
  noise32_z[0] += 3000;
  scale32_x[0] = 8000;
  scale32_y[0] = 8000;
  FillNoise(0);
  MoveFractionalNoiseX(2);
  MoveFractionalNoiseY(2, 0.33);
}
#endif


#ifdef DEF_FIREFLY
static void MultipleStream3() { // Fireline
  #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      setModeSettings(1U+random8(26U), 180U+random8(45U));
    }
  #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

  blurScreen(20); // без размытия как-то пиксельно, по-моему...
  //dimAll(160); // < -- затухание эффекта для последующего кадров
  dimAll(255U - modes[currentMode].Scale * 2);
  for (uint8_t i = 1; i < WIDTH; i += 3) {
    leds[XY( i, CENTER_Y_MINOR)] += CHSV(i * 2 , 255, 255);
  }
  // Noise
  noise32_x[0] += 3000;
  noise32_y[0] += 3000;
  noise32_z[0] += 3000;
  scale32_x[0] = 8000;
  scale32_y[0] = 8000;
  FillNoise(0);
  MoveFractionalNoiseY(3);
  MoveFractionalNoiseX(3);
}
#endif


#ifdef DEF_FIREFLY_TOP
static void MultipleStream5() { // Fractorial Fire
  #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      setModeSettings(1U+random8(26U), 180U+random8(45U));
    }
  #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

  blurScreen(20); // без размытия как-то пиксельно, по-моему...
  //dimAll(140); // < -- затухание эффекта для последующего кадрв
  dimAll(255U - modes[currentMode].Scale * 2);
  for (uint8_t i = 1; i < WIDTH; i += 2) {
    leds[XY( i, HEIGHT - 1)] += CHSV(i * 2, 255, 255);
  }
  // Noise
  noise32_x[0] += 3000;
  noise32_y[0] += 3000;
  noise32_z[0] += 3000;
  scale32_x[0] = 8000;
  scale32_y[0] = 8000;
  FillNoise(0);
  //MoveX(1);
  //MoveY(1);
  MoveFractionalNoiseY(2, 1);
  MoveFractionalNoiseX(2);
}
#endif

/*
static void MultipleStream4() { // Comet
  //dimAll(184); // < -- затухание эффекта для последующего кадрв
  dimAll(255U - modes[currentMode].Scale * 2);
  
  CRGB _eNs_color = CHSV(millis(), 255, 255);
  leds[XY( CENTER_X_MINOR, CENTER_Y_MINOR)] += _eNs_color;
  // Noise
  noise32_x[0] += 2000;
  noise32_y[0] += 2000;
  noise32_z[0] += 2000;
  scale32_x[0] = 4000;
  scale32_y[0] = 4000;
  FillNoise(0);
  MoveFractionalNoiseX(6);
  MoveFractionalNoiseY(5, -0.5);
}
*/

#ifdef DEF_SNAKE
static void MultipleStream8() { // Windows ))
  #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      setModeSettings(random8(2U) ? 1U : 2U+random8(99U), 155U+random8(76U));
    }
  #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
  if (loadingFlag){
    loadingFlag = false;
    if (modes[currentMode].Scale > 1U)
      hue = (modes[currentMode].Scale - 2U) * 2.6;
    else
      hue = random8();
  }
  if (modes[currentMode].Scale <= 1U)
    hue++;
  dimAll(96); // < -- затухание эффекта для последующего кадра на 96/255*100=37%
  //dimAll(255U - modes[currentMode].Scale * 2); // так какая-то хрень получается
  for (uint8_t y = 2; y < HEIGHT-1; y += 5) {
    for (uint8_t x = 2; x < WIDTH-1; x += 5) {
      leds[XY(x, y)]  += CHSV(x * y + hue, 255, 255);
      leds[XY(x + 1, y)] += CHSV((x + 4) * y + hue, 255, 255);
      leds[XY(x, y + 1)] += CHSV(x * (y + 4) + hue, 255, 255);
      leds[XY(x + 1, y + 1)] += CHSV((x + 4) * (y + 4) + hue, 255, 255);
    }
  }
  // Noise
  noise32_x[0] += 3000;
  noise32_y[0] += 3000;
  noise32_z[0] += 3000;
  scale32_x[0] = 8000;
  scale32_y[0] = 8000;
  FillNoise(0);
 
  MoveFractionalNoiseX(3);
  MoveFractionalNoiseY(3);
}
#endif


#ifdef DEF_COMET
//  Follow the Rainbow Comet by Palpalych (Effect for GyverLamp 02/03/2020) //

// Кометы обычные
static void RainbowCometRoutine() {
  #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      setModeSettings(10U+random8(91U), 185U+random8(51U));
    }
  #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

  dimAll(254U); // < -- затухание эффекта для последующего кадра
  CRGB _eNs_color = CHSV(millis() / modes[currentMode].Scale * 2, 255, 255);
  leds[XY(CENTER_X_MINOR, CENTER_Y_MINOR)] += _eNs_color;
  leds[XY(CENTER_X_MINOR + 1, CENTER_Y_MINOR)] += _eNs_color;
  leds[XY(CENTER_X_MINOR, CENTER_Y_MINOR + 1)] += _eNs_color;
  leds[XY(CENTER_X_MINOR + 1, CENTER_Y_MINOR + 1)] += _eNs_color;

  // Noise
  noise32_x[0] += 1500;
  noise32_y[0] += 1500;
  noise32_z[0] += 1500;
  scale32_x[0] = 8000;
  scale32_y[0] = 8000;
  FillNoise(0);
  MoveFractionalNoiseX(WIDTH / 2U - 1U);
  MoveFractionalNoiseY(HEIGHT / 2U - 1U);
}
#endif


#ifdef DEF_COMET_COLOR
// Кометы белые и одноцветные
static void ColorCometRoutine() {
  #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      setModeSettings(random8(20U) ? 1U+random8(99U) : 100U, 185U+random8(51U));
    }
  #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
  
  dimAll(254U); // < -- затухание эффекта для последующего кадра
  CRGB _eNs_color = CRGB::White;

  if (modes[currentMode].Scale < 100) _eNs_color = CHSV((modes[currentMode].Scale) * 2.57, 255, 255); // 2.57 вместо 2.55, потому что при 100 будет белый цвет
  leds[XY(CENTER_X_MINOR, CENTER_Y_MINOR)] += _eNs_color;
  leds[XY(CENTER_X_MINOR + 1, CENTER_Y_MINOR)] += _eNs_color;
  leds[XY(CENTER_X_MINOR, CENTER_Y_MINOR + 1)] += _eNs_color;
  leds[XY(CENTER_X_MINOR + 1, CENTER_Y_MINOR + 1)] += _eNs_color;

  // Noise
  noise32_x[0] += 1500;
  noise32_y[0] += 1500;
  noise32_z[0] += 1500;
  scale32_x[0] = 8000;
  scale32_y[0] = 8000;
  FillNoise(0);
  MoveFractionalNoiseX(WIDTH / 2U - 1U);
  MoveFractionalNoiseY(HEIGHT / 2U - 1U);
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
#define bballsGRAVITY           (-9.81)                                      // Downward (negative) acceleration of gravity in m/s^2
#define bballsH0                (1)                                          // Starting height, in meters, of the ball (strip length)
//#define enlargedOBJECT_MAX_COUNT            (WIDTH * 2)                    // максимальное количество мячиков прикручено при адаптации для бегунка Масштаб
//uint8_t enlargedObjectNUM;                                                 // Number of bouncing balls you want (recommend < 7, but 20 is fun in its own way) ...
//                                                                              количество мячиков теперь задаётся бегунком, а не константой
//uint8_t bballsCOLOR[enlargedOBJECT_MAX_COUNT] ;                            // прикручено при адаптации для разноцветных мячиков
//будем использовать uint8_t trackingObjectHue[trackingOBJECT_MAX_COUNT];
//uint8_t bballsX[enlargedOBJECT_MAX_COUNT] ;                                // прикручено при адаптации для распределения мячиков по радиусу лампы
//будем использовать uint8_t trackingObjectState[trackingOBJECT_MAX_COUNT];
//bool trackingObjectIsShift[enlargedOBJECT_MAX_COUNT] ;                     // прикручено при адаптации для того, чтобы мячики не стояли на месте
static float bballsVImpact0 = SQRT_VARIANT( -2 * bballsGRAVITY * bballsH0 ); // Impact velocity of the ball when it hits the ground if "dropped" from the top of the strip
//float bballsVImpact[enlargedOBJECT_MAX_COUNT] ;                            // As time goes on the impact velocity will change, so make an array to store those values
//будем использовать float trackingObjectSpeedY[trackingOBJECT_MAX_COUNT];
//uint16_t   bballsPos[enlargedOBJECT_MAX_COUNT] ;                           // The integer position of the dot on the strip (LED index)
//будем использовать float trackingObjectPosY[trackingOBJECT_MAX_COUNT];
//long  enlargedObjectTime[enlargedOBJECT_MAX_COUNT] ;                       // The clock time of the last ground strike
//float bballsCOR[enlargedOBJECT_MAX_COUNT] ;                                // Coefficient of Restitution (bounce damping)
//будем использовать float trackingObjectShift[trackingOBJECT_MAX_COUNT];

static void BBallsRoutine() {
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(26U+random8(32U), random8(3U) ? ((random8(4U) ? 127U : 0U) + 9U + random8(12U)) : (random8(4U) ? 255U : 127U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    //ledsClear(); // esphome: FastLED.clear();
    enlargedObjectNUM = (modes[currentMode].Scale - 1U) / 99.0 * (enlargedOBJECT_MAX_COUNT - 1U) + 1U;
    if (enlargedObjectNUM > enlargedOBJECT_MAX_COUNT) enlargedObjectNUM = enlargedOBJECT_MAX_COUNT;
    for (uint8_t i = 0 ; i < enlargedObjectNUM ; i++) {                                               // Initialize variables
      trackingObjectHue[i] = random8();
      trackingObjectState[i] = random8(0U, WIDTH);
      enlargedObjectTime[i] = millis();
      trackingObjectPosY[i] = 0U;                                                                     // Balls start on the ground
      trackingObjectSpeedY[i] = bballsVImpact0;                                                       // And "pop" up at vImpact0
      trackingObjectShift[i] = 0.90 - float(i) / pow(enlargedObjectNUM, 2);                           // это, видимо, прыгучесть. для каждого мячика уникальная изначально
      trackingObjectIsShift[i] = false;
      hue2 = (modes[currentMode].Speed > 127U) ? 255U : 0U;                                           // цветные или белые мячики
      hue = (modes[currentMode].Speed == 128U) ? 255U : 254U - modes[currentMode].Speed % 128U * 2U;  // скорость угасания хвостов 0 = моментально
    }
  }
  
  float bballsHi;
  float bballsTCycle;
  if (deltaValue++ & 0x01) deltaHue++;                                                                // постепенное изменение оттенка мячиков (закомментировать строчку, если не нужно)
  dimAll(hue);
  for (uint8_t i = 0 ; i < enlargedObjectNUM ; i++) {

    bballsTCycle =  (millis() - enlargedObjectTime[i]) / 1000. ; // Calculate the time since the last time the ball was on the ground

    // A little kinematics equation calculates positon as a function of time, acceleration (gravity) and intial velocity
    // bballsHi = 0.5 * bballsGRAVITY * pow(bballsTCycle, 2) + trackingObjectSpeedY[i] * bballsTCycle;
    bballsHi = 0.5 * bballsGRAVITY * bballsTCycle * bballsTCycle + trackingObjectSpeedY[i] * bballsTCycle;

    if ( bballsHi < 0 ) {
      enlargedObjectTime[i] = millis();
      bballsHi = 0;                                                                                   // If the ball crossed the threshold of the "ground," put it back on the ground
      trackingObjectSpeedY[i] = trackingObjectShift[i] * trackingObjectSpeedY[i] ;                    // and recalculate its new upward velocity as it's old velocity * COR

      if ( trackingObjectSpeedY[i] < 0.01 )                                                           // If the ball is barely moving, "pop" it back up at vImpact0
      {
        trackingObjectShift[i] = 0.90 - float(random8(9U)) / pow(random8(4U, 9U), 2);                 // сделал, чтобы мячики меняли свою прыгучесть каждый цикл
        trackingObjectIsShift[i] = trackingObjectShift[i] >= 0.89;                                    // если мячик максимальной прыгучести, то разрешаем ему сдвинуться
        trackingObjectSpeedY[i] = bballsVImpact0;
      }
    }

    // trackingObjectPosY[i] = round( bballsHi * (HEIGHT - 1) / bballsH0); были жалобы, что эффект вылетает
    trackingObjectPosY[i] = constrain(round( bballsHi * (HEIGHT - 1) / bballsH0), 0, HEIGHT - 1);     // Map "h" to a "pos" integer index position on the LED strip
    if (trackingObjectIsShift[i] && (trackingObjectPosY[i] == HEIGHT - 1)) {                          // если мячик получил право, то пускай сдвинется на максимальной высоте 1 раз
      trackingObjectIsShift[i] = false;
      if (trackingObjectHue[i] & 0x01) {                                                              // нечётные налево, чётные направо
        if (trackingObjectState[i] == 0U) trackingObjectState[i] = WIDTH - 1U;
        else --trackingObjectState[i];
      } else {
        if (trackingObjectState[i] == WIDTH - 1U) trackingObjectState[i] = 0U;
        else ++trackingObjectState[i];
      }
    }
    leds[XY(trackingObjectState[i], trackingObjectPosY[i])] = CHSV(trackingObjectHue[i] + deltaHue, hue2, 255U);
    //drawPixelXY(trackingObjectState[i], trackingObjectPosY[i], CHSV(trackingObjectHue[i] + deltaHue, hue2, 255U));  //на случай, если останутся жалобы, что эффект вылетает
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
 */
static byte spirotheta1 = 0;
static byte spirotheta2 = 0;
// byte spirohueoffset = 0; // будем использовать переменную сдвига оттенка hue из эффектов Радуга

static const uint8_t spiroradiusx = WIDTH / 4;  // - 1;
static const uint8_t spiroradiusy = HEIGHT / 4; // - 1;

static const uint8_t spirocenterX = WIDTH / 2;
static const uint8_t spirocenterY = HEIGHT / 2;

static const uint8_t spirominx = spirocenterX - spiroradiusx;
static const uint8_t spiromaxx = spirocenterX + spiroradiusx - (WIDTH%2 == 0 ? 1:0);  // + 1;
static const uint8_t spirominy = spirocenterY - spiroradiusy;
static const uint8_t spiromaxy = spirocenterY + spiroradiusy - (HEIGHT%2 == 0 ? 1:0); // + 1;

static uint8_t spirocount = 1;
static uint8_t spirooffset = 256 / spirocount;
static boolean spiroincrement = false;

static boolean spirohandledChange = false;

static uint8_t mapsin8(uint8_t theta, uint8_t lowest = 0, uint8_t highest = 255) {
  uint8_t beatsin = sin8(theta);
  uint8_t rangewidth = highest - lowest;
  uint8_t scaledbeat = scale8(beatsin, rangewidth);
  uint8_t result = lowest + scaledbeat;
  return result;
}

static uint8_t mapcos8(uint8_t theta, uint8_t lowest = 0, uint8_t highest = 255) {
  uint8_t beatcos = cos8(theta);
  uint8_t rangewidth = highest - lowest;
  uint8_t scaledbeat = scale8(beatcos, rangewidth);
  uint8_t result = lowest + scaledbeat;
  return result;
}
  
static void spiroRoutine() {
    if (loadingFlag)
    {
      #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
        if (selectedSettings){
          uint8_t rnd = random8(6U);
          if (rnd > 1U) rnd++;
          if (rnd > 3U) rnd++;
          setModeSettings(rnd*11U+3U, random8(10U) ? 2U+random8(26U) : 255U);
        }
      #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

      loadingFlag = false;
      setCurrentPalette();
    }
      
      blurScreen(20); // @Palpalych советует делать размытие
      dimAll(255U - modes[currentMode].Speed / 10);

      boolean change = false;
      
      for (uint8_t i = 0; i < spirocount; i++) {
        uint8_t x = mapsin8(spirotheta1 + i * spirooffset, spirominx, spiromaxx);
        uint8_t y = mapcos8(spirotheta1 + i * spirooffset, spirominy, spiromaxy);

        uint8_t x2 = mapsin8(spirotheta2 + i * spirooffset, x - spiroradiusx, x + spiroradiusx);
        uint8_t y2 = mapcos8(spirotheta2 + i * spirooffset, y - spiroradiusy, y + spiroradiusy);


       //CRGB color = ColorFromPalette(PartyColors_p, (hue + i * spirooffset), 128U); // вообще-то палитра должна постоянно меняться, но до адаптации этого руки уже не дошли
       //CRGB color = ColorFromPalette(*curPalette, hue + i * spirooffset, 128U); // вот так уже прикручена к бегунку Масштаба. за
       //leds[XY(x2, y2)] += color;
       if (x2<WIDTH && y2<HEIGHT) // добавил проверки. не знаю, почему эффект подвисает без них
         leds[XY(x2, y2)] += (CRGB)ColorFromPalette(*curPalette, hue + i * spirooffset);
        
        if((x2 == spirocenterX && y2 == spirocenterY) ||
           (x2 == spirocenterX && y2 == spirocenterY)) change = true;
      }

      spirotheta1 += 1;
      spirotheta2 += 2;

      EVERY_N_MILLIS(75) {
        if (change && !spirohandledChange) {
          spirohandledChange = true;
          
          if (spirocount >= WIDTH || spirocount == 1) spiroincrement = !spiroincrement;

          if (spiroincrement) {
            if(spirocount >= 4)
              spirocount *= 2;
            else
              spirocount += 1;
          }
          else {
            if(spirocount > 4)
              spirocount /= 2;
            else
              spirocount -= 1;
          }

          spirooffset = 256 / spirocount;
        }
        
        if(!change) spirohandledChange = false;
      }
      hue += 1;
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
        if (selectedSettings){
          setModeSettings(random8(8U)*11U+1U+random8(11U), 50U+random8(121U));
        }
      #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

      loadingFlag = false;
      setCurrentPalette();

      speedfactor = modes[currentMode].Speed / 127.0;
    }
      
  // get some 2 random moving points
  uint16_t param1 = millis() * speedfactor;
  uint8_t x2 = inoise8(param1, 25355, 685 ) / WIDTH;
  uint8_t y2 = inoise8(param1, 355, 11685 ) / HEIGHT;

  uint8_t x3 = inoise8(param1, 55355, 6685 ) / WIDTH;
  uint8_t y3 = inoise8(param1, 25355, 22685 ) / HEIGHT;

  // and one Lissajou function
  uint8_t x1 = beatsin8(23 * speedfactor, 0, WIDTH - 1U);
  uint8_t y1 = beatsin8(28 * speedfactor, 0, HEIGHT - 1U);

  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {

      // calculate distances of the 3 points from actual pixel
      // and add them together with weightening
      uint8_t  dx =  abs(x - x1);
      uint8_t  dy =  abs(y - y1);
      uint8_t dist = 2 * SQRT_VARIANT((dx * dx) + (dy * dy));

      dx =  abs(x - x2);
      dy =  abs(y - y2);
      dist += SQRT_VARIANT((dx * dx) + (dy * dy));

      dx =  abs(x - x3);
      dy =  abs(y - y3);
      dist += SQRT_VARIANT((dx * dx) + (dy * dy));

      // inverse result
      //byte color = modes[currentMode].Speed * 10 / dist;
      //byte color = 1000U / dist; кажется, проблема была именно тут в делении на ноль
      byte color = (dist == 0) ? 255U : 1000U / dist;

      // map color between thresholds
      if (color > 0 && color < 60) {
        if (modes[currentMode].Scale == 100U)
          drawPixelXY(x, y, CHSV(color * 9, 255, 255));// это оригинальный цвет эффекта
        else
          drawPixelXY(x, y, ColorFromPalette(*curPalette, color * 9));
      } else {
        if (modes[currentMode].Scale == 100U)
          drawPixelXY(x, y, CHSV(0, 255, 255)); // в оригинале центральный глаз почему-то красный
        else
          drawPixelXY(x, y, ColorFromPalette(*curPalette, 0U));
      }
      // show the 3 points, too
      drawPixelXY(x1, y1, CRGB(255, 255, 255));
      drawPixelXY(x2, y2, CRGB(255, 255, 255));
      drawPixelXY(x3, y3, CRGB(255, 255, 255));
    }
  }
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

static void Sinusoid3Routine()
{
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        uint8_t tmp = random8(100U);
        setModeSettings(tmp + 1U, 4U+random8(183U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;

    //deltaHue = (modes[currentMode].Scale - 1U) % ... + 1U;
    deltaValue = (modes[currentMode].Speed - 1U) % 9U;                        // количество режимов
    
    emitterX = WIDTH * 0.5;
    emitterY = HEIGHT * 0.5;
    //speedfactor = 0.004 * modes[currentMode].Speed + 0.015;                 // speed of the movement along the Lissajous curves //const float speedfactor = 
    speedfactor = 0.00145 * modes[currentMode].Speed + 0.015;
  }
  float e_s3_size = 3. * modes[currentMode].Scale / 100.0 + 2;                  // amplitude of the curves

  //float time_shift = float(millis()%(uint32_t)(30000*(1.0/((float)modes[currentMode].Speed/255))));
  uint32_t time_shift = millis() & 0xFFFFFF; // overflow protection

  uint16_t _scale = (((modes[currentMode].Scale - 1U) % 9U) * 10U + 80U) << 7U; // = fmap(scale, 1, 255, 0.1, 3);
  float _scale2 = (float)((modes[currentMode].Scale - 1U) % 9U) * 0.2 + 0.4;    // для спиралей на sinf
  uint16_t _scale3 = ((modes[currentMode].Scale - 1U) % 9U) * 1638U + 3276U;    // для спиралей на sin16


  CRGB color;

  float center1x = float(e_s3_size * sin16(speedfactor * 72.0874 * time_shift)) / 0x7FFF - emitterX;
  float center1y = float(e_s3_size * cos16(speedfactor * 98.301  * time_shift)) / 0x7FFF - emitterY;
  float center2x = float(e_s3_size * sin16(speedfactor * 68.8107 * time_shift)) / 0x7FFF - emitterX;
  float center2y = float(e_s3_size * cos16(speedfactor * 65.534  * time_shift)) / 0x7FFF - emitterY;
  float center3x = float(e_s3_size * sin16(speedfactor * 134.3447 * time_shift)) / 0x7FFF - emitterX;
  float center3y = float(e_s3_size * cos16(speedfactor * 170.3884 * time_shift)) / 0x7FFF - emitterY;
  
  switch (deltaValue) {
    case 0:// Sinusoid I
      for (uint8_t y = 0; y < HEIGHT; y++) {
        for (uint8_t x = 0; x < WIDTH; x++) {
          float cx = x + center1x;
          float cy = y + center1y;
          int8_t v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy))) / 0x7FFF);
          color.r = v;
          cx = x + center3x;
          cy = y + center3y;
          v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy))) / 0x7FFF);
          color.b = v;
          drawPixelXY(x, y, color);
        }
      }
      break;
    case 1: // Sinusoid II
      for (uint8_t y = 0; y < HEIGHT; y++) {
        for (uint8_t x = 0; x < WIDTH; x++) {
          float cx = x + center1x;
          float cy = y + center1y;
          //int8_t v = 127 * (0.001 * time_shift * speedfactor + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy))) / 32767.0);
          uint8_t v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy))) / 0x7FFF);
          color.r = v;
          
          cx = x + center2x;
          cy = y + center2y;
          //v = 127 * (float(0.001 * time_shift * speedfactor) + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy))) / 32767.0);
          v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy))) / 0x7FFF);
          //color.g = (uint8_t)v >> 1;
          color.g = (v - (min(v, color.r) >> 1)) >> 1;
          //color.b = (uint8_t)v >> 2;
          color.b = color.g >> 2;
          color.r = max(v, color.r);
          drawPixelXY(x, y, color);
        }
      }
      break;
    case 2:// Sinusoid III
      for (uint8_t y = 0; y < HEIGHT; y++) {
        for (uint8_t x = 0; x < WIDTH; x++) {
          float cx = x + center1x;
          float cy = y + center1y;
          int8_t v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy))) / 0x7FFF);
          color.r = v;

          cx = x + center2x;
          cy = y + center2y;
          v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy))) / 0x7FFF);
          color.b = v;

          cx = x + center3x;
          cy = y + center3y;
          v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy))) / 0x7FFF);
          color.g = v;
          drawPixelXY(x, y, color);
        }
      }
      break;
    case 3: // Sinusoid IV
      for (uint8_t y = 0; y < HEIGHT; y++) {
        for (uint8_t x = 0; x < WIDTH; x++) {
          float cx = x + center1x;
          float cy = y + center1y;
          int8_t v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy) + time_shift * speedfactor * 100)) / 0x7FFF);
          color.r = ~v;

          cx = x + center2x;
          cy = y + center2y;
          v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy) + time_shift * speedfactor * 100)) / 0x7FFF);
          color.g = ~v;

          cx = x + center3x;
          cy = y + center3y;
          v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy) + time_shift * speedfactor * 100)) / 0x7FFF);
          color.b = ~v;
          drawPixelXY(x, y, color);
        }
      }

      break;
/*    case 4: //changed by stepko // anaglyph sinusoid
      for (uint8_t y = 0; y < HEIGHT; y++) {
        for (uint8_t x = 0; x < WIDTH; x++) {
          float cx = x + center1x;
          float cy = y + center1y;
          int8_t v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy))) / 0x7FFF);// + time_shift * speedfactor)) / 0x7FFF);
          color.r = ~v;

          v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy) + time_shift * speedfactor * 10)) / 0x7FFF); 
          color.b = ~v;

          //same v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy) + time_shift * speedfactor * 10)) / 0x7FFF);
          color.g = ~v;
          drawPixelXY(x, y, color);
        }
      }
      break;
*/
    case 4: // changed by stepko // colored sinusoid
      for (uint8_t y = 0; y < HEIGHT; y++) {
        for (uint8_t x = 0; x < WIDTH; x++) {
          float cx = x + center1x;
          float cy = y + center1y;
          int8_t v = 127 * (1 + float(sin16(_scale * (beatsin16(2,1000,1750)/2550.) * SQRT_VARIANT(cx * cx + cy * cy))) / 0x7FFF);// + time_shift * speedfactor * 5 // mass colors plus by SottNick
          color.r = v;

          //v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy) + time_shift * speedfactor * 7)) / 0x7FFF);
          //v = 127 * (1 + sinf (_scale2 * SQRT_VARIANT(((cx * cx) + (cy * cy)))  + 0.001 * time_shift * speedfactor));
          v = 127 * (1 + float(sin16(_scale * (beatsin16(1,570,1050)/2250.) * SQRT_VARIANT(((cx * cx) + (cy * cy)))  + 13 * time_shift * speedfactor)) / 0x7FFF); // вместо beatsin сперва ставил просто * 0.41
          color.b = v;

          //v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy) + time_shift * speedfactor * 19)) / 0x7FFF);
          //v = 127 * (1 + sinf (_scale2 * SQRT_VARIANT(((cx * cx) + (cy * cy)))  + 0.0025 * time_shift * speedfactor));
          v = 127 * (1 + float(cos16(_scale * (beatsin16(3,1900,2550)/2550.) * SQRT_VARIANT(((cx * cx) + (cy * cy)))  + 41 * time_shift * speedfactor)) / 0x7FFF); // вместо beatsin сперва ставил просто * 0.53
          color.g = v;
          drawPixelXY(x, y, color);
        }
      }
      break;
    case 5: // changed by stepko // sinusoid in net
      for (uint8_t y = 0; y < HEIGHT; y++) {
        for (uint8_t x = 0; x < WIDTH; x++) {
          float cx = x + center1x;
          float cy = y + center1y;
          int8_t v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy) + time_shift * speedfactor * 5)) / 0x7FFF);
          color.g = ~v;

          //v = 127 * (1 + float(sin16(_scale * x) + 0.01 * time_shift * speedfactor) / 0x7FFF);
          v = 127 * (1 + float(sin16(_scale * (x + 0.005 * time_shift * speedfactor))) / 0x7FFF); // proper by SottNick
          
          color.b = ~v;

          //v = 127 * (1 + float(sin16(_scale * y * 127 + float(0.011 * time_shift * speedfactor))) / 0x7FFF);
          v = 127 * (1 + float(sin16(_scale * (y + 0.0055 * time_shift * speedfactor))) / 0x7FFF); // proper by SottNick
          color.r = ~v;
          drawPixelXY(x, y, color);
        }
      }
      break;
    case 6: // changed by stepko // spiral
      for (uint8_t y = 0; y < HEIGHT; y++) {
        for (uint8_t x = 0; x < WIDTH; x++) {
          float cx = x + center1x;
          float cy = y + center1y;
          //uint8_t v = 127 * (1 + float(sin16(_scale * (2 * atan2(cy, cx) + hypot(cy, cx)) + time_shift * speedfactor * 5)) / 0x7FFF);
          uint8_t v = 127 * (1 + sinf (3* atan2(cy, cx)  + _scale2 *  hypot(cy, cx))); // proper by SottNick
          //uint8_t v = 127 * (1 + float(sin16(atan2(cy, cx) * 31255  + _scale3 *  hypot(cy, cx))) / 0x7FFF); // proper by SottNick
          //вырезаем центр спирали - proper by SottNick
          float d = SQRT_VARIANT(cx * cx + cy * cy) / 10.; // 10 - это радиус вырезаемого центра в каких-то условных величинах. 10 = 1 пиксель, 20 = 2 пикселя. как-то так
          if (d < 0.06) d = 0.06;
          if (d < 1) // просто для ускорения расчётов
            v = constrain(v - int16_t(1/d/d), 0, 255);
          //вырезали
          color.r = v;

          cx = x + center2x;
          cy = y + center2y;
          //v = 127 * (1 + float(sin16(_scale * (2 * atan2(cy, cx) + hypot(cy, cx)) + time_shift * speedfactor * 5)) / 0x7FFF);
          v = 127 * (1 + sinf (3* atan2(cy, cx)  + _scale2 *  hypot(cy, cx))); // proper by SottNick
          //v = 127 * (1 + float(sin16(atan2(cy, cx) * 31255  + _scale3 *  hypot(cy, cx))) / 0x7FFF); // proper by SottNick
          //вырезаем центр спирали
          d = SQRT_VARIANT(cx * cx + cy * cy) / 10.; // 10 - это радиус вырезаемого центра в каких-то условных величинах. 10 = 1 пиксель, 20 = 2 пикселя. как-то так
          if (d < 0.06) d = 0.06;
          if (d < 1) // просто для ускорения расчётов
            v = constrain(v - int16_t(1/d/d), 0, 255);
          //вырезали
          color.b = v;

          cx = x + center3x;
          cy = y + center3y;
          //v = 127 * (1 + float(sin16(_scale * (2 * atan2(cy, cx) + hypot(cy, cx)) + time_shift * speedfactor * 5)) / 0x7FFF);
          //v = 127 * (1 + sinf (3* atan2(cy, cx)  + _scale2 *  hypot(cy, cx))); // proper by SottNick
          v = 127 * (1 + float(sin16(atan2(cy, cx) * 31255  + _scale3 *  hypot(cy, cx))) / 0x7FFF); // proper by SottNick
          //вырезаем центр спирали
          d = SQRT_VARIANT(cx * cx + cy * cy) / 10.; // 10 - это радиус вырезаемого центра в каких-то условных величинах. 10 = 1 пиксель, 20 = 2 пикселя. как-то так
          if (d < 0.06) d = 0.06;
          if (d < 1) // просто для ускорения расчётов
            v = constrain(v - int16_t(1/d/d), 0, 255);
          //вырезали
          color.g = v;
          drawPixelXY(x, y, color);
        }
      }
      break;
    case 7: // variant by SottNick
      for (uint8_t y = 0; y < HEIGHT; y++) {
        for (uint8_t x = 0; x < WIDTH; x++) {
          float cx = x + center1x;
          float cy = y + center1y;
          //uint8_t v = 127 * (1 + float(sin16(_scale * (2 * atan2(cy, cx) + hypot(cy, cx)) + time_shift * speedfactor * 5)) / 0x7FFF);
          //uint8_t v = 127 * (1 + float(sin16(3* atan2(cy, cx) + _scale *  hypot(cy, cx) + time_shift * speedfactor * 5)) / 0x7FFF);
          //uint8_t v = 127 * (1 + sinf (3* atan2(cy, cx)  + _scale2 *  hypot(cy, cx))); // proper by SottNick
          uint8_t v = 127 * (1 + float(sin16(atan2(cy, cx) * 31255  + _scale3 *  hypot(cy, cx))) / 0x7FFF); // proper by SottNick
          //вырезаем центр спирали
          float d = SQRT_VARIANT(cx * cx + cy * cy) / 10.; // 10 - это радиус вырезаемого центра в каких-то условных величинах. 10 = 1 пиксель, 20 = 2 пикселя. как-то так
          if (d < 0.06) d = 0.06;
          if (d < 1) // просто для ускорения расчётов
            v = constrain(v - int16_t(1/d/d), 0, 255);
          //вырезали
          color.g = v;

          cx = x + center3x;
          cy = y + center3y;
          //v = 127 * (1 + sinf (3* atan2(cy, cx)  + _scale2 *  hypot(cy, cx))); // proper by SottNick
          v = 127 * (1 + float(sin16(atan2(cy, cx) * 31255  + _scale3 *  hypot(cy, cx))) / 0x7FFF); // proper by SottNick
          //вырезаем центр спирали
          d = SQRT_VARIANT(cx * cx + cy * cy) / 10.; // 10 - это радиус вырезаемого центра в каких-то условных величинах. 10 = 1 пиксель, 20 = 2 пикселя. как-то так
          if (d < 0.06) d = 0.06;
          if (d < 1) // просто для ускорения расчётов
            v = constrain(v - int16_t(1/d/d), 0, 255);
          //вырезали
          color.r = v;

          drawPixelXY(x, y, color);
          //nblend(leds[XY(x, y)], color, 150);
        }
      }
      break;
    case 8: // variant by SottNick
      for (uint8_t y = 0; y < HEIGHT; y++) {
        for (uint8_t x = 0; x < WIDTH; x++) {
          float cx = x + center1x;
          float cy = y + center1y;
          //uint8_t v = 127 * (1 + float(sin16(_scale * (2 * atan2(cy, cx) + hypot(cy, cx)) + time_shift * speedfactor * 5)) / 0x7FFF);
          //uint8_t v = 127 * (1 + sinf (3* atan2(cy, cx)  + _scale2 *  hypot(cy, cx))); // proper by SottNick
          //uint8_t v = 127 * (1 + float(sin16(atan2(cy, cx) * 31255  + _scale3 *  hypot(cy, cx))) / 0x7FFF); // proper by SottNick
          uint8_t v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy))) / 0x7FFF);
          color.g = v;

          cx = x + center2x;
          cy = y + center2y;
          //v = 127 * (1 + float(sin16(_scale * (2 * atan2(cy, cx) + hypot(cy, cx)) + time_shift * speedfactor * 5)) / 0x7FFF);
          //v = 127 * (1 + sinf (3* atan2(cy, cx)  + _scale2 *  hypot(cy, cx))); // proper by SottNick
          v = 127 * (1 + float(sin16(atan2(cy, cx) * 31255  + _scale3 *  hypot(cy, cx))) / 0x7FFF); // proper by SottNick
          //вырезаем центр спирали
          float d = SQRT_VARIANT(cx * cx + cy * cy) / 16.; // 16 - это радиус вырезаемого центра в каких-то условных величинах. 10 = 1 пиксель, 20 = 2 пикселя. как-то так
          if (d < 0.06) d = 0.06;
          if (d < 1) // просто для ускорения расчётов
            v = constrain(v - int16_t(1/d/d), 0, 255);
          //вырезали
          color.g = max(v, color.g);
          color.b = v;// >> 1;
          //color.r = v >> 1;

          drawPixelXY(x, y, color);
          //nblend(leds[XY(x, y)], color, 150);
        }
      }
      break;
  }
}
#endif


#ifdef DEF_WATERFALL_4IN1
// ============= водо/огне/лава/радуга/хренопад ===============
// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.

static void fire2012WithPalette4in1() {
  #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      uint8_t tmp = random(3U);
      if (tmp == 0U)
        tmp = 16U+random8(16U);
      else if (tmp == 1U)
        tmp = 48U;
      else
        tmp = 80U+random8(4U);
      setModeSettings(tmp, 185U+random8(40U)); // 16-31, 48, 80-83 - остальное отстой
    }
  #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

  uint8_t rCOOLINGNEW = constrain((uint16_t)(modes[currentMode].Scale % 16) * 32 / HEIGHT + 16, 1, 255) ;
  // Array of temperature readings at each simulation cell
  // static byte heat[WIDTH][HEIGHT]; будет noise3d[0][WIDTH][HEIGHT]

  for (uint8_t x = 0; x < WIDTH; x++) {
    // Step 1.  Cool down every cell a little
    for (uint8_t i = 0; i < HEIGHT; i++) {
      //noise3d[0][x][i] = qsub8(noise3d[0][x][i], random8(0, ((rCOOLINGNEW * 10) / HEIGHT) + 2));
      noise3d[0][x][i] = qsub8(noise3d[0][x][i], random8(0, rCOOLINGNEW));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (uint8_t k = HEIGHT - 1; k >= 2; k--) {
      noise3d[0][x][k] = (noise3d[0][x][k - 1] + noise3d[0][x][k - 2] + noise3d[0][x][k - 2]) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if (random8() < SPARKINGNEW) {
      uint8_t y = random8(2);
      noise3d[0][x][y] = qadd8(noise3d[0][x][y], random8(160, 255));
    }

    // Step 4.  Map from heat cells to LED colors
    for (uint8_t j = 0; j < HEIGHT; j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      byte colorindex = scale8(noise3d[0][x][j], 240);
      if  (modes[currentMode].Scale < 16) {            // Lavafall
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(LavaColors_p, colorindex);
      } else if (modes[currentMode].Scale < 32) {      // Firefall
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(HeatColors_p, colorindex);
      } else if (modes[currentMode].Scale < 48) {      // Waterfall
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(WaterfallColors4in1_p, colorindex);
      } else if (modes[currentMode].Scale < 64) {      // Skyfall
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(CloudColors_p, colorindex);
      } else if (modes[currentMode].Scale < 80) {      // Forestfall
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(ForestColors_p, colorindex);
      } else if (modes[currentMode].Scale < 96) {      // Rainbowfall
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(RainbowColors_p, colorindex);
      } else {                      // Aurora
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(RainbowStripeColors_p, colorindex);
      }
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
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(1U+random8(100U), 35U+random8(100U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    setCurrentPalette();
  } 
  
//  EVERY_N_MILLIS(33) { маловата задержочка
    hue++; // используем переменную сдвига оттенка из функций радуги, чтобы не занимать память
//  }
  blurScreen(20); // @Palpalych посоветовал делать размытие
  dimAll(255U - (modes[currentMode].Scale - 1U) % 11U * 3U);

  for (uint8_t x = 0; x < WIDTH; x++)
  {
    //uint8_t y = beatsin8(x + 1, 0, HEIGHT-1); // это я попытался распотрошить данную функцию до исходного кода и вставить в неё регулятор скорости
    // вместо 28 в оригинале было 280, умножения на .Speed не было, а вместо >>17 было (<<8)>>24. короче, оригинальная скорость достигается при бегунке .Speed=20
    uint8_t beat = (GET_MILLIS() * (accum88(x + 1)) * 28 * modes[currentMode].Speed) >> 17;
    uint8_t y = scale8(sin8(beat), HEIGHT-1);
    //и получилось!!!
    
    drawPixelXY(x, y, ColorFromPalette(*curPalette, x * 7 + hue));
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

    bool operator==(Vector2& v) {
        return x == v.x && y == v.y;
    }

    bool operator!=(Vector2& v) {
        return !(x == y);
    }

    Vector2 operator+(Vector2& v) {
        return Vector2(x + v.x, y + v.y);
    }
    Vector2 operator-(Vector2& v) {
        return Vector2(x - v.x, y - v.y);
    }

    Vector2& operator+=(Vector2& v) {
        x += v.x;
        y += v.y;
        return *this;
    }
    Vector2& operator-=(Vector2& v) {
        x -= v.x;
        y -= v.y;
        return *this;
    }

    Vector2 operator+(double s) {
        return Vector2(x + s, y + s);
    }
    Vector2 operator-(double s) {
        return Vector2(x - s, y - s);
    }
    Vector2 operator*(double s) {
        return Vector2(x * s, y * s);
    }
    Vector2 operator/(double s) {
        return Vector2(x / s, y / s);
    }
    
    Vector2& operator+=(double s) {
        x += s;
        y += s;
        return *this;
    }
    Vector2& operator-=(double s) {
        x -= s;
        y -= s;
        return *this;
    }
    Vector2& operator*=(double s) {
        x *= s;
        y *= s;
        return *this;
    }
    Vector2& operator/=(double s) {
        x /= s;
        y /= s;
        return *this;
    }

    void set(T x, T y) {
        this->x = x;
        this->y = y;
    }

    void rotate(double deg) {
        double theta = deg / 180.0 * M_PI;
        double c = cos(theta);
        double s = sin(theta);
        double tx = x * c - y * s;
        double ty = x * s + y * c;
        x = tx;
        y = ty;
    }

    Vector2& normalize() {
        if (length() == 0) return *this;
        *this *= (1.0 / length());
        return *this;
    }

    float dist(Vector2 v) const {
        Vector2 d(v.x - x, v.y - y);
        return d.length();
    }
    float length() const {
        //return SQRT_VARIANT(x * x + y * y); некорректно работает через sqrt3, нужно sqrt
        return sqrt(x * x + y * y);
    }

    float mag() const {
        return length();
    }

    float magSq() {
        return (x * x + y * y);
    }

    void truncate(double length) {
        double angle = atan2f(y, x);
        x = length * cos(angle);
        y = length * sin(angle);
    }

    Vector2 ortho() const {
        return Vector2(y, -x);
    }

    static float dot(Vector2 v1, Vector2 v2) {
        return v1.x * v2.x + v1.y * v2.y;
    }
    static float cross(Vector2 v1, Vector2 v2) {
        return (v1.x * v2.y) - (v1.y * v2.x);
    }

    void limit(float max) {
        if (magSq() > max*max) {
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
    byte colorIndex = 0;
    float mass;

    boolean enabled = true;

    Boid() {}

    Boid(float x, float y) {
      acceleration = PVector(0, 0);
      velocity = PVector(randomf(), randomf());
      location = PVector(x, y);
      maxspeed = 1.5;
      maxforce = 0.05;
    }

    static float randomf() {
      return mapfloat(random(0, 255), 0, 255, -.5, .5);
    }

    static float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
      return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

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
      //Force that drives boid away from obstacle.

      PVector futPos = location + velocity; //Calculate future position for more effective behavior.
      PVector dist = obstacle - futPos;
      float d = dist.mag();

      if (d <= radius) {
        PVector repelVec = location - obstacle;
        repelVec.normalize();
        if (d != 0) { //Don't divide by zero.
          // float scale = 1.0 / d; //The closer to the obstacle, the stronger the force.
          repelVec.normalize();
          repelVec *= (maxforce * 7);
          if (repelVec.mag() < 0) { //Don't let the boids turn around to avoid the obstacle.
            repelVec.y = 0;
          }
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
      sep *= 1.5;
      ali *= 1.0;
      coh *= 1.0;
      // Add the force vectors to acceleration
      applyForce(sep);
      applyForce(ali);
      applyForce(coh);
    }

    // Separation
    // Method checks for nearby boids and steers away
    PVector separate(Boid boids [], uint8_t boidCount) {
      PVector steer = PVector(0, 0);
      int count = 0;
      // For every boid in the system, check if it's too close
      for (int i = 0; i < boidCount; i++) {
        Boid other = boids[i];
        if (!other.enabled)
          continue;
        float d = location.dist(other.location);
        // If the distance is greater than 0 and less than an arbitrary amount (0 when you are yourself)
        if ((d > 0) && (d < desiredseparation)) {
          // Calculate vector pointing away from neighbor
          PVector diff = location - other.location;
          diff.normalize();
          diff /= d;        // Weight by distance
          steer += diff;
          count++;            // Keep track of how many
        }
      }
      // Average -- divide by how many
      if (count > 0) {
        steer /= (float) count;
      }

      // As long as the vector is greater than 0
      if (steer.mag() > 0) {
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
      int count = 0;
      for (int i = 0; i < boidCount; i++) {
        Boid other = boids[i];
        if (!other.enabled)
          continue;
        float d = location.dist(other.location);
        if ((d > 0) && (d < neighbordist)) {
          sum += other.velocity;
          count++;
        }
      }
      if (count > 0) {
        sum /= (float) count;
        sum.normalize();
        sum *= maxspeed;
        PVector steer = sum - velocity;
        steer.limit(maxforce);
        return steer;
      }
      else {
        return PVector(0, 0);
      }
    }

    // Cohesion
    // For the average location (i.e. center) of all nearby boids, calculate steering vector towards that location
    PVector cohesion(Boid boids [], uint8_t boidCount) {
      PVector sum = PVector(0, 0);   // Start with empty vector to accumulate all locations
      int count = 0;
      for (int i = 0; i < boidCount; i++) {
        Boid other = boids[i];
        if (!other.enabled)
          continue;
        float d = location.dist(other.location);
        if ((d > 0) && (d < neighbordist)) {
          sum += other.location; // Add location
          count++;
        }
      }
      if (count > 0) {
        sum /= count;
        return seek(sum);  // Steer towards the location
      }
      else {
        return PVector(0, 0);
      }
    }

    // A method that calculates and applies a steering force towards a target
    // STEER = DESIRED MINUS VELOCITY
    PVector seek(PVector target) {
      PVector desired = target - location;  // A vector pointing from the location to the target
      // Normalize desired and scale to maximum speed
      desired.normalize();
      desired *= maxspeed;
      // Steering = Desired minus Velocity
      PVector steer = desired - velocity;
      steer.limit(maxforce);  // Limit to maximum steering force
      return steer;
    }

    // A method that calculates a steering force towards a target
    // STEER = DESIRED MINUS VELOCITY
    void arrive(PVector target) {
      PVector desired = target - location;  // A vector pointing from the location to the target
      float d = desired.mag();
      // Normalize desired and scale with arbitrary damping within 100 pixels
      desired.normalize();
      if (d < 4) {
        float m = map(d, 0, 100, 0, maxspeed);
        desired *= m;
      }
      else {
        desired *= maxspeed;
      }

      // Steering = Desired minus Velocity
      PVector steer = desired - velocity;
      steer.limit(maxforce);  // Limit to maximum steering force
      applyForce(steer);
      //Serial.println(d);
    }

    void wrapAroundBorders() {
      if (location.x < 0) location.x = WIDTH - 1;
      if (location.y < 0) location.y = HEIGHT - 1;
      if (location.x >= WIDTH) location.x = 0;
      if (location.y >= HEIGHT) location.y = 0;
    }

    void avoidBorders() {
      PVector desired = velocity;

      if (location.x < 8) desired = PVector(maxspeed, velocity.y);
      if (location.x >= WIDTH - 8) desired = PVector(-maxspeed, velocity.y);
      if (location.y < 8) desired = PVector(velocity.x, maxspeed);
      if (location.y >= HEIGHT - 8) desired = PVector(velocity.x, -maxspeed);

      if (desired != velocity) {
        PVector steer = desired - velocity;
        steer.limit(maxforce);
        applyForce(steer);
      }

      if (location.x < 0) location.x = 0;
      if (location.y < 0) location.y = 0;
      if (location.x >= WIDTH) location.x = WIDTH - 1;
      if (location.y >= HEIGHT) location.y = HEIGHT - 1;
    }

    bool bounceOffBorders(float bounce) {
      bool bounced = false;

      if (location.x >= WIDTH) {
        location.x = WIDTH - 1;
        velocity.x *= -bounce;
        bounced = true;
      }
      else if (location.x < 0) {
        location.x = 0;
        velocity.x *= -bounce;
        bounced = true;
      }

      if (location.y >= HEIGHT) {
        location.y = HEIGHT - 1;
        velocity.y *= -bounce;
        bounced = true;
      }
      else if (location.y < 0) {
        location.y = 0;
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

static const uint8_t AVAILABLE_BOID_COUNT = 20U;
static Boid boids[AVAILABLE_BOID_COUNT];

#if defined(DEF_FLOCK) || defined(DEF_FLOCK_N_PR)
// ============= ЭФФЕКТ СТАЯ ===============
// https://github.com/pixelmatix/aurora/blob/master/PatternFlock.h
// Адаптация от (c) SottNick и @kDn

// Flocking
// Daniel Shiffman <http://www.shiffman.net>
// The Nature of Code, Spring 2009

static const uint8_t boidCount = 10;
static Boid predator;
static PVector wind;
static bool predatorPresent = true;

static void flockRoutine(bool predatorIs) {
    if (loadingFlag)
    {
      #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
        if (selectedSettings){
          //setModeSettings(random8(8U)*11U+1U+random8(11U), 1U+random8(255U));
          uint8_t tmp = random8(5U);// 0, 1, 5, 6, 7 - остальные 4 палитры с чёрным цветом - стая будет исчезать периодически (2, 3, 4, 8)
          if (tmp > 1U) tmp += 3U;
          setModeSettings(tmp*11U+2U+random8(10U), 1U+random8(255U));
        }
      #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

      loadingFlag = false;
      setCurrentPalette();

      for (int i = 0; i < boidCount; i++) {
        boids[i] = Boid(0, 0);//WIDTH - 1U, HEIGHT - 1U);
        boids[i].maxspeed = 0.380 * modes[currentMode].Speed /127.0+0.380/2;
        boids[i].maxforce = 0.015 * modes[currentMode].Speed /127.0+0.015/2;
      }
      predatorPresent = predatorIs && random8(2U);
      //if (predatorPresent) { нужно присвоить ему значения при первом запуске, иначе он с нулями будет жить
        predator = Boid(0, 0);//WIDTH + WIDTH - 1, HEIGHT + HEIGHT - 1);
        predator.maxspeed = 0.385 * modes[currentMode].Speed /127.0+0.385/2;
        predator.maxforce = 0.020 * modes[currentMode].Speed /127.0+0.020/2;
        predator.neighbordist = 8.0; // было 16.0 и хищник гонял по одной линии всегда
        predator.desiredseparation = 0.0;
      //}
    }
    
      blurScreen(15); // @Palpalych советует делать размытие
      //myLamp.dimAll(254U - (31-(myLamp.effects.getScale()%32))*8);
      dimAll(255U - (modes[currentMode].Scale - 1U) % 11U * 3);

      bool applyWind = random(0, 255) > 240;
      if (applyWind) {
        wind.x = Boid::randomf() * .015 * modes[currentMode].Speed /127.0 + .015/2;
        wind.y = Boid::randomf() * .015 * modes[currentMode].Speed /127.0 + .015/2;
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
        // PVector velocity = boid->velocity;
        // backgroundLayer.drawLine(location.x, location.y, location.x - velocity.x, location.y - velocity.y, color);
        // effects.leds[XY(location.x, location.y)] += color;
        //drawPixelXY(location.x, location.y, color);
        drawPixelXYF(location.x, location.y, color);

        if (applyWind) {
          boid->applyForce(wind);
          applyWind = false;
        }
      }

      if (predatorPresent) {
        predator.run(boids, boidCount);
        predator.wrapAroundBorders();
        color = ColorFromPalette(*curPalette, hue + 128);
        PVector location = predator.location;
        // PVector velocity = predator.velocity;
        // backgroundLayer.drawLine(location.x, location.y, location.x - velocity.x, location.y - velocity.y, color);
        // effects.leds[XY(location.x, location.y)] += color;

        //drawPixelXY(location.x, location.y, color);
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

//uint16_t ff_x; вынесены в общий пул
//uint16_t ff_y;
//uint16_t ff_z;

static const uint8_t ff_speed = 1; // чем выше этот параметр, тем короче переходы (градиенты) между цветами. 1 - это самое красивое
static const uint8_t ff_scale = 26; // чем больше этот параметр, тем больше "языков пламени" или как-то так. 26 - это норм
    
static void whirlRoutine(bool oneColor) {
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        if (oneColor)
          setModeSettings(random8(30U) ? 1U+random8(99U) : 100U, 221U+random8(32U));
        else{
          uint8_t tmp = random8(5U);
          if (tmp > 1U) tmp += 3U;
          setModeSettings(tmp*11U+3U, 221U+random8(32U));
        }
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    setCurrentPalette();

    ff_x = random16();
    ff_y = random16();
    ff_z = random16();

    for (uint8_t i = 0; i < AVAILABLE_BOID_COUNT; i++) {
      boids[i] = Boid(random8(WIDTH), 0);
    }
  } 
  dimAll(240);

  for (uint8_t i = 0; i < AVAILABLE_BOID_COUNT; i++) {
    Boid * boid = &boids[i];
    
    int ioffset = ff_scale * boid->location.x;
    int joffset = ff_scale * boid->location.y;

    byte angle = inoise8(ff_x + ioffset, ff_y + joffset, ff_z);

    boid->velocity.x = (float) sin8(angle) * 0.0078125 - 1.0;
    boid->velocity.y = -((float)cos8(angle) * 0.0078125 - 1.0);
    boid->update();
  
    if (oneColor)
      //drawPixelXY(boid->location.x, boid->location.y, CHSV(modes[currentMode].Scale * 2.55, (modes[currentMode].Scale == 100) ? 0U : 255U, 255U)); // цвет белый для .Scale=100
      drawPixelXYF(boid->location.x, boid->location.y, CHSV(modes[currentMode].Scale * 2.55, (modes[currentMode].Scale == 100) ? 0U : 255U, 255U)); // цвет белый для .Scale=100
    else
      //drawPixelXY(boid->location.x, boid->location.y, ColorFromPalette(*curPalette, angle + hue)); // + hue постепенно сдвигает палитру по кругу
      drawPixelXYF(boid->location.x, boid->location.y, ColorFromPalette(*curPalette, angle + hue)); // + hue постепенно сдвигает палитру по кругу

    if (boid->location.x < 0 || boid->location.x >= WIDTH || boid->location.y < 0 || boid->location.y >= HEIGHT) {
      boid->location.x = random(WIDTH);
      boid->location.y = 0;
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

static byte waveThetaUpdate = 0;
static byte waveThetaUpdateFrequency = 0;
static byte waveTheta = 0;

static byte hueUpdate = 0;
static byte hueUpdateFrequency = 0;
// byte hue = 0; будем использовать сдвиг от эффектов Радуга

static byte waveRotation = 0;
static uint8_t waveScale = 256 / WIDTH;
static uint8_t waveCount = 1;

static void WaveRoutine() {
    if (loadingFlag)
    {
      #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
        if (selectedSettings){
          uint8_t tmp = random8(5U);// 0, 1, 5, 6, 7 - остальные 4 палитры с чёрным цветом - будет мерцать (2, 3, 4, 8)
          if (tmp > 1U) tmp += 3U;
          setModeSettings(tmp*11U+1U+random8(4U), 220U+random8(17U)*2U);
        }
      #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

      loadingFlag = false;
      setCurrentPalette(); //а вот тут явно накосячено. палитры наложены на угол поворота несинхронно, но исправлять особого смысла нет
     
      //waveRotation = random(0, 4);// теперь вместо этого регулятор Масштаб
      waveRotation = (modes[currentMode].Scale % 11U) % 4U;//(modes[currentMode].Scale - 1) / 25U;
      //waveCount = random(1, 3);// теперь вместо этого чётное/нечётное у регулятора Скорость
      waveCount = modes[currentMode].Speed & 0x01;//% 2;
      //waveThetaUpdateFrequency = random(1, 2);
      //hueUpdateFrequency = random(1, 6);      
    }
 
        dimAll(254);
  
        int n = 0;

        switch (waveRotation) {
            case 0:
                for (uint8_t x = 0; x < WIDTH; x++) {
                    n = quadwave8(x * 2 + waveTheta) / waveScale;
                    drawPixelXY(x, n, ColorFromPalette(*curPalette, hue + x));
                    if (waveCount != 1)
                        drawPixelXY(x, HEIGHT - 1 - n, ColorFromPalette(*curPalette, hue + x));
                }
                break;

            case 1:
                for (uint8_t y = 0; y < HEIGHT; y++) {
                    n = quadwave8(y * 2 + waveTheta) / waveScale;
                    drawPixelXY(n, y, ColorFromPalette(*curPalette, hue + y));
                    if (waveCount != 1)
                        drawPixelXY(WIDTH - 1 - n, y, ColorFromPalette(*curPalette, hue + y));
                }
                break;

            case 2:
                for (uint8_t x = 0; x < WIDTH; x++) {
                    n = quadwave8(x * 2 - waveTheta) / waveScale;
                    drawPixelXY(x, n, ColorFromPalette(*curPalette, hue + x));
                    if (waveCount != 1)
                        drawPixelXY(x, HEIGHT - 1 - n, ColorFromPalette(*curPalette, hue + x));
                }
                break;

            case 3:
                for (uint8_t y = 0; y < HEIGHT; y++) {
                    n = quadwave8(y * 2 - waveTheta) / waveScale;
                    drawPixelXY(n, y, ColorFromPalette(*curPalette, hue + y));
                    if (waveCount != 1)
                        drawPixelXY(WIDTH - 1 - n, y, ColorFromPalette(*curPalette, hue + y));
                }
                break;
        }


        if (waveThetaUpdate >= waveThetaUpdateFrequency) {
            waveThetaUpdate = 0;
            waveTheta++;
        }
        else {
            waveThetaUpdate++;
        }

        if (hueUpdate >= hueUpdateFrequency) {
            hueUpdate = 0;
            hue++;
        }
        else {
            hueUpdate++;
        }
        
        blurScreen(20); // @Palpalych советует делать размытие. вот в этом эффекте его явно не хватает...
}
#endif


#ifdef DEF_FIRE_2018
// ============= ЭФФЕКТ ОГОНЬ 2018 ===============
// https://gist.github.com/StefanPetrick/819e873492f344ebebac5bcd2fdd8aa8
// https://gist.github.com/StefanPetrick/1ba4584e534ba99ca259c1103754e4c5
// Адаптация от (c) SottNick

// parameters and buffer for the noise array
// (вместо закомментированных строк используются массивы и переменные от эффекта Кометы для экономии памяти)
//define NUM_LAYERS 2 // менять бесполезно, так как в коде чётко использовано 2 слоя
//uint32_t noise32_x[NUM_LAYERSMAX];
//uint32_t noise32_y[NUM_LAYERSMAX];
//uint32_t noise32_z[NUM_LAYERSMAX];
//uint32_t scale32_x[NUM_LAYERSMAX];
//uint32_t scale32_y[NUM_LAYERSMAX];
//uint8_t noise3d[NUM_LAYERSMAX][WIDTH][HEIGHT];
//uint8_t fire18heat[NUM_LEDS]; будем использовать вместо него ledsbuff[NUM_LEDS].r
// this finds the right index within a serpentine matrix

static void Fire2018_2() {
//  const uint8_t CENTER_Y_MAJOR =  HEIGHT / 2 + (HEIGHT % 2);
//  const uint8_t CENTER_X_MAJOR =  WIDTH / 2  + (WIDTH % 2) ;

  #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      setModeSettings(1U+random8(50U), 195U+random8(44U));
    }
  #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)


  // some changing values
  uint16_t ctrl1 = inoise16(11 * millis(), 0, 0);
  uint16_t ctrl2 = inoise16(13 * millis(), 100000, 100000);
  uint16_t  ctrl = ((ctrl1 + ctrl2) / 2);

  // parameters for the heatmap
  uint16_t speed = 25;
  noise32_x[0] = 3 * ctrl * speed;
  noise32_y[0] = 20 * millis() * speed;
  noise32_z[0] = 5 * millis() * speed ;
  scale32_x[0] = ctrl1 / 2;
  scale32_y[0] = ctrl2 / 2;

  //calculate the noise data
  uint8_t layer = 0;

  for (uint8_t i = 0; i < WIDTH; i++) {
    uint32_t ioffset = scale32_x[layer] * (i - CENTER_X_MAJOR);
    for (uint8_t j = 0; j < HEIGHT; j++) {
      uint32_t joffset = scale32_y[layer] * (j - CENTER_Y_MAJOR);
      uint16_t data = ((inoise16(noise32_x[layer] + ioffset, noise32_y[layer] + joffset, noise32_z[layer])) + 1);
      noise3d[layer][i][j] = data >> 8;
    }
  }

  // parameters for te brightness mask
  speed = 20;
  noise32_x[1] = 3 * ctrl * speed;
  noise32_y[1] = 20 * millis() * speed;
  noise32_z[1] = 5 * millis() * speed ;
  scale32_x[1] = ctrl1 / 2;
  scale32_y[1] = ctrl2 / 2;

  //calculate the noise data
  layer = 1;
  for (uint8_t i = 0; i < WIDTH; i++) {
    uint32_t ioffset = scale32_x[layer] * (i - CENTER_X_MAJOR);
    for (uint8_t j = 0; j < HEIGHT; j++) {
      uint32_t joffset = scale32_y[layer] * (j - CENTER_Y_MAJOR);
      uint16_t data = ((inoise16(noise32_x[layer] + ioffset, noise32_y[layer] + joffset, noise32_z[layer])) + 1);
      noise3d[layer][i][j] = data >> 8;
    }
  }

  // draw lowest line - seed the fire
  for (uint8_t x = 0; x < WIDTH; x++) {
    ledsbuff[XY(x, HEIGHT - 1)].r =  noise3d[0][WIDTH - 1 - x][CENTER_Y_MAJOR - 1]; // хз, почему взято с середины. вожможно, нужно просто с 7 строки вне зависимости от высоты матрицы
  }


  //copy everything one line up
  for (uint8_t y = 0; y < HEIGHT - 1; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      ledsbuff[XY(x, y)].r = ledsbuff[XY(x, y + 1)].r;
    }
  }

  //dim
  for (uint8_t y = 0; y < HEIGHT - 1; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      uint8_t dim = noise3d[0][x][y];
      // high value = high flames
      dim = dim / 1.7;
      dim = 255 - dim;
      ledsbuff[XY(x, y)].r = scale8(ledsbuff[XY(x, y)].r , dim);
    }
  }

  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      // map the colors based on heatmap
      //leds[XY(x, HEIGHT - 1 - y)] = CRGB( ledsbuff[XY(x, y)].r, 1 , 0);
      //leds[XY(x, HEIGHT - 1 - y)] = CRGB( ledsbuff[XY(x, y)].r, ledsbuff[XY(x, y)].r * 0.153, 0);// * 0.153 - лучший оттенок
      leds[XY(x, HEIGHT - 1 - y)] = CRGB( ledsbuff[XY(x, y)].r, (float)ledsbuff[XY(x, y)].r * modes[currentMode].Scale * 0.01, 0);
      

      //пытался понять, как регулировать оттенок пламени...
      //  if (modes[currentMode].Scale > 50)
      //    leds[XY(x, HEIGHT - 1 - y)] = CRGB( ledsbuff[XY(x, y)].r, ledsbuff[XY(x, y)].r * (modes[currentMode].Scale % 50)  * 0.051, 0);
      //  else
      //    leds[XY(x, HEIGHT - 1 - y)] = CRGB( ledsbuff[XY(x, y)].r, 1 , ledsbuff[XY(x, y)].r * modes[currentMode].Scale * 0.051);
      //примерно понял
   
      // dim the result based on 2nd noise layer
      leds[XY(x, HEIGHT - 1 - y)].nscale8(noise3d[1][x][y]);
    }
  }

}
#endif


#ifdef DEF_FIRE_2012
// ============= ЭФФЕКТ ОГОНЬ 2012 ===============
// там выше есть его копии для эффектов Водопад и Водопад 4 в 1
// по идее, надо бы объединить и оптимизировать, но мелких отличий довольно много
// based on FastLED example Fire2012WithPalette: https://github.com/FastLED/FastLED/blob/master/examples/Fire2012WithPalette/Fire2012WithPalette.ino

static void fire2012again()
{
  if (loadingFlag)
  {
    loadingFlag = false;
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        uint8_t tmp = 17U+random8(55U);
        if (tmp>22) tmp += 28;
        setModeSettings(tmp, 185U+random8(50U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    if (modes[currentMode].Scale > 100) modes[currentMode].Scale = 100; // чтобы не было проблем при прошивке без очистки памяти
    if (modes[currentMode].Scale > 50) 
      //fire_p = firePalettes[(int)((float)modes[currentMode].Scale/12)];
      //fire_p = firePalettes[(uint8_t)((modes[currentMode].Scale % 50)/5.56F)];
      curPalette = firePalettes[(uint8_t)((modes[currentMode].Scale - 50)/50.0F * ((sizeof(firePalettes)/sizeof(TProgmemRGBPalette16 *))-0.01F))];
    else
      curPalette = palette_arr[(uint8_t)(modes[currentMode].Scale/50.0F * ((sizeof(palette_arr)/sizeof(TProgmemRGBPalette16 *))-0.01F))];
  }
  
#if HEIGHT/6 > 6
  #define FIRE_BASE 6
#else
  #define FIRE_BASE HEIGHT/6+1
#endif
  // COOLING: How much does the air cool as it rises?
  // Less cooling = taller flames.  More cooling = shorter flames.
  #define cooling 70U
  // SPARKING: What chance (out of 255) is there that a new spark will be lit?
  // Higher chance = more roaring fire.  Lower chance = more flickery fire.
  #define sparking 130U
  // SMOOTHING; How much blending should be done between frames
  // Lower = more blending and smoother flames. Higher = less blending and flickery flames
  #define fireSmoothing 80U
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(256));

  // Loop for each column individually
  for (uint8_t x = 0; x < WIDTH; x++) {
    // Step 1.  Cool down every cell a little
    for (uint8_t i = 0; i < HEIGHT; i++) {
      noise3d[0][x][i] = qsub8(noise3d[0][x][i], random(0, ((cooling * 10) / HEIGHT) + 2));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (uint8_t k = HEIGHT - 1; k > 0; k--) { // fixed by SottNick
      noise3d[0][x][k] = (noise3d[0][x][k - 1] + noise3d[0][x][k - 1] + noise3d[0][x][wrapY(k - 2)]) / 3; // fixed by SottNick
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if (random8() < sparking) {
      uint8_t j = random8(FIRE_BASE);
      noise3d[0][x][j] = qadd8(noise3d[0][x][j], random(160, 255));
    }

    // Step 4.  Map from heat cells to LED colors
    // Blend new data with previous frame. Average data between neighbouring pixels
    for (uint8_t y = 0; y < HEIGHT; y++)
      nblend(leds[XY(x,y)], ColorFromPalette(*curPalette, ((noise3d[0][x][y]*0.7) + (noise3d[0][wrapX(x+1)][y]*0.3))), fireSmoothing);
  }
}
#endif


#if defined(DEF_SIMPLE_RAIN) || defined(DEF_STORMY_RAIN) || defined(DEF_COLOR_RAIN)
// ============= ЭФФЕКТЫ ОСАДКИ / ТУЧКА В БАНКЕ / ГРОЗА В БАНКЕ ===============
// https://github.com/marcmerlin/FastLED_NeoMatrix_SmartMatrix_LEDMatrix_GFX_Demos/blob/master/FastLED/Sublime_Demos/Sublime_Demos.ino
// там по ссылке ещё остались эффекты с 3 по 9 (в SimplePatternList перечислены)

//прикольная процедура добавляет блеск почти к любому эффекту после его отрисовки https://www.youtube.com/watch?v=aobtR1gIyIo
//void addGlitter( uint8_t chanceOfGlitter){
//  if ( random8() < chanceOfGlitter) leds[ random16(NUM_LEDS) ] += CRGB::White;
//}

//static uint8_t intensity = 42;  // будет бегунок масштаба

// Array of temp cells (used by fire, theMatrix, coloredRain, stormyRain)
// uint8_t **tempMatrix; = noise3d[0][WIDTH][HEIGHT]
// uint8_t *splashArray; = line[WIDTH] из эффекта Огонь

static CRGB solidRainColor = CRGB(60,80,90);

static void rain(byte backgroundDepth, byte maxBrightness, byte spawnFreq, byte tailLength, CRGB rainColor, bool splashes, bool clouds, bool storm)
{
  ff_x = random16();
  ff_y = random16();
  ff_z = random16();

  CRGB lightningColor = CRGB(72,72,80);
  CRGBPalette16 rain_p( CRGB::Black, rainColor );
#ifdef SMARTMATRIX
  CRGBPalette16 rainClouds_p( CRGB::Black, CRGB(75,84,84), CRGB(49,75,75), CRGB::Black );
#else
  CRGBPalette16 rainClouds_p( CRGB::Black, CRGB(15,24,24), CRGB(9,15,15), CRGB::Black );
#endif

  //fadeToBlackBy( leds, NUM_LEDS, 255-tailLength);
  dimAll(tailLength);

  // Loop for each column individually
  for (uint8_t x = 0; x < WIDTH; x++) {
    // Step 1.  Move each dot down one cell
    for (uint8_t i = 0; i < HEIGHT; i++) {
      if (noise3d[0][x][i] >= backgroundDepth) {  // Don't move empty cells
        if (i > 0) noise3d[0][x][wrapY(i-1)] = noise3d[0][x][i];
        noise3d[0][x][i] = 0;
      }
    }

    // Step 2.  Randomly spawn new dots at top
    if (random8() < spawnFreq) {
      noise3d[0][x][HEIGHT-1] = random(backgroundDepth, maxBrightness);
    }

    // Step 3. Map from tempMatrix cells to LED colors
    for (uint8_t y = 0; y < HEIGHT; y++) {
      if (noise3d[0][x][y] >= backgroundDepth) {  // Don't write out empty cells
        leds[XY(x,y)] = ColorFromPalette(rain_p, noise3d[0][x][y]);
      }
    }

    // Step 4. Add splash if called for
    if (splashes) {
      // FIXME, this is broken
      byte j = line[x];
      byte v = noise3d[0][x][0];

      if (j >= backgroundDepth) {
        leds[XY(wrapX(x-2),0)] = ColorFromPalette(rain_p, j/3);
        leds[XY(wrapX(x+2),0)] = ColorFromPalette(rain_p, j/3);
        line[x] = 0;   // Reset splash
      }

      if (v >= backgroundDepth) {
        leds[XY(wrapX(x-1),1)] = ColorFromPalette(rain_p, v/2);
        leds[XY(wrapX(x+1),1)] = ColorFromPalette(rain_p, v/2);
        line[x] = v; // Prep splash for next frame
      }
    }

    // Step 5. Add lightning if called for
    if (storm) {
      //uint8_t lightning[WIDTH][HEIGHT];
      // ESP32 does not like static arrays  https://github.com/espressif/arduino-esp32/issues/2567
      uint8_t *lightning = (uint8_t *) malloc(WIDTH * HEIGHT);
      while (lightning == NULL) { Serial.println("lightning malloc failed"); }


      if (random16() < 72) {    // Odds of a lightning bolt
        lightning[scale8(random8(), WIDTH-1) + (HEIGHT-1) * WIDTH] = 255;  // Random starting location
        for(uint8_t ly = HEIGHT-1; ly > 1; ly--) {
          for (uint8_t lx = 1; lx < WIDTH-1; lx++) {
            if (lightning[lx + ly * WIDTH] == 255) {
              lightning[lx + ly * WIDTH] = 0;
              uint8_t dir = random8(4);
              switch (dir) {
                case 0:
                  leds[XY(lx+1,ly-1)] = lightningColor;
                  lightning[(lx+1) + (ly-1) * WIDTH] = 255; // move down and right
                break;
                case 1:
                  leds[XY(lx,ly-1)] = CRGB(128,128,128); // я без понятия, почему у верхней молнии один оттенок, а у остальных - другой
                  lightning[lx + (ly-1) * WIDTH] = 255;    // move down
                break;
                case 2:
                  leds[XY(lx-1,ly-1)] = CRGB(128,128,128);
                  lightning[(lx-1) + (ly-1) * WIDTH] = 255; // move down and left
                break;
                case 3:
                  leds[XY(lx-1,ly-1)] = CRGB(128,128,128);
                  lightning[(lx-1) + (ly-1) * WIDTH] = 255; // fork down and left
                  leds[XY(lx-1,ly-1)] = CRGB(128,128,128);
                  lightning[(lx+1) + (ly-1) * WIDTH] = 255; // fork down and right
                break;
              }
            }
          }
        }
      }
      free(lightning);
    }

    // Step 6. Add clouds if called for
    if (clouds) {
      uint16_t noiseScale = 250;  // A value of 1 will be so zoomed in, you'll mostly see solid colors. A value of 4011 will be very zoomed out and shimmery
      //const uint16_t cloudHeight = (HEIGHT*0.2)+1;
      const uint8_t cloudHeight = HEIGHT * 0.4 + 1; // это уже 40% c лишеним, но на высоких матрицах будет чуть меньше

      // This is the array that we keep our computed noise values in
      //static uint8_t noise[WIDTH][cloudHeight];
      static uint8_t *noise = (uint8_t *) malloc(WIDTH * cloudHeight);
      
      while (noise == NULL) { Serial.println("noise malloc failed"); }
      int xoffset = noiseScale * x + hue;

      for(uint8_t z = 0; z < cloudHeight; z++) {
        int yoffset = noiseScale * z - hue;
        uint8_t dataSmoothing = 192;
        uint8_t noiseData = qsub8(inoise8(ff_x + xoffset,ff_y + yoffset,ff_z),16);
        noiseData = qadd8(noiseData,scale8(noiseData,39));
        noise[x * cloudHeight + z] = scale8( noise[x * cloudHeight + z], dataSmoothing) + scale8( noiseData, 256 - dataSmoothing);
        nblend(leds[XY(x,HEIGHT-z-1)], ColorFromPalette(rainClouds_p, noise[x * cloudHeight + z]), (cloudHeight-z)*(250/cloudHeight));
      }
      ff_z ++;
    }
  }
}

static uint8_t myScale8(uint8_t x) { // даёт масштабировать каждые 8 градаций (от 0 до 7) бегунка Масштаб в значения от 0 до 255 по типа синусоиде
  uint8_t x8 = x % 8U;
  uint8_t x4 = x8 % 4U;
  if (x4 == 0U)
    if (x8 == 0U)       return 0U;
    else                return 255U;
  else if (x8 < 4U)     return (1U   + x4 * 72U); // всего 7шт по 36U + 3U лишних = 255U (чтобы восхождение по синусоиде не было зеркально спуску)
//else
                        return (253U - x4 * 72U); // 253U = 255U - 2U
}

static void coloredRain() // внимание! этот эффект заточен на работу бегунка Масштаб в диапазоне от 0 до 255. пока что единственный.
{
  #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      uint8_t tmp = 1U+random8(255U);
      if ((tmp%4U == 0U) && (tmp%8U != 0U)) tmp--;
      setModeSettings(tmp, 165U+random8(76U));
    }
  #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

  // я хз, как прикрутить а 1 регулятор и длину хвостов и цвет капель
  // ( Depth of dots, maximum brightness, frequency of new dots, length of tails, color, splashes, clouds, ligthening )
  //rain(60, 200, map8(intensity,5,100), 195, CRGB::Green, false, false, false); // было CRGB::Green
  if (modes[currentMode].Scale > 247U)
    rain(60, 200, map8(42,5,100), myScale8(modes[currentMode].Scale), solidRainColor, false, false, false);
  else
    rain(60, 200, map8(42,5,100), myScale8(modes[currentMode].Scale), CHSV(modes[currentMode].Scale, 255U, 255U), false, false, false);
}

static void simpleRain()
{
  #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      setModeSettings(random8(2U) ? 2U+random8(7U) : 9U+random8(70U), 220U+random8(22U));
    }
  #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

  // ( Depth of dots, maximum brightness, frequency of new dots, length of tails, color, splashes, clouds, ligthening )
  //rain(60, 200, map8(intensity,2,60), 10, solidRainColor, true, true, false);
  rain(60, 180,(modes[currentMode].Scale-1) * 2.58, 30, solidRainColor, true, true, false);
}

static void stormyRain()
{
  #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings){
      setModeSettings(random8(2U) ? 2U+random8(15U) : 17U+random8(64U), 220U+random8(22U));
    }
  #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

  // ( Depth of dots, maximum brightness, frequency of new dots, length of tails, color, splashes, clouds, ligthening )
  //rain(0, 90, map8(intensity,0,150)+60, 10, solidRainColor, true, true, true);
  rain(60, 160, (modes[currentMode].Scale-1) * 2.58, 30, solidRainColor, true, true, true);
}
#endif


#ifdef DEF_TWINKLES
// ------------------------------ ЭФФЕКТ МЕРЦАНИЕ ----------------------
// (c) SottNick

#define TWINKLES_SPEEDS 4     // всего 4 варианта скоростей мерцания
#define TWINKLES_MULTIPLIER 6 // слишком медленно, если на самой медленной просто по единичке к яркости добавлять

static void twinklesRoutine(){
    if (loadingFlag)
    {
      #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
        if (selectedSettings){
          setModeSettings(random8(8U)*11U+2U+random8(9U) , 180U+random8(69U));
        }
      #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

      loadingFlag = false;
      setCurrentPalette();

      hue = 0U;
      deltaValue = (modes[currentMode].Scale - 1U) % 11U + 1U;  // вероятность пикселя загореться от 1/1 до 1/11
      for (uint32_t idx=0; idx < NUM_LEDS; idx++) {
        if (random8(deltaValue) == 0){
          ledsbuff[idx].r = random8();                          // оттенок пикселя
          ledsbuff[idx].g = random8(1, TWINKLES_SPEEDS * 2 +1); // скорость и направление (нарастает 1-4 или угасает 5-8)
          ledsbuff[idx].b = random8();                          // яркость
        }
        else
          ledsbuff[idx] = 0;                                    // всё выкл
      }
    }
    for (uint32_t idx=0; idx < NUM_LEDS; idx++) {
      if (ledsbuff[idx].b == 0){
        if (random8(deltaValue) == 0 && hue > 0){                                        // если пиксель ещё не горит, зажигаем каждый ХЗй
          ledsbuff[idx].r = random8();                                                   // оттенок пикселя
          ledsbuff[idx].g = random8(1, TWINKLES_SPEEDS +1);                              // скорость и направление (нарастает 1-4, но не угасает 5-8)
          ledsbuff[idx].b = ledsbuff[idx].g;                                             // яркость
          hue--;                                                                         // уменьшаем количество погасших пикселей
        }
      }
      else if (ledsbuff[idx].g <= TWINKLES_SPEEDS){                                      // если нарастание яркости
        if (ledsbuff[idx].b > 255U - ledsbuff[idx].g - TWINKLES_MULTIPLIER){             // если досигнут максимум
          ledsbuff[idx].b = 255U;
          ledsbuff[idx].g = ledsbuff[idx].g + TWINKLES_SPEEDS;
        }
        else
          ledsbuff[idx].b = ledsbuff[idx].b + ledsbuff[idx].g + TWINKLES_MULTIPLIER;
      }
      else {                                                                             // если угасание яркости
        if (ledsbuff[idx].b <= ledsbuff[idx].g - TWINKLES_SPEEDS + TWINKLES_MULTIPLIER){ // если досигнут минимум
          ledsbuff[idx].b = 0;                                                           // всё выкл
          hue++;                                                                         // считаем количество погасших пикселей
        }
        else
          ledsbuff[idx].b = ledsbuff[idx].b - ledsbuff[idx].g + TWINKLES_SPEEDS - TWINKLES_MULTIPLIER;
      }
      if (ledsbuff[idx].b == 0)
        leds[idx] = 0U;
      else
        leds[idx] = ColorFromPalette(*curPalette, ledsbuff[idx].r, ledsbuff[idx].b);
    }
}
#endif


#ifdef DEF_BALLS_BOUNCE
// ============= BOUNCE / ПРЫЖКИ / МЯЧИКИ БЕЗ ГРАНИЦ ===============
// Aurora : https://github.com/pixelmatix/aurora/blob/master/PatternBounce.h
// Copyright(c) 2014 Jason Coon
// v1.0 - Updating for GuverLamp v1.7 by Palpalych 14.04.2020
//#define e_bnc_COUNT (WIDTH) // теперь enlargedObjectNUM. хз, почему использовалась ширина матрицы тут, если по параметру идёт обращение к массиву boids, у которого может быть меньший размер
#define e_bnc_SIDEJUMP (true)

static PVector gravity = PVector(0, -0.0125);

static void bounceRoutine()
{
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(random8(9U)*11U+3U+random8(9U), random8(4U) ? 3U+random8(26U) : 255U);
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    setCurrentPalette();

    enlargedObjectNUM = (modes[currentMode].Scale - 1U) % 11U / 10.0 * (AVAILABLE_BOID_COUNT - 1U) + 1U;
    uint8_t colorWidth = 256U / enlargedObjectNUM;
    for (uint8_t i = 0; i < enlargedObjectNUM; i++)
    {
      Boid boid = Boid(i % WIDTH, 0);
      boid.velocity.x = 0;
      boid.velocity.y = i * -0.01;
      boid.colorIndex = colorWidth * i;
      boid.maxforce = 10;
      boid.maxspeed = 10;
      boids[i] = boid;
    }
  }
  blurScreen(beatsin8(5U, 1U, 5U));
  dimAll(255U - modes[currentMode].Speed);
  for (uint8_t i = 0; i < enlargedObjectNUM; i++)
  {
    Boid boid = boids[i];
    boid.applyForce(gravity);
    boid.update();
    if (boid.location.x >= WIDTH) boid.location.x = boid.location.x - WIDTH; // это только
    else if (boid.location.x < 0) boid.location.x = boid.location.x + WIDTH; // для субпиксельной версии
    CRGB color = ColorFromPalette(*curPalette, boid.colorIndex);
    drawPixelXYF(boid.location.x, boid.location.y, color);

    if (boid.location.y <= 0)
    {
      boid.location.y = 0;
      boid.velocity.y = -boid.velocity.y;
      boid.velocity.x *= 0.9;
      if (!random8() || boid.velocity.y < 0.01)
      {
#if e_bnc_SIDEJUMP
        boid.applyForce(PVector((float)random(127) / 255 - 0.25, (float)random(255) / 255));
#else
        boid.applyForce(PVector(0, (float)random(255) / 255));
#endif
      }
    }
    boids[i] = boid;
  }
}
#endif


#ifdef DEF_RINGS
// ------------------------------ ЭФФЕКТ КОЛЬЦА / КОДОВЫЙ ЗАМОК ----------------------
// (c) SottNick
// из-за повторного использоваия переменных от других эффектов теперь в этом коде невозможно что-то понять.
// поэтому для понимания придётся сперва заменить названия переменных на человеческие. но всё равно это песец, конечно.
// uint8_t deltaHue2; // максимальне количество пикселей в кольце (толщина кольца) от 1 до HEIGHT / 2 + 1
// uint8_t deltaHue; // количество колец от 2 до HEIGHT
// uint8_t noise3d[1][1][HEIGHT]; // начальный оттенок каждого кольца (оттенка из палитры) 0-255
// uint8_t shiftValue[HEIGHT]; // местоположение начального оттенка кольца 0-WIDTH-1
// uint8_t shiftHue[HEIGHT]; // 4 бита на ringHueShift, 4 на ringHueShift2
// ringHueShift[ringsCount]; // шаг градиета оттенка внутри кольца -8 - +8 случайное число
// ringHueShift2[ringsCount]; // обычная скорость переливания оттенка всего кольца -8 - +8 случайное число
// uint8_t deltaValue; // кольцо, которое в настоящий момент нужно провернуть
// uint8_t step; // оставшееся количество шагов, на которое нужно провернуть активное кольцо - случайное от WIDTH/5 до WIDTH-3
// uint8_t hue, hue2; // количество пикселей в нижнем (hue) и верхнем (hue2) кольцах

static void ringsRoutine(){
    uint8_t h, x, y;
    if (loadingFlag)
    {
      #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
        if (selectedSettings){
          setModeSettings(90U+random8(6U), 175U+random8(61U));
        }
      #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      
      loadingFlag = false;
      setCurrentPalette();

      //deltaHue2 = (modes[currentMode].Scale - 1U) / 99.0 * (HEIGHT / 2 - 1U) + 1U; // толщина кольца в пикселях. если на весь бегунок масштаба (от 1 до HEIGHT / 2 + 1)
      deltaHue2 = (modes[currentMode].Scale - 1U) % 11U + 1U; // толщина кольца от 1 до 11 для каждой из палитр
      deltaHue = HEIGHT / deltaHue2 + ((HEIGHT % deltaHue2 == 0U) ? 0U : 1U); // количество колец
      hue2 = deltaHue2 - (deltaHue2 * deltaHue - HEIGHT) / 2U; // толщина верхнего кольца. может быть меньше нижнего
      hue = HEIGHT - hue2 - (deltaHue - 2U) * deltaHue2; // толщина нижнего кольца = всё оставшееся
      for (uint8_t i = 0; i < deltaHue; i++)
      {
        noise3d[0][0][i] = random8(257U - WIDTH / 2U); // начальный оттенок кольца из палитры 0-255 за минусом длины кольца, делённой пополам
        shiftHue[i] = random8();
        shiftValue[i] = 0U; //random8(WIDTH); само прокрутится постепенно
        step = 0U;
        //do { // песец конструкцию придумал бредовую
        //  step = WIDTH - 3U - random8((WIDTH - 3U) * 2U); само присвоится при первом цикле
        //} while (step < WIDTH / 5U || step > 255U - WIDTH / 5U);
        deltaValue = random8(deltaHue);
      }
      
    }
    for (uint8_t i = 0; i < deltaHue; i++)
    {
      if (i != deltaValue) // если это не активное кольцо
        {
          h = shiftHue[i] & 0x0F; // сдвигаем оттенок внутри кольца
          if (h > 8U)
            //noise3d[0][0][i] += (uint8_t)(7U - h); // с такой скоростью сдвиг оттенка от вращения кольца не отличается
            noise3d[0][0][i]--;
          else
            //noise3d[0][0][i] += h;
            noise3d[0][0][i]++;
        }
      else
        {
          if (step == 0) // если сдвиг активного кольца завершён, выбираем следующее
            {
              deltaValue = random8(deltaHue);
              do {
                step = WIDTH - 3U - random8((WIDTH - 3U) * 2U); // проворот кольца от хз до хз 
              } while (step < WIDTH / 5U || step > 255U - WIDTH / 5U);
            }
          else
            {
              if (step > 127U)
                {
                  step++;
                  shiftValue[i] = (shiftValue[i] + 1U) % WIDTH;
                }
              else
                {
                  step--;
                  shiftValue[i] = (shiftValue[i] - 1U + WIDTH) % WIDTH;
                }
            }
        }
        // отрисовываем кольца
        h = (shiftHue[i] >> 4) & 0x0F; // берём шаг для градиента вутри кольца
        if (h > 8U)
          h = 7U - h;
        for (uint8_t j = 0U; j < ((i == 0U) ? hue : ((i == deltaHue - 1U) ? hue2 : deltaHue2)); j++) // от 0 до (толщина кольца - 1)
        {
          y = i * deltaHue2 + j - ((i == 0U) ? 0U : deltaHue2 - hue);
          // mod для чётных скоростей by @kostyamat - получается какая-то другая фигня. не стоит того
          //for (uint8_t k = 0; k < WIDTH / ((modes[currentMode].Speed & 0x01) ? 2U : 4U); k++) // полукольцо для нечётных скоростей и четверть кольца для чётных
          for (uint8_t k = 0; k < WIDTH / 2U; k++) // полукольцо
            {
              x = (shiftValue[i] + k) % WIDTH; // первая половина кольца
              leds[XY(x, y)] = ColorFromPalette(*curPalette, noise3d[0][0][i] + k * h);
              x = (WIDTH - 1 + shiftValue[i] - k) % WIDTH; // вторая половина кольца (зеркальная первой)
              leds[XY(x, y)] = ColorFromPalette(*curPalette, noise3d[0][0][i] + k * h);
            }
          if (WIDTH & 0x01) //(WIDTH % 2U > 0U) // если число пикселей по ширине матрицы нечётное, тогда не забываем и про среднее значение
          {
            x = (shiftValue[i] + WIDTH / 2U) % WIDTH;
            leds[XY(x, y)] = ColorFromPalette(*curPalette, noise3d[0][0][i] + WIDTH / 2U * h);
          }
        }
    }
}
#endif


#ifdef DEF_CUBE2D
// ------------------------------ ЭФФЕКТ КУБИК РУБИКА 2D ----------------------
// (c) SottNick

#define PAUSE_MAX 7 // пропустить 7 кадров после завершения анимации сдвига ячеек

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

static void cube2dRoutine(){
    uint8_t x, y;
    uint8_t anim0; // будем считать тут начальный пиксель для анимации сдвига строки/колонки
    int8_t shift, kudaVse; // какое-то расчётное направление сдвига (-1, 0, +1)
    CRGB color, color2;
    
    if (loadingFlag)
    {
      #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
        if (selectedSettings){
          uint8_t tmp = random8(9U)*11U+random8(8U); // масштаб 1-7, палитры все 9 
          if (tmp == 45U) tmp = 100U; //+ белый цвет
          setModeSettings(tmp, 175U+random8(66U));
        }
      #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

      loadingFlag = false;
      setCurrentPalette();

      ledsClear(); // esphome: FastLED.clear();

      razmerX = (modes[currentMode].Scale - 1U) % 11U + 1U; // размер ячейки от 1 до 11 пикселей для каждой из 9 палитр
      razmerY = razmerX;
      if (modes[currentMode].Speed & 0x01) // по идее, ячейки не обязательно должны быть квадратными, поэтому можно тут поизвращаться
        razmerY = (razmerY << 1U) + 1U;

      shtukY = HEIGHT / (razmerY + 1U);
      if (shtukY < 2U)
        shtukY = 2U;
      y = HEIGHT / shtukY - 1U;
      if (razmerY > y)
        razmerY = y;
      poleY = (razmerY + 1U) * shtukY;
      shtukX = WIDTH / (razmerX + 1U);
      if (shtukX < 2U)
        shtukX = 2U;
      x = WIDTH / shtukX - 1U;
      if (razmerX > x)
        razmerX = x;
      poleX = (razmerX + 1U) * shtukX;
      seamlessX = (poleX == WIDTH);
      deltaHue = 0U;
      deltaHue2 = 0U;
      globalShiftX = 0;
      globalShiftY = 0;

      for (uint8_t j = 0U; j < shtukY; j++)
      {
        y = j * (razmerY + 1U); // + deltaHue2 т.к. оно =0U
        for (uint8_t i = 0U; i < shtukX; i++)
        {
          x = i * (razmerX + 1U); // + deltaHue т.к. оно =0U
          if (modes[currentMode].Scale == 100U)
            color = CHSV(45U, 0U, 128U + random8(128U));
          else  
            color = ColorFromPalette(*curPalette, random8());
          for (uint8_t k = 0U; k < razmerY; k++)
            for (uint8_t m = 0U; m < razmerX; m++)
              leds[XY(x+m, y+k)] = color;
        }
      }
      step = 4U; // текущий шаг сдвига первоначально с перебором (от 0 до deltaValue-1)
      deltaValue = 4U; // всего шагов сдвига (от razmer? до (razmer?+1) * shtuk?)
      hue2 = 0U; // осталось шагов паузы

      //это лишнее обнуление
      //krutimVertikalno = true;
      //for (uint8_t i = 0U; i < shtukX; i++)
      //  noise3d[0][i][0] = 0U;
    }

  //двигаем, что получилось...
  if (hue2 == 0 && step < deltaValue) // если пауза закончилась, а цикл вращения ещё не завершён
  {
    step++;
    if (krutimVertikalno)
    {
      for (uint8_t i = 0U; i < shtukX; i++)
      {
        x = (deltaHue + i * (razmerX + 1U)) % WIDTH;
        if (noise3d[0][i][0] > 0) // в нулевой ячейке храним оставшееся количество ходов прокрутки
        {
          noise3d[0][i][0]--;
          shift = noise3d[0][i][1] - 1; // в первой ячейке храним направление прокрутки

          if (globalShiftY == 0)
            anim0 = (deltaHue2 == 0U) ? 0U : deltaHue2 - 1U;
          else if (globalShiftY > 0)
            anim0 = deltaHue2;
          else
            anim0 = deltaHue2 - 1U;
          
          if (shift < 0) // если крутим столбец вниз
          {
            color = leds[XY(x, anim0)];                                   // берём цвет от нижней строчки
            for (uint8_t k = anim0; k < anim0+poleY-1; k++)
            {
              color2 = leds[XY(x,k+1)];                                   // берём цвет от строчки над нашей
              for (uint8_t m = x; m < x + razmerX; m++)
                leds[XY(m % WIDTH,k)] = color2;                           // копируем его на всю нашу строку
            }
            for   (uint8_t m = x; m < x + razmerX; m++)
              leds[XY(m % WIDTH,anim0+poleY-1)] = color;                  // цвет нижней строчки копируем на всю верхнюю
          }
          else if (shift > 0) // если крутим столбец вверх
          {
            color = leds[XY(x,anim0+poleY-1)];                            // берём цвет от верхней строчки
            for (uint8_t k = anim0+poleY-1; k > anim0 ; k--)
            {
              color2 = leds[XY(x,k-1)];                                   // берём цвет от строчки под нашей
              for (uint8_t m = x; m < x + razmerX; m++)
                leds[XY(m % WIDTH,k)] = color2;                           // копируем его на всю нашу строку
            }
            for   (uint8_t m = x; m < x + razmerX; m++)
              leds[XY(m % WIDTH, anim0)] = color;                         // цвет верхней строчки копируем на всю нижнюю
          }
        }
      }
    }
    else
    {
      for (uint8_t j = 0U; j < shtukY; j++)
      {
        y = deltaHue2 + j * (razmerY + 1U);
        if (noise3d[0][0][j] > 0) // в нулевой ячейке храним оставшееся количество ходов прокрутки
        {
          noise3d[0][0][j]--;
          shift = noise3d[0][1][j] - 1; // в первой ячейке храним направление прокрутки
      
          if (seamlessX)
            anim0 = 0U;
          else if (globalShiftX == 0)
            anim0 = (deltaHue == 0U) ? 0U : deltaHue - 1U;
          else if (globalShiftX > 0)
            anim0 = deltaHue;
          else
            anim0 = deltaHue - 1U;
          
          if (shift < 0) // если крутим строку влево
          {
            color = leds[XY(anim0, y)];                            // берём цвет от левой колонки (левого пикселя)
            for (uint8_t k = anim0; k < anim0+poleX-1; k++)
            {
              color2 = leds[XY(k+1, y)];                           // берём цвет от колонки (пикселя) правее
              for (uint8_t m = y; m < y + razmerY; m++)
                leds[XY(k, m)] = color2;                           // копируем его на всю нашу колонку
            }
            for   (uint8_t m = y; m < y + razmerY; m++)
              leds[XY(anim0+poleX-1, m)] = color;                  // цвет левой колонки копируем на всю правую
          }
          else if (shift > 0) // если крутим столбец вверх
          {
            color = leds[XY(anim0+poleX-1, y)];                    // берём цвет от правой колонки
            for (uint8_t k = anim0+poleX-1; k > anim0 ; k--)
            {
              color2 = leds[XY(k-1, y)];                           // берём цвет от колонки левее
              for (uint8_t m = y; m < y + razmerY; m++)
                leds[XY(k, m)] = color2;                           // копируем его на всю нашу колонку
            }
            for   (uint8_t m = y; m < y + razmerY; m++)
              leds[XY(anim0, m)] = color;                          // цвет правой колонки копируем на всю левую
          }
        }
      }
    }
   
  }
  else if (hue2 != 0U) // пропускаем кадры после прокрутки кубика (делаем паузу)
    hue2--;

  if (step >= deltaValue) // если цикл вращения завершён, меняем местами соответствующие ячейки (цвет в них) и точку первой ячейки
    {
      step = 0U; 
      hue2 = PAUSE_MAX;
      //если часть ячеек двигалась на 1 пиксель, пододвигаем глобальные координаты начала
      deltaHue2 = deltaHue2 + globalShiftY; //+= globalShiftY;
      globalShiftY = 0;
      //deltaHue += globalShiftX; для бесшовной не годится
      deltaHue = (WIDTH + deltaHue + globalShiftX) % WIDTH;
      globalShiftX = 0;

      //пришла пора выбрать следующие параметры вращения
      kudaVse = 0;
      krutimVertikalno = random8(2U);
      if (krutimVertikalno) // идём по горизонтали, крутим по вертикали (столбцы двигаются)
      {
        for (uint8_t i = 0U; i < shtukX; i++)
        {
          noise3d[0][i][1] = random8(3);
          shift = noise3d[0][i][1] - 1; // в первой ячейке храним направление прокрутки
          if (kudaVse == 0)
            kudaVse = shift;
          else if (shift != 0 && kudaVse != shift)
            kudaVse = 50;
        }
        deltaValue = razmerY + ((deltaHue2 - kudaVse >= 0 && deltaHue2 - kudaVse + poleY < (int)HEIGHT) ? random8(2U) : 1U);

/*        if (kudaVse == 0) // пытался сделать, чтобы при совпадении "весь кубик стоит" сдвинуть его весь на пиксель, но заколебался
        {
          deltaValue = razmerY;
          kudaVse = (random8(2)) ? 1 : -1;
          if (deltaHue2 - kudaVse < 0 || deltaHue2 - kudaVse + poleY >= (int)HEIGHT)
            kudaVse = 0 - kudaVse;
        }
*/
        if (deltaValue == razmerY) // значит полюбому kudaVse было = (-1, 0, +1) - и для нуля в том числе мы двигаем весь куб на 1 пиксель
        {
          globalShiftY = 1 - kudaVse; //временно на единичку больше, чем надо
          for (uint8_t i = 0U; i < shtukX; i++)
            if (noise3d[0][i][1] == 1U) // если ячейка никуда не планировала двигаться
            {
              noise3d[0][i][1] = globalShiftY;
              noise3d[0][i][0] = 1U; // в нулевой ячейке храним количество ходов сдвига
            }
            else
              noise3d[0][i][0] = deltaValue; // в нулевой ячейке храним количество ходов сдвига
          globalShiftY--;
        }
        else
        {
          x = 0;
          for (uint8_t i = 0U; i < shtukX; i++)
            if (noise3d[0][i][1] != 1U)
              {
                y = random8(shtukY);
                if (y > x)
                  x = y;
                noise3d[0][i][0] = deltaValue * (x + 1U); // в нулевой ячейке храним количество ходов сдвига
              }  
          deltaValue = deltaValue * (x + 1U);
        }      
              
      }
      else // идём по вертикали, крутим по горизонтали (строки двигаются)
      {
        for (uint8_t j = 0U; j < shtukY; j++)
        {
          noise3d[0][1][j] = random8(3);
          shift = noise3d[0][1][j] - 1; // в первой ячейке храним направление прокрутки
          if (kudaVse == 0)
            kudaVse = shift;
          else if (shift != 0 && kudaVse != shift)
            kudaVse = 50;
        }
        if (seamlessX)
          deltaValue = razmerX + ((kudaVse < 50) ? random8(2U) : 1U);
        else  
          deltaValue = razmerX + ((deltaHue - kudaVse >= 0 && deltaHue - kudaVse + poleX < (int)WIDTH) ? random8(2U) : 1U);
        
/*        if (kudaVse == 0) // пытался сделать, чтобы при совпадении "весь кубик стоит" сдвинуть его весь на пиксель, но заколебался
        {
          deltaValue = razmerX;
          kudaVse = (random8(2)) ? 1 : -1;
          if (deltaHue - kudaVse < 0 || deltaHue - kudaVse + poleX >= (int)WIDTH)
            kudaVse = 0 - kudaVse;
        }
*/          
        if (deltaValue == razmerX) // значит полюбому kudaVse было = (-1, 0, +1) - и для нуля в том числе мы двигаем весь куб на 1 пиксель
        {
          globalShiftX = 1 - kudaVse; //временно на единичку больше, чем надо
          for (uint8_t j = 0U; j < shtukY; j++)
            if (noise3d[0][1][j] == 1U) // если ячейка никуда не планировала двигаться
            {
              noise3d[0][1][j] = globalShiftX;
              noise3d[0][0][j] = 1U; // в нулевой ячейке храним количество ходов сдвига
            }
            else
              noise3d[0][0][j] = deltaValue; // в нулевой ячейке храним количество ходов сдвига
          globalShiftX--;
        }
        else
        {
          y = 0;
          for (uint8_t j = 0U; j < shtukY; j++)
            if (noise3d[0][1][j] != 1U)
              {
                x = random8(shtukX);
                if (x > y)
                  y = x;
                noise3d[0][0][j] = deltaValue * (x + 1U); // в нулевой ячейке храним количество ходов сдвига
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
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        uint8_t tmp = random8(9U);
        setModeSettings(isColored ? 1U+tmp*tmp : (random8(10U) ? 1U+random8(99U) : 100U), 145U+random8(56U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    hue2 = 0U;
  }
//if (modes[currentMode].Brightness & 0x01) // для проверки движения источника дыма можно включить
  dimAll(254U);//(255U - modes[currentMode].Scale * 2);
//else     ledsClear(); // esphome: FastLED.clear();

  deltaHue++;
  CRGB color;//, color2;
  if (isColored)
  {
    if (hue2 == modes[currentMode].Scale)
      {
        hue2 = 0U;
        hue = random8();
      }
    if (deltaHue & 0x01)//((deltaHue >> 2U) == 0U) // какой-то умножитель охота подключить к задержке смены цвета, но хз какой...
      hue2++;

    //color = CHSV(hue, 255U, 255U);
    hsv2rgb_spectrum(CHSV(hue, 255U, 127U), color);
    //hsv2rgb_spectrum(CHSV(hue, 255U, 88U), color2);
  }
  else {
    //color = CHSV((modes[currentMode].Scale - 1U) * 2.6, (modes[currentMode].Scale > 98U) ? 0U : 255U, 255U);
    hsv2rgb_spectrum(CHSV((modes[currentMode].Scale - 1U) * 2.6, (modes[currentMode].Scale > 98U) ? 0U : 255U, 127U), color);
    //hsv2rgb_spectrum(CHSV((modes[currentMode].Scale - 1U) * 2.6, (modes[currentMode].Scale > 98U) ? 0U : 255U,  88U), color2);
  }

  //deltaHue2--;
  if (random8(WIDTH) != 0U) // встречная спираль движется не всегда синхронно основной
    deltaHue2--;

  for (uint8_t y = 0; y < HEIGHT; y++) {
    leds[XY((deltaHue  + y + 1U)%WIDTH, HEIGHT - 1U - y)] += color;
    leds[XY((deltaHue  + y     )%WIDTH, HEIGHT - 1U - y)] += color; //color2
    leds[XY((deltaHue2 + y     )%WIDTH,               y)] += color;
    leds[XY((deltaHue2 + y + 1U)%WIDTH,               y)] += color; //color2
  }

//if (modes[currentMode].Brightness & 0x01) { // для проверки движения источника дыма можно включить эту опцию
  // Noise
  
  // скорость движения по массиву noise
  //uint32_t mult = 500U * ((modes[currentMode].Scale - 1U) % 10U);
  noise32_x[0] += 1500;//1000;
  noise32_y[0] += 1500;//1000;
  noise32_z[0] += 1500;//1000;

  // хрен знает что
  //mult = 1000U * ((modes[currentMode].Speed - 1U) % 10U);
  scale32_x[0] = 4000;
  scale32_y[0] = 4000;
  FillNoise(0);
  //MoveX(3);
  //MoveY(3);

  // допустимый отлёт зажжённого пикселя от изначально присвоенного местоположения (от 0 до указанного значения. дробное) 
  //mult = (modes[currentMode].Brightness - 1U) % 10U;
  MoveFractionalNoiseX(3);//4
  MoveFractionalNoiseY(3);//4

  blurScreen(20); // без размытия как-то пиксельно, наверное...  
//} endif (modes[currentMode].Brightness & 0x01)
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
  if (loadingFlag)
  {
    loadingFlag = false;
    // setCurrentPalette();    
    // ledsClear(); // esphome: FastLED.clear();
    // not for 3in1
    // enlargedObjectNUM = (modes[currentMode].Scale - 1U) / 99.0 * (enlargedOBJECT_MAX_COUNT - 1U) + 1U;
    // enlargedObjectNUM = (modes[currentMode].Scale - 1U) % 11U / 10.0 * (enlargedOBJECT_MAX_COUNT - 1U) + 1U;
    if (enlargedObjectNUM > enlargedOBJECT_MAX_COUNT) enlargedObjectNUM = enlargedOBJECT_MAX_COUNT;
    if (enlargedObjectNUM < 2U) enlargedObjectNUM = 2U;

    double minSpeed = 0.2, maxSpeed = 0.8;
    
    for (uint8_t i = 0 ; i < enlargedObjectNUM ; i++) { 
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = random8(HEIGHT);

      //curr->color = CHSV(random(1U, 255U), 255U, 255U);
      trackingObjectHue[i] = random8();

      trackingObjectSpeedY[i] = +((-maxSpeed / 3) + (maxSpeed * (float)random8(1, 100) / 100));
      trackingObjectSpeedY[i] += trackingObjectSpeedY[i] > 0 ? minSpeed : -minSpeed;

      trackingObjectShift[i] = +((-maxSpeed / 2) + (maxSpeed * (float)random8(1, 100) / 100));
      trackingObjectShift[i] += trackingObjectShift[i] > 0 ? minSpeed : -minSpeed;

      trackingObjectState[i] = trackingObjectHue[i];

    }
  }
  for (uint8_t i = 0 ; i < enlargedObjectNUM ; i++) {
      if (reset) {
        trackingObjectState[i] = random8();
        trackingObjectSpeedX[i] = (trackingObjectState[i] - trackingObjectHue[i]) / 25;
      }
      if (trackingObjectState[i] != trackingObjectHue[i] && trackingObjectSpeedX[i]) {
        trackingObjectHue[i] += trackingObjectSpeedX[i];
      }
  }

}

static void PicassoPosition(){
  for (uint8_t i = 0 ; i < enlargedObjectNUM ; i++) { 
    if (trackingObjectPosX[i] + trackingObjectSpeedY[i] > WIDTH || trackingObjectPosX[i] + trackingObjectSpeedY[i] < 0) {
      trackingObjectSpeedY[i] = -trackingObjectSpeedY[i];
    }

    if (trackingObjectPosY[i] + trackingObjectShift[i] > HEIGHT || trackingObjectPosY[i] + trackingObjectShift[i] < 0) {
      trackingObjectShift[i] = -trackingObjectShift[i];
    }

    trackingObjectPosX[i] += trackingObjectSpeedY[i];
    trackingObjectPosY[i] += trackingObjectShift[i];
  };
}

static void PicassoRoutine(){
  #if false // defined(singleRANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      setModeSettings(17U+random8(64U) , 190U+random8(41U));
    }
  #endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

  PicassoGenerate(false);
  PicassoPosition();

  for (uint8_t i = 0 ; i < enlargedObjectNUM - 2U ; i+=2) 
    DrawLine(trackingObjectPosX[i], trackingObjectPosY[i], trackingObjectPosX[i+1U], trackingObjectPosY[i+1U], CHSV(trackingObjectHue[i], 255U, 255U));
    // DrawLine(trackingObjectPosX[i], trackingObjectPosY[i], trackingObjectPosX[i+1U], trackingObjectPosY[i+1U], ColorFromPalette(*curPalette, trackingObjectHue[i]));

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

  for (uint8_t i = 0 ; i < enlargedObjectNUM - 1U ; i++) 
    DrawLineF(trackingObjectPosX[i], trackingObjectPosY[i], trackingObjectPosX[i+1U], trackingObjectPosY[i+1U], CHSV(trackingObjectHue[i], 255U, 255U));
    // DrawLineF(trackingObjectPosX[i], trackingObjectPosY[i], trackingObjectPosX[i+1U], trackingObjectPosY[i+1U], ColorFromPalette(*curPalette, trackingObjectHue[i]));

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

  for (uint8_t i = 0 ; i < enlargedObjectNUM - 2U ; i+=2) 
    drawCircleF(fabs(trackingObjectPosX[i] - trackingObjectPosX[i+1U]), fabs(trackingObjectPosY[i] - trackingObjectPosX[i+1U]), fabs(trackingObjectPosX[i] - trackingObjectPosY[i]), CHSV(trackingObjectHue[i], 255U, 255U));
    // drawCircleF(fabs(trackingObjectPosX[i] - trackingObjectPosX[i+1U]), fabs(trackingObjectPosY[i] - trackingObjectPosX[i+1U]), fabs(trackingObjectPosX[i] - trackingObjectPosY[i]), ColorFromPalette(*curPalette, trackingObjectHue[i]));
    
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
      enlargedObjectNUM = (modes[currentMode].Scale - 1U) / 32.0 * (enlargedOBJECT_MAX_COUNT - 3U) + 3U;
    else if (modes[currentMode].Scale >= 68U)      // если масштаб больше 67
      enlargedObjectNUM = (modes[currentMode].Scale - 68U) / 32.0 * (enlargedOBJECT_MAX_COUNT - 3U) + 3U;
    else                                           // для масштабов посередине
      enlargedObjectNUM = (modes[currentMode].Scale - 34U) / 33.0 * (enlargedOBJECT_MAX_COUNT - 1U) + 1U;
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

//Leaper leapers[20];
//вместо класса Leaper будем повторно использовать переменные из эффекта мячики и мотыльки
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
  trackingObjectSpeedX[l] = (1 * (float)random8(1, 100) / 100);
  trackingObjectSpeedY[l] = (2 * (float)random8(1, 100) / 100);

  // for variety, sometimes go 50% faster
  if (random8() < 12) {
    trackingObjectSpeedX[l] += trackingObjectSpeedX[l] * 0.5;
    trackingObjectSpeedY[l] += trackingObjectSpeedY[l] * 0.5;
  }

  // leap towards the centre of the screen
  if (trackingObjectPosX[l] > (WIDTH / 2)) {
    trackingObjectSpeedX[l] = -trackingObjectSpeedX[l];
  }
}

static void LeapersMove_leaper(uint8_t l) {
#define GRAVITY            0.06
#define SETTLED_THRESHOLD  0.1
#define WALL_FRICTION      0.95
#define WIND               0.95    // wind resistance

  trackingObjectPosX[l] += trackingObjectSpeedX[l];
  trackingObjectPosY[l] += trackingObjectSpeedY[l];

  // bounce off the floor and ceiling?
  if (trackingObjectPosY[l] < 0 || trackingObjectPosY[l] > HEIGHT - 1) {
    trackingObjectSpeedY[l] = (-trackingObjectSpeedY[l] * WALL_FRICTION);
    trackingObjectSpeedX[l] = ( trackingObjectSpeedX[l] * WALL_FRICTION);
    trackingObjectPosY[l] += trackingObjectSpeedY[l];
    if (trackingObjectPosY[l] < 0) 
      trackingObjectPosY[l] = 0; // settled on the floor?
    if (trackingObjectPosY[l] <= SETTLED_THRESHOLD && fabs(trackingObjectSpeedY[l]) <= SETTLED_THRESHOLD) {
      LeapersRestart_leaper(l);
    }
  }

  // bounce off the sides of the screen?
  if (trackingObjectPosX[l] <= 0 || trackingObjectPosX[l] >= WIDTH - 1) {
    trackingObjectSpeedX[l] = (-trackingObjectSpeedX[l] * WALL_FRICTION);
    if (trackingObjectPosX[l] <= 0) {
      //trackingObjectPosX[l] = trackingObjectSpeedX[l]; // the bug?
      trackingObjectPosX[l] = -trackingObjectPosX[l];
    } else {
      //trackingObjectPosX[l] = WIDTH - 1 - trackingObjectSpeedX[l]; // the bug?
      trackingObjectPosX[l] = WIDTH + WIDTH - 2 - trackingObjectPosX[l];
    }
  }

  trackingObjectSpeedY[l] -= GRAVITY;
  trackingObjectSpeedX[l] *= WIND;
  trackingObjectSpeedY[l] *= WIND;
}


static void LeapersRoutine(){
  //unsigned num = map(scale, 0U, 255U, 6U, sizeof(boids) / sizeof(*boids));
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(random8(8U)*11U+5U+random8(7U) , 185U+random8(56U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    setCurrentPalette();    

    //ledsClear(); // esphome: FastLED.clear();
    //enlargedObjectNUM = (modes[currentMode].Scale - 1U) / 99.0 * (enlargedOBJECT_MAX_COUNT - 1U) + 1U;
    enlargedObjectNUM = (modes[currentMode].Scale - 1U) % 11U / 10.0 * (enlargedOBJECT_MAX_COUNT - 1U) + 1U;
    if (enlargedObjectNUM > enlargedOBJECT_MAX_COUNT) enlargedObjectNUM = enlargedOBJECT_MAX_COUNT;
    //if (enlargedObjectNUM < 2U) enlargedObjectNUM = 2U;

    for (uint8_t i = 0 ; i < enlargedObjectNUM ; i++) {
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = random8(HEIGHT);

      //curr->color = CHSV(random(1U, 255U), 255U, 255U);
      trackingObjectHue[i] = random8();
    }
  }

  //myLamp.dimAll(0); накой хрен делать затухание на 100%?
  ledsClear(); // esphome: FastLED.clear();

  for (uint8_t i = 0; i < enlargedObjectNUM; i++) {
    LeapersMove_leaper(i);
    //drawPixelXYF(trackingObjectPosX[i], trackingObjectPosY[i], CHSV(trackingObjectHue[i], 255U, 255U));
    drawPixelXYF(trackingObjectPosX[i], trackingObjectPosY[i], ColorFromPalette(*curPalette, trackingObjectHue[i]));
  };

  blurScreen(20);
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
  //trackingObjectSpeedY[l] = (float)random8(1, 11) / 10.0; // скорость пузырей 10 градаций?
  trackingObjectSpeedY[l] = (float)random8(5, 11) / (257U - modes[currentMode].Speed) / 4.0; // если скорость кадров фиксированная
}

static void drawBlob(uint8_t l, CRGB color) { //раз круги нарисовать не получается, будем попиксельно вырисовывать 2 варианта пузырей
  if (trackingObjectShift[l] == 2)
  {
    for (int8_t x = -2; x < 3; x++)
      for (int8_t y = -2; y < 3; y++)
        if (abs(x)+abs(y) < 4)
          drawPixelXYF(fmod(trackingObjectPosX[l]+x +WIDTH,WIDTH), trackingObjectPosY[l]+y, color);
  }
  else
  {
    for (int8_t x = -1; x < 3; x++)
      for (int8_t y = -1; y < 3; y++)
        if (!(x==-1 && (y==-1 || y==2) || x==2 && (y==-1 || y==2)))
          drawPixelXYF(fmod(trackingObjectPosX[l]+x +WIDTH,WIDTH), trackingObjectPosY[l]+y, color);
  }
}

static void LavaLampRoutine(){
  //unsigned num = map(scale, 0U, 255U, 6U, sizeof(boids) / sizeof(*boids));
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings)
    {
      setModeSettings(random8(30U) ? (random8(3U) ? 2U+random8(98U) : 1U) : 100U, 50U+random8(196U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    
    loadingFlag = false;

    enlargedObjectNUM = (WIDTH / 2) -  ((WIDTH - 1) & 0x01);
    uint8_t shift = random8(2);
    for (uint8_t i = 0; i < enlargedObjectNUM; i++) {
      trackingObjectPosY[i] = 0;
      trackingObjectPosX[i] = i * 2U + shift;
      LavaLampGetspeed(i);
      trackingObjectShift[i] = random8(1,3);             // присваивается случайный целочисленный радиус пузырям от 1 до 2
    }
    if (modes[currentMode].Scale != 1U)
      hue = modes[currentMode].Scale * 2.57;
  }

  if (modes[currentMode].Scale == 1U)
  {
    hue2++;
    if (hue2 % 0x10 == 0U)
      hue++;
  }
  CRGB color = CHSV(hue, (modes[currentMode].Scale < 100U) ? 255U : 0U, 255U);
 
  ledsClear(); // esphome: FastLED.clear();

  for (uint8_t i = 0; i < enlargedObjectNUM; i++) {      //двигаем по аналогии с https://jiwonk.im/lavalamp/
    if (trackingObjectPosY[i] + trackingObjectShift[i] >= HEIGHT - 1)
       trackingObjectPosY[i] += (trackingObjectSpeedY[i] * ((HEIGHT - 1 - trackingObjectPosY[i]) / trackingObjectShift[i] + 0.005));
    else if (trackingObjectPosY[i] - trackingObjectShift[i] <= 0)
       trackingObjectPosY[i] += (trackingObjectSpeedY[i] * (trackingObjectPosY[i] / trackingObjectShift[i] + 0.005));
    else
       trackingObjectPosY[i] += trackingObjectSpeedY[i];

    // bounce off the floor and ceiling?
    if (trackingObjectPosY[i] < 0.01){                   // почему-то при нуле появляется мерцание (один кадр, еле заметно)
      LavaLampGetspeed(i);
      trackingObjectPosY[i] = 0.01;
    }
    else if (trackingObjectPosY[i] > HEIGHT - 1.01){     // тоже на всякий пожарный
      LavaLampGetspeed(i);
      trackingObjectSpeedY[i] = -trackingObjectSpeedY[i];
      trackingObjectPosY[i] = HEIGHT - 1.01;
    }
  
    drawBlob(i, color);                                  // рисуем попиксельно 2 размера пузырей
  };

  blurScreen(20);
}
#endif


#ifdef DEF_SHADOWS
// ********************** SHADOWS ***********************
// https://github.com/vvip-68/GyverPanelWiFi/blob/master/firmware/GyverPanelWiFi_v1.04/effects.ino
// (c) vvip-68
static void shadowsRoutine() {
  #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
  if (selectedSettings)
  {
    setModeSettings(1U, 1U+random8(255U));
  }
  #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;
 
  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(map(modes[currentMode].Speed, 1, 255, 100, 255), 32, map(modes[currentMode].Speed, 1, 255, 60, 255));

  uint16_t hue16 = sHue16;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);
  
  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  
  byte effectBrightness = modes[currentMode].Scale * 2.55;

  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;

  for( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);
    
    CRGB newcolor = CHSV( hue8, sat8, map8(bri8, map(effectBrightness, 32, 255, 32,125), map(effectBrightness, 32,255, 125,250))); 
    
    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS-1) - pixelnumber;
    
    nblend( leds[pixelnumber], newcolor, 64);
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

static void wu_pixel(uint32_t x, uint32_t y, CRGB * col) {      //awesome wu_pixel procedure by reddit u/sutaburosu
  // extract the fractional parts and derive their inverses
  uint8_t xx = x & 0xff, yy = y & 0xff, ix = 255 - xx, iy = 255 - yy;
  // calculate the intensities for each affected pixel
  uint8_t wu[4] = {WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy),
                   WU_WEIGHT(ix, yy), WU_WEIGHT(xx, yy)};
  // multiply the intensities by the colour, and saturating-add them to the pixels
  for (uint8_t i = 0; i < 4; i++) {
    uint16_t xy = XY((x >> 8) + (i & 1), (y >> 8) + ((i >> 1) & 1));
    if (xy < NUM_LEDS){
      leds[xy].r = qadd8(leds[xy].r, col->r * wu[i] >> 8);
      leds[xy].g = qadd8(leds[xy].g, col->g * wu[i] >> 8);
      leds[xy].b = qadd8(leds[xy].b, col->b * wu[i] >> 8);
    }
  }
}

static void DNARoutine()
{
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(1U+random8(100U), 1U+random8(200U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    loadingFlag = false;
    step = map8(modes[currentMode].Speed, 10U, 60U);
    hue = modes[currentMode].Scale;
    deltaHue = hue > 50U;
    if (deltaHue)
      hue = 101U - hue;
    hue = 255U - map( 51U - hue, 1U, 50U, 0, 255U);
  }
  double freq = 3000;
  float mn =255.0/13.8;
  
  fadeToBlackBy(leds, NUM_LEDS, step);
  uint16_t ms = millis();
  
if (deltaHue)
  for (uint8_t i = 0; i < WIDTH; i++)
  {
    uint32_t x = beatsin16(step, 0, (HEIGHT - 1) * 256, 0, i * freq);
    uint32_t y = i * 256;
    uint32_t x1 = beatsin16(step, 0, (HEIGHT - 1) * 256, 0, i * freq + 32768);

    CRGB col = CHSV(ms / 29 + i * 255 / (WIDTH - 1), 255, qadd8(hue, beatsin8(step, 60, 255U, 0, i * mn)));
    CRGB col1 = CHSV(ms / 29 + i * 255 / (WIDTH - 1) + 128, 255, qadd8(hue, beatsin8(step, 60, 255U, 0, i * mn + 128)));
    wu_pixel (y , x, &col);
    wu_pixel (y , x1, &col1);
  }
else
  for (uint8_t i = 0; i < HEIGHT; i++)
  {
    uint32_t x = beatsin16(step, 0, (WIDTH - 1) * 256, 0, i * freq);
    uint32_t y = i * 256;
    uint32_t x1 = beatsin16(step, 0, (WIDTH - 1) * 256, 0, i * freq + 32768);

    CRGB col = CHSV(ms / 29 + i * 255 / (HEIGHT - 1), 255, qadd8(hue, beatsin8(step, 60, 255U, 0, i * mn)));
    CRGB col1 = CHSV(ms / 29 + i * 255 / (HEIGHT - 1) + 128, 255, qadd8(hue, beatsin8(step, 60, 255U, 0, i * mn + 128)));
    wu_pixel (x , y, &col);
    wu_pixel (x1 , y, &col1);
  }

  blurScreen(16);
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

static void snakesRoutine(){
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        uint8_t tmp = random8(8U);
        setModeSettings(8U+tmp*tmp, 20U+random8(120U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    speedfactor = (float)modes[currentMode].Speed / 555.0f + 0.001f;
    
    enlargedObjectNUM = (modes[currentMode].Scale - 1U) / 99.0 * (enlargedOBJECT_MAX_COUNT - 1U) + 1U;
    if (enlargedObjectNUM > enlargedOBJECT_MAX_COUNT) enlargedObjectNUM = enlargedOBJECT_MAX_COUNT;
    for (uint8_t i = 0; i < enlargedObjectNUM; i++){
      enlargedObjectTime[i] = 0;
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = random8(HEIGHT);
      trackingObjectSpeedX[i] = (255. + random8()) / 255.;
      trackingObjectSpeedY[i] = 0;
      //trackingObjectShift[i] = 0;
      trackingObjectHue[i] = random8();
      trackingObjectState[i] = random8(4); //     B00           направление головы змейки
                                           // B10     B11
                                           //     B01
    }
    
  }
  //hue++;
  //dimAll(220);
  ledsClear(); // esphome: FastLED.clear();

  int8_t dx, dy;
  for (uint8_t i = 0; i < enlargedObjectNUM; i++){
   trackingObjectSpeedY[i] += trackingObjectSpeedX[i] * speedfactor;
   if (trackingObjectSpeedY[i] >= 1)
   {
    trackingObjectSpeedY[i] = trackingObjectSpeedY[i] - (int)trackingObjectSpeedY[i];
    if (random8(9U) == 0U) // вероятность поворота
      if (random8(2U)){ // <- поворот налево
        enlargedObjectTime[i] = (enlargedObjectTime[i] << 2) | B01; // младший бит = поворот
        switch (trackingObjectState[i]) {
          case B10:
            trackingObjectState[i] = B01;
            if (trackingObjectPosY[i] == 0U)
              trackingObjectPosY[i] = HEIGHT - 1U;
            else
              trackingObjectPosY[i]--;
            break;
          case B11:
            trackingObjectState[i] = B00;
            if (trackingObjectPosY[i] >= HEIGHT - 1U)
              trackingObjectPosY[i] = 0U;
            else
              trackingObjectPosY[i]++;
            break;
          case B00:
            trackingObjectState[i] = B10;
            if (trackingObjectPosX[i] == 0U)
              trackingObjectPosX[i] = WIDTH - 1U;
            else
              trackingObjectPosX[i]--;
            break;
          case B01:
            trackingObjectState[i] = B11;
            if (trackingObjectPosX[i] >= WIDTH - 1U)
              trackingObjectPosX[i] = 0U;
            else
              trackingObjectPosX[i]++;
            break;
        }
      }
      else{ // -> поворот направо
        enlargedObjectTime[i] = (enlargedObjectTime[i] << 2) | B11; // младший бит = поворот, старший = направо
        switch (trackingObjectState[i]) {
          case B11:
            trackingObjectState[i] = B01;
            if (trackingObjectPosY[i] == 0U)
              trackingObjectPosY[i] = HEIGHT - 1U;
            else
              trackingObjectPosY[i]--;
            break;
          case B10:
            trackingObjectState[i] = B00;
            if (trackingObjectPosY[i] >= HEIGHT - 1U)
              trackingObjectPosY[i] = 0U;
            else
              trackingObjectPosY[i]++;
            break;
          case B01:
            trackingObjectState[i] = B10;
            if (trackingObjectPosX[i] == 0U)
              trackingObjectPosX[i] = WIDTH - 1U;
            else
              trackingObjectPosX[i]--;
            break;
          case B00:
            trackingObjectState[i] = B11;
            if (trackingObjectPosX[i] >= WIDTH - 1U)
              trackingObjectPosX[i] = 0U;
            else
              trackingObjectPosX[i]++;
            break;
        }
      }
    else { // двигаем без поворота
        enlargedObjectTime[i] = (enlargedObjectTime[i] << 2);
        switch (trackingObjectState[i]) {
          case B01:
            if (trackingObjectPosY[i] == 0U)
              trackingObjectPosY[i] = HEIGHT - 1U;
            else
              trackingObjectPosY[i]--;
            break;
          case B00:
            if (trackingObjectPosY[i] >= HEIGHT - 1U)
              trackingObjectPosY[i] = 0U;
            else
              trackingObjectPosY[i]++;
            break;
          case B10:
            if (trackingObjectPosX[i] == 0U)
              trackingObjectPosX[i] = WIDTH - 1U;
            else
              trackingObjectPosX[i]--;
            break;
          case B11:
            if (trackingObjectPosX[i] >= WIDTH - 1U)
              trackingObjectPosX[i] = 0U;
            else
              trackingObjectPosX[i]++;
            break;
        }
    }
   }
   
    switch (trackingObjectState[i]) {
     case B01:
       dy = 1;
       dx = 0;
       break;
     case B00:
       dy = -1;
       dx = 0;
       break;
     case B10:
       dy = 0;
       dx = 1;
       break;
     case B11:
       dy = 0;
       dx = -1;
       break;
    }
    long temp = enlargedObjectTime[i];
    uint8_t x = trackingObjectPosX[i];
    uint8_t y = trackingObjectPosY[i];
    //CHSV color = CHSV(trackingObjectHue[i], 255U, 255U);
    //drawPixelXY(x, y, color);
    //drawPixelXYF(x, y, CHSV(trackingObjectHue[i], 255U, trackingObjectSpeedY[i] * 255)); // тут рисуется голова // слишком сложно для простого сложения цветов
    leds[XY(x,y)] += CHSV(trackingObjectHue[i], 255U, trackingObjectSpeedY[i] * 255); // тут рисуется голова

    for (uint8_t m = 0; m < SNAKES_LENGTH; m++){ // 16 бит распаковываем, 14 ещё остаётся без дела в запасе, 2 на хвостик
      x = (WIDTH + x + dx) % WIDTH;
      y = (HEIGHT + y + dy) % HEIGHT;
      //drawPixelXYF(x, y, CHSV(trackingObjectHue[i] + m*4U, 255U, 255U)); // тут рисуется тело // слишком сложно для простого сложения цветов
      //leds[XY(x,y)] += CHSV(trackingObjectHue[i] + m*4U, 255U, 255U); // тут рисуется тело
      leds[XY(x,y)] += CHSV(trackingObjectHue[i] + (m + trackingObjectSpeedY[i])*4U, 255U, 255U); // тут рисуется тело
 
      if (temp & B01){ // младший бит = поворот, старший = направо
        temp = temp >> 1;
        if (temp & B01){ // старший бит = направо
          if (dx == 0){
            dx = 0 - dy;
            dy = 0;
          }
          else{
            dy = dx;
            dx = 0;
          }
        }
        else{ // иначе налево
          if (dx == 0){
            dx = dy;
            dy = 0;
          }
          else{
            dy = 0 - dx;
            dx = 0;
          }
        }
        temp = temp >> 1;
      }
      else { // если без поворота
        temp = temp >> 2;
      }
    }
    x = (WIDTH + x + dx) % WIDTH;
    y = (HEIGHT + y + dy) % HEIGHT;
    //drawPixelXYF(x, y, CHSV(trackingObjectHue[i] + SNAKES_LENGTH*4U, 255U, (1 - trackingObjectSpeedY[i]) * 255)); // хвостик // слишком сложно для простого сложения цветов
    //leds[XY(x,y)] += CHSV(trackingObjectHue[i] + SNAKES_LENGTH*4U, 255U, (1 - trackingObjectSpeedY[i]) * 255); // хвостик
    leds[XY(x,y)] += CHSV(trackingObjectHue[i] + (SNAKES_LENGTH + trackingObjectSpeedY[i])*4U, 255U, (1 - trackingObjectSpeedY[i]) * 255); // хвостик
  }
}
#endif


#if defined(DEF_LIQUIDLAMP) || defined(DEF_LIQUIDLAMP_AUTO)
// ----------------------------- Жидкая лампа ---------------------
// ----------- Эффект "Лавовая лампа" (c) obliterator
// https://github.com/DmytroKorniienko/FireLamp_JeeUI/commit/9bad25adc2c917fbf3dfa97f4c498769aaf76ebe
// с генератором палитр by SottNick

//аналог ардуино функции map(), но только для float
static float fmap(const float x, const float in_min, const float in_max, const float out_min, const float out_max){
        return (out_max - out_min) * (x - in_min) / (in_max - in_min) + out_min;
    }
static float mapcurve(const float x, const float in_min, const float in_max, const float out_min, const float out_max, float (*curve)(float,float,float,float)){
        if (x <= in_min) return out_min;
        if (x >= in_max) return out_max;
        return curve((x - in_min), out_min, (out_max - out_min), (in_max - in_min));
    }
static float InQuad(float t, float b, float c, float d) { t /= d; return c * t * t + b; }    
static float OutQuart(float t, float b, float c, float d) { t = t / d - 1; return -c * (t * t * t * t - 1) + b; }
static float InOutQuad(float t, float b, float c, float d) {
        t /= d / 2;
        if (t < 1) return c / 2 * t * t + b;
        --t;
        return -c / 2 * (t * (t - 2) - 1) + b;
    }

static unsigned MASS_MIN = 10;
static unsigned MASS_MAX = 50;

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

static void LiquidLampPosition(){
  //bool physic_on = modes[currentMode].Speed & 0x01;
  for (uint8_t i = 0; i < enlargedObjectNUM; i++) {
    liquidLampHot[i] += mapcurve(trackingObjectPosY[i], 0, HEIGHT-1, 5, -5, InOutQuad) * speedfactor;

    float heat = (liquidLampHot[i] / trackingObjectState[i]) - 1;
    if (heat > 0 && trackingObjectPosY[i] < HEIGHT-1) {
      trackingObjectSpeedY[i] += heat * liquidLampSpf[i];
    }
    if (trackingObjectPosY[i] > 0) {
      trackingObjectSpeedY[i] -= 0.07;
    }

    if (trackingObjectSpeedY[i]) trackingObjectSpeedY[i] *= 0.85;
    trackingObjectPosY[i] += trackingObjectSpeedY[i] * speedfactor;

    //if (physic_on) {
      if (trackingObjectSpeedX[i]) trackingObjectSpeedX[i] *= 0.7;
      trackingObjectPosX[i] += trackingObjectSpeedX[i] * speedfactor;
    //}

    if (trackingObjectPosX[i] > WIDTH-1) trackingObjectPosX[i] -= WIDTH-1;
    if (trackingObjectPosX[i] < 0) trackingObjectPosX[i] += WIDTH-1;
    if (trackingObjectPosY[i] > HEIGHT-1) trackingObjectPosY[i] = HEIGHT-1;
    if (trackingObjectPosY[i] < 0) trackingObjectPosY[i] = 0;
  };
}

static void LiquidLampPhysic(){
  for (uint8_t i = 0; i < enlargedObjectNUM; i++) {
    //Particle *p1 = (Particle *)&particles[i];
    // отключаем физику на границах, чтобы не слипались шары
    if (trackingObjectPosY[i] < 3 || trackingObjectPosY[i] > HEIGHT - 1) continue;
    for (uint8_t j = 0; j < enlargedObjectNUM; j++) {
      //Particle *p2 = (Particle *)&particles[j];
      if (trackingObjectPosY[j] < 3 || trackingObjectPosY[j] > HEIGHT - 1) continue;
      float radius = 3;//(trackingObjectShift[i] + trackingObjectShift[j]);
      if (trackingObjectPosX[i] + radius > trackingObjectPosX[j]
       && trackingObjectPosX[i] < radius + trackingObjectPosX[j]
       && trackingObjectPosY[i] + radius > trackingObjectPosY[j]
       && trackingObjectPosY[i] < radius + trackingObjectPosY[j]
      ){
          //float dist = EffectMath::distance(p1->position_x, p1->position_y, p2->position_x, p2->position_y);
          float dx =  min((float)fabs(trackingObjectPosX[i] - trackingObjectPosX[j]), (float)WIDTH + trackingObjectPosX[i] - trackingObjectPosX[j]); //по идее бесшовный икс
          float dy =  fabs(trackingObjectPosY[i] - trackingObjectPosY[j]);
          float dist = SQRT_VARIANT((dx * dx) + (dy * dy));
          
          if (dist <= radius) {
            float nx = (trackingObjectPosX[j] - trackingObjectPosX[i]) / dist;
            float ny = (trackingObjectPosY[j] - trackingObjectPosY[i]) / dist;
            float p = 2 * (trackingObjectSpeedX[i] * nx + trackingObjectSpeedY[i] * ny - trackingObjectSpeedX[j] * nx - trackingObjectSpeedY[j] * ny) / (trackingObjectState[i] + trackingObjectState[j]);
            float pnx = p * nx, pny = p * ny;
            trackingObjectSpeedX[i] = trackingObjectSpeedX[i] - pnx * trackingObjectState[i];
            trackingObjectSpeedY[i] = trackingObjectSpeedY[i] - pny * trackingObjectState[i];
            trackingObjectSpeedX[j] = trackingObjectSpeedX[j] + pnx * trackingObjectState[j];
            trackingObjectSpeedY[j] = trackingObjectSpeedY[j] + pny * trackingObjectState[j];
          }
        }
    }
  }
}


// генератор палитр для Жидкой лампы (c) SottNick
// static const uint8_t MBVioletColors_arr[5][4] PROGMEM = // та же палитра, но в формате CHSV

static CRGBPalette16 myPal;

static void fillMyPal16(uint8_t hue, bool isInvert = false){
  int8_t lastSlotUsed = -1;
  uint8_t istart8, iend8;
  CRGB rgbstart, rgbend;
  
  // начинаем с нуля
  if (isInvert)
    //с неявным преобразованием оттенков цвета получаются, как в фотошопе, но для данного эффекта не красиво выглядят
    //rgbstart = CHSV(256 + hue - pgm_read_byte(&MBVioletColors_arr[0][1]), pgm_read_byte(&MBVioletColors_arr[0][2]), pgm_read_byte(&MBVioletColors_arr[0][3])); // начальная строчка палитры с инверсией
    hsv2rgb_spectrum(CHSV(256 + hue - pgm_read_byte(&MBVioletColors_arr[0][1]), pgm_read_byte(&MBVioletColors_arr[0][2]), pgm_read_byte(&MBVioletColors_arr[0][3])), rgbstart);
  else
    //rgbstart = CHSV(hue + pgm_read_byte(&MBVioletColors_arr[0][1]), pgm_read_byte(&MBVioletColors_arr[0][2]), pgm_read_byte(&MBVioletColors_arr[0][3])); // начальная строчка палитры
    hsv2rgb_spectrum(CHSV(hue + pgm_read_byte(&MBVioletColors_arr[0][1]), pgm_read_byte(&MBVioletColors_arr[0][2]), pgm_read_byte(&MBVioletColors_arr[0][3])), rgbstart);
  int indexstart = 0; // начальный индекс палитры
  for (uint8_t i = 1U; i < 5U; i++) { // в палитре @obliterator всего 5 строчек
    int indexend = pgm_read_byte(&MBVioletColors_arr[i][0]);
    if (isInvert)
      //rgbend = CHSV(256 + hue - pgm_read_byte(&MBVioletColors_arr[i][1]), pgm_read_byte(&MBVioletColors_arr[i][2]), pgm_read_byte(&MBVioletColors_arr[i][3])); // следующая строчка палитры с инверсией
      hsv2rgb_spectrum(CHSV(256 + hue - pgm_read_byte(&MBVioletColors_arr[i][1]), pgm_read_byte(&MBVioletColors_arr[i][2]), pgm_read_byte(&MBVioletColors_arr[i][3])), rgbend);
    else
      //rgbend = CHSV(hue + pgm_read_byte(&MBVioletColors_arr[i][1]), pgm_read_byte(&MBVioletColors_arr[i][2]), pgm_read_byte(&MBVioletColors_arr[i][3])); // следующая строчка палитры
      hsv2rgb_spectrum(CHSV(hue + pgm_read_byte(&MBVioletColors_arr[i][1]), pgm_read_byte(&MBVioletColors_arr[i][2]), pgm_read_byte(&MBVioletColors_arr[i][3])), rgbend);
    istart8 = indexstart / 16;
    iend8   = indexend   / 16;
    if ((istart8 <= lastSlotUsed) && (lastSlotUsed < 15)) {
       istart8 = lastSlotUsed + 1;
       if (iend8 < istart8)
         iend8 = istart8;
    }
    lastSlotUsed = iend8;
    fill_gradient_RGB( myPal, istart8, rgbstart, iend8, rgbend);
    indexstart = indexend;
    rgbstart = rgbend;
  }
}

static void LiquidLampRoutine(bool isColored){
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        //1-9,31-38,46-48,93-99
        //1-17,28-38,44-48,89-99
        uint8_t tmp = random8(28U);
        if (tmp > 9U) tmp += 21U;
        if (tmp > 38U) tmp += 7U;
        if (tmp > 48U) tmp += 44U;
        setModeSettings(isColored ? tmp : 27U+random8(54U), 30U+random8(170U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    
    loadingFlag = false;
    //setCurrentPalette();    

    speedfactor = modes[currentMode].Speed / 64.0 + 0.1; // 127 БЫЛО

    //ledsClear(); // esphome: FastLED.clear();
    if (isColored){
      fillMyPal16((modes[currentMode].Scale - 1U) * 2.55, !(modes[currentMode].Scale & 0x01));
      enlargedObjectNUM = enlargedOBJECT_MAX_COUNT / 2U - 2U; //14U;
    }
    else{
      enlargedObjectNUM = (modes[currentMode].Scale - 1U) / 99.0 * (enlargedOBJECT_MAX_COUNT - 1U) + 1U;
      hue = random8();
      deltaHue = random8(2U);
      fillMyPal16(hue, deltaHue);

      //myPal = MBVioletColorsSimple1_gp; // лучшая палитра 1
      //fillMyPal16test((modes[currentMode].Scale - 1U) * 2.55, !(modes[currentMode].Scale & 0x01));
    }
    if (enlargedObjectNUM > enlargedOBJECT_MAX_COUNT) enlargedObjectNUM = enlargedOBJECT_MAX_COUNT;
    else if (enlargedObjectNUM < 2U) enlargedObjectNUM = 2U;

    double minSpeed = 0.2, maxSpeed = 0.8;
    
    for (uint8_t i = 0 ; i < enlargedObjectNUM ; i++) { 
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = 0; //random8(HEIGHT);
      trackingObjectState[i] = random(MASS_MIN, MASS_MAX);
      liquidLampSpf[i] = fmap(trackingObjectState[i], MASS_MIN, MASS_MAX, 0.0015, 0.0005);
      trackingObjectShift[i] = fmap(trackingObjectState[i], MASS_MIN, MASS_MAX, 2, 3);
      liquidLampMX[i] = map(trackingObjectState[i], MASS_MIN, MASS_MAX, 60, 80); // сила возмущения
      liquidLampSC[i] = map(trackingObjectState[i], MASS_MIN, MASS_MAX, 6, 10); // радиус возмущения
      liquidLampTR[i] = liquidLampSC[i]  * 2 / 3; // отсечка расчетов (оптимизация скорости)
    }

  }
  
  LiquidLampPosition();
  //bool physic_on = modes[currentMode].Speed & 0x01;
  //if (physic_on) 
  LiquidLampPhysic;

  if (!isColored){
    hue2++;
    if (hue2 % 0x10 == 0U){
      hue++;
      fillMyPal16(hue, deltaHue);
    }
  }

  for (uint8_t x = 0; x < WIDTH; x++) {
    for (uint8_t y = 0; y < HEIGHT; y++) {
      float sum = 0;
      //for (unsigned i = 0; i < numParticles; i++) {
      for (uint8_t i = 0; i < enlargedObjectNUM; i++) {
        //Particle *p1 = (Particle *)&particles[i];
        if (abs(x - trackingObjectPosX[i]) > liquidLampTR[i] || abs(y - trackingObjectPosY[i]) > liquidLampTR[i]) continue;
        //float d = EffectMath::distance(x, y, p1->position_x, p1->position_y);
        float dx =  min((float)fabs(trackingObjectPosX[i] - (float)x), (float)WIDTH + trackingObjectPosX[i] - (float)x); //по идее бесшовный икс
        float dy =  fabs(trackingObjectPosY[i] - (float)y);
        float d = SQRT_VARIANT((dx * dx) + (dy * dy));
        
        if (d < trackingObjectShift[i]) {
          sum += mapcurve(d, 0, trackingObjectShift[i], 255, liquidLampMX[i], InQuad);
        } 
        else if (d < liquidLampSC[i]){
          sum += mapcurve(d, trackingObjectShift[i], liquidLampSC[i], liquidLampMX[i], 0, OutQuart);
        }
        if (sum >= 255) { sum = 255; break; }
      }
      if (sum < 16) sum = 16;// отрезаем смазанный кусок палитры из-за отсутствия параметра NOBLEND
      CRGB color = ColorFromPalette(myPal, sum); // ,255, NOBLEND
      drawPixelXY(x, y, color);
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
  //deltaHue = !deltaHue; // "Мальчик" <> "Девочка"
  trackingObjectSpeedX[r] = (float)(random(-(WIDTH * HEIGHT + (WIDTH*2)), WIDTH*HEIGHT + (WIDTH*2))) / 256.0; // * (deltaHue ? 1 : -1); // Наклон. "Мальчики" налево, "девочки" направо. :)
  if ((trackingObjectPosX[r] < 0 && trackingObjectSpeedX[r] < 0) || (trackingObjectPosX[r] > (WIDTH-1) && trackingObjectSpeedX[r] > 0)) { // меняем направление только после выхода за пределы экрана
    // leap towards the centre of the screen
    trackingObjectSpeedX[r] = -trackingObjectSpeedX[r];
  }
  // controls the leap height
  trackingObjectSpeedY[r] = (float)(random8() * 8 + HEIGHT * 10) / 256.0;
  trackingObjectHue[r] = random8();
  trackingObjectPosX[r] = random8(WIDTH);
}

static void popcornRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(random8(9U)*11U+3U+random8(9U), 5U+random8(67U)*2U+(random8(4U)?0U:1U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    setCurrentPalette();

    speedfactor = fmap((float)modes[currentMode].Speed, 1., 255., 0.25, 1.0);
    //speedfactor = (float)modes[currentMode].Speed / 127.0f + 0.001f;

    enlargedObjectNUM = (modes[currentMode].Scale - 1U) % 11U / 10.0 * (enlargedOBJECT_MAX_COUNT - 1U) + 1U;
    if (enlargedObjectNUM > enlargedOBJECT_MAX_COUNT) enlargedObjectNUM = enlargedOBJECT_MAX_COUNT;
  
    for (uint8_t r = 0; r < enlargedObjectNUM; r++) {
      trackingObjectPosX[r] = random8(WIDTH);
      trackingObjectPosY[r] = random8(HEIGHT);
      trackingObjectSpeedX[r] = 0;
      trackingObjectSpeedY[r] = -1;
      trackingObjectHue[r] = random8();
    }
  }
  float popcornGravity = 0.1 * speedfactor;
  //if (modes[currentMode].Speed & 0x01) // теперь чётностью скорости определяется белый/цветной попкорн, а чётностью яркости больше ничего
    fadeToBlackBy(leds, NUM_LEDS, 60);
  //else ledsClear(); // esphome: FastLED.clear();// fadeToBlackBy(leds, NUM_LEDS, 250);

//void popcornMove(float popcornGravity) {
  for (uint8_t r = 0; r < enlargedObjectNUM; r++) {
    // add the X & Y velocities to the positions
    trackingObjectPosX[r] += trackingObjectSpeedX[r] ;
    if (trackingObjectPosX[r] > WIDTH - 1)
      trackingObjectPosX[r] = trackingObjectPosX[r] - (WIDTH - 1);
    if (trackingObjectPosX[r] < 0)
      trackingObjectPosX[r] = WIDTH - 1 + trackingObjectPosX[r];
    trackingObjectPosY[r] += trackingObjectSpeedY[r] * speedfactor;
    
    if (trackingObjectPosY[r] > HEIGHT - 1){
      trackingObjectPosY[r] = HEIGHT+HEIGHT - 2 - trackingObjectPosY[r];
      trackingObjectSpeedY[r] = -trackingObjectSpeedY[r];
    }  
    

    // bounce off the floor?
    if (trackingObjectPosY[r] < 0 && trackingObjectSpeedY[r] < -0.7) { // 0.7 вычислено в экселе. скорость свободного падения ниже этой не падает. если ниже, значит ещё есть ускорение
      trackingObjectSpeedY[r] = (-trackingObjectSpeedY[r]) * 0.9375;//* 240) >> 8;
      //trackingObjectPosY[r] = trackingObjectSpeedY[r]; чё это значило вообще?!
      trackingObjectPosY[r] = -trackingObjectPosY[r];
    }

    // settled on the floor?
    if (trackingObjectPosY[r] <= -1)
      popcornRestart_rocket(r);

    // bounce off the sides of the screen?
    /*if (rockets[r].x < 0 || rockets[r].x > (int)WIDTH * 256) {
      trackingObjectSpeedX[r] = (-trackingObjectSpeedX[r] * 248) >> 8;
      // force back onto the screen, otherwise they eventually sneak away
      if (rockets[r].x < 0) {
        rockets[r].x = trackingObjectSpeedX[r];
        trackingObjectSpeedY[r] += trackingObjectSpeedX[r];
      } else {
        rockets[r].x = (WIDTH * 256) - trackingObjectSpeedX[r];
      }
    }*/

    // popcornGravity
    trackingObjectSpeedY[r] -= popcornGravity;

    // viscosity
    trackingObjectSpeedX[r] *= 0.875;
    trackingObjectSpeedY[r] *= 0.875;

  
//void popcornPaint() {
    // make the acme gray, because why not
    if (-0.004 > trackingObjectSpeedY[r] and trackingObjectSpeedY[r] < 0.004)
      drawPixelXYF(trackingObjectPosX[r], trackingObjectPosY[r], (modes[currentMode].Speed & 0x01) ?
                ColorFromPalette(*curPalette, trackingObjectHue[r]) 
              : CRGB::Pink);
    else
      drawPixelXYF(trackingObjectPosX[r], trackingObjectPosY[r], (modes[currentMode].Speed & 0x01) ? 
                CRGB::Gray 
              : ColorFromPalette(*curPalette, trackingObjectHue[r]));
  }
}
#endif


#ifdef DEF_OSCILLATING
// ============= Эффект Реакция Белоусова-Жаботинского (Осциллятор) ===============
// по наводке https://www.wikiwand.com/ru/%D0%9A%D0%BB%D0%B5%D1%82%D0%BE%D1%87%D0%BD%D1%8B%D0%B9_%D0%B0%D0%B2%D1%82%D0%BE%D0%BC%D0%B0%D1%82
// (c) SottNick

static void drawPixelXYFseamless(float x, float y, CRGB color)
{
  uint8_t xx = (x - (int)x) * 255, yy = (y - (int)y) * 255, ix = 255 - xx, iy = 255 - yy;
  // calculate the intensities for each affected pixel
  uint8_t wu[4] = {WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy),
                   WU_WEIGHT(ix, yy), WU_WEIGHT(xx, yy)};
  // multiply the intensities by the colour, and saturating-add them to the pixels
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t xn = (int8_t)(x + (i & 1)) % WIDTH;
    uint8_t yn = (int8_t)(y + ((i >> 1) & 1)) % HEIGHT;
    CRGB clr = getPixColorXY(xn, yn);
    clr.r = qadd8(clr.r, (color.r * wu[i]) >> 8);
    clr.g = qadd8(clr.g, (color.g * wu[i]) >> 8);
    clr.b = qadd8(clr.b, (color.b * wu[i]) >> 8);
    drawPixelXY(xn, yn, clr);
  }
}
/*
class oscillatingCell {
public:
  byte red; // значения 0 или 1
  byte blue; // значения 0 или 1
  byte green; // значения 0 или 1
  byte color; // значения от 0 до 2
};
oscillatingCell oscillatingWorld[WIDTH][HEIGHT];

будем использовать вместо них всех имеющийся в прошивке массив
static uint8_t noise3d[2][WIDTH][HEIGHT]; 
*/

static uint8_t calcNeighbours(uint8_t x, uint8_t y, uint8_t n) {
  return (noise3d[0][(x + 1) % WIDTH][y] == n) +
         (noise3d[0][x][(y + 1) % HEIGHT] == n) +
         (noise3d[0][(x + WIDTH - 1) % WIDTH][y] == n) +
         (noise3d[0][x][(y + HEIGHT - 1) % HEIGHT] == n) +
         (noise3d[0][(x + 1) % WIDTH][(y + 1) % HEIGHT] == n) +
         (noise3d[0][(x + WIDTH - 1) % WIDTH][(y + 1) % HEIGHT] == n) +
         (noise3d[0][(x + WIDTH - 1) % WIDTH][(y + HEIGHT - 1) % HEIGHT] == n) +
         (noise3d[0][(x + 1) % WIDTH][(y + HEIGHT - 1) % HEIGHT] == n);
    }

static void oscillatingRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
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

    loadingFlag = false;
    //setCurrentPalette();

    step = 0U;
    if (modes[currentMode].Scale > 100U) modes[currentMode].Scale = 100U; // чтобы не было проблем при прошивке без очистки памяти
    if (modes[currentMode].Scale <= 50U) 
      curPalette = palette_arr[(uint8_t)(modes[currentMode].Scale/50.0F * ((sizeof(palette_arr)/sizeof(TProgmemRGBPalette16 *))-0.01F))];
    //else
      //curPalette = firePalettes[(uint8_t)((modes[currentMode].Scale - 50)/50.0F * ((sizeof(firePalettes)/sizeof(TProgmemRGBPalette16 *))-0.01F))];
    
    
    //случайное заполнение
    for (uint8_t i = 0; i < WIDTH; i++) {
      for (uint8_t j = 0; j < HEIGHT; j++) {
        noise3d[1][i][j] = random8(3);
        noise3d[0][i][j] = noise3d[1][i][j];
      }
    }
  }

  hue++;
  CRGB currColors[3];
  if (modes[currentMode].Scale == 100U){
    currColors[0U] = CHSV(hue, 255U, 255U);
    currColors[1U] = CHSV(hue, 128U, 255U);
    currColors[2U] = CHSV(hue, 255U, 128U);
  }
  else if (modes[currentMode].Scale > 50U){
    //uint8_t temp = (modes[currentMode].Scale - 50U) * 1.275;
    currColors[0U] = CHSV((modes[currentMode].Scale - 50U) * 5.1, 255U, 255U);
    currColors[1U] = CHSV((modes[currentMode].Scale - 50U) * 5.1, 128U, 255U);
    currColors[2U] = CHSV((modes[currentMode].Scale - 50U) * 5.1, 255U, 128U);
  }
  else
    for (uint8_t c = 0; c < 3; c++)
      currColors[c] = ColorFromPalette(*curPalette, c * 85U + hue);
  ledsClear(); // esphome: FastLED.clear();
  
  // расчёт химической реакции и отрисовка мира
  uint16_t colorCount[3] = {0U, 0U, 0U};  
  for (uint8_t x = 0; x < WIDTH; x++) {
      for (uint8_t y = 0; y < HEIGHT; y++) {
          if (noise3d[0][x][y] == 0U){
             colorCount[0U]++;
             if (calcNeighbours(x, y, 1U) > 2U)
                noise3d[1][x][y] = 1U;
          }
          else if (noise3d[0][x][y] == 1U){
             colorCount[1U]++;
             if (calcNeighbours(x, y, 2U) > 2U)
                noise3d[1][x][y] = 2U;
          }
          else {//if (noise3d[0][x][y] == 2U){
             colorCount[2U]++;
             if (calcNeighbours(x, y, 0U) > 2U)
                noise3d[1][x][y] = 0U;
          }
          drawPixelXYFseamless((float)x + 0.5, (float)y + 0.5, currColors[noise3d[1][x][y]]);
      }
  }

  // проверка зацикливания
  if (colorCount[0] == deltaHue && colorCount[1] == deltaHue2 && colorCount[2] == deltaValue){
    step++;
    if (step > 10U){
      if (colorCount[0] < colorCount[1])
        step = 0;
      else
        step = 1;
      if (colorCount[2] < colorCount[step])
        step = 2;
      colorCount[step] = 0U;
      step = 0U;
    }
  }
  else
    step = 0U;
    
  // вброс хаоса
  if (hue == hue2){// чтобы не каждый ход
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
  for (uint8_t c = 0; c < 3; c++)
  {
    if (colorCount[c] < 6U){
      uint8_t tx = random8(WIDTH);
      uint8_t ty = random8(HEIGHT);
      if (random8(2U)){
        noise3d[1][tx][ty] = c;
        noise3d[1][(tx + 1U) % WIDTH][ty] = c;
        noise3d[1][(tx + 2U) % WIDTH][ty] = c;
      }
      else {
        noise3d[1][tx][ty] = c;
        noise3d[1][tx][(ty + 1U) % HEIGHT] = c;
        noise3d[1][tx][(ty + 2U) % HEIGHT] = c;
      }
    }
  }

  // перенос на следующий цикл
  for (uint8_t x = 0; x < WIDTH; x++) {
      for (uint8_t y = 0; y < HEIGHT; y++) {
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

#define SPARKLES_NUM  (WIDTH / 8U) // не более чем  enlargedOBJECT_MAX_COUNT (WIDTH * 2)
//float   trackingObjectPosX[SPARKLES_NUM]; // это для искорок. по идее должны быть uint8_t, но были только такие
//float   trackingObjectPosY[SPARKLES_NUM];
//uint8_t shiftHue[HEIGHT];
//uint16_t ff_y, ff_z; используем для сдвига нойза переменные из общих
//uint8_t deltaValue;

static void fire2020Routine2(){
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(1U+random8(100U), 195U+random8(40U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    if (modes[currentMode].Scale > 100U) modes[currentMode].Scale = 100U; // чтобы не было проблем при прошивке без очистки памяти
    /*if (modes[currentMode].Scale == 100U)
      deltaValue = random8(9U);
    else
      deltaValue = modes[currentMode].Scale * 0.0899; // /100.0F * ((sizeof(firePalettes)/sizeof(TProgmemRGBPalette16 *))-0.01F))*/
    deltaValue = modes[currentMode].Scale * 0.0899;// /100.0F * ((sizeof(palette_arr) /sizeof(TProgmemRGBPalette16 *))-0.01F));
    if (deltaValue == 3U ||deltaValue == 4U)
      curPalette =  palette_arr[deltaValue]; // (uint8_t)(modes[currentMode].Scale/100.0F * ((sizeof(palette_arr) /sizeof(TProgmemRGBPalette16 *))-0.01F))];
    else
      curPalette = firePalettes[deltaValue]; // (uint8_t)(modes[currentMode].Scale/100.0F * ((sizeof(firePalettes)/sizeof(TProgmemRGBPalette16 *))-0.01F))];
    deltaValue = (((modes[currentMode].Scale - 1U) % 11U + 1U) << 4U) - 8U; // ширина языков пламени (масштаб шума Перлина)
    deltaHue = map(deltaValue, 8U, 168U, 8U, 84U); // высота языков пламени должна уменьшаться не так быстро, как ширина
    step = map(255U-deltaValue, 87U, 247U, 4U, 32U); // вероятность смещения искорки по оси ИКС
    for (uint8_t j = 0; j < HEIGHT; j++) {
      shiftHue[j] = (HEIGHT - 1 - j) * 255 / (HEIGHT - 1); // init colorfade table
    }

    for (uint8_t i = 0; i < SPARKLES_NUM; i++) {
        trackingObjectPosY[i] = random8(HEIGHT);
        trackingObjectPosX[i] = random8(WIDTH);
    }
  }
  for (uint8_t i = 0; i < WIDTH; i++) {
    for (uint8_t j = 0; j < HEIGHT; j++) {
//if (modes[currentMode].Brightness & 0x01)
//      leds[XY(i,HEIGHT-1U-j)] = ColorFromPalette(*curPalette, qsub8(inoise8(i * deltaValue, (j+ff_y+random8(2)) * deltaHue, ff_z), shiftHue[j]), 255U);
//else // немного сгладим картинку
      nblend(leds[XY(i,HEIGHT-1U-j)], ColorFromPalette(*curPalette, qsub8(inoise8(i * deltaValue, (j+ff_y+random8(2)) * deltaHue, ff_z), shiftHue[j]), 255U), 160U);
    } 
  }

  //вставляем искорки из отдельного массива
  for (uint8_t i = 0; i < SPARKLES_NUM; i++) {
    //leds[XY(trackingObjectPosX[i],trackingObjectPosY[i])] += ColorFromPalette(*curPalette, random(156, 255));   //trackingObjectHue[i] 
    if (trackingObjectPosY[i] > 3U){
      leds[XY(trackingObjectPosX[i], trackingObjectPosY[i])] = leds[XY(trackingObjectPosX[i], 3U)];
      leds[XY(trackingObjectPosX[i], trackingObjectPosY[i])].fadeToBlackBy( trackingObjectPosY[i]*2U );
    }
    trackingObjectPosY[i]++;
    if (trackingObjectPosY[i] >= HEIGHT){
      trackingObjectPosY[i] = random8(4U);
      trackingObjectPosX[i] = random8(WIDTH);
    }
    if (!random8(step))
      trackingObjectPosX[i] = (WIDTH + (uint8_t)trackingObjectPosX[i] + 1U - random8(3U)) % WIDTH;
  }
  ff_y++;
  if (ff_y & 0x01)
    ff_z++;
}
#endif


#ifdef DEF_LLAND
// ============= Эффект Кипение ===============
// (c) SottNick
//по мотивам LDIRKO Ленд - эффект номер 10
//...ldir... Yaroslaw Turbin, 18.11.2020 
//https://vk.com/ldirko
//https://www.reddit.com/user/ldirko/

static void LLandRoutine(){
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        uint8_t tmp = random8(6U);
        if (tmp > 1U) tmp += 3U;
        tmp = tmp*11U+4U+random8(8U);
        if (tmp > 97U) tmp = 94U;
        setModeSettings(tmp, 200U+random8(46U));// масштаб 4-11, палитры 0, 1, 5, 6, 7, 8 (кроме 2, 3, 4)
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    setCurrentPalette();

    //speedfactor = fmap(modes[currentMode].Speed, 1., 255., 20., 1.) / 16.;
    deltaValue = 10U * ((modes[currentMode].Scale - 1U) % 11U + 1U);// значения от 1 до 11 
    // значения от 0 до 10 = ((modes[currentMode].Scale - 1U) % 11U)

  }
  hue2 += 32U;
  if (hue2 < 32U)
    hue++;
  //float t = (float)millis() / speedfactor;
  ff_y += 16U;
  
  for (uint8_t y = 0; y < HEIGHT; y++)
    for (uint16_t x = 0; x < WIDTH; x++)
      //drawPixelXY(x, y, ColorFromPalette (*curPalette, map(inoise8(x * 50, y * 50 - t, 0) - y * 255 / (HEIGHT - 1), 0, 255, 205, 255) + hue, 255));
      drawPixelXY(x, y, ColorFromPalette (*curPalette, map(inoise8(x * deltaValue, y * deltaValue - ff_y, ff_z) - y * 255 / (HEIGHT - 1), 0, 255, 205, 255) + hue, 255));
  ff_z++;      
}
#endif


#ifdef DEF_ATTRACT
// ============= ЭФФЕКТ ПРИТЯЖЕНИЕ ===============
// https://github.com/pixelmatix/aurora/blob/master/PatternAttract.h
// Адаптация (c) SottNick

// используются переменные эффекта Стая. Без него работать не будет.
//#define ASTEROIDS_NUM 5U // количество шариков не должно превышать AVAILABLE_BOID_COUNT = 20U;

static void attractRoutine() {
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        uint8_t tmp = random8(8U);
        if (tmp > 3U) tmp++;
        setModeSettings(tmp*11U+3U+random8(9U), 180U+random8(56U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    setCurrentPalette();

    enlargedObjectNUM = (modes[currentMode].Scale - 1U) % 11U + 1U; //(modes[currentMode].Scale - 1U) / 99.0 * (AVAILABLE_BOID_COUNT - 1U) + 1U;
    //if (enlargedObjectNUM > AVAILABLE_BOID_COUNT) enlargedObjectNUM = AVAILABLE_BOID_COUNT;

    for (uint8_t i = 0; i < enlargedObjectNUM; i++) {
      //boids[i] = Boid(random(HEIGHT), 0);
      boids[i] = Boid(random8(WIDTH), random8(HEIGHT));   //WIDTH - 1, HEIGHT - i);
      //boids[i].location.x = random8(WIDTH);             //CENTER_X_MINOR + (float)random8() / 50.;
      //boids[i].location.y = random8(HEIGHT);            //CENTER_Y_MINOR + (float)random8() / 50.;
      boids[i].mass = ((float)random8(33U, 134U)) / 100.; // random(0.1, 2); // сюда можно поставить регулятор разлёта. чем меньше число, тем дальше от центра будет вылет
      //boids[i].velocity.x = ((float) random(40, 50)) / 100.0;
      //boids[i].velocity.x = ((float) random(modes[currentMode].Speed, modes[currentMode].Scale+10)) / 200.0;
      //boids[i].velocity.x = ((float) random8(modes[currentMode].Scale+45, modes[currentMode].Scale+100)) / 500.0;
      boids[i].velocity.x = ((float) random8(46U, 100U)) / 500.0;
      if (random8(2U)) boids[i].velocity.x = -boids[i].velocity.x;
      boids[i].velocity.y = 0;
      boids[i].colorIndex = random8();                    //i * 32;
      //boids[i].maxspeed = 0.380 * modes[currentMode].Speed /63.5+0.380;
      //boids[i].maxforce = 0.015 * modes[currentMode].Speed /63.5+0.015;
    }
  } 
  dimAll(220);
  //ledsClear(); // esphome: FastLED.clear();

  PVector attractLocation = PVector(WIDTH * 0.5, HEIGHT * 0.5);
  //float attractMass = 10;
  //float attractG = .5;
  // перемножаем и получаем 5.

  for (uint8_t i = 0; i < enlargedObjectNUM; i++) 
  {
    Boid boid = boids[i];
    //Boid * boid = &boids[i];
    PVector force = attractLocation - boid.location;    // Calculate direction of force // и вкорячиваем сюда регулировку скорости
    float d = force.mag();                              // Distance between objects
    d = constrain(d, 5.0f, HEIGHT*2.);                  // Limiting the distance to eliminate "extreme" results for very close or very far objects
    force.normalize();                                  // Normalize vector (distance doesn't matter here, we just want this vector for direction)
    float strength = (5. * boid.mass) / (d * d);        // Calculate gravitional force magnitude 5.=attractG*attractMass
    force *= strength;                                  // Get force vector --> magnitude * direction
    
    boid.applyForce(force);

    boid.update();
    drawPixelXYF(boid.location.x, boid.location.y, ColorFromPalette(*curPalette, boid.colorIndex + hue));

    boids[i] = boid;
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
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(random8(30U) ? (random8(40U) ? 2U+random8(99U) : 1U) : 100U, 12U+random8(68U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
   
    loadingFlag = false;
    setCurrentPalette();

    //enlargedObjectNUM = (modes[currentMode].Scale - 1U) % 11U + 1U;//(modes[currentMode].Scale - 1U) / 99.0 * (AVAILABLE_BOID_COUNT - 1U) + 1U;
    enlargedObjectNUM = map(modes[currentMode].Speed, 1, 255, 1, trackingOBJECT_MAX_COUNT);
    //speedfactor = modes[currentMode].Speed / 1048.0f + 0.05f;
    speedfactor = 0.136f; // фиксируем хорошую скорость

    for (uint8_t i = 0U; i < enlargedObjectNUM; i++)
    {
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = random8(HEIGHT);
      trackingObjectSpeedY[i] = random8(150, 250) / 100.; 
      trackingObjectState[i] = random8(127U, 255U);
      //trackingObjectHue[i] = hue; не похоже, что цвет используется
    }
   hue = modes[currentMode].Scale * 2.55;
  } 
  //dimAll(map(modes[currentMode].Speed, 1, 255, 250, 240));
  dimAll(246); // для фиксированной скорости
  
  CHSV color;

  for (uint8_t i = 0U; i < enlargedObjectNUM; i++)
  {
    trackingObjectPosY[i] -= trackingObjectSpeedY[i]*speedfactor;

    if (modes[currentMode].Scale == 100U) {
      color = rgb2hsv_approximate(CRGB::Gray);
      color.val = trackingObjectState[i];
    } else if (modes[currentMode].Scale == 1U) {
      color = CHSV(++hue, 255, trackingObjectState[i]);
    } else {
      color = CHSV(hue, 255, trackingObjectState[i]);
    }


    drawPixelXYF(trackingObjectPosX[i], trackingObjectPosY[i], color);

    #define GLUK 20 // вероятность горизонтального сдвига капли
    if (random8() < GLUK) {
      //trackingObjectPosX[i] = trackingObjectPosX[i] + random(-1, 2);
      trackingObjectPosX[i] = (uint8_t)(trackingObjectPosX[i] + WIDTH - 1U + random8(3U)) % WIDTH ;
      trackingObjectState[i] = random8(196,255);
    }

    if(trackingObjectPosY[i] < -1) {
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = random8(HEIGHT - HEIGHT /2, HEIGHT);
      trackingObjectSpeedY[i] = random8(150, 250) / 100.; 
      trackingObjectState[i] = random8(127U, 255U);
      //trackingObjectHue[i] = hue; не похоже, что цвет используется
    }
  }
}
#endif


#ifdef DEF_SMOKEBALLS
//-------- Эффект Дымовые шашки ----------- aka "Детские сны"
// (c) Stepko
// https://editor.soulmatelights.com/gallery/505
// https://github.com/DmytroKorniienko/FireLamp_JeeUI/blob/master/src/effects.cpp
static void smokeballsRoutine(){
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(random8(9U)*11U+3U+random8(9U), 1U+random8(255U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    setCurrentPalette();
    
    enlargedObjectNUM = enlargedObjectNUM = (modes[currentMode].Scale - 1U) % 11U + 1U;
    speedfactor = fmap(modes[currentMode].Speed, 1., 255., .02, .1); // попробовал разные способы управления скоростью. Этот максимально приемлемый, хотя и сильно тупой.
    //randomSeed(millis());
    for (byte j = 0; j < enlargedObjectNUM; j++) {
      trackingObjectShift[j] =  random((WIDTH * 10) - ((WIDTH / 3) * 20)); // сумма trackingObjectState + trackingObjectShift не должна выскакивать за макс.Х
      //trackingObjectSpeedX[j] = EffectMath::randomf(5., (float)(16 * WIDTH)); //random(50, 16 * WIDTH) / random(1, 10);
      trackingObjectSpeedX[j] = (float)random(25, 80 * WIDTH) / 5.;
      trackingObjectState[j] = random((WIDTH / 2) * 10, (WIDTH / 3) * 20);
      trackingObjectHue[j] = random8();//(9) * 28;
      trackingObjectPosX[j] = trackingObjectShift[j];
    }
  }
  
  //shiftUp();
  for (byte x = 0; x < WIDTH; x++) {
    for (float y = (float)HEIGHT; y > 0.; y-= speedfactor) {
      drawPixelXY(x, y, getPixColorXY(x, y - 1));
    }
  }
  
  fadeToBlackBy(leds, NUM_LEDS, 128U / HEIGHT);
  if (modes[currentMode].Speed & 0x01)
    blurScreen(20);
  for (byte j = 0; j < enlargedObjectNUM; j++) {
    trackingObjectPosX[j] = beatsin16((uint8_t)(trackingObjectSpeedX[j] * (speedfactor * 5.)), trackingObjectShift[j], trackingObjectState[j] + trackingObjectShift[j], trackingObjectHue[j]*256, trackingObjectHue[j]*8);
    drawPixelXYF(trackingObjectPosX[j] / 10., 0.05, ColorFromPalette(*curPalette, trackingObjectHue[j]));
  }

  EVERY_N_SECONDS(20){
    for (byte j = 0; j < enlargedObjectNUM; j++) {
      trackingObjectShift[j] += random(-20,20);
      trackingObjectHue[j] += 28;
    }
  }

  loadingFlag = random8() > 253U;
}
#endif


#ifdef DEF_NEXUS
// ------------- Nexus --------------
// (c) kostyamat
// https://github.com/DmytroKorniienko/FireLamp_JeeUI/blob/master/src/effects.cpp

//#define enlargedOBJECT_MAX_COUNT            (WIDTH * 2)          // максимальное количество червяков
//uint8_t enlargedObjectNUM;                                   // выбранное количество червяков
//float trackingObjectPosX[trackingOBJECT_MAX_COUNT]; // тут будет позиция головы 
//float trackingObjectPosY[trackingOBJECT_MAX_COUNT]; // тут будет позиция головы
//float trackingObjectSpeedX[trackingOBJECT_MAX_COUNT]; // тут будет скорость червяка
//uint8_t trackingObjectHue[trackingOBJECT_MAX_COUNT]; // тут будет цвет червяка
//uint8_t trackingObjectState[trackingOBJECT_MAX_COUNT]; тут будет направление червяка

static void nexusReset(uint8_t i){
      trackingObjectHue[i] = random8();
      trackingObjectState[i] = random8(4);
      //trackingObjectSpeedX[i] = (255. + random8()) / 255.;
      trackingObjectSpeedX[i] = (float)random8(5,11) / 70 + speedfactor; // делаем частицам немного разное ускорение и сразу пересчитываем под общую скорость
        switch (trackingObjectState[i]) {
          case B01:
              trackingObjectPosY[i] = HEIGHT;
              trackingObjectPosX[i] = random8(WIDTH);
            break;
          case B00:
              trackingObjectPosY[i] = -1;
              trackingObjectPosX[i] = random8(WIDTH);
            break;
          case B10:
              trackingObjectPosX[i] = WIDTH;
              trackingObjectPosY[i] = random8(HEIGHT);
            break;
          case B11:
              trackingObjectPosX[i] = -1;
              trackingObjectPosY[i] = random8(HEIGHT);
            break;
        }
}

static void nexusRoutine(){
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(random8(2U) ? 11U+random8(15U) : 26U+random8(55U), 1U+random8(161U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    loadingFlag = false;
    speedfactor = fmap(modes[currentMode].Speed, 1, 255, 0.1, .33);//(float)modes[currentMode].Speed / 555.0f + 0.001f;
    
    enlargedObjectNUM = (modes[currentMode].Scale - 1U) / 99.0 * (enlargedOBJECT_MAX_COUNT - 1U) + 1U;
    if (enlargedObjectNUM > enlargedOBJECT_MAX_COUNT) enlargedObjectNUM = enlargedOBJECT_MAX_COUNT;
    for (uint8_t i = 0; i < enlargedObjectNUM; i++){
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = random8(HEIGHT);
      trackingObjectSpeedX[i] = (float)random8(5,11) / 70 + speedfactor; // делаем частицам немного разное ускорение и сразу пересчитываем под общую скорость
      trackingObjectHue[i] = random8();
      trackingObjectState[i] = random8(4);//     B00           // задаем направление
                                           // B10     B11
                                           //     B01
    }
    deltaValue = 255U - map(modes[currentMode].Speed, 1, 255, 11, 33);
    
  }
  dimAll(deltaValue);

  for (uint8_t i = 0; i < enlargedObjectNUM; i++){
        switch (trackingObjectState[i]) {
          case B01:
            trackingObjectPosY[i] -= trackingObjectSpeedX[i];
            if (trackingObjectPosY[i] <= -1)
              nexusReset(i);
            break;
          case B00:
            trackingObjectPosY[i] += trackingObjectSpeedX[i];
            if (trackingObjectPosY[i] >= HEIGHT)
              nexusReset(i);
            break;
          case B10:
            trackingObjectPosX[i] -= trackingObjectSpeedX[i];
            if (trackingObjectPosX[i] <= -1)
              nexusReset(i);
            break;
          case B11:
            trackingObjectPosX[i] += trackingObjectSpeedX[i];
            if (trackingObjectPosX[i] >= WIDTH)
              nexusReset(i);
            break;
        }
    drawPixelXYF(trackingObjectPosX[i], trackingObjectPosY[i],  CHSV(trackingObjectHue[i], 255U, 255));
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
static void pacifica_one_layer(CRGB *leds, const TProgmemRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff)
{
  uint16_t ci = cistart;
  uint16_t waveangle = ioff;
  uint16_t wavescale_half = (wavescale / 2) + 20;
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    waveangle += 250;
    uint16_t s16 = sin16(waveangle) + 32768;
    uint16_t cs  = scale16(s16 , wavescale_half) + wavescale_half;
    ci += cs;
    uint16_t sindex16 = sin16(ci) + 32768;
    uint8_t  sindex8  = scale16(sindex16, 240);
    CRGB c = ColorFromPalette(p, sindex8, bri, LINEARBLEND);
    leds[i] += c;
  }
}

// Add extra 'white' to areas where the four layers of light have lined up brightly
static void pacifica_add_whitecaps(CRGB *leds)
{
  uint8_t basethreshold = beatsin8( 9, 55, 65);
  uint8_t wave = beat8( 7 );

  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t threshold = scale8( sin8( wave), 20) + basethreshold;
    wave += 7;
    uint8_t l = leds[i].getAverageLight();
    if( l > threshold) {
      uint8_t overage = l - threshold;
      uint8_t overage2 = qadd8( overage, overage);
      leds[i] += CRGB( overage, overage2, qadd8( overage2, overage2));
    }
  }
}

// Deepen the blues and greens
static void pacifica_deepen_colors(CRGB *leds)
{
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i].blue = scale8( leds[i].blue,  145);
    leds[i].green= scale8( leds[i].green, 200);
    leds[i] |= CRGB( 2, 5, 7);
  }
}

static void pacificRoutine()
{
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(100U, 1U+random8(255U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

  // Increment the four "color index start" counters, one for each wave layer.
  // Each is incremented at a different speed, and the speeds vary over time.
  static uint16_t sCIStart1, sCIStart2, sCIStart3, sCIStart4;
  static uint32_t sLastms = 0;
  uint32_t ms = GET_MILLIS();
  uint32_t deltams = ms - sLastms;
  sLastms = ms;
  uint16_t speedfactor1 = beatsin16(3, 179, 269);
  uint16_t speedfactor2 = beatsin16(4, 179, 269);
  uint32_t deltams1 = (deltams * speedfactor1) / map(modes[currentMode].Speed, 1, 255, 620, 60);
  uint32_t deltams2 = (deltams * speedfactor2) / map(modes[currentMode].Speed, 1, 255, 620, 60);
  uint32_t deltams21 = (deltams1 + deltams2) / 2;
  sCIStart1 += (deltams1 * beatsin88(1011,10,13));
  sCIStart2 -= (deltams21 * beatsin88(777,8,11));
  sCIStart3 -= (deltams1 * beatsin88(501,5,7));
  sCIStart4 -= (deltams2 * beatsin88(257,4,6));

  // Clear out the LED array to a dim background blue-green
  fill_solid( leds, NUM_LEDS, CRGB( 2, 6, 10));

  // Render each of four layers, with different scales and speeds, that vary over time
  pacifica_one_layer(&*leds, pacifica_palette_1, sCIStart1, beatsin16( 3, 11 * 256, 14 * 256), beatsin8( 10, 70, 130), 0-beat16( 301) );
  pacifica_one_layer(&*leds, pacifica_palette_2, sCIStart2, beatsin16( 4,  6 * 256,  9 * 256), beatsin8( 17, 40,  80), beat16( 401) );
  pacifica_one_layer(&*leds, pacifica_palette_3, sCIStart3, 6 * 256, beatsin8( 9, 10,38), 0-beat16(503));
  pacifica_one_layer(&*leds, pacifica_palette_3, sCIStart4, 5 * 256, beatsin8( 8, 10,28), beat16(601));

  // Add brighter 'whitecaps' where the waves lines up more
  pacifica_add_whitecaps(&*leds);

  // Deepen the blues and greens a bit
  pacifica_deepen_colors(&*leds);
  blurScreen(20);
}
#endif

//-------- по мотивам Эффектов Particle System -------------------------
// https://github.com/fuse314/arduino-particle-sys
// https://github.com/giladaya/arduino-particle-sys
// https://www.youtube.com/watch?v=S6novCRlHV8&t=51s
//#include <ParticleSys.h>
//при попытке вытащить из этой библиотеки только минимально необходимое выяснилось, что там очередной (третий) вариант реализации субпиксельной графики.
//ну его нафиг. лучше будет повторить визуал имеющимися в прошивке средствами.

static void particlesUpdate2(uint8_t i){
  //age
  trackingObjectState[i]--; //ttl // ещё и сюда надо speedfactor вкорячить. удачи там!

  //apply acceleration
  //trackingObjectSpeedX[i] = min((int)trackingObjectSpeedX[i]+ax, WIDTH);
  //trackingObjectSpeedY[i] = min((int)trackingObjectSpeedY[i]+ay, HEIGHT);

  //apply velocity
  trackingObjectPosX[i] += trackingObjectSpeedX[i];
  trackingObjectPosY[i] += trackingObjectSpeedY[i];
  if(trackingObjectState[i] == 0 || trackingObjectPosX[i] <= -1 || trackingObjectPosX[i] >= WIDTH || trackingObjectPosY[i] <= -1 || trackingObjectPosY[i] >= HEIGHT) 
    trackingObjectIsShift[i] = false;
}


#ifdef DEF_FOUNTAIN
// ============= ЭФФЕКТ ИСТОЧНИК ===============
// (c) SottNick
// выглядит как https://github.com/fuse314/arduino-particle-sys/blob/master/examples/StarfieldFastLED/StarfieldFastLED.ino

static void starfield2Emit(uint8_t i){
  if (hue++ & 0x01)
    hue2++;//counter++;
  //source->update(g); хз зачем это было в оригинале - там только смерть source.isAlive высчитывается, вроде

  trackingObjectPosX[i] = WIDTH * 0.5;//CENTER_X_MINOR;// * RENDERER_RESOLUTION; //  particle->x = source->x;
  trackingObjectPosY[i] = HEIGHT * 0.5;//CENTER_Y_MINOR;// * RENDERER_RESOLUTION; //  // particle->y = source->y;

  //trackingObjectSpeedX[i] = ((float)random8()-127.)/512./0.25*speedfactor; // random(_hVar)-_constVel; // particle->vx
  trackingObjectSpeedX[i] = ((float)random8()-127.)/512.; // random(_hVar)-_constVel; // particle->vx
  //trackingObjectSpeedY[i] = SQRT_VARIANT((speedfactor*speedfactor+0.0001)-trackingObjectSpeedX[i]*trackingObjectSpeedX[i]); // SQRT_VARIANT(pow(_constVel,2)-pow(trackingObjectSpeedX[i],2)); // particle->vy зависит от particle->vx - не ошибка
  trackingObjectSpeedY[i] = SQRT_VARIANT(0.0626-trackingObjectSpeedX[i]*trackingObjectSpeedX[i]); // SQRT_VARIANT(pow(_constVel,2)-pow(trackingObjectSpeedX[i],2)); // particle->vy зависит от particle->vx - не ошибка
  if(random8(2U)) { trackingObjectSpeedY[i]=-trackingObjectSpeedY[i]; }
  trackingObjectState[i] = random8(50, 250); // random8(minLife, maxLife);// particle->ttl
  if (modes[currentMode].Speed & 0x01)
    trackingObjectHue[i] = hue2;// (counter/2)%255; // particle->hue
  else
    trackingObjectHue[i] = random8();
  trackingObjectIsShift[i] = true; // particle->isAlive
}

static void starfield2Routine(){
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(25U+random8(76U), 185U + random8(30U)*2U + (random8(6U) ? 0U : 1U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    //speedfactor = (float)modes[currentMode].Speed / 510.0f + 0.001f;    
    enlargedObjectNUM = (modes[currentMode].Scale - 1U) / 99.0 * (trackingOBJECT_MAX_COUNT - 1U) + 1U;
    if (enlargedObjectNUM > trackingOBJECT_MAX_COUNT) enlargedObjectNUM = trackingOBJECT_MAX_COUNT;
    //deltaValue = 1; // количество зарождающихся частиц за 1 цикл //perCycle = 1;
    deltaValue = enlargedObjectNUM / (SQRT_VARIANT(CENTER_X_MAJOR*CENTER_X_MAJOR + CENTER_Y_MAJOR*CENTER_Y_MAJOR) * 4U) + 1U; // 4 - это потому что за 1 цикл частица пролетает ровно четверть расстояния между 2мя соседними пикселями
    for(int i = 0; i<enlargedObjectNUM; i++)
      trackingObjectIsShift[i] = false; // particle->isAlive
  }
  step = deltaValue; //счётчик количества частиц в очереди на зарождение в этом цикле
  //renderer.fade(leds); = fadeToBlackBy(128); = dimAll(255-128)
  //dimAll(255-128/.25*speedfactor); ахах-ха. очередной эффект, к которому нужно будет "подобрать коэффициенты"
  dimAll(127);

  //go over particles and update matrix cells on the way
  for(int i = 0; i<enlargedObjectNUM; i++) {
    if (!trackingObjectIsShift[i] && step) {
      //emitter->emit(&particles[i], this->g);
      starfield2Emit(i);
      step--;
    }
    if (trackingObjectIsShift[i]){ // particle->isAlive
      //particles[i].update(this->g);
      particlesUpdate2(i);

      //generate RGB values for particle
      CRGB baseRGB = CHSV(trackingObjectHue[i], 255,255); // particles[i].hue

      //baseRGB.fadeToBlackBy(255-trackingObjectState[i]);
      baseRGB.nscale8(trackingObjectState[i]);//эквивалент
      drawPixelXYF(trackingObjectPosX[i], trackingObjectPosY[i], baseRGB);
    }
  }
}
#endif


#ifdef DEF_FAIRY
// ============= ЭФФЕКТ ФЕЯ ===============
// (c) SottNick
#define FAIRY_BEHAVIOR //типа сложное поведение

static void fairyEmit(uint8_t i) //particlesEmit(Particle_Abstract *particle, ParticleSysConfig *g)
{
    if (deltaHue++ & 0x01)
      if (hue++ & 0x01)
        hue2++;//counter++;
    trackingObjectPosX[i] = boids[0].location.x;
    trackingObjectPosY[i] = boids[0].location.y;

    //хотите навставлять speedfactor? - тут не забудьте
    //trackingObjectSpeedX[i] = ((float)random8()-127.)/512./0.25*speedfactor; // random(_hVar)-_constVel; // particle->vx
    trackingObjectSpeedX[i] = ((float)random8()-127.)/512.; // random(_hVar)-_constVel; // particle->vx
    //trackingObjectSpeedY[i] = SQRT_VARIANT((speedfactor*speedfactor+0.0001)-trackingObjectSpeedX[i]*trackingObjectSpeedX[i]); // SQRT_VARIANT(pow(_constVel,2)-pow(trackingObjectSpeedX[i],2)); // particle->vy зависит от particle->vx - не ошибка
    trackingObjectSpeedY[i] = SQRT_VARIANT(0.0626-trackingObjectSpeedX[i]*trackingObjectSpeedX[i]); // SQRT_VARIANT(pow(_constVel,2)-pow(trackingObjectSpeedX[i],2)); // particle->vy зависит от particle->vx - не ошибка
    if(random8(2U)) { trackingObjectSpeedY[i]=-trackingObjectSpeedY[i]; }

    trackingObjectState[i] = random8(20, 80); // random8(minLife, maxLife);// particle->ttl
    trackingObjectHue[i] = hue2;// (counter/2)%255; // particle->hue
    trackingObjectIsShift[i] = true; // particle->isAlive
}

static void fairyRoutine(){
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(14U+random8(87U), 190U + random8(40U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    //speedfactor = (float)modes[currentMode].Speed / 510.0f + 0.001f;    

    deltaValue = 10; // количество зарождающихся частиц за 1 цикл //perCycle = 1;
    enlargedObjectNUM = (modes[currentMode].Scale - 1U) / 99.0 * (trackingOBJECT_MAX_COUNT - 1U) + 1U;
    if (enlargedObjectNUM > trackingOBJECT_MAX_COUNT) enlargedObjectNUM = trackingOBJECT_MAX_COUNT;
    for(int i = 0; i<enlargedObjectNUM; i++)
      trackingObjectIsShift[i] = false; // particle->isAlive

      // лень было придумывать алгоритм для траектории феи, поэтому это будет нулевой "бойд" из эффекта Притяжение
      boids[0] = Boid(random8(WIDTH), random8(HEIGHT));//WIDTH - 1, HEIGHT - 1);
      //boids[0].location.x = random8(WIDTH);
      //boids[0].location.y = random8(HEIGHT);
      boids[0].mass = 0.5;//((float)random8(33U, 134U)) / 100.; // random(0.1, 2); // сюда можно поставить регулятор разлёта. чем меньше число, тем дальше от центра будет вылет
      boids[0].velocity.x = ((float) random8(46U, 100U)) / 500.0;
      if (random8(2U)) boids[0].velocity.x = -boids[0].velocity.x;
      boids[0].velocity.y = 0;
      hue = random8();//boids[0].colorIndex = 
      #ifdef FAIRY_BEHAVIOR
        deltaHue2 = 1U;
      #endif
  }
  step = deltaValue; //счётчик количества частиц в очереди на зарождение в этом цикле
  
#ifdef FAIRY_BEHAVIOR
  if (!deltaHue && deltaHue2 && fabs(boids[0].velocity.x) + fabs(boids[0].velocity.y) < 0.15){ 
    deltaHue2 = 0U;
    
    boids[1].velocity.x = ((float)random8()+255.) / 4080.;
    boids[1].velocity.y = ((float)random8()+255.) / 2040.;
    if (boids[0].location.x > WIDTH * 0.5) boids[1].velocity.x = -boids[1].velocity.x;
    if (boids[0].location.y > HEIGHT * 0.5) boids[1].velocity.y = -boids[1].velocity.y;
  }
  if (!deltaHue2){
    step = 1U;
    
    boids[0].location.x += boids[1].velocity.x;
    boids[0].location.y += boids[1].velocity.y;
    deltaHue2 = (boids[0].location.x <= 0 || boids[0].location.x >= WIDTH-1 || boids[0].location.y <= 0 || boids[0].location.y >= HEIGHT-1);
  }
  else
#endif // FAIRY_BEHAVIOR
  {  
    PVector attractLocation = PVector(WIDTH * 0.5, HEIGHT * 0.5);
    //float attractMass = 10;
    //float attractG = .5;
    // перемножаем и получаем 5.
    Boid boid = boids[0];
    PVector force = attractLocation - boid.location;      // Calculate direction of force
    float d = force.mag();                                // Distance between objects
    d = constrain(d, 5.0f, HEIGHT);//видео снято на 5.0f  // Limiting the distance to eliminate "extreme" results for very close or very far objects
//d = constrain(d, modes[currentMode].Scale / 10.0, HEIGHT);

    force.normalize();                                    // Normalize vector (distance doesn't matter here, we just want this vector for direction)
    float strength = (5. * boid.mass) / (d * d);          // Calculate gravitional force magnitude 5.=attractG*attractMass
//float attractMass = (modes[currentMode].Scale) / 10.0 * .5;
//strength = (attractMass * boid.mass) / (d * d);
    force *= strength;                                    // Get force vector --> magnitude * direction
    boid.applyForce(force);
    boid.update();
    
    if (boid.location.x <= -1) boid.location.x = -boid.location.x;
    else if (boid.location.x >= WIDTH) boid.location.x = -boid.location.x+WIDTH+WIDTH;
    if (boid.location.y <= -1) boid.location.y = -boid.location.y;
    else if (boid.location.y >= HEIGHT) boid.location.y = -boid.location.y+HEIGHT+HEIGHT;
    boids[0] = boid;

    //EVERY_N_SECONDS(20)
    if (!deltaHue){
      if (random8(3U)){
        d = ((random8(2U)) ? boids[0].velocity.x : boids[0].velocity.y) * ((random8(2U)) ? .2 : -.2);
        boids[0].velocity.x += d;
        boids[0].velocity.y -= d;
      }
      else {
        if (fabs(boids[0].velocity.x) < 0.02)
          boids[0].velocity.x = -boids[0].velocity.x;
        else if (fabs(boids[0].velocity.y) < 0.02)
          boids[0].velocity.y = -boids[0].velocity.y;
      }
    }
  }

  //renderer.fade(leds); = fadeToBlackBy(128); = dimAll(255-128)
  //dimAll(255-128/.25*speedfactor); очередной эффект, к которому нужно будет "подобрать коэффициенты"
  //if (modes[currentMode].Speed & 0x01)
    dimAll(127);
  //else ledsClear(); // esphome: FastLED.clear();    

  //go over particles and update matrix cells on the way
  for(int i = 0; i<enlargedObjectNUM; i++) {
    if (!trackingObjectIsShift[i] && step) {
      //emitter->emit(&particles[i], this->g);
      fairyEmit(i);
      step--;
    }
    if (trackingObjectIsShift[i]){ // particle->isAlive
      //particles[i].update(this->g);
      if (modes[currentMode].Scale & 0x01 && trackingObjectSpeedY[i] > -1) trackingObjectSpeedY[i] -= 0.05; //apply acceleration
      particlesUpdate2(i);

      //generate RGB values for particle
      CRGB baseRGB = CHSV(trackingObjectHue[i], 255,255); // particles[i].hue

      //baseRGB.fadeToBlackBy(255-trackingObjectState[i]);
      baseRGB.nscale8(trackingObjectState[i]);//эквивалент
      drawPixelXYF(trackingObjectPosX[i], trackingObjectPosY[i], baseRGB);
    }
  }
  drawPixelXYF(boids[0].location.x, boids[0].location.y, CHSV(hue, 160U, 255U));//boid.colorIndex + hue
}
#endif


#ifdef DEF_SAND
// ============= Эффект Цветные драже ===============
// (c) SottNick
//по мотивам визуала эффекта by Yaroslaw Turbin 14.12.2020
//https://vk.com/ldirko программный код которого он запретил брать

static void sandRoutine(){
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(1U+random8(100U) , 140U+random8(100U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    //setCurrentPalette();

    pcnt = 0U;// = HEIGHT;
  }
  
  // если насыпалось уже достаточно, бахаем рандомные песчинки
  uint8_t temp = map8(random8(), modes[currentMode].Scale * 2.55, 255U);
  if (pcnt >= map8(temp, 2U, HEIGHT - 3U)){
    //temp = 255U - temp + 2;
    //if (temp < 2) temp = 255;
    temp = HEIGHT + 1U - pcnt;
    if (!random8(4U)) // иногда песка осыпается до половины разом
      if (random8(2U))
        temp = 2U;
      else
        temp = 3U;
    //for (uint16_t i = 0U; i < NUM_LEDS; i++)
    for (uint8_t y = 0; y < pcnt; y++)
      for (uint8_t x = 0; x < WIDTH; x++)
        if (!random8(temp))
          leds[XY(x,y)] = 0;
  }

  pcnt = 0U;
  // осыпаем всё, что есть на экране
  for (uint8_t y = 1; y < HEIGHT; y++)
    for (uint8_t x = 0; x < WIDTH; x++)
      if (leds[XY(x,y)])                                                           // проверяем для каждой песчинки
        if (!leds[XY(x,y-1)]){                                                     // если под нами пусто, просто падаем
          leds[XY(x,y-1)] = leds[XY(x,y)];
          leds[XY(x,y)] = 0;
        }
        else if (x>0U && !leds[XY(x-1,y-1)] && x<WIDTH-1 && !leds[XY(x+1,y-1)]){   // если под нами пик
          if (random8(2U))
            leds[XY(x-1,y-1)] = leds[XY(x,y)];
          else
            leds[XY(x-1,y-1)] = leds[XY(x,y)];
          leds[XY(x,y)] = 0;
          pcnt = y-1;
        }
        else if (x>0U && !leds[XY(x-1,y-1)]){                                      // если под нами склон налево
          leds[XY(x-1,y-1)] = leds[XY(x,y)];
          leds[XY(x,y)] = 0;
          pcnt = y-1;
        }
        else if (x<WIDTH-1 && !leds[XY(x+1,y-1)]){                                 // если под нами склон направо
          leds[XY(x+1,y-1)] = leds[XY(x,y)];
          leds[XY(x,y)] = 0;
          pcnt = y-1;
        }
        else                                                                       // если под нами плато
          pcnt = y;
      
  // эмиттер новых песчинок
  if (!leds[XY(CENTER_X_MINOR,HEIGHT-2)] && !leds[XY(CENTER_X_MAJOR,HEIGHT-2)] && !random8(3)){
    temp = random8(2) ? CENTER_X_MINOR : CENTER_X_MAJOR;
    leds[XY(temp,HEIGHT-1)] = CHSV(random8(), 255U, 255U);
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
     if (selectedSettings){
       uint8_t tmp = random8(5U);
       if (tmp > 1U) tmp += 3U;
       setModeSettings(tmp*11U+3U+random8(7U), 1U+random8(180U));
     }
   #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
   
   loadingFlag = false;
   setCurrentPalette();

   pcnt = (modes[currentMode].Scale - 1U) % 11U + 1U; // количество линий от 1 до 11 для каждой из 9 палитр
   speedfactor = fmap(modes[currentMode].Speed, 1, 255, 20., 2.); 
 }
 if (hue2++ & 0x01 && deltaHue++ & 0x01 && deltaHue2++ & 0x01) hue++; // хз. как с 60ю кадрами в секунду скорость замедлять...
 dimAll(205);
 float time_shift = millis() & 0x7FFFFF; // overflow protection proper by SottNick
 time_shift /= speedfactor;
 for (uint8_t c = 0; c < pcnt; c++) {
   float xx = 2. + sin8(time_shift + 6000 * c) / 12.;
   float yy = 2. + cos8(time_shift + 9000 * c) / 12.;
   //DrawLineF(xx, yy, (float)WIDTH - xx - 1, (float)HEIGHT - yy - 1, CHSV(c * (256 / pcnt), 200, 255)); // так было в оригинале
   //if (modes[currentMode].Speed & 0x01)
   //DrawLineF(xx, yy, (float)WIDTH - xx - 1, (float)HEIGHT - yy - 1, ColorFromPalette(*curPalette, hue + c * (255 / pcnt)).nscale8(200)); // кажется, это не работает, хотя и компилируется
   //else
   DrawLineF(xx, yy, (float)WIDTH - xx - 1, (float)HEIGHT - yy - 1, ColorFromPalette(*curPalette, hue + c * (255 / pcnt)));
 }
}
#endif


#ifdef DEF_AURORA
// --------- Эффект "Северное Сияние"
// (c) kostyamat 05.02.2021
// идеи подсмотрены тут https://www.reddit.com/r/FastLED/comments/jyly1e/challenge_fastled_sketch_that_fits_entirely_in_a/
// особая благодарность https://www.reddit.com/user/ldirko/ Yaroslaw Turbin aka ldirko

// вместо набора палитр в оригинальном эффекте сделан генератор палитр
#define AURORA_COLOR_RANGE 10 // (+/-10 единиц оттенка) диапазон, в котором плавает цвет сияния относительно выбранного оттенка 
#define AURORA_COLOR_PERIOD 2 // (2 раза в минуту) частота, с которой происходит колебание выбранного оттенка в разрешённом диапазоне

// генератор палитр для Северного сияния (c) SottNick
// static const uint8_t MBAuroraColors_arr[5][4] PROGMEM = // палитра в формате CHSV
//CRGBPalette16 myPal; уже есть эта переменная в эффекте Жидкая лампа

static void fillMyPal16_2(uint8_t hue, bool isInvert = false){ 
// я бы, конечно, вместо копии функции генерации палитры "_2"
// лучше бы сделал её параметром указатель на массив с базовой палитрой, 
// но я пониятия не имею, как это делается с грёбаным PROGMEM

  int8_t lastSlotUsed = -1;
  uint8_t istart8, iend8;
  CRGB rgbstart, rgbend;
  
  // начинаем с нуля
  if (isInvert)
    //с неявным преобразованием оттенков цвета получаются, как в фотошопе, но для данного эффекта не красиво выглядят
    //rgbstart = CHSV(256 + hue - pgm_read_byte(&MBAuroraColors_arr[0][1]), pgm_read_byte(&MBAuroraColors_arr[0][2]), pgm_read_byte(&MBAuroraColors_arr[0][3])); // начальная строчка палитры с инверсией
    hsv2rgb_spectrum(CHSV(256 + hue - pgm_read_byte(&MBAuroraColors_arr[0][1]), pgm_read_byte(&MBAuroraColors_arr[0][2]), pgm_read_byte(&MBAuroraColors_arr[0][3])), rgbstart);
  else
    //rgbstart = CHSV(hue + pgm_read_byte(&MBAuroraColors_arr[0][1]), pgm_read_byte(&MBAuroraColors_arr[0][2]), pgm_read_byte(&MBAuroraColors_arr[0][3])); // начальная строчка палитры
    hsv2rgb_spectrum(CHSV(hue + pgm_read_byte(&MBAuroraColors_arr[0][1]), pgm_read_byte(&MBAuroraColors_arr[0][2]), pgm_read_byte(&MBAuroraColors_arr[0][3])), rgbstart);
  int indexstart = 0; // начальный индекс палитры
  for (uint8_t i = 1U; i < 5U; i++) { // в палитре @obliterator всего 5 строчек
    int indexend = pgm_read_byte(&MBAuroraColors_arr[i][0]);
    if (isInvert)
      hsv2rgb_spectrum(CHSV(hue + pgm_read_byte(&MBAuroraColors_arr[i][1]), pgm_read_byte(&MBAuroraColors_arr[i][2]), pgm_read_byte(&MBAuroraColors_arr[i][3])), rgbend);
    else
      hsv2rgb_spectrum(CHSV(256 + hue - pgm_read_byte(&MBAuroraColors_arr[i][1]), pgm_read_byte(&MBAuroraColors_arr[i][2]), pgm_read_byte(&MBAuroraColors_arr[i][3])), rgbend);
    istart8 = indexstart / 16;
    iend8   = indexend   / 16;
    if ((istart8 <= lastSlotUsed) && (lastSlotUsed < 15)) {
       istart8 = lastSlotUsed + 1;
       if (iend8 < istart8)
         iend8 = istart8;
    }
    lastSlotUsed = iend8;
    fill_gradient_RGB( myPal, istart8, rgbstart, iend8, rgbend);
    indexstart = indexend;
    rgbstart = rgbend;
  }
}

static unsigned long polarTimer;
//float adjastHeight; // используем emitterX
//uint16_t adjScale; // используем ff_y

static void polarRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(random8(3U) ? 1U+random8(99U) : 100U, 1U+random8(170U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    //setCurrentPalette();

    //fillMyPal16_2((modes[currentMode].Scale - 1U) * 2.55);//, !(modes[currentMode].Scale & 0x01)); фиксированная палитра - для слабаков
    //emitterX = fmap((float)HEIGHT, 8, 32, 28, 12); такое работало с горем пополам только для матриц до 32 пикселей в высоту
    //emitterX = 512. / HEIGHT - 0.0001; // это максимально возможное значение
    emitterX = 400. / HEIGHT; // а это - максимум без яркой засветки крайних рядов матрицы (сверху и снизу)
    
    ff_y = map(WIDTH, 8, 64, 310, 63);
    //ff_z = map(modes[currentMode].Scale, 1, 100, 30, ff_y);
    ff_z = ff_y;
    speedfactor = map(modes[currentMode].Speed, 1, 255, 128, 16); // _speed = map(speed, 1, 255, 128, 16);

  }
  
  if (modes[currentMode].Scale == 100){
    if (hue2++ & 0x01 && deltaHue++ & 0x01 && deltaHue2++ & 0x01) hue++; // это ж бред, но я хз. как с 60ю кадрами в секунду можно эффективно скорость замедлять...
      fillMyPal16_2((uint8_t)((modes[currentMode].Scale - 1U) * 2.55) + hue, modes[currentMode].Scale & 0x01);
  }
  else
    fillMyPal16_2((uint8_t)((modes[currentMode].Scale - 1U) * 2.55) + AURORA_COLOR_RANGE - beatsin8(AURORA_COLOR_PERIOD, 0U, AURORA_COLOR_RANGE+AURORA_COLOR_RANGE), modes[currentMode].Scale & 0x01);

  for (byte x = 0; x < WIDTH; x++) {
    for (byte y = 0; y < HEIGHT; y++) {
      polarTimer++;
      //uint16_t i = x*y;
      leds[XY(x, y)]= 
          ColorFromPalette(myPal,
            qsub8(
              inoise8(polarTimer % 2 + x * ff_z,
                y * 16 + polarTimer % 16,
                polarTimer / speedfactor
              ),
              fabs((float)HEIGHT/2 - (float)y) * emitterX
            )
          );
/*          
      if (flag == 1) { // Тут я модифицирую стандартные палитры 
        CRGB tmpColor = leds[myLamp.getPixelNumber(x, y)];
        leds[myLamp.getPixelNumber(x, y)].g = tmpColor.r;
        leds[myLamp.getPixelNumber(x, y)].r = tmpColor.g;
        leds[myLamp.getPixelNumber(x, y)].g /= 6;
        leds[myLamp.getPixelNumber(x, y)].r += leds[myLamp.getPixelNumber(x, y)].r < 206 ? 48 : 0;;
      } else if (flag == 3) {
        leds[myLamp.getPixelNumber(x, y)].b += 48;
        leds[myLamp.getPixelNumber(x, y)].g += leds[myLamp.getPixelNumber(x, y)].g < 206 ? 48 : 0;
      }
*/      
    }
  }
}
#endif


#ifdef DEF_SPHERES
// ----------- Эффект "Шары"
// (c) stepko and kostyamat https://wokwi.com/arduino/projects/289839434049782281
// 07.02.2021

static float randomf(float min, float max)
{
  return fmap((float)random16(4095), 0.0, 4095.0, min, max);
}

static void ballsfill_circle(float cx, float cy, float radius, CRGB col) {
  radius -= 0.5;
  for (int y = -radius; y <= radius; y++) {
    for (int x = -radius; x <= radius; x++) {
      if (x * x + y * y <= radius * radius)
        drawPixelXYF(cx + x, cy + y, col);
    }
  }
}

static void spheresRoutine() {

  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(random8(8U)*11U+6U+random8(6U), 1U+random8(255U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    setCurrentPalette();

    speedfactor = fmap(modes[currentMode].Speed, 1, 255, 0.15, 0.5);
    
    enlargedObjectNUM = (modes[currentMode].Scale - 1U) % 11U + 1U;
    //if (enlargedObjectNUM > AVAILABLE_BOID_COUNT) enlargedObjectNUM = AVAILABLE_BOID_COUNT;
    emitterY = .5 + HEIGHT / 4. / (2. - 1. / enlargedObjectNUM); // radiusMax

    for (uint8_t i = 0; i < enlargedObjectNUM; i++) {
      trackingObjectShift[i] = randomf(0.5, emitterY); // radius[i] = randomf(0.5, radiusMax); 
      trackingObjectSpeedX[i] = randomf(0.5, 1.1) * speedfactor; // ball[i][2] = 
      trackingObjectSpeedY[i] = randomf(0.5, 1.1) * speedfactor; // ball[i][3] = 
      trackingObjectPosX[i] = random8(WIDTH);  // ball[i][0] = random(0, WIDTH);
      trackingObjectPosY[i] = random8(HEIGHT); // ball[i][1] = random(0, HEIGHT);
      trackingObjectHue[i] = random8();        // color[i] = random(0, 255);
    }
  } 

  dimAll(255-map(modes[currentMode].Speed, 1, 255, 5, 20));//fadeToBlackBy(leds, NUM_LEDS, map(speed, 1, 255, 5, 20));


  for (byte i = 0; i < enlargedObjectNUM; i++) {
    if (trackingObjectIsShift[i]) {  // тут у нас шарики надуваются\сдуваются по ходу движения
      trackingObjectShift[i] += (fabs(trackingObjectSpeedX[i]) > fabs(trackingObjectSpeedY[i])? fabs(trackingObjectSpeedX[i]) : fabs(trackingObjectSpeedY[i])) * 0.1 * speedfactor;
      if (trackingObjectShift[i] >= emitterY) {
        trackingObjectIsShift[i] = false;
      }
    } else {
      trackingObjectShift[i] -= (fabs(trackingObjectSpeedX[i]) > fabs(trackingObjectSpeedY[i])? fabs(trackingObjectSpeedX[i]) : fabs(trackingObjectSpeedY[i])) * 0.1 * speedfactor;
      if (trackingObjectShift[i] < 1.) {
        trackingObjectIsShift[i] = true;
        trackingObjectHue[i] = random(0, 255);
      }
    }


    //EffectMath::drawCircleF(trackingObjectPosY[i], trackingObjectPosX[i], trackingObjectShift[i], ColorFromPalette(*curPalette, trackingObjectHue[i]), 0.5);
    if (trackingObjectShift[i] > 1) 
      ballsfill_circle(trackingObjectPosY[i], trackingObjectPosX[i], trackingObjectShift[i], ColorFromPalette(*curPalette, trackingObjectHue[i]));
    else 
      drawPixelXYF(trackingObjectPosY[i], trackingObjectPosX[i], ColorFromPalette(*curPalette, trackingObjectHue[i]));


    if (trackingObjectPosX[i] + trackingObjectShift[i] >= HEIGHT - 1)
      trackingObjectPosX[i] += (trackingObjectSpeedX[i] * ((HEIGHT - 1 - trackingObjectPosX[i]) / trackingObjectShift[i] + 0.005));
    else if (trackingObjectPosX[i] - trackingObjectShift[i] <= 0)
      trackingObjectPosX[i] += (trackingObjectSpeedX[i] * (trackingObjectPosX[i] / trackingObjectShift[i] + 0.005));
    else
      trackingObjectPosX[i] += trackingObjectSpeedX[i];
    //-----------------------
    if (trackingObjectPosY[i] + trackingObjectShift[i] >= WIDTH - 1)
      trackingObjectPosY[i] += (trackingObjectSpeedY[i] * ((WIDTH - 1 - trackingObjectPosY[i]) / trackingObjectShift[i] + 0.005));
    else if (trackingObjectPosY[i] - trackingObjectShift[i] <= 0)
      trackingObjectPosY[i] += (trackingObjectSpeedY[i] * (trackingObjectPosY[i] / trackingObjectShift[i] + 0.005));
    else
      trackingObjectPosY[i] += trackingObjectSpeedY[i];
    //------------------------
    if (trackingObjectPosX[i] < 0.01) {
      trackingObjectSpeedX[i] = randomf(0.5, 1.1) * speedfactor;
      trackingObjectPosX[i] = 0.01;
    }
    else if (trackingObjectPosX[i] > HEIGHT - 1.01) {
      trackingObjectSpeedX[i] = randomf(0.5, 1.1) * speedfactor;
      trackingObjectSpeedX[i] = -trackingObjectSpeedX[i];
      trackingObjectPosX[i] = HEIGHT - 1.01;
    }
    //----------------------
    if (trackingObjectPosY[i] < 0.01) {
      trackingObjectSpeedY[i] = randomf(0.5, 1.1) * speedfactor;
      trackingObjectPosY[i] = 0.01;
    }
    else if (trackingObjectPosY[i] > WIDTH - 1.01) {
      trackingObjectSpeedY[i] = randomf(0.5, 1.1) * speedfactor;
      trackingObjectSpeedY[i] = -trackingObjectSpeedY[i];
      trackingObjectPosY[i] = WIDTH - 1.01;
    }
  }
  blurScreen(48);
}
#endif


#ifdef DEF_MAGMA
// ============= Эффект Магма ===============
// (c) SottNick
// берём эффекты Огонь 2020 и Прыгуны:
// хуяк-хуяк - и в продакшен!

static void magmaRoutine(){
  //unsigned num = map(scale, 0U, 255U, 6U, sizeof(boids) / sizeof(*boids));
  if (loadingFlag)
  {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        //палитры 0,1,5,6,7
        uint8_t tmp = random8(6U);
        if (tmp>1U) tmp+=3U;
        setModeSettings(tmp*11U+2U+random8(7U) , 185U+random8(48U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    //setCurrentPalette();    

    deltaValue = modes[currentMode].Scale * 0.0899;// /100.0F * ((sizeof(palette_arr) /sizeof(TProgmemRGBPalette16 *))-0.01F));
    if (deltaValue == 3U ||deltaValue == 4U)
      curPalette =  palette_arr[deltaValue]; // (uint8_t)(modes[currentMode].Scale/100.0F * ((sizeof(palette_arr) /sizeof(TProgmemRGBPalette16 *))-0.01F))];
    else
      curPalette = firePalettes[deltaValue]; // (uint8_t)(modes[currentMode].Scale/100.0F * ((sizeof(firePalettes)/sizeof(TProgmemRGBPalette16 *))-0.01F))];
    //deltaValue = (((modes[currentMode].Scale - 1U) % 11U + 1U) << 4U) - 8U; // ширина языков пламени (масштаб шума Перлина)
    deltaValue = 12U;
    deltaHue = 10U;// map(deltaValue, 8U, 168U, 8U, 84U); // высота языков пламени должна уменьшаться не так быстро, как ширина
    //step = map(255U-deltaValue, 87U, 247U, 4U, 32U); // вероятность смещения искорки по оси ИКС
    for (uint8_t j = 0; j < HEIGHT; j++) {
      shiftHue[j] = (HEIGHT - 1 - j) * 255 / (HEIGHT - 1); // init colorfade table
    }
    
    //ledsClear(); // esphome: FastLED.clear();
    //enlargedObjectNUM = (modes[currentMode].Scale - 1U) / 99.0 * (enlargedOBJECT_MAX_COUNT - 1U) + 1U;
    enlargedObjectNUM = (modes[currentMode].Scale - 1U) % 11U / 10.0 * (enlargedOBJECT_MAX_COUNT - 1U) + 1U;
    if (enlargedObjectNUM > enlargedOBJECT_MAX_COUNT) enlargedObjectNUM = enlargedOBJECT_MAX_COUNT;
    //if (enlargedObjectNUM < 2U) enlargedObjectNUM = 2U;

    for (uint8_t i = 0 ; i < enlargedObjectNUM ; i++) {
      trackingObjectPosX[i] = random8(WIDTH);
      trackingObjectPosY[i] = random8(HEIGHT);

      //curr->color = CHSV(random(1U, 255U), 255U, 255U);
      trackingObjectHue[i] = 50U;random8();
    }
  }

  //myLamp.dimAll(0); накой хрен делать затухание на 100%?
  //ledsClear(); // esphome: FastLED.clear();
  //dimAll(255U - modes[currentMode].Scale * 2);
  //dimAll(255U - 44U * 2);
  dimAll(181);

  for (uint8_t i = 0; i < WIDTH; i++) {
    for (uint8_t j = 0; j < HEIGHT; j++) {
      //leds[XY(i,HEIGHT-1U-j)] = ColorFromPalette(*curPalette, qsub8(inoise8(i * deltaValue, (j+ff_y+random8(2)) * deltaHue, ff_z), shiftHue[j]), 255U);
      drawPixelXYF(i,HEIGHT-1U-j,ColorFromPalette(*curPalette, qsub8(inoise8(i * deltaValue, (j+ff_y+random8(2)) * deltaHue, ff_z), shiftHue[j]), 255U));
    } 
  }

  for (uint8_t i = 0; i < enlargedObjectNUM; i++) {
    LeapersMove_leaper(i);
    //drawPixelXYF(trackingObjectPosX[i], trackingObjectPosY[i], CHSV(trackingObjectHue[i], 255U, 255U));
    if (trackingObjectPosY[i] >= HEIGHT/4U)
      drawPixelXYF(trackingObjectPosX[i], trackingObjectPosY[i], ColorFromPalette(*curPalette, trackingObjectHue[i]));
  };

  //blurScreen(20);
  ff_y++;
  if (ff_y & 0x01)
    ff_z++;
  
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

#define FLAME_MAX_DY        256 // максимальная вертикальная скорость перемещения языков пламени за кадр.  имеется в виду 256/256 =   1 пиксель за кадр
#define FLAME_MIN_DY        128 // минимальная вертикальная скорость перемещения языков пламени за кадр.   имеется в виду 128/256 = 0.5 пикселя за кадр
#define FLAME_MAX_DX         32 // максимальная горизонтальная скорость перемещения языков пламени за кадр. имеется в виду 32/256 = 0.125 пикселя за кадр
#define FLAME_MIN_DX       (-FLAME_MAX_DX)
#define FLAME_MAX_VALUE     255 // максимальная начальная яркость языка пламени
#define FLAME_MIN_VALUE     176 // минимальная начальная яркость языка пламени

//пришлось изобрести очередную функцию субпиксельной графики. на этот раз бесшовная по ИКСу, работающая в цветовом пространстве HSV и без смешивания цветов
static void wu_pixel_maxV(int16_t item){
  //uint8_t xx = trackingObjectPosX[item] & 0xff, yy = trackingObjectPosY[item] & 0xff, ix = 255 - xx, iy = 255 - yy;
  uint8_t xx = (trackingObjectPosX[item] - (int)trackingObjectPosX[item]) * 255, yy = (trackingObjectPosY[item] - (int)trackingObjectPosY[item]) * 255, ix = 255 - xx, iy = 255 - yy;
  // calculate the intensities for each affected pixel
  uint8_t wu[4] = {WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy),
                   WU_WEIGHT(ix, yy), WU_WEIGHT(xx, yy)};
  // multiply the intensities by the colour, and saturating-add them to the pixels
  for (uint8_t i = 0; i < 4; i++) {    
    uint8_t x1 = (int8_t)(trackingObjectPosX[item] + (i & 1)) % WIDTH; //делаем бесшовный по ИКСу
    uint8_t y1 = (int8_t)(trackingObjectPosY[item] + ((i >> 1) & 1));
    if (y1 < HEIGHT && trackingObjectHue[item] * wu[i] >> 8 >= noise3d[1][x1][y1]){
      noise3d[0][x1][y1] = trackingObjectShift[item];
      shiftValue[y1] = 255U;//saturation;
      noise3d[1][x1][y1] = trackingObjectHue[item] * wu[i] >> 8;
    }
  }  
}
      
static void execStringsFlame(){ // внимание! эффект заточен на бегунок Масштаб с диапазоном от 0 до 255
  int16_t i,j;
  if (loadingFlag){ 
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        setModeSettings(1U+random8(255U), 20U+random8(236U)); // на свякий случай пусть будет от 1 до 255, а не от нуля
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    enlargedObjectNUM = (modes[currentMode].Speed - 1U) / 254.0 * (trackingOBJECT_MAX_COUNT - 1U) + 1U;
    if (enlargedObjectNUM > enlargedOBJECT_MAX_COUNT) enlargedObjectNUM = enlargedOBJECT_MAX_COUNT;
    if (currentMode >= EFF_MATRIX) {
      ff_x = WIDTH * 2.4;
      enlargedObjectNUM = (ff_x > enlargedOBJECT_MAX_COUNT) ? enlargedOBJECT_MAX_COUNT : ff_x;
    }
 
    hue = map8(myScale8(modes[currentMode].Scale+3U),3,10); // минимальная живучесть/высота языка пламени ...ttl
    hue2 = map8(myScale8(modes[currentMode].Scale+3U),6,31); // максимальная живучесть/высота языка пламени ...ttl
    for (i = 0; i < trackingOBJECT_MAX_COUNT; i++) // чистим массив объектов от того, что не похоже на языки пламени
      if (trackingObjectState[i] > 30U || trackingObjectPosY[i] >= HEIGHT || trackingObjectPosX[i] >= WIDTH || trackingObjectPosY[i] <= 0){
        trackingObjectHue[i] = 0U;
        trackingObjectState[i] = random8(20);
      }
    for (i=0; i < WIDTH; i++) // заполняем массив изображения из массива leds обратным преобразованием, которое нихрена не работает
      for (j=0; j < HEIGHT; j++ ) {
        CHSV tHSV = rgb2hsv_approximate(leds[XY(i,j)]);
        noise3d[0][i][j] = tHSV.hue;
        if (tHSV.val > 100U){ // такая защита от пересвета более-менее достаточна
          shiftValue[j] = tHSV.sat;
          if (tHSV.sat < 100U) // для перехода с очень тусклых эффектов, использующих заливку белым или почти белым светом
            noise3d[1][i][j] = tHSV.val / 3U;
          else
            noise3d[1][i][j] = tHSV.val - 32U;
        }
        else
          noise3d[1][i][j] = 0U;
          
        //CRGB tRGB = leds[XY(i,j)];
        //if (tRGB.r + tRGB.g + tRGB.b < 100U) // не пригодилось
        //  noise3d[1][i][j] = 0U;
      }
  }

  // угасание предыдущего кадра
  for (i=0; i < WIDTH; i++)
    for (j=0; j < HEIGHT; j++ )
      noise3d[1][i][j] = (uint16_t)noise3d[1][i][j] * 237U >> 8;

  // цикл перебора языков пламени
  for (i=0; i < enlargedObjectNUM; i++) {
    if (trackingObjectState[i]) { // если ещё не закончилась его жизнь
      wu_pixel_maxV(i);

      j = trackingObjectState[i];
      trackingObjectState[i]--;

      trackingObjectPosX[i] += trackingObjectSpeedX[i];
      trackingObjectPosY[i] += trackingObjectSpeedY[i];

      trackingObjectHue[i] = (trackingObjectState[i] * trackingObjectHue[i] + j / 2) / j;

      // если вышел за верхнюю границу или потух, то и жизнь закончилась
      if (trackingObjectPosY[i] >= HEIGHT || trackingObjectHue[i] < 2U)
        trackingObjectState[i] = 0;

      // если вылез за край матрицы по горизонтали, перекинем на другую сторону
      if (trackingObjectPosX[i] < 0)
        trackingObjectPosX[i] += WIDTH;
      else if (trackingObjectPosX[i] >= WIDTH)
        trackingObjectPosX[i] -= WIDTH;
    }
    else{ // если жизнь закончилась, перезапускаем
      trackingObjectState[i] = random8(hue, hue2);
      trackingObjectShift[i] = (uint8_t)(254U + modes[currentMode].Scale + random8(20U)); // 254 - это шаг в обратную сторону от выбранного пользователем оттенка (стартовый оттенок диапазона)
                                                                                          // 20 - это диапазон из градиента цвета от выбранного пользователем оттенка (диапазон от 254 до 254+20)
      trackingObjectPosX[i] = (float)random(WIDTH * 255U) / 255.;
      trackingObjectPosY[i] = -.9;
      trackingObjectSpeedX[i] = (float)(FLAME_MIN_DX + random8(FLAME_MAX_DX-FLAME_MIN_DX)) / 256.;
      trackingObjectSpeedY[i] = (float)(FLAME_MIN_DY + random8(FLAME_MAX_DY-FLAME_MIN_DY)) / 256.;
      trackingObjectHue[i] = FLAME_MIN_VALUE + random8(FLAME_MAX_VALUE - FLAME_MIN_VALUE + 1U);
      //saturation = 255U;
    }
  }

  //выводим кадр на матрицу
  for (i=0; i<WIDTH; i++)
    for (j=0; j<HEIGHT; j++)
      //hsv2rgb_spectrum(CHSV(noise3d[0][i][j], shiftValue[j], noise3d[1][i][j] * 1.033), leds[XY(i,j)]); // 1.033 - это коэффициент нормализации яркости (чтобы чутка увеличить яркость эффекта в целом)
      hsv2rgb_spectrum(CHSV(noise3d[0][i][j], shiftValue[j], noise3d[1][i][j]), leds[XY(i,j)]);
}
#endif


#ifdef DEF_FIRE_2021
// ============= Эффект Огонь 2021 ===============
// (c) SottNick
// На основе алгоритма https://editor.soulmatelights.com/gallery/546-fire
// by Stepko

#define FIXED_SCALE_FOR_Y 4U // менять нельзя. корректировка скорости ff_x =... подогнана под него

static void Fire2021Routine(){
  if (loadingFlag){ 
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        uint8_t tmp = 1U+random8(89U); // пропускаем белую палитру
        if (tmp > 44U) tmp += 11U;
        setModeSettings(tmp, 42U+random8(155U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    if (modes[currentMode].Scale > 100U) modes[currentMode].Scale = 100U; // чтобы не было проблем при прошивке без очистки памяти
    deltaValue = modes[currentMode].Scale * 0.0899;// /100.0F * ((sizeof(palette_arr) /sizeof(TProgmemRGBPalette16 *))-0.01F));
    if (deltaValue == 3U ||deltaValue == 4U)
      curPalette =  palette_arr[deltaValue]; // (uint8_t)(modes[currentMode].Scale/100.0F * ((sizeof(palette_arr) /sizeof(TProgmemRGBPalette16 *))-0.01F))];
    else
      curPalette = firePalettes[deltaValue]; // (uint8_t)(modes[currentMode].Scale/100.0F * ((sizeof(firePalettes)/sizeof(TProgmemRGBPalette16 *))-0.01F))];
    deltaValue = (modes[currentMode].Scale - 1U) % 11U + 1U;
    if (modes[currentMode].Speed & 0x01){
      ff_x = modes[currentMode].Speed;
      deltaHue2 = FIXED_SCALE_FOR_Y;
    }
    else{
      if (deltaValue > FIXED_SCALE_FOR_Y)
        speedfactor = .4 * (deltaValue - FIXED_SCALE_FOR_Y) + FIXED_SCALE_FOR_Y;
      else
        speedfactor = deltaValue;
      ff_x = round(modes[currentMode].Speed*64./(0.1686*speedfactor*speedfactor*speedfactor - 1.162*speedfactor*speedfactor + 3.6694*speedfactor + 56.394)); // Ааааа! это тупо подбор коррекции. очень приблизитеьный
      deltaHue2 = deltaValue;
    }
    if (ff_x > 255U) 
      ff_x = 255U;
    if (ff_x == 0U) 
      ff_x = 1U;
    step = map(ff_x * ff_x, 1U, 65025U, (deltaHue2-1U)/2U+1U, deltaHue2 * 18U + 44);
    pcnt = map(step, 1U, 255U, 20U, 128U); // nblend 3th param
    deltaValue = 0.7 * deltaValue * deltaValue + 31.3; // ширина языков пламени (масштаб шума Перлина)
    deltaHue2 = 0.7 * deltaHue2 * deltaHue2 + 31.3; // высота языков пламени (масштаб шума Перлина)
  }
  
  ff_y += step; //static uint32_t t += speed;
  for (byte x = 0; x < WIDTH; x++) {
    for (byte y = 0; y < HEIGHT; y++) {
      int16_t Bri = inoise8(x * deltaValue, (y * deltaHue2) - ff_y, ff_z) - (y * (255 / HEIGHT));
      byte Col = Bri;//inoise8(x * deltaValue, (y * deltaValue) - ff_y, ff_z) - (y * (255 / HEIGHT));
      if (Bri < 0) 
        Bri = 0; 
      if (Bri != 0) 
        Bri = 256 - (Bri * 0.2);
      //leds[XY(x, y)] = ColorFromPalette(*curPalette, Col, Bri);
      nblend(leds[XY(x, y)], ColorFromPalette(*curPalette, Col, Bri), pcnt);
    }
  }
  if (!random8())
    ff_z++;
}
#endif


#ifdef DEF_LUMENJER
// =============== Эффект Lumenjer ================
// (c) SottNick

#define DIMSPEED (254U - 500U / WIDTH / HEIGHT)

static void lumenjerRoutine() {
  if (loadingFlag)
  {
    loadingFlag = false;
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings){
        uint8_t tmp = random8(17U); //= random8(19U);
        if (tmp > 2U) tmp += 2U;
        tmp = (uint8_t)(tmp * 5.556 + 3.);
        if (tmp > 100U) tmp = 100U;
        setModeSettings(tmp, 190U+random8(56U));
      }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    
    if (modes[currentMode].Scale > 100) modes[currentMode].Scale = 100; // чтобы не было проблем при прошивке без очистки памяти    
    if (modes[currentMode].Scale > 50) 
      curPalette = firePalettes[(uint8_t)((modes[currentMode].Scale - 50)/50.0F * ((sizeof(firePalettes)/sizeof(TProgmemRGBPalette16 *))-0.01F))];
    else
      curPalette = palette_arr[(uint8_t)(modes[currentMode].Scale/50.0F * ((sizeof(palette_arr)/sizeof(TProgmemRGBPalette16 *))-0.01F))];
    
    deltaHue = -1;
    deltaHue2 = -1;
    //hue = CENTER_X_MAJOR;
    //hue2 = CENTER_Y_MAJOR;
    dimAll(245U);
  }
  //fadeToBlackBy(leds, N_LEDS, 2);
  dimAll(DIMSPEED);

  deltaHue = random8(3) ? deltaHue : -deltaHue;
  deltaHue2 = random8(3) ? deltaHue2 : -deltaHue2;
#if (WIDTH % 2 == 0 && HEIGHT % 2 == 0)
  hue = (WIDTH + hue + (int8_t)deltaHue * (bool)random8(64)) % WIDTH;
#else
  hue = (WIDTH + hue + (int8_t)deltaHue) % WIDTH;
#endif
  hue2 = (HEIGHT + hue2 + (int8_t)deltaHue2) % HEIGHT;

  if (modes[currentMode].Scale == 100U)
    leds[XY(hue, hue2)] += CHSV(random8(), 255U, 255U);
  else
    leds[XY(hue, hue2)] += ColorFromPalette(*curPalette, step++);
}
#endif


#ifdef DEF_CHRISTMAS_TREE
// =========== Christmas Tree ===========
//             © SlingMaster
//           EFF_CHRISTMAS_TREE
//            Новогодняя Елка
//---------------------------------------
static void clearNoiseArr() {
  for (uint8_t x = 0U; x < WIDTH; x++) {
    for (uint8_t y = 0U; y < HEIGHT; y++) {
      noise3d[0][x][y] = 0;
      noise3d[1][x][y] = 0;
    }
  }
}

//---------------------------------------
static void VirtualSnow(byte snow_type) {
  uint8_t posX = random8(WIDTH - 1);
  //const uint8_t maxX = WIDTH - 1;
  static int deltaPos;
  byte delta = (snow_type == 3) ? 0 : 1;
  for (uint8_t x = delta; x < WIDTH - delta; x++) {

    // заполняем случайно верхнюю строку
    if ((noise3d[0][x][HEIGHT - 2] == 0U) &&  (posX == x) && (random8(0, 2) == 0U)) {
      noise3d[0][x][HEIGHT-1] = 1;
    } else {
      noise3d[0][x][HEIGHT-1] = 0;
    }

    for (uint8_t y = 0U; y < HEIGHT - 1; y++) {
      switch (snow_type) {
        case 0:
          noise3d[0][x][y] = noise3d[0][x][y + 1];
          deltaPos = 0;
          break;
        case 1:
        case 2:
          noise3d[0][x][y] = noise3d[0][x][y + 1];
          deltaPos = 1 - random8(2);
          break;
        default:
          deltaPos = -1;
          if ((x == 0 ) & (y == 0 ) & (random8(2) == 0U)) {
            noise3d[0][WIDTH - 1][random8(CENTER_Y_MAJOR / 2, HEIGHT - CENTER_Y_MAJOR / 4)] = 1;
          }
          if (x > WIDTH - 2) {
            noise3d[0][WIDTH - 1][y] = 0;
          }
          if (x < 1)  {
            noise3d[0][x][y] = noise3d[0][x][y + 1];
          } else {
            noise3d[0][x - 1][y] = noise3d[0][x][y + 1];
          }
          break;
      }

      if (noise3d[0][x][y] > 0) {
        if (snow_type < 3) {
          if (y % 2 == 0U) {
            leds[XY(x - ((x > 0) ? deltaPos : 0), y)] = CHSV(160, 5U, random8(200U, 240U));
          } else {
            leds[XY(x + deltaPos, y)] = CHSV(160, 5U,  random8(200U, 240U));
          }
        } else {
          leds[XY(x, y)] = CHSV(160, 5U,  random8(200U, 240U));
        }
      }
    }
  }
}

//---------------------------------------
static void GreenTree(uint8_t tree_h) {
  hue = floor(step / 32) * 32U;

  for (uint8_t x = 0U; x < WIDTH + 1 ; x++) {
    if (x % 8 == 0) {
      if (modes[currentMode].Scale < 60) {
        // nature -----
        DrawLine(x - 1U - deltaValue, floor(tree_h * 0.70), x + 1U - deltaValue, floor(tree_h * 0.70), 0x002F00);
        DrawLine(x - 1U - deltaValue, floor(tree_h * 0.55), x + 1U - deltaValue, floor(tree_h * 0.55), 0x004F00);
        DrawLine(x - 2U - deltaValue, floor(tree_h * 0.35), x + 2U - deltaValue, floor(tree_h * 0.35), 0x005F00);
        DrawLine(x - 2U - deltaValue, floor(tree_h * 0.15), x + 2U - deltaValue, floor(tree_h * 0.15), 0x007F00);

        drawPixelXY(x - 3U - deltaValue, floor(tree_h * 0.15), 0x001F00);
        drawPixelXY(x + 3U - deltaValue, floor(tree_h * 0.15), 0x001F00);
        if ((x - deltaValue ) >= 0) {
          gradientVertical( x - deltaValue, 0U, x - deltaValue, tree_h, 90U, 90U, 190U, 64U, 255U);
        }
      } else {
        // holiday -----
        drawPixelXY(x - 1 - deltaValue, floor(tree_h * 0.6), CHSV(step, 255U, 128 + random8(128)));
        drawPixelXY(x + 1 - deltaValue, floor(tree_h * 0.6), CHSV(step, 255U, 128 + random8(128)));

        drawPixelXY(x - deltaValue, floor(tree_h * 0.4), CHSV(step, 255U, 200U));

        drawPixelXY(x - deltaValue, floor(tree_h * 0.2), CHSV(step, 255U, 190 + random8(65)));
        drawPixelXY(x - 2 - deltaValue, floor(tree_h * 0.25), CHSV(step, 255U, 96 + random8(128)));
        drawPixelXY(x + 2 - deltaValue, floor(tree_h * 0.25), CHSV(step, 255U, 96 + random8(128)));

        drawPixelXY(x - 2 - deltaValue, 1U, CHSV(step, 255U, 200U));
        drawPixelXY(x - deltaValue, 0U, CHSV(step, 255U, 250U));
        drawPixelXY(x + 2 - deltaValue, 1U, CHSV(step, 255U, 200U));
        if ((x - deltaValue) >= 0) {
          gradientVertical( x - deltaValue, floor(tree_h * 0.75), x - deltaValue, tree_h,  hue, hue, 250U, 0U, 128U);
        }
      }
    }
  }
}

//---------------------------------------
static void ChristmasTree() {
  static uint8_t tree_h = HEIGHT;
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), 10U + random8(128));
    }
#endif
    loadingFlag = false;
    clearNoiseArr();
    deltaValue = 0;
    step = deltaValue;
    ledsClear(); // esphome: FastLED.clear();

    if (HEIGHT > 16) {
      tree_h = 16;
    }
  }

  if (HEIGHT > 16) {
    if (modes[currentMode].Scale < 60) {
      gradientVertical(0, 0, WIDTH, HEIGHT, 160, 160, 64, 128, 255U);
    } else {
      ledsClear(); // esphome: FastLED.clear();
    }
  } else {
    ledsClear(); // esphome: FastLED.clear();
  }
  GreenTree(tree_h);

  if (modes[currentMode].Scale < 60) {
    VirtualSnow(1);
  }
  if (modes[currentMode].Scale > 30) {
    deltaValue++;
  }
  if (deltaValue >= 8) {
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
  uint8_t saturation;
  uint8_t delta;
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed 210
      setModeSettings(random8(100U), random8(200U));
    }
    #endif

    loadingFlag = false;
    deltaValue = 0;
    step = deltaValue;
    ledsClear(); // esphome: FastLED.clear();
  }

  hue = floor(step / 32) * 32U;
  dimAll(180);
  // ------
  saturation = 255U;
  delta = 0;
  for (uint8_t x = 0U; x < WIDTH + 1 ; x++) {
    if (x % 8 == 0) {
      gradientVertical( x - deltaValue, floor(HEIGHT * 0.75), x + 1U - deltaValue, HEIGHT,  hue, hue + 2, 250U, 0U, 255U);
      if (modes[currentMode].Scale > 50) {
        delta = random8(200U);
      }
      drawPixelXY(x - 2 - deltaValue, floor(HEIGHT * 0.7), CHSV(step, saturation - delta, 128 + random8(128)));
      drawPixelXY(x + 2 - deltaValue, floor(HEIGHT * 0.7), CHSV(step, saturation, 128 + random8(128)));

      drawPixelXY(x - deltaValue, floor(HEIGHT * 0.6), CHSV(hue, 255U, 190 + random8(65)));
      if (modes[currentMode].Scale > 50) {
        delta = random8(200U);
      }
      drawPixelXY(x - 1 - deltaValue, CENTER_Y_MINOR, CHSV(step, saturation, 128 + random8(128)));
      drawPixelXY(x + 1 - deltaValue, CENTER_Y_MINOR, CHSV(step, saturation - delta, 128 + random8(128)));

      drawPixelXY(x - deltaValue, floor(HEIGHT * 0.4), CHSV(hue, 255U, 200U));
      if (modes[currentMode].Scale > 50) {
        delta = random8(200U);
      }
      drawPixelXY(x - 2 - deltaValue, floor(HEIGHT * 0.3), CHSV(step, saturation - delta, 96 + random8(128)));
      drawPixelXY(x + 2 - deltaValue, floor(HEIGHT * 0.3), CHSV(step, saturation, 96 + random8(128)));

      gradientVertical( x - deltaValue, 0U, x + 1U - deltaValue, floor(HEIGHT * 0.25),  hue + 2, hue, 0U, 250U, 255U);

      if (modes[currentMode].Scale > 50) {
        drawPixelXY(x + 3 - deltaValue, HEIGHT - 3U, CHSV(step, 255U, 255U));
        drawPixelXY(x - 3 - deltaValue, CENTER_Y_MINOR, CHSV(step, 255U, 255U));
        drawPixelXY(x + 3 - deltaValue, 2U, CHSV(step, 255U, 255U));
      }
    }
  }
  // ------
  deltaValue++;
  if (deltaValue >= 8) {
    deltaValue = 0;
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
      // scale | speed
      setModeSettings(random(10U, 90U), 128);
    }
    loadingFlag = false;
    FPSdelay = 10U;
    deltaValue = 0;
    #endif
  }

  if (modes[currentMode].Scale > 50) {
    if (FPSdelay > 48) deltaValue = 0;
    if (FPSdelay < 5) deltaValue = 1;

    if (deltaValue == 1) {
      FPSdelay++;
    } else {
      FPSdelay--;
    }
    blur2d(WIDTH, HEIGHT, 16);
    
  } else {
    FPSdelay = 20;
    dimAll(240U);
  }
   //LOG.printf_P(PSTR("| deltaValue • %03d | fps %03d\n"), deltaValue, FPSdelay);
  for (byte i = 8; i--;) {
    leds[XY(beatsin8(12 + i, 0, WIDTH - 1), beatsin8(15 - i, 0, HEIGHT - 1))] = CHSV(beatsin8(12, 0, 255), 255, (255 - FPSdelay * 2));
  }
}
#endif


#ifdef DEF_COLORED_PYTHON
// ============ Colored Python ============
//      base code WavingCell from © Stepko
//       Adaptation & modefed © alvikskor
//            Кольоровий Пітон
// --------------------------------------

static uint32_t color_timer = millis();

static void Colored_Python() {
  if (loadingFlag) {
      #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      if (selectedSettings) {
          //                     scale | speed
          setModeSettings(random8(100U), random8(1, 255U));
      }
      #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      loadingFlag = false;
      step = 0;
  }
  uint16_t  t = millis() / (128 - (modes[currentMode].Speed / 2));
  uint8_t palette_number = modes[currentMode].Scale / 10;
  uint8_t thickness;
  if (palette_number < 9)
      step = palette_number;
  else
      if (millis()- color_timer > 30000){
        color_timer = millis();
        step++;
        if(step > 8) step = 0;
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
  switch (modes[currentMode].Scale % 5){
      case 0: thickness = 5; break;
      case 1: thickness = 10; break;
      case 2: thickness = 20; break;
      case 3: thickness = 30; break;
      case 4: thickness = 40; break;
  }
  for(byte x =0; x < WIDTH; x++){
    for(byte y =0; y < HEIGHT; y++){
      leds[XY(x,y)]=ColorFromPalette(currentPalette,((sin8((x*thickness)+sin8(y*5+t*5))+cos8(y*10))+1)+t*(modes[currentMode].Speed%10)); // HeatColors_p -палитра, t*scale/10 -меняет скорость движения вверх, sin8(x*20) -меняет ширину рисунка
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
      // scale | speed
      setModeSettings(random(25U, 90U), random(5U, 250U));
    }
    #endif
    loadingFlag = false;
    FPSdelay = 80U;
    ledsClear(); // esphome: FastLED.clear();
  }

  int a = millis() / floor((255 - modes[currentMode].Speed) / 10);
  hue = floor(modes[currentMode].Scale / 14);
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      int index = XY(x, y);
      uint8_t color1 = pgm_read_byte(&exp_gamma[sin8(cos8((x * 7 +a/5)) - cos8((y * 10) +a/3)/4+a )]);
      uint8_t color2 = pgm_read_byte(&exp_gamma[(sin8(x * 16 + a / 3) + cos8(y * 8 + a / 2)) / 2]);
      uint8_t color3 = pgm_read_byte(&exp_gamma[sin8(cos8(x * 8 + a / 3) + sin8(y * 8 + a / 4) + a)]);
      if (hue == 0) {
        leds[index].b = color3 >> 2;
        leds[index].g = color2;
        leds[index].r = 0;
      } else if (hue == 1) {
        leds[index].b = color1;
        leds[index].g = 0;
        leds[index].r = color3 >> 2;
      } else if (hue == 2) {
        leds[index].b = 0;
        leds[index].g = color1 >> 2;
        leds[index].r = color3;
      } else if (hue == 3) {
        leds[index].b = color1;
        leds[index].g = color2;
        leds[index].r = color3;
      } else if (hue == 4) {
        leds[index].b = color3;
        leds[index].g = color1;
        leds[index].r = color2;
      } else if (hue == 5) {
        leds[index].b = color2;
        leds[index].g = color3;
        leds[index].r = color1;
      } else if (hue >= 6) {
        leds[index].b = color3;
        leds[index].g = color1;
        leds[index].r = color2;
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
static void DropInWater() {
#define Sat (255)
#define MaxRad WIDTH + HEIGHT
  static int rad[(HEIGHT + WIDTH) / 8];
  static byte posx[(HEIGHT + WIDTH) / 8], posy[(HEIGHT + WIDTH) / 8];

  if (loadingFlag) {

#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      // scale | speed
      setModeSettings(random(0U, 100U), random(160U, 215U));
    }
#endif
    loadingFlag = false;
    hue = modes[currentMode].Scale * 2.55;
    for (int i = 0; i < ((HEIGHT + WIDTH) / 8) - 1; i++)  {
      posx[i] = random(WIDTH - 1);
      posy[i] = random(HEIGHT - 1);
      rad[i] = random(-1, MaxRad);
    }
  }
  fill_solid(currentPalette, 16, CHSV(hue, Sat, 230));
  currentPalette[10] = CHSV(hue, Sat - 60, 255);
  currentPalette[9] = CHSV(hue, 255 - Sat, 210);
  currentPalette[8] = CHSV(hue, 255 - Sat, 210);
  currentPalette[7] = CHSV(hue, Sat - 60, 255);
  fillAll(ColorFromPalette(currentPalette, 1));

  for (uint8_t i = ((HEIGHT + WIDTH) / 8 - 1); i > 0 ; i--) {
    drawCircle(posx[i], posy[i], rad[i], ColorFromPalette(currentPalette, (256 / 16) * 8.5 - rad[i]));
    drawCircle(posx[i], posy[i], rad[i] - 1, ColorFromPalette(currentPalette, (256 / 16) * 7.5 - rad[i]));
    if (rad[i] >= MaxRad) {
      rad[i] = 0; // random(-1, MaxRad);
      posx[i] = random(WIDTH);
      posy[i] = random(HEIGHT);
    } else {
      rad[i]++;
    }
  }
  if (modes[currentMode].Scale == 100) {
    hue++;
  }
  blur2d(WIDTH, HEIGHT, 64);
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
static const uint8_t  level = 160;
static const uint8_t  low_level = 110;
static const uint8_t *ptr  = anim;                     // Current pointer into animation data
static const uint8_t  w    = 7;                        // image width
static const uint8_t  h    = 15;                       // image height
static uint8_t        img[w * h];                      // Buffer for rendering image
static uint8_t        deltaX = CENTER_X_MAJOR - 3;     // position img
static uint8_t last_brightness;

static void FeatherCandleRoutine() {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
  if (selectedSettings) {
    // brightness | scale | speed
    // { 21, 220,  40}
    setModeSettings(1U + random8(99U), 190U + random8(65U));
  }
#endif
  if (loadingFlag) {
    ledsClear(); // esphome: FastLED.clear();
    hue = 0;
    trackingObjectState[0] = low_level;
    trackingObjectState[1] = low_level;
    trackingObjectState[2] = low_level;
    trackingObjectState[4] = CENTER_X_MAJOR;
    loadingFlag = false;
  }

  uint8_t a = pgm_read_byte(ptr++);     // New frame X1/Y1
  if (a >= 0x90) {                      // EOD marker? (valid X1 never exceeds 8)
    ptr = anim;                         // Reset animation data pointer to start
    a   = pgm_read_byte(ptr++);         // and take first value
  }
  uint8_t x1 = a >> 4;                  // X1 = high 4 bits
  uint8_t y1 = a & 0x0F;                // Y1 = low 4 bits
  a  = pgm_read_byte(ptr++);            // New frame X2/Y2
  uint8_t x2 = a >> 4;                  // X2 = high 4 bits
  uint8_t y2 = a & 0x0F;                // Y2 = low 4 bits

  // Read rectangle of data from anim[] into portion of img[] buffer
  for (uint8_t y = y1; y <= y2; y++)
    for (uint8_t x = x1; x <= x2; x++) {
      img[y * w + x] = pgm_read_byte(ptr++);
    }
  int i = 0;
  uint8_t color = (modes[currentMode].Scale - 1U) * 2.57;

  // draw flame -------------------
  for (uint8_t y = 1; y < h; y++) {
    if ((HEIGHT < 15) || (WIDTH < 9)) {
      // for small matrix -----
      if (y % 2 == 0) {
        leds[XY(CENTER_X_MAJOR - 1, 7)] = CHSV(color, 255U, 55 + random8(200));
        leds[XY(CENTER_X_MAJOR, 6)] = CHSV(color, 255U, 160 + random8(90));
        leds[XY(CENTER_X_MAJOR + 1, 6)] = CHSV(color, 255U, 205 + random8(50));
        leds[XY(CENTER_X_MAJOR - 1, 5)] = CHSV(color, 255U, 155 + random8(100));
        leds[XY(CENTER_X_MAJOR, 5)] = CHSV(color - 10U , 255U, 120 + random8(130));
        leds[XY(CENTER_X_MAJOR, 4)] = CHSV(color - 10U , 255U, 100 + random8(120));
        DrawLine(0, 2U, WIDTH - 1, 2U, 0x000000);
      }
    } else {
      for (uint8_t x = 0; x < w; x++) {
        uint8_t brightness = img[i];
        leds[XY(deltaX + x, y)] = CHSV(brightness > 240 ? color : color - 10U , 255U, brightness);
        i++;
      }
    }

    // draw body FeatherCandle ------
    if (y <= 3) {
      if (y % 2 == 0) {
        gradientVertical(0, 0, WIDTH, 2, color, color, 48, 128, 20U);
      }
    }

    // drops of wax move -------------
    switch (hue ) {
      case 0:
        if (trackingObjectState[0] < level) {
          trackingObjectState[0]++;
        }
        break;
      case 1:
        if (trackingObjectState[0] > low_level) {
          trackingObjectState[0] --;
        }
        if (trackingObjectState[1] < level) {
          trackingObjectState[1] ++;
        }
        break;
      case 2:
        if (trackingObjectState[1] > low_level) {
          trackingObjectState[1] --;
        }
        if (trackingObjectState[2] < level) {
          trackingObjectState[2] ++;
        }
        break;
      case 3:
        if (trackingObjectState[2] > low_level) {
          trackingObjectState[2] --;
        } else {
          hue++;
          // set random position drop of wax
          trackingObjectState[4] = CENTER_X_MAJOR - 3 + random8(6);
        }
        break;
    }

    if (hue > 3) {
      hue++;
    } else {
      // LOG.printf_P(PSTR("[0] = %03d | [1] = %03d | [2] = %03d \n\r"), trackingObjectState[0], trackingObjectState[1], trackingObjectState[2]);
      if (hue < 2) {
        leds[XY(trackingObjectState[4], 2)] = CHSV(50U, 20U, trackingObjectState[0]);
      }
      if ((hue == 1) || (hue == 2)) {
        leds[XY(trackingObjectState[4], 1)] = CHSV(50U, 15U, trackingObjectState[1]); // - 10;
      }
      if (hue > 1) {
        leds[XY(trackingObjectState[4], 0)] = CHSV(50U, 5U, trackingObjectState[2]); // - 20;
      }
    }
  }

  // next -----------------
  if ((trackingObjectState[0] == level) || (trackingObjectState[1] == level) || (trackingObjectState[2] == level)) {
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
  const uint8_t DELAY_SECOND_EXPLOSION = HEIGHT * 0.25;
  uint8_t horizont = 1U; // HEIGHT * 0.2;
  const int8_t STEP = 255 / HEIGHT;
  uint8_t firstColor = random8(255);
  uint8_t secondColor = 0;
  uint8_t saturation = 255U;
  switch (f_type) {
    case 0:
      secondColor =  random(50U, 255U);
      saturation = random(245U, 255U);
      break;
    case 1: /* сакура */
      firstColor = random(210U, 230U);
      secondColor = random(65U, 85U);
      saturation = 255U;
      break;
    case 2: /* день Независимости */
      firstColor = random(160U, 170U);
      secondColor = random(25U, 50U);
      saturation = 255U;
      break;
    default: /* фризантемы */
      firstColor = random(30U, 40U);
      secondColor = random(25U, 50U);
      saturation = random(128U, 255U);
      break;
  }
  if ((timeline > HEIGHT - 1 ) & (timeline < HEIGHT * 1.75)) {
    for (uint8_t x = 0U; x < WIDTH; x++) {
      for (uint8_t y =  horizont; y < HEIGHT - 1; y++) {
        noise3d[0][x][y] = noise3d[0][x][y + 1];
        uint8_t bri = y * STEP;
        if (noise3d[0][x][y] > 0) {
          if (timeline > (HEIGHT + DELAY_SECOND_EXPLOSION) ) {
            /* second explosion */
            drawPixelXY((x - 2 + random8(4)), y - 1, CHSV(secondColor + random8(16), saturation, bri));
          }
          if (timeline < ((HEIGHT - DELAY_SECOND_EXPLOSION) * 1.75) ) {
            /* first explosion */
            drawPixelXY(x, y, CHSV(firstColor, 255U, bri));
          }
        } else {
          // drawPixelXY(x, y, CHSV(175, 255U, floor((255 - bri) / 4)));
        }
      }
    }
    uint8_t posX = random8(WIDTH);
    for (uint8_t x = 0U; x < WIDTH; x++) {
      // заполняем случайно верхнюю строку
      if (posX == x) {
        if (step % 2 == 0) {
          noise3d[0][x][HEIGHT - 1U] = 1;
        } else {
          noise3d[0][x][HEIGHT - 1U]  = 0;
        }
      } else {
        noise3d[0][x][HEIGHT - 1U]  = 0;
      }
    }
  }
}

// --------------------------------------
static void Firework() {
  const uint8_t MAX_BRIGHTNESS = 40U;            /* sky brightness */
  const uint8_t DOT_EXPLOSION = HEIGHT * 0.95;
  const uint8_t HORIZONT = HEIGHT * 0.25;
  const uint8_t DELTA = 1U;                      /* центровка по вертикали */
  const float stepH = HEIGHT / 128.0;
  const uint8_t FPS_DELAY = 20U;
  //const uint8_t STEP = 3U;
  const uint8_t skyColor = 156U;
  uint8_t sizeH;

  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      // scale | speed
      setModeSettings(1U + random8(100U), 1U + random8(250U));
    }
#endif
    loadingFlag = false;
    deltaHue2 = 0;
    FPSdelay = 255U;
    ledsClear(); // esphome: FastLED.clear();
    step = 0U;
    deltaHue2 = floor(modes[currentMode].Scale / 26);
    hue = 48U;            // skyBright
    if (modes[currentMode].Speed > 85U) {
      sizeH = HORIZONT;
      FPSdelay = FPS_DELAY;
    }
  }

  if (FPSdelay > 128U) {
    /* вечерело */
    FPSdelay--;
    sizeH = (FPSdelay - 128U) * stepH;
    // LOG.printf_P(PSTR("• [%03d] | %03d | %0.2f | \n"), FPSdelay, stepH, sizeH);
    dimAll(200);

    if (ORIENTATION % 2 == 0) {    // if (STRIP_DIRECTION % 2 == 0) {
      gradientDownTop( 0, CHSV(skyColor, 255U, floor(FPSdelay / 2.2)), sizeH, CHSV(skyColor, 255U, 2U));
    } else {
      gradientVertical(0, 0, WIDTH, sizeH, skyColor, skyColor, floor(FPSdelay / 2.2), 2U, 255U);
    }

    if (sizeH > HORIZONT) return;
    if (sizeH == HORIZONT )  FPSdelay = FPS_DELAY;
  }

  if (step > DOT_EXPLOSION ) {
    blurScreen(beatsin8(3, 64, 80));
  }
  if (step == DOT_EXPLOSION - 1) {
    /* включаем фазу затухания */
    FPSdelay = 70;
  }
  if (step > CENTER_Y_MAJOR) {
    dimAll(140);
  } else {
    dimAll(100);
  }


  /* ============ draw sky =========== */
  if (modes[currentMode].Speed < 180U) {
    if (ORIENTATION % 2 == 0) {     // if (STRIP_DIRECTION % 2 == 0) {
      gradientDownTop( 0, CHSV(skyColor, 255U, hue ), HORIZONT, CHSV(skyColor, 255U, 0U ));
    } else {
      gradientVertical(0, 0, WIDTH, HORIZONT, skyColor, skyColor, hue + 1, 0U, 255U);
    }
  }

  /* deltaHue2 - Firework type */
  VirtualExplosion(deltaHue2, step);

  if ((step > DOT_EXPLOSION ) & (step < HEIGHT * 1.5)) {
    /* фаза взрыва */
    FPSdelay += 5U;
  }
  const uint8_t rows = (HEIGHT + 1) / 3U;
  deltaHue = floor(modes[currentMode].Speed / 64) * 64;
  if (step > CENTER_Y_MAJOR) {
    bool dir = false;
    for (uint8_t y = 0; y < rows; y++) {
      /* сдвигаем слои / эмитация разлета */
      for (uint8_t x = 0U ; x < WIDTH; x++) {
        if (dir) {  // <==
          drawPixelXY(x - 1, y * 3 + DELTA, getPixColorXY(x, y * 3 + DELTA));
        } else {    // ==>
          drawPixelXY(WIDTH - x, y * 3 + DELTA, getPixColorXY(WIDTH - x - 1, y * 3 + DELTA));
        }
      }
      dir = !dir;
      /* --------------------------------- */
    }
  }

  /* ========== фаза полета ========== */
  if (step < DOT_EXPLOSION ) {
    FPSdelay ++;
    if (HEIGHT < 20) {
      FPSdelay ++;
    }
    /* закоментируйте следующие две строки если плоская лампа
      подсветка заднего фона */
    if (custom_eff == 1) {
      DrawLine(0U, 0U, 0U, HEIGHT - step, CHSV(skyColor, 255U, 32U));
      DrawLine(WIDTH - 1, 0U, WIDTH - 1U, HEIGHT - step, CHSV(skyColor, 255U, 32U));
    }
    /* ------------------------------------------------------ */

    uint8_t saturation = (step > (DOT_EXPLOSION - 2U)) ? 192U : 20U;
    //uint8_t rndPos = deltaHue2;  //uint8_t rndPos = 3U * deltaHue2 * 0.5;
    drawPixelXY(CENTER_X_MINOR + deltaHue2, step,  CHSV(50U, saturation, 80U));                 // first
    drawPixelXY(CENTER_X_MAJOR - deltaHue2, step - HORIZONT,  CHSV(50U, saturation, 80U));  // second
    /* sky brightness */
    if (hue > 2U) {
      hue -= 1U;
    }
  }
  if (step > HEIGHT * 1.25) {
    /* sky brightness */
    if (hue < MAX_BRIGHTNESS) {
      hue += 2U;
    }
  }

  if (step >= (HEIGHT * 2.5)) {
    step = 0U;
    FPSdelay = FPS_DELAY;
    if (modes[currentMode].Scale <= 1) {
      deltaHue2++;
    }
    if (deltaHue2 >= 4U) deltaHue2 = 0U;  // next Firework type
  }
  //  LOG.printf_P(PSTR("• [%03d] | %03d | sky Bright • [%03d]\n"), step, FPSdelay, hue);
  step ++;
}
#endif


#ifdef DEF_FIREWORK_2
//---------- Эффект "Фейерверк" Салют ---
//адаптация и переписал - kostyamat
//https://gist.github.com/jasoncoon/0cccc5ba7ab108c0a373
//https://github.com/marcmerlin/FastLED_NeoMatrix_SmartMatrix_LEDMatrix_GFX_Demos/blob/master/FastLED/FireWorks2/FireWorks2.ino

  #define MODEL_BORDER (HEIGHT - 4U)  // как далеко за экран может вылетить снаряд, если снаряд вылетает за экран, то всышка белого света (не особо логично)
  #define MODEL_WIDTH  (MODEL_BORDER + WIDTH  + MODEL_BORDER) // не трогать, - матиматика
  #define MODEL_HEIGHT (MODEL_BORDER + HEIGHT + MODEL_BORDER) // -//-
  #define PIXEL_X_OFFSET ((MODEL_WIDTH  - WIDTH ) / 2) // -//-
  #define PIXEL_Y_OFFSET ((MODEL_HEIGHT - HEIGHT) / 2) // -//-

  #define SPARK 8U // максимальное количество снарядов
  #define NUM_SPARKS WIDTH // количество разлетающихся петард (частей снаряда)
  const saccum78 gGravity = 10;
  const fract8  gBounce = 127;
  const fract8  gDrag = 255;

  typedef struct _DOTS_STORE {
    accum88 gBurstx;
    accum88 gBursty;
    saccum78 gBurstxv;
    saccum78 gBurstyv;
    CRGB gBurstcolor;
    bool gSkyburst = false;
  } DOTS_STORE;
  static DOTS_STORE store[SPARK];

  static CRGB& piXY(byte x, byte y);

  class Dot {    // класс для создания снарядов и питард
    public:
      byte    show;
      byte    theType;
      accum88 x;
      accum88 y;
      saccum78 xv;
      saccum78 yv;
      accum88 r;
      CRGB color;

      Dot() {
        show = 0;
        theType = 0;
        x =  0;
        y =  0;
        xv = 0;
        yv = 0;
        r  = 0;
        color.setRGB( 0, 0, 0);
      }

      void Draw()
      {
        if( !show) return;
        byte ix, xe, xc;
        byte iy, ye, yc;
        screenscale( x, MODEL_WIDTH, ix, xe);
        screenscale( y, MODEL_HEIGHT, iy, ye);
        yc = 255 - ye;
        xc = 255 - xe;

        CRGB c00 = CRGB( dim8_video( scale8( scale8( color.r, yc), xc)),
                         dim8_video( scale8( scale8( color.g, yc), xc)),
                         dim8_video( scale8( scale8( color.b, yc), xc))
                         );
        CRGB c01 = CRGB( dim8_video( scale8( scale8( color.r, ye), xc)),
                         dim8_video( scale8( scale8( color.g, ye), xc)),
                         dim8_video( scale8( scale8( color.b, ye), xc))
                         );

        CRGB c10 = CRGB( dim8_video( scale8( scale8( color.r, yc), xe)),
                         dim8_video( scale8( scale8( color.g, yc), xe)),
                         dim8_video( scale8( scale8( color.b, yc), xe))
                         );
        CRGB c11 = CRGB( dim8_video( scale8( scale8( color.r, ye), xe)),
                         dim8_video( scale8( scale8( color.g, ye), xe)),
                         dim8_video( scale8( scale8( color.b, ye), xe))
                         );

        piXY(ix, iy) += c00;
        piXY(ix, iy + 1) += c01;
        piXY(ix + 1, iy) += c10;
        piXY(ix + 1, iy + 1) += c11;
      }

      void Move(byte num, bool Flashing)
      {
        if( !show) return;
        yv -= gGravity;
        xv = scale15by8_local( xv, gDrag);
        yv = scale15by8_local( yv, gDrag);

        if( theType == 2) {
          xv = scale15by8_local( xv, gDrag);
          yv = scale15by8_local( yv, gDrag);
          color.nscale8( 255);
          if( !color) {
            show = 0;
          }
        }
        // if we'd hit the ground, bounce
        if( yv < 0 && (y < (-yv)) ) {
          if( theType == 2 ) {
            show = 0;
          } else {
            yv = -yv;
            yv = scale15by8_local( yv, gBounce);
            if( yv < 500 ) {
              show = 0;
            }
          }
        }
        if (yv < -300) { // && (!(oyv < 0)) ) {
          // pinnacle
          if( theType == 1 ) {

            if( (y > (uint16_t)(0x8000)) && (random8() < 32) && Flashing) {
              // boom
              LEDS.showColor( CRGB::Gray);
              LEDS.showColor( CRGB::Black);
            }

            show = 0;

            store[num].gSkyburst = true;
            store[num].gBurstx = x;
            store[num].gBursty = y;
            store[num].gBurstxv = xv;
            store[num].gBurstyv = yv;
            store[num].gBurstcolor = CRGB(random8(), random8(), random8());
          }
        }
        if( theType == 2) {
          if( ((xv >  0) && (x > xv)) ||
              ((xv < 0 ) && (x < (0xFFFF + xv))) )  {
            x += xv;
          } else {
            show = 0;
          }
        } else {
          x += xv;
        }
        y += yv;

      }

      void GroundLaunch()
      {
        yv = 600 + random16(400 + (25 * HEIGHT));
        if(yv > 1200) yv = 1200;
        xv = (int16_t)random16(600) - (int16_t)300;
        y = 0;
        x = 0x8000;
        color = CHSV(0, 0, 130); // цвет запускаемого снаряда
        show = 1;
      }

      void Skyburst( accum88 basex, accum88 basey, saccum78 basedv, CRGB& basecolor, uint8_t dim)
      {
        yv = (int16_t)0 + (int16_t)random16(1500) - (int16_t)500;
        xv = basedv + (int16_t)random16(2000) - (int16_t)1000;
        y = basey;
        x = basex;
        color = basecolor;
        //EffectMath::makeBrighter(color, 50);
        color *= dim; //50;
        theType = 2;
        show = 1;
      }

      //  CRGB &piXY(byte x, byte y);

      int16_t scale15by8_local( int16_t i, fract8 _scale )
      {
        int16_t result;
        result = (int32_t)((int32_t)i * _scale) / 256;
        return result;
      };

      void screenscale(accum88 a, byte N, byte &screen, byte &screenerr)
      {
        byte ia = a >> 8;
        screen = scale8(ia, N);
        byte m = screen * (256 / N);
        screenerr = (ia - m) * scale8(255, N);
        return;
      };
  };

  static uint16_t launchcountdown[SPARK];
  //bool flashing = true; // нахрен эти вспышки прямо в коде false напишу
  static Dot gDot[SPARK];
  static Dot gSparks[NUM_SPARKS];

  static CRGB overrun;
  static CRGB& piXY(byte x, byte y) {
    x -= PIXEL_X_OFFSET;
    //x = (x - PIXEL_X_OFFSET) % WIDTH; // зацикливаем поле по иксу
    y -= PIXEL_Y_OFFSET;
    if( x < WIDTH && y < HEIGHT) {
      return leds[XY(x, y)];
    } else
      //return empty; // fixed //  CRGB empty = CRGB(0,0,0);
      return overrun;//CRGB(0,0,0);
  }

  static void sparkGen() {
    for (byte c = 0; c < enlargedObjectNUM; c++) { // modes[currentMode].Scale / хз
      if( gDot[c].show == 0 ) {
        if( launchcountdown[c] == 0) {
          gDot[c].GroundLaunch();
          gDot[c].theType = 1;
          launchcountdown[c] = random16(1200 - modes[currentMode].Speed*4) + 1;
        } else {
          launchcountdown[c] --;
        }
      }
     if( store[c].gSkyburst) {
       store[c].gBurstcolor = CHSV(random8(), 200, 100);
       store[c].gSkyburst = false;
       byte nsparks = random8( NUM_SPARKS / 2, NUM_SPARKS + 1);
       for( byte b = 0; b < nsparks; b++) {
         gSparks[b].Skyburst( store[c].gBurstx, store[c].gBursty, store[c].gBurstyv, store[c].gBurstcolor, pcnt);
       }
     }
  }

  //myLamp.blur2d(20);
  }

static void fireworksRoutine()
{
  if (loadingFlag)
  {
    loadingFlag = false;
    enlargedObjectNUM = (modes[currentMode].Scale - 1U) / 99.0 * (SPARK - 1U) + 1U;
    if (enlargedObjectNUM > SPARK) enlargedObjectNUM = SPARK;

    for (byte c = 0; c < SPARK; c++)
      launchcountdown[c] = 0;
  }

  //random16_add_entropy(analogRead(A0));
  pcnt = beatsin8(100, 20, 100);
  if (hue++ % 10 == 0U){//  EVERY_N_MILLIS(EFFECTS_RUN_TIMER * 10) {
    deltaValue = random8(25, 50);
  }
  //  EVERY_N_MILLIS(10) {//странный интервал
    fadeToBlackBy(leds, NUM_LEDS, deltaValue);
    sparkGen();
    //memset8( leds, 0, NUM_LEDS * 3);

    for (byte a = 0; a < enlargedObjectNUM; a++) { //modes[currentMode].Scale / хз
      gDot[a].Move(a, false);//flashing);
      gDot[a].Draw();
    }
    for(byte b = 0; b < NUM_SPARKS; b++) {
      gSparks[b].Move(0, false);//flashing);
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
  const float SIZE = 0.4;
  const uint8_t h = floor(SIZE * HEIGHT);
  uint8_t posX = 0;
  //const uint8_t topPos  = HEIGHT - h;
  const uint8_t route = HEIGHT - h - 1;
  const uint8_t STEP = 18U;
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                          scale | speed 210
      setModeSettings(15U + random8(225U), random8(255U));
    }
#endif
    loadingFlag = false;
    pcnt = 0;
    deltaHue2 = 0;
    hue2 = 0;

    ledsClear(); // esphome: FastLED.clear();
    hue = modes[currentMode].Scale * 2.55;
    for (uint8_t x = 0U; x < ((WIDTH / 2)); x++) {
      for (uint8_t y = 0U; y < h; y++) {
        drawPixelXY(CENTER_X_MINOR - x, HEIGHT - y - 1, CHSV(hue, 255, 255 - x * STEP));
        drawPixelXY(CENTER_X_MAJOR + x, HEIGHT - y - 1, CHSV(hue, 255, 255 - x * STEP));
      }
    }
  }

  if (hue2 == 0) {
    posX = floor(pcnt / 2);
    uint8_t posY = HEIGHT - h - pcnt;
    // LOG.printf_P(PSTR("• [%03d] | posX %03d | deltaHue2 %03d | \n"), step, posX, deltaHue2);

    /* move sand -------- */
    if ((posY < (HEIGHT - h - 2)) && (posY > deltaHue2)) {
      drawPixelXY(CENTER_X_MAJOR, posY, CHSV(hue, 255, 255));
      drawPixelXY(CENTER_X_MAJOR, posY - 2, CHSV(hue, 255, 255));
      drawPixelXY(CENTER_X_MAJOR, posY - 4, CHSV(hue, 255, 255));

      if (posY < (HEIGHT - h - 3)) {
        drawPixelXY(CENTER_X_MAJOR, posY + 1, CHSV(hue, 255, 0 ));
      }
    }

    /* draw body hourglass */
    if (pcnt % 2 == 0) {
      drawPixelXY(CENTER_X_MAJOR - posX, HEIGHT - deltaHue2 - 1, CHSV(hue, 255, 0));
      drawPixelXY(CENTER_X_MAJOR - posX, deltaHue2, CHSV(hue, 255, 255 - posX * STEP));
    } else {
      drawPixelXY(CENTER_X_MAJOR + posX, HEIGHT - deltaHue2 - 1, CHSV(hue, 255, 0));
      drawPixelXY(CENTER_X_MAJOR + posX, deltaHue2, CHSV(hue, 255, 255 - posX * STEP));
    }

    if (pcnt > WIDTH - 1) {
      deltaHue2++;
      pcnt = 0;
      if (modes[currentMode].Scale > 95) {
        hue += 4U;
      }
    }

    pcnt++;
    if (deltaHue2 > h) {
      deltaHue2 = 0;
      hue2 = 1;
    }
  }
  // имитация переворота песочных часов
  if (hue2 > 0) {
    for (uint8_t x = 0U; x < WIDTH; x++) {
      for (uint8_t y = HEIGHT; y > 0U; y--) {
        drawPixelXY(x, y, getPixColorXY(x, y - 1U));
        drawPixelXY(x, y - 1, 0x000000);
      }
    }
    hue2++;
    if (hue2 > route) {
      hue2 = 0;
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
      // scale | speed
      setModeSettings(random8(11U, 69U), random8(150U, 255U));
    }
#endif
    loadingFlag = false;
    ledsClear(); // esphome: FastLED.clear();
    for (int8_t x = -CENTER_X_MAJOR; x < CENTER_X_MAJOR; x++) {
      for (int8_t y = -CENTER_Y_MAJOR; y < CENTER_Y_MAJOR; y++) {
        noise3d[0][x + CENTER_X_MAJOR][y + CENTER_Y_MAJOR] = (atan2(x, y) / PI) * 128 + 127; // thanks ldirko
        noise3d[1][x + CENTER_X_MAJOR][y + CENTER_Y_MAJOR] = hypot(x, y);                    // thanks Sutaburosu
      }
    }
  }

  uint8_t Petals = modes[currentMode].Scale / 10;
  uint16_t color_speed;
  step = modes[currentMode].Scale % 10;
  if (step < 5) color_speed = scale / (3 - step/2);
  else color_speed = scale * (step/2 - 1);
  scale ++;
  for (uint8_t x = 0; x < WIDTH; x++) {
    for (uint8_t y = 0; y < HEIGHT; y++) {
      byte angle = noise3d[0][x][y];
      byte radius = noise3d[1][x][y];
      leds[XY(x, y)] = CHSV(color_speed + radius * (255 / WIDTH), 255, sin8(sin8(scale + angle * Petals + ( radius * (255 / WIDTH))) + scale * 4 + sin8(scale * 4 - radius * (255 / WIDTH)) + angle * Petals));
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
  static uint8_t saturation;
  static uint8_t brightness;
  static uint8_t low_br;
  //uint8_t delta;
  const uint8_t PADDING = HEIGHT * 0.25;
  const uint8_t WARM_LIGHT = 55U;
  const uint8_t STEP = 4U;
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed 210
      setModeSettings(random8(100U), random8(40, 200U));
    }
#endif
    loadingFlag = false;
    deltaValue = 0;
    step = deltaValue;
    if (modes[currentMode].Speed > 52) {
      // brightness = 50 + modes[currentMode].Speed;
      brightness = map(modes[currentMode].Speed, 1, 255, 50U, 250U);
      low_br = 50U;
    } else {
      brightness = 0U;
      low_br = 0U;
    }
    saturation = (modes[currentMode].Scale > 50U) ? 64U : 0U;
    if (abs (70 - modes[currentMode].Scale) <= 5) saturation = 170U;
    ledsClear(); // esphome: FastLED.clear();

  }
  dimAll(170);
  hue = (modes[currentMode].Scale > 95) ? floor(step / 32) * 32U : modes[currentMode].Scale * 2.55;

  // ------
  for (uint8_t x = 0U; x < WIDTH + 1 ; x++) {

    // light ---
    if (low_br > 0) {
      gradientVertical( x - deltaValue, CENTER_Y_MAJOR, x + 1U - deltaValue, HEIGHT - PADDING - 1,  WARM_LIGHT, WARM_LIGHT, brightness, low_br, saturation);
      gradientVertical( WIDTH - x + deltaValue, CENTER_Y_MAJOR, WIDTH - x + 1U + deltaValue, HEIGHT - PADDING - 1,  WARM_LIGHT, WARM_LIGHT, brightness, low_br, saturation);
      gradientVertical( x - deltaValue, PADDING + 1, x + 1U - deltaValue, CENTER_Y_MAJOR, WARM_LIGHT, WARM_LIGHT, low_br + 10, brightness, saturation);
      gradientVertical( WIDTH - x + deltaValue, PADDING + 1, WIDTH - x + 1U + deltaValue, CENTER_Y_MAJOR, WARM_LIGHT, WARM_LIGHT, low_br + 10, brightness, saturation);
    } else {
      if (x % (STEP + 1) == 0) {
        leds[XY(random8(WIDTH), random8(PADDING + 2, HEIGHT - PADDING - 2))] = CHSV(step - 32U, random8(128U, 255U), 255U);
      }
      if ((modes[currentMode].Speed < 25) & (low_br == 0)) {
        deltaValue = 0;
        if (x % 2 != 0) {
          gradientVertical( x - deltaValue, HEIGHT - PADDING, x + 1U - deltaValue, HEIGHT,  hue, hue + 2, 64U, 20U, 255U);
          gradientVertical( (WIDTH - x + deltaValue), 0U,  (WIDTH - x + 1U + deltaValue), PADDING,  hue, hue, 42U, 64U, 255U);
        }
        //        deltaValue = 0;
      }
    }
    if (x % STEP == 0) {
      // body --
      gradientVertical( x - deltaValue, HEIGHT - PADDING, x + 1U - deltaValue, HEIGHT,  hue, hue + 2, 255U, 20U, 255U);
      gradientVertical( (WIDTH - x + deltaValue), 0U,  (WIDTH - x + 1U + deltaValue), PADDING,  hue, hue, 42U, 255U, 255U);
    }
  }
  // ------

  deltaValue++;
  if (deltaValue >= STEP) {
    deltaValue = 0;
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
        // scale | speed
        setModeSettings(1U + random8(100U), 1U + random8(255U));
      }
    #endif
    loadingFlag = false;
    shtukX = WIDTH / 3U + 1U;
    shtukY = HEIGHT / 3U + 1U;
    poleX = modes[currentMode].Speed % 3U;
    poleY = modes[currentMode].Speed / 3U % 3U;

    for (uint8_t i = 0; i < shtukX; i++)
      line[i] = random8(3U);
    for (uint8_t i = 0; i < shtukY; i++)
      shiftValue[i] = random8(3U);
  }

  bool type = random8(2U);
  CRGB color = CHSV(random8(), 255U - random8(modes[currentMode].Scale * 2.55), 255U);

  uint8_t i = random8(shtukX);
  uint8_t j = random8(shtukY);
  int16_t x0 = i * 3 + (poleX + ((modes[currentMode].Scale & 0x01) ? line[j] : 0)) % 3 - 2;
  int16_t y0 = j * 3 + (poleY + ((modes[currentMode].Scale & 0x01) ? 0 : shiftValue[i])) % 3 - 2;
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
//Idea from https://www.youtube.com/watch?v=HsA-6KIbgto&ab_channel=GreatScott%21

static void Octopus() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      // scale | speed
      setModeSettings(random(10U, 101U), random(150U, 255U));
    }
    #endif
    loadingFlag = false;

    for (int8_t x = -CENTER_X_MAJOR; x < CENTER_X_MAJOR + ((int8_t)WIDTH % 2); x++) {
      for (int8_t y = -CENTER_Y_MAJOR; y < CENTER_Y_MAJOR + ((int8_t)HEIGHT % 2); y++) {
        noise3d[0][x + CENTER_X_MAJOR][y + CENTER_Y_MAJOR] = (atan2(x, y) / PI) * 128 + 127; // thanks ldirko
        noise3d[1][x + CENTER_X_MAJOR][y + CENTER_Y_MAJOR] = hypot(x, y); // thanks Sutaburosu
      }
    }
  }
  
  uint8_t legs = modes[currentMode].Scale / 10;
  uint16_t color_speed;
  step = modes[currentMode].Scale % 10;
  if (step < 5) color_speed = scale / (3 - step/2);
  else color_speed = scale * (step/2 - 1);
  scale ++;
  for (uint8_t x = 0; x < WIDTH; x++) {
    for (uint8_t y = 0; y < HEIGHT; y++) {
      byte angle = noise3d[0][x][y];
      byte radius = noise3d[1][x][y];
      leds[XY(x, y)] = CHSV(color_speed - radius * (255 / WIDTH), 255, sin8(sin8((angle * 4 - (radius * (255 / WIDTH))) / 4 + scale) + radius * (255 / WIDTH) - scale * 2 + angle * legs));
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

  uint8_t divider;
  uint8_t entry_point;
  uint16_t value;
  uint16_t max_val;
  if (loadingFlag) {

#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                          scale | speed 210
      setModeSettings(1U + random8(252U), 1 + random8(219U));
    }
#endif
    loadingFlag = false;
    ledsClear(); // esphome: FastLED.clear();
    // blurScreen(beatsin8(5U, 50U, 5U));
    deltaValue = 255U - modes[currentMode].Speed + 1U;
    step = deltaValue;                    // чтообы при старте эффекта сразу покрасить лампу
    hue = floor(21.25 * (random8(11) + 1)); // next color
    deltaHue = hue - 22;                  // last color
    deltaHue2 = 80;                       // min bright
    max_val = constrain(pow(2, WIDTH), 1U, 65535U);
    //    for ( int i = WIDTH; i < (NUM_LEDS - WIDTH); i++) {
    //      leds[i] = CHSV(120U, 24U, 64U);
    //    }
  }

  if (step >= deltaValue) {
    step = 0U;
    // LOG.printf_P(PSTR("%03d | log: %f | val: %03d\n\r"), modes[currentMode].Brightness, log(modes[currentMode].Brightness), deltaHue2);
  }

  // Create Oil Paints --------------
  // выбираем краски  ---------------
  if (step % CENTER_Y_MINOR == 0) {
    divider = floor((modes[currentMode].Scale - 1) / 10);             // маштаб задает диапазон изменения цвета
    deltaHue = hue;                                                   // set last color
    hue += 6 * divider;                                               // new color
    hue2 = 255;                                                       // restore brightness
    deltaHue2 = 80 - floor(log(modes[currentMode].Brightness) * 6);   // min bright
    entry_point = random8(WIDTH);                                     // start X position
    trackingObjectHue[entry_point] = hue;                             // set start position
    drawPixelXY(entry_point,  HEIGHT - 2, CHSV(hue, 255U, 255U));
    // !!! ********
    if (custom_eff == 1) {
      drawPixelXY(entry_point + 1,  HEIGHT - 3, CHSV(hue + 30, 255U, 255U));
    }
    // ************
    // LOG.printf_P(PSTR("BR %03d | SP %03d | SC %03d | hue %03d\n\r"), modes[currentMode].Brightness, modes[currentMode].Speed, modes[currentMode].Scale, hue);
  }

  // формируем форму краски, плавно расширяя струю ----
  if (random8(3) == 1) {
    // LOG.println("<--");
    for (uint8_t x = 1U; x < WIDTH; x++) {
      if (trackingObjectHue[x] == hue) {
        trackingObjectHue[x - 1] = hue;
        break;
      }
    }
  } else {
    // LOG.println("-->");
    for (uint8_t x = WIDTH - 1; x > 0U ; x--) {
      if (trackingObjectHue[x] == hue) {
        trackingObjectHue[x + 1] = hue;
        break;
      }
      // LOG.printf_P(PSTR("x = %02d | value = %03d | hue = %03d \n\r"), x, trackingObjectHue[x], hue);
    }
  }
  // LOG.println("------------------------------------");

  // выводим сформированную строку --------------------- максимально яркую в момент смены цвета
  for (uint8_t x = 0U; x < WIDTH; x++) {
    //                                                                                set color  next |    last  |
    drawPixelXY(x,  HEIGHT - 1, CHSV(trackingObjectHue[x], 255U, (trackingObjectHue[x] == hue) ? hue2 : deltaHue2));
  }
  //  LOG.println("");
  // уменьшаем яркость для следующих строк
  if ( hue2 > (deltaHue2 + 16)) {
    hue2 -= 16U;
  }
  // сдвигаем неравномерно поток вниз ---
  value = random16(max_val);
  //LOG.printf_P(PSTR("value = %06d | "), value);
  for (uint8_t x = 0U; x < WIDTH; x++) {
    if ( bitRead(value, x ) == 0) {
      //LOG.print (" X");
      for (uint8_t y = 0U; y < HEIGHT - 1; y++) {
        drawPixelXY(x, y, getPixColorXY(x, y + 1U));
      }
    }
  }
  // LOG.printf_P(PSTR("%02d | hue2 = %03d | min = %03d \n\r"), step, hue2, deltaHue2);
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
  static int64_t frameCount = 0;
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), random8(40, 200U));
    }
#endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    loadingFlag = false;
    hue = modes[currentMode].Scale / 10;
  }
  FPSdelay = 1;//64 - modes[currentMode].Speed / 4;

  frameCount++;
  uint8_t t1 = cos8((42 * frameCount) / (132 - modes[currentMode].Speed / 2));
  uint8_t t2 = cos8((35 * frameCount) / (132 - modes[currentMode].Speed / 2));
  uint8_t t3 = cos8((38 * frameCount) / (132 - modes[currentMode].Speed / 2));

  for (uint16_t y = 0; y < HEIGHT; y++) {
    for (uint16_t x = 0; x < WIDTH; x++) {
      // Calculate 3 seperate plasma waves, one for each color channel
      uint8_t r = cos8((x << 3) + (t1 >> 1) + cos8(t2 + (y << 3) + modes[currentMode].Scale));
      uint8_t g = cos8((y << 3) + t1 + cos8((t3 >> 2) + (x << 3)) +modes[currentMode].Scale);
      uint8_t b = cos8((y << 3) + t2 + cos8(t1 + x + (g >> 2) + modes[currentMode].Scale));

      switch (hue) {
          case 0:
              r = pgm_read_byte(&exp_gamma[r]);
              g = pgm_read_byte(&exp_gamma[g]);
              b = pgm_read_byte(&exp_gamma[b]);
              break;
          case 1:
              r = pgm_read_byte(&exp_gamma[r]);
              b = pgm_read_byte(&exp_gamma[g]);
              g = pgm_read_byte(&exp_gamma[b]);
              break;
          case 2:
              g = pgm_read_byte(&exp_gamma[r]);
              r = pgm_read_byte(&exp_gamma[g]);
              b = pgm_read_byte(&exp_gamma[b]);
              break;
          case 3:
              r = pgm_read_byte(&exp_gamma[r])/2;
              g = pgm_read_byte(&exp_gamma[g]);
              b = pgm_read_byte(&exp_gamma[b]);
              break;
          case 4:
              r = pgm_read_byte(&exp_gamma[r]);
              g = pgm_read_byte(&exp_gamma[g])/2;
              b = pgm_read_byte(&exp_gamma[b]);
              break;
          case 5:
              r = pgm_read_byte(&exp_gamma[r]);
              g = pgm_read_byte(&exp_gamma[g]);
              b = pgm_read_byte(&exp_gamma[b])/2;
              break;
          case 6:
              r = pgm_read_byte(&exp_gamma[r])*3;
              g = pgm_read_byte(&exp_gamma[g]);
              b = pgm_read_byte(&exp_gamma[b]);
              break;
          case 7:
              r = pgm_read_byte(&exp_gamma[r]);
              g = pgm_read_byte(&exp_gamma[g])*3;
              b = pgm_read_byte(&exp_gamma[b]);
              break;
          case 8:
              r = pgm_read_byte(&exp_gamma[r]);
              g = pgm_read_byte(&exp_gamma[g]);
              b = pgm_read_byte(&exp_gamma[b])*3;
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
  //ledsClear(); // esphome: FastLED.clear();
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      // scale | speed
      setModeSettings(random(10U, 101U), random(150U, 255U));
    }
    #endif
    loadingFlag = false;

    for (int8_t x = -CENTER_X_MAJOR; x < CENTER_X_MAJOR + ((int8_t)WIDTH % 2); x++) {
      for (int8_t y = -CENTER_Y_MAJOR; y < CENTER_Y_MAJOR + ((int8_t)HEIGHT % 2); y++) {
        noise3d[0][x + CENTER_X_MAJOR][y + CENTER_Y_MAJOR] = (atan2(x, y) / PI) * 128 + 127; // thanks ldirko
        noise3d[1][x + CENTER_X_MAJOR][y + CENTER_Y_MAJOR] = hypot(x, y); // thanks Sutaburosu
      }
    }
  }

  uint8_t legs = modes[currentMode].Scale / 10;
  uint16_t color_speed;
  step = modes[currentMode].Scale % 10;
  if (step < 5) color_speed = scale / (3 - step/2);
  else color_speed = scale * (step/2 - 1);
  scale++;
  for (uint8_t x = 0; x < WIDTH; x++) {
    for (uint8_t y = 0; y < HEIGHT; y++) {
      byte angle = noise3d[0][x][y];
      byte radius = noise3d[1][x][y];
      leds[XY(x, y)] = CHSV(color_speed + radius * (255 / WIDTH), 255, sin8(scale * 4 + sin8(scale * 4 - radius * (255 / WIDTH)) + angle * legs));
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
  uint32_t  FLORA_COLOR = 0x2F1F00;
  uint8_t posX =  floor(CENTER_X_MINOR - WIDTH * 0.3);
  uint8_t h =  random8(HEIGHT - 6U) + 4U;
  DrawLine(posX + 1, 1U, posX + 1, h - 1, 0x000000);
  DrawLine(posX + 2, 1U, posX + 2, h, FLORA_COLOR );
  drawPixelXY(posX + 2, h - random8(floor(h * 0.5)), random8(2U) == 1 ? 0xFF00E0 :  random8(2U) == 1 ? 0xFFFF00 : 0x00FF00);
  drawPixelXY(posX + 1, h - random8(floor(h * 0.25)), random8(2U) == 1 ? 0xFF00E0 : 0xFFFF00);
  if (random8(2U) == 1) {
    drawPixelXY(posX + 1, floor(h * 0.5), random8(2U) == 1 ? 0xEF001F :  0x9FFF00);
  }
  h =  floor(h * 0.65);
  if (WIDTH > 8) {
    DrawLine(posX - 1, 1U, posX - 1, h - 1, 0x000000);
  }
  DrawLine(posX, 1U, posX, h, FLORA_COLOR);
  drawPixelXY(posX, h - random8(floor(h * 0.5)), random8(2U) == 1 ? 0xFF00E0 : 0xFFFF00);
}

//---------------------------------------
static void animeBobbles() {
  // сдвигаем всё вверх ----
  for (uint8_t x = CENTER_X_MAJOR; x < WIDTH; x++) {
    for (uint8_t y = HEIGHT; y > 0U; y--) {
      if (getPixColorXY(x, y - 1) == 0xFFFFF7) {
        drawPixelXY(x, y, 0xFFFFF7);
        drawPixelXY(x, y - 1, getPixColorXY(0, y - 1));
      }
    }
  }
  // ----------------------
  if ( step % 4 == 0) {
    drawPixelXY(CENTER_X_MAJOR + random8(5), 0U, 0xFFFFF7);
    if ( step % 12 == 0) {
      drawPixelXY(CENTER_X_MAJOR + 2 + random8(3), 0U, 0xFFFFF7);
    }
  }
}

//---------------------------------------
static void createScene(uint8_t idx) {
  switch (idx) {
    case 0:     // blue green ------
      gradientDownTop(floor((HEIGHT - 1) * 0.5), CHSV(96, 255, 100), HEIGHT, CHSV(160, 255, 255));
      gradientDownTop(0, CHSV(96, 255, 255), CENTER_Y_MINOR, CHSV(96, 255, 100));
      break;
    case 1:     // aquamarine green
      gradientDownTop(floor((HEIGHT - 1) * 0.3), CHSV(96, 255, 100), HEIGHT, CHSV(130, 255, 220));
      gradientDownTop(0, CHSV(96, 255, 255), floor(HEIGHT * 0.3), CHSV(96, 255, 100));
      break;
    case 2:     // blue aquamarine -
      gradientDownTop(floor((HEIGHT - 1) * 0.5), CHSV(170, 255, 100), HEIGHT, CHSV(160, 255, 200));
      gradientDownTop(0, CHSV(100, 255, 255), CENTER_Y_MINOR, CHSV(170, 255, 100));
      break;
    case 3:     // yellow green ----
      gradientDownTop(floor((HEIGHT - 1) * 0.5), CHSV(95, 255, 55), HEIGHT, CHSV(70, 255, 200));
      gradientDownTop(0, CHSV(95, 255, 255), CENTER_Y_MINOR, CHSV(100, 255, 55));
      break;
    case 4:     // sea green -------
      gradientDownTop(floor((HEIGHT - 1) * 0.3), CHSV(120, 255, 55), HEIGHT, CHSV(175, 255, 200));
      gradientDownTop(0, CHSV(120, 255, 255), floor(HEIGHT * 0.3), CHSV(120, 255, 55));
      break;
    default:
      gradientDownTop(floor((HEIGHT - 1) * 0.25), CHSV(180, 255, 85), HEIGHT, CHSV(160, 255, 200));
      gradientDownTop(0, CHSV(80, 255, 255), floor(HEIGHT * 0.25), CHSV(180, 255, 85));
      break;
  }
  flora();
}

//---------------------------------------
static void createSceneM(uint8_t idx) {
  switch (idx) {
    case 0:     // blue green ------
      gradientVertical(0, CENTER_Y_MINOR, WIDTH, HEIGHT, 96, 150, 100, 255, 255U);
      gradientVertical(0, 0, WIDTH, CENTER_Y_MINOR, 96, 96, 255, 100, 255U);
      break;
    case 1:     // aquamarine green
      gradientVertical(0, floor(HEIGHT  * 0.3), WIDTH, HEIGHT, 96, 120, 100, 220, 255U);
      gradientVertical(0, 0, WIDTH, floor(HEIGHT  * 0.3), 96, 96, 255, 100, 255U);
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
      gradientVertical(0, floor(HEIGHT  * 0.3), WIDTH, HEIGHT, 120, 160, 55, 200, 255U);
      gradientVertical(0, 0, WIDTH, floor(HEIGHT  * 0.3), 120, 120, 255, 55, 255U);
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
  #define ALT_GRADIENT (1U)

  uint8_t divider = 0;
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                          scale | speed 210
      setModeSettings(1U + random8(252U), 20 + random8(180U));
    }
#endif
    loadingFlag = false;
    deltaValue = 255U - modes[currentMode].Speed + 1U;
    step = deltaValue;                                          // чтообы при старте эффекта сразу покрасить лампу
    divider = floor((modes[currentMode].Scale - 1) / 20);       // маштаб задает смену палитры воды
    if (ALT_GRADIENT) {
      createSceneM(divider);
    } else {
      createScene(divider);
    }
  }

  if (step >= deltaValue) {
    step = 0U;
  }

  // restore scene after power on ---------
  if (getPixColorXY(0U, HEIGHT - 2) == CRGB::Black) {
    if (ALT_GRADIENT) {
      createSceneM(divider);
    } else {
      createScene(divider);
    }
  }

  // light at the bottom ------------------
  if (!ALT_GRADIENT) {
    if (step % 2 == 0) {
      if (random8(6) == 1) {
        //fill_gradient(leds, NUM_LEDS - WIDTH, CHSV(96U, 255U, 200U), NUM_LEDS, CHSV(50U, 255U, 255U), SHORTEST_HUES);
        if (ORIENTATION < 3 || ORIENTATION == 7) {    // if (STRIP_DIRECTION < 2) {
          fill_gradient(leds, 0, CHSV(96U, 255U, 190U), random8(WIDTH + random8(6)), CHSV(90U, 200U, 255U), SHORTEST_HUES);
        } else {
          fill_gradient(leds, NUM_LEDS - random8(WIDTH + random8(6)), CHSV(96U, 255U, 190U), NUM_LEDS, CHSV(90U, 200U, 255U), SHORTEST_HUES);
        }
      } else {
        //fill_gradient(leds, NUM_LEDS - WIDTH, CHSV(50U, 128U, 255U), NUM_LEDS, CHSV(90U, 255U, 180U), SHORTEST_HUES);
        if (ORIENTATION < 3 || ORIENTATION == 7) {    // if (STRIP_DIRECTION < 2) {
          fill_gradient(leds, 0, CHSV(85U, 128U, 255U), random8(WIDTH), CHSV(90U, 255U, 180U), SHORTEST_HUES);
        } else {
          fill_gradient(leds, NUM_LEDS - random8(WIDTH), CHSV(85U, 128U, 255U), NUM_LEDS, CHSV(90U, 255U, 180U), SHORTEST_HUES);
        }
      }
    }
  }

  // LOG.printf_P(PSTR("%02d | hue2 = %03d | min = %03d \n\r"), step, hue2, deltaHue2);
  // -------------------------------------
  animeBobbles();
  if (custom_eff == 1) {
    blurRows(WIDTH, 3U, 10U);
    // blurScreen(beatsin8(0U, 5U, 0U));
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
  //static const byte COLOR_RANGE = 32;
  static uint8_t customHue;
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      // scale | speed
      setModeSettings(random8(1, 100U), random8(215, 255U) );
    }
#endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    ff_y = map(WIDTH, 8, 64, 310, 63);
    ff_z = ff_y;
    speedfactor = map(modes[currentMode].Speed, 1, 255, 32, 4); // _speed = map(speed, 1, 255, 128, 16);
    customHue = floor( modes[currentMode].Scale - 1U) * 2.55;
    ledsClear(); // esphome: FastLED.clear();
  }
  uint8_t color = customHue + hue;
  if (modes[currentMode].Scale >= 99) {
    if (hue2++ & 0x01 && deltaHue++ & 0x01 && deltaHue2++ & 0x01) hue += 8;
    fillMyPal16_2(customHue + hue, modes[currentMode].Scale & 0x01);
  } else {
    color = customHue;
    fillMyPal16_2(customHue + AURORA_COLOR_RANGE - beatsin8(AURORA_COLOR_PERIOD, 0U, AURORA_COLOR_RANGE * 2), modes[currentMode].Scale & 0x01);
  }

  for (byte x = 0; x < WIDTH; x++) {
    if (x % 2 == 0) {
      leds[XY(x, 0)] = CHSV( color, 255U, 128U);
    }

    emitterX = ((random8(2) == 0U) ? 545. : 390.) / HEIGHT;
    for (byte y = 2; y < HEIGHT - 1; y++) {
      polarTimer++;
      leds[XY(x, y)] =
        ColorFromPalette(myPal,
                         qsub8(
                           inoise8(polarTimer % 2 + x * ff_z,
                                   y * 16 + polarTimer % 16,
                                   polarTimer / speedfactor
                                  ),
                           fabs((float)HEIGHT / 2 - (float)y) * emitterX
                         )
                        ) ;
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
/*должен быть перед эффектом Матрицf бегунок Скорость не регулирует задержку между кадрами,
  но меняет частоту строба*/
static void StrobeAndDiffusion() {
  //const uint8_t SIZE = 3U;
  const uint8_t DELTA = 1U;         // центровка по вертикали
  uint8_t STEP = 2U;
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      // scale | speed
      setModeSettings(1U + random8(100U), 1U + random8(150U));
    }
#endif
    loadingFlag = false;
    FPSdelay = 25U; // LOW_DELAY;
    hue2 = 1;
    ledsClear(); // esphome: FastLED.clear();
  }

  STEP = floor((255 - modes[currentMode].Speed) / 64) + 1U; // for strob
  if (modes[currentMode].Scale > 50) {
    // diffusion ---
    blurScreen(beatsin8(3, 64, 80));
    FPSdelay = LOW_DELAY;
    STEP = 1U;
    if (modes[currentMode].Scale < 75) {
      // chaos ---
      FPSdelay = 30;
      VirtualSnow(1);
    }

  } else {
    // strob -------
    if (modes[currentMode].Scale > 25) {
      dimAll(200);
      FPSdelay = 30;
    } else {
      dimAll(240);
      FPSdelay = 40;
    }
  }

  const uint8_t rows = (HEIGHT + 1) / 3U;
  deltaHue = floor(modes[currentMode].Speed / 64) * 64;
  bool dir = false;
  for (uint8_t y = 0; y < rows; y++) {
    if (dir) {
      if ((step % STEP) == 0) {   // small layers
        drawPixelXY(WIDTH - 1, y * 3 + DELTA, CHSV(step, 255U, 255U ));
      } else {
        drawPixelXY(WIDTH - 1, y * 3 + DELTA, CHSV(170U, 255U, 1U));
      }
    } else {
      if ((step % STEP) == 0) {   // big layers
        drawPixelXY(0, y * 3 + DELTA, CHSV((step + deltaHue), 255U, 255U));
      } else {
        drawPixelXY(0, y * 3 + DELTA, CHSV(0U, 255U, 0U));
      }
    }

    // сдвигаем слои  ------------------
    for (uint8_t x = 1U ; x < WIDTH; x++) {
      if (dir) {  // <==
        drawPixelXY(x - 1, y * 3 + DELTA, getPixColorXY(x, y * 3 + DELTA));
      } else {    // ==>
        drawPixelXY(WIDTH - x, y * 3 + DELTA, getPixColorXY(WIDTH - x - 1, y * 3 + DELTA));
      }
    }
    dir = !dir;
  }

  if (hue2 == 1) {
    step ++;
    if (step >= 254) hue2 = 0;
  } else {
    step --;
    if (step < 1) hue2 = 1;
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
  static bool dark;
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      // scale | speed
      setModeSettings(random8(1U, 100U), random8(100U, 255U));
    }
#endif
    loadingFlag = false;
    hue = random8(8) * 32; // modes[currentMode].Scale;
    hue2 = 255U;
    dark = modes[currentMode].Scale < 76U;
  }

  if  (modes[currentMode].Scale < 81) {
    blurScreen(128U);
  } else 
  if  (modes[currentMode].Scale < 86) {
    blurScreen(96U);
  } else 
  if  (modes[currentMode].Scale < 91) {
    blurScreen(64U);
  } else 
   if  (modes[currentMode].Scale < 96) {
    blurScreen(32U);
   }

  // <==== scroll ===== 
  for (uint8_t y = 0U ; y < HEIGHT; y++) {
    for (uint8_t x = 0U ; x < WIDTH - 1; x++) {
      hue2--;
      if (dark) {   // black delimiter -----
        drawPixelXY(WIDTH - 1, y, CHSV(hue, 255, hue2));
      } else {      // white delimiter -----
        drawPixelXY(WIDTH - 1, y, CHSV(hue, 64 + hue2 / 2, 255 - hue2 / 4));
      }
      drawPixelXY(x, y,  getPixColorXY(x + 1,  y));
    }
  }
  if (modes[currentMode].Scale < 56) {
    return;
  }
  if (modes[currentMode].Scale < 61) {
    hue += 1;
  } else 
  if (modes[currentMode].Scale < 66) {
    hue += 2;
  } else 
  if (modes[currentMode].Scale < 71) {
    hue += 3;
  } else 
    if (modes[currentMode].Scale < 76) {
      hue += 4;
  } else {
      hue += 3;
  }
}
#endif


#ifdef DEF_SWIRL
// ============== Swirl ================
//    © SlingMaster | by Alex Dovby
//              EFF_SWIRL
//--------------------------------------
static void Swirl() {
  uint32_t color;
  uint8_t divider = 0;
  uint8_t lastHue = 0;

  static const uint32_t colors[5][6] PROGMEM = {
    {CRGB::Blue, CRGB::DarkRed, CRGB::Aqua, CRGB::Magenta, CRGB::Gold, CRGB::Green },
    {CRGB::Yellow, CRGB::LemonChiffon, CRGB::LightYellow, CRGB::Gold, CRGB::Chocolate, CRGB::Goldenrod},
    {CRGB::Green, CRGB::DarkGreen, CRGB::LawnGreen, CRGB::SpringGreen, CRGB::Cyan, CRGB::Black },
    {CRGB::Blue, CRGB::DarkBlue, CRGB::MidnightBlue, CRGB::MediumSeaGreen, CRGB::MediumBlue, CRGB:: DeepSkyBlue },
    {CRGB::Magenta, CRGB::Red, CRGB::DarkMagenta, CRGB::IndianRed, CRGB::Gold, CRGB::MediumVioletRed }
  };

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      // scale | speed
      setModeSettings(50U + random8(190U), 250U);
    }
    #endif

    loadingFlag = false;
    ledsClear(); // esphome: FastLED.clear();
    deltaValue = 255U - modes[currentMode].Speed + 1U;
    step = deltaValue;                      // чтообы при старте эффекта сразу покрасить лампу
    deltaHue2 = 0U;                         // count для замедления смены цвета
    deltaHue = 0U;                          // direction | 0 hue-- | 1 hue++ |
    hue2 = 0U;                              // x
  }

  if (step >= deltaValue) {
    step = 0U;
  }
  divider = floor((modes[currentMode].Scale - 1) / 20); // маштаб задает смену палитры
  // задаем цвет и рисуем завиток --------
  color = colors[divider][hue];
  drawPixelXY(hue2, deltaHue2, color);

  hue2++;                     // x
  // два варианта custom_eff задается в сетапе лампы ----
  if (custom_eff == 1) {
    deltaHue2++;              // y
  } else {
    if (hue2 % 2 == 0) {
      deltaHue2++;            // y
    }
  }
  // -------------------------------------

  if  (hue2 > WIDTH) {
    hue2 = 0U;
  }

  if (deltaHue2 >= HEIGHT) {
    deltaHue2 = 0U;
    // new swirl ------------
    hue2 = random8(WIDTH - 2);
    // select new color -----
    hue = random8(6);

    if (lastHue == hue) {
      hue = hue + 1;
      if (hue >= 6) {
        hue = 0;
      }
    }
    lastHue = hue;
  }
  blurScreen(4U + random8(8));
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

const byte OFFSET = 1U;
const uint8_t H = HEIGHT - OFFSET;
  
static void Tornado() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      // scale | speed
      setModeSettings(random8(100U, 255U), random8(20U, 100U));
    }
#endif
    //scale = 1;
    loadingFlag = 0;

    //ledsClear(); // esphome: FastLED.clear();
    for (int8_t x = -CENTER_X_MAJOR; x < CENTER_X_MAJOR; x++) {
      for (int8_t y = -OFFSET; y < H; y++) {
        noise3d[0][x + CENTER_X_MAJOR][y + OFFSET] = 128 * (atan2(y, x) / PI);
        noise3d[1][x + CENTER_X_MAJOR][y + OFFSET] = hypot(x, y);                    // thanks Sutaburosu
      }
    }
  }
  scale += modes[currentMode].Speed / 10;
  for (uint8_t x = 0; x < WIDTH; x++) {
    for (uint8_t y = 0; y < HEIGHT; y++) {
      byte angle = noise3d[0][x][y];
      byte radius = noise3d[1][x][y];
      leds[XY(x, y)] = CHSV((angle * modes[currentMode].Scale / 10) - scale + (radius * modes[currentMode].Scale / 10), min(((uint16_t)y*512U/(uint16_t)HEIGHT),255U), (y < (HEIGHT/8) ? 255 - (((HEIGHT/8) - y) * 16) : 255));
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
static void SmearPaint(uint8_t obj[trackingOBJECT_MAX_COUNT]) {
  uint8_t divider;
  int temp;
  static const uint32_t colors[6][8] PROGMEM = {
    {0x2F0000,  0xFF4040, 0x6F0000, 0xAF0000, 0xff5f00, CRGB::Red, 0x480000, 0xFF0030},
    {0x002F00, CRGB::LawnGreen, 0x006F00, 0x00AF00, CRGB::DarkMagenta, 0x00FF00, 0x004800, 0x00FF30},
    {0x002F1F, CRGB::DarkCyan, 0x00FF7F, 0x007FFF, 0x20FF5F, CRGB::Cyan, 0x004848, 0x7FCFCF },
    {0x00002F, 0x5030FF, 0x00006F, 0x0000AF, CRGB::DarkCyan, 0x0000FF, 0x000048, 0x5F5FFF},
    {0x2F002F, 0xFF4040, 0x6F004A, 0xFF0030, CRGB::DarkMagenta, CRGB::Magenta, 0x480048, 0x3F00FF},
    {CRGB::Blue, CRGB::Red, CRGB::Gold, CRGB::Green, CRGB::DarkCyan, CRGB::DarkMagenta, 0x000000, 0xFF7F00 }
  };
  if (trackingObjectHue[5] == 1) {  // direction >>>
    obj[1]++;
    if (obj[1] >= obj[2]) {
      trackingObjectHue[5] = 0;     // swap direction
      obj[3]--;                     // new line
      if (step % 2 == 0) {
        obj[1]++;
      } else {
        obj[1]--;
      }

      obj[0]--;
    }
  } else {                          // direction <<<
    obj[1]--;
    if (obj[1] <= (obj[2] - obj[0])) {
      trackingObjectHue[5] = 1;     // swap direction
      obj[3]--;                     // new line
      if (obj[0] >= 1) {
        temp = obj[0] - 1;
        if (temp < 0) {
          temp = 0;
        }
        obj[0] = temp;
        obj[1]++;
      }
    }
  }

  if (obj[3] == 255) {
    deltaHue = 255;
  }

  divider = floor((modes[currentMode].Scale - 1) / 16.7);
  if ( (obj[1] >= WIDTH) || (obj[3] == obj[4]) ) {
    // deltaHue value == 255 activate -------
    // set new parameter for new smear ------
    deltaHue = 255;
  }
  drawPixelXY(obj[1], obj[3], colors[divider][hue]);

  // alternative variant without dimmer effect
  // uint8_t h = obj[3] - obj[4];
  // uint8_t br = 266 - 12 * h;
  // if (h > 0) {
  // drawPixelXY(obj[1], obj[3], makeDarker(colors[divider][hue], br));
  // } else {
  // drawPixelXY(obj[1], obj[3], makeDarker(colors[divider][hue], 240));
  // }
}

//---------------------------------------
static void Watercolor() {
  // #define DIMSPEED (254U - 500U / WIDTH / HEIGHT)
  //uint8_t divider;
  if (loadingFlag) {

#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                          scale | speed 250
      setModeSettings(1U + random8(252U), 1 + random8(250U));
    }
#endif
    loadingFlag = false;
    ledsClear(); // esphome: FastLED.clear();
    deltaValue = 255U - modes[currentMode].Speed + 1U;
    step = deltaValue;                    // чтообы при старте эффекта сразу покрасить лампу
    hue = 0;
    deltaHue = 255;                       // last color
    trackingObjectHue[1] = floor(WIDTH * 0.25);
    trackingObjectHue[3] = floor(HEIGHT * 0.25);
  }

  if (step >= deltaValue) {
    step = 0U;
    // LOG.printf_P(PSTR("%03d | log: %f | val: %03d | divider: %d \n\r"), modes[currentMode].Brightness, log(modes[currentMode].Brightness), deltaHue2, divider);
  }

  // ******************************
  // set random parameter for smear
  // ******************************
  if (deltaHue == 255) {

    trackingObjectHue[0] = 4 + random8(floor(WIDTH * 0.25));                // width
    trackingObjectHue[1] = random8(WIDTH - trackingObjectHue[0]);           // x
    int temp =  trackingObjectHue[1] + trackingObjectHue[0];
    if (temp >= (WIDTH - 1)) {
      temp = WIDTH - 1;
      if (trackingObjectHue[1] > 1) {
        trackingObjectHue[1]--;
      } else {
        trackingObjectHue[1]++;
      }
    }
    trackingObjectHue[2] = temp;                                            // x end
    trackingObjectHue[3] = 3 + random8(HEIGHT - 4);                         // y
    temp = trackingObjectHue[3] - random8(3) - 3;
    if (temp <= 0) {
      temp = 0;
    }
    trackingObjectHue[4] = temp;                                            // y end
    trackingObjectHue[5] = 1;
    //divider = floor((modes[currentMode].Scale - 1) / 16.7);                 // маштаб задает смену палитры
    hue = random8(8);
    //    if (step % 127 == 0) {
    //      LOG.printf_P(PSTR("BR %03d | SP %03d | SC %03d | divider %d | [ %d ]\n\r"), modes[currentMode].Brightness, modes[currentMode].Speed, modes[currentMode].Scale, divider, hue);
    //    }
    hue2 = 255;
    deltaHue = 0;
  }
  // ******************************
  SmearPaint(trackingObjectHue);

  // LOG.printf_P(PSTR("%02d | hue2 = %03d | min = %03d \n\r"), step, hue2, deltaHue2);
  // -------------------------------------
  //  if (custom_eff == 1) {
  // dimAll(DIMSPEED);
  if (step % 2 == 0) {
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
static int getRandomPos(uint8_t STEP) {
  uint8_t val = floor(random(0, (STEP * 16 - WIDTH - 1)) / STEP) * STEP;
  return -val;
}

/* --------------------------------- */
static int getHue(uint8_t x, uint8_t y) {
  return ( x * 32 +  y * 24U );
}

/* --------------------------------- */
static uint8_t getSaturationStep() {
  return (modes[currentMode].Speed > 170U) ? ((HEIGHT > 24) ? 12 : 24) : 0;
}

/* --------------------------------- */
static uint8_t getBrightnessStep() {
  return (modes[currentMode].Speed < 85U) ? ((HEIGHT > 24) ? 16 : 24) : 0;
}

/* --------------------------------- */
static void drawPalette(int posX, int posY, uint8_t STEP) {
  int PX, PY;
  const uint8_t SZ = STEP - 1;
  const uint8_t maxY = floor(HEIGHT / SZ);
  uint8_t sat = getSaturationStep();
  uint8_t br  = getBrightnessStep();

  ledsClear(); // esphome: FastLED.clear();
  for (uint8_t y = 0; y < maxY; y++) {
    for (uint8_t x = 0; x < 16; x++) {
      PY = y * STEP;
      PX = posX + x * STEP;
      if ((PX >= - STEP ) && (PY >= - STEP) && (PX < WIDTH) && (PY < HEIGHT)) {
        // LOG.printf_P(PSTR("y: %03d | br • %03d | sat • %03d\n"), y, (240U - br * y), sat);
        drawRecCHSV(PX, PY, PX + SZ, PY + SZ, CHSV( getHue(x, y), (255U - sat * y), (240U - br * y)));
      }
    }
  }
}

/* --------------------------------- */
static void selectColor(uint8_t sc) {
  uint8_t offset = (WIDTH >= 16) ? WIDTH * 0.25 : 0;
  hue = getHue(random(offset, WIDTH - offset), random(HEIGHT));
  uint8_t sat = getSaturationStep();
  uint8_t br  = getBrightnessStep();

  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = offset; x < (WIDTH - offset); x++) {
      CHSV curColor = CHSV(hue, (255U - sat * y), (240U - br * y));
      if (curColor == getPixColorXY(x, y)) {
        /* show srlect color */
        drawRecCHSV(x, y, x + sc, y + sc, CHSV( hue, 64U, 255U));
        // ajs: FastLED.show();
        // ajs: delay(400);
        drawRecCHSV(x, y, x + sc, y + sc, CHSV( hue, 255U, 255U));
        y = HEIGHT;
        x = WIDTH;
      }
    }
  }
}

/* --------------------------------- */
static void WebTools() {
  const uint8_t FPS_D = 24U;
  static uint8_t STEP = 3U;
  static int posX = -STEP;
  static int posY = 0;
  static int nextX = -STEP * 2;
  static bool stop_moving = true;
  uint8_t speed = modes[currentMode].Speed > 65U ? modes[currentMode].Speed : 65U;   //constrain (modes[currentMode].Speed, 65, 255);
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      // scale | speed
       setModeSettings(random(10U, 90U), random(10U, 255U));
    }
#endif
    loadingFlag = false;
    FPSdelay = 1U;
    step = 0;
    STEP = 2U + floor(modes[currentMode].Scale / 35);
    posX = 0;
    posY = 0;
    drawPalette(posX, posY, STEP);
  }
  /* auto scenario */
  //switch (step) {
    if (step == 0){     /* restart ----------- */
      nextX = 0;
      FPSdelay = FPS_D;
    }
    else 
    if (step == speed/16+1){    /* start move -------- 16*/
      nextX = getRandomPos(STEP);
      FPSdelay = FPS_D;
    }
    else
    if (step == speed/10+1){    /* find --------------100 */
      nextX = getRandomPos(STEP);
      FPSdelay = FPS_D;
    }
    else
    if (step == speed/7+1){    /* find 2 ----------- 150*/
      nextX = getRandomPos(STEP);
      FPSdelay = FPS_D;
    }
    else
    if (step == speed/6+1){    /* find 3 -----------200 */
      nextX = - STEP * random(4, 8);
      // nextX = getRandomPos(STEP);
      FPSdelay = FPS_D;
    }
    else
    if (step == speed/5+1){   /* select color ------220 */
      FPSdelay = 200U;
      selectColor(STEP - 1);
    }
    else
    if (step == speed/4+1){   /* show color -------- 222*/
      FPSdelay = FPS_D;
      nextX = WIDTH;
    }
    else
    if (step == speed/4+3){
      step = 252;
    }
    
  //}
  if (posX < nextX) posX++;
  if (posX > nextX) posX--;

  if (stop_moving)   {
    FPSdelay = 80U;
    step++;
  } else {
    drawPalette(posX, posY, STEP);
    if ((nextX == WIDTH) || (nextX == 0)) {
      /* show select color bar gradient */
      // LOG.printf_P(PSTR("step: %03d | Next x: %03d • %03d | fps %03d\n"), step, nextX, posX, FPSdelay);
      if (posX > 1) {
        gradientHorizontal(0, 0, (posX - 1), HEIGHT, hue, hue, 255U, 96U, 255U);
      }
      if (posX > 3) DrawLine(posX - 3, CENTER_Y_MINOR, posX - 3, CENTER_Y_MAJOR, CHSV( hue, 192U, 255U));
    }
  }

  stop_moving = (posX == nextX); 
}
#endif


#ifdef DEF_WINE
// =============== Wine ================
//    © SlingMaster | by Alex Dovby
//               EFF_WINE
//--------------------------------------

static void colorsWine() {
  uint8_t divider;
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      // scale | speed
      setModeSettings(20U + random8(200U), 200U);
    }
#endif
    loadingFlag = false;
    fillAll(CHSV(55U, 255U, 65U));
    deltaValue = 255U - modes[currentMode].Speed + 1U;
    // minspeed 230 maxspeed 250 ============
    // minscale  40 maxscale  75 ============
    // красное вино hue > 0 & <=10
    // розовое вино hue > 10 & <=20
    // белое вино   hue > 20U & <= 40
    // шампанское   hue > 40U & <= 60

    deltaHue2 = 0U;                         // count для замедления смены цвета
    step = deltaValue;                      // чтообы при старте эффекта сразу покрасить лампу
    deltaHue = 1U;                          // direction | 0 hue-- | 1 hue++ |
    hue = 55U;                              // Start Color
    hue2 = 65U;                             // Brightness
    pcnt = 0;
  }

  deltaHue2++;
  // маштаб задает скорость изменения цвета 5 уровней
  divider = 5 - floor((modes[currentMode].Scale - 1) / 20);

  // возвращаем яркость для перехода к белому
  if (hue >= 10 && hue2 < 100U) {
    hue2++;
  }
  // уменьшаем яркость для красного вина
  if (hue < 10 && hue2 > 40U) {
    hue2--;
  }

  // изменение цвета вина -----
  if (deltaHue == 1U) {
    if (deltaHue2 % divider == 0) {
      hue++;
    }
  } else {
    if (deltaHue2 % divider == 0) {
      hue--;
    }
  }
  // --------

  // LOG.printf_P(PSTR("Wine | hue = %03d | Dir = %d | Brightness %03d | deltaHue2 %03d | divider %d | %d\n"), hue, deltaHue, hue2, deltaHue2, divider, step);

  // сдвигаем всё вверх -----------
  for (uint8_t x = 0U; x < WIDTH; x++) {
    for (uint8_t y = HEIGHT; y > 0U; y--) {
      drawPixelXY(x, y, getPixColorXY(x, y - 1U));
    }
  }

  if (hue > 40U) {
    // добавляем перляж для шампанского
    pcnt = random(0, WIDTH);
  } else {
    pcnt = 0;
  }

  // заполняем нижнюю строку с учетом перляжа
  for (uint8_t x = 0U; x < WIDTH; x++) {
    if ((x == pcnt) && (pcnt > 0)) {
      // с перляжем ------
      drawPixelXY(x, 0U, CHSV(hue, 150U, hue2 + 20U + random(0, 50U)));
    } else {
      drawPixelXY(x, 0U, CHSV(hue, 255U, hue2));
    }
  }

  // меняем направление изменения цвета вина от красного к шампанскому и обратно
  // в диапазоне шкалы HUE |0-60|
  if  (hue == 0U) {
    deltaHue = 1U;
  }
  if (hue == 60U) {
    deltaHue = 0U;
  }
  step++;
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

  uint8_t posX = CENTER_X_MAJOR - 3;
  uint8_t posY = 9;
  uint32_t color;
  if (HEIGHT > 16) {
    posY = CENTER_Y_MINOR - 1;
  }
  ledsClear(); // esphome: FastLED.clear();
  for (uint8_t y = 0U; y < 9; y++) {
    for (uint8_t x = 0U; x < 5; x++) {
      color = data[y][x];
      drawPixelXY(posX + x, posY - y, color);
    }
  }
}

static void Ukraine() {
  uint8_t divider;
  uint32_t color;
  //static const uint16_t MAX_TIME = 500;
  uint16_t tMAX = 100;
  static const uint8_t timeout = 100;
  static const uint32_t colors[2][5] = {
    {CRGB::Blue, CRGB::MediumBlue, 0x0F004F, 0x02002F, 0x1F2FFF },
    {CRGB::Yellow, CRGB::Gold, 0x4E4000, 0xFF6F00, 0xFFFF2F }
  };

  // Initialization =========================
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(250U), 200U + random8(50U));
    }
#endif
    loadingFlag = false;
    drawCrest();
    // minspeed 200 maxspeed 250 ============
    // minscale   0 maxscale 100 ============
    deltaValue = 255U - modes[currentMode].Speed + 1U;
    step = deltaValue;                        // чтообы при старте эффекта сразу покрасить лампу
    deltaHue2 = 0U;                           // count для замедления смены цвета
    deltaHue = 0U;                            // direction | 0 hue-- | 1 hue++ |
    hue2 = 0U;                                // Brightness
    ff_x = 1U;                                // counter
    tMAX = 100U;                              // timeout
  }
  divider = floor((modes[currentMode].Scale - 1) / 10); // маштаб задает режим рестарта
  tMAX = timeout + 100 * divider;

  if ((ff_x > timeout - 10) && (ff_x < timeout)) { // таймаут блокировки отрисовки флага
    if (ff_x < timeout - 5) {                      // размытие тризуба
      blurScreen(beatsin8(5U, 60U, 5U));
    } else {
      blurScreen(210U - ff_x);
    }
  }

  if (ff_x > tMAX) {
    if (divider == 0U) {                       // отрисовка тризуба только раз
      ff_x = 0U;
      tMAX += 20;
    } else {
      if (ff_x > tMAX + 100U * divider) {      // рестар эффект
        drawCrest();
        ff_x = 1U;
      }
    }
  }
  if ((ff_x != 0U) || (divider > 0)) {
    ff_x++;
  }

  // Flag Draw =============================
  if ((ff_x > timeout) || (ff_x == 0U))  {     // отрисовка флага
    if (step >= deltaValue) {
      step = 0U;
      hue2 = random8(WIDTH - 2);               // случайное смещение мазка по оси Y
      hue = random8(5);                        // flag color
      // blurScreen(dim8_raw(beatsin8(3, 64, 100)));
      // blurScreen(beatsin8(5U, 60U, 5U));
      // dimAll(200U);
    }
    if (step % 8 == 0 && modes[currentMode].Speed > 230) {
      blurScreen(beatsin8(5U, 5U, 72U));
    }
    hue2++;                                    // x
    deltaHue2++;                               // y

    if (hue2 >= WIDTH) {
      if (deltaHue2 > HEIGHT - 2 ) {           // если матрица высокая дорисовываем остальные мазки
        deltaHue2 = random8(5);                // изменяем положение по Y только отрисовав весь флаг
      }
      if (step % 2 == 0) {
        hue2 = 0U;
      } else {
        hue2 = random8(WIDTH);                 // смещение первого мазка по оси X
      }
    }

    if (deltaHue2 >= HEIGHT) {
      deltaHue2 = 0U;
      if (deltaValue > 200U) {
        hue = random8(5);                      // если низкая скорость меняем цвет после каждого витка
      }
    }

    if (deltaHue2 > floor(HEIGHT / 2) - 1) {    // меняем цвет для разных частей флага
      color = colors[0][hue];
    } else {
      color = colors[1][hue];
    }

    // LOG.printf_P(PSTR("color = %08d | hue2 = %d | speed = %03d | custom_eff = %d\n"), color, hue2, deltaValue, custom_eff);
    drawPixelXY(hue2, deltaHue2, color);
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
static uint8_t nextColor(uint8_t posY, uint8_t base, uint8_t next ) {
  const byte posLine = (HEIGHT > 16) ? 4 : 3;
  if ((posY + 1 == posLine) | (posY == posLine)) {
    return next;
  } else {
    return base;
  }
}

// --------------------------------------
static void Bamboo() {
  const uint8_t gamma[7] = {0, 32, 144, 160, 196, 208, 230};
  static float index;
  const byte DELTA = 4U;
  const uint8_t VG_STEP = 64U;
  const uint8_t V_STEP = 32U;
  const byte posLine = (HEIGHT > 16) ? 4 : 3;
  const uint8_t SX = 5;
  const uint8_t SY = 10;
  static float deltaX = 0;
  static bool direct = false;
  uint8_t posY;
  static uint8_t colLine;
  const float STP = 0.2;
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), random8(128, 255U));
    }
#endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    loadingFlag = false;
    index = STP;
    uint8_t idx = map(modes[currentMode].Scale, 5, 95, 0U, 6U);;
    colLine = gamma[idx];
    step = 0U;
  }

  // *** ---
  for (int y = 0; y < HEIGHT + SY; y++) {
    if (modes[currentMode].Scale < 50U) {
      if (step % 128 == 0U) {
        deltaX += STP * ((direct) ? -1 : 1);
        if ((deltaX > 1) | (deltaX < -1)) direct = !direct;
      }
    } else {
      deltaX = 0;
    }
    posY = y;
    for (int x = 0; x < WIDTH + SX; x++) {
      if (y == posLine) {
        drawPixelXYF(x , y - 1, CHSV(colLine, 255U, 128U));
        drawPixelXYF(x, y, CHSV(colLine, 255U, 96U));
        if (HEIGHT > 16) {
          drawPixelXYF(x, y - 2, CHSV(colLine, 10U, 64U));
        }
      }
      if ((x % SX == 0U) & (y % SY == 0U)) {
        for (int i = 1; i < (SY - 3); i++) {
          if (i < 3) {
            posY = y - i + 1 - DELTA + index;
            drawPixelXYF(x - 3 + deltaX, posY, CHSV(nextColor(posY, 96, colLine), 255U, 255 - V_STEP * i));
            posY = y - i + index;
            drawPixelXYF(x + deltaX, posY, CHSV(nextColor(posY, 96, colLine), 255U, 255 - VG_STEP * i));
          }
          posY = y - i - DELTA + index;
          drawPixelXYF(x - 4 + deltaX, posY , CHSV(nextColor(posY, 96, colLine), 180U, 255 - V_STEP * i));
          posY = y - i + 1 + index;
          drawPixelXYF(x - 1 + deltaX, posY , CHSV(nextColor(posY, ((i == 1) ? 96 : 80), colLine), 255U, 255 - V_STEP * i));
        }
      }
    }
    step++;
  }
  if (index >= SY)  {
    index = 0;
  }
  fadeToBlackBy(leds, NUM_LEDS, 60);
  index += STP;
}
#endif


#ifdef DEF_BALLROUTINE
// =====================================
//          Блуждающий кубик
// =====================================
//
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

    loadingFlag = false;
    //ledsClear(); // esphome: FastLED.clear();

    for (uint8_t i = 0U; i < 2U; i++) {
      coordB[i] = WIDTH / 2 * 10;
      vectorB[i] = random(8, 20);
    }
    // ballSize;
    deltaValue = map(modes[currentMode].Scale * 2.55, 0U, 255U, 2U, max((uint8_t)min(WIDTH, HEIGHT) / 3, 4));
    ballColor = CHSV(random(0, 9) * 28, 255U, 255U);
    _pulse_color = CHSV(random(0, 9) * 28, 255U, 255U);
  }

  //  if (!(modes[currentMode].Scale & 0x01))
  //  {
  //    hue += (modes[currentMode].Scale - 1U) % 11U * 8U + 1U;

  //    ballColor = CHSV(hue, 255U, 255U);
  //  }

  if ((modes[currentMode].Scale & 0x01)) {
    for (uint8_t i = 0U; i < deltaValue; i++) {
      for (uint8_t j = 0U; j < deltaValue; j++) {
        leds[XY(coordB[0U] / 10 + i, coordB[1U] / 10 + j)] = _pulse_color;
      }
    }
  }
  for (uint8_t i = 0U; i < 2U; i++) {
    coordB[i] += vectorB[i];
    if (coordB[i] < 0) {
      coordB[i] = 0;
      vectorB[i] = -vectorB[i];
      if (RANDOM_COLOR) ballColor = CHSV(random(0, 9) * 28, 255U, 255U); // if (RANDOM_COLOR && (modes[currentMode].Scale & 0x01))
      //vectorB[i] += random(0, 6) - 3;
    }
  }
  if (coordB[0U] > (int16_t)((WIDTH - deltaValue) * 10)) {
    coordB[0U] = (WIDTH - deltaValue) * 10;
    vectorB[0U] = -vectorB[0U];
    if (RANDOM_COLOR) ballColor = CHSV(random(0, 9) * 28, 255U, 255U);
    //vectorB[0] += random(0, 6) - 3;
  }
  if (coordB[1U] > (int16_t)((HEIGHT - deltaValue) * 10)) {
    coordB[1U] = (HEIGHT - deltaValue) * 10;
    vectorB[1U] = -vectorB[1U];
    if (RANDOM_COLOR) ballColor = CHSV(random(0, 9) * 28, 255U, 255U);
    //vectorB[1] += random(0, 6) - 3;
  }

  //  if (modes[currentMode].Scale & 0x01)
  //    dimAll(135U);
  // dimAll(255U - (modes[currentMode].Scale - 1U) % 11U * 24U);
  //  else
  ledsClear(); // esphome: FastLED.clear();

  for (uint8_t i = 0U; i < deltaValue; i++) {
    for (uint8_t j = 0U; j < deltaValue; j++) {
      leds[XY(coordB[0U] / 10 + i, coordB[1U] / 10 + j)] = ballColor;
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
  float radius2 = 255.0 / points;
  for (int i = 0; i < points; i++) {
    DrawLine(xlocl + ((little * (sin8(i * radius2 + radius2 / 2 - dangle) - 128.0)) / 128), ylocl + ((little * (cos8(i * radius2 + radius2 / 2 - dangle) - 128.0)) / 128), xlocl + ((biggy * (sin8(i * radius2 - dangle) - 128.0)) / 128), ylocl + ((biggy * (cos8(i * radius2 - dangle) - 128.0)) / 128), ColorFromPalette(*curPalette, koler));
    DrawLine(xlocl + ((little * (sin8(i * radius2 - radius2 / 2 - dangle) - 128.0)) / 128), ylocl + ((little * (cos8(i * radius2 - radius2 / 2 - dangle) - 128.0)) / 128), xlocl + ((biggy * (sin8(i * radius2 - dangle) - 128.0)) / 128), ylocl + ((biggy * (cos8(i * radius2 - dangle) - 128.0)) / 128), ColorFromPalette(*curPalette, koler));

  }
}

// --------------------------------------
static void EffectStars() {
#define STARS_NUM (8U)
#define STAR_BLENDER (128U)
#define CENTER_DRIFT_SPEED (6U)
  static uint8_t spd;
  static uint8_t points[STARS_NUM];
  static float color[STARS_NUM] ;
  static int delay_arr[STARS_NUM];
  static float counter;
  static float driftx;
  static float  drifty;
  static float cangle;
  static float  sangle;
  static uint8_t stars_count;
  static uint8_t blur;

  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), random8(80U, 255U));
    }
#endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    loadingFlag = false;
    counter = 0.0;
    // стартуем с центра
    driftx = (float)WIDTH / 2.0;
    drifty = (float)HEIGHT / 2.0;

    cangle = (float)(sin8(random8(25, 220)) - 128.0f) / 128.0f; //angle of movement for the center of animation gives a float value between -1 and 1
    sangle = (float)(sin8(random8(25, 220)) - 128.0f) / 128.0f; //angle of movement for the center of animation in the y direction gives a float value between -1 and 1
    spd = modes[currentMode].Speed;
    blur = modes[currentMode].Scale / 2;
    stars_count = WIDTH / 2U;

    if (stars_count > STARS_NUM) stars_count = STARS_NUM;
    for (uint8_t num = 0; num < stars_count; num++) {
      points[num] = map(modes[currentMode].Scale, 1, 255, 3U, 7U); //5; // random8(3, 6);                              // количество углов в звезде
      delay_arr[num] = spd / 5 + (num << 2) + 2U;               // задержка следующего пуска звезды
      color[num] = random8();
    }
  }
  // fadeToBlackBy(leds, NUM_LEDS, 245);
  fadeToBlackBy(leds, NUM_LEDS, 165);
  float speedFactor = ((float)spd / 380.0 + 0.05);
  counter += speedFactor;                                                   // определяет то, с какой скоростью будет приближаться звезда

  if (driftx > (WIDTH - spirocenterX / 2U)) cangle = 0 - fabs(cangle);      //change directin of drift if you get near the right 1/4 of the screen
  if (driftx < spirocenterX / 2U) cangle = fabs(cangle);                    //change directin of drift if you get near the right 1/4 of the screen
  if ((uint16_t)counter % CENTER_DRIFT_SPEED == 0) driftx = driftx + (cangle * speedFactor); //move the x center every so often
  if (drifty > ( HEIGHT - spirocenterY / 2U)) sangle = 0 - fabs(sangle);    // if y gets too big, reverse
  if (drifty < spirocenterY / 2U) sangle = fabs(sangle);                    // if y gets too small reverse

  if ((uint16_t)counter % CENTER_DRIFT_SPEED == 0) drifty = drifty + (sangle * speedFactor); //move the y center every so often

  for (uint8_t num = 0; num < stars_count; num++) {
    if (counter >= delay_arr[num]) {              //(counter >= ringdelay)
      if (counter - delay_arr[num] <= WIDTH + 5) {
        drawStar(driftx, drifty, 2 * (counter - delay_arr[num]), (counter - delay_arr[num]), points[num], STAR_BLENDER + color[num], color[num]);
        color[num] += speedFactor;                // в зависимости от знака - направление вращения
      } else {
        delay_arr[num] = counter + (stars_count << 1) + 1U; // задержка следующего пуска звезды
      }
    }
  }
  blur2d(WIDTH, HEIGHT, blur);
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

#define M_PI_2  1.57079632679489661923
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
  //http://pubs.opengroup.org/onlinepubs/009695399/functions/atan2.html
  //Volkan SALMA

  const float ONEQTR_PI = PI / 4.0;
  const float THRQTR_PI = 3.0 * PI / 4.0;
  float r, angle;
  float abs_y = fabs(y) + 1e-10f;      // kludge to prevent 0/0 condition
  if ( x < 0.0f ) {
    r = (x + abs_y) / (abs_y - x);
    angle = THRQTR_PI;
  } else {
    r = (x - abs_y) / (x + abs_y);
    angle = ONEQTR_PI;
  }
  angle += (0.1963f * r * r - 0.9817f) * r;
  if ( y < 0.0f ) {
    return ( -angle );    // negate if in quad III or IV
  } else {
    return ( angle );
  }
}

// --------------------------------------
static float atan_fast(float x) {
  /* A fast look-up method with enough accuracy */
  if (x > 0) {
    if (x <= 1) {
      int index = round(x * 100);
      return LUT[index];
    } else {
      float re_x = 1 / x;
      int index = round(re_x * 100);
      return (M_PI_2 - LUT[index]);
    }
  } else {
    if (x >= -1) {
      float abs_x = -x;
      int index = round(abs_x * 100);
      return -(LUT[index]);
    } else {
      float re_x = 1 / (-x);
      int index = round(re_x * 100);
      return (LUT[index] - M_PI_2);
    }
  }
}

// --------------------------------------
static float tan2pi_fast(float x) {
  float y = (1 - x * x);
  return x * (((-0.000221184 * y + 0.0024971104) * y - 0.02301937096) * y + 0.3182994604 + 1.2732402998 / y);
}


// --------------------------------------
static float code(double t, double i, double x, double y) {
  switch (pcnt) {
    /** © Motus Art @motus_art */
    case 1: /* Plasma */
      hue = 96U; hue2 = 224U;
      return (sin16((x + t) * 8192.0) * 0.5 + sin16((y + t) * 8192.0) * 0.5 + sin16((x + y + t) * 8192.0) * 0.3333333333333333) / 32767.0;
      break;

    case 2: /* Up&Down */
      //return sin(cos(x) * y / 8 + t);
      hue = 255U; hue2 = 160U;
      return sin16((cos16(x * 8192.0) / 32767.0 * y / (HEIGHT / 2.0) + t) * 8192.0) / 32767.0;
      break;

    case 3:
      hue = 255U; hue2 = 96U;
      return sin16((atan_fast(y / x) + t) * 8192.0) / 32767.0;
      break;

    /** © tixy.land website */
    case 4: /* Emitting rings */
      hue = 255U; hue2 = 0U;
      return sin16((t - sqrt3((x - (WIDTH / 2)) * (x - (WIDTH / 2)) + (y - (HEIGHT / 2)) * (y - (HEIGHT / 2)))) * 8192.0) / 32767.0;
      break;

    case 5: /* Rotation  */
      hue = 136U; hue2 = 48U;
      return sin16((PI * 2.5 * atan_fast((y - (HEIGHT / 2)) / (x - (WIDTH / 2))) + 5 * t) * 8192.0) / 32767.0;
      break;

    case 6: /* Vertical fade */
      hue = 160U; hue2 = 0U;
      return sin16((y / 8 + t) * 8192.0) / 32767.0;
      break;

    case 7: /* Waves */
      //return sin(x / 2) - sin(x - t) - y + 6;
      hue = 48U; hue2 = 160U;
      return (sin16(x * 4096.0) - sin16((x - t) * 8192.0)) / 32767.0 - y + (HEIGHT / 2);
      break;

    case 8: /* Drop */
      hue = 136U; hue2 = 160U;
      return fmod(8 * t, 13) - sqrt3((x - (WIDTH / 2)) * (x - (WIDTH / 2)) + (y - (HEIGHT / 2)) * (y - (HEIGHT / 2))); //hypot(x - (WIDTH/2), y - (HEIGHT/2));
      break;

    case 9: /* Ripples @thespite */
      hue = 96U; hue2 = 224U;
      return sin16((t - sqrt3(x * x + y * y)) * 8192.0) / 32767.0;
      break;

    case 10: /* Bloop bloop bloop @v21 */
      hue = 136U; hue2 = 160U;
      return (x - (WIDTH / 2)) * (y - (HEIGHT / 2)) - sin16(t * 4096.0) / 512.0;
      break;

    case 11: /* SN0WFAKER */
      // https://www.reddit.com/r/programming/comments/jpqbux/minimal_16x16_dots_coding_environment/gbgk7c0/
      hue = 96U; hue2 = 160U;
      return sin16((atan_fast((y - (HEIGHT / 2)) / (x - (WIDTH / 2))) + t) * 8192.0) / 32767.0;
      break;
    case 12: /* detunized */
      // https://www.reddit.com/r/programming/comments/jpqbux/minimal_16x16_dots_coding_environment/gbgk30l/
      hue = 136U; hue2 = 160U;
      return sin16((y / (HEIGHT / 2) + t * 0.5) * 8192.0) / 32767.0 + x / 16 - 0.5;
      break;

    /** © @akella | https://twitter.com/akella/status/1323549082552619008 */
    case 13:
      hue = 255U; hue2 = 0U;
      return sin16((6 * atan2_fast(y - (HEIGHT / 2), x) + t) * 8192.0) / 32767.0;
      break;
    case 14:
      hue = 32U; hue2 = 160U;
      return sin16((i / 5 + t) * 16384.0) / 32767.0;
      break;

    /** © Paul Malin | https://twitter.com/P_Malin/ */

    // sticky blood
    // by @joeytwiddle
    //(t,i,x,y) => y-t*3+9+3*cos(x*3-t)-5*sin(x*7)

    //      if (x < 8) {
    //       // hue = 160U;
    //      } else {
    //       // hue = 96U;
    //      }
    //      if ((y == HEIGHT -1)&(x == 8)) {
    //        hue = hue + 30;
    //        if (hue >= 255U) {
    //          hue = 0;
    //        }
    //      }
    //      hue = t/128+8;

    //    case 19: // !!!! paint
    //      // Matrix Rain https://twitter.com/P_Malin/status/1323583013880553472
    //      //return 1. - fmod((x * x - y + t * (fmod(1 + x * x, 5)) * 6), 16) / 16;
    //      return 1. - fmod((x * x - (HEIGHT - y) + t * (1 + fmod(x * x, 5)) * 3), WIDTH) / HEIGHT;
    //      break;

    case 15: /* Burst */
      // https://twitter.com/P_Malin/status/1323605999274594304
      hue = 136U; hue2 = 160U;
      return -10. / ((x - (WIDTH / 2)) * (x - (WIDTH / 2)) + (y - (HEIGHT / 2)) * (y - (HEIGHT / 2)) - fmod(t * 0.3, 0.7) * 200);
      break;

    case 16: /* Rays */
      hue = 255U; hue2 = 0U;
      return sin16((atan2_fast(x, y) * 5 + t * 2) * 8192.0) / 32767.0;
      break;

    case 17: /* Starfield */
      // org | https://twitter.com/P_Malin/status/1323702220320313346
      hue = 255U; hue2 = 160U;
      return !((int)(x + t * 50 / (fmod(y * y, 5.9) + 1)) & 15) / (fmod(y * y, 5.9) + 1);
      //      {
      //        uint16_t _y = HEIGHT - y;
      //        float d = (fmod(_y * _y + 4, 4.1) + 0.85) * 0.5; // коэффициенты тут отвечают за яркость (размер), скорость, смещение, подбираются экспериментально :)
      //        return !((int)(x + t * 7.0 / d) & 15) / d; // 7.0 - множитель скорости
      //      }
      break;

    case 18:
      hue = 255U; hue2 = 0U;
      return sin16((3.5 * atan2_fast(y - (HEIGHT / 2) + sin16(t * 8192.0) * 0.00006, x - (WIDTH / 2) + sin16(t * 8192.0) * 0.00006) + t * 1.5 + 5) * 8192.0) / 32767.0;
      break;

    case 19:
      hue = 255U; hue2 = 224U;
      return (y - 8) / 3 - tan2pi_fast((x / 6 + 1.87) / PI * 2) * sin16(t * 16834.0) / 32767.0;
      break;

    case 20:
      hue = 136U; hue2 = 160U;
      return (y - 8) / 3 - (sin16((x / 4 + t * 2) * 8192.0) / 32767.0);
      break;

    case 21:
      hue = 72U; hue2 = 96U;
      return cos(sin16(x * t * 819.2) / 32767.0 * PI) + cos16((sin16((y * t / 10 + (sqrt3(abs(cos16(x * t * 8192.0) / 32767.0)))) * 8192.0) / 32767.0 * PI) * 8192.0) / 32767.0;
      break;

    case 22: /* bambuk */
      hue = 96U; hue2 = 80U;
      return sin16(x / 3 * sin16(t * 2730.666666666667) / 2.0) / 32767.0 + cos16(y / 4 * sin16(t * 4096.0) / 2.0) / 32767.0;
      break;

    case 23:
      hue = 0U; hue2 = 224U;
      {
        float _x = x - fmod(t, WIDTH);
        float _y = y - fmod(t, HEIGHT);
        return -.4 / (sqrt3(_x * _x + _y * _y) - fmod(t, 2) * 9);
      }
      break;

    case 24: /* honey */
      hue = 255U; hue2 = 40U;
      return sin16(y * t * 2048.0) / 32767.0 * cos16(x * t * 2048.0) / 32767.0;
      break;

    case 25:
      hue = 96U; hue2 = 160U;
      return atan_fast((x - (WIDTH / 2)) * (y - (HEIGHT / 2))) - 2.5 * sin16(t * 8192.0) / 32767.0;
      break;

    default:
      if (pcnt > 25) {
        deltaHue2 += 32;
      }
      pcnt = 1;
      hue = 96U; hue2 = 0U;
      return sin16(t * 8192.0) / 32767.0;
      break;
  }
}

// --------------------------------------
static void processFrame(double t, double x, double y) {
  double i = (y * WIDTH) + x;
  double frame = constrain(code(t, i, x, y), -1, 1) * 255;
  if (frame > 0) {
    if ( hue == 255U) {
      drawPixelXY(x, y, CRGB(frame, frame, frame));
    } else {
      drawPixelXY(x, y, CHSV(hue, frame, frame));
    }
  } else {
    if (frame < 0) {
      if (modes[currentMode].Scale < 5) deltaHue2 = 0;
      drawPixelXY(x, y, CHSV(hue2 + deltaHue2, frame * -1, frame * -1));
    } else {
      drawPixelXY(x, y, CRGB::Black);
    }
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
    loadingFlag = false;
    deltaHue = 0;
    pcnt = map(modes[currentMode].Speed, 5, 250, 1U, 25U);
    FPSdelay = 1;
    deltaHue2 = modes[currentMode].Scale * 2.55;
    hue = 255U; hue2 = 0U;
  }
  // *****
  unsigned long milli = millis();
  double t = milli / 1000.0;

  EVERY_N_SECONDS(20) {
    if ((modes[currentMode].Speed < 5) || (modes[currentMode].Speed > 250)) {
      pcnt++;
    }
  }
  for ( double x = 0; x < WIDTH; x++) {
    for ( double y = 0; y < HEIGHT; y++) {
      processFrame(t, x, y);
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
  return (leds[idx].r + leds[idx].g + leds[idx].b);
}

class Spark {
  private:
    CRGB color;
    uint8_t Bri;
    uint8_t Hue;
    float x, y, speedy = (float)random(5, 30) / 10;

  public:
    void addXY(float nx, float ny) {
      //drawPixelXYF(x, y, 0);
      x += nx;
      y += ny * speedy;
    }

    float getY() {
      return y;
    }

    void reset() {
      uint32_t peak = 0;
      speedy = (float)random(5, 30) / 10;
      y = random(HEIGHT / 4, HEIGHT / 2);
      for (uint8_t i = 0; i < WIDTH; i++) {
        uint32_t temp = RGBweight(XY(i, y));
        if (temp > peak) {
          x = i;
          peak = temp;
        }
      }

      color = leds[XY(x, y)];
    }

    void draw() {
      color.fadeLightBy(256 / (HEIGHT * 0.75));
      drawPixelXYF(x, y, color);
    }
};

const byte sparksCount = WIDTH / 4;
static Spark sparks[sparksCount];

//---------------------------------------
static void  FireSparks() {
  bool withSparks = false; // true/false
  static uint32_t t;
  const uint8_t spacer = HEIGHT / 4;
  byte scale = 50;

  if (loadingFlag) {

#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      // scale | speed
      setModeSettings(random(0U, 99U), random(20U, 100U));
    }
#endif
    loadingFlag = false;
    FPSdelay = DYNAMIC;
    for (byte i = 0; i < sparksCount; i++) sparks[i].reset();
  }
  withSparks = modes[currentMode].Scale >= 50;
  t += modes[currentMode].Speed;

  if (withSparks)
    for (byte i = 0; i < sparksCount; i++) {
      sparks[i].addXY((float)random(-1, 2) / 2, 0.75);
      if (sparks[i].getY() > HEIGHT and !random(0, 50)) sparks[i].reset();
      else sparks[i].draw();
    }

  for (byte x = 0; x < WIDTH; x++) {
    for (byte y = 0; y < HEIGHT; y++) {
      int16_t Bri = inoise8(x * scale, (y * scale) - t) - ((withSparks ? y + spacer : y) * (255 / HEIGHT));
      byte Col = Bri;
      if (Bri < 0) Bri = 0; if (Bri != 0) Bri = 256 - (Bri * 0.2);
      nblend(leds[XY(x, y)], ColorFromPalette(HeatColors_p, Col, Bri), modes[currentMode].Speed);
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
    float thickness = 3.0;
    long startTime;
    uint16_t offset;
    int16_t centerX;
    int16_t centerY;
    int hue;
    int bpm = 10;

    void move() {
      centerX = random(0, WIDTH);
      centerY = random(0, HEIGHT);
    }

    void scroll() {
      centerX--; // = random(0, WIDTH);
      if (centerX < 1) {
        centerX = WIDTH - 1;
      }
      centerY++;
      if (centerY > HEIGHT) {
        centerY = 0;
      }
    }
    void reset() {
      startTime = millis();
      centerX = random(0, WIDTH);
      centerY = random(0, HEIGHT);
      hue = random(0, 255);
      offset = random(0, 60000 / bpm);
    }

    float radius() {
      float radius = beatsin16(modes[currentMode].Speed / 2.5, 0, 500, offset) / 100.0;
      return radius;
    }
};

// -----------------------------------
namespace Circles {
#define NUMBER_OF_CIRCLES WIDTH/2
static Circle circles[NUMBER_OF_CIRCLES] = {};

static void drawCircle(Circle circle) {
  int16_t centerX = circle.centerX;
  int16_t centerY = circle.centerY;
  int hue = circle.hue;
  float radius = circle.radius();

  int16_t startX = centerX - ceil(radius);
  int16_t endX = centerX + ceil(radius);
  int16_t startY = centerY - ceil(radius);
  int16_t endY = centerY + ceil(radius);

  for (int16_t x = startX; x < endX; x++) {
    for (int16_t y = startY; y < endY; y++) {
      int16_t index = XY(x, y);
      if (index < 0 || index > NUM_LEDS)
        continue;
      double distance = sqrt(sq(x - centerX) + sq(y - centerY));
      if (distance > radius)
        continue;

      uint16_t brightness;
      if (radius < 1) { // last pixel
        // brightness = 0; //255.0 * radius;
        deltaValue = 20;
        brightness = 180;
        // brightness = 0;
      } else {
        deltaValue = 200; // 155 + modes[currentMode].Scale;
        double percentage = distance / radius;
        double fraction = 1.0 - percentage;
        brightness = 255.0 * fraction;
      }
      leds[index] += CHSV(hue, deltaValue, brightness);
    }
  }
}

// -----------------------------
static void draw(bool setup) {
  fadeToBlackBy(leds, NUM_LEDS, 100U);
  // fillAll(CRGB::Black);
  for (int i = 0; i < NUMBER_OF_CIRCLES; i++) {
    if (setup) {
      circles[i].reset();
    } else {
      if (circles[i].radius() < 0.5) {
        circles[i].scroll();
      }
    }
    drawCircle(circles[i]);
  }
}
}; // namespace Circles

// ==============
static void Dandelions() {
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      // scale | speed
      setModeSettings(random8(1U, 100U), random8(10U, 255U));
    }
#endif
    loadingFlag = false;
    ledsClear(); // esphome: FastLED.clear();
    Circles::draw(true);
    // deltaValue = 150 + modes[currentMode].Scale;
    deltaValue = 155 + modes[currentMode].Scale;
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
  const byte PADDING = HEIGHT * 0.25;
  const byte BR_INTERWAL = 64 / HEIGHT;
  const byte DELTA = WIDTH  * 0.25;
  // ---------------------

  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      setModeSettings(random8(4, 50), random8(4, 254U));
    }
#endif
    loadingFlag = false;
    deltaValue = 0;
    hue = 0;
    ledsClear(); // esphome: FastLED.clear();
  }
  // ---------------------

  byte step1 = map8(modes[currentMode].Speed, 10U, 60U);
  uint16_t ms = millis();
  double freq = 3000;
  float mn = 255.0 / 13.8;
  byte fade = 180 - abs(128 - step);
  fadeToBlackBy(leds, NUM_LEDS, fade);

  // -----------------
  for (uint16_t y = 0; y < HEIGHT; y++) {
    uint32_t yy = y * 256;
    uint32_t x1 = beatsin16(step1, WIDTH, (HEIGHT - 1) * 256, WIDTH, y * freq + 32768) / 2;

    // change color --------
    CRGB col1 = CHSV(ms / 29 + y * 256 / (HEIGHT - 1) + 128, 255, 255 - (HEIGHT - y) * BR_INTERWAL);
    CRGB col2 = CHSV(ms / 29 + y * 256 / (HEIGHT - 1),       255, 255 - (HEIGHT - y) * BR_INTERWAL);
    // CRGB col3 = CHSV(ms / 29 + y * 256 / (HEIGHT - 1) + step, 255, 255 - (HEIGHT - y) * BR_INTERWAL - fade);

    wu_pixel( (uint32_t)(x1 + hue * DELTA),                                 (uint32_t)(yy - PADDING * (255 - hue)), &col1);
    wu_pixel( (uint32_t)abs((int)((WIDTH - 1) * 256 - (x1 + hue * DELTA))), (uint32_t)(yy - PADDING * hue),         &col2);
  }

  step++;
  if (step % 64) {
    if (deltaValue == 0) {
      hue++;
      if (hue >= 255) {
        deltaValue = 1;
      }
    } else {
      hue--;
      if (hue < 1) {
        deltaValue = 0;
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
static void drawRandomCol(uint8_t x, uint8_t y, uint8_t offset, uint32_t count) {
  const byte STEP = 32;
  const byte D = HEIGHT / 8;
  uint8_t color = floor(y / D) * STEP + offset;

  if (count == 0U) {
    drawPixelXY(x, y, CHSV(color, 255, random8(8U) == 0U ? (step % 2U ? 0 : 255) : 0));
  } else {
    drawPixelXY(x, y, CHSV(color, 255, (bitRead(count, y ) == 1U) ? (step % 5U ? 0 : 255) : 0));
  }
}

//---------------------------------------
static void Turbulence() {
  const byte STEP_COLOR = 255 / HEIGHT;
  const byte STEP_OBJ = 8;
  const byte DEPTH = 2;
  static uint32_t count; // 16777216; = 65536
  uint32_t curColor;
  if (loadingFlag) {
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), random8(1, 255U));
    }
#endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    loadingFlag = false;
    step = 0U;
    deltaValue = 0;
    hue = 0;
    if (modes[currentMode].Speed < 20U) {
      FPSdelay = SpeedFactor(30);
    }
    ledsClear(); // esphome: FastLED.clear();
  }

  deltaValue++;     /* size morph  */

  /* <==== scroll =====> */
  for (uint8_t y = HEIGHT; y > 0; y--) {
    drawRandomCol(0, y - 1, hue, count);
    drawRandomCol(WIDTH - 1, y - 1, hue + 128U, count);

    // left -----
    for (uint8_t x = CENTER_X_MAJOR - 1; x > 0; x--) {
      if (x > CENTER_X_MAJOR) {
        if (random8(2) == 0U) { /* scroll up */
          CRGB newColor = getPixColorXY(x, y - 1 );
        }
      }

      /* ---> */
      curColor = getPixColorXY(x - 1, y - 1);
      if (x < CENTER_X_MAJOR - DEPTH / 2) {
        drawPixelXY(x, y - 1, curColor);
      } else {
        if (curColor != 0U) drawPixelXY(x, y - 1, curColor);
      }
    }

    // right -----
    for (uint8_t x = CENTER_X_MAJOR + 1; x < WIDTH; x++) {
      if (x < CENTER_X_MAJOR + DEPTH ) {
        if (random8(2) == 0U)  {  /* scroll up */
          CRGB newColor = getPixColorXY(x, y - 1 );
        }
      }
      /* <---  */
      curColor = getPixColorXY(x, y - 1);
      if (x > CENTER_X_MAJOR + DEPTH / 2 ) {
        drawPixelXY(x - 1, y - 1, curColor);
      } else {
        if (curColor != 0U) drawPixelXY(x - 1, y - 1, curColor);
      }
    }

    /* scroll center up ---- */
    for (uint8_t x = CENTER_X_MAJOR - DEPTH; x < CENTER_X_MAJOR + DEPTH; x++) {
      drawPixelXY(x, y,  makeDarker(getPixColorXY(x, y - 1 ), 128 / y));
      if (y == 1) {
        drawPixelXY(x, 0, CRGB::Black);
      }
    }
    /* --------------------- */
  }

  if (modes[currentMode].Scale > 50) {
    count++;
    if (count % 256 == 0U) hue += 16U;
  } else {
    count = 0;
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
  if ((arrow_direction & 0x01) > 0) {
    arrow_hue[0] = random8();
    arrow_x[0]   = 0;
    arrow_y[0]   = (int8_t)HEIGHT / 2;
    stop_x[0]    = (int8_t)WIDTH + 7; // скрывается за экраном на 7 пикселей
    stop_y[0]    = 0;                 // неприменимо
  }
  // снизу вверх
  if ((arrow_direction & 0x02) > 0) {
    arrow_hue[1] = random8();
    arrow_y[1]   = 0;
    arrow_x[1]   = (int8_t)WIDTH / 2;
    stop_y[1]    = (int8_t)HEIGHT + 7; // скрывается за экраном на 7 пикселей
    stop_x[1]    = 0;                  // неприменимо
  }
  // справа налево
  if ((arrow_direction & 0x04) > 0) {
    arrow_hue[2] = random8();
    arrow_x[2]   = (int8_t)WIDTH - 1;
    arrow_y[2]   = (int8_t)HEIGHT / 2;
    stop_x[2]    = -7; // скрывается за экраном на 7 пикселей
    stop_y[2]    = 0;  // неприменимо
  }
  // сверху вниз
  if ((arrow_direction & 0x08) > 0) {
    arrow_hue[3] = random8();
    arrow_y[3]   = (int8_t)HEIGHT - 1;
    arrow_x[3]   = (int8_t)WIDTH / 2;
    stop_y[3]    = -7; // скрывается за экраном на 7 пикселей
    stop_x[3]    = 0;  // неприменимо
  }
}

static void arrowSetup_mode2() {
  // Слева направо до половины экрана
  if ((arrow_direction & 0x01) > 0) {
    arrow_hue[0] = random8();
    arrow_x[0]   = 0;
    arrow_y[0]   = (int8_t)HEIGHT / 2;
    stop_x[0]    = (int8_t)WIDTH / 2 - 1; // до центра экрана
    stop_y[0]    = 0;                     // неприменимо
  }
  // снизу вверх до половины экрана
  if ((arrow_direction & 0x02) > 0) {
    arrow_hue[1] = random8();
    arrow_y[1]   = 0;
    arrow_x[1]   = (int8_t)WIDTH / 2;
    stop_y[1]    = (int8_t)HEIGHT / 2 - 1; // до центра экрана
    stop_x[1]    = 0;                      // неприменимо
  }
  // справа налево до половины экрана
  if ((arrow_direction & 0x04) > 0) {
    arrow_hue[2] = random8();
    arrow_x[2]   = (int8_t)WIDTH - 1;
    arrow_y[2]   = (int8_t)HEIGHT / 2;
    stop_x[2]    = (int8_t)WIDTH / 2; // до центра экрана
    stop_y[2]    = 0;                 // неприменимо
  }
  // сверху вниз до половины экрана
  if ((arrow_direction & 0x08) > 0) {
    arrow_hue[3] = random8();
    arrow_y[3]   = (int8_t)HEIGHT - 1;
    arrow_x[3]   = (int8_t)WIDTH / 2;
    stop_y[3]    = (int8_t)HEIGHT / 2; // до центра экрана
    stop_x[3]    = 0;                  // неприменимо
  }
}

static void arrowSetup_mode4() {
  // Слева направо
  if ((arrow_direction & 0x01) > 0) {
    arrow_hue[0] = random8();
    arrow_x[0]   = 0;
    arrow_y[0]   = ((int8_t)HEIGHT / 3) * 2;
    stop_x[0]    = (int8_t)WIDTH + 7; // скрывается за экраном на 7 пикселей
    stop_y[0]    = 0;                 // неприменимо
  }
  // снизу вверх
  if ((arrow_direction & 0x02) > 0) {
    arrow_hue[1] = random8();
    arrow_y[1]   = 0;
    arrow_x[1]   = ((int8_t)WIDTH / 3) * 2;
    stop_y[1]    = (int8_t)HEIGHT + 7; // скрывается за экраном на 7 пикселей
    stop_x[1]    = 0;                  // неприменимо
  }
  // справа налево
  if ((arrow_direction & 0x04) > 0) {
    arrow_hue[2] = random8();
    arrow_x[2]   = (int8_t)WIDTH - 1;
    arrow_y[2]   = (int8_t)HEIGHT / 3;
    stop_x[2]    = -7; // скрывается за экраном на 7 пикселей
    stop_y[2]    = 0;  // неприменимо
  }
  // сверху вниз
  if ((arrow_direction & 0x08) > 0) {
    arrow_hue[3] = random8();
    arrow_y[3]   = (int8_t)HEIGHT - 1;
    arrow_x[3]   = (int8_t)WIDTH / 3;
    stop_y[3]    = -7; // скрывается за экраном на 7 пикселей
    stop_x[3]    = 0;  // неприменимо
  }
}

static void arrowSetupForMode(uint8_t mode, bool change) {
  switch (mode) {
    case 1:
      if (change) arrow_direction = 1;
      arrowSetup_mode1(); // От края матрицы к краю, по центру гориз и верт
      break;
    case 2:
      if (change) arrow_direction = 5;
      arrowSetup_mode2(); // По центру матрицы (гориз / верт) - ограничение -
                          // центр матрицы
      break;
    case 3:
      if (change) arrow_direction = 15;
      arrowSetup_mode2(); // как и в режиме 2 - по центру матрицы (гориз / верт) -
                          // ограничение - центр матрицы
      break;
    case 4:
      if (change) arrow_direction = 5;
      arrowSetup_mode4(); // От края матрицы к краю, верт / гориз
      break;
    case 5:
      if (change) arrow_direction = 15;
      arrowSetup_mode4(); // как и в режиме 4 от края матрицы к краю, на 1/3
      break;
  }
}

static void arrowsRoutine() {
  if (loadingFlag) {
    loadingFlag = false;

    ledsClear(); // esphome: FastLED.clear();

    arrow_complete = false;
    arrow_mode = (arrow_mode_orig == 0 || arrow_mode_orig > 5)
                     ? random8(1, 5)
                     : arrow_mode_orig;

    arrow_play_mode_count_orig[0] = 0;
    arrow_play_mode_count_orig[1] = 4; // 4 фазы - все стрелки показаны по кругу
                                       // один раз - переходить к следующему ->
    arrow_play_mode_count_orig[2] = 4; // 2 фазы - гориз к центру (1), затем верт к центру (2) - обе фазы
                                       // повторить по 2 раза -> 4
    arrow_play_mode_count_orig[3] = 4; // 1 фаза - все к центру (1) повторить по 4 раза -> 4
    arrow_play_mode_count_orig[4] = 4; // 2 фазы - гориз к центру (1), затем верт к центру (2) - обе фазы
                                       // повторить по 2 раза -> 4
    arrow_play_mode_count_orig[5] = 4; // 1 фаза - все сразу (1) повторить по 4 раза -> 4

    for (uint8_t i = 0; i < 6; i++) {
      arrow_play_mode_count[i] = arrow_play_mode_count_orig[i];
    }
    arrowSetupForMode(arrow_mode, true);
  }

  dimAll(160);
  CHSV color;

  // движение стрелки - cлева направо
  if ((arrow_direction & 0x01) > 0) {
    color = CHSV(arrow_hue[0], 255, modes[currentMode].Brightness);
    for (int8_t x = 0; x <= 4; x++) {
      for (int8_t y = 0; y <= x; y++) {
        if (arrow_x[0] - x >= 0 && arrow_x[0] - x <= stop_x[0]) {
          CHSV clr = (x < 4 || (x == 4 && y < 2)) ? color : CHSV(0, 0, 0);
          drawPixelXY(arrow_x[0] - x, arrow_y[0] - y, clr);
          drawPixelXY(arrow_x[0] - x, arrow_y[0] + y, clr);
        }
      }
    }
    arrow_x[0]++;
  }

  // движение стрелки - cнизу вверх
  if ((arrow_direction & 0x02) > 0) {
    color = CHSV(arrow_hue[1], 255, modes[currentMode].Brightness);
    for (int8_t y = 0; y <= 4; y++) {
      for (int8_t x = 0; x <= y; x++) {
        if (arrow_y[1] - y >= 0 && arrow_y[1] - y <= stop_y[1]) {
          CHSV clr = (y < 4 || (y == 4 && x < 2)) ? color : CHSV(0, 0, 0);
          drawPixelXY(arrow_x[1] - x, arrow_y[1] - y, clr);
          drawPixelXY(arrow_x[1] + x, arrow_y[1] - y, clr);
        }
      }
    }
    arrow_y[1]++;
  }

  // движение стрелки - cправа налево
  if ((arrow_direction & 0x04) > 0) {
    color = CHSV(arrow_hue[2], 255, modes[currentMode].Brightness);
    for (int8_t x = 0; x <= 4; x++) {
      for (int8_t y = 0; y <= x; y++) {
        if (arrow_x[2] + x >= stop_x[2] && arrow_x[2] + x < (int8_t)WIDTH) {
          CHSV clr = (x < 4 || (x == 4 && y < 2)) ? color : CHSV(0, 0, 0);
          drawPixelXY(arrow_x[2] + x, arrow_y[2] - y, clr);
          drawPixelXY(arrow_x[2] + x, arrow_y[2] + y, clr);
        }
      }
    }
    arrow_x[2]--;
  }

  // движение стрелки - cверху вниз
  if ((arrow_direction & 0x08) > 0) {
    color = CHSV(arrow_hue[3], 255, modes[currentMode].Brightness);
    for (int8_t y = 0; y <= 4; y++) {
      for (int8_t x = 0; x <= y; x++) {
        if (arrow_y[3] + y >= stop_y[3] && arrow_y[3] + y < (int8_t)HEIGHT) {
          CHSV clr = (y < 4 || (y == 4 && x < 2)) ? color : CHSV(0, 0, 0);
          drawPixelXY(arrow_x[3] - x, arrow_y[3] + y, clr);
          drawPixelXY(arrow_x[3] + x, arrow_y[3] + y, clr);
        }
      }
    }
    arrow_y[3]--;
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
    case 1:
      arrow_complete = arrow_x[0] > stop_x[0];
      break;
    case 2:
      arrow_complete = arrow_y[1] > stop_y[1];
      break;
    case 4:
      arrow_complete = arrow_x[2] < stop_x[2];
      break;
    case 8:
      arrow_complete = arrow_y[3] < stop_y[3];
      break;
    }

    arrow_change_mode = false;
    if (arrow_complete) {
      arrow_direction = (arrow_direction << 1) & 0x0F;
      if (arrow_direction == 0)
        arrow_direction = 1;
      if (arrow_mode_orig == 0) {
        arrow_play_mode_count[1]--;
        if (arrow_play_mode_count[1] == 0) {
          arrow_play_mode_count[1] = arrow_play_mode_count_orig[1];
          arrow_mode = random8(1, 5);
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
    case 5:
      arrow_complete = arrow_x[0] > stop_x[0];
      break; // Стрелка слева и справа встречаются в центре одновременно -
             // проверять только стрелку слева
    case 10:
      arrow_complete = arrow_y[1] > stop_y[1];
      break; // Стрелка снизу и сверху встречаются в центре одновременно -
             // проверять только стрелку снизу
    }

    arrow_change_mode = false;
    if (arrow_complete) {
      arrow_direction = arrow_direction == 5 ? 10 : 5;
      if (arrow_mode_orig == 0) {
        arrow_play_mode_count[2]--;
        if (arrow_play_mode_count[2] == 0) {
          arrow_play_mode_count[2] = arrow_play_mode_count_orig[2];
          arrow_mode = random8(1, 5);
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
    if (WIDTH >= HEIGHT)
      arrow_complete = arrow_x[0] > stop_x[0];
    else
      arrow_complete = arrow_y[1] > stop_y[1];

    arrow_change_mode = false;
    if (arrow_complete) {
      if (arrow_mode_orig == 0) {
        arrow_play_mode_count[3]--;
        if (arrow_play_mode_count[3] == 0) {
          arrow_play_mode_count[3] = arrow_play_mode_count_orig[3];
          arrow_mode = random8(1, 5);
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
    switch (arrow_direction) {
    case 5:
      arrow_complete = arrow_x[0] > stop_x[0];
      break; // Стрелка слева и справа движутся и достигают края одновременно -
             // проверять только стрелку слева
    case 10:
      arrow_complete = arrow_y[1] > stop_y[1];
      break; // Стрелка снизу и сверху движутся и достигают края одновременно -
             // проверять только стрелку снизу
    }

    arrow_change_mode = false;
    if (arrow_complete) {
      arrow_direction = arrow_direction == 5 ? 10 : 5;
      if (arrow_mode_orig == 0) {
        arrow_play_mode_count[4]--;
        if (arrow_play_mode_count[4] == 0) {
          arrow_play_mode_count[4] = arrow_play_mode_count_orig[4];
          arrow_mode = random8(1, 5);
          arrow_change_mode = true;
        }
      }

      arrowSetupForMode(arrow_mode, arrow_change_mode);
    }
    break;

  case 5:
    // Одновременно со всех сторон от края до края со смещением горизонтальной
    // оси на 1/3 высоты, далее Проверять стрелки по самой длинной стороне
    if (WIDTH >= HEIGHT)
      arrow_complete = arrow_x[0] > stop_x[0];
    else
      arrow_complete = arrow_y[1] > stop_y[1];

    arrow_change_mode = false;
    if (arrow_complete) {
      if (arrow_mode_orig == 0) {
        arrow_play_mode_count[5]--;
        if (arrow_play_mode_count[5] == 0) {
          arrow_play_mode_count[5] = arrow_play_mode_count_orig[5];
          arrow_mode = random8(1, 5);
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
  const byte PADDING      = HEIGHT * 0.25;
  const float BR_INTERWAL = WIDTH / HEIGHT;

  // ---------------------
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      setModeSettings(50, random8(2, 254U));
    }
    #endif
    loadingFlag = false;
    deltaValue = 0;
    hue = 0;

    ledsClear(); // esphome: FastLED.clear();
  }
  // ---------------------

  byte step1 = map8(modes[currentMode].Speed, 10U, 60U);
  uint16_t ms = millis();
  double freq = 3000;
  float mn = 255.0 / 13.8;
  const byte fade = 30; // 60 - abs(128 - step) / 3;
  fadeToBlackBy(leds, NUM_LEDS, fade);

  // -----------------
  for (uint16_t y = 0; y < HEIGHT; y++) {
    uint32_t yy = y * 256;
    uint32_t x1 = beatsin16(step1, WIDTH, (HEIGHT - 1) * 256, WIDTH, y * freq + 32768) / 1.5;

    /* change color -------- */
    byte cur_color = ms / 29 + y * 256 / HEIGHT;
    CRGB color = CHSV(cur_color, 255, 255 - y * HEIGHT / 8);
    byte br = constrain(255 - y * HEIGHT / 5, 0, 200);
    CRGB color2 = CHSV(cur_color - 32, 255 - y * HEIGHT / 4, br);

    wu_pixel( x1 + hue + PADDING * hue / 2, yy, &color);
    wu_pixel( abs((int)((WIDTH - 1) * 256 - (x1 + hue))), yy - PADDING * hue, &color2);
  }

  step++;
  if (step % 64) {
    if (deltaValue == 1) {
      hue++;
      if (hue >= 255) {
        deltaValue = 0;
      }
    } else {
      hue--;
      if (hue < 1) {
        deltaValue = 1;
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
static void drawLotusFlowerFragment(uint8_t posX, byte line) {
  const uint8_t h = (HEIGHT > 24) ? HEIGHT * 0.9 : HEIGHT;
  uint8_t flover_color = 128 + abs(128 - hue);                        // 128 -- 255
  uint8_t gleam = 255 - abs(128 - hue2);                              // 255 -- 128
  float f_size = (128 - abs(128 - deltaValue)) / 150.0;               // 1.0 -- 0.0
  const byte lowBri = 112U;

  // clear -----
  DrawLine(posX, 0, posX, h * 1.1, CRGB::Black);

  switch (line) {
    case 0:
      gradientVertical(posX, 0, posX + 1, h * 0.22, 96, 96, 32, 255, 255U);                             // green leaf c
      gradientVertical(posX, h * 0.9, posX + 1, h * 1.1, 64, 48, 64, 205, gleam);                       // pestle
      gradientVertical(posX, 8, posX + 1, h * 0.6, flover_color, flover_color, 128, lowBri, 255U);          // ---
      break;
    case 2:
    case 6:
      gradientVertical(posX, h * 0.2, posX + 1, h - 4, flover_color, flover_color, lowBri, 255, gleam);     //  -->
      gradientVertical(posX, h * 0.05, posX + 1, h * 0.15, 96, 96, 32, 255, 255U);                      // green leaf
      break;
    case 3:
    case 5:
      gradientVertical(posX, h * 0.5, posX + 1, h - 2, flover_color, flover_color, lowBri, 255, 255U);      // ---->
      break;
    case 4:
      gradientVertical(posX, 1 + h * f_size, posX + 1, h, flover_color, flover_color, lowBri, 255, gleam);  // ------>
      break;
    default:
      gradientVertical(posX, h * 0.05, posX + 1, h * 0.2, 80, 96, 160, 64, 255U);                       // green leaf m
      break;
  }
}

//---------------------------------------
static void LotusFlower() {
  const byte STEP_OBJ = 8;
  static uint8_t deltaSpeed = 0;

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), random8(1, 255U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    step = 0U;
    hue2 = 128U;
    deltaValue = 0;
    hue = 224;
    FPSdelay = SpeedFactor(160);

    ledsClear(); // esphome: FastLED.clear();
  }

  if (modes[currentMode].Speed > 128U) {
    if (modes[currentMode].Scale > 50) {
      deltaSpeed = 80U + (128U - abs((int)(128U - deltaValue))) / 1.25;
      FPSdelay = SpeedFactor(deltaSpeed);
      if (step % 256 == 0U ) hue += 32;           /* color morph */
    } else {
      FPSdelay = SpeedFactor(160);
      hue = 28U;
    }
    deltaValue++;     /* size morph  */
    /* <==== scroll ===== */
    drawLotusFlowerFragment(WIDTH - 1, (step % STEP_OBJ));
    for (uint8_t y = 0U ; y < HEIGHT; y++) {
      for (uint8_t x = 0U ; x < WIDTH; x++) {
        drawPixelXY(x - 1, y,  getPixColorXY(x,  y));
      }
    }
  } else {
    /* <==== morph ===== */
    for (uint8_t x = 0U ; x < WIDTH; x++) {
      drawLotusFlowerFragment(x, (x % STEP_OBJ));
      if (x % 2U) {
        hue2++;         /* gleam morph */
      }
    }
    deltaValue++;       /* size morph  */
    if (modes[currentMode].Scale > 50) {
      hue += 8; /* color morph */
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
  uint8_t const gamma[6] = {0, 96, 128, 160, 240, 112};
  const uint8_t PADDING = round(HEIGHT / 8);
  byte br;

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      setModeSettings(random8(100), random8(2, 254U));
    }
    #endif

    loadingFlag = false;
    deltaValue = modes[currentMode].Scale / 20;
    emitterY = 0;
    step = 0;

    ledsClear(); // esphome: FastLED.clear();
  }

  float radius = abs(128 - step) / 127.0 * (float)CENTER_Y_MINOR;
  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      if (x % 2 == 0) {
        br = constrain(255 / (emitterY + 1) * y, 48, 255);

        if ((x % 4) == 0) {
          hue = gamma[deltaValue];
          if (y == ceil(emitterY - radius) + random8(1, 4)) {
            if (step % 2 == 0) {
              drawPixelXYF(x, y + 0.5, CHSV(hue, 200, 255));
            } else {
              drawPixelXY(x, y, CHSV(hue, 200, 255));
            }
          } else {
            drawPixelXY(x, y, CHSV(hue, 255, (y > ceil(emitterY - radius / 2)) ? 0 : br));
          }
        } else {
          hue = gamma[deltaValue + 1];
          if (y == (ceil(emitterY * 0.70 + radius) + random8(3))) {
            drawPixelXYF(x, y - 0.5, CHSV(hue - ceil(radius), 160, 255));
          } else {
            uint8_t delta = ceil(emitterY * 0.70 + radius);
            drawPixelXY(x, y, CHSV(hue - ceil(radius), 255, (y > delta) ? 0 : br));
          }
        }
      } else {
        // clear blur ----
        if (pcnt > PADDING + 2) {
          drawPixelXY(x, y, CRGB::Black);
        }
      }
    }
  }

  if ((emitterY <= PADDING * 2) | (emitterY > HEIGHT - PADDING - 1)) {
    blurScreen(32);
  }

  if (emitterY > pcnt) {
    emitterY -= 0.5;
    if (abs(pcnt - emitterY ) < PADDING) {
      if (emitterY > pcnt) {
        emitterY -= 0.5;
      }
    }
  } else {
    if (emitterY < pcnt) {
      emitterY += 3;
    } else {
      pcnt = random8(2, HEIGHT - PADDING - 1);
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
  const byte PADDING = HEIGHT * 0.13;
  // ---------------------

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      setModeSettings(50, random8(2, 254U));
    }
    #endif

    loadingFlag = false;
    hue = 64;
    for (uint16_t i = 0; i < WIDTH; i++) {
      noise3d[0][i][0] = PADDING + 2;
      noise3d[0][i][1] = PADDING + 3;
    }
    ledsClear(); // esphome: FastLED.clear();
  }
  // ---------------------

  byte xx = random8(WIDTH);
  byte yy = random8(HEIGHT);
  byte fade = 80; //60 - abs(128 - step) / 3;
  fadeToBlackBy(leds, NUM_LEDS, fade);

  // -----------------
  for (uint16_t y = 0; y < HEIGHT; y++) {
    for (uint16_t x = 0; x < WIDTH; x++) {
      if (y > PADDING) {
        if (x % 6U == 0U) {
          /* draw Elevator */
          leds[XY(x, noise3d[0][x][1])] = CHSV(160, 255U, 255U);
        } else {
          /* draw light ------- */
          // if ((x % 2U == 0U) & (y % 2U == 0U)) {
          bool flag = (modes[currentMode].Scale > 50U) ? true : x % 2U == 0U;
          if (flag & (y % 2U == 0U)) {
            if ((x == xx) & (y == yy)) {
              /* change light */
              if (noise3d[0][x][y] == 0) {
                noise3d[0][x][y] = random8(1, 5);
                if (modes[currentMode].Speed > 80) {
                  noise3d[0][random8(WIDTH)][random8(PADDING + 1, HEIGHT - 1)] = 6;
                }
                if (modes[currentMode].Speed > 160) {
                  noise3d[0][random8(WIDTH)][random8(PADDING + 1, HEIGHT - 1)] = 6;
                }

              } else {
                noise3d[0][x][y] = 0;
              }
            }
            if (modes[currentMode].Speed > 250) {
              noise3d[0][x][y] = 2;
            }
            /* draw light ----- */
            if (noise3d[0][x][y] > 0) {
              if (noise3d[0][x][y] == 1U) {
                leds[XY(x, y)] = CHSV(32U, 200U, 255U);
              } else {
                leds[XY(x, y)] =  CHSV(128U, 32U, 255U);
              }
            }
          }
        }
      } else {
        /* draw the lower floors */
        if (y == PADDING) {
          leds[XY(x, y)] = CHSV(hue, 255U, 255U);
        } else {
          leds[XY(x, y)] = CHSV(96U, 128U, 80U + y * 32);
        }
      }
    }
  }

  /* change elevators position */
  if (step % 4U == 0U) {
    for (uint16_t i = 0; i < WIDTH; i++) {
      if (i % 6U == 0U) {
        /* 1 current floor */
        if (noise3d[0][i][0] > noise3d[0][i][1]) noise3d[0][i][1]++;
        if (noise3d[0][i][0] < noise3d[0][i][1]) noise3d[0][i][1]--;
      }
    }
  }

  /* 0 set target floor ----- */
  if (step % 128U == 0U ) {
    for (uint16_t i = 0; i < WIDTH; i++) {
      if (i % 6U == 0U) {
        /* 0 target floor ----- */
        byte target_floor = random8(PADDING + 1, HEIGHT - 1);
        if (target_floor % 2U) target_floor++;
        noise3d[0][i][0] = target_floor;
      }
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
      setModeSettings(random8(10U) ? 2U + random8(99U) : 1U , 185U + random8(52U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    loadingFlag = false;
    ledsClear(); // esphome: FastLED.clear();
  }

  for (uint8_t x = 0U; x < WIDTH; x++)
  {
    // заполняем случайно верхнюю строку
    if (getPixColorXY(x, HEIGHT - 1U) == 0U)
    {
      if (random8(0, 50) == 0U)
      {
        if (modes[currentMode].Scale == 1)
        {
          drawPixelXY(x, HEIGHT - 1U, CHSV(random(0, 9) * 28, 255U, 255U));                               // Радужный дождь
        }
        else
        {
          if (modes[currentMode].Scale == 100)
          {
            drawPixelXY(x, HEIGHT - 1U, 0xE0FFFF - 0x101010 * random(0, 4));                              // Снег
          }
          else
          {
            drawPixelXY(x, HEIGHT - 1U, CHSV(modes[currentMode].Scale * 2.4 + random(0, 16), 255, 255));  // Цветной дождь
          }
        }
      }
    }
    else
    {
      leds[XY(x, HEIGHT - 1U)] -= CHSV(0, 0, random(96, 128));
    }
  }

  // сдвигаем всё вниз
  for (uint8_t x = 0U; x < WIDTH; x++)
  {
    for (uint8_t y = 0U; y < HEIGHT - 1U; y++)
    {
      drawPixelXY(x, y, getPixColorXY(x, y + 1U));
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
  static byte i;
  static bool v_scanner = HEIGHT >= WIDTH;
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      setModeSettings(random8(0, 100), random8(128, 255U));
    }
    deltaValue = 0;
    #endif

    loadingFlag = false;
    hue = modes[currentMode].Scale * 2.55;
    deltaHue = modes[currentMode].Scale;
    i = 5;
    ledsClear(); // esphome: FastLED.clear();
  }

  if (step % 2U == 0U) {
    if (deltaValue == 0U) {
      i++;
    } else {
      i--;
    }
    if (deltaHue == 0U) {
      hue++;
    }
  }
  if (i > 250) {
    i = 0;
    deltaValue = 0;
  }
  fadeToBlackBy(leds, NUM_LEDS, v_scanner ? 50 : 30);

  if (v_scanner) {
    /* vertical scanner */
    if (i >= HEIGHT - 1) {
      deltaValue = 1;
    }

    for (uint16_t x = 0; x < WIDTH; x++) {
      leds[XY(x, i)] = CHSV(hue, 255U, 180U);
      if ((x == i / 2.0) & (i % 2U == 0U)) {
        if (deltaValue == 0U) {
          drawPixelXYF(random(WIDTH) - (random8(2U) ? 1.5 : 1), i * 0.9, CHSV(hue, 16U, 255U) );
        } else {
          drawPixelXYF(random(WIDTH) - 1.5, i * 1.1, CHSV(hue, 16U, 255U) );
        }
      }
    }
  } else {
    /* horizontal scanner */
    if (i >= WIDTH - 1) {
      deltaValue = 1;
    }

    for (uint16_t y = 0; y < HEIGHT; y++) {
      leds[XY(i, y)] = CHSV(hue, 255U, 180U);
      if ((y == i / 2.0) & (i % 2U == 0U)) {
        if (deltaValue == 0U) {
          drawPixelXYF(i * 0.9, random(HEIGHT) - (random8(2U) ? 1.5 : 1), CHSV(hue, 16U, 255U) );
        } else {
          drawPixelXYF( i * 1.1, random(HEIGHT) - 1.5, CHSV(hue, 16U, 255U) );
        }
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
static byte buff[WIDTH + 2][HEIGHT + 2];
// -------------------------------------
static void blur() {
  uint16_t sum;
  for (byte x = 1; x < WIDTH + 1; x++) {
    for (byte y = 1; y < HEIGHT + 1; y++) {
      sum = buff[x][y];
      sum += buff[x + 1][y];
      sum += buff[x][y - 1];
      sum += buff[x][y + 1];
      sum += buff[x - 1][y];
      sum /= 5;
      buff[x][y] = sum;
    }
  }
}

// -------------------------------------
static void drawDot(float x, float y, byte a) {
  uint8_t xx = (x - (int) x) * 255, yy = (y - (int) y) * 255, ix = 255 - xx, iy = 255 - yy;
  uint8_t wu[4] = {
    WU_WEIGHT(ix, iy),
    WU_WEIGHT(xx, iy),
    WU_WEIGHT(ix, yy),
    WU_WEIGHT(xx, yy)
  };

  // multiply the intensities by the colour, and saturating-add them to the pixels
  for (uint8_t i = 0; i < 4; i++) {
    int16_t xn = x + (i & 1), yn = y + ((i >> 1) & 1);
    byte clr = buff[xn][yn];
    clr = constrain(qadd8(clr, (a * wu[i]) >> 8), 0 , 255);
    buff[xn][yn] = clr;
  }
}

// -------------------------------------
static void Mirage() {
  const uint8_t divider = 4;
  const uint8_t val = 255;
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), random8(80U, 255U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    hue = 70;
  }

  blur();
  float x1 = (float)beatsin88(15 * modes[currentMode].Speed, divider, WIDTH * divider) / divider;
  float y1 = (float)beatsin88(20 * modes[currentMode].Speed, divider, HEIGHT * divider) / divider;
  float x2 = (float)beatsin88(16 * modes[currentMode].Speed, divider, (WIDTH - 1) * divider) / divider;
  float y2 = (float)beatsin88(14 * modes[currentMode].Speed, divider, HEIGHT * divider) / divider;
  float x3 = (float)beatsin88(12 * modes[currentMode].Speed, divider, (WIDTH - 1) * divider) / divider;
  float y3 = (float)beatsin88(16 * modes[currentMode].Speed, divider, HEIGHT * divider) / divider;

  drawDot(x1 , y1, val);
  drawDot(x1 + 1, y1, val);
  drawDot(x2 , y2, val);
  drawDot(x2 + 1, y2, val);
  drawDot(x3 , y3, val);
  drawDot(x3 + 1, y3, val);

  hue++;
  for (byte x = 1; x < WIDTH + 1; x++) {
    for (byte y = 1; y < HEIGHT + 1; y++) {
      leds[XY(x - 1, y - 1)] = CHSV(hue , buff[x][y], 255);
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
  const uint8_t V_STEP = 255 / (HEIGHT + 9);
  static uint8_t val_scale;

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), random8(210, 255U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    hue = modes[currentMode].Scale * 2.55;
    val_scale = map(modes[currentMode].Speed, 1, 255, 200U, 255U);;
  }

  for (int index = 0; index < NUM_LEDS; index++) {
    leds[index].nscale8(val_scale);
  }

  for (int i = 0; i < HEIGHT; i++) {
    int tmp = sin8(i + (millis() >> 4));
    tmp = map8(tmp, 2, WIDTH - 2);

    leds[XY(WIDTH - tmp, i)]     = CHSV(hue, V_STEP * i + 32, 205U);
    leds[XY(WIDTH - tmp - 1, i)] = CHSV(hue, 255U, 255 - V_STEP * i);
    leds[XY(WIDTH - tmp + 1, i)] = CHSV(hue, 255U, 255 - V_STEP * i);

    if ((i % 6 == 0) & (modes[currentMode].Scale > 95U)) {
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
  static int64_t frameCount =  0;
  const byte END = WIDTH - 1;
  static byte dX;
  static bool direct;
  static byte divider;
  static byte deltaValue = 0;

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), random8(40, 160U));
    }
    #endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    loadingFlag = false;

    divider = floor(modes[currentMode].Scale / 25);
    direct = true;
    dX = 1;
    pcnt = 0;
    frameCount = 0;
    hue2 == 32;
    clearNoiseArr();
    ledsClear(); // esphome: FastLED.clear();
  }

  // EVERY_N_MILLISECONDS(1000 / 30) {
  frameCount++;
  pcnt++;
  // }

  uint8_t t1 = cos8((42 * frameCount) / 30);
  uint8_t t2 = cos8((35 * frameCount) / 30);
  uint8_t t3 = cos8((38 * frameCount) / 30);
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;

  if (direct) {
    if (dX < END) {
      dX++;
    }
  } else {
    if (dX > 0) {
      dX--;
    }
  }
  if (pcnt > 128) {
    pcnt = 0;
    direct = !direct;
    if (divider > 2) {
      if (dX == 0) {
        deltaValue++;
        if (deltaValue > 2) {
          deltaValue = 0;
        }
      }
    } else {
      deltaValue = divider;
    }

  }

  for (uint16_t y = 0; y < HEIGHT; y++) {
    for (uint16_t x = 0; x < WIDTH; x++) {
      if (x != END - dX) {
        r = cos8((y << 3) + (t1 >> 1) + cos8(t2 + (x << 3)));
        g = cos8((y << 3) + t1 + cos8((t3 >> 2) + (x << 3)));
        b = cos8((y << 3) + t2 + cos8(t1 + x + (g >> 2)));

      } else {
        // line gold -------
        r = 255U;
        g = 255U;
        b = 255U;
      }

      uint8_t val = dX * 8;
      switch (deltaValue) {
        case 0:
          if (r > val) {
            r = r - val;
          } else {
            r = 0;
          }
          if (g > val) {
            g = g - val / 2;
          } else {
            g = 0;
          }
          break;
        case 1:
          if (g > val) {
            g = g - val;
          } else {
            g = 0;
          }
          if (b > val) {
            b = b - val / 2;
          } else {
            b = 0;
          }
          break;
        case 2:
          if (b > val) {
            b = b - val;
          } else {
            b = 0;
          }
          if (r > val) {
            r = r - val / 2;
          } else {
            r = 0;
          }
          break;
      }

      r = exp_gamma[r];
      g = exp_gamma[g];
      b = exp_gamma[b];

      leds[XY(x, y)] = CRGB(r, g, b);
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
  const uint8_t STEP = 255 / CENTER_X_MINOR;
  float distance;

  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      setModeSettings(random8(100), random8(2, 254U));
    }
    #endif

    loadingFlag = false;
    deltaValue = modes[currentMode].Scale;
    hue = 96;
    emitterY = 0;

    ledsClear(); // esphome: FastLED.clear();
  }

  // Calculate the radius based on the sound value --
  float radius = abs(128 - step) / 127.0 * max(CENTER_X_MINOR, CENTER_Y_MINOR);

  // Loop through all matrix points -----------------
  for (uint8_t x = 0; x < WIDTH; x++) {
    for (uint8_t y = 0; y < HEIGHT; y++) {
      // Calculate the distance from the center to the current point
      distance = sqrt(pow(x - CENTER_X_MINOR - 1, 2) + pow(y - CENTER_Y_MINOR - emitterY, 2));
      hue = step + distance * radius;

      // Check if the point is inside the radius ----
      deltaHue = 200 - STEP * distance * 0.25;

      if (distance < radius) {
        if (modes[currentMode].Scale > 50) {
          if (x % 2 & y % 2) {
            drawPixelXYF(x, y - CENTER_Y_MINOR / 2 + emitterY, CHSV(hue, 255, 64));
          } else {
            leds[XY(x, y)] = CHSV(hue + 32, 255 - distance, deltaHue);
          }
        } else {
          leds[XY(x, y)] = CHSV(hue, 255 - distance, 255);
        }

      } else {
        if (modes[currentMode].Scale > 75) {
          leds[XY(x, y)] = CHSV(hue + 96, 255, deltaHue);
        } else {
          leds[XY(x, y)] = CHSV(hue, 255, deltaHue);
        }
      }
    }
  }

  if (modes[currentMode].Scale > 50) {
    if (emitterY > pcnt) {
      emitterY -= 0.25;
    } else {
      if (emitterY < pcnt) {
        emitterY += 0.25;
      } else {
        pcnt = random8(CENTER_Y_MINOR);
      }
    }
  } else {
    emitterY = 0;
  }

  blurScreen(48);
  step++;
}
#endif

#ifdef DEF_RAINBOW_RINGS
// =========== Rainbow Rings ===========
//    base code © Martin Kleppe @aemkei
//             © SlingMaster
//            Радужные кольца
// =====================================
static float codeEff(double t, double i, double x, double y) {
  hue = 255U; hue2 = 0U; // | CENTER_X_MAJOR
  return sin16((t - sqrt3((x - CENTER_X_MAJOR) * (x - CENTER_X_MAJOR) + (y - CENTER_Y_MAJOR) * (y - CENTER_Y_MAJOR))) * 8192.0) / 32767.0;
}

static void drawFrame(double t, double x, double y) {
  static uint32_t t_count;
  static byte scaleXY = 8;
  double i = (y * WIDTH) + x;
  double frame = constrain(codeEff(t, i, x, y), -1, 1) * 255;
  uint16_t tt = floor(i);
  byte xx;
  byte yy;
  byte angle;
  byte radius;
  if (frame > 0) {
    // white or black color
    if (modes[currentMode].Scale > 70) {
      if (modes[currentMode].Scale > 90) {
        drawPixelXY(x, y, CRGB(frame / 4, frame / 2, frame / 2));
      } else {
        drawPixelXY(x, y, CRGB(frame / 2, frame / 2, frame / 4));
      }
    } else {
      drawPixelXY(x, y, CRGB::Black);
    }
  } else {
    if (frame < 0) {
      switch (deltaHue2) {
        case 0:
          hue = step + y * x;
          break;
        case 1:
          hue = 64 + (y + x) * abs(128 - step) / CENTER_Y_MAJOR;
          break;
        case 2:
          hue = y * x + abs(y - CENTER_Y_MAJOR) * 4;
          break;
        case 3:
          xx = (byte)x;
          yy = (byte)y;
          angle = noise3d[0][xx][yy];
          radius = noise3d[1][xx][yy];
          if ((xx == 0) & (yy == 0))  t_count += 8;
          hue = (angle * scaleXY) + (radius * scaleXY) + t_count;
          break;
        default:
          hue = step + y * x;
          break;
      }
      drawPixelXY(x, y, CHSV( hue, frame * -1, frame * -1));
    } else {
      drawPixelXY(x, y, CRGB::Black);
    }
  }
}

static void RainbowRings() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), random8(255U));
    }
    #endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;
    deltaHue = 0;
    FPSdelay = 1;
    deltaHue2 = modes[currentMode].Scale / 22;
    hue = 255U; hue2 = 0U;
    for (int8_t x = -CENTER_X_MAJOR; x < CENTER_X_MAJOR; x++) {
      for (int8_t y = CENTER_X_MAJOR; y < HEIGHT; y++) {
        noise3d[0][x + CENTER_X_MAJOR][y] = 128 * (atan2(y, x) / PI);
        noise3d[1][x + CENTER_X_MAJOR][y] = hypot(x, y);                    // thanks Sutaburosu
      }
    }
  }

  unsigned long milli = millis();
  double t = milli / 1000.0;
  for ( double x = 0; x < WIDTH; x++) {
    for (double y = 0; y < HEIGHT; y++) {
      drawFrame(t, x, y);
    }
  }
  step++;
}
#endif

#ifdef DEF_VYSHYVANKA
// ============ Vyshyvanka =============
//       (с) проект Aurora "Munch"
//     adopted/updated by kostyamat
//          Эффект "Вышиванка" 
// =====================================

static int8_t count = 0;
static int8_t dir = 0;
static uint8_t flip = 0;
static uint8_t generation = 0;
static uint8_t rnd = 4; // 1-8
static uint8_t mic[2];
static uint8_t minDimLocal = max(WIDTH, HEIGHT) > 32 ? 32 : 16;

const uint8_t width_adj = (WIDTH < HEIGHT ? (HEIGHT - WIDTH) / 2 : 0);
const uint8_t height_adj = (HEIGHT < WIDTH ? (WIDTH - HEIGHT) / 2 : 0);
const uint8_t maxDim_steps = 256 / max(WIDTH, HEIGHT);

static void munchRoutine() {
  if (loadingFlag) {
    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    if (selectedSettings) {
      //                     scale | speed
      setModeSettings(random8(100U), random8(255U));
    }
    #endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    loadingFlag = false;

    generation = 0;
    dir = 1;
    count = 0;
    flip = 0;

    ledsClear(); // esphome: FastLED.clear();
  }

  for (uint8_t x = 0; x < minDimLocal; x++) {
    for (uint8_t y = 0; y < minDimLocal; y++) {
      CRGB color = (x ^ y ^ flip) < count ? ColorFromPalette(RainbowColors_p, ((x ^ y) << rnd) + generation, modes[currentMode].Brightness) : CRGB::Black;
      if (x < WIDTH and y < HEIGHT) leds[XY(x, y)] = color;
      if (x + minDimLocal < WIDTH and y < HEIGHT) leds[XY(x + minDimLocal, y)] = color;
      if (y + minDimLocal < HEIGHT and x < WIDTH) leds[XY(x, y + minDimLocal)] = color;
      if (x + minDimLocal < WIDTH and y + minDimLocal < HEIGHT) leds[XY(x + minDimLocal, y + minDimLocal)] = color;
    }
  }

  count += dir;

  if (count <= 0 || count >= mic[0]) {
    dir = -dir;
    if (count <= 0) {
      mic[0] = mic[1];
      if (flip == 0)
        flip = mic[1] - 1;
      else
        flip = 0;
    }
  }
  
  generation++;
  mic[1] = minDimLocal;
}
#endif

}  // namespace matrix_lamp
}  // namespace esphome
