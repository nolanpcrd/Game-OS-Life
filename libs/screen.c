#include "screen.h"
#include <stddef.h>

static volatile uint32_t* framebuffer_ptr = NULL;
static int screen_width = 0;
static int screen_height = 0;
static int screen_pitch = 0;
static int screen_depth = 0;

volatile uint32_t* get_framebuffer_ptr(void) {
    return framebuffer_ptr;
}

#define MAILBOX_BASE    0x3F00B880
#define MAILBOX_READ    (MAILBOX_BASE + 0x00)
#define MAILBOX_STATUS  (MAILBOX_BASE + 0x18)
#define MAILBOX_WRITE   (MAILBOX_BASE + 0x20)
#define MAILBOX_FULL    0x80000000
#define MAILBOX_EMPTY   0x40000000

#define MAILBOX_CHANNEL_PROPERTY 8

static int mailbox_call(void* buffer) {
    uint32_t addr = (uintptr_t)buffer;

    if (addr & 0xF) {
        return -1;
    }

    addr &= ~0xF;
    addr |= MAILBOX_CHANNEL_PROPERTY;

    while (((volatile uint32_t*)MAILBOX_STATUS)[0] & MAILBOX_FULL) {
    }

    ((volatile uint32_t*)MAILBOX_WRITE)[0] = addr;

    uint32_t read_val;
    while (1) {
        while (((volatile uint32_t*)MAILBOX_STATUS)[0] & MAILBOX_EMPTY) {}

        read_val = ((volatile uint32_t*)MAILBOX_READ)[0];

        if ((read_val & 0xF) == MAILBOX_CHANNEL_PROPERTY && (read_val & ~0xF) == (uintptr_t)buffer) {
            break;
        }
    }

    volatile uint32_t* resp_buffer = (volatile uint32_t*)buffer;
    return (resp_buffer[1] == 0x80000000) ? 0 : -1;
}


int screen_init(void) {
    volatile uint32_t mailbox_buffer[35] __attribute__((aligned(16)));

    int index = 1;
    mailbox_buffer[index++] = 0;

    mailbox_buffer[index++] = 0x00048003;
    mailbox_buffer[index++] = 8;
    mailbox_buffer[index++] = 0;
    mailbox_buffer[index++] = SCREEN_WIDTH;
    mailbox_buffer[index++] = SCREEN_HEIGHT;

    mailbox_buffer[index++] = 0x00048004;
    mailbox_buffer[index++] = 8;
    mailbox_buffer[index++] = 0;
    mailbox_buffer[index++] = SCREEN_WIDTH;
    mailbox_buffer[index++] = SCREEN_HEIGHT;

    mailbox_buffer[index++] = 0x00048005;
    mailbox_buffer[index++] = 4;
    mailbox_buffer[index++] = 0;
    mailbox_buffer[index++] = 32;
    screen_depth = 32;

    mailbox_buffer[index++] = 0x00048009;
    mailbox_buffer[index++] = 8;
    mailbox_buffer[index++] = 0;
    mailbox_buffer[index++] = 0;
    mailbox_buffer[index++] = 0;

    mailbox_buffer[index++] = 0x00040001;
    mailbox_buffer[index++] = 8;
    mailbox_buffer[index++] = 0;
    mailbox_buffer[index++] = 16;
    mailbox_buffer[index++] = 0;

    mailbox_buffer[index++] = 0x00040008;
    mailbox_buffer[index++] = 4;
    mailbox_buffer[index++] = 0;
    mailbox_buffer[index++] = 0;


    mailbox_buffer[index++] = 0;

    mailbox_buffer[0] = index * sizeof(uint32_t);


    if (mailbox_call((void*)mailbox_buffer) != 0) {
        framebuffer_ptr = NULL;
        screen_width = 0;
        screen_height = 0;
        screen_pitch = 0;
        screen_depth = 0;
        return -1;
    }

    index = 2;
    while (mailbox_buffer[index] != 0) {
        uint32_t tag_id = mailbox_buffer[index];
        uint32_t tag_buffer_size = mailbox_buffer[index + 1];
        uint32_t tag_code = mailbox_buffer[index + 2];

        if (!(tag_code & 0x80000000)) {
            index += 3 + tag_buffer_size / sizeof(uint32_t);
            continue;
        }

        switch (tag_id) {
            case 0x00040001:
                framebuffer_ptr = (volatile uint32_t*)(uintptr_t)(mailbox_buffer[index + 3] & ~0xC0000000);
                break;

            case 0x00040008:
                screen_pitch = mailbox_buffer[index + 3];
                break;

            case 0x00048003:
                screen_width = mailbox_buffer[index + 3];
                screen_height = mailbox_buffer[index + 4];
                break;

        }
        index += 3 + tag_buffer_size / sizeof(uint32_t);
    }


    if (framebuffer_ptr == NULL || screen_width == 0 || screen_height == 0 || screen_pitch == 0 || screen_depth == 0) {
        framebuffer_ptr = NULL;
        return -1;
    }

    return 0;
}

