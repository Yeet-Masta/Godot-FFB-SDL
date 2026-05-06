#include "ffb_plugin.h"

using namespace godot;

// ---------- bind methods ----------
void FFBPlugin::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_init"), &FFBPlugin::_init);

    // Device enumeration
    ClassDB::bind_method(D_METHOD("get_joystick_names"), &FFBPlugin::get_joystick_names);
    ClassDB::bind_method(D_METHOD("is_joystick_haptic", "device"), &FFBPlugin::is_joystick_haptic);

    // Device lifecycle
    ClassDB::bind_method(D_METHOD("init_ffb", "device"), &FFBPlugin::init_ffb);
    ClassDB::bind_method(D_METHOD("close_ffb_device"), &FFBPlugin::close_ffb_device);

    // Device introspection
    ClassDB::bind_method(D_METHOD("get_device_name"), &FFBPlugin::get_device_name);
    ClassDB::bind_method(D_METHOD("get_num_axes"), &FFBPlugin::get_num_axes);
    ClassDB::bind_method(D_METHOD("get_max_effects"), &FFBPlugin::get_max_effects);
    ClassDB::bind_method(D_METHOD("get_max_effects_playing"), &FFBPlugin::get_max_effects_playing);
    ClassDB::bind_method(D_METHOD("get_features_bitmask"), &FFBPlugin::get_features_bitmask);
    ClassDB::bind_method(D_METHOD("supports_effect_type", "type"), &FFBPlugin::supports_effect_type);
    ClassDB::bind_method(D_METHOD("is_rumble_supported"), &FFBPlugin::is_rumble_supported);
    ClassDB::bind_method(D_METHOD("has_force_feedback"), &FFBPlugin::has_force_feedback);
    ClassDB::bind_method(D_METHOD("has_constant_force"), &FFBPlugin::has_constant_force);
    ClassDB::bind_method(D_METHOD("get_autocenter"), &FFBPlugin::get_autocenter);

    // Global controls
    ClassDB::bind_method(D_METHOD("set_gain", "gain"), &FFBPlugin::set_gain);
    ClassDB::bind_method(D_METHOD("set_autocenter", "autocenter"), &FFBPlugin::set_autocenter);
    ClassDB::bind_method(D_METHOD("pause"), &FFBPlugin::pause);
    ClassDB::bind_method(D_METHOD("resume"), &FFBPlugin::resume);
    ClassDB::bind_method(D_METHOD("stop_all_effects"), &FFBPlugin::stop_all_effects);

    // Constant force (existing API)
    ClassDB::bind_method(D_METHOD("init_constant_force_effect"), &FFBPlugin::init_constant_force_effect);
    ClassDB::bind_method(D_METHOD("update_constant_force_effect", "force", "length", "effect_id"), &FFBPlugin::update_constant_ffb_effect);
    ClassDB::bind_method(D_METHOD("play_constant_force_effect", "effect_id", "iterations"), &FFBPlugin::play_constant_ffb_effect);

    // Ramp
    ClassDB::bind_method(D_METHOD("init_ramp_effect", "start", "end", "length"), &FFBPlugin::init_ramp_effect);
    ClassDB::bind_method(D_METHOD("update_ramp_effect", "effect_id", "start", "end", "length"), &FFBPlugin::update_ramp_effect);

    // Periodic
    ClassDB::bind_method(D_METHOD("init_periodic_effect", "wave_type", "period_ms", "magnitude", "length"), &FFBPlugin::init_periodic_effect);
    ClassDB::bind_method(D_METHOD("update_periodic_effect", "effect_id", "wave_type", "period_ms", "magnitude", "length"), &FFBPlugin::update_periodic_effect);

    // Condition (spring/damper/inertia/friction)
    ClassDB::bind_method(D_METHOD("init_condition_effect",
                                  "condition_type", "left_coeff", "right_coeff",
                                  "left_sat", "right_sat", "deadband", "center", "length"),
                         &FFBPlugin::init_condition_effect);
    ClassDB::bind_method(D_METHOD("update_condition_effect",
                                  "effect_id", "condition_type", "left_coeff", "right_coeff",
                                  "left_sat", "right_sat", "deadband", "center", "length"),
                         &FFBPlugin::update_condition_effect);

    // Generic effect control
    ClassDB::bind_method(D_METHOD("run_effect", "effect_id", "iterations"), &FFBPlugin::run_effect);
    ClassDB::bind_method(D_METHOD("stop_ffb_effect", "effect_id"), &FFBPlugin::stop_ffb_effect);
    ClassDB::bind_method(D_METHOD("destroy_ffb_effect", "effect_id"), &FFBPlugin::destroy_ffb_effect);
    ClassDB::bind_method(D_METHOD("is_effect_playing", "effect_id"), &FFBPlugin::is_effect_playing);

    // Simple rumble
    ClassDB::bind_method(D_METHOD("init_rumble"), &FFBPlugin::init_rumble);
    ClassDB::bind_method(D_METHOD("play_rumble", "strength", "length"), &FFBPlugin::play_rumble);
    ClassDB::bind_method(D_METHOD("stop_rumble"), &FFBPlugin::stop_rumble);

    // Constants — accessible from GDScript as FFBPlugin.WAVE_SINE etc.
    BIND_CONSTANT(WAVE_SINE);
    BIND_CONSTANT(WAVE_SQUARE);
    BIND_CONSTANT(WAVE_TRIANGLE);
    BIND_CONSTANT(WAVE_SAWTOOTHUP);
    BIND_CONSTANT(WAVE_SAWTOOTHDOWN);
    BIND_CONSTANT(CONDITION_SPRING);
    BIND_CONSTANT(CONDITION_DAMPER);
    BIND_CONSTANT(CONDITION_INERTIA);
    BIND_CONSTANT(CONDITION_FRICTION);
}

