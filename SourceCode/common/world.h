/******************************************************************************
 * Nth Pong Wars, world.h for game world properties.
 *
 * Keep track of things specific to this game world.  Like size of board,
 * presence of gravity.  Also load them.
 *
 * AGMS20241108 - Start this header file.
 */
#ifndef _WORLD_H
#define _WORLD_H 1

#include "fixed_point.h"

/* The various worlds we have set up.  Maybe load them from files later?  With
   walls and other obstacles? */
typedef enum world_presets_enum {
  WORLD_PLAIN_SMALL_SCREEN = 0, /* Half a NABU screen, makes for short games. */
  WORLD_PLAIN_NABU_SCREEN, /* Full size NABU screen, except score line. */
  WORLD_GRAVITY_NABU_SCREEN, /* NABU size screen, with gravity. */
  WORLD_MAX
} world_preset;

/* Gravity to use, positive is down, in pixels per frame update squared. */
fx g_world_gravity;

void LoadWorld(world_preset WhichWorld)
{
  switch (WhichWorld)
  {
    WORLD_PLAIN_NABU_SCREEN:
    default:
    {
      g_play_area_height_tiles = 23;
      g_play_area_width_tiles = 32;
      g_screen_height_tiles = 23;
      g_screen_width_tiles = 32;
      g_screen_top_X_tiles = 0;
      g_screen_top_Y_tiles = 1;
      FLOAT_TO_FX(0.125, g_world_gravity);
      break;
    }
  }
}

#endif /* _WORLD_H */

