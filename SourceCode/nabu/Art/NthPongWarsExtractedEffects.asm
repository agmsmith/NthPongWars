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
 db $00F5,4
 db $6F+6,$00FE,$F9,$00FD,$35,48,$6F+1,$006F
 db $00F0

_NthEffectsTileHit:
 db $00F5,4
 db $00F8,$E0,$00FE,$E8,$00FC,$B6,60,$006F
 db $00F0

_NthEffectsHarvest:
 db $00F5,4
 db $6F+3,$00F8,$F0,$00FE,$FF,$00FA,$55,$00FF,$F8,$006C,$6F+1,$006F
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
PUBLIC _NthEffectsHarvest
PUBLIC _NthEffectsBallOnBall

