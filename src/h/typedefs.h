/*
 * typedefs for the run-time system.
 */

typedef int ALIGN;		/* pick most stringent type for alignment */
typedef unsigned int DIGIT;

/*
 * Default sizing and such.
 */

/*
 * Set up typedefs and related definitions depending on whether or not
 * ints and pointers are the same size.
 */

#if IntBits != WordBits
   typedef long int word;
   typedef unsigned long int uword;
#else					/* IntBits != WordBits */
   typedef int word;
   typedef unsigned int uword;
#endif					/* IntBits != WordBits */

/*
 * Typedefs to make some things easier.
 */
typedef void *pointer;
typedef struct descrip *dptr;
typedef word C_integer;

/*
 * A success continuation is referenced by a pointer to an integer function
 *  that takes no arguments.
 */
typedef int (*continuation) (void);
