#pragma once

#include "common.h"

#include "esphome/core/defines.h"
#include "esphome/components/fastled_helper/utils.h"

#define WU_WEIGHT(a, b) ((uint8_t)(((a) * (b) + (a) + (b)) >> 8))

namespace esphome::matrix_lamp {

using namespace fastled_helper;

// ------------------------------------------------
// получить номер пикселя в ленте по координатам
// библиотека FastLED тоже использует эту функцию
static inline uint16_t XY(uint8_t x, uint8_t y) {
  uint8_t THIS_X = x;
  uint8_t THIS_Y = y;
  uint8_t _WIDTH = WIDTH;

  switch (ORIENTATION) {
    case 1: THIS_X = y; THIS_Y = x; _WIDTH = HEIGHT; break;
    case 2: THIS_Y = (HEIGHT - 1U - y); break;
    case 3: THIS_X = (HEIGHT - 1U - y); THIS_Y = x; _WIDTH = HEIGHT; break;
    case 4: THIS_X = (WIDTH - 1U - x); THIS_Y = (HEIGHT - 1U - y); break;
    case 5: THIS_X = (HEIGHT - 1U - y); THIS_Y = (WIDTH - 1U - x); _WIDTH = HEIGHT; break;
    case 6: THIS_X = (WIDTH - 1U - x); break;
    case 7: THIS_X = y; THIS_Y = (WIDTH - 1U - x); _WIDTH = HEIGHT; break;
    default: break; // case 0
  }

  // Расчет индекса (MATRIX_TYPE: 0 - зигзаг, 1 - параллельная)
  if (MATRIX_TYPE || !(THIS_Y & 0x01)) {
    return (uint16_t)THIS_Y * _WIDTH + THIS_X;
  } else {
    return (uint16_t)THIS_Y * _WIDTH + (_WIDTH - 1U - THIS_X);
  }
}


// ------------------------------------------------
// функция отрисовки точки по координатам X Y
#if (WIDTH > 127) || (HEIGHT > 127)
using coord_t = int16_t;
#else
using coord_t = int8_t;
#endif
static void drawPixelXY(coord_t x, coord_t y, CRGB color)
{
  if (x < 0 || x >= (coord_t)WIDTH || y < 0 || y >= (coord_t)HEIGHT) return;
  leds[XY(x, y)] = color;
}


// ------------------------------------------------
// функция получения цвета пикселя по его номеру
static uint32_t getPixColor(uint16_t thisPixel)
{
  if (thisPixel >= NUM_LEDS) return 0U;

  // return (((uint32_t)leds[thisPixel].r << 16) | ((uint32_t)leds[thisPixel].g << 8 ) | (uint32_t)leds[thisPixel].b);
  return (uint32_t)leds[thisPixel];
}


// ------------------------------------------------
// функция получения цвета пикселя в матрице по его координатам
static uint32_t getPixColorXY(coord_t x, coord_t y)
{
  if (x < 0 || x >= (coord_t)WIDTH || y < 0 || y >= (coord_t)HEIGHT) return 0U;
  return getPixColor(XY(x, y));
}


// ------------------------------------------------
static uint8_t SpeedFactor(uint8_t spd)
{
  // uint8_t result = spd * NUM_LEDS / 1024.0;
  // return result;
  return (uint32_t)spd * NUM_LEDS >> 10;
}


// ------------------------------------------------
// неточный, зато более быстрый квадратный корень
static float sqrt3(float x) {
  union { int i; float f; } u;
  u.f = x;
  u.i = 0x5f3759df - (u.i >> 1); // Магическое число для 1/sqrt(x)
  return x * u.f;                // Умножаем на x, чтобы получить sqrt(x)
}


// blur1d: one-dimensional blur filter. Spreads light to 2 line neighbors.
// blur2d: two-dimensional blur filter. Spreads light to 8 XY neighbors.
//
//           0 = no spread at all
//          64 = moderate spreading
//         172 = maximum smooth, even spreading
//
//         173..255 = wider spreading, but increasing flicker
//
//         Total light is NOT entirely conserved, so many repeated
//         calls to 'blur' will also result in the light fading,
//         eventually all the way to black; this is by design so that
//         it can be used to (slowly) clear the LEDs to black.
static void blur1d(uint16_t numLeds, fract8 blur_amount) {
    uint8_t keep = 255 - blur_amount;
    uint8_t seep = blur_amount >> 1;
    CRGB carryover = CRGB::Black;
    
    for(uint16_t i = 0; i < numLeds; ++i) {
        CRGB cur = leds[i];
        CRGB part = cur;
        part.nscale8(seep);
        cur.nscale8(keep);
        
        cur += carryover;
        if(i > 0) leds[i-1] += part; 
        
        leds[i] = cur;
        carryover = part;
    }
}

static void blurRows(uint8_t width, uint8_t height, fract8 blur_amount) {
    // blur rows same as columns, for irregular matrix
    uint8_t keep = 255 - blur_amount;
    uint8_t seep = blur_amount >> 1;

    for (uint8_t row = 0; row < height; row++) {
        CRGB carryover = CRGB::Black;
        for (uint8_t i = 0; i < width; i++) {
            uint16_t index = XY(i, row);

            CRGB cur = leds[index];
            CRGB part = cur;

            part.nscale8(seep);
            cur.nscale8(keep);
            cur += carryover;

            if (i > 0) {
                leds[XY(i - 1, row)] += part;
            }

            leds[index] = cur;
            carryover = part;
        }
    }
}

// blurColumns: perform a blur1d on each column of a rectangular matrix
static void blurColumns(uint8_t width, uint8_t height, fract8 blur_amount) {
    uint8_t keep = 255 - blur_amount;
    uint8_t seep = blur_amount >> 1;

    for (uint8_t col = 0; col < width; ++col) {
        CRGB carryover = CRGB::Black;
        for (uint8_t i = 0; i < height; ++i) {
            uint16_t index = XY(col, i);

            CRGB cur = leds[index];
            CRGB part = cur;

            part.nscale8(seep);
            cur.nscale8(keep);
            cur += carryover;

            if (i > 0) {
                leds[XY(col, i - 1U)] += part;
            }

            leds[index] = cur;
            carryover = part;
        }
    }
}

static void blur2d(uint8_t width, uint8_t height, fract8 blur_amount)
{
    blurRows(width, height, blur_amount);
    blurColumns(width, height, blur_amount);
}


// ------------------------------------------------
// залить все
static void fillAll(CRGB color)
{
  // for (uint16_t i = 0; i < NUM_LEDS; i++) {
  //   leds[i] = color;
  // }
  fill_solid(leds, NUM_LEDS, color);
}

static void ledsClear()
{
  fillAll(CRGB(0,0,0));
}

// стандартные функции библиотеки LEDraw от @Palpalych (для адаптаций его эффектов)
static void blurScreen(fract8 blur_amount)
{
  blur2d(WIDTH, HEIGHT, blur_amount);
}

static void dimAll(uint8_t value, CRGB *LEDarray = leds) {
  nscale8(LEDarray, NUM_LEDS, value);
  // fadeToBlackBy(LEDarray, NUM_LEDS, 255U - value); // эквивалент
}

// kostyamat добавил
// функция уменьшения яркости
static CRGB makeDarker(const CRGB& color, fract8 howMuchDarker)
{
  CRGB newcolor = color;
  // newcolor.nscale8(255 - howMuchDarker);
  newcolor.fadeToBlackBy(howMuchDarker); // эквивалент
  return newcolor;
}


// ------------------------------------------------
static uint8_t wrapX(int16_t x){
  return (x % (int16_t)WIDTH + (int16_t)WIDTH) % (int16_t)WIDTH;
}

static uint8_t wrapY(int16_t y){
  return (y % (int16_t)HEIGHT + (int16_t)HEIGHT) % (int16_t)HEIGHT;
}


// ------------------------------ Дополнительные функции рисования ----------------------
// по мотивам
// https://gist.github.com/sutaburosu/32a203c2efa2bb584f4b846a91066583
static void drawPixelXYF(float x, float y, CRGB color)
{
  // extract the fractional parts and derive their inverses
  // Получаем целую часть координат
  int16_t x_int = (int16_t)x;
  int16_t y_int = (int16_t)y;

  // Извлекаем дробную часть в диапазоне 0-255
  uint8_t xx = (x - x_int) * 255;
  uint8_t yy = (y - y_int) * 255;
  uint8_t ix = 255 - xx;
  uint8_t iy = 255 - yy;

  // calculate the intensities for each affected pixel
  // Веса для 4-х соседних пикселей
  uint8_t wu[4] = {
    WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy),
    WU_WEIGHT(ix, yy), WU_WEIGHT(xx, yy)
  };

