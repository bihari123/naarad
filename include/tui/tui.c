#include "tui.h"
#include "../tui/hashmap.h"
#include "../utils/logs.h"
#include "lock_free_queue.h"
#include <grp.h>
#include <pthread.h>
#include <pwd.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
static OperationCounters counters = {.open_ops = ATOMIC_VAR_INIT(0),
                                     .read_ops = ATOMIC_VAR_INIT(0),
                                     .write_ops = ATOMIC_VAR_INIT(0),
                                     .unlink_ops = ATOMIC_VAR_INIT(0),
                                     .create_ops = ATOMIC_VAR_INIT(0),
                                     .link_ops = ATOMIC_VAR_INIT(0),
                                     .unlink_ops = ATOMIC_VAR_INIT(0),
                                     .syslink_ops = ATOMIC_VAR_INIT(0),
                                     .mkdir_ops = ATOMIC_VAR_INIT(0),
                                     .rmdir_ops = ATOMIC_VAR_INIT(0),
                                     .mknod_ops = ATOMIC_VAR_INIT(0),
                                     .rename_ops = ATOMIC_VAR_INIT(0)};

static uint64_t open_ss = 1;
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

// Function to read the value of a specific counter
uint64_t read_counter(uint8_t op_type) {
  switch (op_type) {
  case OP_READ:
    return atomic_load_explicit(&counters.read_ops, memory_order_relaxed);
  case OP_WRITE:
    return atomic_load_explicit(&counters.write_ops, memory_order_relaxed);
  case OP_UNLINK:
    return atomic_load_explicit(&counters.unlink_ops, memory_order_relaxed);
  case OP_OPEN:

    return atomic_load_explicit(&counters.open_ops, memory_order_relaxed);

  case OP_CREATE:
    return atomic_load_explicit(&counters.create_ops, memory_order_relaxed);
  case OP_LINK:
    return atomic_load_explicit(&counters.link_ops, memory_order_relaxed);
  case OP_SYSLINK:
    return atomic_load_explicit(&counters.syslink_ops, memory_order_relaxed);
  case OP_MKDIR:
    return atomic_load_explicit(&counters.mkdir_ops, memory_order_relaxed);
  case OP_RMDIR:
    return atomic_load_explicit(&counters.rmdir_ops, memory_order_relaxed);
  case OP_MKNOD:
    return atomic_load_explicit(&counters.mknod_ops, memory_order_relaxed);
  case OP_RENAME:
    return atomic_load_explicit(&counters.rename_ops, memory_order_relaxed);
  default:
    return 0; // Invalid operation type
  }
}

// Function to increment a specific counter
void increment_counter(uint8_t op_type) {
  log_info("inside increment for op_type: %d", op_type);
  switch (op_type) {
  case OP_READ:
    atomic_fetch_add_explicit(&counters.read_ops, 1, memory_order_relaxed);
    break;
  case OP_WRITE:
    atomic_fetch_add_explicit(&counters.write_ops, 1, memory_order_relaxed);
    break;
  case OP_UNLINK:
    atomic_fetch_add_explicit(&counters.unlink_ops, 1, memory_order_relaxed);
    break;
  case OP_OPEN:
    atomic_fetch_add_explicit(&counters.open_ops, 1, memory_order_relaxed);
    log_info("after update, the counter for open ius %u",
             read_counter(OP_OPEN));
    break;
  case OP_CREATE:
    atomic_fetch_add_explicit(&counters.create_ops, 1, memory_order_relaxed);
    break;
  case OP_LINK:
    atomic_fetch_add_explicit(&counters.link_ops, 1, memory_order_relaxed);
    break;
  case OP_SYSLINK:
    atomic_fetch_add_explicit(&counters.syslink_ops, 1, memory_order_relaxed);
    break;
  case OP_MKDIR:
    atomic_fetch_add_explicit(&counters.mkdir_ops, 1, memory_order_relaxed);
    break;
  case OP_RMDIR:
    atomic_fetch_add_explicit(&counters.rmdir_ops, 1, memory_order_relaxed);
    break;
  case OP_MKNOD:
    atomic_fetch_add_explicit(&counters.mknod_ops, 1, memory_order_relaxed);
    break;
  case OP_RENAME:
    atomic_fetch_add_explicit(&counters.rename_ops, 1, memory_order_relaxed);
    break;
  default:
    log_info("op type unknown");
    // Do nothing for invalid operation type
    break;
  }
}

WINDOW *content_win;

void clear_screen() {
  if (!initscr()) {
    fprintf(stderr, "Error initializing ncurses.\n");
    exit(1);
  }
  clear();
  refresh();
  endwin();
}

