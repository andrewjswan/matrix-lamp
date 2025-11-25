#pragma once

#include "esphome/components/light/addressable_light_effect.h"

namespace esphome {
namespace matrix_lamp {

class MatrixLamp;

class MatrixLampLightEffect : public light::AddressableLightEffect {
 public:
  MatrixLampLightEffect(const char *name);

  void start() override;
  void stop() override;
  void apply(light::AddressableLight &it, const Color &current_color) override;
  void set_mode(uint8_t mode) { this->mode_ = mode; }
  void set_matrix_lamp(MatrixLamp *matrix_lamp) { this->matrix_lamp_ = matrix_lamp; }

 protected:
  uint8_t mode_{0};
  MatrixLamp *matrix_lamp_{nullptr};
};

}  // namespace matrix_lamp
}  // namespace esphome
