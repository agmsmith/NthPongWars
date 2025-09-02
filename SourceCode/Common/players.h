/******************************************************************************
 * Nth Pong Wars, players.h for player data structures.
 *
 * The players are represented as ball shaped objects (easier collision
 * detection) and on the NABU are rendered as hardware sprites.  The balls are
 * coloured to reflect the player choice (maybe with a drop-shadow), and
 * animated if a power-up is applied to the object (if several power-ups are
 * active, the animations sequence through the individual power up animations).
 *
 * As they collide with squares on the board which aren't their own colour, they
 * change ownership of the square to the player and then bounce off the square.
 * They have position (in pixels), velocity (pixels per frame) and acceleration
 * (player thrust and world gravity) and bounce off things like walls and other
 * balls.  The collisions are handled with some fancy algorithms so they can
 * move large distances and yet collide with things in the way, and also move
 * slowly at fractional pixels per frame.
 *
 * AGMS20241024 - Start this header file.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef _PLAYERS_H
#define _PLAYERS_H 1

#include "fixed_point.h"

/* The maximum number of players allowed.  On the NABU, it's limited to 4 due to
   colour palette limits and the maximum number of visible sprites (4 visible,
   more per scan line don't get drawn, and we use sprites for balls).  For bit
   masking reasons, should be a power of two. */
#define MAX_PLAYERS 4

/* For masking off a counter to have values from 0 to MAX_PLAYERS-1. */
#define MAX_PLAYERS_MASK 3

/* Variable sized balls are problematic.  Having in-between sizes doesn't add
   much to the game, just a few ones are great for sweeping out large areas.
   But even then, we'd double the animation space needed (we have only 2K of
   VRAM for all frames), so we're sticking with fixed size players.  For nice
   detail on the NABU, we use 16x16 sprites (all sprites have to be the same)
   and just draw an 8x8 ball in the center of the sprite and animate power-up
   effects around the ball.  The player is considered to be spherical (with this
   diameter), centered in the hardware sprite box. */
#define PLAYER_PIXEL_DIAMETER_NORMAL 8

#ifdef NABU_H
/* Use this Y sprite position to mark a sprite as not drawable.  It's the same
   as the 0xD0 magic value the hardware uses to mark the end of the sprite list,
   so it's not a value you would normally see as a VDP Y coordinate. */
#define SPRITE_NOT_DRAWABLE 208

/* Predefined colour choices for the players, used on the NABU. */

typedef struct colour_triplet_struct {
  uint8_t main;
  uint8_t shadow;
  uint8_t sparkle;
} colour_triplet_record;

extern const colour_triplet_record k_PLAYER_COLOURS[MAX_PLAYERS];
#endif /* NABU_H */


/* The various brains that can run a player.
*/
typedef enum brain_enum {
  BRAIN_INACTIVE = 0, /* This player is not active, don't display it. */
  BRAIN_KEYBOARD, /* Player controlled by a Human using the local keyboard. */
  BRAIN_JOYSTICK, /* Player controlled by a Human using joystick #n. */
  BRAIN_NETWORK, /* Player controlled by a remote entity over the network. */
  BRAIN_ALGORITHM, /* Player controlled by a combination of algorithms. */
  BRAIN_MAX
};
typedef uint8_t player_brain; /* Want it to be 8 bits, not 16. */


/* When algorithms control the player, we select which algos we want for
   different functions and can set parameters for them.  Some of these options
   override other ones, if they conflict then the earlier one in this structure
   controls the feature.

   Note that AI frames happen at a quarter of the screen update frame rate,
   so about 5hz, since we only update one AI each frame to save on CPU. */
typedef struct player_algo_struct {
  bool steer : 1; /* TRUE to turn towards target, false to drift. */
  int16_t target_pixel_x; /* Location in the game world we head towards. */
  int16_t target_pixel_y;
  int16_t target_distance; /* Updated with current distance to target. */
  uint8_t target_player; /* Index of player to target, MAX_PLAYERS for none. */
  uint8_t target_list_index; /* Where we are in the global list of targets. */
  uint8_t desired_speed; /* Harvest if speed is less than this pixels/frame. */
  uint8_t trail_remaining; /* Num AI frames remaining in harvest or trail. */
  uint8_t delay_remaining; /* Num AI frames remaining in time delay opcode. */
} player_algo_record, *player_algo_pointer;