void init_ncurses() {
  pthread_rwlock_init(&rwlock, NULL);

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  if (has_colors() == FALSE) {
    endwin();
    printf("Your terminal does not support color\n");
    exit(1);
  }
  start_color();
  // Base colors
  init_color(COLOR_BLACK, 110, 110, 110); // #1C1C1C
  init_color(COLOR_WHITE, 878, 878, 878); // #E0E0E0

  // Accent colors
  init_color(10, 0, 1000, 0);   // #00FF00 (bright green)
  init_color(11, 294, 0, 510);  // #4B0082 (indigo)
  init_color(12, 1000, 271, 0); // #FF4500 (orange-red)

  // Status colors
  init_color(13, 196, 804, 196); // #32CD32 (lime green)
  init_color(14, 1000, 843, 0);  // #FFD700 (gold)
  init_color(15, 1000, 0, 0);    // #FF0000 (red)

  // Graph colors
  init_color(16, 118, 565, 1000); // #1E90FF (dodger blue)
  init_color(17, 1000, 412, 706); // #FF69B4 (hot pink)
  init_color(18, 486, 988, 0);    // #7CFC00 (lawn green)
  init_color(19, 1000, 549, 0);   // #FF8C00 (dark orange)

  // Additional colors
  init_color(20, 184, 310, 310); // #2F4F4F (dark slate gray)
  init_color(21, 239, 239, 239); // #3D3D3D (lighter gray)
  init_color(22, 1000, 1000, 0); // #FFFF00 (yellow)
  init_color(23, 255, 412, 882); // #4169E1 (royal blue)
  init_color(24, 439, 502, 565); // #708090 (slate gray)
  init_color(25, 412, 412, 412); // #696969 (dim gray)

  // Define color pairs
  init_pair(1, COLOR_WHITE, COLOR_BLACK); // Default text
  init_pair(2, 25, COLOR_BLACK);          // Primary accent
  init_pair(3, 11, COLOR_BLACK);          // Secondary accent
  init_pair(4, COLOR_WHITE, 21);          // Tertiary accent
  init_pair(5, 13, COLOR_BLACK);          // Success status
  init_pair(6, COLOR_BLACK, 14);          // Warning status
  init_pair(7, COLOR_BLACK, 15);          // Error status
  init_pair(8, COLOR_BLACK, 16);          // Graph bar 1
  init_pair(9, COLOR_BLACK, 17);          // Graph bar 2
  init_pair(10, COLOR_BLACK, 18);         // Graph bar 3
  init_pair(11, 19, COLOR_BLACK);         // Graph bar 4
  init_pair(12, COLOR_WHITE, 20);         // Header/footer text
  init_pair(13, COLOR_WHITE, 21);         // Highlighted row
  init_pair(14, 22, COLOR_BLACK);         // Updated cell
  init_pair(15, COLOR_WHITE, 23);         // Active tab
  init_pair(16, COLOR_WHITE, 24);         // Inactive tab
  init_pair(17, 25, COLOR_BLACK);         // Border color
  clear();
  refresh();

  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);
  content_win = newwin(max_y - 3, max_x, 3, 0);
  wbkgd(content_win, COLOR_PAIR(1));
  // Enable nodelay mode
  nodelay(stdscr, TRUE);
  // Enable immediate echo of characters
  timeout(0);
}

void draw_bar_graph(WINDOW *win) {
  int max_x, max_y;
  getmaxyx(win, max_y, max_x);

  const char *labels[] = {"read", "write", "delete", "rename", "misc"};
  uint64_t values[5];
  uint64_t max_value = 1;

  enum { read = 0, write = 1, delete = 2, rename = 3, misc = 4 };
  values[0] = read_counter(OP_OPEN) + read_counter(OP_READ);
  values[1] =
      read_counter(OP_WRITE) + read_counter(OP_CREATE) + read_counter(OP_MKDIR);
  values[2] = read_counter(OP_UNLINK) + read_counter(OP_RMDIR);
  values[3] = read_counter(OP_RENAME);
  values[4] =
      read_counter(OP_LINK) + read_counter(OP_SYSLINK) + read_counter(OP_MKNOD);
  // Generate random values and find the maximum
  for (int i = 0; i < 5; i++) {
    /* open_ss++; */
    log_info("value[%d] is %d", i, values[i]);
    if (values[i] > max_value) {
      max_value = values[i];
    }
  }

  // Draw bars
  for (int i = 0; i < 5; i++) {
    int bar_length = (values[i] * (max_x - 15)) / max_value;
    int color_pair = 6 + i; // Use color pairs 6-10 for the bars
    wattron(win, COLOR_PAIR(color_pair));
    mvwhline(win, max_y - 3 - i * 2, 2, ' ', bar_length);
    wattroff(win, COLOR_PAIR(color_pair));
    mvwprintw(win, max_y - 3 - i * 2, bar_length + 3, "%d", (int)values[i]);
    mvwprintw(win, max_y - 3 - i * 2, 1, "%s", labels[i]);
  }

  // Draw legend
  mvwprintw(win, max_y - 1, 2, "Legend:");
  for (int i = 0; i < 5; i++) {
    wattron(win, COLOR_PAIR(6 + i));
    mvwprintw(win, max_y - 1, 11 + i * 8, "  ");
    wattroff(win, COLOR_PAIR(6 + i));
    mvwprintw(win, max_y - 1, 13 + i * 8, "%s", labels[i]);
  }
}

