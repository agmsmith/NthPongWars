; ========================================================================

; CHIPNSFX player for Amstrad CPC, ZX Spectrum 128 and other Z80 systems
; featuring AY-3-8910, YM2149 or similar 3-channel soundwave generators.

; Originally written by Cesar Nicolas-Gonzalez / CNGSOFT in 2003 for the
; 4-kb CNGTRO#1; then rewritten since 2017-03-07 until 2017-05-14 as part
; of the CHIPNSFX tracker; many patches, bugfixes and improvements since!

; Copied from http://cngsoft.no-ip.org/chipnsfx.htm version 20241231, and
; configured for Nth Pong Wars.  AGMS20250316

; This program comes with ABSOLUTELY NO WARRANTY; for more details please
; read the GNU Lesser General Public License v3, included as LGPL.TXT.

; ========================================================================

; The player runtime requires several definitions:

; CHIPNSFX_FLAG = +1(? 1.78:1.00 MHz) +2(? 6:3 ch)
; (code length: 3 channels 831B / 6 channels 856B)
; +4 (SONG_ONLY: remove sound effects logic: -23B)
; +8 (NOISELESS: remove all noise handling: -125B)
; +16 (SLIDELESS: rm. portamento+glissando: -101B)
; +32 (LOOPLESS: remove all loop logic+data: -26B)
; +64 (TEMPOLESS: simpler timing, no tempos: -22B)
; +128 (PRECALC'D notes, faster but longer: +181B)
; +256 (PREBUILT chipnsfx_bss + player only: -20B)
; +512 (READ_ONLY player code, all-data BSS:  -1B)
; Some combinations are incompatible, for example
; PREBUILT + READ_ONLY; others are redundant, f.e.
; PREBUILT already includes SONG_ONLY.

CHIPNSFX_FLAG = 1+2 ; Build all features, for Nabu Nth Pong Wars, AGMS20250316.

; EXTERNAL writepsg: PUSH BC; reg.C=A; POP BC; RET
; EXTERNAL chipnsfx =target location or =chip_base
; EXTERNAL chipnsfx_bss =temp: DEFS CHIPNSFX_TOTAL

PUBLIC chip_stop
PUBLIC chip_song
PUBLIC chip_chan
PUBLIC chip_play

writepsg: ; A=VALUE,C=INDEX; -  Access sound hardware on Nabu PC, AGMS20250316.
  PUSH BC
  LD B,A
  LD A,C
  OUT ($41),A  ; Select register to access, out IO_AYLATCH.
  CP 7  ; Register 7 high bits are special on the Nabu, controls AY's IO ports.
  LD A,B
  JR Z,PatchReg7
  OUT ($40),A ; Write data to register, IO_AYDATA.
  POP BC
  RET
PatchReg7: ; Make sure high two bits of value going into register 7 are 01.
  AND A,0b00111111
  OR  A,0b01000000
  OUT ($40),A ; Write patched data to register 7, IO_AYDATA.
  LD A,B ; Restore unpatched register value, it is sometimes used by caller.
  POP BC
  RET


chipnsfx = chip_base ; Code is placed in memory wherever it fits.  AGMS20250316.

; ------------------------------------------------------------------------

; The data format uses the following 8-bit values:

CHIPNSFX_SIZE = $6F
; $00..$6F: NOTES C-0..B#8, SFX ($6C), BRAKE ($6D), FILLER ($6E), REST ($6F)
; $70..$EF: NOTE SIZE (1..128)
; $00F0: END!
; $00F1: RETURN
; $00F2,LL,HH: LONG CALL/JUMP (HL=target-$-2)
; $00F3,LL: SHORT CALL (L=target-$-2, 0..255)
; $00F4,LL: LOOP (1..255 #.TIMES; 0 LOOP END)
; $00F5,LL: SET TEMPO (1..256)
; $00F6,LL: SET TRANS (-128..+127)
; $00F7,LL: ADD TRANS (-128..+127)
; $00F8,LL: SET AMPL. (0:MUTE; 1..255)
; $00F9,LL: ADD AMPL. (-128..+127)
; $00FA,LL: SET NOISE (0:NONE; 1..255)
; $00FB,LL: ADD NOISE (-128..+127)
; $00FC,HL: SET ARPEGGIO: 00 DISABLE SFX; H0 ARPEGGIO 0+H; HL ARPEGGIO 0+H+L
; $00FD,HL: SET VIBRATO: 00 PORTAMENTO; 0L GLISSANDO L; HL VIBRATO 0 +L 0 -L @H
; $00FE,LL: SET AMPL. ENV. (-128..+127)
; $00FF,LL: SET NOISE ENV. (-128..+127)

CHIPNSFX_POS_L = 0 ; POS.L
CHIPNSFX_POS_H = 1 ; POS.H
CHIPNSFX_BACKL = 2 ; POS.L
CHIPNSFX_BACKH = 3 ; POS.H
	if CHIPNSFX_FLAG&32 ; LOOPLESS?
CHIPNSFX_NSIZE = 4 ; 1
	else ; !LOOPLESS
CHIPNSFX_LOOPL = 4 ; POS.L
CHIPNSFX_LOOPH = 5 ; POS.H
CHIPNSFX_NSIZE = 6 ; 1
	endif ; LOOPLESS.
CHIPNSFX_CSIZE = CHIPNSFX_NSIZE+1 ; 1
	if CHIPNSFX_FLAG&64 ; TEMPOLESS?
CHIPNSFX_NAMPL = CHIPNSFX_CSIZE+1 ; $FF
	else ; !TEMPOLESS
CHIPNSFX_NTIME = CHIPNSFX_CSIZE+1 ; 1
CHIPNSFX_CTIME = CHIPNSFX_CSIZE+2 ; 1
CHIPNSFX_NAMPL = CHIPNSFX_CSIZE+3 ; $FF
	endif ; TEMPOLESS.
CHIPNSFX_CAMPL = CHIPNSFX_NAMPL+1 ; 0
CHIPNSFX_NNOTE = CHIPNSFX_NAMPL+2 ; 0
CHIPNSFX_CNOTE = CHIPNSFX_NAMPL+3 ; 0
	if CHIPNSFX_FLAG&32 ; LOOPLESS?
CHIPNSFX_FREQL = CHIPNSFX_NAMPL+4 ; 0
	else ; !LOOPLESS
CHIPNSFX_LOOPZ = CHIPNSFX_NAMPL+4 ; 0
CHIPNSFX_FREQL = CHIPNSFX_NAMPL+5 ; 0
	endif ; LOOPLESS.
CHIPNSFX_FREQH = CHIPNSFX_FREQL+1 ; 0 (in fact WAVELENGTH rather than FREQUENCY)
CHIPNSFX_ENV_A = CHIPNSFX_FREQL+2 ; 0
	if CHIPNSFX_FLAG&16 ; SLIDELESS?
CHIPNSFX_ENT_A = CHIPNSFX_ENV_A+1 ; 0
	else ; !SLIDELESS
CHIPNSFX_ENV_Z = CHIPNSFX_ENV_A+1
CHIPNSFX_ENT_A = CHIPNSFX_ENV_A+2 ; 0
	endif ; SLIDELESS.
CHIPNSFX_ENT_L = CHIPNSFX_ENT_A+1 ; 0
CHIPNSFX_ENT_H = CHIPNSFX_ENT_A+2 ; 0

	if CHIPNSFX_FLAG&8 ; NOISELESS?
CHIPNSFX_ENV_N = CHIPNSFX_ENT_A+3 ; 0
CHIPNSFX_ENT_N = CHIPNSFX_ENT_A+4 ; SECOND-TO-LAST BYTE
CHIPNSFX_ONOTE = CHIPNSFX_ENT_A+5 ; ALWAYS LAST BYTE!
	else ; !NOISELESS
CHIPNSFX_NOISE = CHIPNSFX_ENT_A+3 ; 0
CHIPNSFX_ENV_N = CHIPNSFX_ENT_A+4 ; 0
CHIPNSFX_ENT_N = CHIPNSFX_ENT_A+5 ; SECOND-TO-LAST BYTE
CHIPNSFX_ONOTE = CHIPNSFX_ENT_A+6 ; ALWAYS LAST BYTE!
	endif ; NOISELESS.

CHIPNSFX_BYTES = CHIPNSFX_ONOTE+1
	if CHIPNSFX_FLAG&2 ; DUAL MODE?
CHIPNSFX_TOTES = CHIPNSFX_BYTES*6 ; 3x(music&sound)
	else ; !DUAL MODE
CHIPNSFX_TOTES = CHIPNSFX_BYTES*3 ; 3x(music/sound)
	endif ; DUAL MODE.

	if (CHIPNSFX_FLAG&(512+8))=512 ; READ_ONLY-NOISELESS?
chip_ld_mixer = chipnsfx_bss+CHIPNSFX_TOTES+0
chip_ld_noise = chipnsfx_bss+CHIPNSFX_TOTES+1
chip_addnoise = chipnsfx_bss+CHIPNSFX_TOTES+2
CHIPNSFX_TOTAL = CHIPNSFX_TOTES+3 ; patch 20210626: read-only mode uses BSS
	else ; !READ_ONLY-NOISELESS
CHIPNSFX_TOTAL = CHIPNSFX_TOTES+0
	endif ; READ_ONLY-NOISELESS.


chipnsfx_bss: DEFS CHIPNSFX_TOTAL ; Memory for the player to use.  AGMS20250316.

; ------------------------------------------------------------------------

chip_base: ; base reference

	if CHIPNSFX_FLAG&256 ; PREBUILT?

; precalculated tables for standalone songs
chipnsfx_bss:
		dw chip_song_a,chip_song_a
		if !(CHIPNSFX_FLAG&32) ; !LOOPLESS
			dw chip_song_a
		endif
		if CHIPNSFX_FLAG&64 ; TEMPOLESS?
			db 1,1,-1
		else
			db 1,1,1,1,-1
		endif
		ds CHIPNSFX_BYTES-CHIPNSFX_CAMPL
		if CHIPNSFX_FLAG&2 ; DUAL MODE?
			ds CHIPNSFX_BYTES
		endif ; DUAL MODE.
		dw chip_song_b,chip_song_b
		if !(CHIPNSFX_FLAG&32) ; !LOOPLESS
			dw chip_song_b
		endif
		if CHIPNSFX_FLAG&64 ; TEMPOLESS?
			db 1,1,-1
		else
			db 1,1,1,1,-1
		endif
		ds CHIPNSFX_BYTES-CHIPNSFX_CAMPL
		if CHIPNSFX_FLAG&2 ; DUAL MODE?
			ds CHIPNSFX_BYTES
		endif ; DUAL MODE.
		dw chip_song_c,chip_song_c
		if !(CHIPNSFX_FLAG&32) ; !LOOPLESS
			dw chip_song_c
		endif
		if CHIPNSFX_FLAG&64 ; TEMPOLESS?
			db 1,1,-1
		else
			db 1,1,1,1,-1
		endif
		ds CHIPNSFX_BYTES-CHIPNSFX_CAMPL
		if CHIPNSFX_FLAG&2 ; DUAL MODE?
			ds CHIPNSFX_BYTES
		endif ; DUAL MODE.
		if CHIPNSFX_FLAG&512 ; READ_ONLY?
			db 0,0,0 ; patch 20220831: read-only mixer workaround (1/2), equals CHIPNSFX_TOTES
		endif ; READ_ONLY.

	else ; !PREBUILT

; wipe all channels and stop hardware
chip_stop: ; -; AFBCDEHL!
	ld hl,chipnsfx_bss+CHIPNSFX_POS_L
	ld de,CHIPNSFX_BYTES-1
	if CHIPNSFX_FLAG&2 ; DUAL MODE?
		ld bc,$0606+7
	else ; !DUAL MODE
		ld bc,$0303+7
	endif ; DUAL MODE.
	xor a
chip_stop1:
	call writepsg
	ld (hl),a
	inc hl
	ld (hl),a
	add hl,de
	dec c
	djnz chip_stop1

	if CHIPNSFX_FLAG&8 ; NOISELESS?
		ld a,$38
		jp writepsg
	else ; !NOISELESS
		if -1;CHIPNSFX_FLAG&16 ; SLIDELESS?
			ret
		else ; !SLIDELESS
			ld c,13 ; patch 20230106: setup EXPERIMENTAL hard envelope
			ld a,$8 ; 8=\\\\.. A=\/\/.. C=////.. E=/\/\..
			jp writepsg
		endif ; SLIDELESS.
	endif ; NOISELESS.

; ------------------------------------------------------------------------

; setup all tracks at once
chip_song: ; HL=^HEADER,[CF?SFX:BGM]; HL+++,AFBCDE!
	if CHIPNSFX_FLAG&2 ; DUAL MODE?
		sbc a
	else ; !DUAL MODE
		xor a
	endif ; DUAL MODE.
	call chip_song1-chip_base+chipnsfx
	call chip_song1-chip_base+chipnsfx
chip_song1:
	if CHIPNSFX_FLAG&2 ; DUAL MODE?
		inc a
	else ; !DUAL MODE
	endif ; DUAL MODE.
	ld e,(hl)
	inc hl
	ld d,(hl)
	inc hl
	ex de,hl
	add hl,de
	ex de,hl

; setup just one track
chip_chan: ; A=CHANNEL[0-2/0-5],DE=^TRACK; AFBC!
	inc a
	push af
	push hl
	ld hl,chipnsfx_bss-CHIPNSFX_BYTES
	ld bc,CHIPNSFX_BYTES
	add hl,bc
	dec a
	jr nz,$-2
	ld b,CHIPNSFX_NSIZE/2
	ld (hl),e
	inc hl
	ld (hl),d
	inc hl
	djnz $-4
	inc a
	if CHIPNSFX_FLAG&64 ; TEMPOLESS?
		ld (hl),a
		inc hl
		ld (hl),a
		inc hl
	else ; !TEMPOLESS
		ld b,4 ; NSIZE..CTIME
		ld (hl),a
		inc hl
		djnz $-2
	endif ; TEMPOLESS.
	ld (hl),-1 ; NAMPL
	xor a
	ld b,CHIPNSFX_BYTES-CHIPNSFX_CAMPL
	inc hl
	ld (hl),a
	djnz $-2
	pop hl
	pop af
	ret

	endif ; PREBUILT.

; ------------------------------------------------------------------------

; play one frame
chip_play: ; -; AFBCDEHLIX!

	if CHIPNSFX_FLAG&8 ; NOISELESS?
	else ; !NOISELESS
		if CHIPNSFX_FLAG&512 ; READ_ONLY?
			ld a,(chip_addnoise)
			ld l,a
		else ; !READ_ONLY
			ld l,0
; Simplify expression for z88dk-z80asm.  chip_addnoise = $-1-chip_base+chipnsfx
chip_addnoise = $-1
		endif ; READ_ONLY.
		ld de,chip_ld_noise
		if CHIPNSFX_FLAG&16 ; SLIDELESS?
			ld a,(de)
			call chip_addsubl-chip_base+chipnsfx
		else ; !SLIDELESS
			if CHIPNSFX_FLAG&512 ; READ_ONLY?
			else ; !READ_ONLY
				ld a,l ; patch 20240420: already in A, as seen by Jean-Marie
			endif ; READ_ONLY.
			cp $7F ; patch 20210526: noisy crunch
			ld a,(de)
			jr z,$+6
			call chip_addsubl-chip_base+chipnsfx
			db $01 ; DUMMY "LD BC,$NNNN"
			neg ; avoid turning $FF into $00!
		endif ; SLIDELESS.
		ld (de),a
	endif ; NOISELESS.

	ld bc,256-1
	ld ix,chipnsfx_bss
	call chip_both-chip_base+chipnsfx
	call chip_both-chip_base+chipnsfx

	if CHIPNSFX_FLAG&8 ; NOISELESS?
	else ; !NOISELESS
		call chip_both-chip_base+chipnsfx

		ld c,7
		if CHIPNSFX_FLAG&512 ; READ_ONLY?
			ld a,(chip_ld_mixer)
			call writepsg
		else ; !READ_ONLY
			ld a,0 ; patch 20191109: reduce mixer clobbering
; Simplify expression for z88dk-z80asm.  chip_ld_mixer = $-1-chip_base+chipnsfx
chip_ld_mixer = $-1
			cp 0 ; cache!
chip_cp_mixer = $-1
			ld (chip_cp_mixer-chip_base+chipnsfx),a ; *!* SELF-MODIFYING CODE!!
			call nz,writepsg
		endif ; READ_ONLY.
		cp %00111000
		ret nc ; no noise, skip!

		if CHIPNSFX_FLAG&512 ; READ_ONLY?
			ld a,(chip_ld_noise)
		else ; !READ_ONLY
			ld a,0
; Simplify expression for z88dk-z80asm.  chip_ld_noise = $-1-chip_base+chipnsfx
chip_ld_noise = $-1
			cp 0 ; cache!
chip_cp_noise = $-1
			ret z
			ld (chip_cp_noise-chip_base+chipnsfx),a ; *!* SELF-MODIFYING CODE!!
		endif ; READ_ONLY.
		rrca
		rrca
		rrca
		;and $1F
		ld c,6
		jp writepsg
	endif ; NOISELESS.

chip_both: ; IX=CHANNEL BYTES,B=0,C=-1..1
	inc c
	if CHIPNSFX_FLAG&2 ; DUAL MODE?
		if CHIPNSFX_FLAG&512 ; READ_ONLY?
		else ; !READ_ONLY
			if CHIPNSFX_FLAG&8 ; NOISELESS?
			else ; !NOISELESS
				ld a,$32
				ld (chip_st_mixer-chip_base+chipnsfx),a ; *!* SELF-MODIFYING CODE!!
			endif ; NOISELESS.
		endif ; READ_ONLY.
		call chip_mono-chip_base+chipnsfx
		jr nz,chip_both_ ; SFX?
		call chip_mono-chip_base+chipnsfx
chip_send: ; update hardware
		set 3,c
		call writepsg
		res 3,c
		and a
		ret z ; mute? quit!
		push bc
		sla c
		if -1;CHIPNSFX_FLAG&(8+16) ; NOISELESS+SLIDELESS?
		else ; !NOISELESS+SLIDELESS
			cp $10 ; patch 20230106: EXPERIMENTAL hard envelope output?
			jr c,$+4
			ld c,11
		endif ; NOISELESS+SLIDELESS.
		ld a,l
		call writepsg
		inc c
		ld a,h
		jp writepsg+1 ; SKIP "PUSH BC"!
chip_both_: ; play SFX over BGM
		call chip_send-chip_base+chipnsfx
		if CHIPNSFX_FLAG&512 ; READ_ONLY?
		else ; !READ_ONLY
			if CHIPNSFX_FLAG&8 ; NOISELESS?
			else ; !NOISELESS
				ld a,$3A
				ld (chip_st_mixer-chip_base+chipnsfx),a ; *!* SELF-MODIFYING CODE!!
			endif ; NOISELESS.
		endif ; READ_ONLY.
chip_mono:
	else ; !DUAL MODE
	endif ; DUAL MODE.

	ld e,(ix+CHIPNSFX_POS_L)
	ld d,(ix+CHIPNSFX_POS_H)
	ld a,d
	or e ; empty channel?
	if CHIPNSFX_FLAG&8 ; NOISELESS?
		jr z,chip_exitz
	else ; !NOISELESS
		jr z,chip_exitzz
	endif ; NOISELESS.
	if CHIPNSFX_FLAG&64 ; TEMPOLESS?
	else ; !TEMPOLESS
		dec (ix+CHIPNSFX_CTIME)
		jr nz,chip_nextnz
		ld a,(ix+CHIPNSFX_NTIME)
		ld (ix+CHIPNSFX_CTIME),a
	endif ; TEMPOLESS.
	dec (ix+CHIPNSFX_CSIZE)
chip_nextnz:
	if CHIPNSFX_FLAG&16 ; SLIDELESS?
		jr nz,chip_next
	else ; !SLIDELESS
		jp nz,chip_next-chip_base+chipnsfx
	endif ; SLIDELESS.
chip_read: ; read next command
	ld a,(de)
	inc de
	cp $F0 ; $F0-$FF: special codes
	jr c,chip_func_
	jr z,chip_func0
	ld hl,chip_read-chip_base+chipnsfx
	push hl
	ld l,c
	ld c,a
	ld a,l
	ld hl,chip_funcs-chip_base+chipnsfx-$F1
	add hl,bc ; B already is 0!
	ld c,(hl)
	add hl,bc ; ditto
	ld c,a ; restore C (channel)
	jp (hl)
chip_func_: ; $00-$6F: note
	sub $70
	inc a ; better than SUB $7F: JR C,note: JR Z,note
	jr c,chip_note
	ld (ix+CHIPNSFX_NSIZE),a ; $70-$EF: size
	jr chip_read

chip_func0: ; $00F0: END!
	if CHIPNSFX_FLAG&8 ; NOISELESS?
	else ; !NOISELESS
		call chip_resmixer-chip_base+chipnsfx
		if -1;CHIPNSFX_FLAG&16 ; SLIDELESS?
		else ; !SLIDELESS
			call chip_reswaves-chip_base+chipnsfx ; reset EXPERIMENTAL hard envelope
		endif ; SLIDELESS.
	endif ; NOISELESS.
	xor a ; set ZF for chip_exitz
	ld (ix+CHIPNSFX_POS_L),a
	ld (ix+CHIPNSFX_POS_H),a
chip_exitzz: ; shortcut to chip_exitz
	jr chip_exitz

chip_note: ; create a note. A=0 = REST,A<0 = NOTE
	ld (ix+CHIPNSFX_POS_L),e
	ld (ix+CHIPNSFX_POS_H),d
	ld d,(ix+CHIPNSFX_NSIZE)
	ld (ix+CHIPNSFX_CSIZE),d
	jr z,chip_rest ; patch 20180518: "^^^", two-byte optimisation
	if CHIPNSFX_FLAG&16 ; SLIDELESS?
		cp 0-3
		jr z,chip_note1 ; patch 20171202: "SFX": don't apply transposition here
		jr nc,chip_next ; patch 20171204: "..." ("filler") extends current note
		add $6F-0
	else ; !SLIDELESS
		inc a
		jr z,chip_next ; "..." ("filler")
		cp 1-3
		jr z,chip_note1 ; "SFX", aka "C-9": no transposition here!
		jr nc,chip_brake ; patch 20180920: "===" ("brake") toggles ENV_A and extends note
		add $6F-1
	endif ; SLIDELESS.
	add (ix+CHIPNSFX_NNOTE)
chip_note1: ; set new note parameters
	ld e,a ; chip_note_ will need this value
	ld (ix+CHIPNSFX_CNOTE),a ; A is negative when the note is "SFX" or the transposition overflows
	ld a,(ix+CHIPNSFX_ENT_A)
	if CHIPNSFX_FLAG&16 ; SLIDELESS?
		and %00010000 ; X0:ARPEGGIO; X1: VIBRATO
		ld (ix+CHIPNSFX_ENT_A),a
	else ; !SLIDELESS
		and %00110000 ; 00:NOTHING; 10: ARPEGGIO; 01:PORTAMENTO; 11:VIBRATO
		ld (ix+CHIPNSFX_ENT_A),a
		cp %00010000 ; portamento?
		jr z,chip_note_
	endif ; SLIDELESS.
	if CHIPNSFX_FLAG&8 ; NOISELESS?
	else ; !NOISELESS
		ld a,(ix+CHIPNSFX_NOISE)
		and a
		jr z,chip_note0 ; disable noise
		ld (chip_ld_noise),a
		ld a,(ix+CHIPNSFX_ENV_N) ; patch 20170913: no clashing
		cp $80
		jr z,$+5 ; patch 20170916: special case
		ld (chip_addnoise),a
		adc b ; patch 20180324: enable noise
chip_note0: ; update mixer noise bits
		call chip_zf_mixer-chip_base+chipnsfx
	endif ; NOISELESS.

	if CHIPNSFX_FLAG&16 ; SLIDELESS?
	else ; !SLIDELESS
		ld (ix+CHIPNSFX_ENV_Z),b
	endif ; SLIDELESS.
	ld a,(ix+CHIPNSFX_NAMPL)
	and a
chip_rest: ; set amplitude (0 = REST)
	ld (ix+CHIPNSFX_CAMPL),a
	jr z,chip_exitz
	if -1;CHIPNSFX_FLAG&(8+16) ; NOISELESS+SLIDELESS?
	else ; !NOISELESS+SLIDELESS
		ld a,(ix+CHIPNSFX_ENV_A) ; set/reset EXPERIMENTAL hard envelope?
		cp $80
		call chip_zf_waves
	endif ; NOISELESS+SLIDELESS.
chip_note_: ; calculate frequency
	ld a,e ; better than LD A,(ix+CHIPNSFX_CNOTE)
	jp chip_calc-chip_base+chipnsfx
	if CHIPNSFX_FLAG&16 ; SLIDELESS?
	else ; !SLIDELESS
chip_brake: ; toggle "brake" status
		inc (ix+CHIPNSFX_ENV_Z)
	endif ; SLIDELESS.
chip_next: ; handle AMPL. and FREQ. envelopes
	ld a,(ix+CHIPNSFX_CAMPL)
	and a
chip_exitz: ; jump to chip_exit if zero!
	jp z,chip_exit-chip_base+chipnsfx

	if CHIPNSFX_FLAG&16 ; SLIDELESS?
	else ; !SLIDELESS
		bit 0,(ix+CHIPNSFX_ENV_Z) ; patch 20180920: "brake" on?
		jr nz,chip_next1
	endif ; SLIDELESS.
	ld l,(ix+CHIPNSFX_ENV_A) ; patch 20170717: shorter, more general tremolo/envelope handling
	ld h,a
	ld a,l
	sub $80-$20
	cp $20+$20
	jr nc,chip_next0 ; the range $60-$9F is special: timbres ($60-$7F) and tremolos ($80-$9F)
	add a
	add a
	add a
	;if CHIPNSFX_FLAG&8 ; NOISELESS?
	;else ; !NOISELESS
	;jr z,chip_hard ; $60 and $80 have zero depth; EXPERIMENTAL hard envelopes fall here!
	;endif ; NOISELESS.
	ld l,a
	ld a,(ix+CHIPNSFX_NAMPL)
	jr nc,chip_next_ ; TREMOLO or ENVELOPE?
	cp h
	jr nz,chip_next__
	db $1E; *"LD E,$XX" skips 1 byte
chip_next0: ; restore amplitude
	ld a,h
chip_next_: ; add/substract AMPL. envelope
	call chip_addsubl-chip_base+chipnsfx
chip_next__: ; store amplitude
	ld (ix+CHIPNSFX_CAMPL),a
chip_next1:

	if CHIPNSFX_FLAG&8 ; NOISELESS?
	else ; !NOISELESS
		ld a,(ix+CHIPNSFX_ENV_N) ; patch 20170910: special case $80
		cp $80 ; safer than "add a: call pe,..."
		call z,chip_resmixer-chip_base+chipnsfx
	endif ; NOISELESS.

	ld l,(ix+CHIPNSFX_FREQL)
	ld h,(ix+CHIPNSFX_FREQH)
	if CHIPNSFX_FLAG&16 ; SLIDELESS?
		ld a,(ix+CHIPNSFX_ENT_A)
	else ; !SLIDELESS
		bit 7,(ix+CHIPNSFX_ONOTE)
		jr nz,chip_donenz ; safety: reject notes out of bounds!
		; a slower option is "dec hl: bit 7,h: jr nz,chip_donenz: inc hl"
		ld a,(ix+CHIPNSFX_ENT_A)
		and a
chip_donez: ; jump to chip_done if zero!
		jp z,chip_done-chip_base+chipnsfx
	endif ; SLIDELESS.
	ld e,(ix+CHIPNSFX_ENT_L)
	ld d,(ix+CHIPNSFX_ENT_H)
	bit 4,a
	jr z,chip_arpeggio

	if CHIPNSFX_FLAG&16 ; SLIDELESS?
	else ; !SLIDELESS
		bit 5,a
		jr nz,chip_vibrato
chip_portamento: ; PORTAMENTO
		sbc hl,de ; 20240612: 3 bytes shorter (albeit a bit slower: 7 NOP vs 5 or 6) than
		add hl,de ; "ld a,h: cp d: jr nz,chip_portamento_: ld a,l: cp e"; found by Jean-Marie!
		jr z,chip_donez
chip_portamento_: ; update FREQ
		adc hl,de
		srl h
		rr l
		jr chip_freqr
	endif ; SLIDELESS.

chip_vibrato: ; VIBRATO
	if CHIPNSFX_FLAG&16 ; SLIDELESS?
	else ; !SLIDELESS
		dec d
		inc d
		jr z,chip_glissando
	endif ; SLIDELESS.
	inc a
	ld (ix+CHIPNSFX_ENT_A),a
	and $0F
	cp d
chip_donenz: ; jump to chip_done if nonzero!
	jp nz,chip_done-chip_base+chipnsfx ; patch 20170727: cfr. infra
	xor (ix+CHIPNSFX_ENT_A)
	add $40 ; $40(+),$80(-),$C0(-),$00(+)
	ld (ix+CHIPNSFX_ENT_A),a
chip_glissando:
	bit 3,e ; patch 20170930: bit 3 on: signed vibrato
	jr z,$+3
	cpl
	ld d,a
	ld a,e ; patch 20190221: new scale: 1-2-4-8-16-32-64
	ld e,(ix+CHIPNSFX_ENT_N) ; patch 20210626: stable depth for vibratos
	and $07 ; patch 20190608: value 0: slow glissando
	jr nz,chip_glissando_
	;ld e,h ; patch 20190726: self-adjusting depth for glissandos
	if CHIPNSFX_FLAG&16 ; SLIDELESS?
		ld (ix+CHIPNSFX_ENT_A),%00110000 ; cancel vibrato into glissando
		inc a
	else ; !SLIDELESS
		cp (ix+CHIPNSFX_ENT_H) ; patch 20210420: detect flanging
		ld a,%00110000
		jr nz,chip_flanging_
		inc a
		cp (ix+CHIPNSFX_ENT_A)
		jr z,chip_donez ; handle flanging: freq changes just once
chip_flanging_: ; cancel vibrato into glissando
		ld (ix+CHIPNSFX_ENT_A),a
		ld a,1
	endif ; SLIDELESS.
chip_glissando_:
	inc e
	if CHIPNSFX_FLAG&512 ; READ_ONLY?
		ld b,a
		ld a,d
		ld d,0
		dec b
		jr z,$+7
		ex de,hl
		add hl,hl
		djnz $-1 ; patch 20210626: slow, but without self-modifying code
		ex de,hl
	else ; !READ_ONLY
		xor $07
		ld (chip_vibrato0-1-chip_base+chipnsfx),a ; *!* SELF-MODIFYING CODE!!
		ld a,d
		ld d,b
		ex de,hl
		jr chip_vibrato0
chip_vibrato0: ; vibrato DELTA
		add hl,hl
		add hl,hl
		add hl,hl
		add hl,hl
		add hl,hl
		add hl,hl ; patch 20200918: final "add hl,hl: srl h: rr l" were useless
		ex de,hl
	endif ; READ_ONLY.
	add a
	jr c,chip_vibrato1
	sbc hl,de ; CF must be zero!
	db $16; *"LD D,$XX" skips 1 byte
chip_vibrato1: ; update FREQ
	add hl,de
chip_freqr: ; store new FREQ
	jr chip_freq

chip_arpeggio: ; ARPEGGIO
	add $40
	jr c,$-2 ; $40(H):$80(L):$C0(0)
	ld (ix+CHIPNSFX_ENT_A),a
	if CHIPNSFX_FLAG&16 ; SLIDELESS?
		add a
	else ; !SLIDELESS
		add $100-($80+%00100000)
	endif ; SLIDELESS.
	ld a,d
	jr nc,chip_arpeggio0
	ld a,b
	jr nz,chip_arpeggio0
	or e
	jr nz,chip_arpeggio0
	if CHIPNSFX_FLAG&16 ; SLIDELESS?
		ld (ix+CHIPNSFX_ENT_A),b;0 ; 0H0H0H...
	else ; !SLIDELESS
		ld (ix+CHIPNSFX_ENT_A),0+%00100000 ; 0H0H0H...
	endif ; SLIDELESS.
chip_arpeggio0: ; patch 20170727: special cases $FX and $XF
	cp 13
	jr c,chip_arpeggio1
	rrca
	ld a,24 ; patch 20171121: D=+24
	jr z,chip_arpeggio1
	ld a,-12 ; special case: F=-12
	jr c,$+3
	add a ; patch 20171118: E=-24
	if CHIPNSFX_FLAG&16 ; SLIDELESS?
		ld (ix+CHIPNSFX_ENT_A),$40 ; 0HLLLL/0HL0HL...
	else ; !SLIDELESS
		ld (ix+CHIPNSFX_ENT_A),$40+%00100000 ; 0HLLLL/0HL0HL...
	endif ; SLIDELESS.
chip_arpeggio1: ; update NOTE
	add (ix+CHIPNSFX_CNOTE)
	cp (ix+CHIPNSFX_ONOTE)
	jr z,chip_done

chip_calc: ; A=NOTE,B=0; AF!,B=0,HL=FREQ.
	ld (ix+CHIPNSFX_ONOTE),a
	ld h,b
	ld l,b
	if CHIPNSFX_FLAG&128 ; PRECALC'D?
		cp 12*9 ; "C-9"; only 9 octaves are precalculated
		jr nc,chip_calc0
		add a
		add (chip_calcs-chip_base+chipnsfx)&255
		ld l,a
		adc (chip_calcs-chip_base+chipnsfx)>>8
		sub l
		ld h,a
		ld a,(hl)
		inc hl
		ld h,(hl)
		ld l,a
	else ; !PRECALC'D
		add a
		jr c,chip_calc0
		inc b
		sub 24
		jr nc,$-3
		add (chip_calcs-chip_base+chipnsfx-256+24)&255
		ld l,a
		adc (chip_calcs-chip_base+chipnsfx-256+24)>>8
		sub l
		ld h,a
		ld a,(hl)
		inc hl
		ld h,(hl)
		srl h
		rra
		djnz $-3
		ld l,a
		jr nc,$+3 ; imprecise but quick: ROUND rather than TRUNCATE
		inc hl ; in fact, perhaps too imprecise; TODO: erase this!?
	endif ; PRECALC'D.
chip_calc0:
	if CHIPNSFX_FLAG&16 ; SLIDELESS?
	else ; !SLIDELESS
		ld (ix+CHIPNSFX_ENT_N),h
		ld a,(ix+CHIPNSFX_ENT_A)
		cp %00010000 ; portamento?
		jr nz,chip_freq
		ld (ix+CHIPNSFX_ENT_L),l
		ld (ix+CHIPNSFX_ENT_H),h
		jp chip_next-chip_base+chipnsfx
	endif ; SLIDELESS.

chip_freq: ; HL=FREQ.
	ld (ix+CHIPNSFX_FREQL),l
	ld (ix+CHIPNSFX_FREQH),h
	if CHIPNSFX_FLAG&2 ; DUAL MODE?
	else ; !DUAL MODE
		if -1;CHIPNSFX_FLAG&(8+16) ; NOISELESS+SLIDELESS?
			sla c
		else ; !NOISELESS+SLIDELESS
			push bc ; patch 20230106: EXPERIMENTAL hard envelope?
			sla c
			ld a,(ix+CHIPNSFX_ENV_A)
			cp $80
			jr nz,$+4
			ld c,11
		endif ; NOISELESS+SLIDELESS.
		ld a,l
		call writepsg
		inc c
		ld a,h
		call writepsg
		if -1;CHIPNSFX_FLAG&(8+16) ; NOISELESS+SLIDELESS?
			srl c
		else ; !NOISELESS+SLIDELESS
			pop bc ; patch 20230106: EXPERIMENTAL hard envelope!
		endif ; !NOISELESS+SLIDELESS
	endif ; DUAL MODE.
chip_done: ; [HL=FREQ.]
	if -1;CHIPNSFX_FLAG&(8+16) ; NOISELESS+SLIDELESS?
		ld a,(ix+CHIPNSFX_CAMPL)
	else ; !NOISELESS+SLIDELESS
		ld a,(ix+CHIPNSFX_ENV_A)
		cp $80 ; patch 20230106: EXPERIMENTAL hard envelope output!
		ld a,(ix+CHIPNSFX_CAMPL)
		jr nz,chip_done1
		and a
		jr z,chip_exit
		ld a,$10
		jr chip_exit
chip_done1
	endif ; NOISELESS+SLIDELESS.
	rrca
	rrca
	rrca
	rrca
	and $0F ; set ZF if empty
chip_exit: ; A=AMPL.(0..15)[,HL=FREQ.],ZF=MUTE?
	ld de,CHIPNSFX_BYTES
	add ix,de
	if CHIPNSFX_FLAG&2 ; DUAL MODE?
		ret
	else ; !DUAL MODE
		push bc
		set 3,c
		jp writepsg+1 ; SKIP "PUSH BC"!
	endif ; DUAL MODE.

chip_funcs: ; patch 20180507: AS80 workaround
	db chip_func1-chip_funcs- 0,chip_func2-chip_funcs- 1,chip_func3-chip_funcs- 2
	db chip_func4-chip_funcs- 3,chip_func5-chip_funcs- 4,chip_func6-chip_funcs- 5
	db chip_func7-chip_funcs- 6,chip_func8-chip_funcs- 7,chip_func9-chip_funcs- 8
	db chip_funca-chip_funcs- 9,chip_funcb-chip_funcs-10,chip_funcc-chip_funcs-11
	db chip_funcd-chip_funcs-12,chip_funce-chip_funcs-13,chip_funcf-chip_funcs-14

chip_func1: ; $00F1: RETURN
	ld e,(ix+CHIPNSFX_BACKL)
	ld d,(ix+CHIPNSFX_BACKH)
	ret
chip_func2: ; $00F2: LONG CALL
	ex de,hl
	ld e,(hl)
	inc hl
	ld d,(hl)
	jr chip_func3_
chip_func3: ; $00F3: SHORT CALL
	ld h,b
	ex de,hl
	ld e,(hl)
chip_func3_: ; common CALL code
	inc hl
	ld (ix+CHIPNSFX_BACKL),l
	ld (ix+CHIPNSFX_BACKH),h
	add hl,de
	ex de,hl
	ret
	if CHIPNSFX_FLAG&32 ; LOOPLESS?
	else ; !LOOPLESS
chip_func4: ; $00F4: LOOP
		ld a,(de)
		inc de
		and a
		jr z,chip_func4_ ; END OF LOOP?
		ld (ix+CHIPNSFX_LOOPZ),a
		ld (ix+CHIPNSFX_LOOPL),e
		ld (ix+CHIPNSFX_LOOPH),d
		ret
chip_func4_: ; last loop?
		dec (ix+CHIPNSFX_LOOPZ)
		ret z
		ld e,(ix+CHIPNSFX_LOOPL)
		ld d,(ix+CHIPNSFX_LOOPH)
		ret
	endif ; LOOPLESS.

	if CHIPNSFX_FLAG&64 ; TEMPOLESS?
	else ; !TEMPOLESS
chip_func5: ; $00F5: SET TEMPO
	ld a,(de)
	inc de
	ld (ix+CHIPNSFX_NTIME),a
	ld (ix+CHIPNSFX_CTIME),a
	ret
	endif ; TEMPOLESS.
chip_func7: ; $00F7: ADD TRANS
	if CHIPNSFX_FLAG&260 ; SONG_ONLY|PREBUILT?
	else ; !SONG_ONLY|PREBUILT
		ld a,(de)
		add (ix+CHIPNSFX_NNOTE)
		db $26; *"LD H,$XX" skips 1 byte
	endif ; SONG_ONLY|PREBUILT.
chip_func6: ; $00F6: SET TRANS
	ld a,(de)
	ld (ix+CHIPNSFX_NNOTE),a
	if CHIPNSFX_FLAG&64 ; TEMPOLESS?
chip_func5: ; $00F5*
	endif ; TEMPOLESS.
	if CHIPNSFX_FLAG&32 ; LOOPLESS?
chip_func4: ; $00F4*
	endif ; LOOPLESS.
	if CHIPNSFX_FLAG&8 ; NOISELESS?
chip_funcf: ; $00FF*
chip_funcb: ; $00FB*
chip_funca: ; $00FA*
	endif ; NOISELESS.
	inc de
	ret

chip_funcd: ; $00FD: SET VIBRATO
	ld l,$10
	db $3E; *"LD A,$XX" skips 1 byte
chip_funcc: ; $00FC: SET ARPEGGIO
	ld l,b
	ld a,(de)
	if CHIPNSFX_FLAG&16 ; SLIDELESS?
	else ; !SLIDELESS
		and a
		jr z,$+6; patch 20240925: skip both SET 5,L and AND $0F. Thanks to Jean-Marie again!
		set 5,l ; 00: NOTHING/10: ARPEGGIO; 01: PORTAMENTO/11: VIBRATO
	endif ; SLIDELESS.
	and $0F
	ld (ix+CHIPNSFX_ENT_L),a
	ld a,(de)
	rrca
	rrca
	rrca
	rrca
	and $0F
	ld (ix+CHIPNSFX_ENT_H),a
	ld (ix+CHIPNSFX_ENT_A),l
	inc de
	ret
chip_funce: ; $00FE: SET AMPL. ENV.
	ld a,(de)
	inc de
	ld (ix+CHIPNSFX_ENV_A),a
	ret

chip_func9: ; $00F9: ADD AMPL.
	if CHIPNSFX_FLAG&260 ; SONG_ONLY|PREBUILT?
	else ; !SONG_ONLY|PREBUILT
		ld a,(de)
		ld l,a
		ld a,(ix+CHIPNSFX_NAMPL)
		call chip_addsubl-chip_base+chipnsfx
		db $26; *"LD H,$XX" skips 1 byte
	endif ; SONG_ONLY|PREBUILT.
chip_func8: ; $00F8: SET AMPL.
	ld a,(de)
	ld (ix+CHIPNSFX_NAMPL),a
	inc de
	ret

	if CHIPNSFX_FLAG&8 ; NOISELESS?
	else ; !NOISELESS
chip_funcf: ; $00FF: SET NOISE ENV.
		ld a,(de)
		inc de
		ld (ix+CHIPNSFX_ENV_N),a ; patch 20170913: no clashing
		ret
chip_funcb: ; $00FB: ADD NOISE
		if CHIPNSFX_FLAG&260 ; SONG_ONLY|PREBUILT?
		else ; !SONG_ONLY|PREBUILT
			ld a,(de)
			ld l,a
			ld a,(ix+CHIPNSFX_NOISE)
			call chip_addsubl-chip_base+chipnsfx
			db $26; *"LD H,$XX" skips 1 byte
		endif ; SONG_ONLY|PREBUILT.
chip_funca: ; $00FA: SET NOISE
		ld a,(de)
		ld (ix+CHIPNSFX_NOISE),a
		inc de
		ret
chip_zf_mixer: ; enable or disable noise according to ZF
		if CHIPNSFX_FLAG&512 ; READ_ONLY?
			jr nz,chip_setmixer
chip_resmixer: ; patch 20220831: read-only mixer workaround (2/2), equals SET/RES n,A
			ld a,c ; patch 20240925: again, "cp 1: adc a" is better than "cp 2: sbc -2"; see above!
			cp 1
			adc a ; turn 0:1:2 into 1:2:4
			rlca ; x2
			rlca ; x4
			rlca ; x8
			ld l,a ; OR mask
			ld h,-1 ; AND mask
			jr chip_bitmixer
chip_setmixer: ; long, but not longer than two CALLs to the same function
			ld a,c
			cp 1
			adc a ; turn 0:1:2 into 1:2:4
			rlca ; x2
			rlca ; x4
			rlca ; x8
			cpl
			ld l,b ; OR mask
			ld h,a ; AND mask
chip_bitmixer: ; modify mixer bits
			ld a,(chip_ld_mixer)
			and h
			or l
		else ; !READ_ONLY
			ld a,%11110011 ; CB9F: RES 3,A
			jr nz,$+4
chip_resmixer: ; disable noise
			ld a,%11111011 ; CBDF: SET 3,A
			add c
			rlca
			rlca
			rlca
			ld (chip_bitmixer-chip_base+chipnsfx+1),a ; *!* SELF-MODIFYING CODE!!
			ld a,(chip_ld_mixer)
chip_bitmixer: ; modify mixer bits
			bit 3,a
		endif ; READ_ONLY.
chip_st_mixer: ; *!* SELF-MODIFYING CODE!!
		ld (chip_ld_mixer),a
		ret
	if -1;CHIPNSFX_FLAG&16 ; SLIDELESS?
	else ; !SLIDELESS
chip_zf_waves: ; patch 20230106: enable or disable EXPERIMENTAL hard envelope according to ZF
		if CHIPNSFX_FLAG&512 ; READ_ONLY?
			jr nz,chip_setwaves
chip_reswaves: ; read-only waves workaround, equals SET/RES n,A
			ld a,c
			cp 1 ; patch 20240925: "cp 1: adc a" is better than the old "cp 2: sbc -2". Thanks to Jean-Marie!
			adc a ; turn 0:1:2 into 1:2:4
			ld l,a ; OR mask
			ld h,-1 ; AND mask
			jr chip_bitwaves
chip_setwaves: ; long, but not longer than two CALLs to the same function
			ld a,c
			cp 1
			adc a ; turn 0:1:2 into 1:2:4
			cpl
			ld l,b ; OR mask
			ld h,a ; AND mask
chip_bitwaves: ; modify waves bits
			ld a,(chip_ld_mixer)
			and h
			or l
		else ; !READ_ONLY
			ld a,%11110000 ; CB87: RES 0,A
			jr nz,$+4
chip_reswaves: ; disable noise
			ld a,%11111000 ; CBC7: SET 0,A
			add c
			rlca
			rlca
			rlca
			ld (chip_bitwaves-chip_base+chipnsfx+1),a ; *!* SELF-MODIFYING CODE!!
			ld a,(chip_ld_mixer)
chip_bitwaves: ; modify waves bits
			bit 3,a
		endif ; READ_ONLY.
chip_st_waves:
		ld (chip_ld_mixer),a ; *!* SELF-MODIFYING CODE!!
		ret
	endif ; SLIDELESS.
	endif ; NOISELESS.

chip_addsubl: ; ADD unsigned A,signed L with overflow control!
	add l
	bit 7,l
	jr nz,chip_addsubl_
	ret nc
	sbc a ; A=$FF,CF=1
	;ret ; patch 20240224: Jean-Marie realised that this RET did NOT gain two NOPs at the expense of one byte.
chip_addsubl_:
	ret c
	sbc a ; A=$00,CF=0
	ret

; ========================================================================

chip_calcs: ; AMSTRAD CPC/ZX SPECTRUM+MSX1: 1.00MHz/1.78MHz; INT(.5+62500/(440*(2^((n-b?79:69)/12))))
	if CHIPNSFX_FLAG&128 ; PRECALC'D?
		if CHIPNSFX_FLAG&1 ; 1.78MHz?
			dw 6810,6428,6067,5727,5405,5102,4816,4545,4290,4050
		endif ; 1.78MHz.
			dw 3822,3608 ; the 1.00:1.78 gap almost equates 10 semitones!
			dw 3405,3214,3034,2863,2703,2551,2408,2273,2145,2025,1911,1804
			dw 1703,1607,1517,1432,1351,1276,1204,1136,1073,1012,956,902
			dw 851,804,758,716,676,638,602,568,536,506,478,451
			dw 426,402,379,358,338,319,301,284,268,253,239,225
			dw 213,201,190,179,169,159,150,142,134,127,119,113
			dw 106,100,95,89,84,80,75,71,67,63,60,56
			dw 53,50,47,45,42,40,38,36,34,32,30,28
			dw 27,25,24,22,21,20,19,18,17,16,15,14
		if !(CHIPNSFX_FLAG&1) ; !1.78MHz
			dw 13,13,12,11,11,10,9,9,8,8
		endif ; 1.78MHz.
	else ; !PRECALC'D
		if CHIPNSFX_FLAG&1 ; 1.78MHz?
			dw 13621,12856,12135,11454,10811,10204,9631,9091,8581,8099,7645,7215
		else ; !1.78MHz
			dw 7645,7215,6810,6428,6067,5727,5405,5102,4816,4545,4290,4050
		endif ; 1.78MHz.
	endif ; PRECALC'D.

chip_last:

; ========================================================================

; Glue code for C, copying arguments off stack and into registers.

; void CSFX_stop(void);
PUBLIC _CSFX_stop
_CSFX_stop:
  jp    chip_stop ; -; AFBCDEHL!


; void CSFX_start(void *SongPntr, bool IsEffects);
PUBLIC _CSFX_start
_CSFX_start:
  ld    hl,2  ; Skip over return address.
  add   hl,sp
  ld    e,(hl) ; Get SongPntr argument.
  inc   hl
  ld    d,(hl)
  inc   hl
  xor   a,a    ; Zero A register.
  sub   a,(hl) ; Carry set if IsEffects is TRUE, clear otherwise.
  ex    de,hl
  jp    chip_song ; HL=^HEADER,[CF?SFX:BGM]; HL+++,AFBCDE!


; void CSFX_chan(uint8_t Channel, void *TrackPntr);
PUBLIC _CSFX_chan
_CSFX_chan:
  ld    hl,2    ; Skip over return address.
  add   hl,sp
  ld    a,(hl)  ; Get Channel.
  inc   hl
  ld    e,(hl)  ; Get TrackPntr.
  inc   hl
  ld    d,(hl)
  jp    chip_chan ; A=CHANNEL[0-2/0-5],DE=^TRACK; AFBC!


; void CSFX_play(void);
PUBLIC _CSFX_play
_CSFX_play:
  push  ix ; Save stack frame pointer from C world.
  call  chip_play ; -; AFBCDEHLIX!
  pop   ix
  ret


; bool CSFX_busy(uint8_t Channel);
PUBLIC _CSFX_busy
_CSFX_busy:
  ld    hl,2    ; Skip over return address.
  add   hl,sp
  ld    a,(hl)  ; Get Channel.  Want to find address of channel's data area.
  ld    hl,chipnsfx_bss+CHIPNSFX_POS_L ; Base address of data in channel 0.
  or    a,a     ; Can skip address multiplication for channel 0.
  jr    z,BusyMultiplyDone
  ld    bc,CHIPNSFX_BYTES
BusyChannelMultiply:
  add   hl,bc
  dec   a
  jr    nz,BusyChannelMultiply
BusyMultiplyDone:
  ld    a,(hl) ; CHIPNSFX_POS_L
  inc   hl
  or    a,(hl) ; CHIPNSFX_POS_H
  ld    l, a  ; 8 bit results are in L register.
  ret

