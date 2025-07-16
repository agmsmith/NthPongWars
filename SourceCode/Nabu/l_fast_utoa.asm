SECTION code_z80
PUBLIC _fast_utoa
PUBLIC l_fast_utoa

; Originally from Z88DK system, in file
; z88dk/libsrc/_DEVELOPMENT/l/util/9-common/ascii/num_to_txt/fast/l_fast_utoa.asm
; Extracted by AGMS20250121 because stock utoa() function trashes the caller's
; stack frame somehow (and uses ix (ix is the stack frame base when SDCC
; compiles for speed), and uses the alternate register set).

; C call: char * fast_utoa(uint16_t number, char *buffer);

_fast_utoa:
  ld    hl,5      ; Get pointer to arguments on stack, start at end.
  add   hl,sp
  ld    d,(hl)    ; Fetch the buffer pointer.
  dec   hl
  ld    e,(hl)
  dec   hl
  ld    a,(hl)    ; Fetch the 16 bit number to be printed.
  dec   hl
  ld    l,(hl)
  ld    h,a
  or   a,a       ; Clear carry, don't want leading zeroes.
  call  l_fast_utoa
  ex    de,hl   ; SDCC calling convention #0 wants 16 bit results in hl.
  ld    (hl),0  ; NUL at end of string for C compatibility.
  ret


l_fast_utoa:

   ; write unsigned decimal integer to ascii buffer
   ;
   ; enter : hl = unsigned integer
   ;         de = char *buffer
   ;         carry set to write leading zeroes
   ;
   ; exit  : de = char *buffer (one byte past last char written)
   ;         carry reset
   ;
   ; uses  : af, bc, de, hl

   ld bc,0+256
   push bc

   ld bc,-10+256
   push bc

   inc h
   dec h
   jr z, eight_bit

;  ld bc,-100+256
   ld c,0xff & (-100+256)
   push bc

   ld bc,-1000+256
   push bc

   ld bc,-10000

   ; hl = unsigned int
   ; de = char *buffer
   ; bc = first divisor
   ; carry set for leading zeroes
   ; stack = remaining divisors

   jr c, leading_zeroes


no_leading_zeroes:

   call divide
   cp '0'
   jr nz, write

   pop bc
   djnz no_leading_zeroes

   jr write1s


leading_zeroes:

   call divide

write:

   ld (de),a
   inc de

   pop bc
   djnz leading_zeroes


write1s:

   ld a,l
   add a,'0'

   ld (de),a
   inc de
   ret


divide:

   ld a,'0'-1

divloop:

   inc a
   add hl,bc
   jr c, divloop

   sbc hl,bc
   ret

eight_bit:

   ld bc,-100
   jr nc, no_leading_zeroes

   ; write two leading zeroes to output string

   ld a,'0'
   ld (de),a
   inc de
   ld (de),a
   inc de

   jr leading_zeroes
