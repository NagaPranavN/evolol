#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "style.h"

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 900

#define BOARD_WIDTH 12
#define BOARD_HEIGHT 9

#define CELL_WIDTH ((float) SCREEN_WIDTH / (float) BOARD_WIDTH)
#define CELL_HEIGHT ((float) SCREEN_HEIGHT / (float) BOARD_HEIGHT)

#define AGENTS_COUNT 8
#define FOOD_COUNT 4
#define WALLS_COUNT 4

#define AGENT_PADDING 15.0f

bool scr(bool result)
{
  if(!result){
    const char *err = SDL_GetError();
    fprintf(stderr, "SDL Error: %s\n", err ? err : "unknown error");
    exit(1);
  }
  return result;
}

int scc(int code)
{
  if(code < 0){
    const char *err = SDL_GetError();
    fprintf(stderr, "SDL Error: %s\n", err ? err : "unknown error");
    exit(1);
  }
  return code;
}

void *scp(void *ptr)
{
  if(ptr == NULL){
    const char *err = SDL_GetError();
    fprintf(stderr, "SDL Error: %s\n", err ? err : "unknown error");
    exit(1);
  }
  return ptr;
}

void sdl_set_color_hex(SDL_Renderer *renderer, uint32_t hex)
{   
  scc(SDL_SetRenderDrawColor(
			     renderer,
			     (hex >> (3 * 8)) & 0xFF,
			     (hex >> (2 * 8)) & 0xFF,
			     (hex >> (1 * 8)) & 0xFF,
			     (hex >> (0 * 8)) & 0xFF));
}

typedef enum {
  DIR_RIGHT = 0,
  DIR_UP,
  DIR_LEFT, 
  DIR_DOWN, 
} Dir;

typedef enum{
  ENV_NOTHING = 0,
  ENV_AGENT,
  ENV_FOOD,
  ENV_WALL
} Env;

typedef enum {
  ACTION_NOP = 0,
  ACTION_STEP,
  ACTION_EAT,
  ACTION_ATTACK, 
} Action;

typedef int State;

typedef struct{
  State state;
  Env env;
  Action action;
  State  next_state;

} Brain_Cell;

typedef struct{
  size_t count;
  Brain_Cell cells[]; 
} Brain;

typedef struct {
  int pos_x, pos_y;
  Dir dir;
  int hunger;
  int health;
  State state;
} Agent;

typedef struct{
  int eaten;
  int pos_x;
  int pos_y;
} Food;

typedef struct{
  int pos_x;
  int pos_y;
}Wall;

typedef struct{
  Agent agents[AGENTS_COUNT];
  Food foods[FOOD_COUNT];
  Wall walls[WALLS_COUNT];
} Game;

int random_int_range(int low, int high)
{
  return rand() % (high - low) + low;
}

Dir random_dir(void)
{
  return (Dir) random_int_range(0, 4);
}

Agent random_agent(void)
{
  Agent agent = {0};
  agent.pos_x = random_int_range(0, BOARD_WIDTH);
  agent.pos_y = random_int_range(0, BOARD_HEIGHT);
  agent.dir = random_dir();
  agent.hunger = 100;
  agent.health = 100;
  return agent;
}

Food random_food(void){
  Food food = {0};
  food.pos_x = random_int_range(0, BOARD_WIDTH);
  food.pos_y = random_int_range(0, BOARD_HEIGHT);
  return food;
}

Wall random_wall(void){
  Wall wall  = {0};
  wall.pos_x = random_int_range(0, BOARD_WIDTH);
  wall.pos_y = random_int_range(0, BOARD_HEIGHT);
  return wall;
}

void init_game(Game *game){
  for(int i = 0; i < AGENTS_COUNT; ++i){
    game->agents[i] = random_agent();
    
  }
   for(size_t i = 0; i < FOOD_COUNT; ++i){
    game->foods[i] = random_food();
  }
   for(size_t i = 0; i < WALLS_COUNT; ++i){
     game->walls[i] = random_wall();
  }
}


void render_board_grid(SDL_Renderer *renderer)
{
  sdl_set_color_hex(renderer, GRID_COLOR);
    
  for(int x = 1; x < BOARD_WIDTH; ++x){
    float line_x = (float)x * CELL_WIDTH;
    scc(SDL_RenderLine(renderer, line_x, 0, line_x, SCREEN_HEIGHT));
  }

  for(int y = 1; y < BOARD_HEIGHT; ++y){
    float line_y = (float)y * CELL_HEIGHT;
    scc(SDL_RenderLine(renderer, 0, line_y, SCREEN_WIDTH, line_y));
  }
}