// ---------- ctor / dtor ----------
FFBPlugin::FFBPlugin() {
    haptic = nullptr;
    joy = nullptr;
}

FFBPlugin::~FFBPlugin() {
    close_ffb_device();
}

void FFBPlugin::_init() {}

// ---------- private helpers ----------
Sint16 FFBPlugin::to_s16(float v) {
    if (v > 1.0f) v = 1.0f;
    else if (v < -1.0f) v = -1.0f;
    return (Sint16)(v * 32767.0f);
}

Uint16 FFBPlugin::to_u16(float v) {
    if (v > 1.0f) v = 1.0f;
    else if (v < 0.0f) v = 0.0f;
    return (Uint16)(v * 65535.0f);
}

Uint32 FFBPlugin::to_length(int length_ms) {
    if (length_ms <= 0) return SDL_HAPTIC_INFINITY;
    return (Uint32)length_ms;
}

// ---------- enumeration ----------
PackedStringArray FFBPlugin::get_joystick_names() {
    PackedStringArray result;

    // Make sure subsystem is up so we can query — safe to call repeatedly.
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);

    int count = 0;
    SDL_JoystickID *ids = SDL_GetJoysticks(&count);
    if (!ids) return result;

    for (int i = 0; i < count; i++) {
        const char *name = SDL_GetJoystickNameForID(ids[i]);
        result.push_back(name ? String(name) : String("Unknown"));
    }
    SDL_free(ids);
    return result;
}

bool FFBPlugin::is_joystick_haptic(int p_device) {
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    SDL_InitSubSystem(SDL_INIT_HAPTIC);

    int count = 0;
    SDL_JoystickID *ids = SDL_GetJoysticks(&count);
    if (!ids) return false;

    bool ok = false;
    if (p_device >= 0 && p_device < count) {
        // Open just to check, then close. Cheap enough.
        SDL_Joystick *j = SDL_OpenJoystick(ids[p_device]);
        if (j) {
            ok = SDL_IsJoystickHaptic(j);
            SDL_CloseJoystick(j);
        }
    }
    SDL_free(ids);
    return ok;
}