  // multiply the intensities by the colour, and saturating-add them to the pixels
  for (uint8_t i = 0; i < 4; i++) {
    int16_t xn = x_int + (i & 1);
    int16_t yn = y_int + ((i >> 1) & 1);

    CRGB clr = getPixColorXY(xn, yn);

    clr.r = qadd8(clr.r, (color.r * wu[i]) >> 8);
    clr.g = qadd8(clr.g, (color.g * wu[i]) >> 8);
    clr.b = qadd8(clr.b, (color.b * wu[i]) >> 8);

    drawPixelXY(xn, yn, clr);
  }
}


// ------------------------------------------------
static void DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, CRGB color) {
  int16_t deltaX = abs(x2 - x1);
  int16_t deltaY = abs(y2 - y1);
  int16_t signX = x1 < x2 ? 1 : -1;
  int16_t signY = y1 < y2 ? 1 : -1;
  int16_t error = deltaX - deltaY;

  for (;;) {
    drawPixelXY(x1, y1, color);
    
    if (x1 == x2 && y1 == y2) break;  // Линия закончена

    int16_t error2 = error << 1;      // То же самое, что error * 2

    if (error2 > -deltaY) {
      error -= deltaY;
      x1 += signX;
    }
    if (error2 < deltaX) {
      error += deltaX;
      y1 += signY;
    }
  }
}


