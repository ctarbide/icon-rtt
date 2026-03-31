
#include "rtt.h"

/*#define DEBUG*/
/*#define VERBOSE_PRT_CSTR*/

#define Lit(x) #x
#define WrapLit(x) Lit(x)
#define LINED_LIT_C_STR(lit) lit "/*" WrapLit(__LINE__) "*/"

#ifdef VERBOSE_PRT_CSTR
#define PRT_CSTR(a,b) prt_str(a " /*" WrapLit(__LINE__) "*/",b)
#define PRT_STR(a,b) (prt_str(a,b),prt_str(" /*" WrapLit(__LINE__) "*/",b))
#define PRT_TOK(a,b) (prt_tok(a,b),prt_str(" /*" WrapLit(__LINE__) "*/",b))
#define PRT_CONCATND(a) prt_str("/*" WrapLit(__LINE__) " ConCatNd*/", a)
#else
#define PRT_CSTR(a,b) prt_str(a,b)
#define PRT_STR(a,b) prt_str(a,b)
#define PRT_TOK(a,b) prt_tok(a,b)
#define PRT_CONCATND(a)
#endif

#ifdef DEBUG
#define debugger DEBUG_BREAK()
char* DEBUG_BREAK(void);
char* DEBUG_BREAK()
   {
   return "debug break";
   }
#else
#define debugger
#endif

static struct str_buf sbuf_rttout[1];
#define sbuf sbuf_rttout

#define WHEN_NL_ENUM_LIST 1
#define WHEN_NL_INITIALIZER_LIST 16
#define WHEN_NL_PRIMARY_DECLARATOR_LIST 8
#define WHEN_NL_DECLARATOR_LIST 1
#define WHEN_NL_ARG_LIST 8

#define NotId 0  /* declarator is not simple identifier */
#define IsId  1  /* declarator is simple identifier */

#define OrdFunc -1   /* indicates ordinary C function - non-token value */

/*
 * VArgAlwnc - allowance for the variable part of an argument list in the
 *  most general version of an operation. If it is too small, storage must
 *  be malloced. 3 was chosen because over 90 percent of all writes have
 *  3 or fewer arguments. It is possible that 4 would be a better number,
 *  but 5 is probably overkill.
 */
#define VArgAlwnc 3

#define MAX_NARGS 16
#define MAX_NCHILDREN 16

static int sym_counter = 0;

/*
 * Prototypes for static functions.
 */
static void cnv_fnc       (struct token *t, int typcd,
			       struct node *src, struct node *dflt,
			       struct node *dest, int indent);
static void chk_conj      (struct node *n);
static void chk_nl        (int indent);
static void chk_spc        (void);
static void chk_rsltblk   (int indent);
static void comp_def      (struct node *n);
static int     does_call     (struct node *expr);
static void failure       (int indent, int brace);
static void interp_def    (struct node *n);
static int     len_sel       (struct node *sel,
			       struct parminfo *strt_prms,
			       struct parminfo *end_prms, int indent);
/*static void line_dir      (int nxt_line, char *new_fname);*/
static void parm_locs     (struct sym_entry *op_params);
static void parm_tnd      (struct sym_entry *sym);
static void prt_runerr    (struct token *t, struct node *num,
			       struct node *val, int indent, int brace);
static void prt_tok       (struct token *t, int indent);
static void prt_var       (struct node *n, int indent, int is_lvalue);
static int     retval_dcltor (struct node *dcltor, int indent);
static void ret_value     (struct token *t, struct node *n,
			       int indent);
static void ret_1_arg     (struct token *t, struct node *args,
			       int typcd, char *vwrd_asgn, char *arg_rep,
			       int indent);
static int     rt_walk       (struct node *n, int indent, int brace);
static void spcl_start    (struct sym_entry *op_params);
static void tend_ary      (int n);
static void tend_init     (void);
static void tnd_var       (struct sym_entry *sym, char *strct_ptr, char *access, int indent, int is_lvalue);
static void typ_asrt      (int typcd, struct node *desc,
			       struct token *tok, int indent);
static int     typ_case      (struct node *var, struct node *slct_lst,
			       struct node *dflt,
			       int (*walk)(struct node *n, int xindent,
				 int brace), int maybe_var, int indent);
static void untend        (int indent);

#define MAX_DCLOUT_DATA_ITEMS 1

/* this is used to normalize things like 'struct tag { ... } ident' into
 * a separate declaration and definition
 */
struct dclout_data {
   struct {
      int typagg;			/* type aggregator */
      struct node *node;
      char *chunk_name;
   } items[MAX_DCLOUT_DATA_ITEMS];
   int tally;
};

void tag_name_and_prefix(int id, const char **name, const char **prefix);
void normalize_struct(struct node *n, struct node **declp, struct node **varsp);
struct node *normalize_toplevel(struct node *n, struct node **auxndp);
void top_level_chunk_name(struct dclout_data *data, struct node *n);
static int is_static_function(struct node *head);
static struct node *fnc_head_args(struct node *head);
static int get_args_names(struct node **out, int start, int len, struct node *n);
static int get_comma_children(struct node **out, int start, int len, struct node *n);
/* static int get_declarations(struct node **out, int start, int len, struct node *n); */
static int get_declarations_as_list(struct node **out, int start, int len, struct node *n);
static struct node *header_k_and_r_to_ansi(struct node *head, struct node *prm_dcl);
static struct node *defining_identifier(struct node *n);
static int c_walk_cat(struct node *n, int indent, int brace);
static int c_walk_nl(struct node *n, int indent, int brace, int may_force_nl);
static struct node *has_primry(struct node *n, int p);
static void c_walk_comma(struct node *n, struct token *t, int indent, int brace,
   int may_force_nl, int *counter, int when_nl);
static void c_walk_struct_items(struct node *n, int indent, int brace,
   int may_force_nl);
static int prt_closing_parenthesis(struct node *nxt, int indent);
static int handle_next_indentation(struct node *nxt, int indent);
static int prt_else(struct node *nxt, int indent);
static int does_it_produce_a_compound_statement(struct node *n);

int op_type = OrdFunc;  /* type of operation */
char lc_letter;         /* f = function, o = operator, k = keyword */
char uc_letter;         /* F = function, O = operator, K = keyword */
char prfx1;             /* 1st char of unique prefix for operation */
char prfx2;             /* 2nd char of unique prefix for operation */
char *g_fname = "";     /* current source file name */
int g_line = 0;         /* current source line number */
int g_nxt_sbuf;         /* next string buffer index */
int g_nxt_cbuf;         /* next cset buffer index */
int abs_ret = SomeType; /* type from abstract return(s) */

int g_nl = 0;           /* flag indicating the a new-line should be output */
int g_spc = 0;          /* flag indicating the a new-space should be output */
static int no_nl = 0;   /* flag to suppress line directives */

static int ntend;       /* number of tended descriptor needed */
static char *tendstrct; /* expression to access struct of tended descriptors */
static char *rslt_loc;  /* expression to access result location */
static int varargs = 0; /* flag: operation takes variable number of arguments */

static int no_ret_val;  /* function has return statement with no value */
static struct node *fnc_head; /* header of function being "copied" to output */

/*
 * chk_nl - if a new-line is required, output it and indent the next line.
 */
static void chk_nl(int indent)
   {
   int col;

   if (g_nl) {
      /*
       * new-line required.
       */
      putc('\n', g_out_file);
      ++g_line;
      for (col = 0; col < indent; ++col)
	 putc(' ', g_out_file);
      g_nl = 0;
      g_spc = 0;
      }
   }

static void chk_spc(void)
   {
   if (g_spc) {
      putc(' ', g_out_file);
      g_spc = 0;
      }
   }

/*
 * line_dir - Output a line directive.
 */
#if 0
static void line_dir(nxt_line, new_fname)
int nxt_line;
char *new_fname;
   {
   char *s;

   /*
    * Make sure line directives are desired in the output. Normally,
    *  blank lines surround the directive for readability. However,`
    *  a preceding blank line is suppressed at the beginning of the
    *  output file. In addition, a blank line is suppressed after
    *  the directive if it would force the line number on the directive
    *  to be 0.
    */
   if (line_cntrl) {
      fprintf(g_out_file, "\n");
      if (g_line != 0)
	 fprintf(g_out_file, "\n");
      if (nxt_line == 1)
	 fprintf(g_out_file, "/*#line %d \"", nxt_line);
      else
	 fprintf(g_out_file, "/*#line %d \"", nxt_line - 1);
      for (s = new_fname; *s != '\0'; ++s) {
	 if (*s == '"' || *s == '\\')
	    putc('\\', g_out_file);
	 putc(*s, g_out_file);
	 }
      if (nxt_line == 1)
	 fprintf(g_out_file, "\"*/");
      else
	 fprintf(g_out_file, "\"*/\n");
      g_nl = 1;
      --nxt_line;
      }
    else if ((nxt_line > g_line || g_fname != new_fname) && g_line != 0) {
      /*
       * Line directives are disabled, but we are in a situation where
       *  one or two new-lines are desirable.
       */
      if (nxt_line > g_line + 1 || g_fname != new_fname)
	 fprintf(g_out_file, "\n");
      g_nl = 1;
      --nxt_line;
      }
   g_line = nxt_line;
   g_fname = new_fname;
   }
#endif

/*
 * prt_str - print a string to the output file, possibly preceded by
 *   a new-line and indenting.
 */
void prt_str(s, indent)
char *s;
int indent;
   {
   chk_nl(indent);
   fprintf(g_out_file, "%s", s);
   }

/*
 * prt_tok - print a token.
 */
static void prt_tok(t, indent)
struct token *t;
int indent;
   {
   char *s;

   chk_nl(indent);

   /*
    * Most tokens contain a string of their exact image. However, string
    *  and character literals lack the surrounding quotes.
    */
   s = t->image;
   switch (t->tok_id) {
      case StrLit:
	 fprintf(g_out_file, "\"%s\"", s);
	 break;
      case LStrLit:
	 fprintf(g_out_file, "L\"%s\"", s);
	 break;
      case CharConst:
	 fprintf(g_out_file, "'%s'", s);
	 break;
      case LCharConst:
	 fprintf(g_out_file, "L'%s'", s);
	 break;
      case LShft:
      case LShftAsgn:
	 fprintf(g_out_file, "@%s", s);
	 break;
      default:
	 fprintf(g_out_file, "%s", s);
      }
   }

/*
 * untend - output code to removed the tended descriptors in this
 *  function from the global tended list.
 */
static void untend(indent)
int indent;
   {
   ForceNl();
   PRT_CSTR("tend = ", indent);
   fprintf(g_out_file, "%s.previous;", tendstrct);
   ForceNl();
   /*
    * For varargs operations, the tended structure might have been
    *  malloced. If so, it must be freed.
    */
   if (varargs) {
      PRT_CSTR("if (r_tendp != &r_tend)", indent);
      ForceNl();
      PRT_CSTR("free(r_tendp);", indent + IndentInc);
      ForceNl();
      }
   }

/*
 * tnd_var - output an expression to accessed a tended variable.
 */
static void tnd_var(sym, strct_ptr, access, indent, is_lvalue)
struct sym_entry *sym;
char *strct_ptr, *access;
int indent, is_lvalue;
   {
   if (!is_lvalue && strct_ptr) {
      PRT_CSTR("((struct ", indent);
      prt_str(strct_ptr, indent);
      PRT_CSTR("*)", indent);
      }
   if (sym->id_type & ByRef) {
      /*
       * The tended variable is being accessed indirectly through
       *  a pointer (that is, it is accessed as the argument to a body
       *  function); dereference its identifier.
       */
      PRT_CSTR("(*", indent);
      prt_str(sym->image, indent);
      PRT_CSTR(")", indent);
      }
   else {
      if (sym->t_indx >= 0) {
	 /*
	  * The variable is accessed directly as part of the tended structure.
	  */
	 prt_str(tendstrct, indent);
	 fprintf(g_out_file, ".d[%d]", sym->t_indx);
	 }
      else {
	 /*
	  * This is a direct access to an operation parameter.
	  */
	 PRT_CSTR("r_args[", indent);
	 fprintf(g_out_file, "%d]", sym->u.param_info.param_num + 1);
	 }
      }
   PRT_STR(access, indent);  /* access the vword for tended pointers */
   if (!is_lvalue && strct_ptr)
      PRT_CSTR(")", indent);
   }

/*
 * prt_var - print a variable.
 */
static void prt_var(n, indent, is_lvalue)
struct node *n;
int indent, is_lvalue;
   {
   struct token *t;
   struct sym_entry *sym;

   if (n->nd_id != SymNd && n->nd_id != CompNd) {
      fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
	 __FILE__, __LINE__, g_fname_for___FILE__,
	 g_line_for___LINE__);
      exit(1);
      }

   t = n->tok;
   sym = n->u[0].sym;
   switch (sym->id_type & ~ByRef) {
      case TndDesc:
	 /*
	  * Simple tended descriptor.
	  */
	 tnd_var(sym, NULL, "", indent, is_lvalue);
	 break;
      case TndStr:
	 /*
	  * Tended character pointer.
	  */
	 tnd_var(sym, NULL, ".vword.sptr", indent, is_lvalue);
	 break;
      case TndBlk:
	 /*
	  * Tended block pointer.
	  */
	 if (is_lvalue)
	    tnd_var(sym, NULL, ".vword.ptr", indent, is_lvalue);
	 else
	    tnd_var(sym, sym->u.tnd_var.blk_name,
	       sym->u.tnd_var.blk_name ? ".vword.ptr" : ".vword.bptr",
	       indent, is_lvalue);
	 break;
      case RtParm:
      case DrfPrm:
	 switch (sym->u.param_info.cur_loc) {
	    case PrmTend:
	       /*
		* Simple tended parameter.
		*/
	       tnd_var(sym, NULL, "", indent, is_lvalue);
	       break;
	    case PrmCStr:
	       /*
		* Parameter converted to a (tended) string.
		*/
	       tnd_var(sym, NULL, ".vword.sptr", indent, is_lvalue);
	       break;
	    case PrmInt:
	       /*
		* Parameter converted to a C integer.
		*/
	       chk_nl(indent);
	       fprintf(g_out_file, "r_i%d", sym->u.param_info.param_num);
	       break;
	    case PrmDbl:
	       /*
		* Parameter converted to a C double.
		*/
	       chk_nl(indent);
	       fprintf(g_out_file, "r_d%d", sym->u.param_info.param_num);
	       break;
	    default:
	       errt2(t, "Conflicting conversions for: ", t->image);
	    }
	 break;
      case RtParm | VarPrm:
      case DrfPrm | VarPrm:
	 /*
	  * Parameter representing variable part of argument list.
	  */
	 PRT_CSTR("(&", indent);
	 if (sym->t_indx >= 0)
	    fprintf(g_out_file, "%s.d[%d])", tendstrct, sym->t_indx);
	 else
	    fprintf(g_out_file, "r_args[%d])", sym->u.param_info.param_num + 1);
	 break;
      case VArgLen:
	 /*
	  * Length of variable part of argument list.
	  */
	 chk_nl(indent);
	 fprintf(g_out_file, "(r_nargs - %d)", g_params->u.param_info.param_num);
	 break;
      case RsltLoc:
	 /*
	  * "result" the result location of the operation.
	  */
	 prt_str(rslt_loc, indent);
	 break;
      case Label:
	 /*
	  * Statement label.
	  */
	 prt_str(sym->image, indent);
	 break;
      case OtherDcl:
	 /*
	  * Some other type of variable: accessed by identifier. If this
	  *  is a body function, it may be passed by reference and need
	  *  a level of pointer dereferencing.
	  */
	 if (sym->id_type & ByRef)
	    PRT_CSTR("(*",indent);
	 prt_str(sym->image, indent);
	 if (sym->id_type & ByRef)
	    PRT_CSTR(")",indent);
	 break;
      }
   }

/*
 * does_call - determine if an expression contains a function call by
 *  walking its syntax tree.
 */
static int does_call(expr)
struct node *expr;
   {
   int n_subs;
   int i;

   if (expr == NULL)
      return 0;
   if (expr->nd_id == BinryNd && expr->tok->tok_id == ')')
      return 1;      /* found a function call */

   switch (expr->nd_id) {
      case ExactCnv:
      case PrimryNd:
      case SymNd:
	 n_subs = 0;
	 break;
      case CompNd:
	 /*
	  * Check field 0 below, field 1 is not a subtree, check field 2 here.
	  */
	 n_subs = 1;
	 if (does_call(expr->u[2].child))
	     return 1;
	 break;
      case IcnTypNd:
      case PstfxNd:
      case PreSpcNd:
      case PrefxNd:
	 n_subs = 1;
	 break;
      case AbstrNd:
      case BinryNd:
      case CommaNd:
      case ConCatNd:
      case LstNd:
      case StrDclNd: /* structure field declaration */
	 n_subs = 2;
	 break;
      case TrnryNd:
	 n_subs = 3;
	 break;
      case QuadNd:
	 n_subs = 4;
	 break;
      default:
	 fprintf(stdout, "rtt internal error: unknown node type\n");
	 exit(EXIT_FAILURE);
	 }

   for (i = 0; i < n_subs; ++i)
      if (does_call(expr->u[i].child))
	  return 1;

   return 0;
   }

/*
 * prt_runerr - print code to implement runerr().
 */
static void prt_runerr(t, num, val, indent, brace)
struct token *t;
struct node *num, *val;
const int indent;
const int brace; /* do we already have braces? */
   {
   int ind;

   ind = indent;
   if (op_type == OrdFunc)
      errt1(t, "'runerr' may not be used in an ordinary C function");
   if (!brace) {
      chk_spc();
      PRT_CSTR("{", ind);
      ind += IndentInc;
      ForceNl();
      }
   PRT_CSTR("err_msg(", ind);
   c_walk(num, ind, 0);                /* error number */
   if (val == NULL)
      PRT_CSTR(", NULL);", ind);        /* no offending value */
   else {
      PRT_CSTR(", &(", ind);
      c_walk(val, ind, 0);             /* offending value */
      PRT_CSTR("));", ind);
      }
   /*
    * Handle error conversion. Indicate that operation may fail because
    *  of error conversion and produce the necessary code.
    */
   cur_impl->ret_flag |= DoesEFail;
   failure(ind, 1);
   if (!brace) {
      PRT_CSTR("}", ind);
      }
   ForceNl();
   }

/*
 * typ_name - convert a type code to a string that can be used to
 *  output "T_" or "D_" type codes.
 */
char *typ_name(typcd, tok)
int typcd;
struct token *tok;
   {
   if (typcd == Empty_type)
      errt1(tok, "it is meaningless to assert a type of empty_type");
   else if (typcd == Any_value)
      errt1(tok, "it is useless to assert a type of any_value");
   else if (typcd < 0 || typcd == str_typ)
      return NULL;
   else
      return icontypes[typcd].cap_id;
   /*NOTREACHED*/
   return 0;			/* avoid gcc warning */
   }

/*
 * Produce a C conditional expression to check a descriptor for a
 *  particular type.
 */
static void typ_asrt(typcd, desc, tok, indent)
int typcd;
struct node *desc;
struct token *tok;
int indent;
   {
   if (typcd == str_typ) {
      /*
       * Check dword for the absense of a "not qualifier" flag.
       */
      PRT_CSTR("(!((", indent);
      c_walk(desc, indent, 0);
      PRT_CSTR(").dword & F_Nqual))", indent);
      }
   else if (typcd == TypVar) {
      /*
       * Check dword for the presense of a "variable" flag.
       */
      PRT_CSTR("(((", indent);
      c_walk(desc, indent, 0);
      PRT_CSTR(").dword & D_Var) == D_Var)", indent);
      }
   else if (typcd == int_typ) {
      /*
       * If large integers are supported, an integer can be either
       *  an ordinary integer or a large integer.
       */
      ForceNl();
      PRT_CSTR("(((", indent);
      c_walk(desc, indent, 0);
      PRT_CSTR(").dword == (uword)D_Integer) || ((", indent);
      c_walk(desc, indent, 0);
      PRT_CSTR(").dword == (uword)D_Lrgint))", indent);
      ForceNl();
      }
   else {
      /*
       * Check dword for a specific type code.
       */
      PRT_CSTR("((", indent);
      c_walk(desc, indent, 0);
      fprintf(g_out_file, ").dword == (uword)D_%s)", typ_name(typcd, tok));
      }
   }

/*
 * retval_dcltor - convert the "declarator" part of function declaration
 *  into a declarator for the variable "r_retval" of the same type
 *  as the function result type, outputing the new declarator. This
 *  variable is a temporary location to store the result of the argument
 *  to a C return statement.
 */