// ---------- lifecycle ----------
int FFBPlugin::init_ffb(int p_device) {
    if (!SDL_InitSubSystem(SDL_INIT_JOYSTICK)) return -1;
    if (!SDL_InitSubSystem(SDL_INIT_HAPTIC)) return -1;

    int count = 0;
    SDL_JoystickID *joysticks = SDL_GetJoysticks(&count);
    if (!joysticks || count <= 0) {
        if (joysticks) SDL_free(joysticks);
        return -1;
    }

    if (p_device < 0 || p_device >= count) {
        SDL_free(joysticks);
        return -1;
    }

    joy = SDL_OpenJoystick(joysticks[p_device]);
    SDL_free(joysticks);

    if (joy == nullptr) return -1;

    if (SDL_IsJoystickHaptic(joy) == false) return -1;

    haptic = SDL_OpenHapticFromJoystick(joy);
    if (haptic == nullptr) {
        force_feedback = false;
        return -1;
    }
    force_feedback = true;

    Uint32 features = SDL_GetHapticFeatures(haptic);
    constant_force = (features & SDL_HAPTIC_CONSTANT) != 0;

    // We no longer fail init when the device lacks constant force —
    // it might still support periodic, condition, or rumble effects.
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
    autocenter = 0;
}

// ---------- introspection ----------
String FFBPlugin::get_device_name() const {
    if (!haptic) return String();
    const char *name = SDL_GetHapticName(haptic);
    return name ? String(name) : String();
}

int FFBPlugin::get_num_axes() const {
    if (!haptic) return -1;
    return SDL_GetNumHapticAxes(haptic);
}

int FFBPlugin::get_max_effects() const {
    if (!haptic) return -1;
    return SDL_GetMaxHapticEffects(haptic);
}

int FFBPlugin::get_max_effects_playing() const {
    if (!haptic) return -1;
    return SDL_GetMaxHapticEffectsPlaying(haptic);
}

int FFBPlugin::get_features_bitmask() const {
    if (!haptic) return 0;
    return (int)SDL_GetHapticFeatures(haptic);
}

bool FFBPlugin::supports_effect_type(int p_type) const {
    if (!haptic) return false;
    return (SDL_GetHapticFeatures(haptic) & (Uint32)p_type) != 0;
}

bool FFBPlugin::is_rumble_supported() const {
    if (!haptic) return false;
    return SDL_HapticRumbleSupported(haptic);
}

// ---------- global controls ----------
int FFBPlugin::set_gain(int p_gain) {
    if (!haptic) return -1;
    if (p_gain < 0) p_gain = 0;
    else if (p_gain > 100) p_gain = 100;
    return SDL_SetHapticGain(haptic, p_gain) ? 0 : -1;
}

int FFBPlugin::set_autocenter(int p_autocenter) {
    if (!haptic) return -1;
    if (p_autocenter < 0) p_autocenter = 0;
    else if (p_autocenter > 100) p_autocenter = 100;
    if (!SDL_SetHapticAutocenter(haptic, p_autocenter)) return -1;
    autocenter = p_autocenter;
    return 0;
}

int FFBPlugin::pause() {
    if (!haptic) return -1;
    return SDL_PauseHaptic(haptic) ? 0 : -1;
}

int FFBPlugin::resume() {
    if (!haptic) return -1;
    return SDL_ResumeHaptic(haptic) ? 0 : -1;
}

int FFBPlugin::stop_all_effects() {
    if (!haptic) return -1;
    return SDL_StopHapticEffects(haptic) ? 0 : -1;
}

// ---------- constant force (existing API, unchanged behavior) ----------
int FFBPlugin::init_constant_force_effect() {
    if (constant_force == false) return -1;

    SDL_memset(&effect, 0, sizeof(SDL_HapticEffect));
    effect.type = SDL_HAPTIC_CONSTANT;
    effect.constant.direction.type = SDL_HAPTIC_CARTESIAN;
    effect.constant.direction.dir[0] = 1;
    effect.constant.direction.dir[1] = 0;
    effect.constant.level = 0;
    effect.constant.length = 0;
    effect.constant.attack_length = 0;
    effect.constant.fade_length = 0;

    return SDL_CreateHapticEffect(haptic, &effect);
}