void render_agent(SDL_Renderer *renderer, Agent agent)
{
  float center_x = (float)agent.pos_x * CELL_WIDTH + CELL_WIDTH * 0.5f;
  float center_y = (float)agent.pos_y * CELL_HEIGHT + CELL_HEIGHT * 0.5f;
  float size = (CELL_WIDTH - 2.0f * AGENT_PADDING) / 3.0f;
    
  uint32_t hex = AGENT_COLOR;
  SDL_FColor agent_color = {
    (float) ((hex >> 24) & 0xFF) / 255.0f,
    (float) ((hex >> 16) & 0xFF) / 255.0f,
    (float) ((hex >> 8) & 0xFF) / 255.0f,
    (float) (hex & 0xFF) / 255.0f
  };

  SDL_Vertex vertices[3];
    
  switch(agent.dir){
  case DIR_RIGHT: {
    vertices[0].position = (SDL_FPoint){center_x + size, center_y};             
    vertices[1].position = (SDL_FPoint){center_x - size, center_y - size};     
    vertices[2].position = (SDL_FPoint){center_x - size, center_y + size};     
    break;
  }

  case DIR_UP: {
    vertices[0].position = (SDL_FPoint){center_x, center_y - size};             
    vertices[1].position = (SDL_FPoint){center_x - size, center_y + size};     
    vertices[2].position = (SDL_FPoint){center_x + size, center_y + size};     
    break;
  }

  case DIR_LEFT: {
    vertices[0].position = (SDL_FPoint){center_x - size, center_y};             
    vertices[1].position = (SDL_FPoint){center_x + size, center_y + size};     
    vertices[2].position = (SDL_FPoint){center_x + size, center_y - size};     
    break;
  }

  case DIR_DOWN: {
    vertices[0].position = (SDL_FPoint){center_x, center_y + size};             
    vertices[1].position = (SDL_FPoint){center_x + size, center_y - size};     
    vertices[2].position = (SDL_FPoint){center_x - size, center_y - size};     
    break;
  }
  }
  for(int i = 0; i < 3; i++) {
    vertices[i].color = agent_color;
    vertices[i].tex_coord = (SDL_FPoint){0.0f, 0.0f};
  }

  scr(SDL_RenderGeometry(renderer, NULL, vertices, 3, NULL, 0));
}

void render_food(SDL_Renderer *renderer, Food food){
  // TODO: implement food render
}

void render_wall(SDL_Renderer *renderer, Wall wall){

  sdl_set_color_hex(renderer, WALL_COLOR);
  SDL_FRect rect = {
    wall.pos_x * CELL_WIDTH,
    wall.pos_y * CELL_HEIGHT,
    CELL_WIDTH,
    CELL_HEIGHT,
  };
  SDL_RenderFillRect(renderer, &rect);


}

void render_game(SDL_Renderer *renderer, const Game *game)
{
  for(size_t i = 0; i < AGENTS_COUNT; ++i){
    render_agent(renderer, game->agents[i]);
  }

  for(size_t i = 0; i < FOOD_COUNT; ++i){
    render_food(renderer, game->foods[i]);
  }

  for(size_t i = 0; i < WALLS_COUNT; ++i){
    render_wall(renderer, game->walls[i]);
  }
  render_board_grid(renderer);
  
}

void step_game(Game *game){
  // TODO: step up game

}

Game game = {0};

int main(int argc, char* argv[]) {

  init_game(&game);
  scr(SDL_Init(SDL_INIT_VIDEO));
    
  SDL_Window *window = scp(SDL_CreateWindow(
					    "evolol", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE
					    ));
    
  SDL_Renderer *renderer = scp(SDL_CreateRenderer(window, NULL));
    
  scr(SDL_SetRenderLogicalPresentation(renderer, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX));

  int quit = 0;
  SDL_Event event;    
  while (!quit) {
    while (SDL_PollEvent(&event)) {
      
      switch(event.type) {
      case SDL_EVENT_QUIT: {
	quit = 1;
      } break;
	
      case SDL_EVENT_KEY_DOWN: {
	
	switch(event.key.key){
	case SDLK_SPACE : {
	  step_game(&game);
	}break; 
	}
	
      } break;

      }
	
    }
      
    sdl_set_color_hex(renderer, BACKGROUND_COLOR);
    scc(SDL_RenderClear(renderer));
    
    render_game(renderer, &game);

    SDL_RenderPresent(renderer);
  }
    
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  printf("Exited cleanly.\n");
  return 0;
}
