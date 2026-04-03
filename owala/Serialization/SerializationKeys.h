/*********************************************************************//**
 * \file   SerializationKeys.h
 *
 * \primary_author
 *      Jason Lou (90%)
 * \secondary_author
 *      Ng Jing Wen (10%)
 *
 * \brief   Centralized list of static strings that serve as JSON keys for
 *          serialization and deserialization of game data.
 *
 * \details
 *      This header file provides a single source of truth for all string
 *      keys used in JSON serialization throughout the engine. By centralizing
 *      these keys, we ensure consistency across all serialization operations
 *      and make it easier to update or debug serialization code.
 *
 *      The keys are organized into logical categories:
 *        - Generic scene keys
 *        - Time and vector keys
 *        - Entity properties
 *        - Layer configuration
 *        - Component-specific keys (Transform, RigidBody, Colliders, etc.)
 *        - Tag identifiers
 *        - FSM (Finite State Machine) keys
 *
 *      Example Usage:
 *      \code
 *      // Writing JSON
 *      writer.Key(Serialization::ENTITY_NAME);
 *      writer.String(entityName.c_str());
 *
 *      // Reading JSON
 *      if (doc.HasMember(Serialization::TRANSFORM_COMPONENT)) {
 *          const auto& transform = doc[Serialization::TRANSFORM_COMPONENT];
 *          pos.x = transform[Serialization::VEC2_X].GetFloat();
 *          pos.y = transform[Serialization::VEC2_Y].GetFloat();
 *      }
 *      \endcode
 *
 * \note
 *      Most keys are constexpr for compile-time optimization. Some keys
 *      are marked inline constexpr to allow definition in header without
 *      ODR violations.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without the
 *      prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include "SerializationKeys.h"

namespace Framework::Serialization {
    /* ======================================================================
     * Generic Scene JSON Labels
     * ====================================================================== */
    static constexpr const char* SCENE_NAME = "scene_name";           //!< Name of the scene
    static constexpr const char* ENTITIES = "entities";               //!< Array of entities in scene
    static constexpr const char* PRELOAD_TEXTURES = "preload_textures"; //!< Optional per-scene texture preload keys
    static constexpr const char* SCENE_ASSET_NAME = "name";           //!< Name of scene asset
    static constexpr const char* SCENE_ASSET_PATH = "path";           //!< File path to scene asset

    /* ======================================================================
     * Generic Time JSON Labels
     * ====================================================================== */
    static constexpr const char* ELAPSED_TIME = "elapsed_time";       //!< Time elapsed (for timers)

    /* ======================================================================
     * Generic Vector2D JSON Labels
     * ====================================================================== */
    static constexpr const char* VEC2_X = "x";                        //!< X component of vector
    static constexpr const char* VEC2_Y = "y";                        //!< Y component of vector

    /* ======================================================================
     * Generic Color JSON Labels
     * ====================================================================== */
    static constexpr const char* COLOR = "rgba";                      //!< RGBA color array

    /* ======================================================================
     * Entity Properties JSON Labels
     * ====================================================================== */
    inline constexpr const char* ENTITY_NAME = "name";                //!< Display name of entity
    static constexpr const char* ENTITY_LAYER = "layer";              //!< Layer assignment
    static constexpr const char* ENTITY_COLLISION_LAYER_NAME = "layer_collision_name"; //!< Collision layer name
    static constexpr const char* ENTITY_RENDER_LAYER = "render_layer"; //!< Render layer assignment
    static constexpr const char* ENTITY_CUSTOM_RENDER_ORDER = "custom_render_order"; //!< Custom render order value
    static constexpr const char* ENTITY_IS_ACTIVE = "active";         //!< Whether entity starts active
    static constexpr const char* ENTITY_RENDER_LAYER_NAME = "layer_render_name"; //!< Render layer name
    static constexpr const char* ENTITY_UUID = "uuid";                //!< Unique identifier for entity

    /* ======================================================================
     * Layer Configuration Keys
     * ====================================================================== */
    static constexpr const char* RENDER_LAYERS = "RenderLayers";      //!< Array of render layers
    static constexpr const char* COLLISION_LAYERS = "CollisionLayers"; //!< Array of collision layers
    static constexpr const char* LAYER_NAME = "name";                 //!< Name of layer
    static constexpr const char* LAYER_RENDER_ORDER = "renderOrder";  //!< Render order for layer
    static constexpr const char* LAYER_ID = "id";                     //!< Numeric ID of layer
    static constexpr const char* LAYER_IS_BUILTIN = "isBuiltIn";      //!< Whether layer is built-in
    static constexpr const char* LAYER_IS_VISIBLE = "isVisible";      //!< Whether layer is visible

    /* ======================================================================
     * Component JSON Labels
     * ====================================================================== */
    static constexpr const char* COMPONENTS = "components";           //!< Container for all components

    /* ----- Transform Component ----- */
    static constexpr const char* TRANSFORM_COMPONENT = "Transform";   //!< Transform component
    static constexpr const char* TRANSFORM_POSITION = "Position";     //!< Position vector
    static constexpr const char* TRANSFORM_ROTATION = "Rotation";     //!< Rotation data
    static constexpr const char* ROTATION_ANGLE = "angle";            //!< Rotation angle
    static constexpr const char* ROTATION_TEXTURE_ANGLE = "texture_angle"; //!< Texture rotation angle
    static constexpr const char* ROTATION_COLLIDER_ANGLE = "collider_angle"; //!< Collider rotation angle
    static constexpr const char* ROTATION_TEXTURE_MTX = "texture_matrix"; //!< Texture transform matrix
    static constexpr const char* ROTATION_COLLIDER_MTX = "collider_matrix"; //!< Collider transform matrix
    static constexpr const char* TRANSFORM_SCALE = "Scale";           //!< Scale vector

    /* ----- Movement Components ----- */
    static constexpr const char* VELOCITY_COMPONENT = "Velocity";     //!< Velocity component
    static constexpr const char* ACCELERATION_COMPONENT = "Acceleration"; //!< Acceleration component

    /* ----- Triangle Component (Debug) ----- */
    static constexpr const char* TRIANGLE_COMPONENT = "Triangle";     //!< Triangle component
    static constexpr const char* VERTEX_A = "vertex_a";               //!< First vertex
    static constexpr const char* VERTEX_B = "vertex_b";               //!< Second vertex
    static constexpr const char* VERTEX_C = "vertex_c";               //!< Third vertex

    /* ----- RigidBody Component ----- */
    static constexpr const char* RIGIDBODY_COMPONENT = "RigidBody";   //!< RigidBody component
    static constexpr const char* MASS = "mass";                       //!< Mass value
    static constexpr const char* INVERSE_MASS = "inv_mass";           //!< Inverse mass (1/mass)
    static constexpr const char* FORCE = "force";                     //!< Force vector

    /* ----- Collider Components ----- */
    static constexpr const char* AABB_COLLIDER_COMPONENT = "AABBCollider"; //!< AABB collider
    static constexpr const char* HALF_WIDTH = "half_width";           //!< Half width of AABB
    static constexpr const char* HALF_HEIGHT = "half_height";         //!< Half height of AABB
    static constexpr const char* OFFSET_X = "offset_x";               //!< X offset from transform
    static constexpr const char* OFFSET_Y = "offset_y";               //!< Y offset from transform

    /* ----- Player Components ----- */
    static constexpr const char* PLAYER_STATE_COMPONENT = "PlayerState"; //!< Player state component
    static constexpr const char* PLAYER_IS_FACING_RIGHT = "is_facing_right"; //!< Player facing direction
    static constexpr const char* PLAYER_IS_GROUNDED = "is_grounded";   //!< Player grounded state

    static constexpr const char* PLAYER_HEALTH_COMPONENT = "PlayerHealthComponent"; //!< Player health
    static constexpr const char* PLAYER_HEALTH_NUM = "life_num";       //!< Number of lives

    /* ----- Rotation Flag Component ----- */
    static constexpr const char* ROTATIONFLAG_COMPONENT = "RotationFlag"; //!< Rotation flag component
    static constexpr const char* IS_ROTATING = "is_rotating";          //!< Whether rotation is active
    static constexpr const char* CURRENT_ROTATION = "current_rotation"; //!< Current rotation value
    static constexpr const char* TARGET_ROTATION = "target_rotation";  //!< Target rotation value

    /* ----- Trap Platform Component ----- */
    static constexpr const char* TRAP_PLATFORM_COMPONENT = "TrapPlatform"; //!< Trap platform component
    static constexpr const char* IS_TRIGGERED = "is_triggered";         //!< Whether trap is triggered
    static constexpr const char* HAS_FALLEN = "has_fallen";             //!< Whether platform has fallen
    static constexpr const char* TRIGGER_DELAY = "trigger_delay";       //!< Delay before falling
    static constexpr const char* CURRENT_DELAY = "current_delay";       //!< Current delay timer
    static constexpr const char* FALL_SPEED = "fall_speed";             //!< Speed when falling
    static constexpr const char* ORIGINAL_POSITION = "original_position"; //!< Starting position

    /* ----- Scaling Platform Component ----- */
    static constexpr const char* SCALING_PLATFORM_COMPONENT = "ScalingPlatform"; //!< Scaling platform
    static constexpr const char* BASE_HALFSIZE = "base_half_size";       //!< Base half size
    static constexpr const char* CURRENT_SCALE = "current_scale";        //!< Current scale factor
    static constexpr const char* SCALE_SPEED = "scale_speed";            //!< Speed of scaling
    static constexpr const char* MIN_SCALE = "min_scale";                //!< Minimum scale
    static constexpr const char* MAX_SCALE = "max_scale";                //!< Maximum scale
    static constexpr const char* TRANSFORM_MATRIX = "transform_matrix";  //!< Transform matrix

    /* ----- Texture Component ----- */
    static constexpr const char* TEXTURE_COMPONENT = "TextureComponent"; //!< Texture component
    static constexpr const char* TEXTURE_KEY = "texture_key";            //!< Resource key for texture
    static constexpr const char* HALF_SIZE = "half_size";                //!< Half size of sprite
    static constexpr const char* TEXTURE_FLIP_X = "flip_x";              //!< Flip horizontally
    static constexpr const char* TEXTURE_ALPHA = "alpha";                //!< Alpha transparency
    static constexpr const char* TEXTURE_OVERRIDE_UV = "override_uv";    //!< Whether to override UVs
    static constexpr const char* TEXTURE_MIN_UV = "min_uv";              //!< Min UV coordinates
    static constexpr const char* TEXTURE_MAX_UV = "max_uv";              //!< Max UV coordinates

    /* ----- Audio Component ----- */
    static constexpr const char* AUDIO_COMPONENT = "AudioComponent";     //!< Audio component
    static constexpr const char* SOUND_KEY = "sound_key";                //!< Resource key for sound
    static constexpr const char* SHOULD_LOOP = "should_loop";            //!< Whether sound loops
    static constexpr const char* IS_STREAMING = "is_streaming";          //!< Whether sound is streamed
    static constexpr const char* VOLUME = "volume";                      //!< Volume level
    static constexpr const char* AUDIO_TYPE = "audio_type";              //!< Type (SFX, Music, etc.)

    /* ----- Animation Component ----- */
    static constexpr const char* ANIMATION_COMPONENT = "AnimationComponent"; //!< Animation component
    static constexpr const char* ANIMATION_PROFILE = "profile_name";      //!< Animation profile name
    static constexpr const char* CURRENT_STATE = "current_state";         //!< Current animation state
    static constexpr const char* CURRENT_FRAME = "current_frame";         //!< Current frame index
    static constexpr const char* FRAME_DURATION = "frame_duration";       //!< Duration per frame
    static constexpr const char* START_FRAME = "start_frame";             //!< Starting frame
    static constexpr const char* FRAME_COUNT = "frame_count";             //!< Number of frames
    static constexpr const char* IS_PLAYING = "is_playing";               //!< Whether animation plays
    static constexpr const char* IS_LOOPING = "is_looping";               //!< Whether animation loops
    static constexpr const char* START_DELAY = "start_delay";             //!< Delay before starting
    static constexpr const char* START_DELAY_RANGE = "start_delay_range"; //!< Random delay range

    /* ----- Text Component ----- */
    static constexpr const char* TEXT_COMPONENT = "TextComponent";        //!< Text component
    static constexpr const char* TEXT = "text";                           //!< Text content
    static constexpr const char* FONT_KEY = "font_key";                   //!< Resource key for font
    static constexpr const char* FONT_COLOR = "font_color";               //!< Font color
    static constexpr const char* FONT_SIZE = "font_size";                 //!< Font size
    static constexpr const char* MAX_WIDTH = "max_width";                 //!< Maximum text width

    /* ----- Camera Component ----- */
    static constexpr const char* CAMERA_COMPONENT = "Camera";             //!< Camera component
    static constexpr const char* POSITION = "position";                   //!< Camera position
    static constexpr const char* WIDTH = "width";                         //!< Camera width
    static constexpr const char* HEIGHT = "height";                       //!< Camera height
    static constexpr const char* SIZE = "size";                           //!< Camera size
    static constexpr const char* ZOOM = "zoom";                           //!< Zoom level
    static constexpr const char* TARGET_ENTITY = "target_entity";         //!< Entity to follow
    static constexpr const char* FOLLOW_SPEED = "follow_speed";           //!< Camera follow speed
    static constexpr const char* IS_ACTIVE = "is_active";                 //!< Whether camera is active

    /* ----- Native Scripts Component ----- */
    static constexpr const char* NATIVE_SCRIPTS_COMPONENT = "NativeScripts"; //!< Native scripts
    static constexpr const char* SCRIPTS = "scripts";                     //!< Array of scripts
    static constexpr const char* SCRIPT_TYPE = "type";                    //!< Script type name
    static constexpr const char* SCRIPT_ENABLED = "enabled";              //!< Whether script is enabled
    static constexpr const char* SCRIPT_EXEC_IN_EDIT = "execute_in_edit"; //!< Run in editor mode

    /* ----- Button Component ----- */
    static constexpr const char* BUTTON_COMPONENT = "ButtonComponent";    //!< Button component
    static constexpr const char* IS_HOVERED = "is_hovered";               //!< Button hover state
    static constexpr const char* IS_PRESSED = "is_pressed";               //!< Button pressed state
    static constexpr const char* IS_RELEASED = "is_released";             //!< Button released state
    static constexpr const char* ACTION = "action";                       //!< Button action type
    static constexpr const char* TARGET_STRING = "target_string";         //!< Target scene name
    static constexpr const char* TARGET_VALUE = "target_value";           //!< Target numeric value
    static constexpr const char* HOVER_SOUND_KEY = "hover_sound_key";     //!< Hover sound resource
    static constexpr const char* HOVER_SOUND_VOLUME = "hover_sound_volume"; //!< Hover sound volume
    static constexpr const char* CLICK_SOUND_KEY = "click_sound_key";     //!< Click sound resource
    static constexpr const char* CLICK_SOUND_VOLUME = "click_sound_volume"; //!< Click sound volume
    static constexpr const char* BUTTON_DEFAULT_TEXTURE = "default_texture_key"; //!< Default texture
    static constexpr const char* BUTTON_HOVER_TEXTURE = "hover_texture_key"; //!< Hover texture
    static constexpr const char* BUTTON_PRESSED_TEXTURE = "pressed_texture_key"; //!< Pressed texture
    static constexpr const char* BUTTON_DEFAULT_ANIMATED = "default_is_animated"; //!< Default is animated
    static constexpr const char* BUTTON_HOVER_ANIMATED = "hover_is_animated"; //!< Hover is animated
    static constexpr const char* BUTTON_PRESSED_ANIMATED = "pressed_is_animated"; //!< Pressed is animated
    static constexpr const char* BUTTON_SCENE_TO_UNLOAD = "scene_to_unload_on_click"; //!< Scene to unload

    /* ----- Scene Ownership Component ----- */
    static constexpr const char* SCENE_OWNED_COMPONENT = "SceneOwned";    //!< Scene ownership component
    static constexpr const char* COMP_SCENE_ID = "scene_id";              //!< Owning scene ID
    static constexpr const char* COMP_SCENE_NAME = "sceneName";           //!< Owning scene name

    /* ----- Particle Emitter Component ----- */
    static constexpr const char* PARTICLE_EMITTER_COMPONENT = "ParticleEmitterComponent"; //!< Particle emitter
    static constexpr const char* IS_EMITTING = "is_emitting";             //!< Whether emitting
    static constexpr const char* IS_PARTICLE_ANIMATED = "is_particle_animated"; //!< Particles animated
    static constexpr const char* IS_SCREEN_SPACE = "is_screen_space";     //!< Screen space particles
    static constexpr const char* PARTICLE_TEXTURE_KEY = "particle_texture_key"; //!< Particle texture
    static constexpr const char* PARTICLE_LIFETIME = "particle_lifetime"; //!< Particle lifetime
    static constexpr const char* EMISSION_RATE = "emit_rate";             //!< Emission rate
    static constexpr const char* EMISSION_INTERVAL = "emit_interval";     //!< Emission interval
    static constexpr const char* PARTICLE_EFFECT_TYPE = "particle_effect_type"; //!< Effect type
    static constexpr const char* EFFECT_STRENGTH = "effect_strength";     //!< Effect strength
    static constexpr const char* PARTICLE_SPEED = "particle_speed";       //!< Particle speed
    static constexpr const char* PARTICLE_SIZE = "particle_size";         //!< Particle size
    static constexpr const char* IS_AFFECTED_BY_GRAVITY = "is_affected_by_gravity"; //!< Gravity effect
    static constexpr const char* EMISSION_STYLE = "emission_style";       //!< Emission style
    static constexpr const char* EMISSION_RADIUS = "emission_radius";     //!< Emission radius
    static constexpr const char* PARTICLE_START_VELOCITY = "particle_start_velocity"; //!< Start velocity
    static constexpr const char* PARTICLE_ACCELERATION = "particle_acceleration"; //!< Acceleration
    static constexpr const char* LINE_START = "line_start";               //!< Line start point
    static constexpr const char* LINE_END = "line_end";                   //!< Line end point
    static constexpr const char* CIRCLE_RADIUS = "circle_radius";         //!< Circle radius
    static constexpr const char* VIEWPORT_MIN = "viewport_min";           //!< Min viewport bounds
    static constexpr const char* VIEWPORT_MAX = "viewport_max";           //!< Max viewport bounds
    static constexpr const char* EMISSION_DIRECTION = "emission_direction"; //!< Emission direction
    static constexpr const char* EMISSION_OFFSET = "emission_offset";     //!< Emission offset
    static constexpr const char* TRIGGER_EVENT_NAME = "trigger_event_name"; //!< Trigger event
    static constexpr const char* EMISSION_MODE = "emission_mode";         //!< Emission mode
    static constexpr const char* BURST_COUNT = "burst_count";             //!< Burst particle count
    static constexpr const char* BURST_INTERVAL = "burst_interval";       //!< Burst interval
    static constexpr const char* MIN_BURST_COUNT = "min_burst_count";     //!< Min burst count
    static constexpr const char* MAX_BURST_COUNT = "max_burst_count";     //!< Max burst count
    static constexpr const char* MIN_BURST_INTERVAL = "min_burst_interval"; //!< Min burst interval
    static constexpr const char* MAX_BURST_INTERVAL = "max_burst_interval"; //!< Max burst interval

    /* ----- Slider Component ----- */
    static constexpr const char* SLIDER = "Slider";                       //!< Slider component
    static constexpr const char* SLIDER_CURRENT_VALUE = "current_value";  //!< Current slider value
    static constexpr const char* SLIDER_HANDLE_NAME = "handle_name";      //!< Handle entity name
    static constexpr const char* SLIDER_FILL_NAME = "fill_name";          //!< Fill entity name
    static constexpr const char* SLIDER_HANDLE_UUID = "handle_uuid";      //!< Handle UUID
    static constexpr const char* SLIDER_FILL_UUID = "fill_uuid";          //!< Fill UUID
    static constexpr const char* SLIDER_MIN_VALUE = "min_value";          //!< Minimum value
    static constexpr const char* SLIDER_MAX_VALUE = "max_value";          //!< Maximum value
    static constexpr const char* SLIDER_WIDTH = "width";                  //!< Slider width
    static constexpr const char* SLIDER_WIDTH_OFFSET_X = "width_offset_x"; //!< Width offset
    static constexpr const char* SLIDER_IS_DRAGGING = "is_dragging";      //!< Dragging state
    static constexpr const char* SLIDER_TYPE = "slider_type";             //!< Slider type
    static constexpr const char* SLIDER_ORIGINAL_FILL_HALF_SIZE_X = "original_fill_half_size_x"; //!< Original fill size
    static constexpr const char* SLIDER_ORIGINAL_FILL_LEFT_EDGE_X = "original_fill_left_edge_x"; //!< Original left edge

    /* ======================================================================
     * FSM (Finite State Machine) Keys
     * ====================================================================== */
    static constexpr const char* FSM_COMPONENT = "FSMComponent";          //!< FSM component
    static constexpr const char* FSM_DEFINITION_PATH = "DefinitionPath";  //!< Path to FSM definition
    static constexpr const char* FSM_CURRENT_STATE = "CurrentState";      //!< Current FSM state
    static constexpr const char* FSM_INITIAL_STATE = "InitialState";      //!< Initial FSM state

    /* ======================================================================
     * Tags JSON Labels
     * ====================================================================== */
    static constexpr const char* TAGS = "tags";                           //!< Container for tags
    static constexpr const char* PLAYER_TAG = "PlayerTag";                //!< Player tag
    static constexpr const char* PLATFORM_TAG = "PlatformTag";            //!< Platform tag
    static constexpr const char* BACKGROUND_TAG = "BackgroundTag";        //!< Background tag
    static constexpr const char* ENEMY_TAG = "EnemyTag";                  //!< Enemy tag
    static constexpr const char* GUI_TAG = "GUITag";                      //!< GUI element tag
    static constexpr const char* PERSISTENT_TAG = "PersistentTag";        //!< Persistent entity tag
    static constexpr const char* ACTIVE_TAG = "ActiveTag";                //!< Active entity tag
    static constexpr const char* SPAWN_PT_TAG = "SpawnPointTag";          //!< Spawn point tag
}
