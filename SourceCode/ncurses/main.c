/******************************************************************************
 * Nth Pong Wars, figuring out ncurses library for a text mode version.
 * AGMS20240425 - Try to set up ncurses.
 * AGMS20240501 - Get a basic ball bouncing between walls.
 *
 * Compile with: gcc -g -O2 -lncurses -o NCursesTest.exe main.c
 */

#include <stdlib.h>
#include <curses.h> /* We're actually using ncurses, version 5.2 or later. */
#include <signal.h>

/*******************************************************************************
 * Global Variables
 *

/* Size of the screen in characters. */
NCURSES_SIZE_T g_max_rows = 0;
NCURSES_SIZE_T g_max_cols = 0;

/* Set this to true to make the main loop of the program exit. */
static bool g_stop_running = false;


static void
stop_signal_handler(int sig)
{
  g_stop_running = true;
}


int main(int argc, const char **argv)
{
  int col = 0;
  int row = 0;

  printf ("Starting the Nth Pong Wars experiment.  AGMS20240501\n");

  signal(SIGINT, stop_signal_handler);
  initscr();  /* Initialize the ncurses library, default screen in stdscr. */
  getmaxyx(stdscr, g_max_rows, g_max_cols);
  printf("Screen size %d rows, %d columns.\n",
    (int) g_max_rows, (int) g_max_cols);

  /* Set up the four ball colours, pair 0 is the default white text. */
  
  if (has_colors())
  {
    start_color();
    init_pair(1, COLOR_RED,     COLOR_BLACK);
    init_pair(2, COLOR_YELLOW,  COLOR_BLACK);
    init_pair(3, COLOR_BLUE,    COLOR_BLACK);
    init_pair(4, COLOR_CYAN,    COLOR_BLACK);
  }

  /* Draw the walls around the screen.  Reverse video for white spaces. */

  attr_set(A_REVERSE, 0 /* colour pair # */, NULL /* options */);

  for (row = 0; row < g_max_rows; row++)
  {
    mvaddch(row, 0, ' ');
    mvaddch(row, g_max_cols - 1, ' ');
  }
  for (col = 0; col < g_max_cols; col++)
  {
    mvaddch(0, col, ' ');
    mvaddch(g_max_rows - 1, col, ' ');
  }
  refresh;
  getch(); /* Need to delay to show the screen, gets cleared at exit. */
  printf("Done.\n");
  endwin();
}