/* When a target_list_item with a Y coordinate containing these magic values is
   encountered, it changes the algorithm settings.  The X coordinate is often
   a parameter for the magic.  So you can have AI players follow a path, then
   return to the beginning of the path and follow it again, or hunt a player. */
typedef enum target_list_codes_enum {
  TARGET_CODE_NONE = 240, /* No operation and lowest number opcode. */
  TARGET_CODE_SPEED, /* X coordinate sets desired speed, in pixels/frame.
    Use 255 to run in harvest mode, not leaving a trail behind. */
  TARGET_CODE_STEER, /* X coordinate is 0 to 3 to steer towards a corner.  Zero
    for your corner of the board, 1 for next player's corner, 2 for next next
    player and 3 for the next next next player.  4 to target a rival player
    with the highest score, 5 to target the human with the highest score,
    other values make you drift rather than steer. */
  TARGET_CODE_DELAY, /* X sets the time delay to wait for in AI frames before
    executing the next opcode. */
  TARGET_CODE_GOTO, /* Switch to the item at list index of X coordinate. */
  TARGET_CODE_SKIP_GT, /* Skip the next list item if the distance to the target
    is greater than the X coordinate. */
  TARGET_CODE_SKIP_LT, /* Skip the next list item if the distance to the target
    is less than the X coordinate. */
  TARGET_CODE_SKIP_EQ, /* Skip the next list item if the distance to the target
    is equal to the X coordinate. */
  TARGET_CODE_MAX
};
typedef uint8_t target_code; /* Want it to be 8 bits, not 16. */

/* There is a global list of these targets.  An AI player can be processing one
   of these items at a time.  For plain coordinates, it heads towards that
   location in the game world and once it gets near enough, it advances to the
   next list item.  Some items are instruction codes, for a bit of
   programability.  Theoretically each level you load would have a different
   global target list and pointers to where each particular AI player has
   their code start. */
typedef struct target_list_item_struct {
  uint8_t target_pixel_x; /* 0 to 255, game world column coordinates. */
  uint8_t target_pixel_y; /* 0 to 239 for row number, or a special opcode. */
} target_list_item_record, *target_list_item_pointer;

extern target_list_item_record g_target_list[];

extern uint8_t g_target_start_indices[MAX_PLAYERS];


/* Joystick directions as bits.  0 bit means not pressed.  Whole byte is zero if
   no buttons are pressed.  Same as NABU's JOYSTICKENUM, and our own definition
   for other computer systems.  If you see some buttons being pressed, it means
   a user wants to take over a player.
*/
#ifdef NABU_H
  #define joystick_enum JOYSTICKENUM
#else
typedef enum joystick_enum {
    Joy_Left = 0b00000001,
    Joy_Down = 0b00000010,
    Joy_Right = 0b00000100,
    Joy_Up = 0b00001000,
    Joy_Button = 0b00010000,
  };
#endif
#define JOYSTICK_DIRECTION_MASK 15 /* To get just the direction bits. */


extern uint8_t g_KeyboardFakeJoystickStatus;
/* Your device specific keyboard code puts an emulated joystick state in this
   global, usually reflecting the cursor keys and some other button for fire */

#ifdef NABU_H
  /* NABU-LIB already has a global with joystick status, just use it. */
  #define g_JoystickStatus _joyStatus
#else /* Need to collect joystick data in our own global array instead. */
  extern uint8_t g_JoystickStatus[4];
#endif


