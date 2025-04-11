; CHIPNSFX 25 Hz
; Nth Pong Wars Sound Effects
; By Alexander G. M. Smith, 20250409.
; ===
_NthEffects_a_z:
 dw _NthEffectsSilence-$-2
 dw _NthEffectsWallHit-$-2
 dw _NthEffectsTileHit-$-2
 dw _NthEffectsHarvest-$-2
 dw _NthEffectsBallOnBall-$-2
 
_NthEffectsSilence:
 db $00F0

_NthEffectsWallHit:
  db $00F5,3
 db $6F+4,$00FE,$F0,$00FC,$17,72,$6F+13,$006F
 db $00F0

_NthEffectsTileHit:
 db $00F5,3
 db $6F+2,$00F8,$E0,$00FE,$E0,$00FC,$93,76,$6F+15,$006F
 db $00F0

_NthEffectsHarvest:
 db $00F5,3
 db $6F+3,$00F8,$F0,$00FE,$FF,$00FA,$55,$00FF,$F8,$006C,$6F+14,$006F
 db $00F0

_NthEffectsBallOnBall:
 db $00F5,3
 db $6F+16,$00FE,$FF,$00FA,$CC,$00FF,$7E,$006C,$6F+1,$006F
 db $00F0

; ===
PUBLIC _NthEffects_a_z
PUBLIC _NthEffectsSilence
PUBLIC _NthEffectsWallHit
PUBLIC _NthEffectsTileHit
PUBLIC _NthEffectsHarvest
PUBLIC _NthEffectsBallOnBall