void draw_paragraph(WINDOW *win) {
  const char *text =
      "This is a sample paragraph to demonstrate text wrapping in ncurses. "
      "It will automatically wrap to the next line when it reaches the edge of "
      "the window. "
      "This allows for easy display of longer text content within a confined "
      "space.";

  int max_x, max_y, start_y = 2, start_x = 2;
  getmaxyx(win, max_y, max_x);

  for (const char *c = text; *c != '\0'; c++) {
    mvwaddch(win, start_y, start_x, *c);
    if (++start_x >= max_x - 2) {
      start_x = 2;
      if (++start_y >= max_y - 1)
        break;
    }
  }
}

void draw_list(WINDOW *win) {
  const char *items[] = {"Apple", "Banana", "Cherry", "Date", "Elderberry"};
  int max_y, max_x;
  getmaxyx(win, max_y, max_x);

  for (int i = 0; i < 5 && i < max_y - 2; i++) {
    mvwprintw(win, i + 2, 2, "%d. %s", i + 1, items[i]);
  }
}

void create_blocks(Block blocks[], int num_blocks, int tab_index) {
  int max_y, max_x;
  getmaxyx(content_win, max_y, max_x);

  int block_width = max_x;
  int block_height = max_y / num_blocks;

  for (int i = 0; i < num_blocks; i++) {
    blocks[i].width = block_width;
    blocks[i].height = block_height;
    blocks[i].startx = 0;
    blocks[i].starty = i * block_height;
    blocks[i].win = derwin(content_win, block_height, block_width,
                           blocks[i].starty, blocks[i].startx);
    if (blocks[i].win == NULL) {
      mvprintw(max_y - 1, 0, "Failed to create window for block %d", i + 1);
      refresh();
      return;
    }
    box(blocks[i].win, 0, 0);
    wbkgd(blocks[i].win, COLOR_PAIR(1));
  }
}

void create_table(Tab *tab) {
  int max_y, max_x;
  getmaxyx(content_win, max_y, max_x);

  tab->table_win = derwin(content_win, max_y, max_x, 0, 0);
  if (tab->table_win == NULL) {
    mvprintw(max_y - 1, 0, "Failed to create table window");
    refresh();
    return;
  }

  box(tab->table_win, 0, 0);
  wbkgd(tab->table_win, COLOR_PAIR(1));
}

void draw_table_headers(WINDOW *win) {
  int max_x, max_y;
  getmaxyx(win, max_y, max_x);

  mvwprintw(win, 1, 2, "FilePath");
  mvwprintw(win, 1, 40, "Process");
  mvwprintw(win, 1, 52, "Operation");
  /* mvwprintw(win, 1, 64, "Renamed"); */
  mvwprintw(win, 1, 64, "User");
  mvwprintw(win, 1, 76, "Group");

  mvwchgat(win, 1, 1, max_x - 2, A_BOLD, 5, NULL);
}

