#include "ffb_plugin.h"
#include <algorithm>

using namespace godot;

// Helper to clamp values
template <typename T>
T ffb_clamp(T val, T min, T max) { 
    return (val < min) ? min : ((val > max) ? max : val); 
}

void FFBPlugin::_bind_methods() {
    // --- Existing Methods ---
    ClassDB::bind_method(D_METHOD("_init"), &FFBPlugin::_init);
    ClassDB::bind_method(D_METHOD("init_ffb", "device_index"), &FFBPlugin::init_ffb);
    ClassDB::bind_method(D_METHOD("init_constant_force_effect"), &FFBPlugin::init_constant_force_effect);
    ClassDB::bind_method(D_METHOD("update_constant_force_effect", "force", "length", "effect_id"), &FFBPlugin::update_constant_ffb_effect);
    ClassDB::bind_method(D_METHOD("play_constant_force_effect", "effect_id", "iterations"), &FFBPlugin::play_constant_ffb_effect);
    ClassDB::bind_method(D_METHOD("destroy_ffb_effect", "effect_id"), &FFBPlugin::destroy_ffb_effect);
    ClassDB::bind_method(D_METHOD("close_ffb_device"), &FFBPlugin::close_ffb_device);
    ClassDB::bind_method(D_METHOD("stop_ffb_effect", "effect_id"), &FFBPlugin::stop_ffb_effect);

    // --- New FFB Wheel Functions ---
    ClassDB::bind_method(D_METHOD("init_periodic_effect", "type"), &FFBPlugin::init_periodic_effect);
    ClassDB::bind_method(D_METHOD("update_periodic_effect", "type", "magnitude", "offset", "period", "length", "effect_id"), &FFBPlugin::update_periodic_effect);
    
    ClassDB::bind_method(D_METHOD("init_condition_effect", "type"), &FFBPlugin::init_condition_effect);
    ClassDB::bind_method(D_METHOD("update_condition_effect", "type", "right_sat", "left_sat", "right_coeff", "left_coeff", "deadband", "effect_id"), &FFBPlugin::update_condition_effect);
    
    ClassDB::bind_method(D_METHOD("init_ramp_effect"), &FFBPlugin::init_ramp_effect);
    ClassDB::bind_method(D_METHOD("update_ramp_effect", "start", "end", "length", "effect_id"), &FFBPlugin::update_ramp_effect);
    
    ClassDB::bind_method(D_METHOD("set_gain", "gain"), &FFBPlugin::set_gain);
    ClassDB::bind_method(D_METHOD("set_autocenter", "autocenter"), &FFBPlugin::set_autocenter);
    ClassDB::bind_method(D_METHOD("pause_haptic"), &FFBPlugin::pause_haptic);
    ClassDB::bind_method(D_METHOD("resume_haptic"), &FFBPlugin::resume_haptic);
    ClassDB::bind_method(D_METHOD("stop_all_effects"), &FFBPlugin::stop_all_effects);
    ClassDB::bind_method(D_METHOD("get_max_effects"), &FFBPlugin::get_max_effects);
    ClassDB::bind_method(D_METHOD("get_num_axes"), &FFBPlugin::get_num_axes);
    ClassDB::bind_method(D_METHOD("has_feature", "feature"), &FFBPlugin::has_feature);

    // --- Bind Constants ---
    // Syntax: ClassDB::bind_integer_constant(get_class_static(), "EnumName", "ConstantName", Value);
    
    // Effect Types
    ClassDB::bind_integer_constant(get_class_static(), "FfbEffect", "EFFECT_CONSTANT", SDL_HAPTIC_CONSTANT);
    ClassDB::bind_integer_constant(get_class_static(), "FfbEffect", "EFFECT_SINE", SDL_HAPTIC_SINE);
    ClassDB::bind_integer_constant(get_class_static(), "FfbEffect", "EFFECT_SQUARE", SDL_HAPTIC_SQUARE);
    ClassDB::bind_integer_constant(get_class_static(), "FfbEffect", "EFFECT_TRIANGLE", SDL_HAPTIC_TRIANGLE);
    ClassDB::bind_integer_constant(get_class_static(), "FfbEffect", "EFFECT_SAWTOOTHUP", SDL_HAPTIC_SAWTOOTHUP);
    ClassDB::bind_integer_constant(get_class_static(), "FfbEffect", "EFFECT_SAWTOOTHDOWN", SDL_HAPTIC_SAWTOOTHDOWN);
    ClassDB::bind_integer_constant(get_class_static(), "FfbEffect", "EFFECT_RAMP", SDL_HAPTIC_RAMP);
    ClassDB::bind_integer_constant(get_class_static(), "FfbEffect", "EFFECT_SPRING", SDL_HAPTIC_SPRING);
    ClassDB::bind_integer_constant(get_class_static(), "FfbEffect", "EFFECT_DAMPER", SDL_HAPTIC_DAMPER);
    ClassDB::bind_integer_constant(get_class_static(), "FfbEffect", "EFFECT_INERTIA", SDL_HAPTIC_INERTIA);
    ClassDB::bind_integer_constant(get_class_static(), "FfbEffect", "EFFECT_FRICTION", SDL_HAPTIC_FRICTION);
    
    // Special Values
    ClassDB::bind_integer_constant(get_class_static(), "FfbEffect", "INFINITY", (int)SDL_HAPTIC_INFINITY);
}

