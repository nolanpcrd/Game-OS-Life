#ifndef MAP_H
#define MAP_H

uint8_t* get_map();
int get_map_size();
void update_game();
int get_wrapped_index(int x, int y);
void toggle_cell(int x_selected, int y_selected);

#endif