#include "matrix_lamp.h"
#include "matrix_lamp_effect.h"

#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace matrix_lamp {

MatrixLampLightEffect::MatrixLampLightEffect(const char *name) : AddressableLightEffect(name) {}

void MatrixLampLightEffect::start() {
  ESP_LOGD(TAG, "Effect: %s", this->get_name());
  
  AddressableLightEffect::start();
  if (this->matrix_lamp_) {
    this->matrix_lamp_->reset_current_effect();
  }
}

void MatrixLampLightEffect::stop() {
  AddressableLightEffect::stop();
  if (this->matrix_lamp_) {
    this->matrix_lamp_->reset_current_effect();
  }
}

void MatrixLampLightEffect::apply(light::AddressableLight &it, const Color &current_color) {
  if (this->matrix_lamp_) {
    this->matrix_lamp_->ShowFrame(this->mode_, current_color, &it);
    it.schedule_show();
  }
}

}  // namespace matrix_lamp
}  // namespace esphome