FFBPlugin::FFBPlugin() {
    haptic = nullptr;
    joy = nullptr;
}

FFBPlugin::~FFBPlugin() {
    close_ffb_device();
}

void FFBPlugin::_init() {
}

int FFBPlugin::init_ffb(int p_device) {
    if (!SDL_InitSubSystem(SDL_INIT_JOYSTICK)) return -1;
    if (!SDL_InitSubSystem(SDL_INIT_HAPTIC)) return -1;
    
    int count = 0;
    SDL_JoystickID *joysticks = SDL_GetJoysticks(&count);
    if (!joysticks || count <= 0 || p_device < 0 || p_device >= count) {
        if (joysticks) SDL_free(joysticks);
        return -1;
    }
    
    joy = SDL_OpenJoystick(joysticks[p_device]);
    SDL_free(joysticks);
    
    if (!joy || !SDL_IsJoystickHaptic(joy)) {
        return -1;
    }
    
    haptic = SDL_OpenHapticFromJoystick(joy);
    if (!haptic) {
        force_feedback = false;
        return -1;
    }
    
    force_feedback = true;
    
    // Check for constant support initially
    if ((SDL_GetHapticFeatures(haptic) & SDL_HAPTIC_CONSTANT) == 0) {
        constant_force = false;
    } else {
        constant_force = true;
    }
    
    return 0;
}

void FFBPlugin::close_ffb_device() {
    if (haptic) {
        SDL_CloseHaptic(haptic);
        haptic = nullptr;
    }
    if (joy) {
        SDL_CloseJoystick(joy);
        joy = nullptr;
    }
    force_feedback = false;
    constant_force = false;
}

// --- Constant Force ---

int FFBPlugin::init_constant_force_effect() {
    if (!constant_force) return -1;
    
    SDL_HapticEffect eff;
    SDL_memset(&eff, 0, sizeof(SDL_HapticEffect));
    eff.type = SDL_HAPTIC_CONSTANT;
    eff.constant.direction.type = SDL_HAPTIC_CARTESIAN;
    eff.constant.direction.dir[0] = 1; 
    eff.constant.direction.dir[1] = 0;
    eff.constant.level = 0;
    eff.constant.length = SDL_HAPTIC_INFINITY;
    eff.constant.attack_length = 0; 
    eff.constant.fade_length = 0;
    
    return SDL_CreateHapticEffect(haptic, &eff);
}

