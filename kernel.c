#include "libs/screen.h"
#include "libs/game.h"
#include <stdbool.h>
#include <stddef.h>

extern uint8_t _bss_start[];
extern uint8_t _bss_end[];

#define COLOR_BACKGROUND    0x1A1A2E
#define COLOR_PRIMARY       0x16213E
#define COLOR_SECONDARY     0x0F3460
#define COLOR_ACCENT        0x533483
#define COLOR_HIGHLIGHT     0xE94560
#define COLOR_SUCCESS       0x0FFF50
#define COLOR_WARNING       0xFFA500
#define COLOR_TEXT_PRIMARY  0xFFFFFF
#define COLOR_TEXT_SECONDARY 0xBBBBBB
#define COLOR_CELL_ALIVE    0x00FF88
#define COLOR_CELL_DEAD     0x2A2A3E
#define COLOR_GRID          0x444466
#define COLOR_CURSOR        0xFF6B6B

int uart_available(void) {
    return (*(volatile uint32_t*)(UART0_BASE + 0x18) & 0x10) == 0;
}

char uart_read(void) {
    return *(volatile char*)(UART0_BASE + 0x00);
}

void panic(void) {
    screen_clear(COLOR_HIGHLIGHT);

    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        uint32_t color = COLOR_HIGHLIGHT - (y * 0x100);
        draw_line(0, y, SCREEN_WIDTH, y, color, 1);
    }

    int box_w = 400, box_h = 200;
    int box_x = SCREEN_WIDTH/2 - box_w/2;
    int box_y = SCREEN_HEIGHT/2 - box_h/2;

    draw_rounded_rect(box_x, box_y, box_w, box_h, 20, COLOR_PRIMARY);
    draw_empty_rect(box_x + 5, box_y + 5, box_w - 10, box_h - 10, COLOR_TEXT_PRIMARY);

    draw_string(box_x + 50, box_y + 50, "SYSTEM PANIC", COLOR_TEXT_PRIMARY, 2, 25, true);
    draw_string(box_x + 60, box_y + 100, "CRITICAL ERROR OCCURED", COLOR_TEXT_SECONDARY, 1, 18, false);
    draw_string(box_x + 80, box_y + 140, "SYSTEM HALTED", COLOR_WARNING, 1, 18, false);

    while (1);
}

void draw_header(void) {
    draw_rounded_rect(0, 0, SCREEN_WIDTH, 80, 0, COLOR_PRIMARY);
    draw_line(0, 75, SCREEN_WIDTH, 75, COLOR_ACCENT, 5);

    draw_string(32, 22, "GAME OS LIFE", COLOR_TEXT_SECONDARY, 2, 30, true);
    draw_string(30, 20, "GAME OS LIFE", COLOR_TEXT_PRIMARY, 2, 30, true);

    int badge_x = SCREEN_WIDTH - 180;
    draw_rounded_rect(badge_x, 20, 150, 35, 17, COLOR_ACCENT);
    draw_string(badge_x + 15, 30, "VER TWO", COLOR_TEXT_PRIMARY, 1, 15, true);
}

