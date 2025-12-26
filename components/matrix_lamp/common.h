#pragma once

#define FASTLED_INTERNAL
#define FASTLED_NOSPI
#define FASTLED_NO_PIN_INTERRUPTS

#include <led_sysdefs.h> 
#include <platforms/esp/32/led_sysdefs_esp32.h>

#include <pixeltypes.h>
#include <lib8tion.h>
#include <colorutils.h>
#include <noise.h>
#include <colorpalettes.h>

#include "constants.h"

namespace esphome {
namespace matrix_lamp {

// --- МАТРИЦА ------------------------------------------------------------------------------------------------------------------------------------------

#ifndef ORIENTATION
static uint8_t ORIENTATION = 5;                                    // Ориентация матрицы
#endif
#ifndef MATRIX_TYPE
static uint8_t MATRIX_TYPE = 0;                                    // Тип матрицы: 0 - зигзаг, 1 - параллельная
#endif

/*
  ORIENTATION 0 :: CONNECTION_ANGLE == 0 :: STRIP_DIRECTION == 0
  ORIENTATION 1 :: CONNECTION_ANGLE == 0 :: STRIP_DIRECTION == 1
  ORIENTATION 2 :: CONNECTION_ANGLE == 1 :: STRIP_DIRECTION == 0
  ORIENTATION 3 :: CONNECTION_ANGLE == 1 :: STRIP_DIRECTION == 3
  ORIENTATION 4 :: CONNECTION_ANGLE == 2 :: STRIP_DIRECTION == 2
  ORIENTATION 5 :: CONNECTION_ANGLE == 2 :: STRIP_DIRECTION == 3
  ORIENTATION 6 :: CONNECTION_ANGLE == 3 :: STRIP_DIRECTION == 2
  ORIENTATION 7 :: CONNECTION_ANGLE == 3 :: STRIP_DIRECTION == 1

  CONNECTION_ANGLE - Угол подключения: 0 - левый нижний, 1 - левый верхний, 2 - правый верхний, 3 - правый нижний
  STRIP_DIRECTION  - Направление ленты из угла: 0 - вправо, 1 - вверх, 2 - влево, 3 - вниз
                     при неправильной настройке матрицы вы получите предупреждение "Wrong matrix parameters! Set to default"
                     шпаргалка по настройке матрицы здесь! https://alexgyver.ru/matrix_guide/
*/


// --- Common -------------------------------------------------------------------------------------------------------------------------------------------
static uint8_t FPSdelay = DYNAMIC;

static uint8_t currentMode = MODE_AMOUNT;
static bool loadingFlag = true;

struct ModeType
{
  uint8_t Brightness = 50U;
  uint8_t Speed = 225U;
  uint8_t Scale = 40U;
};

static ModeType modes[MODE_AMOUNT];

#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
static uint8_t selectedSettings = 0U;
#endif //#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

// --- Effects ------------------------------------------------------------------------------------------------------------------------------------------
static uint16_t speed = 20; // speed is set dynamically once we've started up
static uint16_t scale = 30; // scale is set dynamically once we've started up

}  // namespace matrix_lamp
}  // namespace esphome
