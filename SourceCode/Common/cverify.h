/* Compile time assert in effect.  Found on
   https://stackoverflow.com/questions/807244/c-compiler-asserts-how-to-implement
   posted by Stephen C. Steel in 2009.
   Use with a trailing semicolon: COMPILER_VERIFY(sizeof(int) == 4);
*/
#ifndef _CVERIFY_H
#define _CVERIFY_H 1

// combine arguments (after expanding arguments)
#define CVGLUE(a,b) __CVGLUE(a,b)
#define __CVGLUE(a,b) a ## b

#define CVERIFY(expr, msg) typedef char CVGLUE (compiler_verify_, msg) [(expr) ? (+1) : (-1)]
#define COMPILER_VERIFY(exp) CVERIFY (exp, __LINE__)

#endif /* _CVERIFY_H */