void draw_sidebar(bool paused) {
    int sidebar_w = 200;
    int sidebar_x = SCREEN_WIDTH - sidebar_w;

    draw_rounded_rect(sidebar_x, 90, sidebar_w - 10, SCREEN_HEIGHT - 100, 15, COLOR_SECONDARY);

    draw_rounded_rect(sidebar_x + 10, 110, sidebar_w - 30, 100, 10, COLOR_PRIMARY);
    draw_string(sidebar_x + 20, 120, "STATUS", COLOR_TEXT_PRIMARY, 1, 15, true);

    if (paused) {
        draw_rounded_rect(sidebar_x + 20, 160, 100, 35, 12, COLOR_WARNING);
        draw_string(sidebar_x + 25, 148, "PAUSED", COLOR_TEXT_PRIMARY, 1, 12, false);
    } else {
        draw_rounded_rect(sidebar_x + 20, 160, 100, 35, 12, COLOR_SUCCESS);
        draw_string(sidebar_x + 25, 168, "RUNNING", COLOR_TEXT_PRIMARY, 1, 12, false);
    }

    draw_rounded_rect(sidebar_x + 10, 220, sidebar_w - 30, 160, 10, COLOR_PRIMARY);
    draw_string(sidebar_x + 20, 230, "CONTROLS", COLOR_TEXT_PRIMARY, 1, 15, true);

    int y_offset = 260;
    draw_string(sidebar_x + 20, y_offset, "ZQSD", COLOR_SUCCESS, 1, 15, true);
    draw_string(sidebar_x + 90, y_offset, "MOVE", COLOR_TEXT_SECONDARY, 1, 15, false);
    y_offset += 30;
    draw_string(sidebar_x + 20, y_offset, "E", COLOR_SUCCESS, 1, 15, true);
    draw_string(sidebar_x + 90, y_offset, "TOGGLE", COLOR_TEXT_SECONDARY, 1, 15, false);
    y_offset += 30;
    draw_string(sidebar_x + 20, y_offset, "P", COLOR_SUCCESS, 1, 15, true);
    draw_string(sidebar_x + 90, y_offset, "PAUSE", COLOR_TEXT_SECONDARY, 1, 15, false);
    y_offset += 30;
    draw_string(sidebar_x + 20, y_offset, "X", COLOR_HIGHLIGHT, 1, 15, true);
    draw_string(sidebar_x + 90, y_offset, "EXIT", COLOR_TEXT_SECONDARY, 1, 15, false);

    draw_rounded_rect(sidebar_x + 10, 400, sidebar_w - 30, 80, 10, COLOR_PRIMARY);
    draw_string(sidebar_x + 20, 410, "PERFORMANCE", COLOR_TEXT_PRIMARY, 1, 15, true);
    draw_string(sidebar_x + 20, 445, "MEM OK", COLOR_SUCCESS, 1, 15, false);
}

void draw_game_area(bool paused, int x_selected, int y_selected, bool update) {
    int map_size = get_map_size();
    uint8_t* game_map = get_map();

    int game_area_x = 20;
    int game_area_y = 90;
    int game_area_w = SCREEN_WIDTH - 230;
    int game_area_h = SCREEN_HEIGHT - 150;

    draw_rounded_rect(game_area_x, game_area_y, game_area_w, game_area_h, 15, COLOR_SECONDARY);

    int grid_size = 400;
    int grid_x = game_area_x + (game_area_w - grid_size) / 2;
    int grid_y = game_area_y + (game_area_h - grid_size) / 2;

    draw_rounded_rect(grid_x - 5, grid_y - 5, grid_size + 10, grid_size + 10, 10, COLOR_PRIMARY);

    int cell_size = grid_size / 10;

    for (int i = 0; i <= 10; i++) {
        int x = grid_x + i * cell_size;
        int y = grid_y + i * cell_size;
        if (i == 0 || i == 10) {
            draw_line(grid_x, y, grid_x + grid_size, y, COLOR_GRID, 2);
            draw_line(x, grid_y, x, grid_y + grid_size, COLOR_GRID, 2);
        } else {
            draw_line(grid_x, y, grid_x + grid_size, y, COLOR_GRID, 1);
            draw_line(x, grid_y, x, grid_y + grid_size, COLOR_GRID, 1);
        }
    }

    for (int i = 0; i < map_size; i++) {
        int cell_x = grid_x + (i % 10) * cell_size;
        int cell_y = grid_y + (i / 10) * cell_size;
        uint8_t value = game_map[i];

        if (value == 1) {
            draw_rounded_rect(cell_x + 3, cell_y + 3, cell_size - 6, cell_size - 6, 6, COLOR_CELL_ALIVE);
        }

        if (paused && (i % 10) == x_selected && (i / 10) == y_selected) {
            draw_empty_rect(cell_x + 1, cell_y + 1, cell_size - 2, cell_size - 2, COLOR_CURSOR);
            draw_empty_rect(cell_x + 2, cell_y + 2, cell_size - 4, cell_size - 4, COLOR_CURSOR);
        }
    }

    if (update) {
        update_game();
    }
}