static int retval_dcltor(dcltor, indent)
struct node *dcltor;
int indent;
   {
   int flag;

   switch (dcltor->nd_id) {
      case ConCatNd:
	 c_walk(dcltor->u[0].child, indent, 0);
	 retval_dcltor(dcltor->u[1].child, indent);
	 return NotId;
      case PrimryNd:
	 /*
	  * We have reached the function name. Replace it with "r_retval"
	  *  and tell caller we have found it.
	  */
	 PRT_CSTR("r_retval", indent);
	 return IsId;
      case PrefxNd:
	 /*
	  * (...)
	  */
	 PRT_CSTR("(", indent);
	 flag = retval_dcltor(dcltor->u[0].child, indent);
	 PRT_CSTR(")", indent);
	 return flag;
      case BinryNd:
	 if (dcltor->tok->tok_id == ')') {
	    /*
	     * Function declaration. If this is the declarator that actually
	     *  defines the function being processed, discard the paramater
	     *  list including parentheses.
	     */
	    if (retval_dcltor(dcltor->u[0].child, indent) == NotId) {
	       PRT_CSTR("(", indent);
	       c_walk(dcltor->u[1].child, indent, 0);
	       PRT_CSTR(")", indent);
	       }
	    }
	 else {
	    /*
	     * Array.
	     */
	    retval_dcltor(dcltor->u[0].child, indent);
	    PRT_CSTR("[", indent);
	    c_walk(dcltor->u[1].child, indent, 0);
	    PRT_CSTR("]", indent);
	    }
	 return NotId;
      }
   err1("rtt internal error detected in function retval_dcltor()");
   /*NOTREACHED*/
   return 0;			/* avoid gcc warning */
   }

/*
 * cnv_fnc - produce code to handle RTT cnv: and def: constructs.
 */
static void cnv_fnc(t, typcd, src, dflt, dest, indent)
struct token *t;
int typcd;
struct node *src, *dflt, *dest;
int indent;
   {
   int dflt_to_ptr;
   int loc;
   int is_cstr;

   if (src->nd_id == SymNd && src->u[0].sym->id_type & VarPrm)
      errt1(t, "converting entire variable part of param list not supported");

   /*
    * Initial assumptions: result of conversion is a tended location
    *   and is not tended C string.
    */
   loc = PrmTend;
   is_cstr = 0;

  /*
   * Print the name of the conversion function. If it is a conversion
   *  with a default value, determine (through dflt_to_prt) if the
   *  default value is passed by-reference instead of by-value.
   */
   prt_str(cnv_name(typcd, dflt, &dflt_to_ptr), indent);
   PRT_CSTR("(", indent);

   /*
    * Determine what parameter scope, if any, is established by this
    *  conversion. If the conversion needs a buffer, allocate it and
    *  put it in the argument list.
    */
   switch (typcd) {
      case TypCInt:
      case TypECInt:
	 loc = PrmInt;
	 break;
      case TypCDbl:
	 loc = PrmDbl;
	 break;
      case TypCStr:
	 is_cstr = 1;
	 break;
      case TypTStr:
	 fprintf(g_out_file, "r_sbuf[%d], ", g_nxt_sbuf++);
	 break;
      case TypTCset:
	 fprintf(g_out_file, "&r_cbuf[%d], ", g_nxt_cbuf++);
	 break;
      }

   /*
    * Output source of conversion.
    */
   PRT_CSTR("&(", indent);
   c_walk(src, indent, 0);
   PRT_CSTR("), ", indent);

   /*
    * If there is a default value, output it, taking its address if necessary.
    */
   if (dflt != NULL) {
      if (dflt_to_ptr)
	 PRT_CSTR("&(", indent);
      c_walk(dflt, indent, 0);
      if (dflt_to_ptr)
	 PRT_CSTR("), ", indent);
      else
	 PRT_CSTR(", ", indent);
      }

   /*
    * Output the destination of the conversion. This may or may not be
    *  the same as the source.
    */
   PRT_CSTR("&(", indent);
   if (dest == NULL) {
      /*
       * Convert "in place", changing the location of a paramater if needed.
       */
      if (src->nd_id == SymNd && src->u[0].sym->id_type & (RtParm | DrfPrm)) {
	 if (src->u[0].sym->id_type & DrfPrm)
	    src->u[0].sym->u.param_info.cur_loc = loc;
	 else
	    errt1(t, "only dereferenced parameter can be converted in-place");
	 }
      else if ((loc != PrmTend) | is_cstr)
	 errt1(t,
	    "only ordinary parameters can be converted in-place to C values");
      c_walk(src, indent, 0);
      if (is_cstr) {
	 /*
	  * The parameter must be accessed as a tended C string, but only
	  *  now, after the "destination" code has been produced as a full
	  *  descriptor.
	  */
	 src->u[0].sym->u.param_info.cur_loc = PrmCStr;
	 }
      }
   else {
      /*
       * Convert to an explicit destination.
       */
      if (is_cstr) {
	 /*
	  * Access the destination as a full descriptor even though it
	  *  must be declared as a tended C string.
	  */
	 if (dest->nd_id != SymNd || (dest->u[0].sym->id_type != TndStr &&
	       dest->u[0].sym->id_type != TndDesc))
	    errt1(t,
	     "dest. of C_string conv. must be tended descriptor or char *");
	 tnd_var(dest->u[0].sym, NULL, "", indent, 0 /* is_lvalue */);
	 }
      else
	 c_walk(dest, indent, 0);
      }
   PRT_CSTR("))", indent);
   }

/*
 * cnv_name - produce name of conversion routine. Warning, name is
 *   constructed in a static buffer. Also determine if a default
 *   must be passed "by reference".
 */
char *cnv_name(typcd, dflt, dflt_to_ptr)
int typcd;
struct node *dflt;
int *dflt_to_ptr;
   {
   static char buf[15];
   int by_ref;

   /*
    * The names of simple conversion and defaulting conversions have
    *  the same suffixes, but different prefixes.
    */
   if (dflt == NULL)
      strcpy(buf , "cnv_");
   else
       strcpy(buf, "def_");

   by_ref = 0;
   switch (typcd) {
      case TypCInt:
	 strcat(buf, "c_int");
	 break;
      case TypCDbl:
	 strcat(buf, "c_dbl");
	 break;
      case TypCStr:
	 strcat(buf, "c_str");
	 break;
      case TypTStr:
	 strcat(buf, "tstr");
	 by_ref = 1;
	 break;
      case TypTCset:
	 strcat(buf, "tcset");
	 by_ref = 1;
	 break;
      case TypEInt:
	 strcat(buf, "eint");
	 break;
      case TypECInt:
	 strcat(buf, "ec_int");
	 break;
      default:
	 if (typcd == cset_typ) {
	    strcat(buf, "cset");
	    by_ref = 1;
	    }
	 else if (typcd == int_typ)
	    strcat(buf, "int");
	 else if (typcd == real_typ)
	    strcat(buf, "real");
	 else if (typcd == str_typ) {
	    strcat(buf, "str");
	    by_ref = 1;
	    }
      }
   if (dflt_to_ptr != NULL)
      *dflt_to_ptr = by_ref;
   return buf;
   }

/*
 * ret_value - produce code to set the result location of an operation
 *  using the expression on a return or suspend.
 */
static void ret_value(t, n, indent)
struct token *t;
struct node *n;
int indent;
   {
   struct node *caller;
   struct node *args;
   int typcd;

   if (n == NULL)
      errt1(t, "there is no default return value for run-time operations");

   if (n->nd_id == SymNd && n->u[0].sym->id_type == RsltLoc) {
      /*
       * return/suspend result;
       *
       *   result already where it needs to be.
       */
      return;
      }

   if (n->nd_id == PrefxNd && n->tok) {
      switch (n->tok->tok_id) {
	 case C_Integer:
	    /*
	     * return/suspend C_integer <expr>;
	     */
	    prt_str(rslt_loc, indent);
	    PRT_CSTR(".vword.uintgr = ", indent);
	    c_walk(n->u[0].child, indent + IndentInc, 0);
	    PRT_CSTR(";", indent);
	    ForceNl();
	    prt_str(rslt_loc, indent);
	    PRT_CSTR(".dword = D_Integer;", indent);
	    chkabsret(t, int_typ);  /* compare return with abstract return */
	    return;
	 case C_Double:
	    /*
	     * return/suspend C_double <expr>;
	     */
	    prt_str(rslt_loc, indent);
	    PRT_CSTR(".vword.ptr = alcreal(", indent);
	    c_walk(n->u[0].child, indent + IndentInc, 0);
	    PRT_CSTR(");", indent + IndentInc);
	    ForceNl();
	    prt_str(rslt_loc, indent);
	    PRT_CSTR(".dword = D_Real;", indent);
	    /*
	     * The allocation of the real block may fail.
	     */
	    chk_rsltblk(indent);
	    chkabsret(t, real_typ); /* compare return with abstract return */
	    return;
	 case C_String:
	    /*
	     * return/suspend C_string <expr>;
	     */
	    prt_str(rslt_loc, indent);
	    PRT_CSTR(".vword.sptr = (char*)", indent);
	    c_walk(n->u[0].child, indent + IndentInc, 0);
	    PRT_CSTR(";", indent);
	    ForceNl();
	    prt_str(rslt_loc, indent);
	    PRT_CSTR(".dword = strlen(", indent);
	    prt_str(rslt_loc, indent);
	    PRT_CSTR(".vword.sptr);", indent);
	    chkabsret(t, str_typ); /* compare return with abstract return */
	    return;
	 }
      }
   else if (n->nd_id == BinryNd && n->tok->tok_id == ')') {
      /*
       * Return value is in form of function call, see if it is really
       *  a descriptor constructor.
       */
      caller = n->u[0].child;
      args = n->u[1].child;
      if (caller->nd_id == SymNd) {
	 switch (caller->tok->tok_id) {
	    case IconType:
	       typcd = caller->u[0].sym->u.typ_indx;
	       switch (icontypes[typcd].rtl_ret) {
		  case TRetBlkP:
		     /*
		      * return/suspend <type>(<block-pntr>);
		      */
		     ret_1_arg(t, args, typcd, ".vword.ptr = ",
			"(bp)", indent);
		     break;
		  case TRetDescP:
		     /*
		      * return/suspend <type>(<desc-pntr>);
		      */
		     ret_1_arg(t, args, typcd, ".vword.descptr = (dptr)",
			"(dp)", indent);
		     break;
		  case TRetCharP:
		     /*
		      * return/suspend <type>(<char-pntr>);
		      */
		     ret_1_arg(t, args, typcd, ".vword.sptr = (char*)",
			"(s)", indent);
		     break;
		  case TRetCInt:
		     /*
		      * return/suspend <type>(<integer>);
		      */
		     ret_1_arg(t, args, typcd, ".vword.uintgr = (uword)",
			"(i)", indent);
		     break;
		  case TRetSpcl:
		     if (typcd == str_typ) {
			/*
			 * return/suspend string(<len>, <char-pntr>);
			 */
			int emit_parenthesis;
			if (args == NULL || args->nd_id != CommaNd ||
			   args->u[0].child->nd_id == CommaNd)
			   errt1(t, "wrong no. of args for string(n, s)");
			prt_str(rslt_loc, indent);
			emit_parenthesis =
			   is_a(args->u[1].child, SymNd) == NULL &&
			   is_t(args->u[1].child, PrefxNd, '(') == NULL;
			if (emit_parenthesis)
			   PRT_CSTR(".vword.sptr = (char*)(", indent);
			else
			   PRT_CSTR(".vword.sptr = (char*)", indent);
			c_walk(args->u[1].child, indent + IndentInc, 0);
			if (emit_parenthesis)
			   PRT_CSTR(");", indent);
			else
			   PRT_CSTR(";", indent);
			ForceNl();
			prt_str(rslt_loc, indent);
			PRT_CSTR(".dword = ", indent);
			c_walk(args->u[0].child, indent + IndentInc, 0);
			PRT_CSTR(";", indent);
			}
		     else if (typcd == stv_typ) {
			/*
			 * return/suspend tvsubs(<desc-pntr>, <start>, <len>);
			 */
			if (args == NULL || args->nd_id != CommaNd ||
			   args->u[0].child->nd_id != CommaNd ||
			   args->u[0].child->u[0].child->nd_id == CommaNd)
			   errt1(t, "wrong no. of args for tvsubs(dp, i, j)");
			no_nl = 1;
			PRT_CSTR("SubStr(&", indent);
			prt_str(rslt_loc, indent);
			PRT_CSTR(", ", indent);
			c_walk(args->u[0].child->u[0].child, indent + IndentInc,
			   0);
			PRT_CSTR(", ", indent + IndentInc);
			c_walk(args->u[1].child, indent + IndentInc, 0);
			PRT_CSTR(", ", indent + IndentInc);
			c_walk(args->u[0].child->u[1].child, indent + IndentInc,
			  0);
			PRT_CSTR(");", indent + IndentInc);
			no_nl = 0;
			/*
			 * The allocation of the substring trapped variable
			 *   block may fail.
			 */
			chk_rsltblk(indent);
			chkabsret(t, stv_typ); /* compare to abstract return */
			}
		     break;
		  }
	       chkabsret(t, typcd); /* compare return with abstract return */
	       return;
	    case Named_var:
	       /*
		* return/suspend named_var(<desc-pntr>);
		*/
	       if (args == NULL || args->nd_id == CommaNd)
		  errt1(t, "wrong no. of args for named_var(dp)");
	       prt_str(rslt_loc, indent);
	       PRT_CSTR(".vword.descptr = ", indent);
	       c_walk(args, indent + IndentInc, 0);
	       PRT_CSTR(";", indent);
	       ForceNl();
	       prt_str(rslt_loc, indent);
	       PRT_CSTR(".dword = D_Var;", indent);
	       chkabsret(t, TypVar); /* compare return with abstract return */
	       return;
	    case Struct_var:
	       /*
		* return/suspend struct_var(<desc-pntr>, <block_pntr>);
		*/
	       if (args == NULL || args->nd_id != CommaNd ||
		  args->u[0].child->nd_id == CommaNd)
		  errt1(t, "wrong no. of args for struct_var(dp, bp)");
	       prt_str(rslt_loc, indent);
	       PRT_CSTR(".vword.descptr = (dptr)", indent);
	       c_walk(args->u[1].child, indent + IndentInc, 0);
	       PRT_CSTR(";", indent);
	       ForceNl();
	       prt_str(rslt_loc, indent);
	       PRT_CSTR(".dword = D_Var + (uword)((uword*)", indent);
	       c_walk(args->u[0].child, indent + IndentInc, 0);
	       PRT_CSTR(" - (uword*)", indent+IndentInc);
	       prt_str(rslt_loc, indent);
	       PRT_CSTR(".vword.descptr);", indent+IndentInc);
	       ForceNl();
	       chkabsret(t, TypVar); /* compare return with abstract return */
	       return;
	    }
	 }
      }

   /*
    * If it is not one of the special returns, it is just a return of
    *  a descriptor.
    */
   prt_str(rslt_loc, indent);
   PRT_CSTR(" = ", indent);
   c_walk(n, indent + IndentInc, 0);
   PRT_CSTR(";", indent);
   chkabsret(t, SomeType); /* check for preceding abstract return */
   }

/*
 * ret_1_arg - produce code for a special return/suspend with one argument.
 */
static void ret_1_arg(t, args, typcd, vwrd_asgn, arg_rep, indent)
struct token *t;
struct node *args;
int typcd, indent;
char *vwrd_asgn, *arg_rep;
   {
   if (args == NULL || args->nd_id == CommaNd)
      errt3(t, "wrong no. of args for", icontypes[typcd].id, arg_rep);

   /*
    * Assignment to vword of result descriptor.
    */
   prt_str(rslt_loc, indent);
   prt_str(vwrd_asgn, indent);
   c_walk(args, indent + IndentInc, 0);
   PRT_CSTR(";", indent);
   ForceNl();

   /*
    * Assignment to dword of result descriptor.
    */
   prt_str(rslt_loc, indent);
   fprintf(g_out_file, ".dword = D_%s;", icontypes[typcd].cap_id);
   }

/*
 * chk_rsltblk - the result value contains an allocated block, make sure
 *    the allocation succeeded.
 */
static void chk_rsltblk(indent)
int indent;
   {
   ForceNl();
   PRT_CSTR("if (", indent);
   prt_str(rslt_loc, indent);
   PRT_CSTR(".vword.ptr == NULL) {", indent);
   ForceNl();
   PRT_CSTR("err_msg(307, NULL);", indent + IndentInc);
   ForceNl();
   /*
    * Handle error conversion. Indicate that operation may fail because
    *  of error conversion and produce the necessary code.
    */
   cur_impl->ret_flag |= DoesEFail;
   failure(indent + IndentInc, 1);
   PRT_CSTR("}", indent + IndentInc);
   ForceNl();
   }

/*
 * failure - produce code for fail or efail.
 */
static void failure(indent, brace)
const int indent, brace;
   {
   int ind;
   /*
    * If there are tended variables, they must be removed from the tended
    *  list. The C function may or may not return an explicit signal.
    */
   ind = indent;
   ForceNl();
   if (ntend) {
      if (!brace) {
	 PRT_CSTR("{", ind);
	 ind += IndentInc;
	 }
      untend(ind);
      ForceNl();
      if (fnc_ret == RetSig)
	 PRT_CSTR("return A_Resume;", ind);
      else
	 PRT_CSTR("return;", ind);
      if (!brace) {
	 ForceNl();
	 PRT_CSTR("}", ind);
	 }
      }
   else {
      if (fnc_ret == RetSig)
	 PRT_CSTR("return A_Resume;", ind);
      else
	 PRT_CSTR("return;", ind);
      }
   ForceNl();
   }

/*
 * c_walk - walk the syntax tree for extended C code and output the
 *  corresponding ordinary C. Return and indication of whether execution
 *  falls through the code.
 */
int c_walk(n, indent, brace)
struct node *n;
const int indent, brace;
   {
   return c_walk_nl(n, indent, brace, 1);
   }

