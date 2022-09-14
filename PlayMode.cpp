#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>



GLuint cyber_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > cyber_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("CyberSauras.pnct"));
	cyber_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > cyber_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("CyberSauras.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = cyber_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = cyber_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

float PlayMode::float_abs(float val) {
	if (val < 0.0f) { return -1.0f*val;}
	return val;
}


// Get transforms
void PlayMode::get_transforms() {
	for (auto &transform : scene.transforms) {
		// Platform
		if (transform.name == "Platform_Base") platform = &transform;
		else if (transform.name == "Left_Bound") left_bound_obj = &transform;
		else if (transform.name == "Right_Bound") right_bound_obj = &transform;

		// Player
		else if (transform.name == "Player_Leg_L") player_left_leg = &transform;
		else if (transform.name == "Player_Leg_R") player_right_leg = &transform;
		else if (transform.name == "Player_Head") player_head = &transform;
		else if (transform.name == "Player_Torso") player_torso = &transform;

		else {
			for(uint32_t i = 0; i < 6; i++) {
				std::string obstacle_str = std::string("Obstacle_") + std::to_string(i + 1);
				std::string enemye_str = std::string("EnemyE_Body_") + std::to_string(i + 1);
				std::string enemys_str = std::string("EnemyS_Body_") + std::to_string(i + 1);
				if(transform.name == obstacle_str) obstacle[i] = &transform;
				if(transform.name == enemye_str) enemy_eatable[i] = &transform;
				if(transform.name == enemys_str) enemy_shooter[i] = &transform;
			}
			for(uint32_t i = 0; i < 20; i++) {
				std::string laser_str = std::string("Mball_") + std::to_string(i + 1);
				if(transform.name == laser_str) laser[i] = &transform;
			}
		}
		
	}
	
	if (platform == nullptr) throw std::runtime_error("platform not found.");
	if (left_bound_obj == nullptr) throw std::runtime_error("left_bound_obj not found.");
	if (right_bound_obj == nullptr) throw std::runtime_error("right_bound_obj leg not found.");
	
	if (player_left_leg == nullptr) throw std::runtime_error("player_left_leg not found.");
	if (player_right_leg == nullptr) throw std::runtime_error("player_right_leg not found.");
	if (player_head == nullptr) throw std::runtime_error("player_head not found.");
	if (player_torso == nullptr) throw std::runtime_error("player_torso not found.");

	for(uint32_t i = 0; i < 6; i++) {
		std::string obstacle_msg = std::string("Obstacle_") + std::to_string(i + 1) + std::string(" not found.");
		std::string enemye_msg = std::string("EnemyE_Body_") + std::to_string(i + 1) + std::string(" not found.");
		std::string enemys_msg = std::string("EnemyS_Body_") + std::to_string(i + 1) + std::string(" not found.");
		if (obstacle[i] == nullptr) throw std::runtime_error(obstacle_msg);
		if (enemy_eatable[i] == nullptr) throw std::runtime_error(enemye_msg);
		if (enemy_shooter[i] == nullptr) throw std::runtime_error(enemys_msg);
	}
	for(uint32_t i = 0; i < 20; i++) {
		std::string laser_msg = std::string("Mball_") + std::to_string(i + 1) + std::string(" not found");
		if (laser[i] == nullptr) throw std::runtime_error(laser_msg);
	}

	left_leg_rotation = player_left_leg->rotation;
	right_leg_rotation = player_right_leg->rotation;
	
	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
}


// Hide objects away from camera
void PlayMode::hide_object(Scene::Transform *object) {
	object->position.x = 350.0f;
	object->position.y = -350.0f;
}


// Check if collision happens
int PlayMode::is_collided(Scene::Transform *obj1, Scene::Transform *obj2, float scalex, float scaley) {
	if((float_abs(obj1->position.x - obj2->position.x) <= (object_size + scalex - 1.0f))) 
		if((float_abs(obj1->position.y - obj2->position.y) <= (object_size + scaley)))
			return 1;
	
	return 0;
}

// Called every init or game over
void PlayMode::init_state(int state) {
	
	// Initialize values
	wobble = 0.0f;
	wobble_factor = 14.0f;

	speedup = 1.09f;
	
	PlayerSpeed = 0.7f;
	EnemySpeed = 0.7f;

	score = 0;
	fuel = 100;
	update_val = 0;
	score_update_rate = 10;
	level_update_rate = 200;
	fuel_update_rate = 50;
	laser_speed = EnemySpeed*2.0f;

	enemy_update_rate = 50.0f;
	enemy_frame_counter = 0.0f;
	enemy_frame_rate = 0.5f;

	enemy_laser_update_rate = 50.0f;
	enemy_laser_frame_counter = 0.0f;
	enemy_laser_frame_rate = 0.5f;

	object_size = 2.0f;
	space_debounce = 0;

	collision_scale_x = 1.0;

	
	if(state == 0) {  // Run only on first init - get transforms and set initial asset positions
		camera = nullptr;
		platform = nullptr;
		left_bound_obj = nullptr;
		right_bound_obj = nullptr;

		player_head = nullptr;
		player_torso = nullptr;
		player_left_leg = nullptr;
		player_right_leg = nullptr;

		for(uint32_t i = 0; i < 6; i++) {
			obstacle[i] = nullptr;
			enemy_eatable[i] = nullptr;
			enemy_shooter[i] = nullptr;
		}

		for(uint32_t i = 0; i < 20; i++) {
			laser[i] = nullptr;
		}

		//Get Transform Pointers for all objects only once
		get_transforms();

		for(uint32_t i = 0; i < 6; i++) {
		enemy_eatable[i]->position.z = player_head->position.z;
		}
		for(uint32_t i = 0; i < 20; i++) {
			laser[i]->position.z = enemy_shooter[0]->position.z;
		}
		camera->transform->position.x = -190.143188f;
		camera->transform->position.y = 0.052472f;
		camera->transform->position.z = 56.843361f;

		bound_left = 36.0f + 500.0f;
		bound_right = -36.0f + 500.0f;

		bound_front = 250.0f + 100.0f;
		bound_back = -17.0f + 100.0f;

		left_bound_obj->position.y += 500.0f;
		right_bound_obj->position.y += 500.0f;
		platform->position.y += 500.0f;
		player_head->position.y += 500.0f;
		player_torso->position.y += 500.0f;
		player_left_leg->position.y += 500.0f;
		player_right_leg->position.y += 500.0f;
		camera->transform->position.y += 500.0f;

		left_bound_obj->position.x += 100.0f;
		right_bound_obj->position.x += 100.0f;
		platform->position.x += 100.0f;
		player_head->position.x += 100.0f;
		player_torso->position.x += 100.0f;
		player_left_leg->position.x += 100.0f;
		player_right_leg->position.x += 100.0f;
		camera->transform->position.x += 100.0f;
	}

	
	
	
	for(uint32_t i = 0; i < 6; i++) {
		hide_object(obstacle[i]);
		hide_object(enemy_eatable[i]);
		hide_object(enemy_shooter[i]);
	}
	for(uint32_t i = 0; i < 20; i++) {
		hide_object(laser[i]);
	}

}


// Speed update
void PlayMode::level_up(){
	wobble_factor *= speedup;
	PlayerSpeed *= speedup;
	EnemySpeed *= speedup;
	enemy_frame_rate *= speedup;
	enemy_laser_frame_rate *= speedup;
	laser_speed *= speedup;
	collision_scale_x *= speedup;
}

// Reset state
void PlayMode::game_over(){
	init_state(1);
}


PlayMode::PlayMode() : scene(*cyber_scene) {
	init_state(0);
}

PlayMode::~PlayMode() {
}


bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE && space_debounce == 0) { // Flag to ensure one action per key press
			space.downs += 1;
			space.pressed = true;
			space_debounce = 1;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = false;
			space_debounce = 0;
			return true;
		}
	} 

	return false;
}