// ------------------------------------------------
static void DrawLineF(float x1f, float y1f, float x2f, float y2f, CRGB color) {
    // 1. Быстрая проверка на одинаковые точки
    if (std::abs(x1f - x2f) < 0.1f && std::abs(y1f - y2f) < 0.1f) {
        drawPixelXY((int16_t)x1f, (int16_t)y1f, color);
        return;
    }

    // 2. Подготовка (Steep и Swap)
    bool steep = std::abs(y2f - y1f) > std::abs(x2f - x1f);
    if (steep) { std::swap(x1f, y1f); std::swap(x2f, y2f); }
    if (x1f > x2f) { std::swap(x1f, x2f); std::swap(y1f, y2f); }

    // 3. Переход в Fixed Point (16.16)
    // Умножаем на 65536 (сдвиг на 16) для высокой точности
    int32_t x1 = (int32_t)(x1f * 65536.0f);
    int32_t x2 = (int32_t)(x2f * 65536.0f);
    int32_t y1 = (int32_t)(y1f * 65536.0f);
    int32_t y2 = (int32_t)(y2f * 65536.0f);

    int32_t dx = x2 - x1;
    int32_t dy = y2 - y1;
    
    // Градиент в фиксированной точке
    // Используем 64-битное деление, чтобы избежать переполнения при расчете шага
    int32_t gradient = (int32_t)(((int64_t)dy << 16) / (dx >> 16));

    int32_t intery = y1;
    int16_t startX = x1 >> 16;
    int16_t endX = x2 >> 16;

    // 4. Основной "реактивный" цикл
    for (int16_t x = startX; x <= endX; x++) {
        // Получаем целую часть Y и дробную (вес)
        int16_t y_int = (int16_t)(intery >> 16);
        // Вес — это верхние 8 бит из нижней 16-битной части
        uint8_t weight2 = (uint8_t)((intery >> 8) & 0xFF);
        uint8_t weight1 = 255U - weight2;

        // Определяем координаты пикселей в зависимости от крутизны (steep)
        int16_t px = steep ? y_int : x;
        int16_t py = steep ? x : y_int;

        // Отрисовка двух соседних пикселей
        // Пиксель 1
        CRGB bg1 = getPixColorXY(px, py);
        drawPixelXY(px, py, CRGB(qadd8(bg1.r, (color.r * weight1) >> 8),
                                 qadd8(bg1.g, (color.g * weight1) >> 8),
                                 qadd8(bg1.b, (color.b * weight1) >> 8)));

        // Пиксель 2 (соседний по Y или X)
        int16_t px2 = steep ? px + 1 : px;
        int16_t py2 = steep ? py : py + 1;
        
        CRGB bg2 = getPixColorXY(px2, py2);
        drawPixelXY(px2, py2, CRGB(qadd8(bg2.r, (color.r * weight2) >> 8),
                                   qadd8(bg2.g, (color.g * weight2) >> 8),
                                   qadd8(bg2.b, (color.b * weight2) >> 8)));

        intery += gradient;
    }
}


