#include "matrix_lamp.h"

#include "esphome/core/defines.h"

namespace esphome::matrix_lamp {

#if defined(MATRIX_LAMP_USE_DISPLAY)
MatrixLamp_Icon::MatrixLamp_Icon(const uint8_t *data_start, uint32_t width, uint32_t height, uint32_t animation_frame_count, esphome::image::ImageType type, std::string icon_name, bool revers, uint16_t frame_duration, esphome::image::Transparency transparency) {
  this->animation_ = new esphome::animation::Animation(data_start, width, height, animation_frame_count, type, transparency);
  this->name = icon_name;
  this->reverse = revers;
  this->frame_duration = frame_duration;
  this->counting_up = true;
}

void MatrixLamp_Icon::next_frame() {
  if (this->animation_ == nullptr) return;

  if (this->animation_->get_animation_frame_count() > 1) {
    if (this->counting_up) {
      if (this->reverse && (this->animation_->get_current_frame() == this->animation_->get_animation_frame_count() - 2)) {
        this->counting_up = false;
      }
      this->animation_->next_frame();
    } else {
      if (this->animation_->get_current_frame() == 1) {
        this->counting_up = true;
      }
      this->animation_->prev_frame();
    }
  }
}
#endif // #if defined(MATRIX_LAMP_USE_DISPLAY)

}  // namespace esphome::matrix_lamp
