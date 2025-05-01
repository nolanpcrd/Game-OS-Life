#include "libs/screen.h"
#include "libs/game.h"
#include <stdbool.h>
#include <stddef.h>

extern uint8_t _bss_start[];
extern uint8_t _bss_end[];

void panic(void) {
    screen_clear(0xFF0000);

    draw_rect(SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 100, 400, 200, 0xFF0000);
    draw_string(SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2-40, "KERNEL PANIC", 0xFFFFFF, 4, 50, true);

    while (1);
}

void init_draw(void) {
    screen_clear(0xFFFFFF);
    draw_string(10, 10, "GAME OS LIFE", 0x000000, 3, 40, true);
    draw_string(SCREEN_WIDTH - 400, 10, "FIRST VERSION", 0x000000, 2, 30, true);
    draw_empty_rect(SCREEN_WIDTH/2 - 250, SCREEN_HEIGHT/2 - 250 ,500, 500, 0x000000);
}

void draw_map() {
    init_draw();
    int map_size = get_map_size();
    uint8_t* game_map = get_map();

    int startX = SCREEN_WIDTH/2 - 250;
    int startY = SCREEN_HEIGHT/2 - 250;
    int cell_size = 500 / 10;

    for (int i = 0; i < map_size; i++) {
        int x = startX + (i % 10) * cell_size;
        int y = startY + (i / 10) * cell_size;
        uint8_t value = game_map[i];

        if (value == 1) {
            draw_rect(x, y, cell_size, cell_size, 0x000000);
        } else {
            draw_empty_rect(x, y, cell_size, cell_size, 0x000000);
        }
    }
    update_game();
}

void kernel_main(void) {
    for (uint8_t* bss = _bss_start; bss < _bss_end; bss++) {
        *bss = 0;
    }

    if (screen_init() != 0 || get_framebuffer_ptr() == NULL) {
        panic();
        return;
    }

    init_draw();

    int frame = 0;
    while (1) {
        if(frame++ % 100000000 == 0) {
            draw_map();
        }
    }
}
