#pragma once

#include "esphome.h"
#include "common.h"

#define WU_WEIGHT(a, b) ((uint8_t)(((a) * (b) + (a) + (b)) >> 8))

namespace esphome {
namespace matrix_lamp {

using namespace fastled_helper;

// ------------------------------------------------
// получить номер пикселя в ленте по координатам
// библиотека FastLED тоже использует эту функцию
static uint16_t XY(uint8_t x, uint8_t y)
{
  uint8_t THIS_X;
  uint8_t THIS_Y;
  uint8_t _WIDTH = WIDTH;

  switch (ORIENTATION)
  {
    case 0:
      THIS_X = x;
      THIS_Y =y;
      break;
    case 1:
      _WIDTH = HEIGHT;
      THIS_X = y;
      THIS_Y = x;
      break;
    case 2:
      THIS_X = x;
      THIS_Y = (HEIGHT - y - 1);
      break;
    case 3:
      _WIDTH = HEIGHT;
      THIS_X = (HEIGHT - y - 1);
      THIS_Y = x;
      break;
    case 4:
      THIS_X = (WIDTH - x - 1);
      THIS_Y = (HEIGHT - y - 1);
      break;
    case 5:
      _WIDTH = HEIGHT;
      THIS_X = (HEIGHT - y - 1);
      THIS_Y = (WIDTH - x - 1);
      break;
    case 6:
      THIS_X = (WIDTH - x - 1);
      THIS_Y =y;
      break;
    case 7:
      _WIDTH = HEIGHT;
      THIS_X = y;
      THIS_Y = (WIDTH - x - 1);
      break;
    default :
      THIS_X = x;                                     // !! смотрите инструкцию: https://alexgyver.ru/wp-content/uploads/2018/11/scheme3.jpg
      THIS_Y =y;                                      // !! такого сочетания CONNECTION_ANGLE и STRIP_DIRECTION не бывает
      break;
   }

   if (!(THIS_Y & 0x01) || MATRIX_TYPE)               // Even rows run forwards
     return (THIS_Y * _WIDTH + THIS_X);
   else
     return (THIS_Y * _WIDTH + _WIDTH - THIS_X - 1);  // Odd rows run backwards
}

// ------------------------------------------------
static uint8_t SpeedFactor(uint8_t spd)
{
  uint8_t result = spd * NUM_LEDS / 1024.0;
  return result;
}


// ------------------------------------------------
// функция отрисовки точки по координатам X Y
#if (WIDTH > 127) || (HEIGHT > 127)
static void drawPixelXY(int16_t x, int16_t y, CRGB color)
#else
static void drawPixelXY(int8_t x, int8_t y, CRGB color)
#endif
{
  if (x < 0 || x > (WIDTH - 1) || y < 0 || y > (HEIGHT - 1)) return;
  leds[XY(x, y)] = color;
}


// ------------------------------------------------
// функция получения цвета пикселя по его номеру
static uint32_t getPixColor(uint16_t thisPixel)
{
  if (thisPixel >= NUM_LEDS) return 0;
  return (((uint32_t)leds[thisPixel].r << 16) | ((uint32_t)leds[thisPixel].g << 8 ) | (uint32_t)leds[thisPixel].b); // а почему не просто return (leds[thisPixel])?
}


// ------------------------------------------------
// функция получения цвета пикселя в матрице по его координатам
static uint32_t getPixColorXY(uint8_t x, uint8_t y)
{
  return getPixColor(XY(x, y));
}


// ------------------------------------------------
// неточный, зато более быстрый квадратный корень
static float sqrt3(const float x)
{
  union
  {
    int i;
    float x;
  } u;

  u.x = x;
  u.i = (1 << 29) + (u.i >> 1) - (1 << 22);
  return u.x;
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
static void blur1d(uint16_t numLeds, fract8 blur_amount)
{
    uint8_t keep = 255 - blur_amount;
    uint8_t seep = blur_amount >> 1;
    CRGB carryover = CRGB::Black;
    for( uint16_t i = 0; i < numLeds; ++i) {
        CRGB cur = leds[i];
        CRGB part = cur;
        part.nscale8( seep);
        cur.nscale8( keep);
        cur += carryover;
        if( i) leds[i-1] += part;
        leds[i] = cur;
        carryover = part;
    }
}

static void blurRows(uint8_t width, uint8_t height, fract8 blur_amount)
{
    // blur rows same as columns, for irregular matrix
    uint8_t keep = 255 - blur_amount;
    uint8_t seep = blur_amount >> 1;
    for( uint8_t row = 0; row < height; row++) {
        CRGB carryover = CRGB::Black;
        for( uint8_t i = 0; i < width; i++) {
            CRGB cur = leds[XY(i,row)];
            CRGB part = cur;
            part.nscale8( seep);
            cur.nscale8( keep);
            cur += carryover;
            if(i) leds[XY(i-1,row)] += part;
            leds[XY(i,row)] = cur;
            carryover = part;
        }
    }
}

// blurColumns: perform a blur1d on each column of a rectangular matrix
static void blurColumns(uint8_t width, uint8_t height, fract8 blur_amount)
{
    // blur columns
    uint8_t keep = 255 - blur_amount;
    uint8_t seep = blur_amount >> 1;
    for( uint8_t col = 0; col < width; ++col) {
        CRGB carryover = CRGB::Black;
        for( uint8_t i = 0; i < height; ++i) {
            CRGB cur = leds[XY(col,i)];
            CRGB part = cur;
            part.nscale8( seep);
            cur.nscale8( keep);
            cur += carryover;
            if(i) leds[XY(col,i-1)] += part;
            leds[XY(col,i)] = cur;
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
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
  }
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
  //newcolor.nscale8( 255 - howMuchDarker);
  newcolor.fadeToBlackBy(howMuchDarker);//эквивалент
  return newcolor;
}


// ------------------------------ Дополнительные функции рисования ----------------------
// по мотивам
// https://gist.github.com/sutaburosu/32a203c2efa2bb584f4b846a91066583
static void drawPixelXYF(float x, float y, CRGB color) //, uint8_t darklevel = 0U)
{
//  if (x<0 || y<0) return; //не похоже, чтобы отрицательные значения хоть как-нибудь учитывались тут // зато с этой строчкой пропадает нижний ряд
  // extract the fractional parts and derive their inverses
  uint8_t xx = (x - (int)x) * 255, yy = (y - (int)y) * 255, ix = 255 - xx, iy = 255 - yy;
  // calculate the intensities for each affected pixel
  uint8_t wu[4] = {WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy),
                   WU_WEIGHT(ix, yy), WU_WEIGHT(xx, yy)};
  // multiply the intensities by the colour, and saturating-add them to the pixels
  for (uint8_t i = 0; i < 4; i++) {
    int16_t xn = x + (i & 1), yn = y + ((i >> 1) & 1);
    CRGB clr = getPixColorXY(xn, yn);
    clr.r = qadd8(clr.r, (color.r * wu[i]) >> 8);
    clr.g = qadd8(clr.g, (color.g * wu[i]) >> 8);
    clr.b = qadd8(clr.b, (color.b * wu[i]) >> 8);
//if (darklevel) drawPixelXY(xn, yn, makeDarker(clr, darklevel));
//else
    drawPixelXY(xn, yn, clr);
  }
}


// ------------------------------------------------
static void DrawLine(int x1, int y1, int x2, int y2, CRGB color)
{
  int deltaX = abs(x2 - x1);
  int deltaY = abs(y2 - y1);
  int signX = x1 < x2 ? 1 : -1;
  int signY = y1 < y2 ? 1 : -1;
  int error = deltaX - deltaY;

  drawPixelXY(x2, y2, color);
  while (x1 != x2 || y1 != y2) {
      drawPixelXY(x1, y1, color);
      int error2 = error * 2;
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
static void DrawLineF(float x1, float y1, float x2, float y2, CRGB color)
{
  float deltaX = std::fabs(x2 - x1);
  float deltaY = std::fabs(y2 - y1);
  float error = deltaX - deltaY;

  float signX = x1 < x2 ? 0.5 : -0.5;
  float signY = y1 < y2 ? 0.5 : -0.5;

  while (x1 != x2 || y1 != y2) { // (true) - а я то думаю - "почему функция часто вызывает вылет по вачдогу?" А оно вон оно чё, Михалычь!
      if ((signX > 0 && x1 > x2+signX) || (signX < 0 && x1 < x2+signX)) break;
      if ((signY > 0 && y1 > y2+signY) || (signY < 0 && y1 < y2+signY)) break;
      drawPixelXYF(x1, y1, color); // интересно, почему тут было обычное drawPixelXY() ???
      float error2 = error;
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
static void drawCircleF(float x0, float y0, float radius, CRGB color)
{
  float x = 0, y = radius, error = 0;
  float delta = 1. - 2. * radius;

  while (y >= 0) {
    drawPixelXYF(fmod(x0 + x +WIDTH,WIDTH), y0 + y, color); // сделал, чтобы круги были бесшовными по оси х
    drawPixelXYF(fmod(x0 + x +WIDTH,WIDTH), y0 - y, color);
    drawPixelXYF(fmod(x0 - x +WIDTH,WIDTH), y0 + y, color);
    drawPixelXYF(fmod(x0 - x +WIDTH,WIDTH), y0 - y, color);
    error = 2. * (delta + y) - 1.;
    if (delta < 0 && error <= 0) {
      ++x;
      delta += 2. * x + 1.;
      continue;
    }
    error = 2. * (delta - x) - 1.;
    if (delta > 0 && error > 0) {
      --y;
      delta += 1. - 2. * y;
      continue;
    }
    ++x;
    delta += 2. * (x - y);
    --y;
  }
}


// ------------------------------------------------
static uint8_t wrapX(int8_t x){
  return (x + WIDTH)%WIDTH;
}
static uint8_t wrapY(int8_t y){
  return (y + HEIGHT)%HEIGHT;
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
static uint8_t validMinMax(float val, uint8_t minV, uint32_t maxV) {
  uint8_t result;
  if (val <= minV) {
    result = minV;
  } else if (val >= maxV) {
    result = maxV;
  } else {
    result = ceil(val);
  }
  //  LOG.printf_P(PSTR( "result: %f | val: %f \n\r"), result, val);
  return result;
}


// ------------------------------------------------
// альтернативный градиент для ламп собраных из лент с вертикальной компоновкой
// gradientHorizontal | gradientVertical менее производительный но работает на всех видах ламп
static void gradientHorizontal(uint8_t startX, uint8_t startY, uint8_t endX, uint8_t endY, uint8_t start_color, uint8_t end_color, uint8_t start_br, uint8_t end_br, uint8_t saturate) {
  float step_color = 0;
  float step_br = 0;
  if (startX == endX) {
    endX++;
  }
  if (startY == endY) {
    endY++;
  }
  step_color = (end_color - start_color) / abs(startX - endX);
  if (start_color >  end_color) {
    step_color -= 1.2;
  } else {
    step_color += 1.2;
  }

  step_br = (end_br - start_br) / abs(startX - endX);
  if (start_br >  end_color) {
    step_br -= 1.2;
  } else {
    step_br += 1.2;
  }

  // LOG.printf_P(PSTR( "\n step_color: %f | step_br: %f \n\n\r"), step_color, step_br);
  for (uint8_t x = startX; x < endX; x++) {
    for (uint8_t y = startY; y < endY; y++) {
      CHSV thisColor = CHSV((uint8_t) validMinMax((start_color + (x - startX) * step_color), 1, 254), saturate,
                            (uint8_t) validMinMax((start_br + (x - startX) * step_br), 0, 255) );
      drawPixelXY(x, y, thisColor);
    }
  }
}


// ------------------------------------------------
static void gradientVertical(uint8_t startX, uint8_t startY, uint8_t endX, uint8_t endY, uint8_t start_color, uint8_t end_color, uint8_t start_br, uint8_t end_br, uint8_t saturate) {
  float step_color = 0;
  float step_br = 0;
  if (startX == endX) {
    endX++;
  }
  if (startY == endY) {
    endY++;
  }
  step_color = (end_color - start_color) / abs(startY - endY);

  if (start_color >  end_color) {
    step_color -= 1.2;
  } else {
    step_color += 1.2;
  }

  step_br = (end_br - start_br) / abs(startY - endY);
  if (start_br >  end_color) {
    step_br -= 1.2;
  } else {
    step_br += 1.2;
  }
  for (uint8_t y = startY; y < endY; y++) {
    CHSV thisColor = CHSV( (uint8_t) validMinMax((start_color + (y - startY) * step_color), 0, 255), saturate,
                           (uint8_t) validMinMax((start_br + (y - startY) * step_br), 0, 255) );
    for (uint8_t x = startX; x < endX; x++) {
      drawPixelXY(x, y, thisColor);
    }
  }
}


// ------------------------------------------------
// gradientDownTop • более плавный градиент в отличие от gradientVertical
// но может некоректно работать на лампах собранных на ленточных светодиодах
static void gradientDownTop( uint8_t bottom, CHSV bottom_color, uint8_t top, CHSV top_color ) {
  //  FORWARD_HUES:  hue always goes clockwise
  //  BACKWARD_HUES: hue always goes counter-clockwise
  //  SHORTEST_HUES: hue goes whichever way is shortest
  //  LONGEST_HUES:  hue goes whichever way is longest
  if (ORIENTATION < 3 || ORIENTATION == 7)
  {
    // STRIP_DIRECTION to UP ========
    fill_gradient(leds, top * WIDTH, top_color, bottom * WIDTH, bottom_color, fl::SHORTEST_HUES);
  }
  else
  {
    // STRIP_DIRECTION to DOWN ======
    fill_gradient(leds, NUM_LEDS - bottom * WIDTH - 1, bottom_color, NUM_LEDS - top * WIDTH, top_color, fl::SHORTEST_HUES);
  }
}


// ------------------------------------------------
// новый фейдер
static void fadePixel(uint8_t i, uint8_t j, uint8_t step)
{
  int32_t pixelNum = XY(i, j);
  if (getPixColor(pixelNum) == 0U) return;

  if (leds[pixelNum].r >= 30U ||
      leds[pixelNum].g >= 30U ||
      leds[pixelNum].b >= 30U)
  {
    leds[pixelNum].fadeToBlackBy(step);
  }
  else
  {
    leds[pixelNum] = 0U;
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

}  // namespace matrix_lamp
}  // namespace esphome
