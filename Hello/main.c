/* AGMS20240304 Hello World basic test of C compilers and runtime.
 * AGMS20240416 Add some code to see how the compiler handles 16 bit fixed
 * point fractions.
 * AGMS20250302 Add some sound experiments with the AY-3-8910 chip.
 *
 * Compile for NABU + RetroNet Cloud CP/M (generates a hard disk image and a
 * usable .COM file, --math32 for floating point) with:
 * zcc +cpm -v --list --c-code-in-asm -gen-map-file -gen-symbol-file -create-app -compiler=sdcc -O2 --opt-code-speed=all --fverbose-asm --math32 main.c z80_delay_ms.asm z80_delay_tstate.asm CHIPNSFX.asm SquareRoots.asm -o "HELLO.COM" ; cp -v *.COM ~/Documents/NABU\ Internet\ Adapter/Store/D/0/
*/

#define NUMBER_TEST 0
#define CPM_READ_TEST 0
#define NOTE_TEST 0
#define WACKA_TEST 1
#define NTH_SOUND_TEST 1
#define CHIPNSFX_SOUND_TEST 1

#pragma printf = "%f %d %ld %c %s %X %lX" /* Need these printf formats. */
#pragma output nogfxglobals /* No global variables from Z88DK for graphics. */
#pragma output noprotectmsdos /* No need for MS-DOS warning code. */

#include <stdio.h>
/* #include <fcntl.h> /* For lower level file open, read, close and FDs. */
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

extern void z80_delay_ms(uint16_t ms);
/* Busy wait exactly the number of milliseconds, which includes the
  time needed for an unconditional call and the ret, but not C glue code. */

static char Buffer[129]; /* CP/M reads in 128 byte chunks. */

#if NUMBER_TEST
signed short FixedPoint16Test(signed short NumA, signed short NumB)
{
  signed short NumC;

  NumC = NumA + NumB;
  return NumC;
}


signed long FixedPoint32Test(signed long NumA, signed long NumB)
{
  signed long NumC;

  NumC = NumA + NumB;
  return NumC;
}


float Float32Test(float NumA, float NumB)
{
  float NumC;

  NumC = NumA + NumB;
  return NumC;
}
#endif /* NUMBER_TEST */


#if CPM_READ_TEST
void ReadTest(void)
{
  int DataFile = -1;
  const char *FileName = "FORTUNES.TXT";
  ssize_t AmountRead;
  ssize_t TotalRead;

  printf ("Test for reading binary files from CP/M into memory by AGMS20241030.\n");

  /* Can't use fopen/fread since it converts line ends to CRLF etc, even
     if you specify the "rb" binary option in fopen, which messes up file
     contents and positions. */
  DataFile = open(FileName, O_RDONLY, _IOREAD | _IOUSE);
  if (DataFile < 0)
  {
    printf ("Failed to open file %s.\n", FileName);
    return;
  }

  printf ("Successfully opened %s, fd %d.\n", FileName, DataFile);
  AmountRead = 12345;
  TotalRead = 0;
  Buffer[sizeof(Buffer) - 1] = 0;
  while (AmountRead > 0)
  {
    AmountRead = read(DataFile, Buffer, 80);
    if (AmountRead > 0 && AmountRead < sizeof(Buffer))
    {
      Buffer[AmountRead] = 0;
      printf("\nRead %d bytes: %s", AmountRead, Buffer);
      TotalRead += AmountRead;
    }
    else
    {
      printf("\nRead %d bytes, ending?\n", AmountRead);
    }
    printf(".");
  }
  printf("\nRead total %d bytes.\n", TotalRead);
  close (DataFile);
  printf("File closed.\n");
}
#endif /* CPM_READ_TEST */


/*******************************************************************************
 * Sound support - Copied from NABULIB
 *
 * These are special function register definitions where you can read and write
 * values to the ports. using these compiles to one line of assembly, OUT or IN,
 * which generates efficient assembly.
 *
 * Note: For example, to send a value to PORTA
 *
 * IO_AYLATCH = IOPORTA;
 * IO_AYDATA = 0x04;
 *
 * *Note: For example to retrieve a value from PORTB
 *
 * IO_LATCH = IOPORTB;
 * return IO_AYDATA;
 */

__sfr __at 0x40 IO_AYDATA;
__sfr __at 0x41 IO_AYLATCH;

void ayWrite(uint8_t reg, uint8_t val)
{
  IO_AYLATCH = reg;
  IO_AYDATA = val;
}

uint8_t ayRead(uint8_t reg)
{
  IO_AYLATCH = reg;
  return IO_AYDATA;
}