static int c_walk_nl(n, indent, brace, may_force_nl)
struct node *n;
const int indent, brace, may_force_nl;
   {
   struct token *t;
   struct node *n1, *n2;
   struct sym_entry *sym;
   int fall_thru;
   int save_break;
   static int does_break = 0;
   static int may_brnchto;  /* may reach end of code by branching into middle */
   static int found_switch = 0;
   int ind;    /* auxiliary indentation */

   if (n == NULL)
      return 1;

   /* fprintf(stderr, "/""*%d,%s*""/\n", __LINE__, node_name(n)); */

   t = n->tok;
   switch (n->nd_id) {
      case PrimryNd: /* simply a token */
	 chk_spc();
	 switch (t->tok_id) {
	    case Fail:
	       if (op_type == OrdFunc)
		  errt1(t, "'fail' may not be used in an ordinary C function");
	       cur_impl->ret_flag |= DoesFail;
	       failure(indent, brace);
	       chkabsret(t, SomeType);  /* check preceding abstract return */
	       return 0;
	    case Errorfail:
	       if (op_type == OrdFunc)
		  errt1(t,
		     "'errorfail' may not be used in an ordinary C function");
	       cur_impl->ret_flag |= DoesEFail;
	       failure(indent, brace);
	       return 0;
	    case Break:
	       prt_tok(t, indent);
	       PRT_CSTR(";", indent);
	       ForceNl();
	       does_break = 1;
	       return 0;
	    default:
	       /*
		* Other "primary" expressions are just their token image,
		*  possibly followed by a semicolon.
		*/
	       PRT_TOK(t, indent);
	       if (t->tok_id == Continue) {
		  PRT_CSTR(";", indent);
		  ForceNl();
		  }
	       return 1;
	    }
      case PrefxNd: /* a prefix expression */
	 chk_spc();
	 switch (t->tok_id) {
	    case Sizeof:
	       prt_tok(t, indent);                /* sizeof */
	       PRT_CSTR("(", indent);
	       c_walk(n->u[0].child, indent, 0);
	       PRT_CSTR(")", indent);
	       return 1;
	    case '{':
	       /*
		* Initializer list.
		*/
	       if ((n1 = is_t(n->u[0].child, CommaNd, ','))) {
		  int counter;
		  chk_spc();
		  PRT_TOK(t, indent + IndentInc);     /* { */
		  ForceNl();
		  counter = 1;
		  c_walk_comma(n1->u[0].child, NULL, indent + IndentInc * 2,
		     brace, may_force_nl, &counter, WHEN_NL_INITIALIZER_LIST);
		  c_walk_comma(n1->u[1].child, n1->tok, indent + IndentInc * 2,
		     brace, may_force_nl, &counter, WHEN_NL_INITIALIZER_LIST);
		  ForceNl();
		  PRT_CSTR("}", indent + IndentInc * 2);
		  }
	       else if ((n1 = is_t(n->u[0].child, PrefxNd, '{'))) {
		  chk_spc();
		  PRT_TOK(t, indent);     /* { */
		  c_walk(n1, indent, 0 /* brace */);
		  PRT_CSTR("}", indent);
		  }
	       else {
		  if (is_a(n->u[0].child, PrimryNd) == NULL)
		     /* multiple items, force nl */
		     ForceNl();
		  chk_spc();
		  PRT_TOK(t, indent + IndentInc);     /* { */
		  c_walk(n->u[0].child, indent + IndentInc, 0);
		  PRT_CSTR("}", indent + IndentInc);
		  }
	       return 1;
	    case PassThru:
	       if ((n1 = is_tt(n->u[0].child, CommaNd, ',', ':'))) {
		  int counter = 1;
		  c_walk_comma(n1->u[0].child, NULL, indent + IndentInc,
		     brace, may_force_nl, &counter, WHEN_NL_ARG_LIST);
		  c_walk_comma(n1->u[1].child, n1->tok, indent + IndentInc,
		     brace, may_force_nl, &counter, WHEN_NL_ARG_LIST);
		  }
	       else
		  c_walk(n->u[0].child, indent, 0);
	       return 1;
	    case CompatAsm:
	       prt_tok(t, indent);                /* __asm__ */
	       PRT_CSTR("(", indent);
	       ForceNl();
	       if ((n1 = is_tt(n->u[0].child, CommaNd, ',', ':'))) {
		  int counter = 1;
		  c_walk_comma(n1->u[0].child, NULL, indent + IndentInc,
		     brace, may_force_nl, &counter, WHEN_NL_ARG_LIST);
		  c_walk_comma(n1->u[1].child, n1->tok, indent + IndentInc,
		     brace, may_force_nl, &counter, WHEN_NL_ARG_LIST);
		  }
	       else
		  c_walk(n->u[0].child, indent, 0);
	       PRT_CSTR(")", indent);
	       return 1;
	    case Default:
	       /*
		* default : <statement>
		*/
	       ForceNl();
	       PRT_TOK(t, indent);
	       n1 = n->u[0].child;
	       if (is_t(n1, BinryNd, Case)) {
		  PRT_CSTR(":", indent);
		  fall_thru = c_walk(n1, indent, 0 /* brace */);
		  }
	       else {
		  if (is_t(n1, CompNd, '{'))
		     PRT_CSTR(": ", indent);
		  else {
		     PRT_CSTR(":", indent);
		     ForceNl();
		     }
		  fall_thru = c_walk(n1, indent + IndentInc, 0 /* brace */);
		  }
	       may_brnchto = 1;
	       return fall_thru;
	    case Goto:
	       prt_tok(t, indent);                 /* goto */
	       PRT_CSTR(" ", indent);
	       c_walk(n->u[0].child, indent, 0);
	       PRT_CSTR(";", indent);
	       ForceNl();
	       return 0;
	    case Return:
	       if (n->u[0].child)
		  no_ret_val = 0;  /* note that return statement has no value */

	       if (op_type == OrdFunc || fnc_ret == RetInt ||
		  fnc_ret == RetDbl) {
		  /*
		   * ordinary C return: ignore C_integer, C_double, and
		   *  C_string qualifiers on return expression (the first
		   *  two may legally occur when fnc_ret is RetInt or RetDbl).
		   */
		  n1 = n->u[0].child;
		  if (n1 && n1->nd_id == PrefxNd && n1->tok) {
		     switch (n1->tok->tok_id) {
			case C_Integer:
			case C_Double:
			case C_String:
			   n1 = n1->u[0].child;
			}
		     }
		  if (ntend) {
		     /*
		      * There are tended variables that must be removed from
		      *  the tended list.
		      */
		     ind = indent;
		     if (!brace) {
			chk_spc();
			PRT_CSTR("{", ind);
			ind += IndentInc;
			}
		     if (does_call(n1)) {
			/*
			 * The return expression contains a function call;
			 *  the variables must remain tended while it is
			 *  computed, so compute it into a temporary variable
			 *  named r_retval.Output a declaration for r_retval;
			 *  its type must match the return type of the C
			 *  function.
			 */
			ForceNl();
			if (op_type == OrdFunc) {
			   just_type(fnc_head->u[0].child, ind, 0);
			   PRT_CSTR(" ", ind);
			   retval_dcltor(fnc_head->u[1].child, ind);
			   PRT_CSTR(";", ind);
			   }
			else if (fnc_ret == RetInt)
			   PRT_CSTR("C_integer r_retval;", ind);
			else    /* fnc_ret == RetDbl */
			   PRT_CSTR("double r_retval;", ind);
			ForceNl();

			/*
			 * Output code to compute the return value, untend
			 *  the variable, then return the value.
			 */
			PRT_CSTR("r_retval = ", ind);
			c_walk(n1, ind + IndentInc, 0);
			PRT_CSTR(";", ind);
			untend(ind);
			ForceNl();
			PRT_CSTR("return r_retval;", ind);
			}
		     else {
			/*
			 * It is safe to untend the variables and return
			 *  the result value directly with a return
			 *  statement.
			 */
			untend(ind);
			ForceNl();
			prt_tok(t, ind);    /* return */
			ForceSpc();
			c_walk(n1, ind, 0);
			PRT_CSTR(";", ind);
			}
		     if (!brace) {
			ForceNl();
			PRT_CSTR("}", ind);
			}
		     ForceNl();
		     }
		  else {
		     /*
		      * There are no tended variable, just output the
		      *  return expression.
		      */
		     prt_tok(t, indent);     /* return */
		     ForceSpc();
		     c_walk(n1, indent, 0);
		     PRT_CSTR(";", indent);
		     }
		  ForceNl();
		  /*
		   * If this is a body function, check the return against
		   *  preceding abstract returns.
		   */
		  if (fnc_ret == RetInt)
		     chkabsret(n->tok, int_typ);
		  else if (fnc_ret == RetDbl)
		     chkabsret(n->tok, real_typ);
		  }
	       else {
		  /*
		   * Return from Icon operation. Indicate that the operation
		   *  returns, compute the value into the result location,
		   *  untend variables if necessary, and return a signal
		   *  if the function requires one.
		   */
		  cur_impl->ret_flag |= DoesRet;
		  ind = indent;
		  if (!brace) {
		     chk_spc();
		     PRT_CSTR("{", indent);
		     ind += IndentInc;
		     ForceNl();
		     }
		  /* TODO: ret_value() has the rules needed to know if a
		   * brace will be needed or not, together with 'ntend'
		   * (due to 'untend') and 'brace', use this to selectively
		   * output braces or not
		   */
		  ret_value(t, n->u[0].child, ind);
		  if (ntend)
		     untend(ind);
		  ForceNl();
		  if (fnc_ret == RetSig)
		     PRT_CSTR("return A_Continue;", ind);
		  else if (fnc_ret == RetNoVal)
		     PRT_CSTR("return;", ind);
		  ForceNl();
		  if (!brace) {
		     PRT_CSTR("}", ind);
		     ForceNl();
		     }
		  }
	       return 0;
	    case Suspend:
	       if (op_type == OrdFunc)
		  errt1(t, "'suspend' may not be used in an ordinary C function"
		     );
	       cur_impl->ret_flag |= DoesSusp; /* note suspension */
	       ForceNl();
	       ind = indent;
	       if (!brace) { /* already brace? */
		  PRT_CSTR("{", ind);
		  ind += IndentInc;
		  }
	       ForceNl();
	       PRT_CSTR("int signal;", ind);
	       ForceNl();
	       ret_value(t, n->u[0].child, ind);
	       ForceNl();
	       /*
		* The operator suspends by calling the success continuation
		*  if there is one or just returns if there is none. For
		*  the interpreter, interp() is the success continuation.
		*  A non-A_Resume signal from the success continuation must
		*  returned to the caller. If there are tended variables
		*  they must be removed from the tended list before a signal
		*  is returned.
		*/
	       if (iconx_flg) {
		  PRT_STR(
		     "if ((signal = interp(G_Csusp, r_args)) != A_Resume) {",
			ind);
		  }
	       else {
		  PRT_CSTR("if (r_s_cont == (continuation)NULL) {", ind);
		  if (ntend)
		     untend(ind + IndentInc);
		  ForceNl();
		  PRT_CSTR("return A_Continue;", ind + IndentInc);
		  ForceNl();
		  PRT_CSTR("}", ind);
		  ForceNl();
		  PRT_CSTR("else if ((signal = (*r_s_cont)()) != A_Resume) {",
		     ind);
		  }
	       ForceNl();
	       if (ntend)
		  untend(ind + IndentInc);
	       ForceNl();
	       PRT_CSTR("return signal;", ind + IndentInc);
	       ForceNl();
	       PRT_CSTR("}", ind + IndentInc);
	       ForceNl();
	       if (!brace) {
		  PRT_CSTR("}", ind);
		  ForceNl();
		  }
	       return 1;
	    case '(':
	       /*
		* Parenthesized expression.
		*/
	       prt_tok(t, indent);     /* ( */
	       fall_thru = c_walk(n->u[0].child, indent, 0);
	       PRT_CSTR(")", indent);
	       return fall_thru;
	    default:
	       /*
		* All other prefix expressions are printed as the token
		*  image of the operation followed by the operand.
		*/
	       PRT_TOK(t, indent);
	       c_walk(n->u[0].child, indent, 0);
	       return 1;
	    }
      case PstfxNd: /* a postfix expression */
	 /*
	  * All postfix expressions are printed as the operand followed
	  *  by the token image of the operation.
	  */
	 chk_spc();
	 fall_thru = c_walk(n->u[0].child, indent, 0);
	 prt_tok(t, indent);
	 if (t->tok_id == ';')
	    ForceNl();
	 return fall_thru;
      case PreSpcNd: /* prefix expression that needs a space after it */
	 /*
	  * This prefix expression (pointer indication in a declaration) needs
	  *  a space after it.
	  */
	 prt_tok(t, indent);
	 c_walk(n->u[0].child, indent, 0);
	 PRT_CSTR(" ", indent);
	 return 1;
      case SymNd: /* a symbol (identifier) node */
	 /*
	  * Identifier.
	  */
	 chk_spc();
	 prt_var(n, indent, 0 /* is_lvalue */);
	 return 1;
      case BinryNd: /* a binary expression (not necessarily infix) */
	 chk_spc();
	 switch (t->tok_id) {
	    case '[':
	       /*
		* subscripting expression or declaration: <expr> [ <expr> ]
		*/
	       c_walk(n->u[0].child, indent, 0);
	       PRT_CSTR("[", indent);
	       c_walk(n->u[1].child, indent, 0);
	       PRT_CSTR("]", indent);
	       return 1;
	    case '(':
	       /*
		* cast: ( <type> ) <expr>
		*/
	       prt_tok(t, indent);  /* ) */
	       c_walk(n->u[0].child, indent, 0);
	       PRT_CSTR(")", indent);
	       c_walk(n->u[1].child, indent, 0);
	       return 1;
	    case ')':
	       /*
		* function call or declaration: <expr> ( <expr-list> )
		*/
	       /* TODO: use nav_t? nav_n_t? */
	       if (n->u[0].child->tok->tok_id == PassThru) {
		  return c_walk(n->u[1].child, indent, 0);
		  }
	       n2 = NULL;
	       c_walk(n->u[0].child, indent, 0);
	       PRT_CSTR("(", indent);
	       if ((n1 = is_t(n->u[0].child, PrimryNd, Identifier)) &&
		     n1->tok && n1->tok->image == g_str___ASM__) {
		  if ((n2 = is_t(n->u[1].child, PrefxNd, PassThru))) {
		     ForceNl();
		     }
		  }
	       if ((n1 = is_t(n->u[1].child, CommaNd, ','))) {
		  int counter = 1;
		  c_walk_comma(n1->u[0].child, NULL, indent + IndentInc,
		     brace, may_force_nl, &counter, WHEN_NL_ARG_LIST);
		  c_walk_comma(n1->u[1].child, n1->tok, indent + IndentInc,
		     brace, may_force_nl, &counter, WHEN_NL_ARG_LIST);
		  }
	       else
		  c_walk(n->u[1].child, indent, 0);
	       if (n2)
		  ForceNl();
	       prt_tok(t, indent + IndentInc);   /* ) */
	       return call_ret(n->u[0].child);
	    case Struct:
	    case Union:
	       /*
		* struct/union <ident>
		* struct/union <opt-ident> { <field-list> }
		*/
	       prt_tok(t, indent);   /* struct or union */
	       if (n->u[0].child) {
		  PRT_CSTR(" ", indent);
		  c_walk(n->u[0].child, indent, 0);
		  }
	       if (n->u[1].child != NULL) {
		  /*
		   * Field declaration list.
		   */
		  PRT_CSTR(" {", indent);
		  ForceNl();
		  c_walk_struct_items(n->u[1].child, indent + IndentInc,
		     0, may_force_nl);
		  ForceNl();
		  PRT_CSTR("}", indent + IndentInc);
		  }
	       return 1;
	    case Enum:
	       /*
		* enum <ident>
		* enum <opt-ident> { <enum-list> }
		*/
	       prt_tok(t, indent);   /* enum */
	       if (n->u[0].child) {
		  PRT_CSTR(" ", indent);
		  c_walk(n->u[0].child, indent, 0);
		  }
	       if (n->u[1].child != NULL) {
		  /*
		   * enumerator list.
		   */
		  PRT_CSTR(" {", indent);
		  ForceNl();

		  if ((n1 = is_t(n->u[1].child, CommaNd, ','))) {
		     int counter = 1;
		     c_walk_comma(n1->u[0].child, NULL, indent + IndentInc,
			brace, may_force_nl, &counter, WHEN_NL_ENUM_LIST);
		     c_walk_comma(n1->u[1].child, n1->tok, indent + IndentInc,
			brace, may_force_nl, &counter, WHEN_NL_ENUM_LIST);
		     }
		  else
		     c_walk(n->u[1].child, indent + IndentInc, 0);

		  ForceNl();
		  PRT_CSTR("}", indent + IndentInc);
		  }
	       return 1;
	    case ';':
	       /*
		* <type-specs> <declarator> ;
		*/
	       if ((n1 = is_t(n->u[1].child, CommaNd, ','))) {
		  /*
		   * a list of declarators
		   */
		  int counter = 1, when_nl = 1;

		  c_walk(n->u[0].child, indent, 0);
		  if (is_ttt(n->u[0].child, BinryNd, Struct, Union, Enum)) {
		     ForceNl();
		     when_nl = WHEN_NL_DECLARATOR_LIST;
		     }
		  else if (is_a(n->u[0].child, PrimryNd)) {
		     PRT_CSTR(" ", indent);
		     when_nl = WHEN_NL_PRIMARY_DECLARATOR_LIST;
		     }
		  else
		     ForceNl();
		  c_walk_comma(n1->u[0].child, NULL, indent + IndentInc,
		     brace, may_force_nl, &counter, when_nl);
		  c_walk_comma(n1->u[1].child, n1->tok, indent + IndentInc,
		     brace, may_force_nl, &counter, when_nl);
		  prt_tok(t, indent);  /* ; */
		  }
	       else {
		  if ((n1 = n->u[0].child))
		     c_walk(n->u[0].child, indent, 0);
		  if ((n2 = n->u[1].child)) {
		     if (n1)
			PRT_CSTR(" ", indent);
		     c_walk(n2, indent, 0);
		     }
		  prt_tok(t, indent);  /* ; */
		  }
	       ForceNl();
	       return 1;
	    case ':':
	       /*
		* <label> : <statement>
		*/
	       c_walk(n->u[0].child, indent, 0);
	       prt_tok(t, indent);   /* : */
	       PRT_CSTR(" ", indent);
	       fall_thru = c_walk(n->u[1].child, indent, 0);
	       may_brnchto = 1;
	       return fall_thru;
	    case Case:
	       /*
		* case <expr> : <statement>
		*/
	       ForceNl();
	       prt_tok(t, indent);
	       PRT_CSTR(" ", indent);
	       c_walk(n->u[0].child, indent, 0);
	       n1 = n->u[1].child;
	       if (is_t(n1, BinryNd, Case) || is_t(n1, PrefxNd, Default)) {
		  PRT_CSTR(":", indent);
		  fall_thru = c_walk(n1, indent, 0 /* brace */);
		  }
	       else {
		  if (is_t(n1, CompNd, '{'))
		     PRT_CSTR(": ", indent);
		  else {
		     PRT_CSTR(":", indent);
		     ForceNl();
		     }
		  fall_thru = c_walk(n1, indent + IndentInc, 0 /* brace */);
		  }
	       may_brnchto = 1;
	       return fall_thru;
	    case Switch:
	       /*
		* switch ( <expr> ) <statement>
		*/
	       found_switch = 1;
	       prt_tok(t, indent);  /* switch */
	       PRT_CSTR(" (", indent);
	       c_walk(n->u[0].child, indent, 0);
	       PRT_CSTR(")", indent);
	       ForceSpc();
	       save_break = does_break;
	       fall_thru = c_walk(n->u[1].child, indent, 0);
	       fall_thru |= does_break;
	       does_break = save_break;
	       return fall_thru;
	    case While: {
	       struct node *n0;
	       /*
		* while ( <expr> ) <statement>
		*/
	       ind = indent;
	       n0 = n->u[0].child;
	       prt_tok(t, ind);  /* while */
	       PRT_CSTR(" (", ind);
	       c_walk(n0, ind, 0);
	       ind += prt_closing_parenthesis(n->u[1].child, ind);
	       save_break = does_break;
	       c_walk(n->u[1].child, ind, 0);
	       /*
		* check for an infinite loop, while (1) ... :
		*  a condition consisting of an IntConst with image=="1"
		*  and no breaks in the body.
		*/
	       if (n0->nd_id == PrimryNd && n0->tok->tok_id == IntConst &&
		   !strcmp(n0->tok->image,"1") && !does_break)
		  fall_thru = 0;
	       else
		  fall_thru = 1;
	       does_break = save_break;
	       return fall_thru;
	       }
	    case Do:
	       /*
		* do <statement> <while> ( <expr> )
		*/
	       ind = indent;
	       prt_tok(t, ind);  /* do */
	       ForceSpc();
	       n1 = n->u[0].child;
	       ind += handle_next_indentation(n1, ind);
	       ForceSpc();
	       c_walk_nl(n1, ind, 0, 0);
	       if (ind == indent) { /* next element has '{' */
		  g_nl = 0;
		  ForceSpc();
		  }
	       chk_spc();
	       PRT_CSTR("while (", ind);
	       save_break = does_break;
	       c_walk(n->u[1].child, ind, 0);
	       does_break = save_break;
	       PRT_CSTR(");", ind);
	       ForceNl();
	       return 1;
	    case '.':
	    case Arrow:
	       /*
		* Field access: <expr> . <expr>  and  <expr> -> <expr>
		*/
	       c_walk(n->u[0].child, indent, 0);
	       prt_tok(t, indent);   /* . or -> */
	       c_walk(n->u[1].child, indent, 0);
	       return 1;
	    case Runerr:
	       /*
		* runerr ( <error-number> )
		* runerr ( <error-number> , <offending-value> )
		*/
	       prt_runerr(t, n->u[0].child, n->u[1].child, indent, brace);
	       return 0;
	    case Is:
	       /*
		* is : <type> ( <expr> )
		*/
	       typ_asrt(icn_typ(n->u[0].child), n->u[1].child,
		  n->u[0].child->tok, indent);
	       return 1;
	    default:
	       /*
		* All other binary expressions are infix notation and
		*  are printed with spaces around the operator.
		*/
	       c_walk(n->u[0].child, indent, 0);
	       PRT_CSTR(" ", indent);
	       prt_tok(t, indent);
	       PRT_CSTR(" ", indent);
	       c_walk(n->u[1].child, indent, 0);
	       return 1;
	    }
      case LstNd: /* list of declaration parts */
	 /*
	  * <declaration-part> <declaration-part>
	  *
	  * Need space between parts
	  */
	 c_walk(n->u[0].child, indent, 0);
	 if (is_t(n->u[1].child, BinryNd, ';') == NULL)
	    PRT_CSTR(" ", indent);
	 c_walk(n->u[1].child, indent, 0);
	 return 1;
      case ConCatNd: /* two ajacent pieces of code with no other syntax */
	 /*
	  * <some-code> <some-code>
	  *
	  * Various lists of code parts that do not need space between them.
	  */
	 PRT_CONCATND(indent);
	 ind = indent;
#if 0
	 if (is_t(n->u[0].child, CompNd, '{'))
	    ind += IndentInc;
#endif
	 if (c_walk(n->u[0].child, ind, 0 /* brace */)) {
	    PRT_CONCATND(indent);
	    /*fprintf(g_out_file, "/""*%d,%s*""/\n", __LINE__, node_name(n->u[1].child));*/
	    fall_thru = c_walk(n->u[1].child, ind, 0 /* brace */);
	    PRT_CONCATND(indent);
	    return fall_thru;
	    }
	 /*
	  * Cannot directly reach the second piece of code, see if
	  *  it is possible to branch into it.
	  */
	 may_brnchto = 0;
	 PRT_CONCATND(indent);
	 fall_thru = c_walk(n->u[1].child, ind, 0);
	 PRT_CONCATND(indent);
	 return may_brnchto & fall_thru;
      case StrDclNd: /* structure field declaration */
	 /*
	  * Structure field declaration. Bit field declarations have
	  *  a semicolon and a field width.
	  */
	 c_walk(n->u[0].child, indent, 0);
	 if (n->u[1].child != NULL) {
	    PRT_CSTR(": ", indent);
	    c_walk(n->u[1].child, indent, 0);
	    }
	 return 1;
      case CompNd: /* compound statement */
	 /*
	  * Compound statement.
	  */
	 ind = indent;
	 if (!brace) {
	    chk_spc();
	    PRT_TOK(t, indent);  /* { */
	    ind += IndentInc;
	    }
	 ForceNl();
	 c_walk(n->u[0].child, ind, 1 /* brace */);
	 /*
	  * we are in an inner block. tended locations may need to
	  *  be set to values from declaration initializations.
	  */
	 for (sym = n->u[1].sym; sym != NULL; sym = sym->u.tnd_var.next) {
	    if (sym->u.tnd_var.init != NULL) {
	       prt_str(tendstrct, IndentInc);
	       fprintf(g_out_file, ".d[%d]", sym->t_indx);
	       switch (sym->id_type) {
		  case TndDesc:
		     PRT_CSTR(" = ", IndentInc);
		     break;
		  case TndStr:
		     PRT_CSTR(".vword.sptr = (char*)", IndentInc);
		     break;
		  case TndBlk:
		     PRT_CSTR(".vword.ptr = ",
			IndentInc);
		     break;
		  }
	       c_walk(sym->u.tnd_var.init, 2 * IndentInc, 0);
	       PRT_CSTR(";", 2 * IndentInc);
	       ForceNl();
	       }
	    }
#if 1
	 /*
	  * If there are no declarations, suppress braces that
	  *  may be required for a one-statement body; we already
	  *  have a set.
	  */
	 {
	    int brace1 = n->u[0].child == NULL && n->u[1].sym == NULL;
	    if (found_switch) {
	       found_switch = 0;
	       fall_thru = c_walk_cat(n->u[2].child, ind, brace1);
	       }
	    else
	       fall_thru = c_walk(n->u[2].child, ind, brace1);
	    }
#else
	 if (found_switch) {
	    found_switch = 0;
	    fall_thru = c_walk_cat(n->u[2].child, ind, 1);
	    }
	 else
	    fall_thru = c_walk(n->u[2].child, ind, 1);
#endif

	 if (!brace) {
	    ForceNl();
	    PRT_CSTR("}", ind);
	    }
	 if (may_force_nl)
	    ForceNl();
	 return fall_thru;
      case TrnryNd: /* an expression with 3 subexpressions */
	 switch (t->tok_id) {
	    case '?':
	       /*
		* <expr> ? <expr> : <expr>
		*/
	       c_walk(n->u[0].child, indent, 0);
	       PRT_CSTR(" ", indent);
	       prt_tok(t, indent);  /* ? */
	       PRT_CSTR(" ", indent);
	       c_walk(n->u[1].child, indent, 0);
	       PRT_CSTR(" : ", indent);
	       c_walk(n->u[2].child, indent, 0);
	       return 1;
	    case If:
	       /*
		* if ( <expr> ) <statement>
		* if ( <expr> ) <statement> else <statement>
		*/
	       ind = indent;
	       chk_nl(ind);
	       chk_spc();
	       PRT_TOK(t, ind);  /* if */
	       PRT_CSTR(" (", ind);
	       c_walk(n->u[0].child, ind, 0);
	       ind += prt_closing_parenthesis(n1 = n->u[1].child, ind);
	       fall_thru = c_walk(n1, ind, 0 /* brace */);
	       if (is_t(n1, PstfxNd, ';'))
		  ForceNl();
	       if ((n1 = n->u[2].child)) { /* there is an 'else' */
		  ind = indent;
		  ForceNl();
		  ind += prt_else(n1, ind);
		  fall_thru |= c_walk(n1, ind, 0 /* brace */);
		  }
	       else
		  fall_thru = 1;
	       return fall_thru;
	    case Type_case:
	       /*
		* type_case <expr> of { <section-list> }
		* type_case <expr> of { <section-list> <default-clause> }
		*/
	       return typ_case(n->u[0].child, n->u[1].child, n->u[2].child,
		  c_walk, 1, indent);
	    case Cnv:
	       /*
		* cnv : <type> ( <source> , <destination> )
		*/
	       cnv_fnc(t, icn_typ(n->u[0].child), n->u[1].child, NULL,
		  n->u[2].child,
		  indent);
	       return 1;
	    }
      case QuadNd: /* an expression with 4 subexpressions */
	 switch (t->tok_id) {
	    case For:
	       /*
		* for ( <expr> ; <expr> ; <expr> ) <statement>
		*/
	       ind = indent;
	       PRT_TOK(t, ind);  /* for */
	       PRT_CSTR(" (", ind);
	       c_walk(n->u[0].child, ind, 0);
	       PRT_CSTR(";", ind);
	       ForceSpc();
	       c_walk(n->u[1].child, ind, 0);
	       PRT_CSTR(";", ind);
	       ForceSpc();
	       c_walk(n->u[2].child, ind, 0);
	       save_break = does_break;
	       n1 = n->u[3].child;
	       ind += prt_closing_parenthesis(n1, ind);
	       c_walk(n1, ind, 0 /* brace */);
	       if (is_t(n1, PstfxNd, ';'))
		  ForceNl();
	       if (n->u[1].child == NULL && !does_break)
		  fall_thru = 0;
	       else
		  fall_thru = 1;
	       does_break = save_break;
	       return fall_thru;
	    case Def:
	       /*
		* def : <type> ( <source> , <default> , <destination> )
		*/
	       cnv_fnc(t, icn_typ(n->u[0].child), n->u[1].child, n->u[2].child,
		  n->u[3].child, indent);
	       return 1;
	    }
      case LValueNd:
	 n1 = n->u[0].child;
	 if (n1->nd_id == SymNd || n1->nd_id == CompNd)
	    prt_var(n1, indent, 1 /* is_lvalue */);
	 else
	    c_walk(n1, indent, 0);
	 return 1;
      case CommaNd:
	 /*
	  *  see usages of 'c_walk_comma'
	  */
	 fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
	    __FILE__, __LINE__, g_fname_for___FILE__,
	    g_line_for___LINE__);
	 exit(1);
      default:
	 fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
	    __FILE__, __LINE__, g_fname_for___FILE__,
	    g_line_for___LINE__);
	 exit(1);
      }
   /*NOTREACHED*/
   return 0;			/* avoid gcc warning */
   }

