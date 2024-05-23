#include "../preproc/preproc.h"
#include "../preproc/pproto.h"

/* gln stands for grammar line number */

#define GLN_POSTFIX_EXPR_ARRAY 98
#define GLN_POSTFIX_EXPR_NOPARAMS 100
#define GLN_POSTFIX_EXPR_PARAMS 102
#define GLN_POSTFIX_EXPR_DOT 104
#define GLN_POSTFIX_EXPR_ARROW 105
#define GLN_POSTFIX_EXPR_INCR 106
#define GLN_POSTFIX_EXPR_DECR 107
#define GLN_POSTFIX_EXPR_IS 109
#define GLN_POSTFIX_EXPR_CNV 111
#define GLN_POSTFIX_EXPR_DEF 114
#define GLN_STRUCT_DCLTION_LST 357
#define GLN_ENUMERATOR 419
#define GLN_LOCAL_DCLS 612
#define GLN_STMT_LST 631
#define GLN_EXPR_STMT_OPT_EXPR_SEMICOLON 638
#define GLN_SELECTION_STMT_IFSTMT 642
#define GLN_SELECTION_STMT_ELSE 644
#define GLN_ITERATION_STMT_DO_STMT_WHILE_SEMICOLON 669
#define GLN_ITERATION_STMT_FOR 673
#define GLN_FUNC_HEAD_TYP_DCLTION_SPECS_DCLTOR 712

#define IndentInc 3
#define MaxCol 80

#define Max(x,y)        ((x)>(y)?(x):(y))

/*
 * cfile is used to create a list of cfiles created from a source file.
 */
struct cfile {
   char *name;
   struct cfile *next;
   };

/*
 * srcfile is an entry of dependants of a source file.
 */
struct srcfile {
   char *name;
   struct cfile *dependents;
   struct srcfile *next;
   };

#define ForceNl() g_nl = 1;
extern int g_nl;  /* flag: a new-line is needed in the output */

extern int g_switch_level; /* let nodes be aware of their existence in a switch */

/*
 * The lexical analyzer recognizes 3 states. Operators are treated differently
 *  in each state.
 */
#define DfltLex  0    /* Covers most input. */
#define OpHead   1    /* In head of an operator definition. */
#define TypeComp 2    /* In abstract type computation */

extern int lex_state;      /* state of operator recognition */
extern char *compiler_def; /* #define for COMPILER */
extern FILE *g_out_file;     /* output file */
extern int def_fnd;        /* C input defines something concrete */
extern char *inclname;     /* include file to be included by C compiler */
extern int iconx_flg;      /* flag: indicate that iconx style code is needed */
extern char *largeints;    /* "Largeints" or "NoLargeInts" */

/*
 * The symbol table is used by the lexical analyser to decide whether an
 *  identifier is an ordinary identifier, a typedef name, or a reserved
 *  word. It is used by the parse tree builder to decide whether an
 *  identifier is an ordinary C variable, a tended variable, a parameter
 *  to a run-time routine, or the special variable "result".
 */
struct sym_entry {
   int tok_id;	       /* Ident, TokType, or identification of reserved word */
   char *image;		/* image of symbol */
   int id_type;		/* OtherDcl, TndDesc, TndStr, TndBlk, Label, RtParm,
                           DrfPrm, RsltLoc */
   union {
      struct {			/* RtParm: */
         int param_num;		/*   parameter number */
         int cur_loc;		/*   PrmTend, PrmCStr, PrmInt, or PrmDbl */
         int non_tend;		/*   non-tended locations used */
         int parm_mod;          /*   something may have modified it */
         struct sym_entry *next;
         } param_info;
      struct {                  /* TndDesc, TndStr, TndBlk: */
         struct node *init;     /*   initial value from declaration */
         char *blk_name;	/*   TndBlk: struct name of block */
         struct sym_entry *next;
         } tnd_var;
      struct {			/* OtherDcl from "declare {...}": */
         struct node *tqual;	/*   storage class, type qualifier list */
         struct node *dcltor;	/*   declarator */
         struct node *init;     /*   initial value from declaration */
         struct sym_entry *next;
         } declare_var;
      int typ_indx;             /* index into arrays of type information */
      word lbl_num;             /* label number used in in-line code */
      int referenced;		/* RsltLoc: is referenced */
      } u;
   int t_indx;		/* index into tended array */
   int il_indx;		/* index used in in-line code */
   int nest_lvl;	/* 0 - reserved word, 1 - global, >= 2 - local */
   int may_mod;         /* may be modified in particular piece of code */
   int ref_cnt;
   struct sym_entry *next;
   };