void initNABULIBAudio(void)
{
  // Noise envelope period.
  ayWrite(6, 0b00000000);

  // Turn off all channels: volume 0 and disable envelope effect on volume.
  ayWrite(8, 0b00000000);
  ayWrite(9, 0b00000000);
  ayWrite(10, 0b00000000);

  // Enable only the Tone generators on A B C,
  // High 2 bits also set port A to output (used for interrupt enable bits),
  // port B to input (status bits like HCCA errors, interrupt request priority).
  ayWrite(7, 0b01111000);

  // Envelope period to 0.
  ayWrite(11, 0);
  ayWrite(12, 0);
  // Envelope mode flags to zero.
  ayWrite(13, 0);
}

const uint8_t _NOTES_COURSE[] = {
13, //  0, midi note: 36 (C2)
12, //  1, midi note: 37 (C#2/Db2)
11, //  2, midi note: 38 (D2)
11, //  3, midi note: 39 (D#2/Eb2)
10, //  4, midi note: 40 (E2)
10, //  5, midi note: 41 (F2)
9, //  6, midi note: 42 (F#2/Gb2)
8, //  7, midi note: 43 (G2)
8, //  8, midi note: 44 (G#2/Ab2)
7, //  9, midi note: 45 (A2)
7, //  10, midi note: 46 (A#2/Bb2)
7, //  11, midi note: 47 (B2)
6, //  12, midi note: 48 (C3)
6, //  13, midi note: 49 (C#3/Db3)
5, //  14, midi note: 50 (D3)
5, //  15, midi note: 51 (D#3/Eb3)
5, //  16, midi note: 52 (E3)
5, //  17, midi note: 53 (F3)
4, //  18, midi note: 54 (F#3/Gb3)
4, //  19, midi note: 55 (G3)
4, //  20, midi note: 56 (G#3/Ab3)
3, //  21, midi note: 57 (A3)
3, //  22, midi note: 58 (A#3/Bb3)
3, //  23, midi note: 59 (B3)
3, //  24, midi note: 60 (C4)
3, //  25, midi note: 61 (C#4/Db4)
2, //  26, midi note: 62 (D4)
2, //  27, midi note: 63 (D#4/Eb4)
2, //  28, midi note: 64 (E4)
2, //  29, midi note: 65 (F4)
2, //  30, midi note: 66 (F#4/Gb4)
2, //  31, midi note: 67 (G4)
2, //  32, midi note: 68 (G#4/Ab4)
1, //  33, midi note: 69 (A4)
1, //  34, midi note: 70 (A#4/Bb4)
1, //  35, midi note: 71 (B4)
1, //  36, midi note: 72 (C5)
1, //  37, midi note: 73 (C#5/Db5)
1, //  38, midi note: 74 (D5)
1, //  39, midi note: 75 (D#5/Eb5)
1, //  40, midi note: 76 (E5)
1, //  41, midi note: 77 (F5)
1, //  42, midi note: 78 (F#5/Gb5)
1, //  43, midi note: 79 (G5)
1, //  44, midi note: 80 (G#5/Ab5)
0, //  45, midi note: 81 (A5)
0, //  46, midi note: 82 (A#5/Bb5)
0, //  47, midi note: 83 (B5)
0, //  48, midi note: 84 (C6)
0, //  49, midi note: 85 (C#6/Db6)
0, //  50, midi note: 86 (D6)
0, //  51, midi note: 87 (D#6/Eb6)
0, //  52, midi note: 88 (E6)
0, //  53, midi note: 89 (F6)
0, //  54, midi note: 90 (F#6/Gb6)
0, //  55, midi note: 91 (G6)
0, //  56, midi note: 92 (G#6/Ab6)
0, //  57, midi note: 93 (A6)
0, //  58, midi note: 94 (A#6/Bb6)
0, //  59, midi note: 95 (B6)
0, //  60, midi note: 96 (C7)
0, //  61, midi note: 97 (C#7/Db7)
0, //  62, midi note: 98 (D7)
0, //  63, midi note: 99 (D#7/Eb7)
0, //  64, midi note: 100 (E7)
0, //  65, midi note: 101 (F7)
0, //  66, midi note: 102 (F#7/Gb7)
0, //  67, midi note: 103 (G7)
0, //  68, midi note: 104 (G#7/Ab7)
0, //  69, midi note: 105 (A7)
0, //  70, midi note: 106 (A#7/Bb7)
0, //  71, midi note: 107 (B7)
};