/*
 * call_ret - decide whether a function being called might return.
 */
int call_ret(n)
struct node *n;
   {
   /*
    * Assume functions return except for c_exit(), fatalerr(), and syserr().
    */
   if (n->tok != NULL &&
      (strcmp("c_exit",   n->tok->image) == 0 ||
       strcmp("fatalerr", n->tok->image) == 0 ||
       strcmp("syserr",   n->tok->image) == 0))
      return 0;
   else
      return 1;
   }

/*
 * new_prmloc - allocate an array large enough to hold a flag for every
 *  parameter of the current operation. This flag indicates where
 *  the parameter is in terms of scopes created by conversions.
 */
struct parminfo *new_prmloc()
   {
   struct parminfo *parminfo;
   int nparams;
   int i;

   if (g_params == NULL)
      return NULL;
   nparams = g_params->u.param_info.param_num + 1;
   parminfo = alloc(nparams * sizeof(struct parminfo));
   for (i = 0; i < nparams; ++i) {
      parminfo[i].cur_loc = 0;
      parminfo [i].parm_mod = 0;
      }
   return parminfo;
   }

/*
 * ld_prmloc - load parameter location information that has been
 *  saved in an arrary into the symbol table.
 */
void ld_prmloc(parminfo)
struct parminfo *parminfo;
   {
   struct sym_entry *sym;
   int param_num;

   for (sym = g_params; sym != NULL; sym = sym->u.param_info.next) {
      param_num = sym->u.param_info.param_num;
      if (sym->id_type & DrfPrm) {
	 sym->u.param_info.cur_loc = parminfo[param_num].cur_loc;
	 sym->u.param_info.parm_mod = parminfo[param_num].parm_mod;
	 }
      }
   }

/*
 * sv_prmloc - save parameter location information from the the symbol table
 *  into an array.
 */
void sv_prmloc(parminfo)
struct parminfo *parminfo;
   {
   struct sym_entry *sym;
   int param_num;

   for (sym = g_params; sym != NULL; sym = sym->u.param_info.next) {
      param_num = sym->u.param_info.param_num;
      if (sym->id_type & DrfPrm) {
	 parminfo[param_num].cur_loc = sym->u.param_info.cur_loc;
	 parminfo[param_num].parm_mod = sym->u.param_info.parm_mod;
	 }
      }
   }

/*
 * mrg_prmloc - merge parameter location information in the symbol table
 *  with other information already saved in an array. This may result
 *  in conflicting location information, but conflicts are only detected
 *  when a parameter is actually used.
 */
void mrg_prmloc(parminfo)
struct parminfo *parminfo;
   {
   struct sym_entry *sym;
   int param_num;

   for (sym = g_params; sym != NULL; sym = sym->u.param_info.next) {
      param_num = sym->u.param_info.param_num;
      if (sym->id_type & DrfPrm) {
	 parminfo[param_num].cur_loc |= sym->u.param_info.cur_loc;
	 parminfo[param_num].parm_mod |= sym->u.param_info.parm_mod;
	 }
      }
   }

/*
 * clr_prmloc - indicate that this execution path contributes nothing
 *   to the location of parameters.
 */
void clr_prmloc()
   {
   struct sym_entry *sym;

   for (sym = g_params; sym != NULL; sym = sym->u.param_info.next) {
      if (sym->id_type & DrfPrm) {
	 sym->u.param_info.cur_loc = 0;
	 sym->u.param_info.parm_mod = 0;
	 }
      }
   }

/*
 * typ_case - translate a type_case statement into C. This is called
 *  while walking a syntax tree of either RTL code or C code; the parameter
 *  "walk" is a function used to process the subtrees within the type_case
 *  statement.
 */
static int typ_case(var, slct_lst, dflt, walk, maybe_var, indent)
struct node *var, *slct_lst, *dflt;
int (*walk)(struct node *n, const int xindent, const int brace);
int maybe_var, indent;
   {
   struct node *lst, *select, *slctor;
   struct parminfo *strt_prms, *end_prms;
   int remaining, first, fnd_slctrs;
   int maybe_str = 1;
   int dflt_lbl = -1;
   int typcd, fall_thru;
   char *s;

   /*
    * This statement involves multiple paths that may establish new
    *  scopes for parameters. Remember the starting scope information
    *  and initialize an array in which to compute the final information.
    */
   strt_prms = new_prmloc();
   sv_prmloc(strt_prms);
   end_prms = new_prmloc();

   /*
    * First look for cases that must be checked with "if" statements.
    *  These include string qualifiers and variables.
    */
   remaining = 0;      /* number of cases skipped in first pass */
   first = 1;          /* next case to be output is the first */
   if (dflt == NULL)
      fall_thru = 1;
   else
      fall_thru = 0;
   for (lst = slct_lst; lst != NULL; lst = lst->u[0].child) {
      select = lst->u[1].child;
      fnd_slctrs = 0; /* flag: found type selections for clause for this pass */
      /*
       * A selection clause may include several types.
       */
      for (slctor = select->u[0].child; slctor != NULL; slctor =
	slctor->u[0].child) {
	 typcd = icn_typ(slctor->u[1].child);
	 if(typ_name(typcd, slctor->u[1].child->tok) == NULL) {
	    /*
	     * This type must be checked with the "if". Is this the
	     *  first condition checked for this clause? Is this the
	     *  first clause output?
	     */
	    if (fnd_slctrs)
	       PRT_CSTR(" || ", indent);
	    else {
	       ForceNl();
	       if (first)
		  first = 0;
	       else {
		  PRT_CSTR("else ", indent);
		  }
	       PRT_CSTR("if (", indent);
	       fnd_slctrs = 1;
	       }

	    /*
	     * Output type check
	     */
	    typ_asrt(typcd, var, slctor->u[1].child->tok, indent + IndentInc);

	    if (typcd == str_typ)
	       maybe_str = 0;  /* string has been taken care of */
	    else if (typcd == Variable)
	       maybe_var = 0;  /* variable has been taken care of */
	    }
	 else
	    ++remaining;
	 }
      if (fnd_slctrs) {
	 /*
	  * We have found and output type selections for this clause;
	  *  output the body of the clause. Remember any changes to
	  *  paramter locations caused by type conversions within the
	  *  clause.
	  */
	 PRT_CSTR(") {", indent + IndentInc);
	 ForceNl();
	 if ((*walk)(select->u[1].child, indent + IndentInc, 1)) {
	    fall_thru |= 1;
	    mrg_prmloc(end_prms);
	    }
	 PRT_CSTR("}", indent + IndentInc);
	 ForceNl();
	 ld_prmloc(strt_prms);
	 }
      }
   /*
    * The rest of the cases can be checked with a "switch" statement, look
    *  for them..
    */
   if (remaining == 0) {
      if (dflt != NULL) {
	 /*
	  * There are no cases to handle with a switch statement, but there
	  *  is a default clause; handle it with an "else".
	  */
	 PRT_CSTR("else {", indent);
	 ForceNl();
	 fall_thru |= (*walk)(dflt, indent + IndentInc, 1);
	 ForceNl();
	 PRT_CSTR("}", indent + IndentInc);
	 ForceNl();
	 }
      }
   else {
      /*
       * If an "if" statement was output, the "switch" must be in its "else"
       *   clause.
       */
      if (!first)
	 PRT_CSTR("else ", indent);

      /*
       * A switch statement cannot handle types that are not simple type
       *  codes. If these have not taken care of, output code to check them.
       *  This will either branch around the switch statement or into
       *  its default clause.
       */
      if (maybe_str || maybe_var) {
	 dflt_lbl = g_lbl_num++;      /* allocate a label number */
	 PRT_CSTR("{", indent);
	 ForceNl();
	 PRT_CSTR("if (((", indent);
	 c_walk(var, indent + IndentInc, 0);
	 PRT_CSTR(").dword & D_Typecode) != D_Typecode) ", indent);
	 ForceNl();
	 chk_nl(indent + IndentInc);
	 fprintf(g_out_file, "goto L%d;  /* default */ ", dflt_lbl);
	 ForceNl();
	 }

      no_nl = 1; /* suppress #line directives */
      PRT_CSTR("switch (Type(", indent);
      c_walk(var, indent + IndentInc, 0);
      PRT_CSTR(")) {", indent + IndentInc);
      no_nl = 0;
      ForceNl();

      /*
       * Loop through the case clauses producing code for them.
       */
      for (lst = slct_lst; lst != NULL; lst = lst->u[0].child) {
	 select = lst->u[1].child;
	 fnd_slctrs = 0;
	 /*
	  * A selection clause may include several types.
	  */
	 for (slctor = select->u[0].child; slctor != NULL; slctor =
	   slctor->u[0].child) {
	    typcd = icn_typ(slctor->u[1].child);
	    s = typ_name(typcd, slctor->u[1].child->tok);
	    if (s != NULL) {
	       /*
		* A type selection has been found that can be checked
		*  in the switch statement. Note that large integers
		*  require special handling.
		*/
	       fnd_slctrs = 1;

	       if (typcd == int_typ) {
		 ForceNl();
		 PRT_CSTR("case T_Lrgint: ", indent + IndentInc);
		 ForceNl();
	       }

	       chk_nl(indent + IndentInc);
	       fprintf(g_out_file, "case T_%s: ", s);
	       }
	    }
	 if (fnd_slctrs) {
	    /*
	     * We have found and output type selections for this clause;
	     *  output the body of the clause. Remember any changes to
	     *  paramter locations caused by type conversions within the
	     *  clause.
	     */
	    ForceNl();
	    if ((*walk)(select->u[1].child, indent + 2 * IndentInc, 0)) {
	       fall_thru |= 1;
	       ForceNl();
	       PRT_CSTR("break;", indent + 2 * IndentInc);
	       mrg_prmloc(end_prms);
	       }
	    ForceNl();
	    ld_prmloc(strt_prms);
	    }
	 }
      if (dflt != NULL) {
	 /*
	  * This type_case statement has a default clause. If there is
	  *  a branch into this clause, output the label. Remember any
	  *  changes to paramter locations caused by type conversions
	  *  within the clause.
	  */
	 ForceNl();
	 PRT_CSTR("default:", indent + 1 * IndentInc);
	 ForceNl();
	 if (maybe_str || maybe_var) {
	    chk_nl(0);
	    fprintf(g_out_file, "L%d: ; /* default */", dflt_lbl);
	    ForceNl();
	    }
	 if ((*walk)(dflt, indent + 2 * IndentInc, 0)) {
	    fall_thru |= 1;
	    mrg_prmloc(end_prms);
	    }
	 ForceNl();
	 ld_prmloc(strt_prms);
	 }
      PRT_CSTR("}", indent + IndentInc);

      if (maybe_str || maybe_var) {
	 if (dflt == NULL) {
	    /*
	     * There is a branch around the switch statement. Output
	     *  the label.
	     */
	    ForceNl();
	    chk_nl(0);
	    fprintf(g_out_file, "L%d: ; /* default */", dflt_lbl);
	    }
	 ForceNl();
	 PRT_CSTR("}", indent + IndentInc);
	 }
      ForceNl();
      }

   /*
    * Put ending parameter locations into effect.
    */
   mrg_prmloc(end_prms);
   ld_prmloc(end_prms);
   if (strt_prms != NULL)
      free(strt_prms);
   if (end_prms != NULL)
      free(end_prms);
   return fall_thru;
   }

/*
 * chk_conj - see if the left argument of a conjunction is an in-place
 *   conversion of a parameter other than a conversion to C_integer or
 *   C_double. If so issue a warning.
 */
static void chk_conj(n)
struct node *n;
   {
   struct node *cnv_type, *src, *dest;
   int typcd;

   if (n->nd_id == BinryNd && n->tok->tok_id == And)
      n = n->u[1].child;

   switch (n->nd_id) {
      case TrnryNd:
	 /*
	  * Must be Cnv.
	  */
	 cnv_type = n->u[0].child;
	 src = n->u[1].child;
	 dest = n->u[2].child;
	 break;
      case QuadNd:
	 /*
	  * Must be Def.
	  */
	 cnv_type = n->u[0].child;
	 src = n->u[1].child;
	 dest = n->u[3].child;
	 break;
      default:
	 return;   /* not a  conversion */
      }

   /*
    * A conversion has been found. See if it meets the criteria for
    *  issuing a warning.
    */

   if (src->nd_id != SymNd || !(src->u[0].sym->id_type & DrfPrm))
      return;  /* not a dereferenced parameter */

   typcd = icn_typ(cnv_type);
   switch (typcd) {
      case TypCInt:
      case TypCDbl:
      case TypECInt:
	 return;
      }

   if (dest != NULL)
      return;   /* not an in-place convertion */

   fprintf(stderr,
    "%s: file %s, line %d, warning: in-place conversion may or may not be\n",
      g_progname, cnv_type->tok->fname, cnv_type->tok->line);
   fprintf(stderr, "\tundone on subsequent failure.\n");
   }

/*
 * len_sel - translate a clause form a len_case statement into a C case
 *  clause. Return an indication of whether execution falls through the
 *  clause.
 */
static int len_sel(sel, strt_prms, end_prms, indent)
struct node *sel;
struct parminfo *strt_prms, *end_prms;
int indent;
   {
   int fall_thru;

   PRT_CSTR("case ", indent);
   prt_tok(sel->tok, indent + IndentInc);           /* integer selection */
   PRT_CSTR(":", indent + IndentInc);
   fall_thru = rt_walk(sel->u[0].child, indent + IndentInc, 0 /* brace */);/* clause body */
   ForceNl();

   if (fall_thru) {
      PRT_CSTR("break;", indent + IndentInc);
      ForceNl();
      /*
       * Remember any changes to paramter locations caused by type conversions
       *  within the clause.
       */
      mrg_prmloc(end_prms);
      }

   ld_prmloc(strt_prms);
   return fall_thru;
   }

/*
 * rt_walk - walk the part of the syntax tree containing rtt code, producing
 *   code for the most-general version of the routine.
 */
