/******************************************************************************
 * Nth Pong Wars, figuring out ncurses library for a text mode version.
 * AGMS20240425 - Try to set up ncurses.
 * AGMS20240501 - Get a basic ball bouncing between walls.
 * AGMS20240720 - Switch to fixed point math using a structure.
 *
 * Compile with: gcc -g -O2 -lncurses -o NCursesTest.exe main.c
 */

#include <ctype.h> /* For toupper() */
#include <stdio.h> /* For printf and so on. */
#include <curses.h> /* We're actually using ncurses, version 5.2 or later. */
#include <signal.h>

#include "../common/fixed_point.h" /* Use our "fx" fixed point math library. */


/*******************************************************************************
 * Constants, types.
 */


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
  int frame_count = 0;
  int row = 0;
  char score_string[80];
  fx FX_ONE_HALF;
  fx fx_test;
  fx fx_quarter;

  /* Rather than having to compute it each time it is used, save a constant. */
  FLOAT_TO_FX(0.5, FX_ONE_HALF);

  FLOAT_TO_FX(123.456, fx_test);
  printf ("123.456 is %d or %X.\n", fx_test.as_int, fx_test.as_int);
  printf ("Int part is %d.\n", GET_FX_INTEGER(fx_test));
  printf ("Fract part is %d / 65536 or %6.4f.\n",
    GET_FX_FRACTION(fx_test),
    GET_FX_FRACTION(fx_test) / 65536.0);

  INT_TO_FX(-41, fx_test);
  printf ("-41 is %d or %X.\n", fx_test.as_int, fx_test.as_int);
  printf ("Int part is %d.\n", GET_FX_INTEGER(fx_test));
  printf ("Fract part is %d / 65536 or %6.4f.\n",
    GET_FX_FRACTION(fx_test),
    GET_FX_FRACTION(fx_test) / 65536.0);

  DIV4_FX(fx_test, fx_quarter);
  printf ("-41/4 is %d or %X.\n", fx_quarter.as_int, fx_quarter.as_int);
  printf ("Int part is %d.\n", GET_FX_INTEGER(fx_quarter));
  printf ("Fract part is %d / 65536 or %6.4f.\n",
    GET_FX_FRACTION(fx_quarter),
    GET_FX_FRACTION(fx_quarter) / 65536.0);

  INT_TO_FX(0, fx_test);
  SUBTRACT_FX(fx_test, fx_quarter, fx_test);
  printf ("-(-41/4) is %d or %X.\n", fx_test.as_int, fx_test.as_int);
  printf ("Int part is %d.\n", GET_FX_INTEGER(fx_test));
  printf ("Fract part is %d / 65536 or %6.4f.\n",
    GET_FX_FRACTION(fx_test),
    GET_FX_FRACTION(fx_test) / 65536.0);

  fx ball_velocity_x; INT_TO_FX(0, ball_velocity_x);
  fx ball_velocity_y; INT_TO_FX(0, ball_velocity_y);
  fx ball_position_x; INT_TO_FX(1, ball_position_x);
  fx ball_position_y; INT_TO_FX(1, ball_position_y);
  fx wall_left_x, wall_right_x, wall_top_y, wall_bottom_y;

  printf ("Starting the Nth Pong Wars experiment.  AGMS20240501\n");

  signal(SIGINT, stop_signal_handler);
  initscr();  /* Initialize the ncurses library, default screen in stdscr. */

  /* Find size of our screen, which sets our game board size too. */

  getmaxyx(stdscr, g_max_rows, g_max_cols);
  printf("Screen size %d rows, %d columns.\n",
    (int) g_max_rows, (int) g_max_cols);

  /* Ball position X can be from 1.0 to almost width-1, for an 80 column screen
     that would be 1.0 to 78.99.  It's drawn at the integer portion character
     cell, so 1 to 78.  Wall is cells at X = 0 and 79.  Y is similar. */
  INT_TO_FX(1, wall_left_x);
  INT_FRACTION_TO_FX(g_max_cols - 2, MAX_FX_FRACTION, wall_right_x);
  INT_TO_FX(1, wall_top_y);
  INT_FRACTION_TO_FX(g_max_rows - 2, MAX_FX_FRACTION, wall_bottom_y);

  cbreak(); /* take input chars one at a time, no wait for \n */
  noecho(); /* Don't print typed characters while playing the game. */
  /* Set the wait for a key before returning no key to the amount of time
     between updates.  Sets the frame rate in effect. */
  timeout(50 /* milliseconds */);
  keypad(stdscr, true); /* Convert function keys to single key code. */

  /* Set up the four ball colours, pair 0 is the default white text. */

  if (has_colors())
  {
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_RED, COLOR_BLACK);
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

  mvaddstr(0 /* row */, 1 /* col */, "Frame:");

  refresh; /* Later need to getch() to actually show the screen. */

  while (!g_stop_running)
  {
    int letter;

    letter = getch(); /* Read a character and do a time delay if none. */
    if (letter != ERR) /* Returns ERR if no character ready. */
    {
      /* Decode user input.  Q to quit, WASD or cursor keys to move,
         space to fire. */
      char letter_upcase = toupper(letter);

      if (letter_upcase == 'Q')
        g_stop_running = true;
      else if (letter == KEY_DOWN || letter_upcase == 'S')
        ADD_FX(ball_velocity_y, FX_ONE_HALF, ball_velocity_y);
      else if (letter == KEY_UP || letter_upcase == 'W')
        SUBTRACT_FX(ball_velocity_y, FX_ONE_HALF, ball_velocity_y);
      else if (letter == KEY_LEFT || letter_upcase == 'A')
        SUBTRACT_FX(ball_velocity_x, FX_ONE_HALF, ball_velocity_x);
      else if (letter == KEY_RIGHT || letter_upcase == 'D')
        ADD_FX(ball_velocity_x, FX_ONE_HALF, ball_velocity_x);
    }

    /* Erase old ball position with a space character. */

    attr_set(A_NORMAL, 0 /* colour pair # */, NULL /* options */);
    col = GET_FX_INTEGER(ball_position_x);
    row = GET_FX_INTEGER(ball_position_y);
    mvaddch(row, col, ' ');

    /* Update ball position and velocities - bounce off walls. */

    ADD_FX(ball_position_x, ball_velocity_x, ball_position_x);
    if (COMPARE_FX(ball_position_x, wall_left_x) < 0)
    {
      ball_position_x = wall_left_x; /* Don't go through wall. */
      /* velocity = -(vel - vel/4), reverse direction and reduce speed. */
      DIV4_FX(ball_velocity_x, fx_quarter);
      SUBTRACT_FX(fx_quarter, ball_velocity_x, ball_velocity_x);
    }
    else if (COMPARE_FX(ball_position_x, wall_right_x) > 0)
    {
      ball_position_x = wall_right_x;
      DIV4_FX(ball_velocity_x, fx_quarter);
      SUBTRACT_FX(fx_quarter, ball_velocity_x, ball_velocity_x);
    }

    ADD_FX(ball_position_y, ball_velocity_y, ball_position_y);
    if (COMPARE_FX(ball_position_y, wall_top_y) < 0)
    {
      ball_position_y = wall_top_y;
      DIV4_FX(ball_velocity_y, fx_quarter);
      SUBTRACT_FX(fx_quarter, ball_velocity_y, ball_velocity_y);
    }
    else if (COMPARE_FX(ball_position_y, wall_bottom_y) > 0)
    {
      ball_position_y = wall_bottom_y;
      DIV4_FX(ball_velocity_y, fx_quarter);
      SUBTRACT_FX(fx_quarter, ball_velocity_y, ball_velocity_y);
    }

    /* Draw the ball. */

    attr_set(A_NORMAL, 1 /* colour pair # */, NULL /* options */);
    col = GET_FX_INTEGER(ball_position_x);
    row = GET_FX_INTEGER(ball_position_y);
    mvaddch(row, col, 'O');

    /* Print the score. */

    snprintf(score_string, sizeof(score_string) - 1,
      "%d, (%9.5f,%9.5f), (%9.5f,%9.5f)", frame_count,
      GET_FX_FLOAT(ball_position_x),
      GET_FX_FLOAT(ball_position_y),
      GET_FX_FLOAT(ball_velocity_x),
      GET_FX_FLOAT(ball_velocity_y));
    attr_set(A_REVERSE, 0 /* colour pair # */, NULL /* options */);
    mvaddstr(0 /* row */, 10 /* col */, score_string);

    refresh;
    frame_count++;
  }

  endwin();
  printf("Done.\n");
}