int FFBPlugin::update_constant_ffb_effect(float force, int length, int effect_id) {
    if (!force_feedback || !constant_force || effect_id == -1) return -1;
    
    SDL_HapticEffect eff;
    SDL_memset(&eff, 0, sizeof(SDL_HapticEffect));
    eff.type = SDL_HAPTIC_CONSTANT;
    eff.constant.direction.type = SDL_HAPTIC_CARTESIAN;
    eff.constant.direction.dir[0] = 1; 
    eff.constant.direction.dir[1] = 0;
    eff.constant.level = (Sint16)(ffb_clamp(force, -1.0f, 1.0f) * 32767.0f);
    eff.constant.length = (length == 0) ? SDL_HAPTIC_INFINITY : (Uint32)length;
    eff.constant.attack_length = 0; 
    eff.constant.fade_length = 0;

    return SDL_UpdateHapticEffect(haptic, effect_id, &eff) ? 0 : -1;
}

int FFBPlugin::play_constant_ffb_effect(int effect_id, int iterations) {
    if (!force_feedback || !constant_force || effect_id == -1) return -1;
    if (iterations == 0) iterations = SDL_HAPTIC_INFINITY;
    return SDL_RunHapticEffect(haptic, effect_id, (Uint32)iterations) ? 0 : -1;
}

// --- Periodic Effects (Rumble/Road Noise) ---

int FFBPlugin::init_periodic_effect(int p_type) {
    if (!force_feedback) return -1;
    
    Uint16 sdl_type;
    switch(p_type) {
        case 0: sdl_type = SDL_HAPTIC_SINE; break;
        case 1: sdl_type = SDL_HAPTIC_SQUARE; break;
        case 2: sdl_type = SDL_HAPTIC_TRIANGLE; break;
        case 3: sdl_type = SDL_HAPTIC_SAWTOOTHUP; break;
        case 4: sdl_type = SDL_HAPTIC_SAWTOOTHDOWN; break;
        default: return -1;
    }
    
    if ((SDL_GetHapticFeatures(haptic) & sdl_type) == 0) return -2;

    SDL_HapticEffect eff;
    SDL_memset(&eff, 0, sizeof(SDL_HapticEffect));
    eff.type = sdl_type;
    eff.periodic.direction.type = SDL_HAPTIC_CARTESIAN;
    eff.periodic.direction.dir[0] = 1; 
    eff.periodic.direction.dir[1] = 0;
    eff.periodic.period = 1000; // Default 1Hz
    eff.periodic.magnitude = 16384; // 50% strength
    eff.periodic.offset = 0;
    eff.periodic.phase = 0;
    eff.periodic.length = SDL_HAPTIC_INFINITY;
    eff.periodic.attack_length = 0;
    eff.periodic.fade_length = 0;
    
    return SDL_CreateHapticEffect(haptic, &eff);
}

int FFBPlugin::update_periodic_effect(int p_type, float magnitude, float offset, int period, int length, int effect_id) {
    if (!force_feedback || effect_id == -1) return -1;
    
    Uint16 sdl_type;
    switch(p_type) {
        case 0: sdl_type = SDL_HAPTIC_SINE; break;
        case 1: sdl_type = SDL_HAPTIC_SQUARE; break;
        case 2: sdl_type = SDL_HAPTIC_TRIANGLE; break;
        case 3: sdl_type = SDL_HAPTIC_SAWTOOTHUP; break;
        case 4: sdl_type = SDL_HAPTIC_SAWTOOTHDOWN; break;
        default: return -1;
    }

    SDL_HapticEffect eff;
    SDL_memset(&eff, 0, sizeof(SDL_HapticEffect));
    eff.type = sdl_type;
    eff.periodic.direction.type = SDL_HAPTIC_CARTESIAN;
    eff.periodic.direction.dir[0] = 1; 
    eff.periodic.direction.dir[1] = 0;
    eff.periodic.magnitude = (Sint16)(ffb_clamp(magnitude, -1.0f, 1.0f) * 32767.0f);
    eff.periodic.offset = (Sint16)(ffb_clamp(offset, -1.0f, 1.0f) * 32767.0f);
    eff.periodic.period = (period > 0) ? (Uint16)period : 1000;
    eff.periodic.length = (length == 0) ? SDL_HAPTIC_INFINITY : (Uint32)length;
    eff.periodic.attack_length = 0;
    eff.periodic.fade_length = 0;
    
    return SDL_UpdateHapticEffect(haptic, effect_id, &eff) ? 0 : -1;
}

