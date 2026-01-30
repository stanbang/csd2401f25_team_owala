/*****************************************************************//**
* \file   SerializationKeys.h
*
* \primary_author
*       Jason Lou
* \secondary_author
*
*
* \brief    This file is a centralized list of static strings that serves
*			as JSON keys.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "SerializationKeys.h"

namespace Framework::Serialization {
	/* Generic Scene JSON Labels */
	static constexpr const char* SCENE_NAME = "scene_name";
	static constexpr const char* ENTITIES = "entities";
	static constexpr const char* SCENE_ASSET_NAME = "name";
	static constexpr const char* SCENE_ASSET_PATH = "path";

	/* Generic Time JSON Labels */
	static constexpr const char* ELAPSED_TIME = "elapsed_time";

	/* Generic Vector2D JSON Labels */
	static constexpr const char* VEC2_X = "x";
	static constexpr const char* VEC2_Y = "y";

	/* Generic Color JSON Labels */
	static constexpr const char* COLOR = "rgba";

	/* Entity Properties JSON Labels */
	inline constexpr const char* ENTITY_NAME = "name";
	static constexpr const char* ENTITY_LAYER = "layer";
	static constexpr const char* ENTITY_COLLISION_LAYER_NAME = "layer_collision_name";
	static constexpr const char* ENTITY_RENDER_LAYER = "render_layer";          
	static constexpr const char* ENTITY_CUSTOM_RENDER_ORDER = "custom_render_order"; 
	static constexpr const char* ENTITY_IS_ACTIVE = "active";
	static constexpr const char* ENTITY_RENDER_LAYER_NAME = "layer_render_name";

	/* Layer Configuration Keys */
	static constexpr const char* RENDER_LAYERS = "RenderLayers";
	static constexpr const char* COLLISION_LAYERS = "CollisionLayers";
	static constexpr const char* LAYER_NAME = "name";
	static constexpr const char* LAYER_RENDER_ORDER = "renderOrder";
	static constexpr const char* LAYER_ID = "id";
	static constexpr const char* LAYER_IS_BUILTIN = "isBuiltIn";
	static constexpr const char* LAYER_IS_VISIBLE = "isVisible";

	/* Component JSON Labels */
	static constexpr const char* COMPONENTS = "components";

	static constexpr const char* TRANSFORM_COMPONENT = "Transform";
	static constexpr const char* TRANSFORM_POSITION = "Position";
	static constexpr const char* TRANSFORM_ROTATION = "Rotation";
	static constexpr const char* ROTATION_ANGLE = "angle";
	static constexpr const char* ROTATION_TEXTURE_ANGLE = "texture_angle";
	static constexpr const char* ROTATION_COLLIDER_ANGLE = "collider_angle";
	static constexpr const char* ROTATION_TEXTURE_MTX = "texture_matrix";
	static constexpr const char* ROTATION_COLLIDER_MTX = "collider_matrix";
	static constexpr const char* TRANSFORM_SCALE = "Scale";

	static constexpr const char* SPAWN_PT_COMPONENT = "SpawnPoint";
	static constexpr const char* VELOCITY_COMPONENT = "Velocity";
	static constexpr const char* ACCELERATION_COMPONENT = "Acceleration";

	static constexpr const char* TRIANGLE_COMPONENT = "Triangle";
	static constexpr const char* VERTEX_A = "vertex_a";
	static constexpr const char* VERTEX_B = "vertex_b";
	static constexpr const char* VERTEX_C = "vertex_c";

	static constexpr const char* RIGIDBODY_COMPONENT = "RigidBody";
	static constexpr const char* MASS = "mass";
	static constexpr const char* INVERSE_MASS = "inv_mass";
	static constexpr const char* FORCE = "force";
	
	static constexpr const char* AABB_COLLIDER_COMPONENT = "AABBCollider";
	static constexpr const char* HALF_WIDTH = "half_width";
	static constexpr const char* HALF_HEIGHT = "half_height";
	static constexpr const char* OFFSET_X = "offset_x";
	static constexpr const char* OFFSET_Y = "offset_y";

	static constexpr const char* PLAYER_STATE_COMPONENT = "PlayerState";
	static constexpr const char* PLAYER_IS_FACING_RIGHT = "is_facing_right";
	static constexpr const char* PLAYER_IS_GROUNDED = "is_grounded";

	static constexpr const char* ROTATIONFLAG_COMPONENT = "RotationFlag";
	static constexpr const char* IS_ROTATING = "is_rotating";
	static constexpr const char* CURRENT_ROTATION = "current_rotation";
	static constexpr const char* TARGET_ROTATION = "target_rotation";

	static constexpr const char* TRAP_PLATFORM_COMPONENT = "TrapPlatform";
	static constexpr const char* IS_TRIGGERED = "is_triggered";
	static constexpr const char* HAS_FALLEN = "has_fallen";
	static constexpr const char* TRIGGER_DELAY = "trigger_delay";
	static constexpr const char* CURRENT_DELAY = "current_delay";
	static constexpr const char* FALL_SPEED = "fall_speed";
	static constexpr const char* ORIGINAL_POSITION = "original_position";
	//static constexpr const char* CURRENT_FALL_DISTANCE = "current_fall_distance";

	static constexpr const char* SCALING_PLATFORM_COMPONENT = "ScalingPlatform";
	static constexpr const char* BASE_HALFSIZE = "base_half_size";
	static constexpr const char* CURRENT_SCALE = "current_scale";
	static constexpr const char* SCALE_SPEED = "scale_speed";
	static constexpr const char* MIN_SCALE = "min_scale";
	static constexpr const char* MAX_SCALE = "max_scale";
	static constexpr const char* TRANSFORM_MATRIX = "transform_matrix";

	static constexpr const char* TEXTURE_COMPONENT = "TextureComponent";
	static constexpr const char* TEXTURE_KEY = "texture_key";
	static constexpr const char* HALF_SIZE = "half_size";
	static constexpr const char* TEXTURE_FLIP_X = "flip_x";
	
	static constexpr const char* AUDIO_COMPONENT = "AudioComponent";
	static constexpr const char* SOUND_KEY = "sound_key";
	static constexpr const char* SHOULD_LOOP = "should_loop";
	static constexpr const char* IS_STREAMING = "is_streaming";
	static constexpr const char* VOLUME = "volume";

	static constexpr const char* ANIMATION_COMPONENT = "AnimationComponent";
	static constexpr const char* ANIMATION_PROFILE = "profile_name";
	static constexpr const char* CURRENT_STATE = "current_state";
	static constexpr const char* CURRENT_FRAME = "current_frame";
	static constexpr const char* FRAME_DURATION = "frame_duration";
	static constexpr const char* START_FRAME = "start_frame";
	static constexpr const char* FRAME_COUNT = "frame_count";
	static constexpr const char* IS_PLAYING = "is_playing";
	static constexpr const char* IS_LOOPING = "is_looping";

	static constexpr const char* TEXT_COMPONENT = "TextComponent";
	static constexpr const char* TEXT = "text";
	static constexpr const char* FONT_KEY = "font_key";
	static constexpr const char* FONT_COLOR = "font_color";
	static constexpr const char* FONT_SIZE = "font_size";
	static constexpr const char* MAX_WIDTH = "max_width";

	static constexpr const char* CAMERA_COMPONENT = "Camera";
	static constexpr const char* POSITION = "position";
	static constexpr const char* WIDTH = "width";
	static constexpr const char* HEIGHT = "height";
	static constexpr const char* SIZE = "size";
	static constexpr const char* ZOOM = "zoom";
	static constexpr const char* TARGET_ENTITY = "target_entity";
	static constexpr const char* FOLLOW_SPEED = "follow_speed";
	static constexpr const char* IS_ACTIVE = "is_active";

	static constexpr const char* NATIVE_SCRIPTS_COMPONENT = "NativeScripts";
	static constexpr const char* SCRIPTS = "scripts";
	static constexpr const char* SCRIPT_TYPE = "type";
	static constexpr const char* SCRIPT_ENABLED = "enabled";
	static constexpr const char* SCRIPT_EXEC_IN_EDIT = "execute_in_edit";

	static constexpr const char* BUTTON_COMPONENT = "ButtonComponent";
	static constexpr const char* IS_HOVERED = "is_hovered";
	static constexpr const char* IS_PRESSED = "is_pressed";
	static constexpr const char* IS_RELEASED = "is_released";
	static constexpr const char* ACTION = "action";
	static constexpr const char* TARGET_STRING = "target_string";
	static constexpr const char* TARGET_VALUE = "target_value";
	static constexpr const char* HOVER_SOUND_KEY = "hover_sound_key";
	static constexpr const char* HOVER_SOUND_VOLUME = "hover_sound_volume";
	static constexpr const char* CLICK_SOUND_KEY = "click_sound_key";
	static constexpr const char* CLICK_SOUND_VOLUME = "click_sound_volume";
	static constexpr const char* BUTTON_DEFAULT_TEXTURE = "default_texture_key";
	static constexpr const char* BUTTON_HOVER_TEXTURE = "hover_texture_key";
	static constexpr const char* BUTTON_PRESSED_TEXTURE = "pressed_texture_key";
	static constexpr const char* BUTTON_DEFAULT_ANIMATED = "default_is_animated";
	static constexpr const char* BUTTON_HOVER_ANIMATED = "hover_is_animated";
	static constexpr const char* BUTTON_PRESSED_ANIMATED = "pressed_is_animated";

	static constexpr const char* SCENE_OWNED_COMPONENT = "SceneOwned";
	static constexpr const char* COMP_SCENE_ID = "scene_id";
	static constexpr const char* COMP_SCENE_NAME = "sceneName";

	static constexpr const char* PARTICLE_EMITTER_COMPONENT = "ParticleEmitterComponent";
	static constexpr const char* IS_EMITTING = "is_emitting";
	static constexpr const char* IS_PARTICLE_ANIMATED = "is_particle_animated";
	static constexpr const char* IS_SCREEN_SPACE = "is_screen_space";
	static constexpr const char* PARTICLE_TEXTURE_KEY = "particle_texture_key";
	static constexpr const char* PARTICLE_LIFETIME = "particle_lifetime";
	static constexpr const char* EMISSION_RATE = "emit_rate";
	static constexpr const char* EMISSION_INTERVAL = "emit_interval";
	static constexpr const char* PARTICLE_EFFECT_TYPE = "particle_effect_type";
	static constexpr const char* EFFECT_STRENGTH = "effect_strength";
	static constexpr const char* PARTICLE_SPEED = "particle_speed";
	static constexpr const char* PARTICLE_SIZE = "particle_size";
	static constexpr const char* IS_AFFECTED_BY_GRAVITY = "is_affected_by_gravity";
	static constexpr const char* EMISSION_STYLE = "emission_style";
	static constexpr const char* EMISSION_RADIUS = "emission_radius";
	static constexpr const char* PARTICLE_START_VELOCITY = "particle_start_velocity";
	static constexpr const char* PARTICLE_ACCELERATION = "particle_acceleration";
	static constexpr const char* LINE_START = "line_start";
	static constexpr const char* LINE_END = "line_end";
	static constexpr const char* CIRCLE_RADIUS = "circle_radius";
	static constexpr const char* VIEWPORT_MIN = "viewport_min";
	static constexpr const char* VIEWPORT_MAX = "viewport_max";
	static constexpr const char* EMISSION_DIRECTION = "emission_direction";
	static constexpr const char* EMISSION_OFFSET = "emission_offset";
	static constexpr const char* TRIGGER_EVENT_NAME = "trigger_event_name";
	static constexpr const char* EMISSION_MODE = "emission_mode";
	static constexpr const char* BURST_COUNT = "burst_count";
	static constexpr const char* BURST_INTERVAL = "burst_interval";

	static constexpr const char* SLIDER = "Slider";
	static constexpr const char* SLIDER_CURRENT_VALUE = "current_value";
	static constexpr const char* SLIDER_HANDLE_NAME = "handle_name";
	static constexpr const char* SLIDER_MIN_VALUE = "min_value";
	static constexpr const char* SLIDER_MAX_VALUE = "max_value";
	static constexpr const char* SLIDER_WIDTH = "width";
	static constexpr const char* SLIDER_WIDTH_OFFSET_X = "width_offset_x";
	static constexpr const char* SLIDER_IS_DRAGGING = "is_dragging";

	//// bird AI Keys
	//static constexpr const char* BIRD_AI_COMPONENT = "BirdAIComponent";
	//static constexpr const char* AI_STATE = "state";
	//static constexpr const char* IS_FACING_RIGHT = "is_facing_right";  
	//static constexpr const char* STATE_TIMER = "state_timer";
	//static constexpr const char* WAKEUP_RADIUS = "wakeup_radius";
	//static constexpr const char* WAKEUP_TIME = "wakeup_time";
	//static constexpr const char* MOVE_SPEED = "move_speed";
	//static constexpr const char* HOVER_RADIUS = "hover_radius";
	//static constexpr const char* HOVER_HEIGHT = "hover_height";
	//static constexpr const char* HOVER_SPEED = "hover_speed";
	//static constexpr const char* HOVER_NODE_REACHED_DIST = "hover_node_reached_dist";
	//static constexpr const char* CURRENT_HOVER_NODE = "current_hover_node";
	//static constexpr const char* ATTACK_RADIUS = "attack_radius";
	//static constexpr const char* ATTACK_SPEED = "attack_speed";
	//static constexpr const char* ATTACK_DURATION = "attack_duration";
	//static constexpr const char* ATTACK_COOLDOWN = "attack_cooldown";
	//static constexpr const char* ATTACK_TIMER = "attack_timer";
	//static constexpr const char* ATTACK_TARGET_POSITION = "attack_target_position";
	//static constexpr const char* PATH_RECALCULATE_INTERVAL = "path_recalculate_interval";
	//static constexpr const char* PATH_RECALCULATE_TIMER = "path_recalculate_timer";
	//static constexpr const char* CHASE_RADIUS = "chase_radius";
	//static constexpr const char* STUN_DURATION = "stun_duration";

	//// Skeleton AI Keys
	//static constexpr const char* SKELETON_AI_COMPONENT = "SkeletonAIComponent";
	//// AI_STATE = "state" (re-use from Bird)
	//// MOVE_SPEED = "move_speed" (re-use from Bird)
	//static constexpr const char* DETECTION_RADIUS = "detection_radius";

	// FSM KEYS
	static constexpr const char* FSM_COMPONENT = "FSMComponent";
	static constexpr const char* FSM_DEFINITION_PATH = "DefinitionPath";
	static constexpr const char* FSM_CURRENT_STATE = "CurrentState";
	static constexpr const char* FSM_INITIAL_STATE = "InitialState";
	
	/* Tags JSON Labels */
	static constexpr const char* TAGS = "tags";
	static constexpr const char* PLAYER_TAG = "PlayerTag";
	static constexpr const char* PLATFORM_TAG = "PlatformTag";
	static constexpr const char* BACKGROUND_TAG = "BackgroundTag";
	static constexpr const char* ENEMY_TAG = "EnemyTag";
	static constexpr const char* GUI_TAG = "GUITag";
	static constexpr const char* PERSISTENT_TAG = "PersistentTag";
	static constexpr const char* ACTIVE_TAG = "ActiveTag";
}