/*
  Dipendenze:

  - SDL2

    sudo pacman -S sdl2 (archlinux)
    sudo apt-get install libsdl2-dev (ubuntu)

  - SDL2_font

    sudo pacman -S sdl2_ttf (archlinux)
    sudo apt-get install libsdl2-ttf-dev (ubuntu)

 */

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <SDL2/SDL.h>
#include <SDL_ttf.h>

#define SCREEN_WIDTH  900
#define SCREEN_HEIGHT 900

#define BOARD_WIDTH 30
#define BOARD_HEIGHT 30

#define CELL_WIDTH ((SCREEN_WIDTH / BOARD_WIDTH))
#define CELL_HEIGHT ((SCREEN_HEIGHT / BOARD_HEIGHT))

#define MAX_SNAKE_MOVEMENT 0.15
#define MIN_SNAKE_MOVEMENT 0.06
#define STEP_SNAKE_MOVEMENT 0.03

#define MAX_SNAKE_LENGTH ((BOARD_WIDTH) * (BOARD_HEIGHT))

#define DELAY_FOOD_SPAWN 3
#define FOODS_COUNT 1

#define OBSTACLES_COUNT 20

#define FONT_PATH "./fonts/LiberationMono-Regular.ttf"

// NOTE: to use these off has to be an int initialized to 0
#define STAR_OBSTACLE(game, off, x, y)					\
  (game)->obs[off++] = (Obstacle){(Pos) {x    , y}    , 1};		\
  (game)->obs[off++] = (Obstacle){(Pos) {x + 1, y}    , 1};		\
  (game)->obs[off++] = (Obstacle){(Pos) {x - 1, y}    , 1};		\
  (game)->obs[off++] = (Obstacle){(Pos) {x    , y + 1}, 1};		\
  (game)->obs[off++] = (Obstacle){(Pos) {x    , y - 1}, 1};

#define HORIZONTAL_WALL_OBSACLE(game, off, x, y)		\
  (game)->obs[off++] = (Obstacle){(Pos) {x    , y}    , 1};	\
  (game)->obs[off++] = (Obstacle){(Pos) {x + 1, y}    , 1};	\
  (game)->obs[off++] = (Obstacle){(Pos) {x + 1, y}    , 1};	\
  (game)->obs[off++] = (Obstacle){(Pos) {x - 1, y}    , 1};	\
  (game)->obs[off++] = (Obstacle){(Pos) {x -2 , y}    , 1};


// RGBA, Red Green Blue Alpha
#define BACKGROUND_COLOR 0x000000FF
#define GRID_COLOR       0xFFFFFFFF
#define SNAKE_COLOR      0xEE72F100
#define FOOD_COLOR       0x77B28C00
#define OBSTACLE_COLOR   0x964B0000
#define SCORE_COLOR      0xFFFFFF00

// Tsoding
// https://www.twitch.tv/tsoding
// https://github.com/tsoding
#define HEX_COLOR(hex)							\
  ((hex) >> (3 * 8)) & 0xFF,						\
  ((hex) >> (2 * 8)) & 0xFF,						\
  ((hex) >> (1 * 8)) & 0xFF,						\
  ((hex) >> (0 * 8)) & 0xFF

// -------------------
// STRUTTURE DATI

typedef enum {
  DIR_RIGHT = 0,
  DIR_UP,
  DIR_LEFT,
  DIR_DOWN,
} Dir;

typedef struct {
  int x;
  int y;
} Pos;

typedef struct {
  Pos pos;
  int score;
} Food;

typedef struct {
  Pos pos;
  int init;
} Obstacle;

typedef struct {
  Pos body[MAX_SNAKE_LENGTH];
  int length;
  Dir dir;
} Snake;

typedef struct {
  Snake snake;
  Food food[FOODS_COUNT];
  Obstacle obs[OBSTACLES_COUNT];
  double game_speed;
  int quit;
  int global_score;
} Game;


// -------------------
// DICHIARAZIONI FUNZIONI

void scc(int code);
void *scp(void *ptr);

int random_int_range(int low, int high);
Pos random_board_pos(void);
int pos_is_not_empty(Game *game, Pos p);
Pos random_empty_board_pos(Game *game);

void init_game(Game *game);

Pos *get_snake_head(Snake *snake);
int allow_snake_movement(int manual, Game *game);
Pos peak_next_pos(Snake *snake, Dir new_dir);
void move_snake(Game *game, Dir new_dir, int manual);
void eat_food(Game *game, Food *f);

void init_food(Game *game);
int allow_refresh_food(void);
Food *check_for_food(Game *game);
void update_food(Game *game);

int check_for_obstacles(Game *game);