static int rt_walk(n, indent, brace)
struct node *n;
const int indent, brace;
   {
   struct token *t, *t1;
   struct node *n1, *errnum;
   int fall_thru;

   if (n == NULL)
      return 1;

   t = n->tok;

   switch (n->nd_id) {
      case PrefxNd:
	 switch (t->tok_id) {
	    case '{':
	       /*
		* RTL code: { <actions> }
		*/
#if 1
	       if (!brace) {
		  chk_spc();
		  PRT_TOK(t, indent);  /* { */
		  }
	       fall_thru = rt_walk(n->u[0].child, indent, 1 /* brace */);
	       if (!brace)
		  PRT_CSTR("}", indent);
#else
	       fall_thru = rt_walk(n->u[0].child, indent, 0 /* brace */);
#endif
	       return fall_thru;
	    case '!':
	       /*
		* RTL type-checking and conversions: ! <simple-type-check>
		*/
	       prt_tok(t, indent);
	       rt_walk(n->u[0].child, indent, 0 /* brace */);
	       return 1;
	    case Body:
	    case Inline:
	       /*
		* RTL code: body { <c-code> }
		*           inline { <c-code> }
		*/
	       fall_thru = c_walk(n->u[0].child, indent, brace);
	       if (!fall_thru)
		  clr_prmloc();
	       return fall_thru;
	    }
	 break;
      case BinryNd:
	 switch (t->tok_id) {
	    case Runerr:
	       /*
		* RTL code: runerr( <message-number> )
		*           runerr( <message-number>, <descriptor> )
		*/
	       prt_runerr(t, n->u[0].child, n->u[1].child, indent, brace);

	       /*
		* Execution cannot continue on this execution path.
		*/
	       clr_prmloc();
	       return 0;
	    case And:
	       /*
		* RTL type-checking and conversions:
		*   <type-check> && <type_check>
		*/
	       chk_conj(n->u[0].child);  /* is a warning needed? */
	       rt_walk(n->u[0].child, indent, 0 /* brace */);
	       PRT_CSTR(" ", indent);
	       prt_tok(t, indent);       /* && */
	       PRT_CSTR(" ", indent);
	       rt_walk(n->u[1].child, indent, 0 /* brace */);
	       return 1;
	    case Is:
	       /*
		* RTL type-checking and conversions:
		*   is: <icon-type> ( <variable> )
		*/
	       typ_asrt(icn_typ(n->u[0].child), n->u[1].child,
		  n->u[0].child->tok, indent);
	       return 1;
	    }
	 break;
      case ConCatNd:
	 /*
	  * "Glue" for two constructs.
	  */
	 fall_thru = rt_walk(n->u[0].child, indent, 0);
	 return fall_thru & rt_walk(n->u[1].child, indent, 0);
      case AbstrNd:
	 /*
	  * Ignore abstract type computations while producing C code
	  *  for library routines.
	  */
	 return 1;
      case TrnryNd:
	 switch (t->tok_id) {
	    case If: {
	       /*
		* RTL code for "if" statements:
		*  if <type-check> then <action>
		*  if <type-check> then <action> else <action>
		*
		*  <type-check> may include parameter conversions that create
		*  new scoping. It is necessary to keep track of paramter
		*  types and locations along success and failure paths of
		*  these conversions. The "then" and "else" actions may
		*  also establish new scopes.
		*/
	       struct parminfo *then_prms = NULL;
	       struct parminfo *else_prms;

	       /*
		* Save the current parameter locations. These are in
		*  effect on the failure path of any type conversions
		*  in the condition of the "if".
		*/
	       else_prms = new_prmloc();
	       sv_prmloc(else_prms);

	       ForceNl();
	       PRT_TOK(t, indent);       /* if */
	       PRT_CSTR(" (", indent);
	       n1 = n->u[0].child;
	       rt_walk(n1, indent + IndentInc, 0 /* brace */);   /* type check */
	       PRT_CSTR(") {", indent);
	       ForceNl();

	       /*
		* If the condition is negated, the failure path is to the "then"
		*  and the success path is to the "else".
		*/
	       if (n1->nd_id == PrefxNd && n1->tok->tok_id == '!') {
		  then_prms = else_prms;
		  else_prms = new_prmloc();
		  sv_prmloc(else_prms);
		  ld_prmloc(then_prms);
		  }

	       /*
		* Then Clause.
		*/
	       fall_thru = rt_walk(n->u[1].child, indent + IndentInc, 1 /* brace */);
	       ForceNl();
	       PRT_CSTR("}", indent + IndentInc);

	       /*
		* Determine if there is an else clause and merge parameter
		*  location information from the alternate paths through
		*  the statement.
		*/
	       n1 = n->u[2].child;
	       if (n1 == NULL) {
		  if (fall_thru)
		     mrg_prmloc(else_prms);
		  ld_prmloc(else_prms);
		  fall_thru = 1;
		  }
	       else {
		  if (then_prms == NULL)
		     then_prms = new_prmloc();
		  if (fall_thru)
		     sv_prmloc(then_prms);
		  ld_prmloc(else_prms);
		  ForceNl();
		  PRT_CSTR("else {", indent);
		  if (rt_walk(n1, indent + IndentInc, 1 /* brace */)) {  /* else clause */
		     fall_thru = 1;
		     mrg_prmloc(then_prms);
		     }
		  ForceNl();
		  PRT_CSTR("}", indent + IndentInc);
		  ld_prmloc(then_prms);
		  }
	       ForceNl();
	       if (then_prms != NULL)
		  free(then_prms);
	       if (else_prms != NULL)
		  free(else_prms);
	       }
	       return fall_thru;
	    case Len_case: {
	       /*
		* RTL code:
		*   len_case <variable> of {
		*      <integer>: <action>
		*        ...
		*      default: <action>
		*      }
		*/
	       struct parminfo *strt_prms;
	       struct parminfo *end_prms;

	       /*
		* A case may contain parameter conversions that create new
		*  scopes. Remember the parameter locations at the start
		*  of the len_case statement.
		*/
	       strt_prms = new_prmloc();
	       sv_prmloc(strt_prms);
	       end_prms = new_prmloc();

	       n1 = n->u[0].child;
	       if (!(n1->u[0].sym->id_type & VArgLen))
		  errt1(t, "len_case must select on length of vararg");
	       /*
		* The len_case statement is implemented as a C switch
		*  statement.
		*/
	       PRT_CSTR("switch (", indent);
	       prt_var(n1, indent, 0 /* is_lvalue */);
	       PRT_CSTR(") {", indent);
	       ForceNl();
	       fall_thru = 0;
	       for (n1 = n->u[1].child; n1->nd_id == ConCatNd;
		  n1 = n1->u[0].child)
		     fall_thru |= len_sel(n1->u[1].child, strt_prms, end_prms,
			indent + IndentInc);
	       fall_thru |= len_sel(n1, strt_prms, end_prms,
		  indent + IndentInc);

	       /*
		* Handle default clause.
		*/
	       PRT_CSTR("default:", indent + IndentInc);
	       ForceNl();
	       fall_thru |= rt_walk(n->u[2].child, indent + 2 * IndentInc, 0 /* brace */);
	       ForceNl();
	       PRT_CSTR("}", indent + IndentInc);
	       ForceNl();

	       /*
		* Put into effect the location of parameters at the end
		*  of the len_case statement.
		*/
	       mrg_prmloc(end_prms);
	       ld_prmloc(end_prms);
	       if (strt_prms != NULL)
		  free(strt_prms);
	       if (end_prms != NULL)
		  free(end_prms);
	       }
	       return fall_thru;
	    case Type_case: {
	       /*
		* RTL code:
		*   type_case <variable> of {
		*       <icon_type> : ... <icon_type> : <action>
		*          ...
		*       }
		*
		*   last clause may be: default: <action>
		*/
	       int maybe_var;
	       struct node *var;
	       struct sym_entry *sym;

	       /*
		* If we can determine that the value being checked is
		*  not a variable reference, we don't have to produce code
		*  to check for that possibility.
		*/
	       maybe_var = 1;
	       var = n->u[0].child;
	       if (var->nd_id == SymNd) {
		  sym = var->u[0].sym;
		  switch(sym->id_type) {
		     case DrfPrm:
		     case OtherDcl:
		     case TndDesc:
		     case TndStr:
		     case RsltLoc:
			if (sym->nest_lvl > 1) {
			   /*
			    * The thing being tested is either a
			    *  dereferenced parameter or a local
			    *  descriptor which could only have been
			    *  set by a conversion which does not
			    *  produce a variable reference.
			    */
			   maybe_var = 0;
			   }
		      }
		  }
	       return typ_case(var, n->u[1].child, n->u[2].child, rt_walk,
		  maybe_var, indent);
	       }
	    case Cnv:
	       /*
		* RTL code: cnv: <type> ( <source> )
		*           cnv: <type> ( <source> , <destination> )
		*/
	       cnv_fnc(t, icn_typ(n->u[0].child), n->u[1].child, NULL,
		  n->u[2].child, indent);
	       return 1;
	    case Arith_case: {
	       /*
		* arith_case (<variable>, <variable>) of {
		*   C_integer: <statement>
		*   integer: <statement>
		*   C_double: <statement>
		*   }
		*
		* This construct does type conversions and provides
		*  alternate execution paths. It is necessary to keep
		*  track of parameter locations.
		*/
	       struct parminfo *strt_prms;
	       struct parminfo *end_prms;
	       struct parminfo *tmp_prms;

	       strt_prms = new_prmloc();
	       sv_prmloc(strt_prms);
	       end_prms = new_prmloc();
	       tmp_prms = new_prmloc();

	       fall_thru = 0;

	       n1 = n->u[2].child;   /* contains actions for the 3 cases */

	       /*
		* Set up an error number node for use in runerr().
		*/
	       t1 = copy_t(t);
	       t1->tok_id = IntConst;
	       t1->image = "102";
	       errnum = node0(PrimryNd, t1);

	       /*
		* Try converting both arguments to a C_integer.
		*/
	       PRT_CSTR("if (", indent);
	       cnv_fnc(t, TypECInt, n->u[0].child, NULL, NULL, indent);
	       PRT_CSTR(" && ", indent);
	       cnv_fnc(t, TypECInt, n->u[1].child, NULL, NULL, indent);
	       PRT_CSTR(") ", indent);
	       ForceNl();
	       if (rt_walk(n1->u[0].child, indent + IndentInc, 0 /* brace */)) {
		  fall_thru |= 1;
		  mrg_prmloc(end_prms);
		  }
	       ForceNl();

	       /*
		* Try converting both arguments to an integer.
		*/
	       ld_prmloc(strt_prms);
	       PRT_CSTR("else if (", indent);
	       cnv_fnc(t, TypEInt, n->u[0].child, NULL, NULL, indent);
	       PRT_CSTR(" && ", indent);
	       cnv_fnc(t, TypEInt, n->u[1].child, NULL, NULL, indent);
	       PRT_CSTR(") ", indent);
	       ForceNl();
	       if (rt_walk(n1->u[1].child, indent + IndentInc, 0 /* brace */)) {
		  fall_thru |= 1;
		  mrg_prmloc(end_prms);
		  }
	       ForceNl();

	       /*
		* Try converting both arguments to a C_double
		*/
	       ld_prmloc(strt_prms);
	       PRT_CSTR("else {", indent);
	       ForceNl();
	       PRT_CSTR("if (!", indent + IndentInc);
	       cnv_fnc(t, TypCDbl, n->u[0].child, NULL, NULL,
		  indent + IndentInc);
	       PRT_CSTR(")", indent + IndentInc);
	       ForceNl();
	       sv_prmloc(tmp_prms);   /* use original parm locs for error */
	       ld_prmloc(strt_prms);
	       prt_runerr(t, errnum, n->u[0].child, indent + 2 * IndentInc, brace);
	       ld_prmloc(tmp_prms);
	       PRT_CSTR("if (!", indent + IndentInc);
	       cnv_fnc(t, TypCDbl, n->u[1].child, NULL, NULL,
		  indent + IndentInc);
	       PRT_CSTR(") ", indent + IndentInc);
	       ForceNl();
	       sv_prmloc(tmp_prms);   /* use original parm locs for error */
	       ld_prmloc(strt_prms);
	       prt_runerr(t, errnum, n->u[1].child, indent + 2 * IndentInc, brace);
	       ld_prmloc(tmp_prms);
	       if (rt_walk(n1->u[2].child, indent + IndentInc, 0 /* brace */)) {
		  fall_thru |= 1;
		  mrg_prmloc(end_prms);
		  }
	       ForceNl();
	       PRT_CSTR("}", indent + IndentInc);
	       ForceNl();

	       ld_prmloc(end_prms);
	       free(strt_prms);
	       free(end_prms);
	       free(tmp_prms);
	       free_tree(errnum);
	       return fall_thru;
	       }
	    }
      case QuadNd:
	 /*
	  * RTL code: def: <type> ( <source> , <default>)
	  *           def: <type> ( <source> , <default> , <destination> )
	  */
	 cnv_fnc(t, icn_typ(n->u[0].child), n->u[1].child, n->u[2].child,
	    n->u[3].child, indent);
	 return 1;
      }
   /*NOTREACHED*/
   return 0;  /* avoid gcc warning */
   }

/*
 * spcl_dcls - print special declarations for tended variables, parameter
 *  conversions, and buffers.
 */
void spcl_dcls(op_params)
struct sym_entry *op_params; /* operation parameters or NULL */
   {
   struct sym_entry *sym, *sym1;

   /*
    * Output declarations for buffers and locations to hold conversions
    *  to C values.
    */
   spcl_start(op_params);

   /*
    * Determine if this operation takes a variable number of arguments.
    *  Use that information in deciding how large a tended array to
    *  declare.
    */
   varargs = (op_params != NULL && op_params->id_type & VarPrm);
   if (varargs)
      tend_ary(ntend + VArgAlwnc - 1);
   else
      tend_ary(ntend);

   if (varargs) {
      /*
       * This operation takes a variable number of arguments. A declaration
       *  for a tended array has been made that will usually hold them, but
       *  sometimes it is necessary to malloc() a tended array at run
       *  time. Produce code to check for this.
       */
      cur_impl->ret_flag |= DoesEFail;  /* error conversion from allocation */
      PRT_CSTR("pointer r_tendp;", IndentInc);
      ForceNl();
      PRT_CSTR("int r_n;\n", IndentInc);
      ++g_line;
      ForceNl();
      chk_nl(IndentInc);
      fprintf(g_out_file, "if (r_nargs <= %d)", op_params->u.param_info.param_num + VArgAlwnc);
      ForceNl();
      PRT_CSTR("r_tendp = &r_tend;", 2 * IndentInc);
      ForceNl();
      PRT_CSTR("else {", IndentInc);
      ForceNl();
      prt_str(
       "r_tendp = malloc((sizeof(struct tend_desc)",
	 2 * IndentInc);
      ForceNl();
      chk_nl(3 * IndentInc);
      fprintf(g_out_file, "+ (r_nargs + %d) * sizeof(struct descrip)));",
	 ntend - 2 - op_params->u.param_info.param_num);
      ForceNl();
      PRT_CSTR("if (r_tendp == NULL) {", 2 * IndentInc);
      ForceNl();
      PRT_CSTR("err_msg(305, NULL);", 3 * IndentInc);
      ForceNl();
      PRT_CSTR("return A_Resume;", 3 * IndentInc);
      ForceNl();
      PRT_CSTR("}", 3 * IndentInc);
      ForceNl();
      PRT_CSTR("}", 2 * IndentInc);
      ForceNl();
      tendstrct = "(*r_tendp)";
      }
   else
      tendstrct = "r_tend";

   /*
    * Produce code to initialize the tended array. These are for tended
    *  declarations and parameters.
    */
   tend_init();  /* initializations for tended declarations. */
   if (varargs) {
      /*
       * This operation takes a variable number of arguments. Produce code
       *  to dereference or copy this into its portion of the tended
       *  array.
       */
      chk_nl(IndentInc);
      fprintf(g_out_file, "for (r_n = %d; r_n < r_nargs; ++r_n)",
	  op_params->u.param_info.param_num);
      ForceNl();
      if (op_params->id_type & DrfPrm) {
	 PRT_CSTR("deref(&r_args[r_n], &", IndentInc * 2);
	 fprintf(g_out_file, "%s.d[r_n + %d]);", tendstrct, ntend - 1 -
	    op_params->u.param_info.param_num);
	 }
      else {
	 prt_str(tendstrct, IndentInc * 2);
	 fprintf(g_out_file, ".d[r_n + %d] = r_args[r_n];", ntend - 1 -
	    op_params->u.param_info.param_num);
	 }
      ForceNl();
      sym = op_params->u.param_info.next;
      }
   else
      sym = op_params; /* no variable part of arg list */

   /*
    * Go through the fixed part of the parameter list, producing code
    *  to copy/dereference parameters into the tended array.
    */
   while (sym != NULL) {
      /*
       * A there may be identifiers for dereferenced and/or undereferenced
       *  versions of a paramater. If there are both, sym1 references the
       *  second identifier.
       */
      sym1 = sym->u.param_info.next;
      if (sym1 != NULL && sym->u.param_info.param_num !=
	 sym1->u.param_info.param_num)
	    sym1 = NULL;    /* the next entry is not for the same parameter */

      /*
       * If there are not enough arguments to supply a value for this
       *  parameter, set it to the null value.
       */
      chk_nl(IndentInc);
      fprintf(g_out_file, "if (r_nargs > %d) {", sym->u.param_info.param_num);
      ForceNl();
      parm_tnd(sym);
      if (sym1 != NULL) {
	 ForceNl();
	 parm_tnd(sym1);
	 }
      ForceNl();
      PRT_CSTR("} else {", IndentInc);
      ForceNl();
      prt_str(tendstrct, IndentInc * 2);
      fprintf(g_out_file, ".d[%d].dword = D_Null;", sym->t_indx);
      if (sym1 != NULL) {
	 ForceNl();
	 prt_str(tendstrct, IndentInc * 2);
	 fprintf(g_out_file, ".d[%d].dword = D_Null;", sym1->t_indx);
	 }
      ForceNl();
      PRT_CSTR("}", 2 * IndentInc);
      ForceNl();
      if (sym1 == NULL)
	 sym = sym->u.param_info.next;
      else
	 sym = sym1->u.param_info.next;
      }

   /*
    * Finish setting up the tended array structure and link it into the tended
    *  list.
    */
   if (ntend) {
      prt_str(tendstrct, IndentInc);
      if (varargs)
	 fprintf(g_out_file, ".num = %d + Max(r_nargs - %d, 0);", ntend - 1,
	    op_params->u.param_info.param_num);
      else
	 fprintf(g_out_file, ".num = %d;", ntend);
      ForceNl();
      prt_str(tendstrct, IndentInc);
      PRT_CSTR(".previous = tend;", IndentInc);
      ForceNl();
      chk_nl(IndentInc);
      fprintf(g_out_file, "tend = &%s;", tendstrct);
      ForceNl();
      }
   }

/*
 * spcl_start - do initial work for outputing special declarations. Output
 *  declarations for buffers and locations to hold conversions to C values.
 *  Determine what tended locations are needed for parameters.
 */
static void spcl_start(op_params)
struct sym_entry *op_params;
   {
   ForceNl();
   if (g_n_tmp_str > 0) {
      PRT_CSTR("char r_sbuf[", IndentInc);
      fprintf(g_out_file, "%d][MaxCvtLen];", g_n_tmp_str);
      ForceNl();
      }
   if (g_n_tmp_cset > 0) {
      PRT_CSTR("struct b_cset r_cbuf[", IndentInc);
      fprintf(g_out_file, "%d];", g_n_tmp_cset);
      ForceNl();
      }
   if (g_tend_lst == NULL)
      ntend = 0;
   else
      ntend = g_tend_lst->t_indx + 1;
   parm_locs(op_params); /* see what parameter conversion there are */
   }

/*
 * tend_ary - write struct containing array of tended descriptors.
 */
static void tend_ary(n)
int n;
   {
   if (n == 0)
      return;
   PRT_CSTR("struct {", IndentInc);
   ForceNl();
   PRT_CSTR("struct tend_desc *previous;", 2 * IndentInc);
   ForceNl();
   PRT_CSTR("int num;", 2 * IndentInc);
   ForceNl();
   PRT_CSTR("struct descrip d[", 2 * IndentInc);
   fprintf(g_out_file, "%d];", n);
   ForceNl();
   PRT_CSTR("} r_tend;\n", 2 * IndentInc);
   ++g_line;
   ForceNl();
   }

/*
 * tend_init - produce code to initialize entries in the tended array
 *  corresponding to tended declarations. Default initializations are
 *  supplied when there is none in the declaration.
 */
static void tend_init()
   {
   struct init_tend *tnd;

   for (tnd = g_tend_lst; tnd != NULL; tnd = tnd->next) {
      switch (tnd->init_typ) {
	 case TndDesc:
	    /*
	     * Simple tended declaration.
	     */
	    prt_str(tendstrct, IndentInc);
	    if (tnd->init == NULL)
	       fprintf(g_out_file, ".d[%d].dword = D_Null;", tnd->t_indx);
	    else {
	       fprintf(g_out_file, ".d[%d] = ", tnd->t_indx);
	       c_walk(tnd->init, 2 * IndentInc, 0);
	       PRT_CSTR(";", 2 * IndentInc);
	       }
	    break;
	 case TndStr:
	    /*
	     * Tended character pointer.
	     */
	    prt_str(tendstrct, IndentInc);
	    if (tnd->init == NULL)
	       fprintf(g_out_file, ".d[%d] = emptystr;", tnd->t_indx);
	    else {
	       fprintf(g_out_file, ".d[%d].dword = 0;", tnd->t_indx);
	       ForceNl();
	       prt_str(tendstrct, IndentInc);
	       fprintf(g_out_file, ".d[%d].vword.sptr = (char*)", tnd->t_indx);
	       c_walk(tnd->init, 2 * IndentInc, 0);
	       PRT_CSTR(";", 2 * IndentInc);
	       }
	    break;
	 case TndBlk:
	    /*
	     * A tended block pointer of some kind.
	     */
	    prt_str(tendstrct, IndentInc);
	    if (tnd->init == NULL)
	       fprintf(g_out_file, ".d[%d] = nullptr;", tnd->t_indx);
	    else {
	       fprintf(g_out_file, ".d[%d].dword = F_Ptr | F_Nqual;",tnd->t_indx);
	       ForceNl();
	       prt_str(tendstrct, IndentInc);
	       fprintf(g_out_file, ".d[%d].vword.ptr = ",
		   tnd->t_indx);
	       c_walk(tnd->init, 2 * IndentInc, 0);
	       PRT_CSTR(";", 2 * IndentInc);
	       }
	    break;
	 }
      ForceNl();
      }
   }

