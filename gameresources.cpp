#include "gameresources.h"
#include "map.h"
#include "character.h"
#include "eventhandler.h"
#include "pathfinding/pathfinder.h"
#include "pathfinding/pos.h"

#include <deque>
#include <iostream>
#include <cmath>
#include <cstring>

#include <SDL2/SDL.h>
#include <SDL2/extensions/SDL_image.h>

#define PATHFINDING


GameResources::GameResources(const SDLResources& sdl): state(GAME_GOOD), map("maps/map1.conf"), chaser_timer(0), runner(8, 42), runner_timer(0){
	entities.resize(5, Entity());
	SDL_Renderer* r = sdl.GetRenderer();
	//config map
	map.SetRenderEntities(&entities.at(0), &entities.at(1));
	SDL_Surface* surface = IMG_Load("textures/path.png");
	if(surface==nullptr){
		state = GAME_IMG_LOAD_FAIL;
		return;
	}
	map.path->texture = SDL_CreateTextureFromSurface(r, surface);
	SDL_QueryTexture(map.path->texture, nullptr, nullptr, &map.path->srect.w, &map.path->srect.h);
	SDL_FreeSurface(surface);
	surface = IMG_Load("textures/wall.png");
	if(surface==nullptr){
		state = GAME_IMG_LOAD_FAIL;
		return;
	}
	map.wall->texture = SDL_CreateTextureFromSurface(r, surface);
	SDL_FreeSurface(surface);
	SDL_QueryTexture(map.wall->texture, nullptr, nullptr, &map.wall->srect.w, &map.wall->srect.h);
	//characters
	runner.SetRenderEntity(&entities.at(2));
	surface = IMG_Load("textures/runner.png");
	if(surface==nullptr){
		state = GAME_IMG_LOAD_FAIL;
		return;
	}
	runner.render_entity->texture = SDL_CreateTextureFromSurface(r, surface);
	SDL_QueryTexture(runner.render_entity->texture, nullptr, nullptr, &runner.render_entity->srect.w, &runner.render_entity->srect.h);
	SDL_FreeSurface(surface);

	chasers.reserve(2);
	chasers.push_back(std::move(Character(39, 0)));
	chasers[0].SetRenderEntity(&entities.at(3));
	surface = IMG_Load("textures/chaser.png");
	if(surface==nullptr){
		state = GAME_IMG_LOAD_FAIL;
		return;
	}
	chasers[0].render_entity->texture = SDL_CreateTextureFromSurface(r, surface);
	SDL_QueryTexture(chasers[0].render_entity->texture, nullptr, nullptr, &chasers[0].render_entity->srect.w, &chasers[0].render_entity->srect.h);
	SDL_FreeSurface(surface);
	chasers.push_back(std::move(Character(36, 2)));
	chasers[1].SetRenderEntity(&entities.at(4));
	surface = IMG_Load("textures/chaser.png");
	if(surface==nullptr){
		state = GAME_IMG_LOAD_FAIL;
		return;
	}
	chasers[1].render_entity->texture = SDL_CreateTextureFromSurface(r, surface);
	SDL_QueryTexture(chasers[1].render_entity->texture, nullptr, nullptr, &chasers[1].render_entity->srect.w, &chasers[1].render_entity->srect.h);
	SDL_FreeSurface(surface);

}

GameResources::~GameResources(){


}