void update_game_speed(Game *game);

void update_game_state(Game *game);

void render_game(SDL_Renderer *renderer, Game *game, TTF_Font *font);
void render_snake(SDL_Renderer *renderer, Game *game);
void render_food(SDL_Renderer *renderer, Game *game);
void render_obstacles(SDL_Renderer *renderer, Game *game);
void render_game_score(SDL_Renderer *renderer, Game *game, TTF_Font *font);
void render_board(SDL_Renderer *renderer);
void render_square(SDL_Renderer *renderer, Pos pos, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

// -------------------
// VARIABILI GLOBALI

Game GAME = {0};

// -------------------
// UTILS

// Thanks Tsoding, once again :D
void scc(int code) {
  if (code < 0) {
    printf("SDL error: %s\n", SDL_GetError());
    exit(1);
  }

  return;
}

void *scp(void *ptr) {
  if (ptr == NULL) {
    printf("SDL error: %s\n", SDL_GetError());
    exit(1);
  }

  return ptr;
}

int random_int_range(int low, int high) {
  return (rand() % (high - low)) + low;
}

Pos random_board_pos(void) {
  Pos p = {0};
  p.x = random_int_range(0, BOARD_WIDTH);
  p.y = random_int_range(0, BOARD_HEIGHT);

  return p;
}

Dir random_dir(void) {
  return (Dir) random_int_range(0, 4);
  
}

int pos_is_not_empty(Game *game, Pos p) {
  // Food is here?
  for (int i = 0; i < FOODS_COUNT; i++) {
    if (p.x == game->food[i].pos.x && p.y == game->food[i].pos.y)
      return 1;
  }

  for (int i = 0; i < game->snake.length; i++) {
    if (p.x == game->snake.body[i].x &&
	p.y == game->snake.body[i].y)
      return 1;
  }  

  for (int i = 0; i < OBSTACLES_COUNT; i++) {
    if (p.x == game->obs[i].pos.x && p.y == game->obs[i].pos.y)
      return 1;
  }  

  return 0;
}

Pos random_empty_board_pos(Game *game) {
  Pos p = {0};
  do {
    p = random_board_pos();
  } while (pos_is_not_empty(game, p));

  return p;
}

// -------------------
// GAME LOGIC FUNCTIONS

void init_game(Game *game) {
  // -- init snake
  game->snake.body[0] = random_board_pos();
  game->snake.length = 1;
  game->snake.dir = random_dir();
  
  init_food(game);

  // init obstacles
  int off = 0;  
  STAR_OBSTACLE(game, off, 10, 10);
  HORIZONTAL_WALL_OBSACLE(game, off, 20, 20)

  game->quit = 0;
  game->global_score = 0;
  game->game_speed = MAX_SNAKE_MOVEMENT;
}

Pos *get_snake_head(Snake *snake) {
  return &snake->body[snake->length - 1];
}

int allow_snake_movement(int manual, Game *game) {
  static struct timeval old_t = {0};
  static struct timeval new_t = {0};
  static int init = -1;
  double time_elapsed = -1;

  if (init == -1) {
    // -- first call to function
    init = 1;
    gettimeofday(&old_t, NULL);

    return manual ? 1 : 0;
  }
    
  gettimeofday(&new_t, NULL);
  time_elapsed = (double) (new_t.tv_usec - old_t.tv_usec) / 1000000 +
    (double) (new_t.tv_sec - old_t.tv_sec);
  
  if (!manual && time_elapsed < game->game_speed) {
    // not enough time has passed for automatic movement
    return 0;
  } else {
    old_t = new_t;
    return 1;
  }
}

Pos peak_next_pos(Snake *snake, Dir new_dir) {
  Pos new_pos;
  Pos *head_pos = get_snake_head(snake);

  switch(new_dir) {
  case DIR_RIGHT:
    new_pos.x = (head_pos->x + 1) % BOARD_WIDTH;
    new_pos.y = head_pos->y;
    break;
    
  case DIR_LEFT:
    new_pos.x = head_pos->x == 0 ? BOARD_WIDTH - 1 : head_pos->x - 1;
    new_pos.y = head_pos->y;
    break;

  case DIR_UP:
    new_pos.x = head_pos->x;    
    new_pos.y = head_pos->y == 0 ? BOARD_HEIGHT - 1 : head_pos->y - 1;
    break;

  case DIR_DOWN:
    new_pos.x = head_pos->x;    
    new_pos.y = (head_pos->y + 1) % BOARD_HEIGHT;
    break;
  }

  return new_pos;
  
}

void move_snake(Game *game, Dir new_dir, int manual) {
  if(!allow_snake_movement(manual, game)) {
    return;
  }

  Snake *snake = &game->snake;
  Pos new_pos = peak_next_pos(snake, new_dir);

  // cant move back to snake's own tail
  if (snake->length >= 2 &&
      new_pos.x == snake->body[snake->length - 2].x &&
      new_pos.y == snake->body[snake->length - 2].y)
    return;

  // perform movement
  Pos *head_pos = get_snake_head(snake);
  Pos old_pos = *head_pos;
  Pos tmp_pos = old_pos;

  *head_pos = new_pos;
  snake->dir = new_dir;

  for (int i = snake->length -2 ; i >= 0; i--) {
    tmp_pos = snake->body[i];
    snake->body[i] = old_pos;
    old_pos = tmp_pos;
  }
}

void eat_food(Game *game, Food *f) {
  Snake *snake = &game->snake;

  // eat food
  game->global_score += f->score;
  f->score = 0;

  // grow snake's body
  Pos new_pos = peak_next_pos(snake, snake->dir);
  snake->length += 1;
  snake->body[snake->length - 1] = new_pos;

  return;
}

void init_food(Game *game) {
  for (int i = 0; i < FOODS_COUNT; i++) {
    game->food[i].score = 1;
    game->food[i].pos = random_empty_board_pos(game);
  }

  return;
}

int allow_refresh_food(void) {
  // TODO: check if (OS) windows has it
  static struct timeval old_t = {0};
  static struct timeval new_t = {0};
  static int init = -1;
  Uint32 time_elapsed = -1;

  if (init == -1) {
    init = 1;
    gettimeofday(&old_t, NULL);
    return 1;
  }

  gettimeofday(&new_t, NULL);
  time_elapsed = (double) (new_t.tv_usec - old_t.tv_usec) / 1000000 + (double) (new_t.tv_sec - old_t.tv_sec);

  if (time_elapsed < DELAY_FOOD_SPAWN) {
    return 0;
  } else {
    old_t = new_t;
    return 1;
  }
}

Food *check_for_food(Game *game) {
  Snake *snake = &game->snake;
  Pos head_pos = *get_snake_head(snake);

  for (int i = 0; i < FOODS_COUNT; i++) {
    Food *f = &game->food[i];

    if(f->pos.x == head_pos.x && f->pos.y == head_pos.y && f->score > 0) {
      return f;
    }
  }

  return NULL;
}

void update_food(Game *game) {
  if (allow_refresh_food()) {
    init_food(game);
  }
  return;
}

void update_game_speed(Game *game) {
  double step_update = game->global_score * STEP_SNAKE_MOVEMENT;
  
  if (MAX_SNAKE_MOVEMENT - step_update < MIN_SNAKE_MOVEMENT) {
    game->game_speed = MIN_SNAKE_MOVEMENT;
  } else {
    game->game_speed = MAX_SNAKE_MOVEMENT - step_update;
  }

  return;
}

int check_for_obstacles(Game *game) {
  Snake *s = &game->snake;
  Pos head_pos = *get_snake_head(s);

  // did we go into our own tail?
  for (int i = 0; i < s->length - 2; i++) {
    if (s->body[i].x == head_pos.x &&
	s->body[i].y == head_pos.y)
      return 1;
  }

  // or against an initialized obstacle?
  for (int i = 0; i < OBSTACLES_COUNT; i++) {
    Obstacle ob = game->obs[i];
    if (ob.init &&
	ob.pos.x == head_pos.x &&
	ob.pos.y == head_pos.y)
      return 1;
  }

  return 0;
}

void update_game_state(Game *game) {
  move_snake(&GAME, GAME.snake.dir, 0);
  GAME.quit |= check_for_obstacles(&GAME);    
  Food *f = check_for_food(&GAME);
  if (f) {
    eat_food(&GAME, f);
    update_game_speed(&GAME);
  }
  update_food(&GAME);  
}

// -------------------
// RENDER FUNCTIONS

void render_game(SDL_Renderer *renderer, Game *game, TTF_Font *font) {
  scc(SDL_SetRenderDrawColor(renderer, HEX_COLOR(BACKGROUND_COLOR)));
  SDL_RenderClear(renderer);
  
  // render_board(renderer);
  render_snake(renderer, game);
  render_food(renderer, game);
  render_obstacles(renderer, game);
  render_game_score(renderer, game, font);

  SDL_RenderPresent(renderer);
}

void render_board(SDL_Renderer *renderer) {
  scc(SDL_SetRenderDrawColor(renderer, HEX_COLOR(GRID_COLOR)));
  
  for(int x = 0; x < BOARD_WIDTH; x++) {
    SDL_RenderDrawLine(renderer,
		       x * CELL_WIDTH, 0,             // starting (x_1, y_1)
		       x * CELL_WIDTH, SCREEN_HEIGHT  // ending   (x_2, y_2)
		       );
  }

  for(int y = 0; y < BOARD_HEIGHT; y++) {
    SDL_RenderDrawLine(renderer,
		       0, y * CELL_HEIGHT,	      // starting (x_1, y_1)
		       SCREEN_WIDTH, y * CELL_HEIGHT  // ending   (x_2, y_2)
		       );
  }
}

void render_snake(SDL_Renderer *renderer, Game *game) {
  for (int i = game->snake.length - 1; i >= 0; i--) {
    render_square(renderer, game->snake.body[i], HEX_COLOR(SNAKE_COLOR));
  }
}

void render_food(SDL_Renderer *renderer, Game *game) {
  for (int i = 0; i < FOODS_COUNT; i++) {
    if (game->food[i].score == 0) {
      continue; // Skip foods that have already been eaten
    }
    render_square(renderer, game->food[i].pos, HEX_COLOR(FOOD_COLOR));
  }
}

void render_obstacles(SDL_Renderer *renderer, Game *game) {
  scc(SDL_SetRenderDrawColor(renderer, HEX_COLOR(OBSTACLE_COLOR)));

  for (int i = 0; i < OBSTACLES_COUNT; i++) {
    Obstacle ob = game->obs[i];

    if (!ob.init)
      // do not render Uninitialized obstacles 
      continue;

    render_square(renderer, ob.pos, HEX_COLOR(OBSTACLE_COLOR));
  }    
}

void render_game_score(SDL_Renderer *renderer, Game *game, TTF_Font *font) {
  static SDL_Surface *surface;
  static SDL_Texture *texture;
  static int init = -1;
  static int prev_score = -1;

  if(prev_score == game->global_score) {
    // -- nothing to update
    return;
  }

  if (init != -1) {
    // -- clean previous allocations
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
  }

  // -- create new string
  char str[32];
  sprintf(str, "Score: %d", game->global_score);

  // -- allocate it and display it
  surface = TTF_RenderText_Solid(font, str, (SDL_Color){HEX_COLOR(SCORE_COLOR)});
  
  if(!surface) {
    printf("Error: %s\n", TTF_GetError());
    exit(1);
  }
  
  texture = scp(SDL_CreateTextureFromSurface(renderer, surface));

  int textW = 0;
  int textH = 0;
  SDL_QueryTexture(texture, NULL, NULL, &textW, &textH);
  SDL_Rect text_rect = {0, 0, textW, textH};

  SDL_RenderCopy(renderer, texture, NULL, &text_rect);
  
}

/*
  Draws a square in the grid at position (pos.x, pos.y)
  with the specified color.
*/
void render_square(SDL_Renderer *renderer, Pos pos, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
  scc(SDL_SetRenderDrawColor(renderer, r, g, b, a));
  
  SDL_Rect rect = {
    (int) floorf(pos.x * CELL_WIDTH),
    (int) floorf(pos.y * CELL_HEIGHT),
    (int) floorf(CELL_WIDTH),
    (int) floorf(CELL_HEIGHT),
  };

  scc(SDL_RenderFillRect(renderer, &rect));
}

// -------------------


int main(void) {
  srand(time(0));

  // init classic SDL
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *const window = scp(SDL_CreateWindow("Description", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE));  
  SDL_Renderer *const renderer = scp(SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED));

  // init font
  TTF_Init();
  TTF_Font *font = TTF_OpenFont(FONT_PATH, 30);
  if(!font) {
    printf("Error loading font `%s`: %s\n", FONT_PATH, TTF_GetError());
    exit(1);
  }  

  init_game(&GAME);
  
  while(!GAME.quit) {
    SDL_Event event;

    // event handling
    while(SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
	GAME.quit = 1;
      }

      if (event.type == SDL_KEYDOWN) {
	switch (event.key.keysym.sym) {
	case SDLK_UP: {
	  move_snake(&GAME, DIR_UP, 1);
	  break;
	}

	case SDLK_DOWN: {
	  move_snake(&GAME, DIR_DOWN, 1);
	  break;
	}

	case SDLK_LEFT: {
	  move_snake(&GAME, DIR_LEFT, 1);
	  break;
	}

	case SDLK_RIGHT: {
	  move_snake(&GAME, DIR_RIGHT, 1);
	  break;
	}	  
	}
      }
    }

    // main logic loop
    update_game_state(&GAME);
    // rendering stuff
    render_game(renderer, &GAME, font);
  }

  TTF_CloseFont(font);
  TTF_Quit();
  SDL_Quit();
  
  return 0;
}