void PlayMode::update(float elapsed) {


	//=========================
	//Player leg movement Logic
	//=========================

	//slowly rotates through [0,1):
	wobble += elapsed / 10.0f;
	wobble -= std::floor(wobble);
	
	// Following function is used to rotate the legs for run animation
	player_left_leg->rotation = left_leg_rotation * glm::angleAxis(
		glm::radians(25.0f * std::sin(wobble * wobble_factor * 2.0f * float(M_PI))),
		glm::vec3(1.0f, 0.0f, 0.0f)
	);
	player_right_leg->rotation = right_leg_rotation * glm::angleAxis(
		glm::radians(-25.0f * std::sin(wobble * wobble_factor * 2.0f * float(M_PI))),
		glm::vec3(1.0f, 0.0f, 0.0f)
	);

	//=========================
	// Player Movement Logic
	//=========================

	//combine inputs into a move:
	glm::vec2 move = glm::vec2(0.0f);
	if (left.pressed && !right.pressed) move.y =1.0f;
	if (!left.pressed && right.pressed) move.y = -1.0f;
	if (down.pressed && !up.pressed) move.x =-1.0f;
	if (!down.pressed && up.pressed) move.x = 1.0f;

	if (move != glm::vec2(0.0f)) move = move * PlayerSpeed; // Multiply by player speed
	
	//Check bounds left positive, right negative - y , top x pos, bottom x neg
	if(player_left_leg->position.y + move.y >= bound_left)
	{
		move.y = bound_left - player_left_leg->position.y;
	}
	if(player_right_leg->position.y + move.y <= bound_right)
	{
		move.y = bound_right - player_right_leg->position.y;
	}
	if(player_head->position.x + move.x >= bound_front)
	{
		move.x = bound_front - player_head->position.x;
	}
	if(player_head->position.x + move.x <= bound_back) 
	{
		move.x = bound_back - player_head->position.x;
	}
	
	player_head->position.x += move.x;
	player_head->position.y += move.y;
	player_torso->position.x += move.x;
	player_torso->position.y += move.y;
	player_left_leg->position.x += move.x;
	player_left_leg->position.y += move.y;
	player_right_leg->position.x += move.x;
	player_right_leg->position.y += move.y;
	

	//============================
	// Score and fuel update logic
	//============================

	update_val += 1;

	// Update score every 10 calls
	if(update_val % score_update_rate == 0) {
		score += 1;
	}

	// Update fuel every 50 calls
	if(update_val % fuel_update_rate == 0) {
		fuel--;
	}

	// Game over logic
	if(fuel <= 0) game_over();

	// Level Up logic
	if(score % level_update_rate == 0 && score != 0) level_up();
	
	

	//=========================
	// Enemy selection logic
	//=========================
	
	enemy_frame_counter += enemy_frame_rate; // Determines enemy spawn rate

	if(enemy_frame_counter >= enemy_update_rate) {
		enemy_frame_counter = 0.0f;
		randnum = std::rand() % 10;
		float enemy_pos = 1.0f*(std::rand() % int(bound_left - bound_right)) + bound_right;
		//printf("Position = %f\n", enemy_pos);
		if(weight[randnum] == 0) { // Obstacle
			for(uint32_t i = 0; i < 6; i++) {
				if(obstacle[i]->position.y == -350.0f) {
					obstacle[i]->position.y = enemy_pos;
					break;
				}
			}
		}
		if(weight[randnum] == 1) { // Enemy Eater
			for(uint32_t i = 0; i < 6; i++) {
				if(enemy_eatable[i]->position.y == -350.0f) {
					enemy_eatable[i]->position.y = enemy_pos;
					break;
				}
			}
		}
		if(weight[randnum] == 2) { // Enemy Shooter
			for(uint32_t i = 0; i < 6; i++) {
				if(enemy_shooter[i]->position.y == -350.0f) {
					enemy_shooter[i]->position.y = enemy_pos;
					break;
				}
			}
		}
	}
	
	//=========================
	// Enemy movement logic
	//=========================
	for(uint32_t i = 0; i < 6; i++) {
		if(obstacle[i]->position.y != -350.0f) {
			obstacle[i]->position.x -= EnemySpeed;
			if(obstacle[i]->position.x < bound_back)
				hide_object(obstacle[i]);
		}
	}
	for(uint32_t i = 0; i < 6; i++) {
		if(enemy_eatable[i]->position.y != -350.0f) {
			enemy_eatable[i]->position.x -= EnemySpeed;
			if(enemy_eatable[i]->position.x < bound_back)
				hide_object(enemy_eatable[i]);
		}
	}
	for(uint32_t i = 0; i < 6; i++) {
		if(enemy_shooter[i]->position.y != -350.0f) {
			enemy_shooter[i]->position.x -= EnemySpeed;
			if(enemy_shooter[i]->position.x < bound_back)
				hide_object(enemy_shooter[i]);
		}
	}


	//=========================
	// Enemy collision logic
	//=========================
	for(uint32_t i = 0; i < 6; i++) {
		if(is_collided(obstacle[i], player_right_leg, collision_scale_x, 0.0f) == 1 || is_collided(obstacle[i], player_left_leg, collision_scale_x, 0.0f) == 1 || is_collided(obstacle[i], player_head, collision_scale_x, 0.0f) == 1 || is_collided(obstacle[i], player_torso, collision_scale_x, 0.0f) == 1)
		{
			game_over();
		}
	}
	
	for(uint32_t i = 0; i < 6; i++) {
		if(is_collided(enemy_eatable[i], player_right_leg, collision_scale_x, 0.0f) == 1 || is_collided(enemy_eatable[i], player_left_leg, collision_scale_x, 0.0f) == 1 || is_collided(enemy_eatable[i], player_head, collision_scale_x, 0.0f) == 1 || is_collided(enemy_eatable[i], player_torso, collision_scale_x, 0.0f) == 1)
		{
			fuel+= 10;
			hide_object(enemy_eatable[i]);
			if(fuel > 100) fuel = 100;
		}
	}
	for(uint32_t i = 0; i < 6; i++) {
		if(is_collided(enemy_shooter[i], player_right_leg, collision_scale_x, 0.0f) == 1 || is_collided(enemy_shooter[i], player_left_leg, collision_scale_x, 0.0f) == 1 || is_collided(enemy_shooter[i], player_head, collision_scale_x, 0.0f) == 1 || is_collided(enemy_shooter[i], player_torso, collision_scale_x, 0.0f) == 1)
		{
			game_over();
		}
	}




	//=========================
	// Player shooting logic
	//=========================
	if(space.pressed && space_debounce == 1) {
		space_debounce = 2;
		for(uint32_t i = 0; i < 20; i++) {
			if(laser[i]->position.y == -350.0f) {
				laser[i]->position.x = player_head->position.x + laser_speed;
				laser[i]->position.y = player_head->position.y;
				fuel -= 5;
				if(fuel <= 0) game_over();
				laser_movement_dir[i] = 1; //Forward
				break;
			}
		}
	}

	
	//=========================
	// Enemy shooting logic
	//=========================
	enemy_laser_frame_counter += enemy_laser_frame_rate; // Determines enemy laser spawn rate

	if(enemy_laser_frame_counter >= enemy_laser_update_rate) {
		enemy_laser_frame_counter = 0.0f;
		for(uint32_t i = 0; i < 6; i++) {
			if(enemy_shooter[i]->position.y != -350.0f) {
				for(uint32_t j = 0; j < 20; j++) {
					if(laser[j]->position.y == -350.0f) {
						laser[j]->position.x = enemy_shooter[j]->position.x - laser_speed;
						laser[j]->position.y = enemy_shooter[j]->position.y;
						laser_movement_dir[j] = 0; //Backward
						break;
					}
				}
			}
		}
	}

	//=========================
	// Laser Movement
	//=========================
	for(uint32_t i = 0; i < 20; i++) {
		if(laser[i]->position.y != -350.0f) {
			if(laser_movement_dir[i] == 1) // Forward from player
				laser[i]->position.x += laser_speed;
			else if(laser_movement_dir[i] == 0) // Backward from enemy
				laser[i]->position.x -= laser_speed;
		}
	}

	//=========================
	// Laser Collision 
	//========================= 
	for(uint32_t i = 0; i < 20; i++) {
		if(laser[i]->position.y != -350.0f){
			for(uint32_t j = 0; j < 6; j++) // To enemy
			{
					if(is_collided(obstacle[j], laser[i], collision_scale_x, 1.0f) == 1) {
						hide_object(obstacle[j]);
						hide_object(laser[i]);
						continue;
					}
					if(is_collided(enemy_eatable[j], laser[i], collision_scale_x, 1.0f) == 1) {
						hide_object(enemy_eatable[j]);
						hide_object(laser[i]);
						continue;
					}
					if(is_collided(enemy_shooter[j], laser[i], collision_scale_x, 1.0f) == 1) {
						hide_object(enemy_shooter[j]);
						hide_object(laser[i]);
						continue;
					}
			}
			// From enemy
			if(is_collided(player_head, laser[i], collision_scale_x, 1.0f) == 1) {
				game_over();
			}
			if(is_collided(player_torso, laser[i], collision_scale_x, 1.0f) == 1) {
				game_over();
			}
		}	
	}

	


	//=========================
	// Hide laser
	//=========================
	for(uint32_t i = 0; i < 20; i++) {
		if(laser[i]->position.y != -350.0f) {
			if(laser[i]->position.x < bound_back || laser[i]->position.x > bound_front)
				hide_object(laser[i]);
		}
	}



	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	space.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		std::string score_string = std::string("Score:") + std::to_string(score); // To tell score
		lines.draw_text(score_string,
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		
		std::string fuel_string = std::string("Fuel Left:") + std::to_string(fuel); // To tell fuel amount
		float ofs = 2225.0f / drawable_size.y;
		lines.draw_text(fuel_string,
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
}