void invert_color(uint32_t* color) {
    uint8_t r = (*color >> 16) & 0xFF;
    uint8_t g = (*color >> 8) & 0xFF;
    uint8_t b = *color & 0xFF;

    *color = (b << 16) | (g << 8) | r;
}

void draw_pixel(int x, int y, uint32_t color) {
    if (framebuffer_ptr == NULL || x < 0 || x >= screen_width || y < 0 || y >= screen_height) {
        return;
    }
    volatile uint32_t* pixel_address = framebuffer_ptr + (y * screen_pitch + x * (screen_depth / 8)) / sizeof(uint32_t);

    invert_color(&color);
    *pixel_address = color;
}

void draw_rect(int x, int y, int w, int h, uint32_t color) {
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            draw_pixel(x + i, y + j, color);
        }
    }
}

void draw_empty_rect(int x, int y, int w, int h, uint32_t color) {
    if (framebuffer_ptr == NULL) return;

    draw_line(x, y, x + w - 1, y, color, 1);
    draw_line(x, y + h - 1, x + w - 1, y + h - 1, color, 1);
    draw_line(x, y, x, y + h - 1, color, 1);
    draw_line(x + w - 1, y, x + w - 1, y + h - 1, color, 1);
}

void screen_clear(uint32_t color) {
    if (framebuffer_ptr == NULL) return;

    int num_pixels = screen_width * screen_height;
    if (screen_pitch == screen_width * (screen_depth / 8)) {
        for (int i = 0; i < num_pixels; i++) {
            framebuffer_ptr[i] = color;
        }
    } else {
        for (int y = 0; y < screen_height; y++) {
            for (int x = 0; x < screen_width; x++) {
                draw_pixel(x, y, color);
            }
        }
    }
}

void draw_circle(int x0, int y0, int radius, uint32_t color) {
    if (framebuffer_ptr == NULL) return;

    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                draw_pixel(x0 + x, y0 + y, color);
            }
        }
    }
}

