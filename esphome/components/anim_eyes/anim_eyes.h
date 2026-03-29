#pragma once

#include "esphome/core/component.h"
#include "esphome/components/display/display_buffer.h"
#include <vector>
#include <map>
#include <string>
#include <cstdint>
#include "esp_timer.h"

namespace esphome {
namespace anim_eyes {

// Emotion structure to hold emotion data with weight
struct Emotion {
  std::string name;
  float weight;
  
  Emotion(const std::string &name, float weight) : name(name), weight(weight) {}
};

// Eye shape structure for rectangular eyes
struct EyeShape {
  int center_x;
  int center_y;
  int width;
  int height;
  int border_radius;
};

// Animation state for eye movement and expression
struct AnimationState {
  float look_x;           // Pupil horizontal position (-1.0 to 1.0)
  float look_y;           // Pupil vertical position (-1.0 to 1.0)
  bool is_blinking;
  uint8_t blink_progress;  // 0-255 for smooth blink animation
  float eyelid_top;       // Top eyelid position (0.0 to 1.0, where 1.0 is fully open)
  float eyelid_bottom;    // Bottom eyelid position (0.0 to 1.0, where 1.0 is fully open)
  uint32_t last_blink_time;
  uint32_t last_look_time;
  uint32_t last_behavior_time;
};

class AnimEyes : public Component {
 public:
  AnimEyes() = default;
  
  // Setup and lifecycle
  void setup() override;
  void dump_config() override;
  
  // Display configuration
  void set_display(display::DisplayBuffer *display) { display_ = display; }
  
  // Eye dimensions
  void set_eye_size(uint16_t size) { eye_size_ = size; }
  void set_eye_distance(uint16_t distance) { eye_distance_ = distance; }
  uint16_t get_eye_size() const { return eye_size_; }
  uint16_t get_eye_distance() const { return eye_distance_; }
  
  // Rendering performance
  void set_update_interval(uint32_t interval_ms) { update_interval_ms_ = interval_ms; }
  uint32_t get_update_interval() const { return update_interval_ms_; }
  
  // Behavior toggles
  void set_random_behavior(bool enabled) { random_behavior_ = enabled; }
  void set_random_blink(bool enabled) { random_blink_ = enabled; }
  void set_random_look(bool enabled) { random_look_ = enabled; }
  
  bool get_random_behavior() const { return random_behavior_; }
  bool get_random_blink() const { return random_blink_; }
  bool get_random_look() const { return random_look_; }
  
  // Timing intervals
  void set_blink_interval(uint32_t interval_ms) { blink_interval_ms_ = interval_ms; }
  void set_look_interval(uint32_t interval_ms) { look_interval_ms_ = interval_ms; }
  void set_behavior_interval(uint32_t interval_ms) { behavior_interval_ms_ = interval_ms; }
  
  uint32_t get_blink_interval() const { return blink_interval_ms_; }
  uint32_t get_look_interval() const { return look_interval_ms_; }
  uint32_t get_behavior_interval() const { return behavior_interval_ms_; }
  
  // Emotion management
  void add_emotion(const std::string &name, float weight);
  void set_current_emotion(const std::string &name);
  const std::string &get_current_emotion() const { return current_emotion_; }
  const std::vector<Emotion> &get_emotions() const { return emotions_; }
  
  // Animation control
  void trigger_blink();
  void trigger_look();
  void trigger_behavior_change();
  
  // Drawing methods
  void draw_eye_(const EyeShape &eye);
  void draw_eyes();
  void clear_display();
  
  // Animation update callback
  void animate_();
  
 protected:
  // Display reference
  display::DisplayBuffer *display_{nullptr};
  
  // Eye configuration
  uint16_t eye_size_{22};
  uint16_t eye_distance_{6};
  
  // Rendering
  uint32_t update_interval_ms_{50};
  uint32_t last_update_time_{0};
  
  // Behavior configuration
  bool random_behavior_{true};
  bool random_blink_{true};
  bool random_look_{true};
  
  // Timing intervals
  uint32_t blink_interval_ms_{3000};
  uint32_t look_interval_ms_{2500};
  uint32_t behavior_interval_ms_{4000};
  
  // Emotion system
  std::vector<Emotion> emotions_;
  std::string current_emotion_{"normal"};
  
  // Animation state
  AnimationState state_{0.0f, 0.0f, false, 0, 1.0f, 1.0f, 0, 0, 0};
  
  // Internal helper methods
  void update_animation_state_();
  void calculate_look_direction_();
  void update_blink_();
  Emotion *get_random_emotion_();
  EyeShape calculate_left_eye_shape_();
  EyeShape calculate_right_eye_shape_();
  void draw_rounded_rectangle_(int x, int y, int w, int h, int radius, display::Color color, bool filled);
};

}  // namespace anim_eyes
}  // namespace esphome
