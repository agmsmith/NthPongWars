; ===============================================================
; Mar 2014
; ===============================================================
;
; void z80_delay_ms(uint ms)
;
; Busy wait exactly the number of milliseconds, which includes the
; time needed for an unconditional call and the ret.
;
; AGMS20241026 Hacked up to use NABU clock speed, can't get it to
; use it when compiling for CP/M running under NABU.
; ===============================================================

SECTION code_z80

PUBLIC asm_z80_delay_ms
EXTERN asm_z80_delay_tstate

; NABU clock speed is an oddball, related to TV frequencies.
defc __CPU_CLOCK = 3579545

asm_z80_delay_ms:

   ; enter : hl = milliseconds (0 = 65536)
   ;
   ; uses  : af, bc, de, hl

   ld e,l
   ld d,h

ms_loop:

   dec de

   ld a,d
   or e
   jr z, last_ms

   ld hl,+(__CPU_CLOCK / 1000) - 43
   call asm_z80_delay_tstate

   jr ms_loop

last_ms:

   ; we will be exact

   ld hl,+(__CPU_CLOCK / 1000) - 54
   jp asm_z80_delay_tstate


; Glue code for C, copying arguments off stack and into registers, to implement
; void z80_delay_ms(uint ms);

PUBLIC _z80_delay_ms

_z80_delay_ms:

   pop af
   pop hl

   push hl
   push af

   jp asm_z80_delay_ms

