#include "ffb_plugin.h"

using namespace godot;

void FFBPlugin::_bind_methods(){
    ClassDB::bind_method(D_METHOD("_init"), &FFBPlugin::_init);
    ClassDB::bind_method(D_METHOD("init_ffb"), &FFBPlugin::init_ffb);
    ClassDB::bind_method(D_METHOD("init_constant_force_effect"), &FFBPlugin::init_constant_force_effect);
    ClassDB::bind_method(D_METHOD("update_constant_force_effect"), &FFBPlugin::update_constant_ffb_effect);
    ClassDB::bind_method(D_METHOD("play_constant_force_effect"), &FFBPlugin::play_constant_ffb_effect);
    ClassDB::bind_method(D_METHOD("destroy_ffb_effect"), &FFBPlugin::destroy_ffb_effect);
    ClassDB::bind_method(D_METHOD("close_ffb_device"), &FFBPlugin::close_ffb_device);
	ClassDB::bind_method(D_METHOD("stop_ffb_effect"), &FFBPlugin::stop_ffb_effect);
}

FFBPlugin::FFBPlugin(){
    // Initialize pointers
    haptic = nullptr;
    joy = nullptr;
}

FFBPlugin::~FFBPlugin(){
    close_ffb_device();
}

void FFBPlugin::_init(){
}

int FFBPlugin::init_ffb(int p_device){
    // Initialize the joystick subsystem and haptic system
    if (!SDL_InitSubSystem(SDL_INIT_JOYSTICK)) return -1;
    if (!SDL_InitSubSystem(SDL_INIT_HAPTIC)) return -1;

    // SDL3 uses Joystick IDs instead of direct indices. We get the array of IDs first.
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
    SDL_free(joysticks); // Must free the array allocated by SDL3

    if (joy == nullptr) {
        return -1;
    }

    if (SDL_IsJoystickHaptic(joy) == false) {
        return -1;
    }

    haptic = SDL_OpenHapticFromJoystick(joy);
    if (haptic == nullptr){
        force_feedback = false;
        return -1;
    }

    force_feedback = true;

    // See if it can do constant force
    if ((SDL_GetHapticFeatures(haptic) & SDL_HAPTIC_CONSTANT) == 0) {
        SDL_CloseHaptic(haptic); // No Constant effect
        haptic = nullptr;
        constant_force = false;
        return -1;
    }
    
    constant_force = true;
    return 0;
}

int FFBPlugin::init_constant_force_effect(){
    if (constant_force == false) {
        return -1;
    }

    int effect_id = -1;
    SDL_memset(&effect, 0, sizeof(SDL_HapticEffect)); // 0 is safe default
    effect.type = SDL_HAPTIC_CONSTANT;
    effect.constant.direction.type = SDL_HAPTIC_CARTESIAN; // Cartesian coordinates
    effect.constant.direction.dir[0] = 1;
    effect.constant.direction.dir[1] = 0;
    effect.constant.level = 0;
    effect.constant.length = 0; 
    effect.constant.attack_length = 0;
    effect.constant.fade_length = 0;

    // Upload the effect
    effect_id = SDL_CreateHapticEffect(haptic, &effect);

    return effect_id;
}

int FFBPlugin::update_constant_ffb_effect(float force, int length, int effect_id){
    if (!force_feedback || !constant_force || effect_id == -1){
        return -1;
    }

    // clamp ffb force between -1 and 1
    if (force > 1.0f) force = 1.0f;
    else if (force < -1.0f) force = -1.0f;

    // Use sdl infinity if length == 0
    if (length == 0) {
        length = SDL_HAPTIC_INFINITY;
    }

    effect.constant.level = (short) (force * 32767.0f);
    effect.constant.length = length;

    // SDL3 Update returns bool (true on success)
    if (!SDL_UpdateHapticEffect(haptic, effect_id, &effect)) {
        return -1;
    }
    return 0;
}

int FFBPlugin::play_constant_ffb_effect(int effect_id, int iterations){
    if (!force_feedback || !constant_force || effect_id == -1){
        return -1;
    }

    if (iterations == 0){
        iterations = SDL_HAPTIC_INFINITY;
    }

    // SDL3 Run returns bool (true on success)
    if (!SDL_RunHapticEffect(haptic, effect_id, iterations)){
        return -1;
    }

    return 0;
}

void FFBPlugin::destroy_ffb_effect(int effect_id){
    if (haptic && effect_id != -1) {
        SDL_DestroyHapticEffect(haptic, effect_id);
    }
}

void FFBPlugin::close_ffb_device(){
    if (haptic) {
        SDL_CloseHaptic(haptic);
        haptic = nullptr;
    }
    if (joy) {
        SDL_CloseJoystick(joy);
        joy = nullptr;
    }
}

void FFBPlugin::stop_ffb_effect(int effect_id) {
    if (haptic && effect_id != -1) SDL_StopHapticEffect(haptic, effect_id);
}