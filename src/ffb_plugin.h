#ifndef FFB_PLUGIN_H
#define FFB_PLUGIN_H

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_haptic.h>
#include <SDL3/SDL_joystick.h>
#include <godot_cpp/classes/node.hpp>

namespace godot {

class FFBPlugin : public Node {
	GDCLASS(FFBPlugin, Node)

public:
	FFBPlugin();
	~FFBPlugin();

	void _init();
	
	// Initialization
	int init_ffb(int p_device);
	void close_ffb_device();
	
	// Constant Effect
	int init_constant_force_effect();
	int update_constant_ffb_effect(float force, int length, int effect_id);
	int play_constant_ffb_effect(int effect_id, int iterations);
	
	// Periodic Effect (Sine, Square, etc.)
	int init_periodic_effect(int p_type);
	int update_periodic_effect(int p_type, float magnitude, float offset, int period, int length, int effect_id);
	
	// Condition Effect (Spring, Damper, etc.)
	int init_condition_effect(int p_type);
	int update_condition_effect(int p_type, float right_sat, float left_sat, float right_coeff, float left_coeff, float deadband, int effect_id);
	
	// Ramp Effect
	int init_ramp_effect();
	int update_ramp_effect(float start, float end, int length, int effect_id);
	
	// Device Control
	int set_gain(int p_gain);
	int set_autocenter(int p_autocenter);
	void pause_haptic();
	void resume_haptic();
	void stop_all_effects();
	void stop_ffb_effect(int effect_id);
	void destroy_ffb_effect(int effect_id);
	
	// Queries
	int get_max_effects() const;
	int get_num_axes() const;
	bool has_feature(int p_feature) const;
	bool has_force_feedback() const { return force_feedback; }
	bool has_constant_force() const { return constant_force; }

protected:
	static void _bind_methods();

private:
	SDL_Haptic *haptic = nullptr;
	SDL_Joystick *joy = nullptr;
	
	// We store a generic effect struct for updates, but for complex effects 
	// it's often safer to reconstruct the struct on update or store specific ones.
	// For this implementation, we will reconstruct structs in update functions 
	// to ensure thread/state safety, so we don't strictly need a member struct 
	// except for simple constant updates if desired. 
	SDL_HapticEffect effect; 
	
	bool force_feedback = false;
	bool constant_force = false;
};

} // namespace godot

#endif // FFB_PLUGIN_H