const uint8_t _NOTES_FINE[] = {
92, //  0, midi note: 36 (C2)
156, //  1, midi note: 37 (C#2/Db2)
231, //  2, midi note: 38 (D2)
60, //  3, midi note: 39 (D#2/Eb2)
155, //  4, midi note: 40 (E2)
2, //  5, midi note: 41 (F2)
114, //  6, midi note: 42 (F#2/Gb2)
235, //  7, midi note: 43 (G2)
106, //  8, midi note: 44 (G#2/Ab2)
242, //  9, midi note: 45 (A2)
127, //  10, midi note: 46 (A#2/Bb2)
20, //  11, midi note: 47 (B2)
174, //  12, midi note: 48 (C3)
78, //  13, midi note: 49 (C#3/Db3)
243, //  14, midi note: 50 (D3)
158, //  15, midi note: 51 (D#3/Eb3)
77, //  16, midi note: 52 (E3)
1, //  17, midi note: 53 (F3)
185, //  18, midi note: 54 (F#3/Gb3)
117, //  19, midi note: 55 (G3)
53, //  20, midi note: 56 (G#3/Ab3)
249, //  21, midi note: 57 (A3)
191, //  22, midi note: 58 (A#3/Bb3)
138, //  23, midi note: 59 (B3)
87, //  24, midi note: 60 (C4)
39, //  25, midi note: 61 (C#4/Db4)
249, //  26, midi note: 62 (D4)
207, //  27, midi note: 63 (D#4/Eb4)
166, //  28, midi note: 64 (E4)
128, //  29, midi note: 65 (F4)
92, //  30, midi note: 66 (F#4/Gb4)
58, //  31, midi note: 67 (G4)
26, //  32, midi note: 68 (G#4/Ab4)
252, //  33, midi note: 69 (A4)
223, //  34, midi note: 70 (A#4/Bb4)
197, //  35, midi note: 71 (B4)
171, //  36, midi note: 72 (C5)
147, //  37, midi note: 73 (C#5/Db5)
124, //  38, midi note: 74 (D5)
103, //  39, midi note: 75 (D#5/Eb5)
83, //  40, midi note: 76 (E5)
64, //  41, midi note: 77 (F5)
46, //  42, midi note: 78 (F#5/Gb5)
29, //  43, midi note: 79 (G5)
13, //  44, midi note: 80 (G#5/Ab5)
254, //  45, midi note: 81 (A5)
239, //  46, midi note: 82 (A#5/Bb5)
226, //  47, midi note: 83 (B5)
213, //  48, midi note: 84 (C6)
201, //  49, midi note: 85 (C#6/Db6)
190, //  50, midi note: 86 (D6)
179, //  51, midi note: 87 (D#6/Eb6)
169, //  52, midi note: 88 (E6)
160, //  53, midi note: 89 (F6)
151, //  54, midi note: 90 (F#6/Gb6)
142, //  55, midi note: 91 (G6)
134, //  56, midi note: 92 (G#6/Ab6)
127, //  57, midi note: 93 (A6)
119, //  58, midi note: 94 (A#6/Bb6)
113, //  59, midi note: 95 (B6)
106, //  60, midi note: 96 (C7)
100, //  61, midi note: 97 (C#7/Db7)
95, //  62, midi note: 98 (D7)
89, //  63, midi note: 99 (D#7/Eb7)
84, //  64, midi note: 100 (E7)
80, //  65, midi note: 101 (F7)
75, //  66, midi note: 102 (F#7/Gb7)
71, //  67, midi note: 103 (G7)
67, //  68, midi note: 104 (G#7/Ab7)
63, //  69, midi note: 105 (A7)
59, //  70, midi note: 106 (A#7/Bb7)
56, //  71, midi note: 107 (B7)
};


void playNoteDelay(uint8_t channel, uint8_t note, uint16_t delayLength)
{
  /* Set the envelope length */
  ayWrite(11, delayLength >> 8);
  ayWrite(12, delayLength & 0xff);

  switch (channel) {
    case 0:
      ayWrite(8, 0b00010000);
      ayWrite(9, 0b00000000);
      ayWrite(10, 0b00000000);
      ayWrite(0x00, _NOTES_FINE[note]);
      ayWrite(0x01, _NOTES_COURSE[note]);
      ayWrite(13, 0b00000000);
      break;

    case 1:
      ayWrite(8, 0b00000000);
      ayWrite(9, 0b00010000);
      ayWrite(10, 0b00000000);
      ayWrite(0x02, _NOTES_FINE[note]);
      ayWrite(0x03, _NOTES_COURSE[note]);
      ayWrite(13, 0b00000000);
      break;

    case 2:
      ayWrite(8, 0b00000000);
      ayWrite(9, 0b00000000);
      ayWrite(10, 0b00010000);
      ayWrite(0x04, _NOTES_FINE[note]);
      ayWrite(0x05, _NOTES_COURSE[note]);
      ayWrite(13, 0b00000000);
      break;
  }
}