// ------------------------------------------------
static void drawCircleF(float x0, float y0, float radius, CRGB color) {
  // Используем целые числа для алгоритма (скорость!)
  int16_t x = 0;
  int16_t y = static_cast<int16_t>(radius);
  int16_t delta = 1 - 2 * y;
  int16_t error = 0;

  // Центры в целых числах для удобства
  int16_t cx0 = static_cast<int16_t>(x0);
  int16_t cy0 = static_cast<int16_t>(y0);

  while (y >= 0) {
    // Используем wrapX для бесшовности по горизонтали 
    // и обычный y для защиты по вертикали внутри drawPixelXY
    drawPixelXY(wrapX(cx0 + x), cy0 + y, color);
    drawPixelXY(wrapX(cx0 + x), cy0 - y, color);
    drawPixelXY(wrapX(cx0 - x), cy0 + y, color);
    drawPixelXY(wrapX(cx0 - x), cy0 - y, color);

    // Добавляем еще 4 точки для плотности круга (8-октетный алгоритм)
    drawPixelXY(wrapX(cx0 + y), cy0 + x, color);
    drawPixelXY(wrapX(cx0 + y), cy0 - x, color);
    drawPixelXY(wrapX(cx0 - y), cy0 + x, color);
    drawPixelXY(wrapX(cx0 - y), cy0 - x, color);

    error = 2 * (delta + y) - 1;
    if (delta < 0 && error <= 0) {
      delta += 2 * (++x) + 1;
      continue;
    }
    error = 2 * (delta - x) - 1;
    if (delta > 0 && error > 0) {
      delta += 1 - 2 * (--y);
      continue;
    }
    delta += 2 * (++x - --y);
  }
}


// ------------------------------------------------
static void drawRec(uint8_t startX, uint8_t startY, uint8_t endX, uint8_t endY, uint32_t color) {
  for (uint8_t y = startY; y < endY; y++) {
    for (uint8_t x = startX; x < endX; x++) {
      drawPixelXY(x, y, color);
    }
  }
}

// ------------------------------------------------
static void drawRecCHSV(uint8_t startX, uint8_t startY, uint8_t endX, uint8_t endY, CHSV color) {
  for (uint8_t y = startY; y < endY; y++) {
    for (uint8_t x = startX; x < endX; x++) {
      drawPixelXY(x, y, color);
    }
  }
}

// ------------------------------------------------
static uint8_t validMinMax(float val, uint8_t minV, uint8_t maxV) {
  if (val <= (float)minV) return minV;
  if (val >= (float)maxV) return maxV;
  
  // ceil вручную, приведение к int и добавление 1, если была дробная часть
  uint8_t iVal = static_cast<uint8_t>(val);
  return (val > (float)iVal) ? (iVal + 1U) : iVal;
}


// ------------------------------------------------
// альтернативный градиент для ламп собраных из лент с вертикальной компоновкой
// gradientHorizontal | gradientVertical менее производительный но работает на всех видах ламп
static void gradientHorizontal(uint8_t startX, uint8_t startY, uint8_t endX, uint8_t endY, 
                               uint8_t start_color, uint8_t end_color, 
                               uint8_t start_br, uint8_t end_br, uint8_t saturate) {
  
  // Определяем фактические границы для циклов (всегда от меньшего к большему)
  uint8_t xMin = startX < endX ? startX : endX;
  uint8_t xMax = startX < endX ? endX : startX;
  uint8_t yMin = startY < endY ? startY : endY;
  uint8_t yMax = startY < endY ? endY : startY;

  // Количество шагов (знаковое!)
  int16_t dx = (int16_t)endX - (int16_t)startX;
  if (dx == 0) dx = 1; // Защита от деления на 0

  for (uint8_t x = xMin; x < xMax; x++) {
    // Вычисляем положение текущего X относительно НАЧАЛЬНОЙ точки startX
    int16_t offset = (int16_t)x - (int16_t)startX;
    
    // Формула сохраняет направление: если dx отрицательный, градиент пойдет вспять
    uint8_t this_hue = start_color + (int32_t)(end_color - start_color) * offset / dx;
    uint8_t this_br  = start_br + (int32_t)(end_br - start_br) * offset / dx;
    
    CHSV col = CHSV(this_hue, saturate, this_br);

    for (uint8_t y = yMin; y < yMax; y++) {
      drawPixelXY(x, y, col);
    }
  }
}