int FFBPlugin::update_constant_ffb_effect(float force, int length, int effect_id) {
    if (!force_feedback || !constant_force || effect_id == -1) return -1;

    if (force > 1.0f) force = 1.0f;
    else if (force < -1.0f) force = -1.0f;

    if (length == 0) length = SDL_HAPTIC_INFINITY;

    effect.constant.level = (short)(force * 32767.0f);
    effect.constant.length = length;

    if (!SDL_UpdateHapticEffect(haptic, effect_id, &effect)) return -1;
    return 0;
}

int FFBPlugin::play_constant_ffb_effect(int effect_id, int iterations) {
    if (!force_feedback || !constant_force || effect_id == -1) return -1;
    if (iterations == 0) iterations = SDL_HAPTIC_INFINITY;
    if (!SDL_RunHapticEffect(haptic, effect_id, iterations)) return -1;
    return 0;
}

// ---------- ramp ----------
int FFBPlugin::init_ramp_effect(float start, float end, int length) {
    if (!haptic) return -1;
    if (!supports_effect_type(SDL_HAPTIC_RAMP)) return -1;

    SDL_HapticEffect e;
    SDL_memset(&e, 0, sizeof(e));
    e.type = SDL_HAPTIC_RAMP;
    e.ramp.direction.type = SDL_HAPTIC_CARTESIAN;
    e.ramp.direction.dir[0] = 1;
    e.ramp.length = to_length(length);
    e.ramp.start = to_s16(start);
    e.ramp.end = to_s16(end);
    return SDL_CreateHapticEffect(haptic, &e);
}

int FFBPlugin::update_ramp_effect(int effect_id, float start, float end, int length) {
    if (!haptic || effect_id == -1) return -1;

    SDL_HapticEffect e;
    SDL_memset(&e, 0, sizeof(e));
    e.type = SDL_HAPTIC_RAMP;
    e.ramp.direction.type = SDL_HAPTIC_CARTESIAN;
    e.ramp.direction.dir[0] = 1;
    e.ramp.length = to_length(length);
    e.ramp.start = to_s16(start);
    e.ramp.end = to_s16(end);
    return SDL_UpdateHapticEffect(haptic, effect_id, &e) ? 0 : -1;
}

// ---------- periodic ----------
int FFBPlugin::init_periodic_effect(int wave_type, int period_ms, float magnitude, int length) {
    if (!haptic) return -1;
    if (!supports_effect_type(wave_type)) return -1;

    SDL_HapticEffect e;
    SDL_memset(&e, 0, sizeof(e));
    e.type = (Uint16)wave_type;
    e.periodic.direction.type = SDL_HAPTIC_CARTESIAN;
    e.periodic.direction.dir[0] = 1;
    e.periodic.length = to_length(length);
    e.periodic.period = (Uint16)(period_ms < 0 ? 0 : period_ms);
    e.periodic.magnitude = to_s16(magnitude);
    e.periodic.offset = 0;
    e.periodic.phase = 0;
    return SDL_CreateHapticEffect(haptic, &e);
}

int FFBPlugin::update_periodic_effect(int effect_id, int wave_type, int period_ms, float magnitude, int length) {
    if (!haptic || effect_id == -1) return -1;

    SDL_HapticEffect e;
    SDL_memset(&e, 0, sizeof(e));
    e.type = (Uint16)wave_type;
    e.periodic.direction.type = SDL_HAPTIC_CARTESIAN;
    e.periodic.direction.dir[0] = 1;
    e.periodic.length = to_length(length);
    e.periodic.period = (Uint16)(period_ms < 0 ? 0 : period_ms);
    e.periodic.magnitude = to_s16(magnitude);
    e.periodic.offset = 0;
    e.periodic.phase = 0;
    return SDL_UpdateHapticEffect(haptic, effect_id, &e) ? 0 : -1;
}

