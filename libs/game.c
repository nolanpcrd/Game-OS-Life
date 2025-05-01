#include <stdint.h>

#define MAP_SIZE 10

uint8_t game_map[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
        0, 0, 1, 0, 1, 0, 0, 0, 0, 0,
        0, 0, 0, 1, 1, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

const int map_size = MAP_SIZE * MAP_SIZE;

uint8_t* get_map() {
    return game_map;
}

int get_map_size() {
    return map_size;
}

int get_wrapped_index(int x, int y) {
    int wrapped_x = (x + MAP_SIZE) % MAP_SIZE;
    int wrapped_y = (y + MAP_SIZE) % MAP_SIZE;

    return wrapped_y * MAP_SIZE + wrapped_x;
}

void update_game() {
    static uint8_t new_map[MAP_SIZE * MAP_SIZE];

    for (int i = 0; i < MAP_SIZE * MAP_SIZE; i++) {
        new_map[i] = 0;
    }

    for (int y = 0; y < MAP_SIZE; y++) {
        for (int x = 0; x < MAP_SIZE; x++) {
            int i = y * MAP_SIZE + x;
            int s = 0;

            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;

                    int neighbor_index = get_wrapped_index(x + dx, y + dy);
                    if (game_map[neighbor_index] == 1) {
                        s++;
                    }
                }
            }

            if (game_map[i] == 1) {
                if (s == 2 || s == 3) new_map[i] = 1;
            } else {
                if (s == 3) new_map[i] = 1;
            }
        }
    }

    for (int i = 0; i < MAP_SIZE * MAP_SIZE; i++) {
        game_map[i] = new_map[i];
    }
}