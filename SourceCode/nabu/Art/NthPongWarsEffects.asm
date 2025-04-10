; CHIPNSFX 25 Hz
; Nth Pong Wars Sound Effects
; By Alexander G. M. Smith, 20250409.
; ===
_NthEffects_a_z:
 dw _NthEffects_a-$-2
 dw _NthEffects_b-$-2
 dw _NthEffects_c-$-2
_NthEffects_a:
 db $00F5,4
 db $6F+17,$006F
 db $006F
 db $006F
 db $6F+3,$00F8,$F0,$00FE,$FF,$00FA,$55,$00FF,$F8,$006C,$6F+14,$006F
 db $6F+16,$00F8,$FF,$00FA,$CC,$00FF,$7E,$006C,$6F+1,$006F
 db $00F0
_NthEffects_b:
 db $00F5,4
 db $6F+17,$006F
 db $6F+6,$00FE,$F9,$00FD,$35,48,$6F+11,$006F
 db $6F+17,$006F
 db $006F
 db $006F
 db $00F0
_NthEffects_c:
 db $00F5,4
 db $6F+17,$006F
 db $006F
 db $6F+1,$00F8,$E0,$00FE,$E8,$00FC,$B6,60,$6F+16,$006F
 db $6F+17,$006F
 db $006F
 db $00F0
; ===
PUBLIC _NthEffects_a_z, _NthEffects_a, _NthEffects_b, _NthEffects_c