/*
 * parm_tnd - produce code to put a parameter in its tended location.
 */
static void parm_tnd(sym)
struct sym_entry *sym;
   {
   /*
    * A parameter may either be dereferenced into its tended location
    *  or copied.
    */
   if (sym->id_type & DrfPrm) {
      PRT_CSTR("deref(&r_args[", IndentInc * 2);
      fprintf(g_out_file, "%d], &%s.d[%d]);", sym->u.param_info.param_num,
	 tendstrct, sym->t_indx);
      }
   else {
      prt_str(tendstrct, IndentInc * 2);
      fprintf(g_out_file, ".d[%d] = r_args[%d];", sym->t_indx,
	 sym->u.param_info.param_num);
      }
   }

/*
 * parm_locs - determine what locations are needed to hold parameters and
 *  their conversions. Produce declarations for the C_integer and C_double
 *  locations.
 */
static void parm_locs(op_params)
struct sym_entry *op_params;
   {
   struct sym_entry *next_parm;

   /*
    * Parameters are stored in reverse order: Recurse down the list
    *  and perform processing on the way back.
    */
   if (op_params == NULL)
      return;
   next_parm = op_params->u.param_info.next;
   parm_locs(next_parm);

   /*
    * For interpreter routines, extra tended descriptors are only needed
    *  when both dereferenced and undereferenced values are requested.
    */
   if (iconx_flg && (next_parm == NULL ||
      op_params->u.param_info.param_num != next_parm->u.param_info.param_num))
      op_params->t_indx = -1;
   else
      op_params->t_indx = ntend++;
   if (op_params->u.param_info.non_tend & PrmInt) {
      chk_nl(IndentInc);
      fprintf(g_out_file, "uword r_i%d;", op_params->u.param_info.param_num);
      ForceNl();
      }
   if (op_params->u.param_info.non_tend & PrmDbl) {
      chk_nl(IndentInc);
      fprintf(g_out_file, "double r_d%d;", op_params->u.param_info.param_num);
      ForceNl();
      }
   }

void dclout(n)
struct node *n;
   {
   int idx;
   const char *aggregator;
   struct dclout_data data = {0};
   struct node *auxnd = NULL, *n1;

   if ((n1 = nav_t(n, BinryNd, ';', 1))) {
      struct node *n0 = n->u[0].child;

      if (nav_n_is_t(n1, ConCatNd, 1, BinryNd, ')')) {
	 /* a function prototype, ignore */
	 free_tree(n);
	 return;
	 }
      if (nav_n_is_t(n0, LstNd, 0, PrimryNd, Extern)) {
	 /* an extern, ignore */
	 free_tree(n);
	 return;
	 }
      }

   if ((n = normalize_toplevel(n, &auxnd)) == NULL) {
      /* non-concrete type, discard */
      if (auxnd) {
	 dclout(auxnd);
	 auxnd = NULL;
	 }
      return;
      }

   /* 'n' is returned in 'data' to be properly freed */
   top_level_chunk_name(&data, n);
   if (data.tally == 0) {
      free_tree(n);
      return;
      }

   /* before normalization 'data' could return multiple items */
   assert(data.tally == 1);

   for (idx = 0; data.tally; data.tally--, idx++) {
      int typagg = data.items[idx].typagg;
      const char *chunk_name = data.items[idx].chunk_name;

      switch (typagg) {
      case Identifier:
	 aggregator = "globals";
	 break;
      case Typedef:
	 aggregator = "typedefs";
	 break;
      case Struct:
      case Union:
	 aggregator = "structs and unions";
	 break;
      case Enum:
	 aggregator = "enums";
	 break;
      case ';':		/* forward declarations */
	 aggregator = "forward decls";
	 break;
      default:
	 fprintf(stderr, "/""*%d,%s*""/\n", __LINE__, node_name(n));
	 fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
	    __FILE__, __LINE__, g_fname_for___FILE__,
	    g_line_for___LINE__);
	 exit(1);
	 break;
      }

      chk_nl(0);
      if (chunk_name)
	 fprintf(g_out_file, "<<%s>>=\n<<%s>>\n<<%s>>=\n",
	    aggregator, chunk_name, chunk_name);
      else	/* aggregator only */
	 fprintf(g_out_file, "<<%s>>=\n", aggregator);

      c_walk(data.items[idx].node, 0, 0);
      ForceNl();
      free_tree(data.items[idx].node);
      }
   ForceNl();
   if (auxnd) {
      dclout(auxnd);
      auxnd = NULL;
      }
   }

static struct node *has_primry(n, p)
struct node *n;
int p;
   {
   if (n == NULL)
      return NULL;
   if (is_n(n, LstNd)) {
      struct node *r;
      if ((r = has_primry(n->u[0].child, p)))
	 return r;
      return has_primry(n->u[1].child, p);
      }
   return is_t(n, PrimryNd, p);
   }

static int is_static_function(head)
struct node *head;
   {
   struct node *nd1;
   if ((nd1 = nav_n_n(head, LstNd, 0, LstNd, 0))) {
      if (is_a(nd1, PrimryNd)) {
	 switch (nd1->tok->tok_id) {
	    case Static:
	       return 1;
	    case Long:
	    case Int:
	    case Char:
	    case Short:
	    case Float:
	    case Doubl:
	    case Unsigned:
	    case Const:
	    case CompatInlineC99:
	    case CompatInlineRTT:
	    case CompatNoreturnC11:
	    case CompatNoreturnC23:
	    case CompatNoreturnRTT:
	       return 0;
	    default:
	       fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
		  __FILE__, __LINE__, g_fname_for___FILE__,
		  g_line_for___LINE__);
	       exit(1);
	    }
	 }
      else if (is_n(nd1, LstNd))
	 return has_primry(nd1, Static) != NULL;
      else {
	 fprintf(stderr, "Exhaustion %s:%d.\n", __FILE__, __LINE__);
	 exit(1);
	 }
      }
   return 0;
   }

static struct node *fnc_head_args(head)
struct node *head;
   {
   struct node *nd1;
   if ((nd1 = nav_n_n_t(head, LstNd, 1, ConCatNd, 1, BinryNd, ')', 1)))
      return nd1;
   return NULL;
   }

static int get_args_names(out, start, len, n)
struct node **out;
int start, len;
struct node *n;
   {
   if (is_a(n, CommaNd)) {
      start = get_args_names(out, start, len, n->u[0].child);
      start = get_args_names(out, start, len, n->u[1].child);
      }
   else if (is_ttt(n, PrimryNd, Identifier, Type, IconType)) {
      if (start >= len) {
	 fprintf(stderr, "Exhaustion %s:%d, too many arguments, more than %d.\n", __FILE__, __LINE__, len);
	 exit(1);
	 }
      out[start++] = n;
      }
   else {
      fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
	 __FILE__, __LINE__, g_fname_for___FILE__,
	 g_line_for___LINE__);
      exit(1);
      }
   return start;
   }

static int get_comma_children(out, start, len, n)
struct node **out, *n;
int start, len;
   {
   if (n->nd_id == CommaNd) {
      start = get_comma_children(out, start, len, n->u[0].child);
      start = get_comma_children(out, start, len, n->u[1].child);
      }
   else {
      if (start >= len) {
	 fprintf(stderr, "Exhaustion %s:%d, too many arguments, more than %d.\n", __FILE__, __LINE__, len);
	 exit(1);
	 }
      out[start++] = n;
      }
   return start;
   }

static int get_declarations_as_list(out, start, len, n)
struct node **out, *n;
int start, len;
   {
   if (n->nd_id == LstNd) {
      start = get_declarations_as_list(out, start, len, n->u[0].child);
      start = get_declarations_as_list(out, start, len, n->u[1].child);
      }
   else if (n->nd_id == BinryNd && n->tok->tok_id == ';') {
      struct node *nd;
      if (start >= len) {
	 fprintf(stderr, "Exhaustion %s:%d, too many arguments, more than %d.\n", __FILE__, __LINE__, len);
	 exit(1);
	 }
      if ((nd = n->u[1].child)) {
	 if (nd->nd_id == ConCatNd) {
	    out[start++] = node2(LstNd, NULL,
	       copy_tree(n->u[0].child),
	       copy_tree(n->u[1].child));
	    }
	 else if (nd->nd_id == CommaNd) {
	    int nchildren, i;
	    struct node *children[MAX_NCHILDREN], *typedefname;
	    nchildren = get_comma_children(children, 0, sizeof(children)/sizeof(*children), nd);
	    typedefname = n->u[0].child;
	    for (i=0; i<nchildren; i++) {
	       out[start++] = node2(LstNd, NULL,
		  copy_tree(typedefname),
		  copy_tree(children[i]));
	       }
	    }
	 }
      else {
	 fprintf(stderr, "Exhaustion %s:%d.\n", __FILE__, __LINE__);
	 exit(1);
	 }
      }
   else {
      fprintf(stderr, "Exhaustion %s:%d.\n", __FILE__, __LINE__);
      exit(1);
      }
   return start;
   }

static struct node *header_k_and_r_to_ansi(head, prm_dcl)
struct node *head, *prm_dcl;
   {
   struct node *args;

   args = fnc_head_args(head);

   if (args == NULL) {
      struct node *new_head, *nd;
      struct token *t;

      new_head = copy_tree(head);
      nd = nav_n_n(new_head, LstNd, 1, ConCatNd, 1); /* args holder */
      if (nd == NULL) {
	 fprintf(stderr, "Exhaustion %s:%d.\n", __FILE__, __LINE__);
	 exit(1);
	 }
      t = new_token(Void, "void", __FILE__, __LINE__);
      nd->u[1].child = node0(PrimryNd, t);
      NODE_UPDATE_TRACE(new_head);
      return new_head;
      }
   else if (prm_dcl) {
      struct node *new_head, *names[MAX_NARGS], *decls[MAX_NARGS], *decl_lst;
      int i, argc, argc_check;

      argc = get_args_names(names, 0, sizeof(names)/sizeof(*names), args);

      argc_check = get_declarations_as_list(decls, 0, sizeof(decls)/sizeof(*decls), prm_dcl);
      if (argc != argc_check) {
	 fprintf(stderr, "Exhaustion %s:%d.\n", __FILE__, __LINE__);
	 exit(1);
	 }

      decl_lst = NULL;

      for (i=0; i<argc; i++) {
	 int j;
	 struct node *found = NULL;
	 char *id;
	 id = names[i]->tok->image;
	 for (j=0; j<argc; j++) {
	    struct node *decl, *nd, *nd2;
	    decl = decls[j];
	    if ((nd = nav_n_n(decl, LstNd, 1, ConCatNd, 1))) {
	       if (nd->tok == NULL) {
		  fprintf(stderr, "Exhaustion %s:%d.\n", __FILE__, __LINE__);
		  exit(1);
		  }
	       if (is_ttt(nd, PrimryNd, Identifier, Type, IconType)) {
		  if (nd->tok->image == id) {
		     found = decl;
		     break;
		     }
		  }
	       else if ((nd2 = nav_t_is_ttt(nd, BinryNd, '[', 0, PrimryNd, Identifier, Type, IconType))) {
		  if (nd2->tok->image == id) {
		     found = decl;
		     break;
		     }
		  }
	       else {
		  fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
		     __FILE__, __LINE__, g_fname_for___FILE__,
		     g_line_for___LINE__);
		  exit(1);
		  }
	       }
	    else {
	       fprintf(stderr, "Exhaustion %s:%d.\n", __FILE__, __LINE__);
	       exit(1);
	       }
	    }
	 if (found == NULL) {
	    fprintf(stderr, "Exhaustion %s:%d, declaration for identifier \"%s\" not found.\n",
	       __FILE__, __LINE__, id);
	    exit(1);
	    }
	 if (decl_lst == NULL) {
	    decl_lst = node2(LstNd, NULL,
	       copy_tree(found->u[0].child),
	       copy_tree(found->u[1].child));
	    }
	 else {
	    decl_lst = node2(CommaNd, new_token(',', ",", __FILE__, __LINE__),
	       decl_lst,
	       node2(LstNd, NULL,
		  copy_tree(found->u[0].child),
		  copy_tree(found->u[1].child))
	       );
	    }
	 }

      for (i=0; i<argc; i++) {
	 /* get_declarations_as_list returns a copied tree */
	 free_tree(decls[i]);
	 }

      new_head = node2(LstNd, NULL,
	 copy_tree(nav_n(head, LstNd, 0)),
	 node2(ConCatNd, NULL,
	    copy_tree(nav_n_n(head, LstNd, 1, ConCatNd, 0)),
	    node2(BinryNd, new_token(')', ")", __FILE__, __LINE__),
	       copy_tree(nav_n_n_t(head, LstNd, 1, ConCatNd, 1, BinryNd, ')', 0)),
	       decl_lst)));

      return new_head;
      }
   return head;
   }

/*
 * fncout - output code for a C function.
 */
void fncout(head, prm_dcl, block)
struct node *head, *prm_dcl, *block;
   {
   struct node *head_ansi, *nd1, *fnc_name;

   g_nxt_sbuf = 0;    /* clear number of string buffers */
   g_nxt_cbuf = 0;    /* clear number of cset buffers */

   /*
    * Output the function header and the parameter declarations.
    */
   fnc_head = head;

   ForceNl();

   head_ansi = header_k_and_r_to_ansi(head, prm_dcl);

   if ((nd1 = nav_n_n_t(head_ansi, LstNd, 1, ConCatNd, 1, BinryNd, ')', 0)) == NULL) {
      fprintf(stderr, "Exhaustion %s:%d.\n", __FILE__, __LINE__);
      exit(1);
      }

   if ((fnc_name = is_t(nd1, PrimryNd, Identifier)) == NULL) {
      if ((fnc_name = nav_t_n_t_is_t(nd1, PrefxNd, '(', 0, ConCatNd, 1, BinryNd, ')', 0, PrimryNd, Identifier)) == NULL) {
	 fprintf(stderr, "Exhaustion %s:%d.\n", __FILE__, __LINE__);
	 exit(1);
	 }
      }

   if (no_proto_for(fnc_name->tok->image) == NULL) {
      const char *p;
      chk_nl(0);
      if (is_static_function(head_ansi))
	 p = "<<protos - static>>=\n";
      else
	 p = "<<protos>>=\n";
      fprintf(g_out_file, "%s<<proto %s>>\n<<proto %s>>=\n",
	 p, fnc_name->tok->image, fnc_name->tok->image);
      c_walk(head_ansi, 0, 0);
      PRT_CSTR(";", 0);
      ForceNl();
      }

   chk_nl(0);
   fprintf(g_out_file, "<<impl>>=\n<<impl %s>>\n<<impl %s>>=\n",
      fnc_name->tok->image, fnc_name->tok->image);
   c_walk(head_ansi, 0, 0);
   ForceNl();

   /*
    * Handle outer block.
    */
   chk_spc();
   PRT_TOK(block->tok, IndentInc);          /* { */
   ForceNl();

   c_walk(block->u[0].child, IndentInc, 0); /* non-tended declarations */
   spcl_dcls(NULL);                         /* tended declarations */
   no_ret_val = 1;
   c_walk(block->u[2].child, IndentInc, 0); /* statement list */
   if (ntend && no_ret_val) {
      /*
       * This function contains no return statements with values, assume
       *  that the programmer is using the implicit return at the end
       *  of the function and update the tending of descriptors.
       */
      untend(IndentInc);
      }
   ForceNl();
   PRT_CSTR("}\n@", IndentInc);
   ForceNl();

   /*
    * free storage.
    */
   if (head_ansi != head)
      free_tree(head_ansi);
   free_tree(head);
   free_tree(prm_dcl);
   free_tree(block);
   pop_cntxt();
   clr_def();
   }

/*
 * defout - output operation definitions (except for constant keywords)
 */
void defout(n)
struct node *n;
   {
   struct sym_entry *sym, *sym1;

   g_nxt_sbuf = 0;
   g_nxt_cbuf = 0;

   /*
    * Somewhat different code is produced for the interpreter and compiler.
    */
   if (iconx_flg)
      interp_def(n);
   else
      comp_def(n);

   free_tree(n);
   /*
    * The declarations for the declare statement are not associated with
    *  any compound statement and must be freed here.
    */
   sym = g_dcl_stk->tended;
   while (sym != NULL) {
      sym1 = sym;
      sym = sym->u.tnd_var.next;
      free_sym(sym1);
      }
   while (g_decl_lst != NULL) {
      sym1 = g_decl_lst;
      g_decl_lst = g_decl_lst->u.declare_var.next;
      free_sym(sym1);
      }
   op_type = OrdFunc;
   pop_cntxt();
   clr_def();
   }

/*
 * comp_def - output code for the compiler for operation definitions.
 */
static void comp_def(n)
struct node *n;
   {
   #ifdef Rttx
      fprintf(stdout,
	 "rtt was compiled to only support the interpreter, use -x\n");
      exit(EXIT_FAILURE);
   #else				/* Rttx */
   struct sym_entry *sym;
   struct node *n1;
   FILE *f_save;

   char buf1[5], buf[MaxPath];
   char *cname;
   long min_result, max_result;
   int ret_flag, resume;
   char *name, *s;

   f_save = g_out_file;

   /*
    * Note if the result location is explicitly referenced and note
    *  how it is accessed in the generated code.
    */
   cur_impl->use_rslt = sym_lkup(g_str_rslt)->u.referenced;
   rslt_loc = "(*r_rslt)";

   /*
    * In several contexts, letters are used to distinguish kinds of operations.
    */
   switch (op_type) {
      case Function:
	 lc_letter = 'f';
	 uc_letter = 'F';
	 break;
      case Keyword:
	 lc_letter = 'k';
	 uc_letter = 'K';
	 break;
      case Operator:
	 lc_letter = 'o';
	 uc_letter = 'O';
      }
   prfx1 = cur_impl->prefix[0];
   prfx2 = cur_impl->prefix[1];

   if (op_type != Keyword) {
      /*
       * First pass through the operation: produce most general routine.
       */
      fnc_ret = RetSig;  /* most general routine always returns a signal */

      /*
       * Compute the file name in which to output the function.
       */
      snprintf(buf1, sizeof(buf1), "%c_%c%c", lc_letter, prfx1, prfx2);
      cname = salloc(makename(buf, sizeof(buf), SourceDir, buf1, NWSuffix));
      if ((g_out_file = fopen(cname, "w")) == NULL)
	 err2("cannot open output file", cname);
      else
	 addrmlst(cname, g_out_file);

      /*
       * Output function header that corresponds to standard calling
       *  convensions. The function name is constructed from the letter
       *  for the operation type, the prefix that makes the function
       *  name unique, and the name of the operation.
       */
      PRT_CSTR("<<impl>>=\n", 0);
      fprintf(g_out_file, "int %c%c%c_%s(uword r_nargs, dptr r_args, dptr r_rslt, continuation r_s_cont)",
	 uc_letter, prfx1, prfx2, cur_impl->name);
      ForceNl();
      PRT_CSTR("{", IndentInc);
      ForceNl();
      g_fname = cname;

      /*
       * Output ordinary declarations from declare clause.
       */
      for (sym = g_decl_lst; sym != NULL; sym = sym->u.declare_var.next) {
	 c_walk(sym->u.declare_var.tqual, IndentInc, 0);
	 PRT_CSTR(" ", IndentInc);
	 c_walk(sym->u.declare_var.dcltor, IndentInc, 0);
	 if ((n1 = sym->u.declare_var.init) != NULL) {
	    PRT_CSTR(" = ", IndentInc);
	    c_walk(n1, IndentInc, 0);
	    }
	 PRT_CSTR(";", IndentInc);
	 }

      /*
       * Output code for special declarations along with code to initial
       *  them. This includes buffers and tended locations for parameters
       *  and tended variables.
       */
      spcl_dcls(g_params);

      if (rt_walk(n, IndentInc, 0 /* brace */)) {  /* body of operation */
	 if (n->nd_id == ConCatNd)
	    s = n->u[1].child->tok->fname;
	 else
	    s = n->tok->fname;
	 fprintf(stderr, "%s: file %s, warning: ", g_progname, s);
	 fprintf(stderr, "execution may fall off end of operation \"%s\"\n",
	     cur_impl->name);
	 }

      ForceNl();
      PRT_CSTR("}\n@", IndentInc);
      ForceNl();
      if (fclose(g_out_file) != 0)
	 err2("cannot close ", cname);
      put_c_fl(cname, 1);  /* note name of output file for operation */
      }

   /*
    * Second pass through operation: produce in-line code and special purpose
    *  routines.
    */
   for (sym = g_params; sym != NULL; sym = sym->u.param_info.next)
      if (sym->id_type & DrfPrm)
	 sym->u.param_info.cur_loc = PrmTend;  /* reset location of parameter */
   in_line(n);

   /*
    * Ensure that the fail/return/suspend statements are consistent
    *  with the result sequence indicated.
    */
   min_result = cur_impl->min_result;
   max_result = cur_impl->max_result;
   ret_flag = cur_impl->ret_flag;
   resume = cur_impl->resume;
   name = cur_impl->name;
   if (min_result == NoRsltSeq && ret_flag & (DoesFail|DoesRet|DoesSusp))
      err2(name,
	 ": result sequence of {}, but fail, return, or suspend present");
   if (min_result != NoRsltSeq && ret_flag == 0)
      err2(name,
	 ": result sequence indicated, no fail, return, or suspend present");
   if (max_result != NoRsltSeq) {
      if (max_result == 0 && ret_flag & (DoesRet|DoesSusp))
	 err2(name,
	    ": result sequence of 0 length, but return or suspend present");
      if (max_result != 0 && !(ret_flag & (DoesRet | DoesSusp)))
	 err2(name,
	    ": result sequence length > 0, but no return or suspend present");
      if ((max_result == UnbndSeq || max_result > 1 || resume) &&
	 !(ret_flag & DoesSusp))
	 err2(name,
	    ": result sequence indicates suspension, but no suspend present");
      if ((max_result != UnbndSeq && max_result <= 1 && !resume) &&
	 ret_flag & DoesSusp)
	 err2(name,
	    ": result sequence indicates no suspension, but suspend present");
      }
   if (min_result != NoRsltSeq && max_result != UnbndSeq &&
      min_result > max_result)
      err2(name, ": minimum result sequence length greater than maximum");

   g_out_file = f_save;
#endif					/* Rttx */
   }

