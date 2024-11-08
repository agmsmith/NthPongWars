/******************************************************************************
 * Nth Pong Wars, input_queue.h for generic input collection.
 *
 * Collect input from joysticks, keyboards, network and AI programs.  The
 * results are stored in an array of player input states, which the next step
 * of the game update will use.  If too many inputs happen, some will be lost.
 *
 * Also collects miscellaneous requests, such as a remote player wanting to
 * join a game.
 *
 * AGMS20241108 - Start this header file.
 */
#ifndef _INPUT_QUEUE_H
#define _INPUT_QUEUE_H 1

/* The various things you can input are limited by the joystick hardware.  You
   can move it in one of 8 directions (including diagonals) or no direction.
   The directions are clockwise, starting going to the right, which is how an
   angle would increase in the upside down screen coordinates (Y increases
   downwards), so we can look up sin and cos of the angle to get vectors. */
typedef enum input_joy_angle_enum {
  INPUT_MOVE_NONE = 0, /* Joystick not being pushed in any direction. */
  INPUT_MOVE_EAST,
  INPUT_MOVE_SOUTH_EAST,
  INPUT_MOVE_SOUTH,
  INPUT_MOVE_SOUTH_WEST,
  INPUT_MOVE_WEST,
  INPUT_MOVE_NORTH_WEST,
  INPUT_MOVE_NORTH,
  INPUT_MOVE_NORTH_EAST,
  INPUT_MOVE_MAX
} input_joy_angle;

/* Miscellaneous requests.  Will need a queue for this, and store network
   addresses of requester. */
typedef enum input_request_enum {
  INPUT_REQUEST_FULL_GAME_STATE = 0, /* Send tiles and players to requester. */
  INPUT_REQUEST_JOIN_GAME,
  INPUT_REQUEST_LEAVE_GAME,
  INPUT_REQUEST_MAX
} input_joy_angle;

/* What the player wants the ball to do.  Conveniently the whole record is zero
   if nothing is requested. */
typedef struct player_input_struct {
  input_joy_angle joystick;
  bool fire_button; /* TRUE if pressed, FALSE otherwise. */
} player_input_record;

/* Player inputs are parsed and recorded in this array. */
static player_input_record g_player_inputs[MAX_PLAYERS];

#endif /* _INPUT_QUEUE_H */