// --- Condition Effects (Spring, Damper, Friction, Inertia) ---

int FFBPlugin::init_condition_effect(int p_type) {
    if (!force_feedback) return -1;
    
    Uint16 sdl_type;
    switch(p_type) {
        case 0: sdl_type = SDL_HAPTIC_SPRING; break;
        case 1: sdl_type = SDL_HAPTIC_DAMPER; break;
        case 2: sdl_type = SDL_HAPTIC_INERTIA; break;
        case 3: sdl_type = SDL_HAPTIC_FRICTION; break;
        default: return -1;
    }
    
    if ((SDL_GetHapticFeatures(haptic) & sdl_type) == 0) return -2;

    SDL_HapticEffect eff;
    SDL_memset(&eff, 0, sizeof(SDL_HapticEffect));
    eff.type = sdl_type;
    eff.condition.length = SDL_HAPTIC_INFINITY;
    
    // Initialize all axes to safe defaults (no force)
    for(int i=0; i<3; ++i) {
        eff.condition.right_sat[i] = 0xFFFF; 
        eff.condition.left_sat[i] = 0xFFFF;
        eff.condition.right_coeff[i] = 0; 
        eff.condition.left_coeff[i] = 0;
        eff.condition.deadband[i] = 0; 
        eff.condition.center[i] = 0;
    }
    
    return SDL_CreateHapticEffect(haptic, &eff);
}

int FFBPlugin::update_condition_effect(int p_type, float right_sat, float left_sat, float right_coeff, float left_coeff, float deadband, int effect_id) {
    if (!force_feedback || effect_id == -1) return -1;
    
    Uint16 sdl_type;
    switch(p_type) {
        case 0: sdl_type = SDL_HAPTIC_SPRING; break;
        case 1: sdl_type = SDL_HAPTIC_DAMPER; break;
        case 2: sdl_type = SDL_HAPTIC_INERTIA; break;
        case 3: sdl_type = SDL_HAPTIC_FRICTION; break;
        default: sdl_type = SDL_HAPTIC_SPRING;
    }

    SDL_HapticEffect eff;
    SDL_memset(&eff, 0, sizeof(SDL_HapticEffect));
    eff.type = sdl_type;
    eff.condition.length = SDL_HAPTIC_INFINITY;
    
    // Apply parameters to all axes (usually steering wheel only uses axis 0, but this ensures consistency)
    for(int i=0; i<3; ++i) {
        // Saturation: 0.0 to 1.0 -> 0 to 0xFFFF
        eff.condition.right_sat[i] = (Uint16)(ffb_clamp((int)(right_sat * 0xFFFF), 0, 0xFFFF));
        eff.condition.left_sat[i] = (Uint16)(ffb_clamp((int)(left_sat * 0xFFFF), 0, 0xFFFF));
        
        // Coefficient: -1.0 to 1.0 -> -0x7FFF to 0x7FFF
        eff.condition.right_coeff[i] = (Sint16)(ffb_clamp((int)(right_coeff * 0x7FFF), -0x7FFF, 0x7FFF));
        eff.condition.left_coeff[i] = (Sint16)(ffb_clamp((int)(left_coeff * 0x7FFF), -0x7FFF, 0x7FFF));
        
        // Deadband: 0.0 to 1.0 -> 0 to 0xFFFF
        eff.condition.deadband[i] = (Uint16)(ffb_clamp((int)(deadband * 0xFFFF), 0, 0xFFFF));
        eff.condition.center[i] = 0;
    }
    
    return SDL_UpdateHapticEffect(haptic, effect_id, &eff) ? 0 : -1;
}

