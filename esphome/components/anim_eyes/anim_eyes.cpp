#include "anim_eyes.h"
#include "esphome/core/log.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include "esp_random.h"
#include "esp_timer.h"

namespace esphome {
namespace anim_eyes {

static const char *const TAG = "anim_eyes";

// Blink animation speed (ms)
static const uint32_t BLINK_DURATION = 150;

// Look smoothing
static const float LOOK_SMOOTHING = 0.1f;

void AnimEyes::setup() {
  if (display_ == nullptr) {
    ESP_LOGE(TAG, "Display not configured!");
    return;
  }
  
  ESP_LOGI(TAG, "AnimEyes setup complete");
  
  // Initialize with default emotion if not already set
  if (emotions_.empty()) {
    add_emotion("normal", 1.5f);
    add_emotion("happy", 1.2f);
    add_emotion("sad", 0.6f);
    add_emotion("angry", 0.4f);
    add_emotion("surprised", 0.8f);
    add_emotion("fearful", 0.3f);
    add_emotion("content", 1.0f);
    current_emotion_ = "normal";
  }
  
  // Initialize animation state
  state_.look_x = 0.0f;
  state_.look_y = 0.0f;
  state_.is_blinking = false;
  state_.blink_progress = 0;
  state_.last_blink_time = 0;
  state_.last_look_time = 0;
  state_.last_behavior_time = 0;
}

void AnimEyes::update() {
  if (display_ == nullptr) {
    return;
  }
  
  uint32_t now = esp_timer_get_time() / 1000;  // Convert microseconds to milliseconds
  
  // Check if it's time to update (respecting update_interval_ms)
  if (now - last_update_time_ < update_interval_ms_) {
    return;
  }
  last_update_time_ = now;
  
  // Update animation states
  update_animation_state_();
  
  // Clear and redraw
  clear_display();
  draw_eyes();
  display_->update();
}

void AnimEyes::dump_config() {
  ESP_LOGCONFIG(TAG, "AnimEyes:");
  ESP_LOGCONFIG(TAG, "  Eye Size: %d", eye_size_);
  ESP_LOGCONFIG(TAG, "  Eye Distance: %d", eye_distance_);
  ESP_LOGCONFIG(TAG, "  Update Interval: %d ms (~%.1f FPS)", 
                update_interval_ms_, 1000.0f / update_interval_ms_);
  ESP_LOGCONFIG(TAG, "  Random Behavior: %s", random_behavior_ ? "true" : "false");
  ESP_LOGCONFIG(TAG, "  Random Blink: %s", random_blink_ ? "true" : "false");
  ESP_LOGCONFIG(TAG, "  Random Look: %s", random_look_ ? "true" : "false");
  ESP_LOGCONFIG(TAG, "  Blink Interval: %d ms", blink_interval_ms_);
  ESP_LOGCONFIG(TAG, "  Look Interval: %d ms", look_interval_ms_);
  ESP_LOGCONFIG(TAG, "  Behavior Interval: %d ms", behavior_interval_ms_);
  ESP_LOGCONFIG(TAG, "  Current Emotion: %s", current_emotion_.c_str());
  ESP_LOGCONFIG(TAG, "  Emotions configured: %d", emotions_.size());
}

void AnimEyes::add_emotion(const std::string &name, float weight) {
  emotions_.emplace_back(name, weight);
  ESP_LOGV(TAG, "Added emotion: %s with weight %.2f", name.c_str(), weight);
}

void AnimEyes::set_current_emotion(const std::string &name) {
  // Check if emotion exists
  bool found = false;
  for (const auto &emotion : emotions_) {
    if (emotion.name == name) {
      found = true;
      break;
    }
  }
  
  if (found) {
    current_emotion_ = name;
    ESP_LOGD(TAG, "Emotion changed to: %s", name.c_str());
  } else {
    ESP_LOGW(TAG, "Emotion not found: %s", name.c_str());
  }
}

void AnimEyes::trigger_blink() {
  if (!state_.is_blinking) {
    state_.is_blinking = true;
    state_.blink_progress = 0;
    state_.last_blink_time = esp_timer_get_time() / 1000;  // Convert to milliseconds
    ESP_LOGV(TAG, "Blink triggered");
  }
}

void AnimEyes::trigger_look() {
  if (random_look_) {
    calculate_look_direction_();
    state_.last_look_time = esp_timer_get_time() / 1000;  // Convert to milliseconds
    ESP_LOGV(TAG, "Look triggered");
  }
}

void AnimEyes::trigger_behavior_change() {
  if (random_behavior_) {
    Emotion *emotion = get_random_emotion_();
    if (emotion != nullptr) {
      set_current_emotion(emotion->name);
    }
    state_.last_behavior_time = esp_timer_get_time() / 1000;  // Convert to milliseconds
    ESP_LOGV(TAG, "Behavior changed");
  }
}

void AnimEyes::draw_eyes() {
  if (display_ == nullptr) {
    return;
  }
  
  // Get display dimensions
  uint16_t display_width = display_->get_width();
  uint16_t display_height = display_->get_height();
  
  // Calculate eye positions
  EyePosition left_eye = calculate_eye_center_(true);
  EyePosition right_eye = calculate_eye_center_(false);
  
  // Draw left eye
  draw_eye_(left_eye.x, left_eye.y);
  
  // Draw right eye
  draw_eye_(right_eye.x, right_eye.y);
}

void AnimEyes::clear_display() {
  if (display_ != nullptr) {
    display_->clear();
  }
}

void AnimEyes::update_animation_state_() {
  uint32_t now = esp_timer_get_time() / 1000;  // Convert microseconds to milliseconds
  
  // Update blink
  if (state_.is_blinking) {
    update_blink_();
  }
  
  // Trigger blink randomly or at interval
  if (random_blink_ && (now - state_.last_blink_time >= blink_interval_ms_)) {
    if ((esp_random() % 100) < 30) {  // 30% chance to blink
      trigger_blink();
    }
  }
  
  // Trigger look randomly or at interval
  if (random_look_ && (now - state_.last_look_time >= look_interval_ms_)) {
    if ((esp_random() % 100) < 40) {  // 40% chance to look around
      trigger_look();
    }
  }
  
  // Trigger behavior change randomly or at interval
  if (random_behavior_ && (now - state_.last_behavior_time >= behavior_interval_ms_)) {
    if ((esp_random() % 100) < 50) {  // 50% chance to change emotion
      trigger_behavior_change();
    }
  }
}

void AnimEyes::update_blink_() {
  uint32_t now = millis();
  uint32_t elapsed = now - state_.last_blink_time;
  
  if (elapsed >= BLINK_DURATION) {
    state_.is_blinking = false;
    state_.blink_progress = 0;
  } else {
    // Calculate blink progress (0-255, where 127-128 is fully closed)
    state_.blink_progress = (elapsed * 255) / BLINK_DURATION;
  }
}

void AnimEyes::calculate_look_direction_() {
  // Generate random look direction
  // Range: -1.0 to 1.0 for both x and y
  state_.look_x = ((esp_random() % 200) - 100) / 100.0f;
  state_.look_y = ((esp_random() % 200) - 100) / 100.0f;
  
  // Clamp to valid range
  state_.look_x = std::max(-1.0f, std::min(1.0f, state_.look_x));
  state_.look_y = std::max(-1.0f, std::min(1.0f, state_.look_y));
}

Emotion *AnimEyes::get_random_emotion_() {
  if (emotions_.empty()) {
    return nullptr;
  }
  
  // Use weighted random selection
  float total_weight = 0.0f;
  for (const auto &emotion : emotions_) {
    total_weight += emotion.weight;
  }
  
  float random_value = ((esp_random() % 10000) / 10000.0f) * total_weight;
  float accumulated = 0.0f;
  
  for (auto &emotion : emotions_) {
    accumulated += emotion.weight;
    if (random_value <= accumulated) {
      return &emotion;
    }
  }
  
  return &emotions_.back();
}

EyePosition AnimEyes::calculate_eye_center_(bool is_left) {
  uint16_t display_width = display_->get_width();
  uint16_t display_height = display_->get_height();
  
  EyePosition pos;
  
  if (is_left) {
    pos.x = (display_width / 2 - eye_distance_) / 2;
  } else {
    pos.x = (display_width / 2 + eye_distance_) + (display_width / 2 - eye_distance_) / 2;
  }
  
  pos.y = display_height / 2;
  
  return pos;
}

void AnimEyes::draw_eye_(int center_x, int center_y) {
  if (display_ == nullptr) {
    return;
  }
  
  // Draw eye white (circle)
  display_->filled_circle(center_x, center_y, eye_size_ / 2, display::COLOR_ON);
  
  // Calculate pupil position based on look direction
  int pupil_x = center_x + (state_.look_x * (eye_size_ / 4));
  int pupil_y = center_y + (state_.look_y * (eye_size_ / 4));
  
  // Apply blink effect (shrink pupil vertically)
  int pupil_size = eye_size_ / 4;
  float blink_factor = 1.0f;
  
  if (state_.is_blinking) {
    // Blink progress goes from 0 to 255, where 127-128 is fully closed
    float progress = state_.blink_progress / 255.0f;
    
    // Make blink smooth: 0->1->0 over the duration
    float eased = std::abs(std::sin(progress * 3.14159f));
    blink_factor = 1.0f - (eased * 0.9f);  // Keep slight opening
  }
  
  int pupil_height = pupil_size * blink_factor;
  
  // Draw pupil
  if (pupil_height > 1) {
    display_->filled_circle(pupil_x, pupil_y, pupil_x > center_x ? pupil_size / 2 : pupil_size / 2, 
                           display::COLOR_OFF);
  }
}

}  // namespace anim_eyes
}  // namespace esphome