void update_table(Tab *tab) {
  Message *array = displayEntries();

  if (array == NULL) {
    return;
  }

  werase(tab->table_win);
  box(tab->table_win, 0, 0);
  draw_table_headers(tab->table_win);

  for (int i = 0; i < MAX_ENTRIES; i++) {
    log_info("inside the index %d", i);
    if (array[i].op == STOP_MESSAG || array[i].file[0] == '\0') {
      break;
    }
    struct passwd *pw;
    struct group *gr;
    pw = getpwuid(array[i].uid);
    gr = getgrgid(array[i].gid);

    /*
     * #define OP_OPEN 0
#define OP_READ 1
#define OP_WRITE 2
#define OP_CREATE 3
#define OP_LINK 4
#define OP_UNLINK 5
#define OP_SYSLINK 6
#define OP_MKDIR 7
#define OP_RMDIR 8
#define OP_MKNOD 9
#define OP_RENAME 10


     * */

    char operation[10];
    log_info("the code at the time of showing to the ui is %d", array[i].op);
    switch (array[i].op) {
    case 0: // OP_OPEN
      snprintf(operation, sizeof(operation), "open");
      log_info("open ops");
      break;
    case 1: // OP_READ
      snprintf(operation, sizeof(operation), "read");
      log_info("read ops");
      break;
    case 2: // OP_WRITE
      snprintf(operation, sizeof(operation), "write");
      log_info("write ops");
      break;
    case 3: // OP_CREATE
      snprintf(operation, sizeof(operation), "create");
      log_info("create ops");
      break;
    case 4: // OP_LINK
      snprintf(operation, sizeof(operation), "link");
      log_info("link ops");
      break;
    case 5: // OP_UNLINK
      snprintf(operation, sizeof(operation), "unlink");
      log_info("unlink ops");
      break;
    case 6: // OP_SYSLINK
      snprintf(operation, sizeof(operation), "syslink");
      log_info("syslink ops");
      break;
    case 7: // OP_MKDIR
      snprintf(operation, sizeof(operation), "mkdir");
      log_info("mkdir ops");
      break;
    case 8: // OP_RMDIR
      snprintf(operation, sizeof(operation), "rmdir");
      log_info("rmdir ops");
      break;
    case 9: // OP_MKNOD
      snprintf(operation, sizeof(operation), "mknod");
      log_info("mknod ops");
      break;
    case 10: // OP_RENAME
      snprintf(operation, sizeof(operation), "rename");
      log_info("rename ops");
      break;
    default:
      log_info("unknown ops");
      snprintf(operation, sizeof(operation), "unknown");
    }
    mvwprintw(tab->table_win, i + 3, 2, "%8s", array[i].file);
    mvwprintw(tab->table_win, i + 3, 30, "%15s", array[i].comm);
    mvwprintw(tab->table_win, i + 3, 48, "%8s", operation);
    mvwprintw(tab->table_win, i + 3, 62, "%8s", pw->pw_name);
    mvwprintw(tab->table_win, i + 3, 72, "%8s", gr->gr_name);
  }
  wnoutrefresh(tab->table_win);
}

void create_tabs(Tab tabs[], int num_tabs) {
  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  int tab_width = max_x / num_tabs;

  tabs[0].win = newwin(3, tab_width, 0, 0 * tab_width);
  if (tabs[0].win == NULL) {
    mvprintw(max_y - 1, 0, "Failed to create tab window %d", 1);
    refresh();
    return;
  }
  tabs[0].title = malloc(20 * sizeof(char));
  snprintf(tabs[0].title, 20, "Overview");
  create_blocks(tabs[0].blocks, NUM_BLOCKS, 0);

  tabs[1].win = newwin(3, tab_width, 0, 1 * tab_width);
  if (tabs[1].win == NULL) {
    mvprintw(max_y - 1, 0, "Failed to create tab window %d", 2);
    refresh();
    return;
  }
  tabs[1].title = malloc(20 * sizeof(char));
  snprintf(tabs[1].title, 20, "File Activity");
  create_table(&tabs[1]);
}

void draw_tabs(Tab tabs[], int num_tabs, int current_tab) {
  for (int i = 0; i < num_tabs; i++) {
    if (i == current_tab) {
      wbkgd(tabs[i].win, COLOR_PAIR(4));
    } else {
      wbkgd(tabs[i].win, COLOR_PAIR(2));
    }
    werase(tabs[i].win);
    box(tabs[i].win, 0, 0);
    mvwprintw(tabs[i].win, 1, 2, "%s", tabs[i].title);
    wnoutrefresh(tabs[i].win);
  }
}

void show_tab(Tab *tab, int tab_index, int current_block) {
  werase(content_win);
  if (tab_index == 0) {
    for (int i = 0; i < NUM_BLOCKS; i++) {
      if (i == current_block) {
        wbkgd(tab->blocks[i].win, COLOR_PAIR(3));
      } else {
        wbkgd(tab->blocks[i].win, COLOR_PAIR(1));
      }
      werase(tab->blocks[i].win);
      box(tab->blocks[i].win, 0, 0);
      switch (i) {
      case 0:
        mvwprintw(tab->blocks[i].win, 1, 2, "Bar Graph");
        draw_bar_graph(tab->blocks[i].win);
        break;
      case 1:
        mvwprintw(tab->blocks[i].win, 1, 2, "Paragraph");
        draw_paragraph(tab->blocks[i].win);
        break;
      case 2:
        mvwprintw(tab->blocks[i].win, 1, 2, "List");
        draw_list(tab->blocks[i].win);
        break;
      }
      wnoutrefresh(tab->blocks[i].win);
    }
  } else {
    update_table(tab);
  }
  wnoutrefresh(content_win);
}

void redraw_screen(Tab tabs[], int num_tabs, int current_tab,
                   int current_block) {
  clear();
  usleep(10000);
  refresh();
  draw_tabs(tabs, num_tabs, current_tab);
  show_tab(&tabs[current_tab], current_tab, current_block);
  mvprintw(LINES - 1, 0,
           "Use arrow keys to move, TAB/SHIFT+TAB to switch tabs, 'q' to quit");
  refresh();
}