#if WACKA_TEST
/* Recreating the sound code from Leo Binkowski's NABU Pacman. */

typedef struct SoundStateStruct {
  bool active; /* WACFLG or CKAFLG */
  bool alt; /* Alternates updates on every other frame. */
  uint16_t fineCoarse; /* Period of note to be played. */
  uint8_t controlLoop;
  uint8_t wait;
  uint8_t waitDelay; /* WKDEL or WKDEL1 */
  uint16_t initialFineCoarse;
  uint16_t plusNote1;
  uint16_t plusNote2;
} SoundState;

void Wacka(void)
{
  SoundState Wa, Cka, *WackaPntr;

  initNABULIBAudio();

  Wa.initialFineCoarse = 0x252;
  Wa.waitDelay = 2;
  Wa.plusNote1 = (uint16_t) (-0xC8);
  Wa.plusNote2 = 0x64;

  Cka.initialFineCoarse = 0x54;
  Cka.waitDelay = 4;
  Cka.plusNote1 = 0xAA;
  Cka.plusNote2 = (uint16_t) (-0x50);

  /* Stuff for the background ambient whooo-whooo sound, non-music music. */
  uint16_t AmbientPeriod = 0; /* Note frequency to play, using channel C. */
  uint8_t AmbientState = 255; /* Counts up and different things happen. */

  /* Run the sound loop for a few seconds, then exit. */

  uint16_t frameCounter;
  for (frameCounter = 600; frameCounter != 0; frameCounter--)
  {
    if ((frameCounter & 63) == 0) /* Once a second, start Wa sound. */
      Wa.active = true;

    if ((frameCounter & 63) == 32) /* Once a second, start Cka sound. */
      Cka.active = true;

    for (WackaPntr = &Wa; WackaPntr != NULL;
    WackaPntr = ((WackaPntr == &Wa) ? &Cka : NULL))
    {
      if (WackaPntr->active)
      {
        if (WackaPntr->wait == 0) /* Starting up/playing the sound. */
        {
          ayWrite(9, 0x0E); /* Set volume on channel B to 14/15. */
          WackaPntr->alt = !WackaPntr->alt;
          if (WackaPntr->alt)
          {
            WackaPntr->fineCoarse = WackaPntr->fineCoarse + WackaPntr->plusNote1;
#if 0 /* Leftover code which writes frequency twice. */
            ayWrite(2, WackaPntr->fineCoarse);
            ayWrite(3, WackaPntr->fineCoarse >> 8);
            z80_delay_ms(8); /* Not in Pacman code, else can't hear this! */
#endif
            WackaPntr->fineCoarse = WackaPntr->fineCoarse + WackaPntr->plusNote2;
            ayWrite(2, WackaPntr->fineCoarse);
            ayWrite(3, WackaPntr->fineCoarse >> 8);
            if (++WackaPntr->controlLoop >= 4)
            { /* Silence the sound. */
              ayWrite(3, 0);
              ayWrite(9, 0);
              WackaPntr->wait++; /* Start the waiting portion. */
            }
          }
        }
        else /* Letting silence reign, wait a while. */
        {
          WackaPntr->wait++;
          if (WackaPntr->wait >= WackaPntr->waitDelay)
          {
            /* Finshed waiting, reset for next time. */
            WackaPntr->active = false;
            WackaPntr->controlLoop = 0;
            WackaPntr->wait = 0;
            WackaPntr->fineCoarse = WackaPntr->initialFineCoarse;
            WackaPntr->alt = false;
          }
        }
      }
    }

    /* Make ambient sounds, which is a soft siren going up and down in the
    background, simpler than having background music. */

    if (AmbientState < 23)
      AmbientPeriod -= 25;
    else if (AmbientState < 46)
      AmbientPeriod += 25;
    else /* Start a new cycle. */
    {
      AmbientState = 0;
      AmbientPeriod = 0x2BC;
      ayWrite(10, 8); /* Set channel C to low volume. */
    }

    AmbientState++;
    ayWrite(4, AmbientPeriod); /* Change frequency of channel C. */
    ayWrite(5, AmbientPeriod >> 8);

    z80_delay_ms(16); /* Time delay to simulate 60hz vertical blanking. */
  }
}
#endif /* WACKA_TEST */


