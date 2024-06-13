
#include "rtt.h"

char *g_str_FILE;
char *g_str_IGNORE;

/*
 * prototypes for static functions.
 */
static void add_tdef (char *name);
int               yyparse   (void);

/*
 * refpath is used to locate the standard include files for the Icon
 *  run-time system. If patchpath has been patched in the binary of rtt,
 *  the string that was patched in is used for refpath.
 */
char *refpath;
char patchpath[18 + MaxPath] = "%PatchStringHere->";

static char *ostr = "+ECPD:I:U:d:cir:st:x";

static char *options =
   "[-E] [-C] [-P] [-Dname[=[text]]] [-Uname] [-Ipath] [-dfile]\n    \
[-rpath] [-tname] [-x] [files]";

#if 0
/*
 * The relative path to grttin.h and rt.h depends on whether they are
 *  interpreted as relative to where rtt.exe is or where rtt.exe is
 *  invoked.
 */
static char *grttin_path = "../h/grttin.h";
#endif

#if 0
static char *rt_path = "../h/rt.h";
#endif

/*
 *  Note: rtt presently does not process system include files. If this
 *   is needed, it may be necessary to add other options that set
 *   manifest constants in such include files.  See pmain.c for the
 *   stand-alone preprocessor for examples of what's needed.
 */

char *g_progname = "rtt";
char *compiler_def;
FILE *g_out_file;

#if 0
char *inclname;
#endif

int g_def_fnd = 0;
char *largeints = "LargeInts";

int iconx_flg = 0;

static char *curlst_nm = "rttcur.lst";
static FILE *curlst;
static char *cur_src;

/*
 * tdefnm is used to construct a list of identifiers that
 *  must be treated by rtt as typedef names.
 */
struct tdefnm {
   char *name;
   struct tdefnm *next;
   };

static char *dbname = "rt.db";
static int pp_only = 0;
static char *opt_lst;
static char **opt_args;

#if 0
static char *in_header;
#endif

int main(int argc, char **argv);

static struct tdefnm *tdefnm_lst = NULL;

int main(argc, argv)
int argc;
char **argv;
   {
   int c;
   int nopts;
   char buf[MaxPath];		/* file name construction buffer */
   struct fileparts *fp;

   /*
    * See if the location of include files has been patched into the
    *  rtt executable.
    */
   if ((int)strlen(patchpath) > 18)
      refpath = patchpath + 18;
   else
      refpath = relfile(argv[0], "/../");

   /* Initialize the string table.
    */
   init_str();

   /*
    * By default, the spelling of white space in unimportant (it can
    *  only be significant with the -E option) and #line directives
    *  are required in the output.
    */
   g_whsp_image = NoSpelling;
   line_cntrl = 1;

   /*
    * opt_lst and opt_args are the options and corresponding arguments
    *  that are passed along to the preprocessor initialization routine.
    *  Their number is at most the number of arguments to rtt.
    */
   opt_lst = alloc(argc);
   opt_args = alloc(argc * sizeof (char *));
   nopts = 0;

   /*
    * Process options.
    */
   while ((c = getopt(argc, argv, ostr)) != EOF)
      switch (c) {
	 case 'E': /* run preprocessor only */
	    pp_only = 1;
	    if (g_whsp_image == NoSpelling)
	       g_whsp_image = NoComment;
	    break;
	 case 'C':  /* retain spelling of white space, only effective with -E */
	    g_whsp_image = FullImage;
	    break;
	  case 'P': /* do not produce #line directives in output */
	    line_cntrl = 0;
	    break;
	  case 'd': /* -d name: name of data base */
	    dbname = optarg;
	    break;
	 case 'r':  /* -r path: location of include files */
	    refpath = optarg;
	    break;
	 case 't':  /* -t ident : treat ident as a typedef name */
	    add_tdef(spec_str(optarg));
	    break;
	 case 'x':  /* produce code for interpreter rather than compiler */
	    iconx_flg = 1;
	    break;

	 case 'D':  /* define preprocessor symbol */
	 case 'I':  /* path to search for preprocessor includes */
	 case 'U':  /* undefine preprocessor symbol */
	    /*
	     * Save these options for the preprocessor initialization routine.
	     */
	    opt_lst[nopts] = c;
	    opt_args[nopts] = optarg;
	    ++nopts;
	    break;
	 default:
	    show_usage();
	 }

   #ifdef Rttx
      if (!iconx_flg) {
	 fprintf(stdout,
	    "rtt was compiled to only support the intepreter, use -x\n");
	 exit(EXIT_FAILURE);
	 }
   #endif				/* Rttx */

   if (iconx_flg)
      compiler_def = "#define COMPILER 0\n";
   else
      compiler_def = "#define COMPILER 1\n";

#if 0
   in_header = alloc(strlen(refpath) + strlen(grttin_path) + 1);
   strcpy(in_header, refpath);
   strcat(in_header, grttin_path);
#endif

#if 0
   inclname = alloc(strlen(refpath) + strlen(rt_path) + 1);
   strcpy(inclname, refpath);
   strcat(inclname, rt_path);
#endif

   opt_lst[nopts] = '\0';

   /*
    * At least one file name must be given on the command line.
    */
   if (optind == argc)
      show_usage();

   /*
    * When creating the compiler run-time system, rtt outputs a list
    *  of names of C files created, because most of the file names are
    *  not derived from the names of the input files.
    */
   if (!iconx_flg) {
      curlst = fopen(curlst_nm, "w");
      if (curlst == NULL)
	 err2("cannot open ", curlst_nm);
      }

   /*
    * Unless the input is only being preprocessed, set up the in-memory data
    *  base (possibly loading it from a file).
    */
   if (!pp_only) {
      fp = fparse(dbname);
      if (*fp->ext == '\0')
	 dbname = salloc(makename(buf, sizeof(buf), SourceDir, dbname, DBSuffix));
      else if (!smatch(fp->ext, DBSuffix))
	 err2("bad data base name:", dbname);
      loaddb(dbname);
      }

   /*
    * Scan file name arguments, and translate the files.
    */
   while (optind < argc)  {
      trans(argv[optind]);
      optind++;
      }

   #ifndef Rttx
      /*
       * Unless the user just requested the preprocessor be run, we
       *   have created C files and updated the in-memory data base.
       *   If this is the compiler's run-time system, we must dump
       *   to data base to a file and create a list of all output files
       *   produced in all runs of rtt that created the data base.
       */
      if (!(pp_only || iconx_flg)) {
	 if (fclose(curlst) != 0)
	    err2("cannot close ", curlst_nm);
	 dumpdb(dbname);
	 full_lst("rttfull.lst");
	 }
   #endif				/* Rttx */

   return EXIT_SUCCESS;
   }

