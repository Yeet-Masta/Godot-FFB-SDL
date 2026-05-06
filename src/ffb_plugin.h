#ifndef FFB_PLUGIN_H
#define FFB_PLUGIN_H

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_haptic.h>
#include <SDL3/SDL_joystick.h>

#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/variant/packed_string_array.hpp"

namespace godot {
class FFBPlugin : public Node {
  GDCLASS(FFBPlugin, Node)

public:
  // Wave-type constants for init_periodic_effect (mirror SDL_HAPTIC_* bits).
  // Bound as Godot integer constants so they're usable from GDScript
  // as FFBPlugin.WAVE_SINE, etc.
  static constexpr int WAVE_SINE         = 1 << 1;
  static constexpr int WAVE_SQUARE       = 1 << 2;
  static constexpr int WAVE_TRIANGLE     = 1 << 3;
  static constexpr int WAVE_SAWTOOTHUP   = 1 << 4;
  static constexpr int WAVE_SAWTOOTHDOWN = 1 << 5;

  // Condition-type constants for init_condition_effect.
  static constexpr int CONDITION_SPRING   = 1 << 7;
  static constexpr int CONDITION_DAMPER   = 1 << 8;
  static constexpr int CONDITION_INERTIA  = 1 << 9;
  static constexpr int CONDITION_FRICTION = 1 << 10;

  FFBPlugin();
  ~FFBPlugin();
  void _init();

  // ---- Device enumeration (callable BEFORE init_ffb) ----
  PackedStringArray get_joystick_names();
  bool is_joystick_haptic(int p_device);

  // ---- Device lifecycle ----
  int init_ffb(int p_device);
  void close_ffb_device();

  // ---- Device introspection (call after init_ffb) ----
  String get_device_name() const;
  int get_num_axes() const;
  int get_max_effects() const;
  int get_max_effects_playing() const;
  int get_features_bitmask() const;
  bool supports_effect_type(int p_type) const;
  bool is_rumble_supported() const;

  // ---- Global device controls ----
  int set_gain(int p_gain);             // 0-100, master gain for all effects
  int set_autocenter(int p_autocenter); // 0-100, hardware autocenter
  int pause();
  int resume();
  int stop_all_effects();

  // ---- Constant force (existing API, unchanged) ----
  int init_constant_force_effect();
  int update_constant_ffb_effect(float force, int length, int effect_id);
  int play_constant_ffb_effect(int effect_id, int iterations);

  // ---- Ramp effect (force ramps linearly from start to end) ----
  int init_ramp_effect(float start, float end, int length);
  int update_ramp_effect(int effect_id, float start, float end, int length);

  // ---- Periodic effect (sine / square / triangle / sawtooth) ----
  int init_periodic_effect(int wave_type, int period_ms, float magnitude, int length);
  int update_periodic_effect(int effect_id, int wave_type, int period_ms, float magnitude, int length);

  // ---- Condition effects (spring / damper / inertia / friction share one struct) ----
  int init_condition_effect(int condition_type,
                            float left_coeff, float right_coeff,
                            float left_sat, float right_sat,
                            float deadband, float center,
                            int length);
  int update_condition_effect(int effect_id, int condition_type,
                              float left_coeff, float right_coeff,
                              float left_sat, float right_sat,
                              float deadband, float center,
                              int length);

  // ---- Generic effect control (works on ANY effect type) ----
  int run_effect(int effect_id, int iterations);
  int stop_ffb_effect(int effect_id);
  void destroy_ffb_effect(int effect_id);
  bool is_effect_playing(int effect_id);

  // ---- Simple rumble API (no effect management; for gamepads) ----
  int init_rumble();
  int play_rumble(float strength, int length);
  int stop_rumble();

  // Convenience accessors
  bool has_force_feedback() const { return force_feedback; }
  bool has_constant_force() const { return constant_force; }
  int get_autocenter() const { return autocenter; }

protected:
  static void _bind_methods();

private:
  SDL_Haptic *haptic = nullptr;
  SDL_Joystick *joy = nullptr;
  SDL_HapticEffect effect; // Used by the existing constant-force API

  int autocenter = 0;
  bool force_feedback = false;
  bool constant_force = false;

  // Helpers for normalized float -> SDL fixed-point
  static Sint16 to_s16(float v);          // [-1, 1] -> [-32767, 32767]
  static Uint16 to_u16(float v);          // [ 0, 1] -> [0, 65535]
  static Uint32 to_length(int length_ms); // 0 -> SDL_HAPTIC_INFINITY
};
} // namespace godot
#endif // FFB_PLUGIN_H