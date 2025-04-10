/******************************************************************************
 * Headers for the glue C code for CHIPNSFX music player library.
 *
 * This glue was written for Nth Pong Wars in 2025.  The rest of the glue is in
 * CHIPNSFX.asm, appended to the standard player code (which was itself hacked
 * up to compile with Z88DK's z80asm, which has problems with overly complex
 * link-time expressions).
 *
 * See http://cngsoft.no-ip.org/chipnsfx.htm for CHIPNSFX, download link near
 * the bottom of that page.  Unzip it and you'll find CHIPNSFX.TXT with docs,
 * CHIPNSFX.EXE the Windows Tracker editor for it (you can also recompile it
 * for Linux), and CHIPNSFX.I80 with assembler code for the library itself.
 *
 * AGMS20250410 - Start this header file.
 *
 * This program comes with ABSOLUTELY NO WARRANTY; for more details please
 * read the GNU Lesser General Public License v3, included as LGPL.TXT.
 */
#ifndef _CHIPNSFX_H
#define _CHIPNSFX_H 1

extern void CSFX_stop(void);
/* Stop music, also used for initialising the library, so call this first. */

extern void CSFX_start(void *EffectsPntr, void *SongPntr);
/* Starts playing a given pair of songs.  The EffectsPntr one has priority,
   with the SongPntr channels only being heard when the effects are silent. */ 

extern void CSFX_chan(uint8_t Channel, void *TrackPntr);
/* Play a specific track on a given channel.  Channels 0 to 2 are for effects
   and have priority, 3 to 5 are for background music.  For consistency, use
   channel 0 or 3 for white noise effects, since there is only one white noise
   generator available in the hardware. */

extern void CSFX_play(void);
/* Call this to update the sound (volumes, frequencies and other hardware
   configuration changes) every frame or so.  We're aiming for a 30hz frame
   rate (2 frames per update), so make the music with that in mind (there's a
   setting in the editor for it so you can hear what you're editing at the
   right speed). */

#endif /* _CHIPNSFX_H */

