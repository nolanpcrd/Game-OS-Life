#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>
#include <stdbool.h>

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768
#define UART0_BASE    0x3F201000

// Fonctions d'initialisation et de dessin
int screen_init(void);
void screen_clear(uint32_t color);
void draw_pixel(int x, int y, uint32_t color);
void draw_rect(int x, int y, int w, int h, uint32_t color);
void draw_circle(int x0, int y0, int radius, uint32_t color);
void draw_line(int x0, int y0, int x1, int y1, uint32_t color, int thickness);
void draw_rounded_rect(int x, int y, int w, int h, int r, uint32_t color);
void draw_empty_rect(int x, int y, int w, int h, uint32_t color);
void draw_letter(int x, int y, char letter, uint32_t color, int font_size, bool bold);
void draw_string(int x, int y, const char* str, uint32_t color, int font_size, int letter_spacing, bool bold);
int abs(int n);
char read_uart();

// Export du pointeur framebuffer et informations sur l'écran pour d'autres modules
extern volatile uint32_t* get_framebuffer_ptr(void);
extern int get_sceen_pitch(void);
extern int get_screen_depth(void);

#endif // SCREEN_H