/* This is the main player status record.  Where they are, what they are doing.
   Inactive players also get a record, in case they join a game in progress.
*/
typedef struct player_struct {
  uint8_t player_array_index;
  /* The index of this player in the g_player_array.  Player number in other
     words, used for things like deciding the sound to make for the player.
     Avoids having code to calculate the index from a player pointer. */

  fx pixel_center_x;
  fx pixel_center_y;
  /* Position of the center of the player in the play area, in pixels.  Used
     during collision detection to avoid recalculating it.  Need 16 bits since
     the play area can be larger than the NABU screen, so can go over 256 in
     pixel coordinates.  Signed, because players can go off screen, though
     tiles can't, but we do math with both.  The sprite's top left corner has
     to be calculated from the center. */
  uint8_t pixel_flying_height;
    /* How far above the board the player is.  Will draw the shadow sprite
       offset diagonally by this many pixels. 2 is a good value, 1 is hard to
       see, 0 is hidden by the ball (ball on ground?).  Can be larger to make
       it look like you are flying, and perhaps not colliding with tiles.*/

  fx velocity_x;
  fx velocity_y;
  /* The velocity vector the player is moving by, in pixels per update. */

  uint8_t speed;
  /* Manhattan speed of the player in pixels per frame, calculated in
     Simulate() as part of the velocity updates, so it will lag a frame.  It's
     abs(velocity_x) + abs(velocity_y). */

  uint8_t velocity_octant;
  /* Direction the velocity is going in, calculated from velocity x,y but only
     when needed (if joystick direction used or harvest succssful), by
     UpdatePlayerInputs().  Actually this is the lower boundary of the octant,
     the upper one is 45 degrees clockwise from this. */

  bool velocity_octant_right_on;
  /* Set to TRUE if the velocity is right along the velocity_octant.  Needed so
     that we can aim for the next more counter-clockwise octant.  The octant
     finding code carefully uses the lower bound if the velocity is exactly on
     an octant direction. */ 

  fx step_velocity_x;
  fx step_velocity_y;
  /* The speed the player is moving, in pixels per step, with several steps per
    frame update to ensure that each step doesn't advance more than one tile
    width so that we don't skip over tiles and miss collisions.  This is a
    temporary value used by Simulate(). */

  uint8_t player_collision_count;
  /* Used in the simulation code to mark players which have collided with
     another player.  To avoid immediate bouncing around against nearby tiles
     and hitting each other again, they stop laying down new tiles and just
     consume any tiles while in collision mode.  After a number of frames
     (each collision adds some amount to this counter; it gets decremented at
     the start of the next frame), this gets back to zero and they can lay down
     tiles again. */

  bool thrust_active;
  /* Set to TRUE when thrust is busy harvesting tiles.  Means the fire button
     is pressed and a direction is specified.  Saves having to read the
     buttons repeatedly during simulation. */

  uint8_t thrust_harvested;
  /* How much extra thrust (pixels per update) you get from using Thrust (fire
     button plus a direction).  Simulation sets it by counting how many of
     your own tiles you pass over.  Gets added to velocity after the
     simulation step.  Also used for OWNER_PUP_FAST for a speed burst. */

  uint8_t power_up_timers[OWNER_MAX];
  /* Counts down the number of frames for the power up to be active.  Zero
     means the power-up is inactive. */

  uint8_t joystick_inputs;
  /* What action is the player currently requesting?  See joystick_enum for
     the various bits.  Zero if no buttons pressed.  Human, AI and remote
     players all talk to the simulation code through these five bits. */

  player_brain brain;
  /* What controls this player, or marks it as inactive (not drawn). */

  union brain_info_union {
    uint8_t iJoystick; /* For BRAIN_JOYSTICK, which joystick number? */
    player_algo_record algo; /* For BRAIN_ALGORITHM - various settings. */
  } brain_info;
  /* Extra information about this brain.  Joystick number for joysticks,
     network identification for remote players, algorithm stuff for AI. */

  uint16_t last_brain_activity_time;
  /* Frame counter when the last brain activity was seen (joystick moved off
     neutral), so we can detect idle players.  After about 30 seconds, we
     change that player slot to idle. */

  /* uint16_t score; Use player colour tile in g_TileOwnerCounts, in tiles.h */
  /* Current score of each player.  Counts the number of tiles they own.  Once
     a player has reached the goal score, the game is over.  Gave up on doing
     it as percentages, too CPU expensive.  Count incremental updates are done
     in SetTileOwner(). */

  uint16_t score_displayed;
  /* The score value currently being shown on the display and in score_text.
     Don't need to update the display if it's the same as score. */

  char score_text[8];
  /* Text version of the score, corresponds to score_displayed.  May have a
     following % sign, and at most 5 digits, and a NUL.  On the NABU it can use
     different letters in the font for coloured digits for each player. */

  uint8_t main_colour;
  /* Predefined colour for this player's main graphic.  On the NABU this is a
     number from 1 to 15 in the standard TMS9918A palette, see the
     k_PLAYER_COLOURS[iPlayer].main value.  In Unix, using ncurses, this is a
     number from 1 to 4, which we'll calculate from the player index. */

#ifdef NABU_H
  uint8_t vdpSpriteX; /* 0 left, to 255 almost off right side. */
  uint8_t vdpSpriteY;
  /* $E1 past top, $FF=-1 for fully visible at top of screen, 191 off bottom,
     207 maximum past bottom, 208=$D0 to end sprite list.  We also use that
     magic value to flag sprites that are off screen and shouldn't be drawn. */
  uint8_t vdpEarlyClock32Left; /* 0, or $80 for shift left 32 pixels. */
  /* Ball location converted to sprite coordinates.  The main ball sprite and
     the sparkle sprite are at these locations.  The shadow sprite is offset
     a bit to show depth. */
  uint8_t vdpShadowSpriteX;
  uint8_t vdpShadowSpriteY;
  uint8_t vdpShadowEarlyClock32Left;

  SpriteAnimRecord main_anim;/* Animation for the main ball sprite. */

  uint8_t shadow_colour;
  /* Predefined colour for this player's shadow sprite.  We're using colours
     which have a darker version of the same colour, so that can be a
     drop-shadow.  Black also looks nice, but is invisible against the black
     background when flying.  Caches the values from k_PLAYER_COLOURS[player#].
     The shadow sprite will be showing the main animation, just offset a bit. */

  uint8_t sparkle_colour;
  /* Predefined colour for this player's sparkle sprite.  It's usually used for
     power-up animations and other such things around the ball.  Caches the
     values from k_PLAYER_COLOURS[player#]. */

  SpriteAnimRecord sparkle_anim;
#endif /* NABU_H */
} player_record, *player_pointer;