/*
 * interp_def - output code for the interpreter for operation definitions.
 */
static void interp_def(n)
struct node *n;
   {
   struct sym_entry *sym;
   struct node *n1;
   int nparms, has_underef;
   char letter = '?', *name, *s, *rtt_type;

   /*
    * Note how result location is accessed in generated code.
    */
   rslt_loc = "r_args[0]";

   /*
    * Determine if the operation has any undereferenced parameters.
    */
   has_underef = 0;
   for (sym = g_params; sym != NULL; sym = sym->u.param_info.next)
      if (sym->id_type  & RtParm) {
	 has_underef = 1;
	 break;
	 }

   /*
    * Determine the nuber of parameters. A negative value is used
    *  to indicate an operation that takes a variable number of
    *  arguments.
    */
   if (g_params == NULL)
      nparms = 0;
   else {
      nparms = g_params->u.param_info.param_num + 1;
      if (g_params->id_type & VarPrm)
	 nparms = -nparms;
      }

   fnc_ret = RetSig;  /* interpreter routine always returns a signal */
   name = cur_impl->name;

   /*
    * Determine what letter is used to prefix the operation name.
    */
   switch (op_type) {
      case Function:
	 rtt_type = "RTT_FUNCTION";
	 letter = 'Z';
	 break;
      case Keyword:
	 rtt_type = "RTT_KEYWORD";
	 letter = 'K';
	 break;
      case Operator:
	 rtt_type = "RTT_OPERATOR";
	 letter = 'O';
	 break;
      default:
	 fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
	    __FILE__, __LINE__, g_fname_for___FILE__,
	    g_line_for___LINE__);
	 exit(1);
	 }

   if (op_type != Keyword) {
      /*
       * Output prototype. Operations taking a variable number of arguments
       *   have an extra parameter: the number of arguments.
       */
      PRT_CSTR("<<protos>>=\n", 0 /* indent */);
      fprintf(g_out_file, "%s int %c%s(", rtt_type, letter, name);
#if 0
      if (g_params && (g_params->id_type & VarPrm))
	 fprintf(g_out_file, "uword r_nargs, ");
      fprintf(g_out_file, "dptr r_args);\n");
#else
      /* unified function signature */
      fprintf(g_out_file, "uword r_nargs, dptr r_args);\n");
#endif

      /*
       * Output procedure block.
       */
      switch (op_type) {
	 case Function:
	    fprintf(g_out_file, "<<FncBlock>>=\nFncBlock(%s, %d, %d)\n", name, nparms,
	       (has_underef ? -1 : 0));
	    break;
	 case Operator:
	    if (strcmp(cur_impl->op,"\\") == 0)
	       fprintf(g_out_file, "<<OpBlock>>=\nOpBlock(%s, %d, \"%s\", 0)\n", name, nparms,
		  "\\\\");
	    else
	       fprintf(g_out_file, "<<OpBlock>>=\nOpBlock(%s, %d, \"%s\", 0)\n", name, nparms,
		  cur_impl->op);
	 }
      }
   else {
      /* Output keyword prototype. */
      PRT_CSTR("<<protos>>=\n", 0 /* indent */);
      fprintf(g_out_file, "%s int %c%s(uword r_nargs, dptr r_args);\n", rtt_type, letter, name);
      }

   /*
    * Output function header. Operations taking a variable number of arguments
    *   have an extra parameter: the number of arguments.
    */
   fprintf(g_out_file, "<<impl>>=\n");
   fprintf(g_out_file, "%s int %c%s(", rtt_type, letter, name);
#if 0
   if (g_params != NULL && (g_params->id_type & VarPrm))
      fprintf(g_out_file, "uword r_nargs, ");
   fprintf(g_out_file, "dptr r_args)");
#else
   /* unified function signature */
   fprintf(g_out_file, "uword r_nargs, dptr r_args)");
#endif
   ForceNl();
   PRT_CSTR("{", IndentInc);
   ForceNl();

   /*
    * Output ordinary declarations from the declare clause.
    */
   for (sym = g_decl_lst; sym != NULL; sym = sym->u.declare_var.next) {
      c_walk(sym->u.declare_var.tqual, IndentInc, 0);
      PRT_CSTR(" ", IndentInc);
      c_walk(sym->u.declare_var.dcltor, IndentInc, 0);
      if ((n1 = sym->u.declare_var.init) != NULL) {
	 PRT_CSTR(" = ", IndentInc);
	 c_walk(n1, IndentInc, 0);
	 }
      PRT_CSTR(";", IndentInc);
      }

   /*
    * Output special declarations and initial processing.
    */
   tendstrct = "r_tend";
   spcl_start(g_params);
   tend_ary(ntend);
   if (has_underef && g_params != NULL && g_params->id_type == (VarPrm | DrfPrm))
      PRT_CSTR("int r_n;\n", IndentInc);
   tend_init();

   /*
    * See which parameters need to be dereferenced. If all are dereferenced,
    *  it is done by before the routine is called.
    */
   if (has_underef) {
      sym = g_params;
      if (sym != NULL && sym->id_type & VarPrm) {
	 if (sym->id_type & DrfPrm) {
	    /*
	     * There is a variable part of the parameter list and it
	     *  must be dereferenced.
	     */
	    chk_nl(IndentInc);
	    fprintf(g_out_file, "for (r_n = %d; r_n <= r_nargs; ++r_n)",
		sym->u.param_info.param_num + 1);
	    ForceNl();
	    PRT_CSTR("Deref(r_args[r_n]);", IndentInc * 2);
	    ForceNl();
	    }
	 sym = sym->u.param_info.next;
	 }

      /*
       * Produce code to dereference any fixed parameters that need to be.
       */
      while (sym != NULL) {
	 if (sym->id_type & DrfPrm) {
	    /*
	     * Tended index of -1 indicates that the parameter can be
	     *  dereferened in-place (this is the usual case).
	     */
	    if (sym->t_indx == -1) {
	       chk_nl(IndentInc * 2);
	       fprintf(g_out_file, "Deref(r_args[%d]);", sym->u.param_info.param_num + 1);
	       }
	    else {
	       chk_nl(IndentInc * 2);
	       fprintf(g_out_file, "deref(&r_args[%d], &r_tend.d[%d]);",
		  sym->u.param_info.param_num + 1, sym->t_indx);
	       }
	    }
	 ForceNl();
	 sym = sym->u.param_info.next;
	 }
      }

   /*
    * Finish setting up the tended array structure and link it into the tended
    *  list.
    */
   if (ntend) {
      chk_nl(IndentInc);
      fprintf(g_out_file, "r_tend.num = %d;", ntend);
      ForceNl();
      PRT_CSTR("r_tend.previous = tend;", IndentInc);
      ForceNl();
      PRT_CSTR("tend = &r_tend;", IndentInc);
      ForceNl();
      }

   if (rt_walk(n, IndentInc, 0 /* brace */)) { /* body of operation */
      if (n->nd_id == ConCatNd)
	 s = n->u[1].child->tok->fname;
      else
	 s = n->tok->fname;
      fprintf(stderr, "%s: file %s, warning: ", g_progname, s);
      fprintf(stderr, "execution may fall off end of operation \"%s\"\n",
	  cur_impl->name);
      }
   ForceNl();
   PRT_CSTR("}\n@", IndentInc);
   ForceNl();
   }

/*
 * keyconst - produce code for a constant keyword.
 */
void keyconst(t)
struct token *t;
   {
   struct il_code *il;
   int n;

   if (iconx_flg) {
      /*
       * For the interpreter, output a C function implementing the keyword.
       */
      rslt_loc = "r_args[0]";  /* result location */

      PRT_CSTR("<<protos>>=\n", 0);
      fprintf(g_out_file, "RTT_KEYCONST int K%s(uword r_nargs, dptr r_args);\n", cur_impl->name);
      fprintf(g_out_file, "<<impl>>=\n");
      fprintf(g_out_file, "RTT_KEYCONST int K%s(uword r_nargs, dptr r_args)", cur_impl->name);
      ForceNl();
      PRT_CSTR("{", IndentInc);
      ForceNl();
      switch (t->tok_id) {
	 case StrLit:
	    prt_str(rslt_loc, IndentInc);
	    PRT_CSTR(".vword.sptr = (char*)\"", IndentInc);
	    n = prt_i_str(g_out_file, t->image, (int)strlen(t->image));
	    PRT_CSTR("\";", IndentInc);
	    ForceNl();
	    prt_str(rslt_loc, IndentInc);
	    fprintf(g_out_file, ".dword = %d;", n);
	    break;
	 case CharConst:
	    PRT_CSTR("static struct b_cset cset_blk = ", IndentInc);
	    cset_init(g_out_file, bitvect(t->image, (int)strlen(t->image)));
	    ForceNl();
	    prt_str(rslt_loc, IndentInc);
	    PRT_CSTR(".dword = D_Cset;", IndentInc);
	    ForceNl();
	    prt_str(rslt_loc, IndentInc);
	    PRT_CSTR(".vword.ptr = &cset_blk;", IndentInc);
	    break;
	 case DblConst:
	    PRT_CSTR("static struct b_real real_blk = {T_Real, ", IndentInc);
	    fprintf(g_out_file, "%s};", t->image);
	    ForceNl();
	    prt_str(rslt_loc, IndentInc);
	    PRT_CSTR(".dword = D_Real;", IndentInc);
	    ForceNl();
	    prt_str(rslt_loc, IndentInc);
	    PRT_CSTR(".vword.ptr = &real_blk;", IndentInc);
	    break;
	 case IntConst:
	    prt_str(rslt_loc, IndentInc);
	    PRT_CSTR(".dword = D_Integer;", IndentInc);
	    ForceNl();
	    prt_str(rslt_loc, IndentInc);
	    PRT_CSTR(".vword.uintgr = ", IndentInc);
	    prt_str(t->image, IndentInc);
	    PRT_CSTR(";", IndentInc);
	    break;
	 }
      ForceNl();
      PRT_CSTR("return A_Continue;", IndentInc);
      ForceNl();
      PRT_CSTR("}\n@", IndentInc);
      ForceNl();
      }
   else {
      /*
       * For the compiler, make an entry in the data base for the keyword.
       */
      cur_impl->use_rslt = 0;

      il = new_il(IL_Const, 2);
      switch (t->tok_id) {
	 case StrLit:
	    il->u[0].n = str_typ;
	    il->u[1].s = alloc(strlen(t->image) + 3);
	    sprintf(il->u[1].s, "\"%s\"", t->image);
	    break;
	 case CharConst:
	    il->u[0].n = cset_typ;
	    il->u[1].s = alloc(strlen(t->image) + 3);
	    sprintf(il->u[1].s, "'%s'", t->image);
	    break;
	 case DblConst:
	    il->u[0].n = real_typ;
	    il->u[1].s = t->image;
	    break;
	 case IntConst:
	    il->u[0].n = int_typ;
	    il->u[1].s = t->image;
	    break;
	 }
      cur_impl->in_line = il;
      }

   /*
    * Reset the translator and free storage.
    */
   op_type = OrdFunc;
   free_t(t);
   pop_cntxt();
   clr_def();
   }

/*
 * passthrudir - This is related to passing through, but at the preprocessor
 * level using #passthru directive
 */
void passthrudir(n, v)
struct token *n, *v;
   {
   ForceNl();
   PRT_CSTR("<<", 0);
   prt_str(n->image, 0);
   PRT_CSTR(">>=\n", 0);
   prt_str(v->image, 0);
   PRT_CSTR("\n@", 0);
   ForceNl();
   }

/*
 * outputdir - sets output file name, #output directive
 */
void outputdir(t)
struct token *t;
   {
   char buf[MaxPath], *cname;

   cname = salloc(makename(buf, sizeof(buf), SourceDir, t->image, NULL));
   if (g_out_file) {
      fclose(g_out_file);
      g_out_file = NULL;
      }
   if ((g_out_file = fopen(cname, "w")) == NULL)
      err2("cannot open output file ", cname);
   if (strcmp(cname, "/dev/stdout") == 0)
      ;
   else if (strcmp(cname, "/dev/stderr") == 0)
      ;
   else
      addrmlst(cname, g_out_file);
   }

/*
 * prologue - print standard comments and preprocessor directives at the
 *   start of an output file.
 */
void prologue()
   {
   id_comment(g_out_file);
   fprintf(g_out_file, "<<preamble>>=\n");
#if 0
   fprintf(g_out_file, "%s", compiler_def);
   fprintf(g_out_file, "#include \"%s\"\n", inclname);
#else
   /* TODO: evaluate the need for prologue() */
   fprintf(g_out_file, "/* %s */\n", "compiler_def");
   fprintf(g_out_file, "#include \"%s\"\n", "inclname");
#endif
   fprintf(g_out_file, "#include \"protos.h\"\n@");
   ForceNl();
   }

static struct node *defining_identifier(n)
struct node *n;
   {
   if (n == NULL)
      return NULL;

   switch (n->nd_id) {
      /* 0 */

      case ExactCnv:
	 fprintf(stderr, "Exhaustion %s:%d.\n", __FILE__, __LINE__);
	 exit(1);
	 return NULL;

      case PrimryNd: {
	    int tok_id = n->tok->tok_id;
	    if (tok_id == Identifier || tok_id == TypeDefName ||
	       tok_id == IconType || tok_id == C_Integer ||
	       tok_id == C_Double || tok_id == C_String ||
	       tok_id == Inline
	       )
	       return n;
	    }
	 return NULL;

      /* 1 */

      case SymNd:
      case IcnTypNd:
      case PstfxNd:
      case PreSpcNd:
      case PrefxNd:
	 return defining_identifier(n->u[0].child);

      /* 2 */

      case BinryNd:
	 switch (n->tok->tok_id) {
	    case Struct:
	    case Union:
	    case Enum:
	       return NULL;
	    }
	 /* falls through */
      case ConCatNd:
      case CommaNd:
      case LstNd: {
	    struct node *res;
	    if ((res = defining_identifier(n->u[0].child)))
	       return res;
	    return defining_identifier(n->u[1].child);
	    }

      case StrDclNd:
      case AbstrNd:
      default:
	 fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
	    __FILE__, __LINE__, g_fname_for___FILE__,
	    g_line_for___LINE__);
	 exit(1);
      }
      return NULL;
   }

static char *str_install_local_sbuf(char *s);
static char *str_install_local_sbuf(s)
char *s;
   {
   init_sbuf(sbuf);
   for (; *s != '\0'; ++s)
      AppChar(sbuf, *s);
   return str_install(sbuf);
   }

static char *gensym(const char *prefix);
static char *gensym(prefix)
const char *prefix;
   {
   char buf[100];
   snprintf(buf, sizeof(buf), "%s%d", prefix, sym_counter++);
   return str_install_local_sbuf(buf);
   }

void
tag_name_and_prefix(id, name, prefix)
int id;			/* token id */
const char **name, **prefix;
   {
   switch (id) {
      case Enum:
	 *name = "enum";
	 *prefix = "_tag_enum_";
	 break;
      case Union:
	 *name = "union";
	 *prefix = "_tag_union_";
	 break;
      case Struct:
	 *name = "struct";
	 *prefix = "_tag_struct_";
	 break;
      default:
	 fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
	    __FILE__, __LINE__, g_fname_for___FILE__,
	    g_line_for___LINE__);
	 exit(1);
      }
   }

struct node *new_binary_node_semicolon(struct node *left, struct node *right, const char *fname, int line);
struct node *
new_binary_node_semicolon(left, right, fname, line)
struct node *left, *right;
int line;
const char *fname;
   {
   struct token *t;

   t = new_token(';', ";", (char*)fname, line);
   return node2(BinryNd, t, left, right);
   }

struct node *new_list_node(struct node *left, struct node *right);
struct node *
new_list_node(left, right)
struct node *left, *right;
   {
   return node2(LstNd, NULL, left, right);
   }

struct node *new_static_node(const char *fname, int line);
struct node *
new_static_node(fname, line)
const char *fname;
int line;
   {
   return node0(PrimryNd, new_token(Static, "static", (char*)fname, line));
   }

struct node *new_typedef_node(const char *fname, int line);
struct node *
new_typedef_node(fname, line)
const char *fname;
int line;
   {
   return node0(PrimryNd, new_token(Typedef, "typedef", (char*)fname, line));
   }

struct node *new_toplvlnd(struct node *a, struct node *d);
struct node *new_toplvlnd(a, d)
struct node *a, *d;
   {
   return node2(TopLvlNd, NULL, a, d);
   }

/*
; left-leaning binary tree, first '(cdr x)' is the last item

(define (unspine x)
  (let loop ((x x) (acc '()))
    (if (pair? x)
        (loop (car x) (cons (cdr x) acc))
        (cons x acc))))
 */
struct node *unspine_loop(struct node *typ, struct node *n, struct node *acc);
struct node *unspine_loop(typ, n, acc)
struct node *typ, *n, *acc;
   {
   if (is_t(n, CommaNd, ',')) {
      struct node *res, *cdr_x;

      /* (cdr x) */
      cdr_x = new_binary_node_semicolon(copy_tree(typ), n->u[1].child, __FILE__, __LINE__);
      n->u[1].child = NULL;	/* detach (cdr x) */

      /* (loop (car x) (cons (cdr x) acc)) */
      res = unspine_loop(typ, n->u[0].child, new_toplvlnd(cdr_x, acc));
      n->u[0].child = NULL;	/* detach (car x) */
      free_tree(n);		/* free "cons cell" */
      return res;
      }
   return new_toplvlnd(new_binary_node_semicolon(copy_tree(typ), n, __FILE__, __LINE__), acc);
   }

#define TOPLVLND_NIL new_toplvlnd(NULL, NULL)
struct node *unspine(struct node *typ, struct node *n);
struct node *unspine(typ, n)
struct node *typ, *n;
   {
   return unspine_loop(typ, n, TOPLVLND_NIL);
   }

/* add storage class to simple definition */
void add_storcls_to_simple_def(struct node *sc, struct node *n);
void add_storcls_to_simple_def(sc, n)
struct node *sc, *n;
   {
   if (nav_t_is_a(n, BinryNd, ';', 0, BinryNd) == NULL) {
      fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
	 __FILE__, __LINE__, g_fname_for___FILE__,
	 g_line_for___LINE__);
      }
   n->u[0].child = new_list_node(sc, n->u[0].child);
   NODE_UPDATE_TRACE(n->u[0].child);
   NODE_UPDATE_TRACE(n);
   }

void add_static_to_simple_def(struct node *n, const char *fname, int line);
void add_static_to_simple_def(n, fname, line)
struct node *n;
const char *fname;
int line;
   {
   add_storcls_to_simple_def(new_static_node(fname, line), n);
   }

void add_typedef_to_simple_def(struct node *n, const char *fname, int line);
void add_typedef_to_simple_def(n, fname, line)
struct node *n;
const char *fname;
int line;
   {
   add_storcls_to_simple_def(new_typedef_node(fname, line), n);
   }

