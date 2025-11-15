/******************************************************************************
 * Nth Pong Wars, debug_print.c for printing stuff even when no OS is running.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "debug_print.h"


/*******************************************************************************
 * Print a string to the debug output, which is stdout in CP/M (you can redirect
 * it to a telnet session with "STAT CON:=UC1:") or the telnet session that the
 * Nabu Server is providing, when compiled for bare mode.  Use line feed
 * characters to start new lines, no need for a carriage return.
 */
void DebugPrintString(const char *MessageText)
{
#ifdef __NABU_BARE__ /* Defined for Z88DK builds with +nabu -subtype=bare */
  /* Can only write at most 255 bytes at a time. */
  const char *pChar = MessageText;
  uint16_t charCount;
  charCount = strlen(pChar);
  while (charCount != 0)
  {
    uint8_t amountToWrite;
    if (charCount > 255)
      amountToWrite = 255;
    else
      amountToWrite = charCount;
  
    rn_TCPServerWrite(0 /* dataOffset */, amountToWrite /* dataLen */,
      (char *) pChar);
    
    pChar += amountToWrite;
    charCount -= amountToWrite;
  }
#else /* CP/M Mode, use standard output, or maybe stderr if there is one? */
  fputs(MessageText, stdout);
#endif
}


/*******************************************************************************
 * Utility function to append a readable ASCII version of the given 16 bit
 * unsigned integer, to the g_TempBuffer.  Assume it doesn't overflow.  Returns
 * a pointer to the end of the string in g_TempBuffer.  Doesn't trash memory
 * like printf("%u") does in Z88DK due to mangling the IX frame pointer.
 */
char * AppendDecimalUInt16(uint16_t Number)
{
  return fast_utoa(Number, g_TempBuffer + strlen(g_TempBuffer));
}

