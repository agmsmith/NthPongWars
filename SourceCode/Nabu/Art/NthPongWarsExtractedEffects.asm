; CHIPNSFX 25 Hz
; Nth Pong Wars Sound Effects
; By Alexander G. M. Smith, 20250715.
; ===
_NthEffects_a_z: ; A song that we don't really use, with only one noise effect.
 dw _NthEffectsTileHit-$-2
 dw _NthEffectsTileHitCopy-$-2
 dw _NthEffectsBallOnBall-$-2
 
_NthEffectsSilence:
 db $00F0

_NthEffectsWallHit:
 db $00F5,4
 db $6F+5,$00FE,$F8,48,$6F+1,$006F
 db $00F0

_NthEffectsTileHit:
 db $00F5,4
 db $00F8,$E0,$00FE,$E8,$00FC,$B6
_NthEffectsTileHitNote:
; Vary the pitch of the tile hit to reflect the player doing the hit.  Can vary
; from 0 (low, C-0) to $6B (high, B#8), 12 notes per octave.
 db 60
 db $00F0

; Make a copy of _NthEffectsTileHit so we can play it simultaneously twice,
; with both having different custom tones.
_NthEffectsTileHitCopy:
 db $00F5,4
 db $00F8,$E0,$00FE,$E8,$00FC,$B6
_NthEffectsTileHitCopyNote:
 db 84
 db $00F0

_NthEffectsHarvest:
 db $00F5,4
 db $00F8,$F0,$00FE,$E8,$00FA
_NthEffectsHarvestPlayer:
; Noise pitch $01=high, to $FF low.  We map player numbers to $10 to $D0
; in steps of $40 ($10 steps are hard to distinguish).
 db $10
 db $006C
 db $00F0

_NthEffectsBallOnBall:
 db $00F5,4
 db $6F+16,$00FE,$FF,$00FA,$CC,$00FF,$7E,$006C,$6F+1,$006F
 db $00F0

; ===
PUBLIC _NthEffects_a_z
PUBLIC _NthEffectsSilence
PUBLIC _NthEffectsWallHit
PUBLIC _NthEffectsTileHit
PUBLIC _NthEffectsTileHitNote
PUBLIC _NthEffectsTileHitCopy
PUBLIC _NthEffectsTileHitCopyNote
PUBLIC _NthEffectsHarvest
PUBLIC _NthEffectsHarvestPlayer
PUBLIC _NthEffectsBallOnBall

