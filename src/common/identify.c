#include "../h/gsupport.h"

#include "../h/version.h"

extern char *g_progname;

/*
 * id_comment - output a comment C identifying the date and time and what
 *  program is producing the output.
 */
void id_comment(f)
FILE *f;
   {
   static char sbuf[26];
   static int first_time = 1;
   time_t ct;

   if (first_time) {
      time(&ct);
      strcpy(sbuf, ctime(&ct));
      first_time = 0;
      }
   fprintf(f, "<<id comment>>=\n/*\n");
   fprintf(f, " * %s", sbuf);
   fprintf(f, " * This file was produced by\n");
   fprintf(f, " *   %s: %s\n", g_progname, Version);
   fprintf(f, " */\n@\n");
   }