/* An array of players.  To avoid using alloc/free with resulting fragmentation
   of the minuscule NABU memory, it is a global array sized in advance to be
   as large as possible, and we just flag inactive players (can be
   non-sequential as joysticks are picked up or go idle). */
extern player_record g_player_array[MAX_PLAYERS];

/* Calculated by InitialisePlayers(), these are the adjusted game coordinates
   pixel boundaries of the play area.  Adjusted by the ball radius to make
   collision tests easier.  If the ball center position is beyond this value,
   it has hit the wall. */
extern int16_t g_play_area_wall_bottom_y;
extern int16_t g_play_area_wall_left_x;
extern int16_t g_play_area_wall_right_x;
extern int16_t g_play_area_wall_top_y;

/* When player speed is greater than this in pixels/frame, friction is
   applied.  Needs to be under 8 pixels per frame, which is when an extra
   physics step gets added and that slows everything down. */
#define FRICTION_SPEED 2

/* Amount to add to the velocity of one of the players to separate them if
   needed.  g_SeparationVelocityFxAdd set once in in InitialisePlayers(),
   g_SeparationVelocityFxStepAdd updated in Simulate() when the step size
   changes. */
extern fx g_SeparationVelocityFxAdd;
extern fx g_SeparationVelocityFxStepAdd;

extern void InitialisePlayers(void);
/* Set up the initial player data, mostly colours and animations. */

extern void UpdatePlayerAnimations(void);
/* Update animations for the next frame.  Also convert location of the player
   to hardware pixel coordinates. */

extern void UpdatePlayerInputs(void);
/* Process the joystick and keyboard inputs.  Input activity assigns a player
   to the joystick or keyboard.  Also updates brains to generate fake joystick
   inputs if needed.  Then use the joystick inputs to modify the player's
   velocities (steering if just specifying a direction, thrusting if the fire
   button is pressed too). */

#ifdef NABU_H
extern void CopyPlayersToSprites(void);
/* Copy all the players to hardware sprites.  Inactive players don't use any
   sprites.  Also inactive sparkle animations don't use a sprite.  They are
   copied in priority order, all balls, then sparkles, then shadows.  Should
   be the first thing updated as the vertical blanking starts, since sprites
   updated during display look bad. */
#endif /* NABU_H */

#endif /* _PLAYERS_H */

