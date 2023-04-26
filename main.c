#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <math.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define CELLS_DIM 4
#define IMAGE_FILEPATH "./img/koda3.jpg"

typedef struct {
    float x;
    float y;
} Vec2;

float vec2_distance(Vec2 v1, Vec2 v2) {
    return sqrt((v2.x - v1.x) * (v2.x - v1.x) + (v2.y - v1.y) * (v2.y - v1.y));
}

typedef enum {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT,
} Direction;

Direction dir_from_vec2(Vec2 v1, Vec2 v2) {
    Vec2 dir = (Vec2) {
        .x = v2.x - v1.x,
        .y = v2.y - v1.y,
    };

    int len = sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len != 0) {
        dir.x = dir.x / len;
        dir.y = dir.y / len;
    }

    if (fabs(dir.x) >= fabs(dir.y)) {
        return (dir.x > 0) ? DIR_RIGHT : DIR_LEFT;
    } else {
        return (dir.y > 0) ? DIR_DOWN : DIR_UP;
    }
}

typedef struct {
    size_t x;
    size_t y;
} Point;

#define POINT_EMPTY ((Point) { -1, -1 })

bool point_eq(Point p1, Point p2) {
    return p1.x == p2.x && p1.y == p2.y;
}

typedef struct {
    Point cells[CELLS_DIM][CELLS_DIM];
} Board;

void board_randomize(Board *board) {
    srand(time(NULL));

    for (size_t y = 0; y < CELLS_DIM; ++y) {
        for (size_t x = 0; x < CELLS_DIM; ++x) {
            board->cells[y][x] = (Point) { x, y };
        }
    }

    for (size_t y = CELLS_DIM - 1; y > 0; --y) {
        for (size_t x = CELLS_DIM - 1; x > 0; --x) {
            size_t i = rand() % (x + 1);
            size_t j = rand() % (y + 1);

            Point temp = board->cells[y][x];
            board->cells[y][x] = board->cells[j][i];
            board->cells[j][i] = temp;
        }
    }

    size_t x = rand() % CELLS_DIM;
    size_t y = rand() % CELLS_DIM;
    board->cells[x][y] = POINT_EMPTY;
}

void board_slide_piece(Board *board, size_t x, size_t y, Direction dir) {
    int i = x;
    int j = y;

    switch (dir) {
        case DIR_RIGHT: i++; break;
        case DIR_LEFT: i--; break;
        case DIR_UP: j--; break;
        case DIR_DOWN: j++; break;
    }

    if (i < 0 || i > CELLS_DIM || j < 0 || j > CELLS_DIM) {
        return;
    }

    if (!point_eq(board->cells[j][i], POINT_EMPTY)) {
        return;
    }

    Point temp = board->cells[y][x];
    board->cells[y][x] = board->cells[j][i];
    board->cells[j][i] = temp;
}

int main(void) {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
        exit(1);
    }

    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(img_flags) & img_flags)) {
        fprintf(stderr, "SDL_image Error: %s\n", IMG_GetError());
        exit(1);
    }

    SDL_Surface *puzzle_surface = IMG_Load(IMAGE_FILEPATH);

    if (puzzle_surface == NULL) {
        fprintf(stderr, "SDL_image Error: %s\n", IMG_GetError());
        exit(1);
    }

    int window_width = puzzle_surface->w;
    int window_height = puzzle_surface->h;

    if (window_height % CELLS_DIM != 0 || window_width % CELLS_DIM != 0) {
        printf("Warning: the size of the image should be divisible by CELLS_DIM (%d)\n", CELLS_DIM);
        printf("Otherwise the tiles won't be correct, sorry for that\n");
    }

    int field_width = window_width / CELLS_DIM;
    int field_height = window_height / CELLS_DIM;

    Board board = { 0 };
    board_randomize(&board);

    SDL_Window *window = SDL_CreateWindow(
        "nuzzle",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width,
        window_height,
        SDL_WINDOW_SHOWN
    );

    if (window == NULL) {
        fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (renderer == NULL) {
        fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_Texture *puzzle_texture = SDL_CreateTextureFromSurface(renderer, puzzle_surface);

    int mouse_x = 0;
    int mouse_y = 0;

    Vec2 mouse_click_pos = { 0 };
    Vec2 mouse_pos = { 0 };
    Uint32 mouse_last_state = SDL_GetMouseState(NULL, NULL);
    bool allow_slide = true;

    bool quit = false;
    SDL_Event event = { 0 };

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }

        Uint32 mouse_state = SDL_GetMouseState(&mouse_x, &mouse_y);
        mouse_pos.x = (float) mouse_x;
        mouse_pos.y = (float) mouse_y;

        if (mouse_state & SDL_BUTTON_LEFT) {
            if (!(mouse_last_state & SDL_BUTTON_LEFT)) {
                allow_slide = true;
                mouse_click_pos = mouse_pos;
            }

            if (allow_slide && vec2_distance(mouse_click_pos, mouse_pos) > 60) {
                Direction dir = dir_from_vec2(mouse_click_pos, mouse_pos);
                size_t x = mouse_click_pos.x / field_width;
                size_t y = mouse_click_pos.y / field_height;

                board_slide_piece(&board, x, y, dir);
                allow_slide = false;
            }
        }

        mouse_last_state = mouse_state;

        SDL_RenderClear(renderer);

        for (size_t y = 0; y < CELLS_DIM; ++y) {
            for (size_t x = 0; x < CELLS_DIM; ++x) {
                Point p = board.cells[y][x];

                if (point_eq(p, POINT_EMPTY)) {
                    continue;
                }

                SDL_Rect src = { .x = p.x * field_width, .y = p.y * field_height, .w = field_width, .h = field_height };
                SDL_Rect dst = { .x = x * field_width, .y = y * field_height, .w = field_width, .h = field_height };

                SDL_RenderCopy(renderer, puzzle_texture, &src, &dst);
            }
        }

        for (size_t i = 1; i < CELLS_DIM; ++i) {
            int posx = i * field_width;
            int posy = i * field_height;
            SDL_RenderDrawLine(renderer, 0, posx, window_width, posx);
            SDL_RenderDrawLine(renderer, posy, 0, posy, window_height);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_FreeSurface(puzzle_surface);
    SDL_DestroyTexture(puzzle_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