// ---------- condition (spring / damper / inertia / friction) ----------
// Helper-style fill — used by both init and update.
static void fill_condition(SDL_HapticEffect &e,
                           int condition_type,
                           float left_coeff, float right_coeff,
                           float left_sat, float right_sat,
                           float deadband, float center,
                           int length) {
    SDL_memset(&e, 0, sizeof(e));
    e.type = (Uint16)condition_type;
    e.condition.direction.type = SDL_HAPTIC_CARTESIAN;
    e.condition.direction.dir[0] = 1;
    e.condition.length = (length <= 0) ? SDL_HAPTIC_INFINITY : (Uint32)length;

    auto clamp_s16 = [](float v) -> Sint16 {
        if (v > 1.0f) v = 1.0f; else if (v < -1.0f) v = -1.0f;
        return (Sint16)(v * 32767.0f);
    };
    auto clamp_u16 = [](float v) -> Uint16 {
        if (v > 1.0f) v = 1.0f; else if (v < 0.0f) v = 0.0f;
        return (Uint16)(v * 65535.0f);
    };

    Sint16 lc = clamp_s16(left_coeff);
    Sint16 rc = clamp_s16(right_coeff);
    Uint16 ls = clamp_u16(left_sat);
    Uint16 rs = clamp_u16(right_sat);
    Uint16 db = clamp_u16(deadband);
    Sint16 ct = clamp_s16(center);

    // Apply the same values to all three axes — fine for single-axis devices
    // (the unused axes are simply ignored by the device).
    for (int i = 0; i < 3; i++) {
        e.condition.left_coeff[i]  = lc;
        e.condition.right_coeff[i] = rc;
        e.condition.left_sat[i]    = ls;
        e.condition.right_sat[i]   = rs;
        e.condition.deadband[i]    = db;
        e.condition.center[i]      = ct;
    }
}

int FFBPlugin::init_condition_effect(int condition_type,
                                     float left_coeff, float right_coeff,
                                     float left_sat, float right_sat,
                                     float deadband, float center,
                                     int length) {
    if (!haptic) return -1;
    if (!supports_effect_type(condition_type)) return -1;

    SDL_HapticEffect e;
    fill_condition(e, condition_type, left_coeff, right_coeff,
                   left_sat, right_sat, deadband, center, length);
    return SDL_CreateHapticEffect(haptic, &e);
}

int FFBPlugin::update_condition_effect(int effect_id, int condition_type,
                                       float left_coeff, float right_coeff,
                                       float left_sat, float right_sat,
                                       float deadband, float center,
                                       int length) {
    if (!haptic || effect_id == -1) return -1;

    SDL_HapticEffect e;
    fill_condition(e, condition_type, left_coeff, right_coeff,
                   left_sat, right_sat, deadband, center, length);
    return SDL_UpdateHapticEffect(haptic, effect_id, &e) ? 0 : -1;
}

// ---------- generic effect control ----------
int FFBPlugin::run_effect(int effect_id, int iterations) {
    if (!haptic || effect_id == -1) return -1;
    Uint32 it = (iterations <= 0) ? SDL_HAPTIC_INFINITY : (Uint32)iterations;
    return SDL_RunHapticEffect(haptic, effect_id, it) ? 0 : -1;
}

int FFBPlugin::stop_ffb_effect(int effect_id) {
    if (!haptic || effect_id == -1) return -1;
    return SDL_StopHapticEffect(haptic, effect_id) ? 0 : -1;
}

void FFBPlugin::destroy_ffb_effect(int effect_id) {
    if (haptic && effect_id != -1) {
        SDL_DestroyHapticEffect(haptic, effect_id);
    }
}

bool FFBPlugin::is_effect_playing(int effect_id) {
    if (!haptic || effect_id == -1) return false;
    // Returns true if playing, false if stopped or on error.
    return SDL_GetHapticEffectStatus(haptic, effect_id);
}

// ---------- simple rumble ----------
int FFBPlugin::init_rumble() {
    if (!haptic) return -1;
    if (!SDL_HapticRumbleSupported(haptic)) return -1;
    return SDL_InitHapticRumble(haptic) ? 0 : -1;
}

int FFBPlugin::play_rumble(float strength, int length) {
    if (!haptic) return -1;
    if (strength < 0.0f) strength = 0.0f;
    else if (strength > 1.0f) strength = 1.0f;
    return SDL_PlayHapticRumble(haptic, strength, to_length(length)) ? 0 : -1;
}

int FFBPlugin::stop_rumble() {
    if (!haptic) return -1;
    return SDL_StopHapticRumble(haptic) ? 0 : -1;
}