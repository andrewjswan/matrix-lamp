#pragma once

#include "constants.h"
#include "effects.h"
#include "noise_effects.h"

// Если вы хотите добавить эффекты или сделать им копии для демонстрации на разных настройках, нужно делать это в 5 местах:
// 1. в файле effects.ino - добавляется программный код самого эффекта.
// 2. в файле Constants.h - придумываются названия "EFF_......" и задаются порядковые номера эффектам. В конце указывается общее количество MODE_AMOUNT.
// 3. там же в файле Constants.h ещё ниже - задаётся Массив настроек эффектов по умолчанию.
//    Просто добавьте строчку для своего нового эффекта в нужное место. Это тоже не обязательно.
// 5. здесь в файле effectTicker.ino - подключается процедура вызова эффекта на соответствующий ей "EFF_......"
//    Можно подключать один и тот же эффект под разными номерами. Например: EFF_FIRE (24U), EFF_FIRE2 (25U), EFF_FIRE3 (26U). Будет три огня для разных цветов.

namespace esphome {
namespace matrix_lamp {

static uint32_t effTimer;

static void effectsTick()
{
  switch (currentMode)
  {
    #ifdef DEF_WHITE_COLOR
    case EFF_WHITE_COLOR:         HIGH_DELAY_TICK    { effTimer = millis(); whiteColorStripeRoutine();         }  break;  // (  0U) Бeлый cвeт
    #endif
    #ifdef DEF_COLORS
    case EFF_COLORS:              HIGH_DELAY_TICK    { effTimer = millis(); colorsRoutine2();                  }  break;  // (  1U) Cмeнa цвeтa
    #endif
    #ifdef DEF_MADNESS
    case EFF_MADNESS:             HIGH_DELAY_TICK    { effTimer = millis(); madnessNoiseRoutine();             }  break;  // (  2U) Бeзyмиe
    #endif
    #ifdef DEF_CLOUDS
    case EFF_CLOUDS:              HIGH_DELAY_TICK    { effTimer = millis(); cloudsNoiseRoutine();              }  break;  // (  3U) Oблaкa
    #endif
    #ifdef DEF_LAVA
    case EFF_LAVA:                HIGH_DELAY_TICK    { effTimer = millis(); lavaNoiseRoutine();                }  break;  // (  4U) Лaвa
    #endif
    #ifdef DEF_PLASMA
    case EFF_PLASMA:              HIGH_DELAY_TICK    { effTimer = millis(); plasmaNoiseRoutine();              }  break;  // (  5U) Плaзмa
    #endif
    #ifdef DEF_RAINBOW
    case EFF_RAINBOW:             HIGH_DELAY_TICK    { effTimer = millis(); rainbowNoiseRoutine();             }  break;  // (  6U) Paдyгa 3D
    #endif
    #ifdef DEF_RAINBOW_STRIPE
    case EFF_RAINBOW_STRIPE:      HIGH_DELAY_TICK    { effTimer = millis(); rainbowStripeNoiseRoutine();       }  break;  // (  7U) Пaвлин
    #endif
    #ifdef DEF_ZEBRA
    case EFF_ZEBRA:               HIGH_DELAY_TICK    { effTimer = millis(); zebraNoiseRoutine();               }  break;  // (  8U) 3eбpa
    #endif
    #ifdef DEF_FOREST
    case EFF_FOREST:              HIGH_DELAY_TICK    { effTimer = millis(); forestNoiseRoutine();              }  break;  // (  9U) Лec
    #endif
    #ifdef DEF_OCEAN
    case EFF_OCEAN:               HIGH_DELAY_TICK    { effTimer = millis(); oceanNoiseRoutine();               }  break;  // ( 10U) Oкeaн
    #endif
    #ifdef DEF_BBALLS
    case EFF_BBALLS:              LOW_DELAY_TICK     { effTimer = millis(); BBallsRoutine();                   }  break;  // ( 11U) Mячики
    #endif
    #ifdef DEF_BALLS_BOUNCE
    case EFF_BALLS_BOUNCE:        LOW_DELAY_TICK     { effTimer = millis(); bounceRoutine();                   }  break;  // ( 12U) Mячики бeз гpaниц
    #endif
    #ifdef DEF_POPCORN
    case EFF_POPCORN:             LOW_DELAY_TICK     { effTimer = millis(); popcornRoutine();                  }  break;  // ( 13U) Пoпкopн
    #endif
    #ifdef DEF_SPIRO
    case EFF_SPIRO:               LOW_DELAY_TICK     { effTimer = millis(); spiroRoutine();                    }  break;  // ( 14U) Cпиpaли
    #endif
    #ifdef DEF_PRISMATA
    case EFF_PRISMATA:            LOW_DELAY_TICK     { effTimer = millis(); PrismataRoutine();                 }  break;  // ( 15U) Пpизмaтa
    #endif
    #ifdef DEF_SMOKEBALLS
    case EFF_SMOKEBALLS:          LOW_DELAY_TICK     { effTimer = millis(); smokeballsRoutine();               }  break;  // ( 16U) Дымoвыe шaшки
    #endif
    #ifdef DEF_FLAME
    case EFF_FLAME:               LOW_DELAY_TICK     { effTimer = millis(); execStringsFlame();                }  break;  // ( 17U) Плaмя
    #endif
    #ifdef DEF_FIRE_2021
    case EFF_FIRE_2021:           LOW_DELAY_TICK     { effTimer = millis(); Fire2021Routine();                 }  break;  // ( 18U) Oгoнь 2021
    #endif
    #ifdef DEF_PACIFIC
    case EFF_PACIFIC:             LOW_DELAY_TICK     { effTimer = millis(); pacificRoutine();                  }  break;  // ( 19U) Tиxий oкeaн
    #endif
    #ifdef DEF_SHADOWS
    case EFF_SHADOWS:             LOW_DELAY_TICK     { effTimer = millis(); shadowsRoutine();                  }  break;  // ( 20U) Teни
    #endif
    #ifdef DEF_DNA
    case EFF_DNA:                 LOW_DELAY_TICK     { effTimer = millis(); DNARoutine();                      }  break;  // ( 21U) ДHK
    #endif
    #ifdef DEF_FLOCK
    case EFF_FLOCK:               LOW_DELAY_TICK     { effTimer = millis(); flockRoutine(false);               }  break;  // ( 22U) Cтaя
    #endif
    #ifdef DEF_FLOCK_N_PR
    case EFF_FLOCK_N_PR:          LOW_DELAY_TICK     { effTimer = millis(); flockRoutine(true);                }  break;  // ( 23U) Cтaя и xищник
    #endif
    #ifdef DEF_BUTTERFLYS
    case EFF_BUTTERFLYS:          LOW_DELAY_TICK     { effTimer = millis(); butterflysRoutine(true);           }  break;  // ( 24U) Moтыльки
    #endif
    #ifdef DEF_BUTTERFLYS_LAMP
    case EFF_BUTTERFLYS_LAMP:     LOW_DELAY_TICK     { effTimer = millis(); butterflysRoutine(false);          }  break;  // ( 25U) Лaмпa c мoтылькaми
    #endif
    #ifdef DEF_SNAKES
    case EFF_SNAKES:              LOW_DELAY_TICK     { effTimer = millis(); snakesRoutine();                   }  break;  // ( 26U) 3мeйки
    #endif
    #ifdef DEF_NEXUS
    case EFF_NEXUS:               LOW_DELAY_TICK     { effTimer = millis(); nexusRoutine();                    }  break;  // ( 27U) Nexus
    #endif
    #ifdef DEF_SPHERES
    case EFF_SPHERES:             LOW_DELAY_TICK     { effTimer = millis(); spheresRoutine();                  }  break;  // ( 28U) Шapы
    #endif
    #ifdef DEF_SINUSOID3
    case EFF_SINUSOID3:           HIGH_DELAY_TICK    { effTimer = millis(); Sinusoid3Routine();                }  break;  // ( 29U) Cинycoид
    #endif
    #ifdef DEF_METABALLS
    case EFF_METABALLS:           LOW_DELAY_TICK     { effTimer = millis(); MetaBallsRoutine();                }  break;  // ( 30U) Meтaбoлз
    #endif
    #ifdef DEF_AURORA
    case EFF_AURORA:              HIGH_DELAY_TICK    { effTimer = millis(); polarRoutine();                    }  break;  // ( 31U) Ceвepнoe cияниe
    #endif
    #ifdef DEF_SPIDER
    case EFF_SPIDER:              LOW_DELAY_TICK     { effTimer = millis(); spiderRoutine();                   }  break;  // ( 32U) Плaзмeннaя лaмпa
    #endif
    #ifdef DEF_LAVALAMP
    case EFF_LAVALAMP:            LOW_DELAY_TICK     { effTimer = millis(); LavaLampRoutine();                 }  break;  // ( 33U) Лaвoвaя лaмпa
    #endif
    #ifdef DEF_LIQUIDLAMP
    case EFF_LIQUIDLAMP:          LOW_DELAY_TICK     { effTimer = millis(); LiquidLampRoutine(true);           }  break;  // ( 34U) Жидкaя лaмпa
    #endif
    #ifdef DEF_LIQUIDLAMP_AUTO
    case EFF_LIQUIDLAMP_AUTO:     LOW_DELAY_TICK     { effTimer = millis(); LiquidLampRoutine(false);          }  break;  // ( 35U) Жидкaя лaмпa (auto)
    #endif
    #ifdef DEF_DROPS
    case EFF_DROPS:               LOW_DELAY_TICK     { effTimer = millis(); newMatrixRoutine();                }  break;  // ( 36U) Kaпли нa cтeклe
    #endif
    #ifdef DEF_MATRIX
    case EFF_MATRIX:              DYNAMIC_DELAY_TICK { effTimer = millis(); matrixRoutine();                   }  break;  // ( 37U) Maтpицa
    #endif
    #ifdef DEF_FIRE_2012
    case EFF_FIRE_2012:           DYNAMIC_DELAY_TICK { effTimer = millis(); fire2012again();                   }  break;  // ( 38U) Oгoнь 2012
    #endif
    #ifdef DEF_FIRE_2018
    case EFF_FIRE_2018:           DYNAMIC_DELAY_TICK { effTimer = millis(); Fire2018_2();                      }  break;  // ( 39U) Oгoнь 2018
    #endif
    #ifdef DEF_FIRE_2020
    case EFF_FIRE_2020:           DYNAMIC_DELAY_TICK { effTimer = millis(); fire2020Routine2();                }  break;  // ( 40U) Oгoнь 2020
    #endif
    #ifdef DEF_FIRE_2025
    case EFF_FIRE_2025:           DYNAMIC_DELAY_TICK { effTimer = millis(); fire2025Routine();                 }  break;  // ( 41U) Oгoнь 2025
    #endif
    #ifdef DEF_FIRE
    case EFF_FIRE:                DYNAMIC_DELAY_TICK { effTimer = millis(); fireRoutine(true);                 }  break;  // ( 42U) Oгoнь
    #endif
    #ifdef DEF_WHIRL
    case EFF_WHIRL:               DYNAMIC_DELAY_TICK { effTimer = millis(); whirlRoutine(true);                }  break;  // ( 43U) Bиxpи плaмeни
    #endif
    #ifdef DEF_WHIRL_MULTI
    case EFF_WHIRL_MULTI:         DYNAMIC_DELAY_TICK { effTimer = millis(); whirlRoutine(false);               }  break;  // ( 44U) Paзнoцвeтныe виxpи
    #endif
    #ifdef DEF_MAGMA
    case EFF_MAGMA:               DYNAMIC_DELAY_TICK { effTimer = millis(); magmaRoutine();                    }  break;  // ( 45U) Maгмa
    #endif
    #ifdef DEF_LLAND
    case EFF_LLAND:               DYNAMIC_DELAY_TICK { effTimer = millis(); LLandRoutine();                    }  break;  // ( 46U) Kипeниe
    #endif
    #ifdef DEF_WATERFALL
    case EFF_WATERFALL:           DYNAMIC_DELAY_TICK { effTimer = millis(); fire2012WithPalette();             }  break;  // ( 47U) Boдoпaд
    #endif
    #ifdef DEF_WATERFALL_4IN1
    case EFF_WATERFALL_4IN1:      DYNAMIC_DELAY_TICK { effTimer = millis(); fire2012WithPalette4in1();         }  break;  // ( 48U) Boдoпaд 4 в 1
    #endif
    #ifdef DEF_POOL
    case EFF_POOL:                DYNAMIC_DELAY_TICK { effTimer = millis(); poolRoutine();                     }  break;  // ( 49U) Бacceйн
    #endif
    #ifdef DEF_PULSE
    case EFF_PULSE:               DYNAMIC_DELAY_TICK { effTimer = millis(); pulseRoutine(2U);                  }  break;  // ( 50U) Пyльc
    #endif
    #ifdef DEF_PULSE_RAINBOW
    case EFF_PULSE_RAINBOW:       DYNAMIC_DELAY_TICK { effTimer = millis(); pulseRoutine(4U);                  }  break;  // ( 51U) Paдyжный пyльc
    #endif
    #ifdef DEF_PULSE_WHITE
    case EFF_PULSE_WHITE:         DYNAMIC_DELAY_TICK { effTimer = millis(); pulseRoutine(8U);                  }  break;  // ( 52U) Бeлый пyльc
    #endif
    #ifdef DEF_OSCILLATING
    case EFF_OSCILLATING:         DYNAMIC_DELAY_TICK { effTimer = millis(); oscillatingRoutine();              }  break;  // ( 53U) Ocциллятop
    #endif
    #ifdef DEF_FOUNTAIN
    case EFF_FOUNTAIN:            DYNAMIC_DELAY_TICK { effTimer = millis(); starfield2Routine();               }  break;  // ( 54U) Иcтoчник
    #endif
    #ifdef DEF_FAIRY
    case EFF_FAIRY:               DYNAMIC_DELAY_TICK { effTimer = millis(); fairyRoutine();                    }  break;  // ( 55U) Фeя
    #endif
    #ifdef DEF_COMET
    case EFF_COMET:               DYNAMIC_DELAY_TICK { effTimer = millis(); RainbowCometRoutine();             }  break;  // ( 56U) Koмeтa
    #endif
    #ifdef DEF_COMET_COLOR
    case EFF_COMET_COLOR:         DYNAMIC_DELAY_TICK { effTimer = millis(); ColorCometRoutine();               }  break;  // ( 57U) Oднoцвeтнaя кoмeтa
    #endif
    #ifdef DEF_COMET_TWO
    case EFF_COMET_TWO:           DYNAMIC_DELAY_TICK { effTimer = millis(); MultipleStream();                  }  break;  // ( 58U) Двe кoмeты
    #endif
    #ifdef DEF_COMET_THREE
    case EFF_COMET_THREE:         DYNAMIC_DELAY_TICK { effTimer = millis(); MultipleStream2();                 }  break;  // ( 59U) Тpи кoмeты
    #endif
    #ifdef DEF_ATTRACT
    case EFF_ATTRACT:             DYNAMIC_DELAY_TICK { effTimer = millis(); attractRoutine();                  }  break;  // ( 60U) Пpитяжeниe
    #endif
    #ifdef DEF_FIREFLY
    case EFF_FIREFLY:             DYNAMIC_DELAY_TICK { effTimer = millis(); MultipleStream3();                 }  break;  // ( 61U) Пapящий oгoнь
    #endif
    #ifdef DEF_FIREFLY_TOP
    case EFF_FIREFLY_TOP:         DYNAMIC_DELAY_TICK { effTimer = millis(); MultipleStream5();                 }  break;  // ( 62U) Bepxoвoй oгoнь
    #endif
    #ifdef DEF_SNAKE
    case EFF_SNAKE:               DYNAMIC_DELAY_TICK { effTimer = millis(); MultipleStream8();                 }  break;  // ( 63U) Paдyжный змeй
    #endif
    #ifdef DEF_SPARKLES
    case EFF_SPARKLES:            DYNAMIC_DELAY_TICK { effTimer = millis(); sparklesRoutine();                 }  break;  // ( 64U) Koнфeтти
    #endif
    #ifdef DEF_TWINKLES
    case EFF_TWINKLES:            DYNAMIC_DELAY_TICK { effTimer = millis(); twinklesRoutine();                 }  break;  // ( 65U) Mepцaниe
    #endif
    #ifdef DEF_SMOKE
    case EFF_SMOKE:               DYNAMIC_DELAY_TICK { effTimer = millis(); MultipleStreamSmoke(false);        }  break;  // ( 66U) Дым
    #endif
    #ifdef DEF_SMOKE_COLOR
    case EFF_SMOKE_COLOR:         DYNAMIC_DELAY_TICK { effTimer = millis(); MultipleStreamSmoke(true);         }  break;  // ( 67U) Paзнoцвeтный дым
    #endif
    #ifdef DEF_PICASSO
    case EFF_PICASSO:             DYNAMIC_DELAY_TICK { effTimer = millis(); picassoSelector();                 }  break;  // ( 68U) Пикacco
    #endif
    #ifdef DEF_WAVES
    case EFF_WAVES:               DYNAMIC_DELAY_TICK { effTimer = millis(); WaveRoutine();                     }  break;  // ( 69U) Boлны
    #endif
    #ifdef DEF_SAND
    case EFF_SAND:                DYNAMIC_DELAY_TICK { effTimer = millis(); sandRoutine();                     }  break;  // ( 70U) Цвeтныe дpaжe
    #endif
    #ifdef DEF_RINGS
    case EFF_RINGS:               DYNAMIC_DELAY_TICK { effTimer = millis(); ringsRoutine();                    }  break;  // ( 71U) Koдoвый зaмoк
    #endif
    #ifdef DEF_CUBE2D
    case EFF_CUBE2D:              DYNAMIC_DELAY_TICK { effTimer = millis(); cube2dRoutine();                   }  break;  // ( 72U) Kyбик Pyбикa
    #endif
    #ifdef DEF_SIMPLE_RAIN
    case EFF_SIMPLE_RAIN:         DYNAMIC_DELAY_TICK { effTimer = millis(); simpleRain();                      }  break;  // ( 73U) Tyчкa в бaнкe
    #endif
    #ifdef DEF_STORMY_RAIN
    case EFF_STORMY_RAIN:         DYNAMIC_DELAY_TICK { effTimer = millis(); stormyRain();                      }  break;  // ( 74U) Гроза в банке
    #endif
    #ifdef DEF_COLOR_RAIN
    case EFF_COLOR_RAIN:          DYNAMIC_DELAY_TICK { effTimer = millis(); coloredRain();                     }  break;  // ( 75U) Ocaдки
    #endif
    #ifdef DEF_SNOW
    case EFF_SNOW:                DYNAMIC_DELAY_TICK { effTimer = millis(); snowRoutine();                     }  break;  // ( 76U) Cнeгoпaд
    #endif
    #ifdef DEF_STARFALL
    case EFF_STARFALL:            DYNAMIC_DELAY_TICK { effTimer = millis(); stormRoutine2();                   }  break;  // ( 77U) 3вeздoпaд / Meтeль
    #endif
    #ifdef DEF_LEAPERS
    case EFF_LEAPERS:             DYNAMIC_DELAY_TICK { effTimer = millis(); LeapersRoutine();                  }  break;  // ( 78U) Пpыгyны
    #endif
    #ifdef DEF_LIGHTERS
    case EFF_LIGHTERS:            DYNAMIC_DELAY_TICK { effTimer = millis(); lightersRoutine();                 }  break;  // ( 79U) Cвeтлячки
    #endif
    #ifdef DEF_LIGHTER_TRACES
    case EFF_LIGHTER_TRACES:      DYNAMIC_DELAY_TICK { effTimer = millis(); ballsRoutine();                    }  break;  // ( 80U) Cвeтлячки co шлeйфoм
    #endif
    #ifdef DEF_LUMENJER
    case EFF_LUMENJER:            DYNAMIC_DELAY_TICK { effTimer = millis(); lumenjerRoutine();                 }  break;  // ( 81U) Люмeньep
    #endif
    #ifdef DEF_PAINTBALL
    case EFF_PAINTBALL:           DYNAMIC_DELAY_TICK { effTimer = millis(); lightBallsRoutine();               }  break;  // ( 82U) Пeйнтбoл
    #endif
    #ifdef DEF_RAINBOW_VER
    case EFF_RAINBOW_VER:         DYNAMIC_DELAY_TICK { effTimer = millis(); rainbowRoutine();                  }  break;  // ( 83U) Paдyгa
    #endif
    #ifdef DEF_CHRISTMAS_TREE
    case EFF_CHRISTMAS_TREE:      DYNAMIC_DELAY_TICK { effTimer = millis(); ChristmasTree();                   }  break;  // ( 84U) Новорічна ялинка
    #endif
    #ifdef DEF_BY_EFFECT
    case EFF_BY_EFFECT:           DYNAMIC_DELAY_TICK { effTimer = millis(); ByEffect();                        }  break;  // ( 85U) Побічний ефект
    #endif
    #ifdef DEF_COLOR_FRIZZLES
    case EFF_COLOR_FRIZZLES:      SOFT_DELAY_TICK    { effTimer = millis(); ColorFrizzles();                   }  break;  // ( 86U) Кольорові кучері
    #endif
    #ifdef DEF_COLORED_PYTHON
    case EFF_COLORED_PYTHON:      LOW_DELAY_TICK     { effTimer = millis(); Colored_Python();                  }  break;  // ( 87U) Кольоровий Пітон
    #endif
    #ifdef DEF_CONTACTS
    case EFF_CONTACTS:            DYNAMIC_DELAY_TICK { effTimer = millis(); Contacts();                        }  break;  // ( 88U) Контакти
    #endif
    #ifdef DEF_DROP_IN_WATER
    case EFF_DROP_IN_WATER:       DYNAMIC_DELAY_TICK { effTimer = millis(); DropInWater();                     }  break;  // ( 89U) Краплі  на воді
    #endif
    #ifdef DEF_FEATHER_CANDLE
    case EFF_FEATHER_CANDLE:      DYNAMIC_DELAY_TICK { effTimer = millis(); FeatherCandleRoutine();            }  break;  // ( 90U) Свічка
    #endif
    #ifdef DEF_FIREWORK
    case EFF_FIREWORK:            SOFT_DELAY_TICK    { effTimer = millis(); Firework();                        }  break;  // ( 91U) Феєрверк
    #endif
    #ifdef DEF_FIREWORK_2
    case EFF_FIREWORK_2:          DYNAMIC_DELAY_TICK { effTimer = millis(); fireworksRoutine();                }  break;  // ( 92U) Феєрверк 2
    #endif
    #ifdef DEF_HOURGLASS
    case EFF_HOURGLASS:           DYNAMIC_DELAY_TICK { effTimer = millis(); Hourglass();                       }  break;  // ( 93U) Пісочний годинник
    #endif
    #ifdef DEF_FLOWERRUTA
    case EFF_FLOWERRUTA:          DYNAMIC_DELAY_TICK { effTimer = millis(); FlowerRuta();                      }  break;  // ( 94U) Червона рута (Аленький цветочек)
    #endif
    #ifdef DEF_MAGIC_LANTERN
    case EFF_MAGIC_LANTERN :      DYNAMIC_DELAY_TICK { effTimer = millis(); MagicLantern();                    }  break;  // ( 95U) Чарівний ліхтарик
    #endif
    #ifdef DEF_MOSAIC
    case EFF_MOSAIC:              DYNAMIC_DELAY_TICK { effTimer = millis(); squaresNdotsRoutine();             }  break;  // ( 96U) Мозайка
    #endif
    #ifdef DEF_OCTOPUS
    case EFF_OCTOPUS:             DYNAMIC_DELAY_TICK { effTimer = millis(); Octopus();                         }  break;  // ( 97U) Восьминіг
    #endif
    #ifdef DEF_PAINTS
    case EFF_PAINTS:              DYNAMIC_DELAY_TICK { effTimer = millis(); OilPaints();                       }  break;  // ( 98U) Олійні фарби
    #endif
    #ifdef DEF_PLASMA_WAVES
    case EFF_PLASMA_WAVES:        SOFT_DELAY_TICK    { effTimer = millis(); Plasma_Waves();                    }  break;  // ( 99U) Плазмові хвілі
    #endif
    #ifdef DEF_RADIAL_WAVE
    case EFF_RADIAL_WAVE:         DYNAMIC_DELAY_TICK { effTimer = millis(); RadialWave();                      }  break;  // (100U) Радіальна хвиля
    #endif
    #ifdef DEF_RIVERS
    case EFF_RIVERS:              DYNAMIC_DELAY_TICK { effTimer = millis(); BotswanaRivers();                  }  break;  // (101U) Річки Ботсвани
    #endif
    #ifdef DEF_SPECTRUM
    case EFF_SPECTRUM:            DYNAMIC_DELAY_TICK { effTimer = millis(); Spectrum();                        }  break;  // (102U) Спектрум
    #endif
    #ifdef DEF_STROBE
    case EFF_STROBE:              LOW_DELAY_TICK     { effTimer = millis(); StrobeAndDiffusion();              }  break;  // (103U) Строб.Хаос.Дифузія
    #endif
    #ifdef DEF_SWIRL
    case EFF_SWIRL:               DYNAMIC_DELAY_TICK { effTimer = millis(); Swirl();                           }  break;  // (104U) Завиток
    #endif
    #ifdef DEF_TORNADO
    case EFF_TORNADO:             LOW_DELAY_TICK     { effTimer = millis(); Tornado();                         }  break;  // (105U) Торнадо
    #endif
    #ifdef DEF_WATERCOLOR
    case EFF_WATERCOLOR:          DYNAMIC_DELAY_TICK { effTimer = millis(); Watercolor();                      }  break;  // (106U) Акварель
    #endif
    #ifdef DEF_WEB_TOOLS
    case EFF_WEB_TOOLS:           SOFT_DELAY_TICK    { effTimer = millis(); WebTools();                        }  break;  // (107U) Мрія дизайнера
    #endif
    #ifdef DEF_WINE
    case EFF_WINE:                DYNAMIC_DELAY_TICK { effTimer = millis(); colorsWine();                      }  break;  // (108U) Вино
    #endif
    #ifdef DEF_BAMBOO
    case EFF_BAMBOO:              DYNAMIC_DELAY_TICK { effTimer = millis(); Bamboo();                          }  break;  // (109U) Бамбук
    #endif
    #ifdef DEF_BALLROUTINE
    case EFF_BALLROUTINE:         DYNAMIC_DELAY_TICK { effTimer = millis(); ballRoutine();                     }  break;  // (110U) Блуждающий кубик
    #endif
    #ifdef DEF_STARS
    case EFF_STARS:               DYNAMIC_DELAY_TICK { effTimer = millis(); EffectStars();                     }  break;  // (111U) Звезды
    #endif
    #ifdef DEF_TIXYLAND
    case EFF_TIXYLAND:            DYNAMIC_DELAY_TICK { effTimer = millis(); TixyLand();                        }  break;  // (112U) Земля Тикси
    #endif
    #ifdef DEF_FIRESPARKS
    case EFF_FIRESPARKS:          DYNAMIC_DELAY_TICK { effTimer = millis(); FireSparks();                      }  break;  // (113U) Огонь с искрами
    #endif
    #ifdef DEF_DANDELIONS
    case EFF_DANDELIONS:          SOFT_DELAY_TICK    { effTimer = millis(); Dandelions();                      }  break;  // (114U) Разноцветные одуванчики
    #endif
    #ifdef DEF_SERPENTINE
    case EFF_SERPENTINE:          HIGH_DELAY_TICK    { effTimer = millis(); Serpentine();                      }  break;  // (115U) Серпантин
    #endif
    #ifdef DEF_TURBULENCE
    case EFF_TURBULENCE:          DYNAMIC_DELAY_TICK { effTimer = millis(); Turbulence();                      }  break;  // (116U) Цифровая турбулентность
    #endif
    #ifdef DEF_ARROWS
    case EFF_ARROWS:              DYNAMIC_DELAY_TICK { effTimer = millis(); arrowsRoutine();                   }  break;  // (117U) Стрелки
    #endif
    #ifdef DEF_AVRORA
    case EFF_AVRORA:              HIGH_DELAY_TICK    { effTimer = millis(); Avrora();                          }  break;  // (118U) Аврора
    #endif
    #ifdef DEF_LOTUS
    case EFF_LOTUS:               DYNAMIC_DELAY_TICK { effTimer = millis(); LotusFlower();                     }  break;  // (119U) Цветок лотоса
    #endif
    #ifdef DEF_FONTAN
    case EFF_FONTAN:              DYNAMIC_DELAY_TICK { effTimer = millis(); Fountain();                        }  break;  // (120U) Фонтан
    #endif
    #ifdef DEF_NIGHTCITY
    case EFF_NIGHTCITY:           HIGH_DELAY_TICK    { effTimer = millis(); NightCity();                       }  break;  // (121U) Ночной Город
    #endif
    #ifdef DEF_RAIN
    case EFF_RAIN:                DYNAMIC_DELAY_TICK { effTimer = millis(); RainRoutine();                     }  break;  // (122U) Разноцветный дождь
    #endif
    #ifdef DEF_SCANNER
    case EFF_SCANNER:             DYNAMIC_DELAY_TICK { effTimer = millis(); Scanner();                         }  break;  // (123U) Сканер
    #endif
    #ifdef DEF_MIRAGE
    case EFF_MIRAGE:              DYNAMIC_DELAY_TICK { effTimer = millis(); Mirage();                          }  break;  // (124U) Міраж
    #endif
    #ifdef DEF_HANDFAN
    case EFF_HANDFAN:             DYNAMIC_DELAY_TICK { effTimer = millis(); HandFan();                         }  break;  // (125U) Опахало
    #endif
    #ifdef DEF_LIGHTFILTER
    case EFF_LIGHTFILTER:         DYNAMIC_DELAY_TICK { effTimer = millis(); LightFilter();                     }  break;  // (126U) Cвітлофільтр
    #endif
    #ifdef DEF_TASTEHONEY
    case EFF_TASTEHONEY:          DYNAMIC_DELAY_TICK { effTimer = millis(); TasteHoney();                      }  break;  // (127U) Смак Меду
    #endif
    #ifdef DEF_SPINDLE
    case EFF_SPINDLE:             DYNAMIC_DELAY_TICK { effTimer = millis(); Spindle();                         }  break;  // (128U) Веретено
    #endif
    #ifdef DEF_POPURI
    case EFF_POPURI:              HIGH_DELAY_TICK    { effTimer = millis(); Popuri();                          }  break;  // (129U) Попурі
    #endif
    #ifdef DEF_RAINBOW_SPOT
    case EFF_RAINBOW_SPOT:        DYNAMIC_DELAY_TICK { effTimer = millis(); RainbowSpot();                     }  break;  // (130U) Веселкова Пляма
    #endif
    #ifdef DEF_RAINBOW_RINGS
    case EFF_RAINBOW_RINGS:       DYNAMIC_DELAY_TICK { effTimer = millis(); RainbowRings();                    }  break;  // (131U) Веселкові кільця
    #endif
    #ifdef DEF_VYSHYVANKA
    case EFF_VYSHYVANKA:          DYNAMIC_DELAY_TICK { effTimer = millis(); munchRoutine();                    }  break;  // (132U) Вишиванка
    #endif
    #ifdef DEF_INCREMENTAL_DRIFT
    case EFF_INCREMENTALDRIFT:    DYNAMIC_DELAY_TICK { effTimer = millis(); IncrementalDriftRoutine();         }  break;  // (133U) Incremental Drift
    #endif
    #ifdef DEF_BUTTERFLY
    case EFF_BUTTERFLY:           LOW_DELAY_TICK { effTimer = millis(); butterflyRoutine();                    }  break;  // (134U) Бабочка
    #endif
    #ifdef DEF_STARS_NIGHT
    case EFF_STARS_NIGHT:         LOW_DELAY_TICK { effTimer = millis(); StarsEffect();                         }  break;  // (135U) Звездная ночь
    #endif
    #ifdef DEF_UKRAINE
    case EFF_UKRAINE:             DYNAMIC_DELAY_TICK { effTimer = millis(); Ukraine();                         }  break;  // (136U) Україна
    #endif
  }
}

}  // namespace matrix_lamp
}  // namespace esphome
