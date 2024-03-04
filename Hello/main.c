/* Hello World basic test of C compilers and runtime.  AGMS20240304
 *
 * Compile for CP/M with:
 * zcc +cpm --list -m -create-app -compiler=sdcc -O3 --opt-code-speed main.c -o "hello"
*/

#include <stdio.h>

int main(int argc, char **argv)
{
  printf ("Hello world!\n");
  printf ("Test for basic compatibility by AGMS20240304.\n");
  return 0;
}
