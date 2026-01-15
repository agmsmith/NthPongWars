/******************************************************************************
 * Nth Pong Wars, debug_print.h for printing stuff even when no OS is running.
 *
 * AGMS20251114 - Start this header file.
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
#ifndef _DEBUG_PRINT_H
#define _DEBUG_PRINT_H 1

/* Print a string to the debug output, which is stdout in CP/M (you can redirect
   it to a telnet session with "STAT CON:=UC1:") or the telnet session that the
   Nabu Server is providing, when compiled for bare mode.  Use line feed
   characters to start new lines, no need for a carriage return. */
extern void DebugPrintString(const char *MessageText);

/* Utility function to append a readable ASCII version of the given 16 bit
   unsigned integer, to the g_TempBuffer.  Assume it doesn't overflow.  Returns
   a pointer to the end of the string in g_TempBuffer.  Doesn't trash memory
   like printf("%u") does in Z88DK due to mangling the IX frame pointer. */
extern char * AppendDecimalUInt16(uint16_t Number);

/* Same, but prints negative numbers with a minus sign. */
extern char * AppendDecimalInt16(int16_t Number);

#endif /* _DEBUG_PRINT_H */