void GameResources::Update(const EventHandler& events, const SDLResources& sdl){
	//game end check
	for(int i=0; i<chasers.size(); ++i){
		if(chasers[i].row==runner.row && chasers[i].col==runner.col){
			state = GAME_OVER;
			return;
		}
	}
	int window_width = sdl.WindowW();
	int window_height = sdl.WindowH();
	int box_width = window_width/map.cols;
	int box_height = window_height/map.rows;
	//update map
	Entity& map_path = *map.path;
	Entity& map_wall = *map.wall;
	map_path.drect.w = box_width;
	map_path.drect.h = box_height;
	map_wall.drect.w = box_width;
	map_wall.drect.h = box_height;
	//update runner
	if(runner.timer++ % (10-runner.speed) == 0){
		const std::vector<float>& mapping = map.GetMapping();
		if(events[EventHandler::S] && runner.row<map.rows && mapping[(runner.row+1)*map.cols+runner.col]==-1.0){
			runner.row += 1;
			runner.render_entity->angle = 180.0;
		}
		//runner.row -= (events[EventHandler::W] && runner.row>0 && mapping[(runner.row-1)*map.cols+runner.col]==-1.0)? 1 : 0;
		if(events[EventHandler::W] && runner.row>0 && mapping[(runner.row-1)*map.cols+runner.col]==-1.0){
			runner.row -= 1;
			runner.render_entity->angle = 0.0;
		}
		//runner.col += (events[EventHandler::D] && runner.col<map.cols && mapping[runner.row*map.cols+runner.col+1]==-1.0) ? 1 : 0;
		if(events[EventHandler::D] && runner.col<map.cols && mapping[runner.row*map.cols+runner.col+1]==-1.0){
			runner.col += 1;
			runner.render_entity->angle = 90.0;
		}
		//runner.col -= (events[EventHandler::A] && runner.col>0 && mapping[runner.row*map.cols+runner.col-1]==-1.0) ? 1 : 0;
		if(events[EventHandler::A] && runner.col>0 && mapping[runner.row*map.cols+runner.col-1]==-1.0){
			runner.col -= 1;
			runner.render_entity->angle = 270.0;
		}
		Entity& runner_entity = *runner.render_entity;
		runner_entity.drect.x = window_width*runner.col/map.cols;
		runner_entity.drect.y = window_height*runner.row/map.rows;
		runner_entity.drect.w = box_width;
		runner_entity.drect.h = box_height;
	}
	//update chaser
	for(int i=0; i<chasers.size(); ++i){
		Character& chaser = chasers[i];
		if(chaser.timer++%(10-chaser.speed)==0 && sqrt(pow(runner.row-chaser.row,2)+pow(runner.col-chaser.col,2))<30.0f){
			const std::vector<float>& mapping = map.GetMapping();
#ifdef PATHFINDING
			if(path_finder.FindPath(mapping, map.rows, map.cols, chaser.row, chaser.col, runner.row, runner.col)){
				const std::vector<Kha::Pos>& route = path_finder.GetRoute();
				chaser.row = route.at(0).row;
				chaser.col = route.at(0).col;
			}
#endif
			Entity& chaser_entity = *chaser.render_entity;
			chaser_entity.drect.x = window_width*chaser.col/map.cols;
			chaser_entity.drect.y = window_height*chaser.row/map.rows;
			chaser_entity.drect.w = box_width;
			chaser_entity.drect.h = box_height;
		}
	}
	if(...){
		level_complete = true;
	}
}

void GameResources::Render(const SDLResources& sdl){
	SDL_Renderer* renderer = sdl.GetRenderer();
	int window_width = sdl.WindowW();
	int window_height = sdl.WindowH();
	SDL_RenderClear(renderer);
	//draw map
	const std::vector<float>& mapping = map.GetMapping();
	int size = mapping.size();
	for(int i=0; i<size; ++i){
		Entity& entity = entities[mapping[i]+2];
		entity.drect.x = window_width * (i%map.cols) / map.cols;
		entity.drect.y = window_height * (i/map.cols) / map.rows;
		SDL_RenderCopyEx(renderer, entity.texture, &entity.srect, &entity.drect, entity.angle, nullptr, entity.flip);
	}
	//draw characters
	size = entities.size();
	for(int i=2; i<size; ++i){
		Entity& entity = entities.at(i);
		if(!entity.enabled) continue;
		SDL_RenderCopyEx(renderer, entity.texture, &entity.srect, &entity.drect, entity.angle, nullptr, entity.flip);
	}
	SDL_RenderPresent(renderer);
	if(level_complete){
		LoadNextLevel();
		level_complete = false;
	}
}

uint8_t GameResources::State() const{
	return state;
}

void GameResources::LoadNextLevel(){
	//settings
	std::string map_dimensions = "";
	std::vector<std::string> map_mapping;
	std::string map_entrance = "";
	std::string map_exit= "";
	std::vector<std::string> chasers;
	std::string new_level = "";
	//parse file
	std::ifstream ifs(next_level.c_str());
	std::string line = "";
	bool parse_complete = false;
	while(!parse_complete){
		getline(ifs, line);
		if(line.at(0)=='>'){
			switch(line){
				case ">map_dimensions":
					getline(ifs, line);
					int cols = 0; int rows = 0;
					std::stringstream(line)>>cols>>rows;
					map.cols = cols; map.rows = rows;
					break;
				case ">chasers":
			}
		}else{

		}
	}




	chasers.clear();
	if(next_level == ""){
		state = GAME_COMPLETE;
		return;
	};
	std::vector<std::string> file_content;
	std::ifstream ifs(next_level.c_str());
	std::string line = "";
	while(getline(ifs, line)){
		file_content.push_back(line);
	}
	//map properties
	int rows = 0; int cols = 0; std::string mapping_str = "";
	for(int i=0; i<file_content.size(); ++i){
		if(file_content.at(i).at(0)=='>'){
			switch(file_content.at(i)){
				case ">map_dimensions":
					std::stringstream(file_content[++i])>>rows>>cols;
					break;
				case ">map_mapping":
				case ">map_entrance":
				case ">map_exit":
				case ">chasers":
					++i;
					while(file_content.at(i).at(0)!='>'){
						int r = 0; int c = 0;
						std::stringstream(file_content[i++])>>r>>c;
						chasers.push_back(Character(r,c));
						chasers[].texture = ...;
					}
					break;
				case ">new_level":
					
			}
		}else{
			state = GAME_LOAD_LEVEL_FAIL;
			break;
		}
	}
}