// --- Ramp Effect ---

int FFBPlugin::init_ramp_effect() {
    if (!force_feedback) return -1;
    if ((SDL_GetHapticFeatures(haptic) & SDL_HAPTIC_RAMP) == 0) return -2;
    
    SDL_HapticEffect eff;
    SDL_memset(&eff, 0, sizeof(SDL_HapticEffect));
    eff.type = SDL_HAPTIC_RAMP;
    eff.ramp.direction.type = SDL_HAPTIC_CARTESIAN;
    eff.ramp.direction.dir[0] = 1; 
    eff.ramp.direction.dir[1] = 0;
    eff.ramp.start = 0;
    eff.ramp.end = 0;
    eff.ramp.length = SDL_HAPTIC_INFINITY;
    eff.ramp.attack_length = 0;
    eff.ramp.fade_length = 0;
    
    return SDL_CreateHapticEffect(haptic, &eff);
}

int FFBPlugin::update_ramp_effect(float start, float end, int length, int effect_id) {
    if (!force_feedback || effect_id == -1) return -1;
    
    SDL_HapticEffect eff;
    SDL_memset(&eff, 0, sizeof(SDL_HapticEffect));
    eff.type = SDL_HAPTIC_RAMP;
    eff.ramp.direction.type = SDL_HAPTIC_CARTESIAN;
    eff.ramp.direction.dir[0] = 1; 
    eff.ramp.direction.dir[1] = 0;
    eff.ramp.start = (Sint16)(ffb_clamp(start, -1.0f, 1.0f) * 32767.0f);
    eff.ramp.end = (Sint16)(ffb_clamp(end, -1.0f, 1.0f) * 32767.0f);
    eff.ramp.length = (length == 0) ? SDL_HAPTIC_INFINITY : (Uint32)length;
    eff.ramp.attack_length = 0;
    eff.ramp.fade_length = 0;
    
    return SDL_UpdateHapticEffect(haptic, effect_id, &eff) ? 0 : -1;
}

// --- Device Control ---

int FFBPlugin::set_gain(int p_gain) {
    if (!force_feedback) return -1;
    // SDL expects 0-100
    return SDL_SetHapticGain(haptic, ffb_clamp(p_gain, 0, 100)) ? 0 : -1;
}

int FFBPlugin::set_autocenter(int p_autocenter) {
    if (!force_feedback) return -1;
    // SDL expects 0-100
    return SDL_SetHapticAutocenter(haptic, ffb_clamp(p_autocenter, 0, 100)) ? 0 : -1;
}

void FFBPlugin::pause_haptic() { 
    if (force_feedback) SDL_PauseHaptic(haptic); 
}

void FFBPlugin::resume_haptic() { 
    if (force_feedback) SDL_ResumeHaptic(haptic); 
}

void FFBPlugin::stop_all_effects() { 
    if (force_feedback) SDL_StopHapticEffects(haptic); 
}

void FFBPlugin::stop_ffb_effect(int effect_id) {
    if (haptic && effect_id != -1) SDL_StopHapticEffect(haptic, effect_id);
}

void FFBPlugin::destroy_ffb_effect(int effect_id) {
    if (haptic && effect_id != -1) SDL_DestroyHapticEffect(haptic, effect_id);
}

int FFBPlugin::get_max_effects() const { 
    return force_feedback ? SDL_GetMaxHapticEffects(haptic) : 0; 
}

int FFBPlugin::get_num_axes() const { 
    return force_feedback ? SDL_GetNumHapticAxes(haptic) : 0; 
}

bool FFBPlugin::has_feature(int p_feature) const { 
    return force_feedback && (SDL_GetHapticFeatures(haptic) & p_feature); 
}