// ------------------------------------------------
static void gradientVertical(uint8_t startX, uint8_t startY, uint8_t endX, uint8_t endY, 
                             uint8_t start_color, uint8_t end_color, 
                             uint8_t start_br, uint8_t end_br, uint8_t saturate) {
  
  // Определяем физические границы для циклов
  uint8_t xMin = startX < endX ? startX : endX;
  uint8_t xMax = startX < endX ? endX : startX;
  uint8_t yMin = startY < endY ? startY : endY;
  uint8_t yMax = startY < endY ? endY : startY;

  // Дистанция по вертикали (знаковая, чтобы сохранить направление)
  int16_t dy = (int16_t)endY - (int16_t)startY;
  if (dy == 0) dy = 1; // Защита от деления на 0

  for (uint8_t y = yMin; y < yMax; y++) {
    // Вычисляем смещение текущего Y относительно начального startY
    int16_t offset = (int16_t)y - (int16_t)startY;

    // Целочисленный расчет цвета и яркости (сохраняет направление)
    uint8_t this_hue = start_color + (int32_t)(end_color - start_color) * offset / dy;
    uint8_t this_br  = start_br + (int32_t)(end_br - start_br) * offset / dy;
    
    CHSV col = CHSV(this_hue, saturate, this_br);

    // Заполняем горизонтальную линию этим цветом
    for (uint8_t x = xMin; x < xMax; x++) {
      drawPixelXY(x, y, col);
    }
  }
}


// ------------------------------------------------
// gradientDownTop • более плавный градиент в отличие от gradientVertical
static void gradientDownTop(uint8_t bottom, CHSV bottom_color, uint8_t top, CHSV top_color) {
  // 1. Рассчитываем реальные индексы строк (с учетом возможных смещений)
  uint8_t yStart = bottom;
  uint8_t yEnd = top;
  if (yStart > yEnd) std::swap(yStart, yEnd);
  if (yEnd > HEIGHT) yEnd = HEIGHT;

  // 2. Идем по строкам. Построчный fill_gradient — это идеальный баланс скорости и корректности.
  for (uint8_t y = yStart; y < yEnd; y++) {
    // Вычисляем цвет для текущей строки (интерполяция между bottom и top)
    // blend — очень быстрая функция FastLED
    CHSV rowColor = blend(bottom_color, top_color, map(y, yStart, yEnd - 1, 0, 255), fl::SHORTEST_HUES);

    // Получаем индекс первого и последнего пикселя в этой строке
    uint16_t firstPixel = XY(0, y);
    uint16_t lastPixel  = XY(WIDTH - 1, y);

    // 3. Используем fill_gradient для всей строки целиком.
    // FastLED сам поймет направление, если firstPixel > lastPixel (для зигзага)
    fill_gradient(leds, firstPixel, rowColor, lastPixel, rowColor, fl::SHORTEST_HUES);
  }
}


// ------------------------------------------------
// новый фейдер
static void fadePixel(uint8_t i, uint8_t j, uint8_t step) {
  uint16_t pixelNum = XY(i, j);
  CRGB& pixel = leds[pixelNum]; // Ссылка на пиксель, чтобы не копировать его

  // Если пиксель уже черный — выходим
  if (!pixel) return; 

  // Если хотя бы один канал выше порога, гасим плавно
  if (pixel.r > 30U || pixel.g > 30U || pixel.b > 30U) {
    pixel.fadeToBlackBy(step);
  } else {
    pixel = 0U; // Резко в ноль, чтобы избежать "грязных" хвостов
  }
}


// ------------------------------------------------
// Settings
static void restoreSettings()
{
  for (uint8_t i = 0; i < MODE_AMOUNT; i++)
  {
    modes[i].Brightness = pgm_read_byte(&defaultSettings[i][0]);
    modes[i].Speed      = pgm_read_byte(&defaultSettings[i][1]);
    modes[i].Scale      = pgm_read_byte(&defaultSettings[i][2]);
  }
}

// ------------------------------------------------
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
static void setModeSettings(uint8_t Scale = 0U, uint8_t Speed = 0U){
  selectedSettings = 0U;

  modes[currentMode].Scale = Scale ? Scale : pgm_read_byte(&defaultSettings[currentMode][2]);
  modes[currentMode].Speed = Speed ? Speed : pgm_read_byte(&defaultSettings[currentMode][1]);
}
#endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

}  // namespace esphome::matrix_lamp