void draw_footer(void) {
    int footer_y = SCREEN_HEIGHT - 40;
    draw_line(0, footer_y, SCREEN_WIDTH, footer_y, COLOR_ACCENT, 2);

    draw_string(20, footer_y + 10, "CONWAY S GAME OF LIFE OS EDITION", COLOR_TEXT_SECONDARY, 1, 18, false);

    draw_string(SCREEN_WIDTH - 190, footer_y + 5, "X", COLOR_HIGHLIGHT, 1, 18, true);
    draw_string(SCREEN_WIDTH - 150, footer_y + 5, "TO EXIT", COLOR_TEXT_SECONDARY, 1, 18, false);
}

void draw_exit_screen(void) {
    screen_clear(COLOR_BACKGROUND);

    int box_w = 600, box_h = 300;
    int box_x = SCREEN_WIDTH/2 - box_w/2;
    int box_y = SCREEN_HEIGHT/2 - box_h/2;

    draw_rounded_rect(box_x, box_y, box_w, box_h, 25, COLOR_PRIMARY);
    draw_empty_rect(box_x + 5, box_y + 5, box_w - 10, box_h - 10, COLOR_ACCENT);

    draw_string(box_x + 150, box_y + 50, "THANK YOU", COLOR_TEXT_PRIMARY, 2, 25, true);
    draw_string(box_x + 100, box_y + 100, "FOR USING GAME OF LIFE OS", COLOR_TEXT_SECONDARY, 1, 15, false);
    draw_string(box_x + 200, box_y + 150, "GOODBYE", COLOR_ACCENT, 2, 20, true);

    draw_circle(box_x + 50, box_y + 50, 20, COLOR_SUCCESS);
    draw_circle(box_x + box_w - 50, box_y + 50, 20, COLOR_SUCCESS);
    draw_circle(box_x + 50, box_y + box_h - 50, 20, COLOR_SUCCESS);
    draw_circle(box_x + box_w - 50, box_y + box_h - 50, 20, COLOR_SUCCESS);
}

#define TIMER_BASE 0x3F003000
#define TIMER_VALUE (TIMER_BASE + 0x04)

void wait(unsigned int ms) {
    unsigned int start_time, end_time;
    unsigned int timer_freq = 1000000;
    start_time = *(volatile unsigned int*)TIMER_VALUE;
    end_time = start_time + (ms * timer_freq) / 1000;
    while (*(volatile unsigned int*)TIMER_VALUE < end_time);
}

void kernel_main(void) {
    for (uint8_t* bss = _bss_start; bss < _bss_end; bss++) {
        *bss = 0;
    }

    if (screen_init() != 0 || get_framebuffer_ptr() == NULL) {
        panic();
        return;
    }

    screen_clear(COLOR_BACKGROUND);

    bool paused = false;
    int x_selected = 0;
    int y_selected = 0;

    while (1) {
        screen_clear(COLOR_BACKGROUND);
        draw_header();
        draw_sidebar(paused);
        draw_game_area(paused, x_selected, y_selected, !paused);
        draw_footer();


        if (uart_available()) {
            char c = uart_read();
            switch (c) {
                case 'z':
                    if (y_selected > 0) y_selected--;
                    break;
                case 's':
                    if (y_selected < 9) y_selected++;
                    break;
                case 'q':
                    if (x_selected > 0) x_selected--;
                    break;
                case 'd':
                    if (x_selected < 9) x_selected++;
                    break;
                case 'e':
                    if (paused) {
                        toggle_cell(x_selected, y_selected);
                    }
                    break;
                case 'p':
                    paused = !paused;
                    break;
                case 'x':
                    draw_exit_screen();
                    wait(3000);
                    return;
                default:
                    break;
            }
        }

        if (paused) {
            wait(300);
        } else {
            wait(500);
        }
    }
}