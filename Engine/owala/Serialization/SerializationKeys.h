#pragma once

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

	/* Component JSON Labels */
	static constexpr const char* COMPONENTS = "components";

	static constexpr const char* POSITION_COMPONENT = "Position";
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

	static constexpr const char* PLAYER_STATE_COMPONENT = "PlayerState";

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
	static constexpr const char* CURRENT_FALL_DISTANCE = "current_fall_distance";

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
	
	static constexpr const char* AUDIO_COMPONENT = "AudioComponent";
	static constexpr const char* SOUND_KEY = "sound_key";
	static constexpr const char* SHOULD_LOOP = "should_loop";
	static constexpr const char* IS_STREAMING = "is_streaming";
	static constexpr const char* VOLUME = "volume";

	static constexpr const char* ANIMATION_COMPONENT = "AnimationComponent";
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

	static constexpr const char* CAMERA_COMPONENT = "Camera";
	static constexpr const char* POSITION = "position";
	static constexpr const char* WIDTH = "width";
	static constexpr const char* HEIGHT = "height";
	static constexpr const char* SIZE = "size";
	static constexpr const char* ZOOM = "zoom";
	static constexpr const char* TARGET_ENTITY = "target_entity";
	static constexpr const char* FOLLOW_SPEED = "follow_speed";
	static constexpr const char* IS_ACTIVE = "is_active";

	static constexpr const char* SCENE_OWNED_COMPONENT = "SceneOwned";
	static constexpr const char* COMP_SCENE_NAME = "sceneName";
	
	/* Tags JSON Labels */
	static constexpr const char* TAGS = "tags";
	static constexpr const char* PLAYER_TAG = "PlayerTag";
	static constexpr const char* PLATFORM_TAG = "PlatformTag";
	static constexpr const char* BACKGROUND_TAG = "BackgroundTag";
	static constexpr const char* ENEMY_TAG = "EnemyTag";
	static constexpr const char* PERSISTENT_TAG = "PersistentTag";
	static constexpr const char* ACTIVE_TAG = "ActiveTag";
}