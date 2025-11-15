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

extern void DebugPrintString(const char *MessageText);

extern char * AppendDecimalUInt16(uint16_t Number);

#endif /* _DEBUG_PRINT_H */

