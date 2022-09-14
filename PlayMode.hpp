#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	// Init state values
	void init_state(int state);

	// Load all transform variables
	void get_transforms();

	// Hide object
	void hide_object(Scene::Transform *object);

	// Level up logic
	void level_up();

	// Game over logic
	void game_over();

	// Get absolue float value
	float float_abs(float val);

	// Check if collision happened between two objects. 
	int is_collided(Scene::Transform *obj1, Scene::Transform *obj2, float scalex, float scaley);
	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, space;

	// Flag for debounce
	uint32_t space_debounce;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;


	// Get Game Bounds
	float bound_back;
	float bound_front;
	float bound_left;
	float bound_right;
	float object_size;

	// Collision update scale
	float collision_scale_x;

	// Transforms for all objects
	Scene::Transform *platform;
	Scene::Transform *left_bound_obj;
	Scene::Transform *right_bound_obj;

	Scene::Transform *player_head;
	Scene::Transform *player_torso;
	Scene::Transform *player_left_leg;
	Scene::Transform *player_right_leg;

	Scene::Transform* obstacle[6];

	Scene::Transform* enemy_eatable[6];

	Scene::Transform* enemy_shooter[6];

	Scene::Transform* laser[20];
	
	// To rotate player legs to simulate running
	glm::quat left_leg_rotation;
	glm::quat right_leg_rotation;
	float wobble;
	float wobble_factor;	

	// Level Up factors
	//uint32_t level = 0;
	float speedup;

	// Movement Factors
	float PlayerSpeed;
	float EnemySpeed;


	// Laser 
	float laser_speed;
	uint32_t laser_movement_dir[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	float enemy_laser_update_rate;
	float enemy_laser_frame_counter;
	float enemy_laser_frame_rate;
	
	// Enemy Spawn
	float enemy_update_rate;
	float enemy_frame_counter;
	float enemy_frame_rate;



	// Weighted random array
	uint32_t weight[10] = {0,0,0,0,1,1,1,2,2,2};; // O - obstacle, 1 - eate, 2 - eats
	uint32_t randnum;

	// Fuel and Score
	uint32_t score;
	int fuel;
	
	// UI updates
	uint32_t update_val;
	uint32_t score_update_rate;
	uint32_t level_update_rate;
	uint32_t fuel_update_rate;


	//camera
	Scene::Camera *camera;

};