/* normalize struct/union/enum */
void
normalize_struct(n, declp, varsp)
struct node *n, **declp, **varsp;
   {
   struct node *typ;	/* type declaration */
   struct node *decl;	/* struct/union type definition */
   struct node *var;	/* variable definition */

   if ((typ = nav_t_is_ttt(n, BinryNd, ';', 0, BinryNd, Struct, Union, Enum)) == NULL) {
      fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
	 __FILE__, __LINE__, g_fname_for___FILE__,
	 g_line_for___LINE__);
      }

   decl = typ->u[1].child;
   var = n->u[1].child;
   n->u[1].child = NULL;	/* detach 'var' */

   if (decl) {
      if (typ->u[0].child == NULL) {
	 /* unnamed struct/union, has definition but not a tag, generate a tag */
	 char *tag_id;
	 struct token *tag_tok;
	 const char *name, *prefix;

	 tag_name_and_prefix(typ->tok->tok_id, &name, &prefix);
	 tag_id = gensym(prefix);
	 tag_tok = new_token(Identifier, tag_id, __FILE__, __LINE__);
	 typ->u[0].child = node0(PrimryNd, tag_tok);
	 NODE_UPDATE_TRACE(typ);
	 }
      *declp = n;
      typ = copy_tree(typ);		/* dup, already at 'n' */
      free_tree(typ->u[1].child);	/* free struct declaration, already at 'n' */
      typ->u[1].child = NULL;	/* detach, just freed above */
      NODE_UPDATE_TRACE(typ);
      }
   else {
      n->u[0].child = NULL;		/* detach 'typ' */
      free_tree(n);
      n = NULL;
      }

   if (var) {
      struct node *tmp;

      if ((tmp = nav_n(var, ConCatNd, 1)) &&
	    (tmp->nd_id == PrimryNd || tmp->nd_id == BinryNd)
	 ) {
	 /* single definition */
	 var = new_binary_node_semicolon(typ, var,
	    __FILE__, __LINE__);
	 *varsp = new_toplvlnd(var, TOPLVLND_NIL);
	 }
      else if ((tmp = nav_t_n(var, BinryNd, '=', 0, ConCatNd, 1)) &&
	    (tmp->nd_id == PrimryNd || tmp->nd_id == BinryNd)
	 ) {
	 /* assignment to a single definition */
	 var = new_binary_node_semicolon(typ, var,
	    __FILE__, __LINE__);
	 *varsp = new_toplvlnd(var, TOPLVLND_NIL);
	 }
      else if (is_t(var, CommaNd, ',')) {
	 /* multiple definitions */
	 *varsp = unspine(typ, var);		/* 'var' is freed by 'unspine' */
	 free_tree(typ);
	 typ = NULL;
	 }
      else {
	 fprintf(stderr, "/""*%d,%s*""/\n", __LINE__, node_name(var));
	 fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
	    __FILE__, __LINE__, g_fname_for___FILE__,
	    g_line_for___LINE__);
	 exit(1);
	 }
      }
   }

/* may return a root node
 */
struct node *
normalize_toplevel(n, auxndp)
struct node *n, **auxndp;
   {
   struct node *n0;

   if ((n0 = nav_t_is_ttt(n, BinryNd, ';', 0, BinryNd, Struct, Union, Enum))) {
      /* a struct/union/enum */
      struct node *decl = NULL, *vars = NULL;
      normalize_struct(n, &decl, &vars);
      n = decl;
      *auxndp = vars;
      }
   else if ((n0 = nav_t_n_is_ttt(n, BinryNd, ';', 0, LstNd, 1, BinryNd, Struct, Union, Enum))) {
      struct node *lst_nd = n->u[0].child;

      if (is_t(lst_nd->u[0].child, PrimryNd, Typedef)) {
	 /* normalize without 'typedef' then re-attach 'typedef' to
	  * the definitions */
	 struct node *decl = NULL, *vars = NULL;

	 normalize_struct(
	    new_binary_node_semicolon(
	       lst_nd->u[1].child,
	       n->u[1].child,
	       __FILE__, __LINE__),
	    &decl, &vars);
	 lst_nd->u[1].child = NULL;	/* detach struct/union/enum */
	 n->u[1].child = NULL;		/* detach vars */
	 lst_nd = NULL;
	 free_tree(n);
	 if (vars) {
	    struct node *var, *iter = vars;

	    while ((var = nav_n(iter, TopLvlNd, 0))) {
	       add_typedef_to_simple_def(var, __FILE__, __LINE__);
	       iter = iter->u[1].child;
	       }
	    }
	 n = decl;
	 *auxndp = vars;
	 }
      else if (is_t(lst_nd->u[0].child, PrimryNd, Static)) {
	 /* normalize without 'static' then re-attach 'static' to
	  * the definitions */
	 struct node *decl = NULL, *vars = NULL;

	 normalize_struct(
	    new_binary_node_semicolon(
	       lst_nd->u[1].child,
	       n->u[1].child,
	       __FILE__, __LINE__),
	    &decl, &vars);
	 lst_nd->u[1].child = NULL;	/* detach struct/union/enum */
	 n->u[1].child = NULL;		/* detach vars */
	 lst_nd = NULL;
	 free_tree(n);
	 if (vars) {
	    struct node *var, *iter = vars;

	    while ((var = nav_n(iter, TopLvlNd, 0))) {
	       add_static_to_simple_def(var, __FILE__, __LINE__);
	       iter = iter->u[1].child;
	       }
	    }
	 n = decl;
	 *auxndp = vars;
	 }
#if 0
      else {
	 fprintf(stderr, "/""*%d,%s*""/\n", __LINE__, node_name(n));
	 }
#endif
      }
   else if ((n0 = nav_t_is_t(n, BinryNd, ';', 1, CommaNd, ','))) {
      /* multiple global definitions */
      n0 = unspine(n->u[0].child /* typ */, n0);
      free_tree(n->u[0].child);		/* free type */
      n->u[0].child = NULL;		/* detach, just freed above */
      n->u[1].child = NULL;		/* detach, freed by unspine via 'n0' */
      free_tree(n);			/* free "cons cell" */
      n = n0->u[0].child;		/* n = (car n) */
      *auxndp = n0->u[1].child;		/* tail = (cdr n) */
      n0->u[0].child = n0->u[1].child = NULL;		/* detach */
      free_tree(n0);			/* free "cons cell" */
      }
   else if (is_n(n, TopLvlNd)) {
      if (n->u[0].child == NULL) {	/* '() */
	 assert(n->u[1].child == NULL);
	 free_tree(n);
	 return NULL;
	 }
      n0 = n->u[0].child;	/* n0 = (car n) */
      *auxndp = n->u[1].child;	/* tail = (cdr n) */
      n->u[0].child = n->u[1].child = NULL;	/* detach */
      free_tree(n);		/* free "cons cell" */
      n = n0;
      }
#if 0
   else {
      fprintf(stderr, "/""*%d,%s*""/\n", __LINE__, node_name(n));
      }
#endif
   return n;
   }

void
top_level_chunk_name(data, n)
struct dclout_data *data;
struct node *n;
   {
   char buf[100];
   struct node *nd1;
   int idx;

   if (data->tally >= MAX_DCLOUT_DATA_ITEMS) {
      fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
	 __FILE__, __LINE__, g_fname_for___FILE__,
	 g_line_for___LINE__);
      exit(1);
      }
   idx = data->tally;
   data->items[idx].node = n;

   /*fprintf(stderr, "/""*%d,idx=%d,%s*""/\n", __LINE__, idx, node_name(n));*/

   if ((nd1 = nav_t(n, BinryNd, ';', 0))) {
      struct node *nd2;

      if (is_a(nd1, BinryNd)) {

	 /* after struct/union normalization, a tag is always present */
	 assert(nd1->u[0].child);

	 if ((nd2 = nd1->u[0].child)) {
	    if (is_t(nd2, PrimryNd, Identifier)) {
	       char *type_name;

	       switch (nd1->tok->tok_id) {
		  case Enum: type_name = "enum"; break;
		  case Union: type_name = "union"; break;
		  case Struct: type_name = "struct"; break;
		  default:
		     fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
			__FILE__, __LINE__, g_fname_for___FILE__,
			g_line_for___LINE__);
		     exit(1);
		  }

	       if (nd1->u[1].child == NULL) {
		  /* no struct/union type definition */

		  if (n->u[1].child) {	/* has variable definition */
		     struct node *id_nd, *comma_nd;

		     /* also no struct/union type definition, expected after normalization */
		     assert(nd1->u[1].child == NULL);

		     if ((id_nd = is_t(n->u[1].child, PrimryNd, Identifier)) && id_nd->tok->image == g_str_GENSYM) {
			/* TODO: add comments to this case */

			/* is this still valid? */
			fprintf(stderr, "/""*%d,%s*""/\n", __LINE__, node_name(n->u[1].child));
			fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
			   __FILE__, __LINE__, g_fname_for___FILE__,
			   g_line_for___LINE__);
			exit(1);

			n->u[1].child = NULL;
			NODE_UPDATE_TRACE(n);
			free(id_nd);
			data->items[idx].chunk_name = g_str_GENSYM;
			data->tally++;
			return;
			}

		     if ((id_nd = nav_n_is_t(n->u[1].child, ConCatNd, 1, PrimryNd, Identifier))) {
			/* one variable definition */
			data->items[idx].typagg = Identifier;
			snprintf(buf, sizeof(buf), "global %s", id_nd->tok->image);
			data->items[idx].chunk_name = str_install_local_sbuf(buf);
			data->tally++;
			}
		     else if ((id_nd = nav_t_n_is_t(n->u[1].child, BinryNd, '=', 0, ConCatNd, 1, PrimryNd, Identifier))) {
			/* assignment to one variable definition */
			data->items[idx].typagg = Identifier;
			snprintf(buf, sizeof(buf), "global %s", id_nd->tok->image);
			data->items[idx].chunk_name = str_install_local_sbuf(buf);
			data->tally++;
			}
		     else if ((id_nd = nav_t_n_t_is_t(n->u[1].child, BinryNd, '=', 0, ConCatNd, 1, BinryNd, '[', 0, PrimryNd, Identifier))) {
			/* array assignment to one variable definition */
			data->items[idx].typagg = Identifier;
			snprintf(buf, sizeof(buf), "global %s", id_nd->tok->image);
			data->items[idx].chunk_name = str_install_local_sbuf(buf);
			data->tally++;
			}
		     else if ((comma_nd = is_t(n->u[1].child, CommaNd, ','))) {
			/* many variable definitions, add directly to globals */
			data->items[idx].typagg = Identifier;
			data->items[idx].chunk_name = NULL;	/* NULL means aggregate only */
			data->tally++;
			}
		     else {
			fprintf(stderr, "/""*%d,%s*""/\n", __LINE__, node_name(n->u[1].child));
			fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
			   __FILE__, __LINE__, g_fname_for___FILE__,
			   g_line_for___LINE__);
			exit(1);
			}
		     return;
		     }
		  else {
		     /* only a foward declaration, incomplete type */
		     /* TODO: this needs a dedicated type aggregator and chunk name */
		     data->items[idx].typagg = ';'; /* use ';' to signal forward declarations */
		     snprintf(buf, sizeof(buf), "forward decls - %s %s", type_name, nd2->tok->image);
		     data->items[idx].chunk_name = str_install_local_sbuf(buf);
		     data->tally++;
		     return;
		     }
		  fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
		     __FILE__, __LINE__, g_fname_for___FILE__,
		     g_line_for___LINE__);
		  exit(1);
		  }

	       data->items[idx].typagg = nd1->tok->tok_id;

	       snprintf(buf, sizeof(buf), "%s %s", type_name, nd2->tok->image);
	       data->items[idx].chunk_name = str_install_local_sbuf(buf);
	       data->tally++;
	       return;
	       }
	    else {
	       fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
		  __FILE__, __LINE__, g_fname_for___FILE__,
		  g_line_for___LINE__);
	       exit(1);
	       }
	    }
	 else if (nd1->u[0].child == NULL) {
	    /* does not have tag, shouldn't happen after normalization */
	    fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
	       __FILE__, __LINE__, g_fname_for___FILE__,
	       g_line_for___LINE__);
	    exit(1);
	    }
	 else {
	    fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
	       __FILE__, __LINE__, g_fname_for___FILE__,
	       g_line_for___LINE__);
	    exit(1);
	    }
	 }
      else if ((nd2 = nav_n(nd1, LstNd, 0))) {
	 struct node *nd3;
	 if (is_t(nd2, PrimryNd, Typedef)) {
	    struct node *id, *maybe_ignore;

	    maybe_ignore = nav_n_is_t(nd1, LstNd, 1, PrimryNd, TypeDefName);
	    if (maybe_ignore && maybe_ignore->tok->image == g_str_IGNORE) {
	       return;
	       }
	    if ((id = defining_identifier(n->u[1].child))) {
	       data->items[idx].typagg = Typedef;
	       snprintf(buf, sizeof(buf),
		  "typedef %s", id->tok->image);
	       data->items[idx].chunk_name = str_install_local_sbuf(buf);
	       data->tally++;
	       return;
	       }
	    fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
	       __FILE__, __LINE__, g_fname_for___FILE__,
	       g_line_for___LINE__);
	    exit(1);
	    }
	 if ((nd3 = nav_n(nd2, LstNd, 0))) {
	    if (is_t(nd3, PrimryNd, Typedef)) {
	       struct node *id;
	       if ((id = defining_identifier(n->u[1].child))) {
		  data->items[idx].typagg = Typedef;
		  snprintf(buf, sizeof(buf),
		     "typedef %s", id->tok->image);
		  data->items[idx].chunk_name = str_install_local_sbuf(buf);
		  data->tally++;
		  return;
		  }
	       fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
		  __FILE__, __LINE__, g_fname_for___FILE__,
		  g_line_for___LINE__);
	       exit(1);
	       }
	    if (is_tttt(nd3, PrimryNd, Const, Int, Unsigned, Static) ||
		  is_tttt(nd3, PrimryNd, Extern, Long, CompatExtension, CompatConst)
	       ) {
	       struct node *id;
	       if ((id = defining_identifier(n->u[1].child))) {
		  data->items[idx].typagg = Identifier;
		  snprintf(buf, sizeof(buf),
		     "global %s", id->tok->image);
		  data->items[idx].chunk_name = str_install_local_sbuf(buf);
		  data->tally++;
		  return;
		  }
	       fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
		  __FILE__, __LINE__, g_fname_for___FILE__,
		  g_line_for___LINE__);
	       exit(1);
	       }
	    fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
	       __FILE__, __LINE__, g_fname_for___FILE__,
	       g_line_for___LINE__);
	    exit(1);
	    }
	 if (is_tttt(nd2, PrimryNd, Const, Int, Unsigned, Char) ||
	       is_tttt(nd2, PrimryNd, Static, Extern, Long, Volatile)
	    ) {
	    struct node *id;
	    if ((id = defining_identifier(n->u[1].child))) {
	       data->items[idx].typagg = Identifier;
	       snprintf(buf, sizeof(buf),
		  "global %s", id->tok->image);
	       data->items[idx].chunk_name = str_install_local_sbuf(buf);
	       data->tally++;
	       return;
	       }
	    fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
	       __FILE__, __LINE__, g_fname_for___FILE__,
	       g_line_for___LINE__);
	    exit(1);
	    }
	 fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
	    __FILE__, __LINE__, g_fname_for___FILE__,
	    g_line_for___LINE__);
	 exit(1);
	 }
      else if (is_tttt(nd1, PrimryNd, TypeDefName, Int, Void, Long) ||
	    is_tttt(nd1, PrimryNd, Char, Doubl, Unsigned, Float)
	 ) {
	 struct node *id;
	 if ((id = defining_identifier(n->u[1].child))) {
	    data->items[idx].typagg = Identifier;
	    snprintf(buf, sizeof(buf),
	       "global %s", id->tok->image);
	    data->items[idx].chunk_name = str_install_local_sbuf(buf);
	    data->tally++;
	    return;
	    }
	 fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
	    __FILE__, __LINE__, g_fname_for___FILE__,
	    g_line_for___LINE__);
	 exit(1);
	 }
      else {
	 fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
	    __FILE__, __LINE__, g_fname_for___FILE__,
	    g_line_for___LINE__);
	 exit(1);
	 }
      }
   fprintf(stderr, "/""*%d,%s*""/\n", __LINE__, node_name(n));
   fprintf(stderr, "Exhaustion %s:%d, last line read: %s:%d.\n",
      __FILE__, __LINE__, g_fname_for___FILE__,
      g_line_for___LINE__);
   exit(1);
   }

static int c_walk_cat(n, indent, brace)
struct node *n;
const int indent, brace;
   {
   int fall_thru, newindent = indent;
   struct node *nd;
   if ((nd = nav_n(n, ConCatNd, 0))) {
      fall_thru = c_walk_cat(nd, indent, 0);
      return fall_thru | c_walk_cat(n->u[1].child, indent, 0);
      }
   if (is_t(n, BinryNd, Case) == NULL && is_t(n, PrefxNd, Default) == NULL)
      newindent += IndentInc;
   return c_walk(n, newindent, brace);
   }

static void c_walk_comma(n, t, indent, brace, may_force_nl, counter, when_nl)
struct node *n;
struct token *t;
const int indent, brace;
int *counter, may_force_nl, when_nl;
   {
   if (is_tt(n, CommaNd, ',', ':')) {
      c_walk_comma(n->u[0].child, NULL, indent, brace, may_force_nl, counter, when_nl);
      c_walk_comma(n->u[1].child, n->tok, indent, brace, may_force_nl, counter, when_nl);
      return;
      }
   if (n) {
      if (t) {
	 if (t->tok_id == ':') {
	    *counter = 0;
	    ForceNl();
	    }
	 prt_tok(t, indent);
	 if (*counter == when_nl) {
	    *counter = 1;
	    ForceNl();
	    }
	 else {
	    PRT_CSTR(" ", indent);
	    (*counter)++;
	    }
	 }
      if (is_t(n, PrefxNd, '{')) {
	 ForceNl();
	 c_walk_nl(n, indent - IndentInc * !!indent, brace, may_force_nl);
	 }
      else
	 c_walk_nl(n, indent, brace, may_force_nl);
      }
   return;
   }

static void c_walk_struct_items(n, indent, brace, may_force_nl)
struct node *n;
const int indent, brace, may_force_nl;
   {
   struct node *nd1;
   if ((nd1 = is_n(n, LstNd))) {
      c_walk_struct_items(nd1->u[0].child, indent, brace, may_force_nl);
      c_walk_struct_items(nd1->u[1].child, indent, brace, may_force_nl);
      return;
      }
   else if ((nd1 = nav_t_is_t(n, BinryNd, ';', 1, CommaNd, ','))) {
      int counter;

      c_walk(n->u[0].child, indent, brace);
      PRT_CSTR(" ", indent);

      counter = 1;
      c_walk_comma(nd1->u[0].child, NULL, indent + IndentInc, brace,
	 may_force_nl, &counter, WHEN_NL_PRIMARY_DECLARATOR_LIST);
      c_walk_comma(nd1->u[1].child, nd1->tok, indent + IndentInc, brace,
	 may_force_nl, &counter, WHEN_NL_PRIMARY_DECLARATOR_LIST);

      prt_tok(n->tok, indent);  /* ; */
      ForceNl();
      return;
      }
   c_walk_nl(n, indent, brace, may_force_nl);
   return;
   }

void id_is_tag(n)
struct node *n;
   {
   if (is_t(n, PrimryNd, TypeDefName)) {
      n->tok->tok_id = Identifier;
      NODE_UPDATE_TRACE(n);
      }
   }

static int prt_closing_parenthesis(nxt, indent)
struct node *nxt; /* next element */
int indent;
{
   int inc;
   PRT_CSTR(")", indent);
   if ((inc = handle_next_indentation(nxt, indent)))
      ForceNl();
   else
      ForceSpc();
   return inc;
   }

static int handle_next_indentation(nxt, indent)
struct node *nxt; /* next element */
int indent;
{
   if (is_t(nxt, CompNd, '{') || is_n(nxt, QuadNd) ||
	 is_t(nxt, BinryNd, Runerr) || does_it_produce_a_compound_statement(nxt)
	 ) {
      return 0;
      }
   return IndentInc;
   }

static int prt_else(nxt, indent)
struct node *nxt; /* next element */
int indent;
{
   int inc;
   PRT_CSTR("else", indent);
   if ((inc = handle_next_indentation(nxt, indent))) {
      if (is_t(nxt, TrnryNd, If)) {
	 /*
	  * Don't indent an "else if"
	  */
	 ForceSpc();
	 return 0;
	 }
      ForceNl();
      }
   else /* next element has '{' */
      ForceSpc();
   return inc;
   }

/* TODO: this is largely incomplete, see 'ret_value' for the final word
 * on this matter, no semantic impact, just formatting, maybe handling
 * the AST (instead of prt_str/fprintf) is the only proper solution
 */
static int does_it_produce_a_compound_statement(n)
struct node *n;
   {
   struct node *n1;
   if ((n1 = is_t(n, PrefxNd, Return)) && n1->u[0].child == NULL && ntend) /* return; && ntend */
      return 1;
   if ((n1 = nav_t(n, PrefxNd, Return, 0)) == NULL) /* not 'return something;' */
      return 0;
   if (ntend) /* return something; && ntend; */
      return 1;
   if (op_type == OrdFunc || fnc_ret == RetInt || fnc_ret == RetDbl) /* ordinary c function return */
      return 0;
   if (is_ttt(n1, SymNd, C_Integer, C_Double, C_String)) /* return special result */
      return 1;
   if (nav_t_is_t(n1, BinryNd, ')', 0, SymNd, IconType))
      return 1;
   if (is_t(n1, SymNd, Identifier))
      return n1->u[0].sym->id_type == RsltLoc; /* return result; */
   return 0;
   }
