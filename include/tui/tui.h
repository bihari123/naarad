#pragma once

#include <ncurses.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define NUM_BLOCKS 3
#define NUM_TABS 2
// #define TABLE_ROWS 5
#define TABLE_COLS 3
#define BAR_HEIGHT 5
#define NUM_COLORS 5

// Enum to represent different operation types

// Structure to hold all operation counters
typedef struct {
  _Atomic uint64_t open_ops;
  _Atomic uint64_t read_ops;
  _Atomic uint64_t write_ops;
  _Atomic uint64_t unlink_ops;
  _Atomic uint64_t create_ops;
  _Atomic uint64_t link_ops;
  _Atomic uint64_t syslink_ops;
  _Atomic uint64_t mkdir_ops;
  _Atomic uint64_t rmdir_ops;
  _Atomic uint64_t mknod_ops;
  _Atomic uint64_t rename_ops;
} OperationCounters;
// OperationCounters counters;
// Function to read the value of a specific counter
uint64_t read_counter(uint8_t op_type);
void increment_counter(uint8_t op_type);

typedef struct {
  WINDOW *win;
  int startx, starty;
  int width, height;
} Block;

typedef struct {
  WINDOW *win;
  char *title;
  Block blocks[NUM_BLOCKS];
  WINDOW *table_win;
} Tab;

void clear_screen();
void init_ncurses();
void draw_bar_graph(WINDOW *win);
void draw_paragraph(WINDOW *win);
void draw_list(WINDOW *win);
void create_blocks(Block blocks[], int num_blocks, int tab_index);
void create_table(Tab *tab);
void draw_table_headers(WINDOW *win);
void update_table(Tab *tab);
void show_tab(Tab *tab, int tab_index, int current_block);
void draw_tabs(Tab tabs[], int num_tabs, int current_tab);
void create_tabs(Tab tabs[], int num_tabs);
void redraw_screen(Tab tabs[], int num_tabs, int current_tab,
                   int current_block);
