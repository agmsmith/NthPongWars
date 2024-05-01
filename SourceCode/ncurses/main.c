/******************************************************************************
 * Nth Pong Wars, figuring out ncurses library for a text mode version.
 * AGMS20240425 - Try to set up ncurses.
 * AGMS20240501 - Get a basic ball bouncing between walls.
 *
 * Compile with: gcc -g -O2 -lncurses -o NCursesTest.exe main.c
 */

#include <stdint.h> /* For __int16_t */
#include <ctype.h> /* For toupper() */
#include <stdio.h> /* For printf and so on. */
#include <curses.h> /* We're actually using ncurses, version 5.2 or later. */
#include <signal.h>

/*******************************************************************************
 * Constants, types.
 */

typedef __int32_t fixed_fraction; /* Game uses 16 bit int, 16 bit fraction. */
#define FRACTION_SHIFT 16
#define FRACTION_FLOAT ((float) (1 << FRACTION_SHIFT))
#define GET_FIXED_FRACTION(x) ((__int16_t) (x & 0xFFFF))
#define GET_FIXED_INTEGER(x) ((__int16_t) (x >> FRACTION_SHIFT))
#define INT_TO_FIXED_FRACTION(x) (((fixed_fraction) x) << FRACTION_SHIFT)
#define FLOAT_TO_FIXED_FRACTION(x) ((fixed_fraction) (x * FRACTION_FLOAT))
#define FIXED_TO_FLOAT_FRACTION(x) (x / FRACTION_FLOAT)


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

  fixed_fraction ff = 0;
  ff = FLOAT_TO_FIXED_FRACTION(123.456);
  printf ("123.456 is %d or %X.\n", ff, ff);
  printf ("Int part is %d.\n", GET_FIXED_INTEGER(ff));
  printf ("Fract part is %d / 65536 or %6.4f.\n",
    GET_FIXED_FRACTION(ff),
    GET_FIXED_FRACTION(ff) / 65536.0);
  ff = INT_TO_FIXED_FRACTION(-42);
  printf ("-42 is %d or %X.\n", ff, ff);
  printf ("Int part is %d.\n", GET_FIXED_INTEGER(ff));
  printf ("Fract part is %d / 65536 or %6.4f.\n",
    GET_FIXED_FRACTION(ff),
    GET_FIXED_FRACTION(ff) / 65536.0);

  fixed_fraction ball_velocity_x = 0;
  fixed_fraction ball_velocity_y = 0;
  fixed_fraction ball_position_x = INT_TO_FIXED_FRACTION(1);
  fixed_fraction ball_position_y = INT_TO_FIXED_FRACTION(1);
  fixed_fraction wall_left_x, wall_right_x, wall_top_y, wall_bottom_y;

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
  wall_left_x = INT_TO_FIXED_FRACTION(1);
  wall_right_x = INT_TO_FIXED_FRACTION(g_max_cols - 1) - 1;
  wall_top_y = INT_TO_FIXED_FRACTION(1);
  wall_bottom_y = INT_TO_FIXED_FRACTION(g_max_rows - 1) - 1;

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
        ball_velocity_y += FLOAT_TO_FIXED_FRACTION(0.5);
      else if (letter == KEY_UP || letter_upcase == 'W')
        ball_velocity_y -= FLOAT_TO_FIXED_FRACTION(0.5);
      else if (letter == KEY_LEFT || letter_upcase == 'A')
        ball_velocity_x -= FLOAT_TO_FIXED_FRACTION(0.5);
      else if (letter == KEY_RIGHT || letter_upcase == 'D')
        ball_velocity_x += FLOAT_TO_FIXED_FRACTION(0.5);
    }

    /* Erase old ball position with a space character. */

    attr_set(A_NORMAL, 0 /* colour pair # */, NULL /* options */);
    col = GET_FIXED_INTEGER(ball_position_x);
    row = GET_FIXED_INTEGER(ball_position_y);
    mvaddch(row, col, ' ');

    /* Update ball position and velocities - bounce off walls. */

    ball_position_x += ball_velocity_x;
    if (ball_position_x < wall_left_x)
    {
      ball_position_x = wall_left_x;
      ball_velocity_x = - (ball_velocity_x - ball_velocity_x / 4);
    }
    else if (ball_position_x > wall_right_x)
    {
      ball_position_x = wall_right_x;
      ball_velocity_x = - (ball_velocity_x - ball_velocity_x / 4);
    }

    ball_position_y += ball_velocity_y;
    if (ball_position_y < wall_top_y)
    {
      ball_position_y = wall_top_y;
      ball_velocity_y = - (ball_velocity_y - ball_velocity_y / 4);
    }
    else if (ball_position_y > wall_bottom_y)
    {
      ball_position_y = wall_bottom_y;
      ball_velocity_y = - (ball_velocity_y - ball_velocity_y / 4);
    }

    /* Draw the ball. */

    attr_set(A_NORMAL, 1 /* colour pair # */, NULL /* options */);
    col = GET_FIXED_INTEGER(ball_position_x);
    row = GET_FIXED_INTEGER(ball_position_y);
    mvaddch(row, col, 'O');

    /* Print the score. */

    snprintf(score_string, sizeof(score_string) - 1,
      "%d, (%9.5f,%9.5f), (%9.5f,%9.5f)", frame_count,
      FIXED_TO_FLOAT_FRACTION(ball_position_x),
      FIXED_TO_FLOAT_FRACTION(ball_position_y),
      FIXED_TO_FLOAT_FRACTION(ball_velocity_x),
      FIXED_TO_FLOAT_FRACTION(ball_velocity_y));
    attr_set(A_REVERSE, 0 /* colour pair # */, NULL /* options */);
    mvaddstr(0 /* row */, 10 /* col */, score_string);

    refresh;
    frame_count++;
  }

  endwin();
  printf("Done.\n");
}