#if NTH_SOUND_TEST
void NthPongSounds(void)
{
  /* Stuff for the background ambient sound, non-music music. */
  uint16_t AmbientPeriod = 0; /* Note frequency to play, using channel C. */
  uint8_t AmbientState = 255; /* Counts up and different things happen at times. */

  initNABULIBAudio();

  /* Run the sound loop for a few seconds, then exit. */

  uint16_t frameCounter;
  for (frameCounter = 1000; frameCounter != 0; frameCounter--)
  {
    /* Make ambient sounds, which is a soft noise warbling around, simpler
       than having background music. */

    if (AmbientState < 16)
      AmbientPeriod -= 20;
    else if (AmbientState < 96)
      AmbientPeriod += (AmbientState & 31) - 17;
    else if (AmbientPeriod < 700) /* Get back to original frequency to loop. */
      AmbientPeriod += 7;
    else /* Start a new cycle. */
    {
      AmbientState = 0;
      AmbientPeriod = 700;
      ayWrite(10, 8); /* Set channel C to low volume. */
    }

    AmbientState++;
    ayWrite(4, AmbientPeriod); /* Change frequency of channel C. */
    ayWrite(5, AmbientPeriod >> 8);

    z80_delay_ms(16); /* Simulated 60hz vertical blank delay. */
  }
}
#endif /* NTH_SOUND_TEST */


#if CHIPNSFX_SOUND_TEST
extern void CSFX_stop(void);
extern void CSFX_song(void *SongPntr);
extern void CSFX_chan(uint8_t Channel, void *TrackPntr);
extern void CSFX_play(void);

extern char SquareRoots[]; /* The song data, stored in an assembler file. */

void ChipnSFXSounds(void)
{
  initNABULIBAudio();
  CSFX_stop();
  CSFX_song(&SquareRoots);

  /* Run the sound loop for a few seconds, then exit. */

  uint16_t frameCounter;
  for (frameCounter = 2000; frameCounter != 0; frameCounter--)
  {
    CSFX_play();
    z80_delay_ms(16); /* Simulated 60hz vertical blank delay. */
  }
}
#endif /* CHIPNSFX_SOUND_TEST */


void SoundTest(void)
{
#if NOTE_TEST
  initNABULIBAudio();

  printf("Three notes sequentially.\n");
  /* channel (0 to 2), note (0 to 71), delayLength (0 to 64K-1) */
  playNoteDelay(0, 0, 100);
  z80_delay_ms(2000);
  playNoteDelay(1, 35, 200);
  z80_delay_ms(2000);
  playNoteDelay(2, 71, 300);
  z80_delay_ms(2000);

  printf("Three notes at once.\n");
  playNoteDelay(0, 0, 100);
  playNoteDelay(1, 35, 200);
  playNoteDelay(2, 71, 300);
  z80_delay_ms(2000);

  printf("Finished with playing notes.\n");
#endif /* NOTE_TEST */

#if WACKA_TEST
  printf("Starting Pacman sounds.\n");
  Wacka();
  printf("Finished Pacman sounds.\n");
#endif

#if NTH_SOUND_TEST
  printf("Starting Nth Pong Wars sounds.\n");
  NthPongSounds();
  printf("Finished Nth Pong Wars sounds.\n");
#endif /* NTH_SOUND_TEST */

#if CHIPNSFX_SOUND_TEST
  printf("Starting CHIPNSFX sounds.\n");
  ChipnSFXSounds();
  printf("Finished CHIPNSFX sounds.\n");
#endif /* CHIPNSFX_SOUND_TEST */
}


int main(void)
{
  short unsigned int i = 12345;

  printf("Hello World %d!\n", i);

#if NUMBER_TEST
  printf ("Test for computation speed by AGMS20240304.\n");

  for (i = 0; i < 50000; i++)
  {
    FixedPoint16Test(2.1 * 64, 1.7 * 64);
  }
  printf ("Fixed 16 done.\n");

  for (i = 0; i < 50000; i++)
  {
    FixedPoint32Test(2.1 * 1024, 1.7 * 1024);
  }
  printf ("Fixed 32 done.\n");

  for (i = 0; i < 50000; i++)
  {
    Float32Test(2.1, 1.7);
  }
  printf ("Float 32 done.\n");
#endif /* NUMBER_TEST */

#if CPM_READ_TEST
  ReadTest();
#endif /* CPM_READ_TEST */

  SoundTest();

  printf("Tests complete, exiting to CP/M.\n");
  return 0;
}
