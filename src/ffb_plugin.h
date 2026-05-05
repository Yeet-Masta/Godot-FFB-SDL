#ifndef FFB_PLUGIN_H
#define FFB_PLUGIN_H

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_haptic.h>
#include <SDL3/SDL_joystick.h>

#include "godot_cpp/classes/node.hpp"

namespace godot {
class FFBPlugin : public Node {
  GDCLASS(FFBPlugin, Node)

public:
  FFBPlugin();
  ~FFBPlugin();
  void _init();

  int init_ffb(int p_device);
  int init_constant_force_effect();

  int update_constant_ffb_effect(float force, int length, int effect_id);
  int play_constant_ffb_effect(int effect_id, int iterations);

  void destroy_ffb_effect(int effect_id);
  void close_ffb_device();
  void stop_ffb_effect(int effect_id);

  bool has_force_feedback() const { return force_feedback; }
  bool has_constant_force() const { return constant_force; }

protected:
  static void _bind_methods();

private:
  SDL_Haptic *haptic = nullptr;
  SDL_Joystick *joy = nullptr;
  SDL_HapticEffect effect;

  int autocenter = 0;
  bool force_feedback = false;
  bool constant_force = false;
};
} // namespace godot
#endif // FFB_PLUGIN_H