void draw_line(int x0, int y0, int x1, int y1, uint32_t color, int thickness) {
    if (framebuffer_ptr == NULL) return;

    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        draw_rect(x0, y0, thickness, thickness, color);

        if (x0 == x1 && y0 == y1) {
            break;
        }

        int err2 = err * 2;
        if (err2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (err2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void draw_rounded_rect(int x, int y, int w, int h, int r, uint32_t color) {
    if (framebuffer_ptr == NULL) return;

    draw_rect(x + r, y, w - 2 * r, h, color);
    draw_rect(x, y + r, w, h - 2 * r, color);

    draw_circle(x + r, y + r, r, color);
    draw_circle(x + w - r, y + r, r, color);
    draw_circle(x + r, y + h - r, r, color);
    draw_circle(x + w - r, y + h - r, r, color);
}

typedef struct {
    int x0;
    int y0;
    int x1;
    int y1;
} Vertex;

typedef struct {
    char letter;
    Vertex* vertices;
    int vertex_count;
} Letter;

static Letter letters[] = {
        {
                .letter = 'A',
                .vertex_count = 5,
                .vertices = (Vertex[]) {
                        {0, 0, 0, 20},
                        {0, 0, 10, 0},
                        {10, 0, 10, 20},
                        {10, 10, 0, 10},
                        {0, 10, 0, 20}
                }
        },
        {
                .letter = 'B',
                .vertex_count = 6,
                .vertices = (Vertex[]) {
                        {0, 0, 0, 20},
                        {0, 0, 10, 0},
                        {10, 0, 10, 20},
                        {10, 10, 0, 10},
                        {0, 10, 0, 20},
                        {10, 20, 0, 20}
                }
        },
        {
                .letter = 'C',
                .vertex_count = 3,
                .vertices = (Vertex[]) {
                        {0, 0, 0, 20},
                        {0, 0, 10, 0},
                        {10, 20, 0, 20}
                }
        },
        {
                .letter = 'D',
                .vertex_count = 5,
                .vertices = (Vertex[]) {
                        {0, 0, 0, 20},
                        {0, 0, 10, 0},
                        {10, 0, 10, 20},
                        {10, 20, 0, 20},
                        {10, 20, 0, 20}
                }
        },
        {
                .letter = 'E',
                .vertex_count = 4,
                .vertices = (Vertex[]) {
                        {0, 0, 0, 20},
                        {0, 0, 10, 0},
                        {10, 20, 0, 20},
                        {0, 10, 10, 10}
                }
        },
        {
                .letter = 'F',
                .vertex_count = 3,
                .vertices = (Vertex[]) {
                        {0, 0, 0, 20},
                        {0, 0, 10, 0},
                        {10, 10, 0, 10}
                }
        },
        {
                .letter = 'G',
                .vertex_count = 6,
                .vertices = (Vertex[]) {
                        {0, 0, 0, 20},
                        {0, 0, 10, 0},
                        {10, 20, 0, 20},
                        {10, 10, 5, 10},
                        {10, 20, 10, 20},
                        {10, 20, 10, 10}
                }
        },
        {
                .letter = 'H',
                .vertex_count = 3,
                .vertices = (Vertex[]) {
                        {0, 0, 0, 20},
                        {10, 0, 10, 20},
                        {10, 10, 0, 10}
                }
        },
        {
                .letter = 'I',
                .vertex_count = 3,
                .vertices = (Vertex[]) {
                        {5, 0, 5, 20},
                        {0, 0, 10, 0},
                        {10, 20, 0, 20}
                }
        },
        {
                .letter = 'J',
                .vertex_count = 4,
                .vertices = (Vertex[]) {
                        {10, 0, 10, 20},
                        {0, 0, 10, 0},
                        {10, 20, 0, 20},
                        {0, 20, 0, 10}
                }
        },
        {
                .letter = 'K',
                .vertex_count = 3,
                .vertices = (Vertex[]) {
                        {0, 0, 0, 20},
                        {0, 10, 10, 0},
                        {10, 20, 0, 10}
                }
        },
        {
                .letter = 'L',
                .vertex_count = 2,
                .vertices = (Vertex[]) {
                        {0, 0, 0, 20},
                        {0, 20, 10, 20}
                }
        },
        {
                .letter = 'M',
                .vertex_count = 4,
                .vertices = (Vertex[]) {
                        {0, 0, 0, 20},
                        {0, 0, 5, 10},
                        {10, 0, 5, 10},
                        {10, 0, 10, 20},
                }
        },
        {
                .letter = 'N',
                .vertex_count = 3,
                .vertices = (Vertex[]) {
                        {0, 0, 0, 20},
                        {0, 0, 10, 20},
                        {10, 0, 10, 20}
                }
        },
        {
                .letter = 'O',
                .vertex_count = 4,
                .vertices = (Vertex[]) {
                        {0, 0, 0, 20},
                        {10, 0, 10, 20},
                        {0, 20, 10, 20},
                        {0, 0, 10, 0}
                }
        },
        {
                .letter = 'P',
                .vertex_count = 4,
                .vertices = (Vertex[]) {
                        {0, 0, 0, 20},
                        {0, 0, 10, 0},
                        {10, 0, 10, 10},
                        {10, 10, 0, 10},
                }
        },
        {
                .letter = 'Q',
                .vertex_count = 5,
                .vertices = (Vertex[]) {
                        {0, 0, 0, 20},
                        {10, 0, 10, 20},
                        {0, 20, 10, 20},
                        {0, 0, 10, 0},
                        {5, 15, 10, 20}
                }
        },
        {
                .letter = 'R',
                .vertex_count = 6,
                .vertices = (Vertex[]) {
                        {0, 20, 0, 0},
                        {0, 0, 10, 0},
                        {10, 0, 10, 10},
                        {10, 10, 0, 10},
                        {0, 10, 5, 15},
                        {5, 15, 10, 20}
                }
        },
        {
                .letter = 'S',
                .vertex_count = 5,
                .vertices = (Vertex[]) {
                        {0, 20, 10, 20},
                        {10, 20, 10, 10},
                        {10, 10, 0, 10},
                        {0, 10, 0, 0},
                        {0, 0, 10, 0}
                }
        },
        {
                .letter = 'T',
                .vertex_count = 3,
                .vertices = (Vertex[]) {
                        {0, 0, 10, 0},
                        {5, 0, 5, 10},
                        {5, 10, 5, 20}
                }
        },
        {
                .letter = 'U',
                .vertex_count = 3,
                .vertices = (Vertex[]) {
                        {0, 0, 0, 20},
                        {10, 0, 10, 20},
                        {0, 20, 10, 20}
                }
        },
        {
                .letter = 'V',
                .vertex_count = 2,
                .vertices = (Vertex[]) {
                        {0, 0, 5, 20},
                        {10, 0, 5, 20}
                }
        },
        {
                .letter = 'W',
                .vertex_count = 4,
                .vertices = (Vertex[]) {
                        {0, 0, 0, 20},
                        {0, 20, 5, 10},
                        {10, 20, 5, 10},
                        {10, 20, 10, 0}
                }
        },
        {
                .letter = 'X',
                .vertex_count = 4,
                .vertices = (Vertex[]) {
                        {0, 0, 5, 10},
                        {5, 10, 10, 20},
                        {0, 20, 5, 10},
                        {5, 10, 10, 0}
                }
        },
        {
                .letter = 'Y',
                .vertex_count = 3,
                .vertices = (Vertex[]) {
                        {0, 0, 5, 10},
                        {10, 0, 5, 10},
                        {5, 10, 5, 20}
                }
        },
        {
                .letter = 'Z',
                .vertex_count = 3,
                .vertices = (Vertex[]) {
                        {0, 0, 10, 0},
                        {10, 0, 0, 20},
                        {0, 20, 10, 20}
                }
        }
};

void draw_letter(int x, int y, char letter, uint32_t color, int font_size, bool bold) {
    if (framebuffer_ptr == NULL) return;

    for (int i = 0; i < sizeof(letters) / sizeof(Letter); i++) {
        if (letters[i].letter == letter) {
            for (int j = 0; j < letters[i].vertex_count; j++) {
                Vertex* vertex = &letters[i].vertices[j];
                draw_line(x + vertex->x0 * font_size, y + vertex->y0 * font_size,x + vertex->x1 * font_size, y + vertex->y1 * font_size, color, bold ? 4 : 2);
            }
            break;
        }
    }
}

void draw_string(int x, int y, const char* str, uint32_t color, int font_size, int letter_spacing, bool bold) {
    while (*str) {
        draw_letter(x, y, *str, color, font_size, bold);
        x += letter_spacing;
        str++;
    }
}

int abs(int n) {
    return (n < 0) ? -n : n;
}
