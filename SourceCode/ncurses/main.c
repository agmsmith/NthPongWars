/******************************************************************************
 * Nth Pong Wars, figuring out ncurses library for a text mode version.
 * AGMS20240425
 *
 * Compile with: gcc -g -O2 -lncurses -o NCursesTest main.c
 */

#include <stdlib.h>
#include <curses.h> /* We're actually using ncurses, version 5.2 or later. */
#include <signal.h>


static void finish(int sig)
{
    endwin();
    exit(0);
}


int main(int argc, const char **argv)
{
  int error = 0;
  int col = 0;
  int row = 0;
  NCURSES_SIZE_T MaxCols, MaxRows;

  printf ("Hello world.\n");

  signal(SIGINT, finish);      /* arrange interrupts to terminate */
  initscr();      /* initialize the curses library */
  getmaxyx(stdscr, MaxRows, MaxCols);
  printf("Screen size %d rows, %d columns.\n", MaxRows, MaxCols);

  error = addstr("Test String");
  printf("Error %d when adding a string.\n", error);
  for (row = 0; row < 24; row++)
  {
    for (col = row; col < 80; col += 10)
    {
      move(row, col);
      addch('A' | A_NORMAL);
    }
  }
  refresh;
  getch(); /* Need to delay to show the screen, gets cleared at exit. */
  printf("Done.\n");
  finish(0);
}

