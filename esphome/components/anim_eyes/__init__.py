"""Animation Eyes custom component for ESPHome."""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import display
from esphome.const import CONF_ID, CONF_UPDATE_INTERVAL

# Namespaces
anim_eyes_ns = cg.esphome_ns.namespace("anim_eyes")
AnimEyes = anim_eyes_ns.class_("AnimEyes", cg.Component)
Emotion = anim_eyes_ns.class_("Emotion")

# Dependencies
DEPENDENCIES = ["display"]
MULTI_CONF = True

# Configuration constants
CONF_DISPLAY = "display"
CONF_EYE_SIZE = "eye_size"
CONF_EYE_DISTANCE = "eye_distance"
CONF_RANDOM_BEHAVIOR = "random_behavior"
CONF_RANDOM_BLINK = "random_blink"
CONF_RANDOM_LOOK = "random_look"
CONF_BLINK_INTERVAL = "blink_interval"
CONF_LOOK_INTERVAL = "look_interval"
CONF_BEHAVIOR_INTERVAL = "behavior_interval"
CONF_EMOTIONS = "emotions"
CONF_NAME = "name"
CONF_WEIGHT = "weight"

# Emotion configuration schema
EMOTION_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_NAME): cv.string,
        cv.Optional(CONF_WEIGHT, default=1.0): cv.positive_float,
    }
)

# Main component configuration schema
CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(AnimEyes),
        cv.Required(CONF_DISPLAY): cv.use_id(display.DisplayBuffer),
        cv.Optional(CONF_EYE_SIZE, default=22): cv.positive_int,
        cv.Optional(CONF_EYE_DISTANCE, default=6): cv.positive_int,
        cv.Optional(CONF_UPDATE_INTERVAL, default=50): cv.positive_int,
        cv.Optional(CONF_RANDOM_BEHAVIOR, default=True): cv.boolean,
        cv.Optional(CONF_RANDOM_BLINK, default=True): cv.boolean,
        cv.Optional(CONF_RANDOM_LOOK, default=True): cv.boolean,
        cv.Optional(CONF_BLINK_INTERVAL, default=3000): cv.positive_int,
        cv.Optional(CONF_LOOK_INTERVAL, default=2500): cv.positive_int,
        cv.Optional(CONF_BEHAVIOR_INTERVAL, default=4000): cv.positive_int,
        cv.Optional(CONF_EMOTIONS, default=[]): cv.ensure_list(EMOTION_SCHEMA),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    """Generate C++ code for the AnimEyes component."""
    
    # Create the component variable
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    # Get display reference
    display_buffer = await cg.get_variable(config[CONF_DISPLAY])
    cg.add(var.set_display(display_buffer))
    
    # Set eye dimensions
    cg.add(var.set_eye_size(config[CONF_EYE_SIZE]))
    cg.add(var.set_eye_distance(config[CONF_EYE_DISTANCE]))
    
    # Set rendering performance
    cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))
    
    # Set behavior toggles
    cg.add(var.set_random_behavior(config[CONF_RANDOM_BEHAVIOR]))
    cg.add(var.set_random_blink(config[CONF_RANDOM_BLINK]))
    cg.add(var.set_random_look(config[CONF_RANDOM_LOOK]))
    
    # Set timing intervals
    cg.add(var.set_blink_interval(config[CONF_BLINK_INTERVAL]))
    cg.add(var.set_look_interval(config[CONF_LOOK_INTERVAL]))
    cg.add(var.set_behavior_interval(config[CONF_BEHAVIOR_INTERVAL]))
    
    # Add emotions if defined
    emotions = config[CONF_EMOTIONS]
    if emotions:
        for emotion in emotions:
            emotion_name = emotion[CONF_NAME]
            emotion_weight = emotion[CONF_WEIGHT]
            cg.add(var.add_emotion(emotion_name, emotion_weight))
    else:
        # Add default emotions if none specified
        default_emotions = [
            ("normal", 1.5),
            ("happy", 1.2),
            ("sad", 0.6),
            ("angry", 0.4),
            ("surprised", 0.8),
            ("fearful", 0.3),
            ("content", 1.0),
        ]
        for emotion_name, emotion_weight in default_emotions:
            cg.add(var.add_emotion(emotion_name, emotion_weight))
    
    return var