/*
 * Path-specific parameter information must be saved and merged for
 *  branching and joining of paths.
 */
struct parminfo {
   int cur_loc;
   int parm_mod;
   };

/*
 * A list is maintained of information needed to initialize tended descriptors.
 */
struct init_tend {
   int t_indx;         /* index into tended array */
   int init_typ;       /* TndDesc, TndStr, TndBlk */
   struct node *init;  /* initial value from declaration */
   int nest_lvl;            /* level of nesting of current use of tended slot */
   int in_use;              /* tended slot is being used in current scope */
   struct init_tend *next;
   };


extern int op_type;                  /* Function, Keyword, Operator, or OrdFunc */
extern char lc_letter;               /* f = function, o = operator, k = keyword */
extern char uc_letter;               /* F = function, O = operator, K = keyword */
extern char prfx1;                   /* 1st char of unique prefix for operation */
extern char prfx2;                   /* 2nd char of unique prefix for operation */
extern char *g_fname;                /* current source file name */
extern int g_line;                   /* current source line number */
extern struct implement *cur_impl;   /* data base entry for current operator */
extern struct token *comment;        /* descriptive comment for current oper */
extern int g_n_tmp_str;              /* total number of string buffers needed */
extern int g_n_tmp_cset;             /* total number of cset buffers needed */
extern int g_nxt_sbuf;               /* index of next string buffer */
extern int g_nxt_cbuf;               /* index of next cset buffer */
extern struct sym_entry *g_params;   /* current list of parameters */
extern struct sym_entry *g_decl_lst; /* declarations from "declare {...}" */
extern struct init_tend *g_tend_lst; /* list of allocated tended slots */
extern char *g_str_rslt;             /* string "result" in string table */
extern word g_lbl_num;               /* next unused label number */
extern struct sym_entry *g_v_len;    /* symbol entry for size of varargs */
extern int g_il_indx;                /* next index into data base symbol table */

/*
 * lvl_entry keeps track of what is happening at a level of nested declarations.
 */
struct lvl_entry {
   int nest_lvl;
   int kind_dcl;	/* IsTypedef, TndDesc, TndStr, TndBlk, or OtherDcl */
   char *blk_name;      /* for TndBlk, the struct name of the block */
   int parms_done;      /* level consists of parameter list which is complete */
   struct sym_entry *tended; /* symbol table entries for tended variables */
   struct lvl_entry *next;
   };

extern struct lvl_entry *g_dcl_stk; /* stack of declaration contexts */

extern int fnc_ret;  /* RetInt, RetDbl, RetNoVal, or RetSig for current func */

#define NoAbstr  -1001 /* no abstract return statement has been encountered */
#define SomeType -1002 /* assume returned value is consistent with abstr ret */
extern int abs_ret; /* type from abstract return statement */

/*
 * Definitions for use in parse tree nodes.
 */

#define PrimryNd  1 /* simply a token */
#define PrefxNd   2 /* a prefix expression */
#define PstfxNd   3 /* a postfix expression */
#define BinryNd   4 /* a binary expression (not necessarily infix) */
#define TrnryNd   5 /* an expression with 3 subexpressions */
#define QuadNd    6 /* an expression with 4 subexpressions */
#define LstNd     7 /* list of declaration parts */
#define CommaNd   8 /* arg lst, declarator lst, or init lst, not comma op */
#define StrDclNd  9 /* structure field declaration */
#define PreSpcNd 10 /* prefix expression that needs a space after it */
#define ConCatNd 11 /* two ajacent pieces of code with no other syntax */
#define SymNd    12 /* a symbol (identifier) node */
#define ExactCnv 13 /* (exact)integer or (exact)C_integer conversion */
#define CompNd   14 /* compound statement */
#define AbstrNd  15 /* abstract type computation */
#define IcnTypNd 16 /* name of an Icon type */

#define NewNode(size) alloc_node(size)

union field {
   struct node *child;
   struct sym_entry *sym;   /* used with SymNd & CompNd*/
   };

struct node {
   struct token *tok;
#ifdef TRACE_NODE_MEMBER
   char *trace;
#endif
   int16_t nd_id;
   int16_t gln; /* grammar line number (in rttgram.y) */
   union field u[1]; /* actual size varies with node type */
   };

#include "rttproto.h"
