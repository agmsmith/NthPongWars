/* Nth Pong Wars Music Header
 *
 * AGMS20250410 Add a header file to identify available music to C code.
 */
#ifndef _NTHPONGMUSIC_H
#define _NTHPONGMUSIC_H 1

extern char NthMusic_a_z[], NthMusic_a[], NthMusic_b[], NthMusic_c[];

extern char NthEffects_a_z[], NthEffectsSilence[], NthEffectsWallHit[];
extern char NthEffectsTileHit[], NthEffectsBallOnBall[];
/* Set NthEffectsHarvestPlayer to $10 to $40, step $10 for player variations. */
extern char NthEffectsHarvest[], NthEffectsHarvestPlayer[];

#endif /* _NTHPONGMUSIC_H */

