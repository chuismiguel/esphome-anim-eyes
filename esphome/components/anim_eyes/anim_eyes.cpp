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
  state_.eyelid_top = 1.0f;
  state_.eyelid_bottom = 1.0f;
  state_.last_blink_time = 0;
  state_.last_look_time = 0;
  state_.last_behavior_time = 0;
  
  // Register periodic animation callback
  set_interval("anim_eyes", update_interval_ms_, [this]() { animate_(); });
}

void AnimEyes::animate_() {
  if (display_ == nullptr) {
    return;
  }
  
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


void AnimEyes::draw_eye_(const EyeShape &eye) {
  if (display_ == nullptr) {
    return;
  }
  
  // Draw eye white background (rounded rectangle)
  draw_rounded_rectangle_(eye.center_x - eye.width / 2, 
                         eye.center_y - eye.height / 2,
                         eye.width, eye.height,
                         eye.border_radius,
                         display::COLOR_ON, true);
  
  // Apply eyelid effect (upper and lower eyelids closing in during blink)
  float eyelid_top_pos = eye.border_radius + (eye.height / 2 - eye.border_radius) * (1.0f - state_.eyelid_top);
  float eyelid_bottom_pos = eye.border_radius + (eye.height / 2 - eye.border_radius) * (1.0f - state_.eyelid_bottom);
  
  // Draw top eyelid (black bar covering from top)
  if (state_.eyelid_top < 1.0f) {
    display_->filled_rectangle(eye.center_x - eye.width / 2,
                              eye.center_y - eye.height / 2,
                              eye.width,
                              (int)eyelid_top_pos,
                              display::COLOR_OFF);
  }
  
  // Draw bottom eyelid (black bar covering from bottom)
  if (state_.eyelid_bottom < 1.0f) {
    int bottom_y = eye.center_y + eye.height / 2 - (int)eyelid_bottom_pos;
    display_->filled_rectangle(eye.center_x - eye.width / 2,
                              bottom_y,
                              eye.width,
                              (int)eyelid_bottom_pos,
                              display::COLOR_OFF);
  }
  
  // Calculate pupil position based on look direction
  int pupil_x = eye.center_x + (state_.look_x * (eye.width / 3));
  int pupil_y = eye.center_y + (state_.look_y * (eye.height / 3));
  
  // Draw pupil (small circle)
  int pupil_radius = eye.width / 8;
  if (pupil_radius > 1) {
    display_->filled_circle(pupil_x, pupil_y, pupil_radius, display::COLOR_OFF);
  }
}

void AnimEyes::draw_eyes() {
  if (display_ == nullptr) {
    return;
  }
  
  // Calculate eye positions and shapes
  EyeShape left_eye = calculate_left_eye_shape_();
  EyeShape right_eye = calculate_right_eye_shape_();
  
  // Draw both eyes
  draw_eye_(left_eye);
  draw_eye_(right_eye);
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
  uint32_t now = esp_timer_get_time() / 1000;  // Convert microseconds to milliseconds
  uint32_t elapsed = now - state_.last_blink_time;
  
  if (elapsed >= BLINK_DURATION) {
    state_.is_blinking = false;
    state_.blink_progress = 0;
    state_.eyelid_top = 1.0f;
    state_.eyelid_bottom = 1.0f;
  } else {
    // Calculate blink progress (0-1, where 0.5 is fully closed)
    float progress = elapsed / (float)BLINK_DURATION;
    
    // Make eyelids close smoothly: 1->0->1 over the duration (sine wave)
    float eyelid_open = std::abs(std::cos(progress * 3.14159f));
    state_.eyelid_top = eyelid_open;
    state_.eyelid_bottom = eyelid_open;
    
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

EyeShape AnimEyes::calculate_left_eye_shape_() {
  uint16_t display_width = display_->get_width();
  uint16_t display_height = display_->get_height();
  
  EyeShape eye;
  eye.center_x = (display_width / 4);  // Left quarter of display
  eye.center_y = (display_height / 2);
  eye.width = eye_size_;
  eye.height = eye_size_;
  eye.border_radius = eye_size_ / 6;  // Rounded corners
  
  return eye;
}

EyeShape AnimEyes::calculate_right_eye_shape_() {
  uint16_t display_width = display_->get_width();
  uint16_t display_height = display_->get_height();
  
  EyeShape eye;
  eye.center_x = (3 * display_width / 4);  // Right quarter of display
  eye.center_y = (display_height / 2);
  eye.width = eye_size_;
  eye.height = eye_size_;
  eye.border_radius = eye_size_ / 6;  // Rounded corners
  
  return eye;
}

void AnimEyes::draw_rounded_rectangle_(int x, int y, int w, int h, int radius, display::Color color, bool filled) {
  if (!filled) {
    // Draw outline with rounded corners (simple approach with rectangles and circles)
    display_->rectangle(x, y, w, h, color);
    return;
  }
  
  // Draw filled rounded rectangle
  // This is a simplified version using filled rectangle in the middle
  // and filled circles at corners
  if (radius > 0) {
    // Main body rectangle
    display_->filled_rectangle(x + radius, y, w - 2 * radius, h, color);
    display_->filled_rectangle(x, y + radius, w, h - 2 * radius, color);
    
    // Four corner circles to create rounded effect
    display_->filled_circle(x + radius, y + radius, radius, color);
    display_->filled_circle(x + w - radius, y + radius, radius, color);
    display_->filled_circle(x + radius, y + h - radius, radius, color);
    display_->filled_circle(x + w - radius, y + h - radius, radius, color);
  } else {
    // No rounding, just a filled rectangle
    display_->filled_rectangle(x, y, w, h, color);
  }
}

}  // namespace anim_eyes
}  // namespace esphome