/*
 * trans - translate a source file.
 */
void trans(src_file)
char *src_file;
   {
   char buf[MaxPath];		/* file name construction buffer */
   struct fileparts *fp;

   if (g_out_file == NULL) {
      if ((g_out_file = fopen("/dev/null", "w")) == NULL)
	 err1("cannot open output file /dev/null");
      }

   cur_src = src_file;

   /*
    * Read standard header file for preprocessor directives and
    * typedefs, but don't write anything to output.
    */
   init_preproc(opt_lst, opt_args);
#if 0
   source(in_header);
   str_src("<rtt initialization>", compiler_def, (int)strlen(compiler_def));
#endif

   init_sym();

   /* FILE must be treated as a typedef name.
    */
   add_tdef((g_str_FILE = spec_str("FILE")));
   add_tdef((g_str_IGNORE = spec_str("IGNORE")));
   do {
      struct tdefnm *td;
      for (td = tdefnm_lst; td != NULL; td = td->next)
	 sym_add(TypeDefName, td->name, OtherDcl, 1);
   } while (0);

   init_lex();
   yyparse();

   /*
    * Make sure we have a .r file or standard input.
    */
   if (strcmp(cur_src, "-") == 0) {
      source("-"); /* tell preprocessor to read standard input */
      }
   else {
      fp = fparse(cur_src);
      if (*fp->ext == '\0')
	 cur_src = salloc(makename(buf, sizeof(buf), SourceDir, cur_src, RttSuffix));
      else if (!smatch(fp->ext, RttSuffix))
	 err2("unknown file suffix ", cur_src);
      cur_src = spec_str(cur_src);

      /*
       * For the compiler, remove from the data base the list of
       *  files produced from this input file.
       */
      if (!iconx_flg)
	 clr_dpnd(cur_src);
      source(cur_src);  /* tell preprocessor to read source file */
      }

   if (pp_only)
      output(stdout); /* invoke standard preprocessor output routine */
   else {
      /*
       * For the compiler, non-RTL code is put in a file whose name
       *  is derived from input file name. The flag g_def_fnd indicates
       *  if anything interesting is put in the file.
       */
      g_def_fnd = 0;
      yyparse();  /* translate the input */
      prt_str("", 0); /* ensure last LF */
      if (fclose(g_out_file) != 0)
	 err1("cannot close output file");
      }
   finish_preproc();
   /* report_waste(); */
   }

/*
 * add_tdef - add identifier to list of typedef names.
 */
static void add_tdef(name)
char *name;
   {
   struct tdefnm *td;

   td = NewStruct(tdefnm);
   td->name = name;
   td->next = tdefnm_lst;
   tdefnm_lst = td;
   }

/*
 * Add name of file to the output list, and if it contains "interesting"
 *  code, add it to the dependency list in the data base.
 */
void put_c_fl(fname, keep)
char *fname;
int keep;
   {
   struct fileparts *fp;

   fp = fparse(fname);
   fprintf(curlst, "%s\n", fp->name);
   if (keep)
      add_dpnd(src_lkup(cur_src), fname);
   }

/*
 * Print an error message if called incorrectly.
 */
void show_usage()
   {
   fprintf(stderr, "usage: %s %s\n", g_progname, options);
   exit(EXIT_FAILURE);
   }

/*
 * yyerror - error routine called by yacc.
 */
void yyerror(s)
char *s;
   {
   struct token *t;

   t = yylval.t;
   if (t == NULL)
      err2(s, " at end of file");
   else
      errt1(t, s);
   }
