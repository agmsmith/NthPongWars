/* Nth Pong Wars Music Header
 *
 * AGMS20250410 Add a header file to identify available music to C code.
 */
#ifndef _NTHPONGMUSIC_H
#define _NTHPONGMUSIC_H 1

extern uint8_t NthMusic_a_z[], NthMusic_a[], NthMusic_b[], NthMusic_c[];

extern uint8_t NthEffects_a_z[], NthEffectsSilence[], NthEffectsWallHit[];

/* Set NthEffectsTileHitNote to $00 to $6b, for pitch variations. */
extern uint8_t NthEffectsTileHit[], NthEffectsTileHitCopy[];
extern uint8_t NthEffectsTileHitNote[], NthEffectsTileHitCopyNote[];

/* Set NthEffectsHarvestPlayer to $10 to $40, step $10 for player variations. */
extern uint8_t NthEffectsHarvest[], NthEffectsHarvestPlayer[];

extern uint8_t NthEffectsBallOnBall[];

#endif /* _NTHPONGMUSIC_H */

