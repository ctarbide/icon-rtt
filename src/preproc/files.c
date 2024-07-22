/*
 * This file contains routines for setting up characters sources from
 *  files. It contains code to handle the search for include files.
 */
#include "../preproc/preproc.h"
#include "../preproc/pproto.h"

int g_nostdinc_flg = 0;

static struct str_buf sbuf_files[1];
#define sbuf sbuf_files

#define IsRelPath(fname) (fname[0] != '/')

static void file_src (char *fname, FILE *f, struct search_dir *orig);

/*
 * Standard locations to search for header files.
 */
static struct search_dir *search_dirs;

/*
 * file_src - set up the structures for a characters source from a file,
 *  putting the source on the top of the stack.
 */
static void file_src(fname, f, orig)
char *fname;
FILE *f;
struct search_dir *orig;
   {
   union src_ref ref;

   /* fprintf(stderr, "TRACE: %s:%d, file_src(fname=\"%s\", f, orig=%p)\n", __FILE__, __LINE__, fname, (void*)orig); */

   ref.cs = new_cs(fname, f, CBufSize);
   push_src(CharSrc, &ref, orig);
   g_next_char = NULL;
   fill_cbuf();
   }

/*
 * source - Open the file named fname or use stdin if fname is "-". fname
 *  is the first file from which to read input (that is, the outermost file).
 */
void source(fname)
char *fname;
   {
   FILE *f;

   assert(fname);

   /* fprintf(stderr, "TRACE: %s:%d, source(fname=\"%s\")\n", __FILE__, __LINE__, fname); */

   if (strcmp(fname, "-") == 0)
      file_src("<stdin>", stdin, NULL /* orig */);
   else {
      if ((f = fopen(fname, "r")) == NULL)
	 err2("cannot open ", fname);
      file_src(fname, f, NULL /* orig */);
      }
   }

/*
 * include - open the file named fname and make it the current input file.
 */
void include(trigger, fname, system, start)
struct token *trigger;
char *fname;
int system, start;
   {
   char *s;
   char *path = NULL;
   char *end_prfx;
   struct src *sp;
   struct char_src *cs;
   struct search_dir *prefix;
   FILE *f;

   /* fprintf(stderr, "TRACE: %s:%d, include(trigger, fname=\"%s\", system=%d, start=%d)\n",
      __FILE__, __LINE__, fname, system, start); */

   init_sbuf(sbuf);
   /*
    * See if this is an absolute path name.
    */
   if (IsRelPath(fname)) {
      f = NULL;
      if (!system) {
	 /*
	  * This is not a system include file, so search the locations
	  *  of the "ancestor files".
	  */
	 sp = g_src_stack;
	 while (f == NULL && sp) {
	    if (sp->kind == CharSrc) {
	       cs = sp->u.cs;
	       if (cs->f) {
		  /*
		   * This character source is a file.
		   */
		  end_prfx = NULL;
		  for (s = cs->fname; *s != '\0'; ++s)
		     if (*s == '/')
			end_prfx = s;
		  if (end_prfx)
		     for (s = cs->fname; s <= end_prfx; ++s)
			AppChar(sbuf, *s);
		  for (s = fname; *s != '\0'; ++s)
		     AppChar(sbuf, *s);
		  path = str_install(sbuf);
		  if ((f = fopen(path, "r")))
		     break;
		  }
	       }
	    sp = sp->next;
	    }
	 if (sp)
	    prefix = sp->orig;
	 }
      if (f == NULL) {
	 /*
	  * Search in the locations for the system include files.
	  */
	 prefix = search_dirs + start;
	 while (prefix->dir) {
	    for (s = prefix->dir; *s != '\0'; ++s)
	       AppChar(sbuf, *s);
	    if (s > prefix->dir && s[-1] != '/')
	       AppChar(sbuf, '/');
	    for (s = fname; *s != '\0'; ++s)
	       AppChar(sbuf, *s);
	    path = str_install(sbuf);
	    if ((f = fopen(path, "r")))
	       break;
	    ++prefix;
	    }
	 }
      }
   else {                               /* The path is absolute. */
      path = fname;
      f = fopen(path, "r");
      prefix = NULL;
      fprintf(stderr, "TODO: %s:%d, inherith appropriate 'prefix'? absolute path case\n",
	 __FILE__, __LINE__);
      }
   if (f == NULL)
      errt2(trigger, "cannot open include file ", fname);
   file_src(path, f, prefix /* orig */);
   }

/*
 * init_files - Initialize this module, setting up the search path for
 *  system header files.
 */
void init_files(opt_lst, opt_args)
char *opt_lst;
char **opt_args;
   {
   int n_paths = 0;
   int i, j;
   char *s, *s1;

   /*
    *  Determine the number of standard locations to search for
    *  header files and provide any declarations needed for the code
    *  that establishes these search locations.
    */

   static char *sysdir = "/usr/include";

   /*
    * Count the number of -I options to the preprocessor.
    */
   for (i = 0; opt_lst[i] != '\0'; ++i)
      if (opt_lst[i] == 'I')
	 ++n_paths;

   if (g_nostdinc_flg == 0)
      ++n_paths;

   /*
    * Set up the array of standard locations to search for header files.
    */
   search_dirs = alloc((n_paths + 1) * sizeof(*search_dirs));

   /*
    * Get the locations from the -I options to the preprocessor.
    */
   for (i = j = 0; opt_lst[i] != '\0'; ++i)
      if (opt_lst[i] == 'I') {
	 s = opt_args[i];
	 s1 = alloc(strlen(s) + 1);
	 strcpy(s1, s);
	 search_dirs[j].pos = j;
	 search_dirs[j].dir = s1;
	 j++;
	 }

   /*
    *  Establish the standard locations to search after the -I options
    *  on the preprocessor.
    */
   if (g_nostdinc_flg == 0) {
      search_dirs[j].pos = j;
      search_dirs[j].dir = sysdir;
      }
   }
