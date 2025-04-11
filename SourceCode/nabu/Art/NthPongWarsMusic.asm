; CHIPNSFX 25 Hz
; Nth Pong Wars for Nabu PC, 2025.
; Background "music" by Alexander G. M. Smith.
; ===
_NthMusic_a_z:
 dw _NthMusic_a-$-2
 dw _NthMusic_b-$-2
 dw _NthMusic_c-$-2
_NthMusic_a:
 db $00F5,3
_NthMusic_a_l:
 db $6F+4,$00F8,$D0,$00FE,$FA,$00FA,$44,$00FF,$E8,48,$006C,$006C,$006C
  db $006C,$6F+30,$00FE,$FF,$00FA,$CC,$00FF,$7E,$006C,$6F+2,$006F
 db $00F2
  dw _NthMusic_a_l-$-2
_NthMusic_b:
 db $00F5,3
_NthMusic_b_l:
 db $6F+2,$00F8,$C0,$00FE,$FC,$00FC,$00,48,52,55,59,62,65,69,72,76,79,72
  db 76,69,72,65,69,62,65,59,62,55,59,52,55,48,52
 db $00FE,$FF,$00FC,$CD,52,48,55,52,59,55,62,59,65,62,69,65,72,69,76,72
  db 79,76,72,69,65,62,59,55,52,48
 db $00FC,$4E,48,52,55,59,62,65,69,72,76,79,72,76,69,72,65,69,62,65,59
  db 62,55,59,52,55,48,52
 db $00F2
  dw _NthMusic_b_l-$-2
_NthMusic_c:
 db $00F0
; ===
PUBLIC _NthMusic_a_z, _NthMusic_a, _NthMusic_b, _NthMusic_c

