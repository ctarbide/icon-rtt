#include "rtt.h"

int g_switch_level = 0;

extern unsigned long trace_count;
extern int can_output_trace(void);

static int node_nfields(struct node *n);

/* order must be in sync with rtt1.h
 */
static char *nd_id_names[] = {
   "0-invalid/error",
   "prim" /* "1-PrimryNd" */, /* simply a token */
   "2-PrefxNd", /* a prefix expression */
   "3-PstfxNd", /* a postfix expression */
   "bin" /* "4-BinryNd" */, /* a binary expression (not necessarily infix) */
   "5-TrnryNd", /* an expression with 3 subexpressions */
   "6-QuadNd", /* an expression with 4 subexpressions */
   "7-LstNd", /* list of declaration parts */
   "8-CommaNd", /* arg lst, declarator lst, or init lst, not comma op */
   "9-StrDclNd", /* structure field declaration */
   "10-PreSpcNd", /* prefix expression that needs a space after it */
   "11-ConCatNd", /* two ajacent pieces of code with no other syntax */
   "12-SymNd", /* a symbol (identifier) node */
   "13-ExactCnv", /* (exact)integer or (exact)C_integer conversion */
   "14-CompNd", /* compound statement */
   "15-AbstrNd", /* abstract type computation */
   "16-IcnTypNd" /* name of an Icon type */
   };

#if defined(TRACE_NODE_MEMBER) && defined(TRACE_NODE_ADD_INFO)
static void token_name(struct token *tok, char *buf, size_t bufsz);
static void token_name(struct token *tok, char *buf, size_t bufsz)
   {
   if (tok) {
      if (32 < tok->tok_id && tok->tok_id < 127)
	 snprintf(buf, bufsz, "'%c'", tok->tok_id);
      else
	 snprintf(buf, bufsz, "%d/\"%s\"", tok->tok_id, tok->image);
      }
   else
      snprintf(buf, bufsz, "nulltok");
   }
#endif

/*
 * node0 - create a syntax tree leaf node.
 */
struct node *node0(id, tok)
int id;
struct token *tok;
   {
   struct node *n;

#ifdef TRACE_NODE
   if (can_output_trace()) {
      fprintf(stderr, "TRACE:%s:%d:%lu: node0(id=%s, tok=\"%s\" (%d)) from %s:%d\n", __FILE__, __LINE__, trace_count,
	 node_id_name(id), tok->image, tok->tok_id, tok->fname, tok->line);
      }
#endif

   n = NewNode(0);
   n->nd_id = id;
   n->tok = tok;
#if defined(TRACE_NODE_MEMBER) && defined(TRACE_NODE_ADD_INFO)
   {
      char buf[100];
      token_name(tok, buf, sizeof(buf));
      n->trace = concat(node_id_name(id), ":", buf, NULL);
      }
#endif
   return n;
   }

/*
 * node0ex - create a syntax tree leaf node.
 */
struct node *node0ex(gln, id, tok)
int gln, id;
struct token *tok;
   {
   struct node *n;

#ifdef TRACE_NODE
   if (can_output_trace()) {
      fprintf(stderr, "TRACE:%s:%d:%lu: node0(id=%s, tok=\"%s\" (%d)) from %s:%d\n", __FILE__, __LINE__, trace_count,
	 node_id_name(id), tok->image, tok->tok_id, tok->fname, tok->line);
      }
#endif

   n = NewNode(0);
   n->gln = gln;
   n->nd_id = id;
   n->tok = tok;
#if defined(TRACE_NODE_MEMBER) && defined(TRACE_NODE_ADD_INFO)
   {
      char buf[100];
      token_name(tok, buf, sizeof(buf));
      n->trace = concat(node_id_name(id), ":", buf, NULL);
      }
#endif
   return n;
   }

/*
 * node1 - create a syntax tree node with one child.
 */
struct node *node1(id, tok, n1)
int id;
struct token *tok;
struct node *n1;
   {
   struct node *n;

#ifdef TRACE_NODE
   if (can_output_trace()) {
      fprintf(stderr, "TRACE:%s:%d:%lu: node1(id=%s) [n1=%s]\n", __FILE__, __LINE__, trace_count,
	 node_id_name(id), node_name(n1));
      }
#endif

   n = NewNode(1);
   n->nd_id = id;
   n->tok = tok;
   n->u[0].child = n1;
#if defined(TRACE_NODE_MEMBER) && defined(TRACE_NODE_ADD_INFO)
   {
      char buf[100];
      token_name(tok, buf, sizeof(buf));
      n->trace = concat(
	 node_id_name(id),
	 ":", buf, ":[",
	 node_name(n1), "]",
	 NULL);
      }
#endif
   return n;
   }

/*
 * node1ex - create a syntax tree node with one child.
 */
struct node *node1ex(gln, id, tok, n1)
int gln, id;
struct token *tok;
struct node *n1;
   {
   struct node *n;

#ifdef TRACE_NODE
   if (can_output_trace()) {
      fprintf(stderr, "TRACE:%s:%d:%lu: node1(id=%s) [n1=%s]\n", __FILE__, __LINE__, trace_count,
	 node_id_name(id), node_name(n1));
      }
#endif

   n = NewNode(1);
   n->gln = gln;
   n->nd_id = id;
   n->tok = tok;
   n->u[0].child = n1;
#if defined(TRACE_NODE_MEMBER) && defined(TRACE_NODE_ADD_INFO)
   {
      char buf[100];
      token_name(tok, buf, sizeof(buf));
      n->trace = concat(
	 node_id_name(id),
	 ":", buf, ":[",
	 node_name(n1), "]",
	 NULL);
      }
#endif
   return n;
   }

/*
 * node2 - create a syntax tree node with two children.
 */
struct node *node2(id, tok, n1, n2)
int id;
struct token *tok;
struct node *n1, *n2;
   {
   struct node *n;

#ifdef TRACE_NODE
   if (can_output_trace()) {
      fprintf(stderr, "TRACE:%s:%d:%lu: node2(id=%s) [n1=%s, n2=%s]\n", __FILE__, __LINE__, trace_count,
	 node_id_name(id), node_name(n1), node_name(n2));
   }
#endif

   n = NewNode(2);
   n->nd_id = id;
   n->tok = tok;
   n->u[0].child = n1;
   n->u[1].child = n2;
#if defined(TRACE_NODE_MEMBER) && defined(TRACE_NODE_ADD_INFO)
   if (tok) {
      char buf[100];
      token_name(tok, buf, sizeof(buf));
      if (id == CommaNd) {
	 n->trace = concat(
	    "[", node_name(n1), "]",
	    buf /* always ',' according to rttgram.y */,
	    "[", node_name(n2), "]",
	    NULL);
	 }
      else {
	 n->trace = concat(
	    node_id_name(id),
	    ":", buf, ":[",
	    node_name(n1), "]:[",
	    node_name(n2), "]",
	    NULL);
	 }
      }
   else {
      if (id == LstNd) {
	 n->trace = concat(
	    "list:[",
	    node_name(n1), "]:[",
	    node_name(n2), "]",
	    NULL);
	 }
      else if (id == ConCatNd) {
	 n->trace = concat(
	    "cat:[",
	    node_name(n1), "]:[",
	    node_name(n2), "]",
	    NULL);
	 }
      else {
	 n->trace = concat(
	    node_id_name(id),
	    "::[",
	    node_name(n1), "]:[",
	    node_name(n2), "]",
	    NULL);
	 }
      }
#endif
   return n;
   }

/*
 * node2ex - create a syntax tree node with two children.
 */
struct node *node2ex(gln, id, tok, n1, n2)
int gln, id;
struct token *tok;
struct node *n1, *n2;
   {
   struct node *n;

#ifdef TRACE_NODE
   if (can_output_trace()) {
      fprintf(stderr, "TRACE:%s:%d:%lu: node2(id=%s,gln=%d) [n1=%s, n2=%s]\n", __FILE__, __LINE__, trace_count,
	 node_id_name(id), gln, node_name(n1), node_name(n2));
   }
#endif

   n = NewNode(2);
   n->gln = gln;
   n->nd_id = id;
   n->tok = tok;
   n->u[0].child = n1;
   n->u[1].child = n2;
#if defined(TRACE_NODE_MEMBER) && defined(TRACE_NODE_ADD_INFO)
   if (tok) {
      char buf[100];
      token_name(tok, buf, sizeof(buf));
      if (id == CommaNd) {
	 n->trace = concat(
	    "[", node_name(n1), "]",
	    buf /* always ',' according to rttgram.y */,
	    "[", node_name(n2), "]",
	    NULL);
	 }
      else {
	 n->trace = concat(
	    node_id_name(id),
	    ":", buf, ":[",
	    node_name(n1), "]:[",
	    node_name(n2), "]",
	    NULL);
	 }
      }
   else {
      if (id == LstNd) {
	 n->trace = concat(
	    "list:[",
	    node_name(n1), "]:[",
	    node_name(n2), "]",
	    NULL);
	 }
      else if (id == ConCatNd) {
	 n->trace = concat(
	    "cat:[",
	    node_name(n1), "]:[",
	    node_name(n2), "]",
	    NULL);
	 }
      else {
	 n->trace = concat(
	    node_id_name(id),
	    "::[",
	    node_name(n1), "]:[",
	    node_name(n2), "]",
	    NULL);
	 }
      }
#endif
   return n;
   }

/*
 * node3 - create a syntax tree node with three children.
 */
struct node *node3(id, tok, n1, n2, n3)
int id;
struct token *tok;
struct node *n1, *n2, *n3;
   {
   struct node *n;

#ifdef TRACE_NODE
   if (can_output_trace()) {
      fprintf(stderr, "TRACE:%s:%d:%lu: node3(id=%s) [n1=%s, n2=%s, n3=%s]\n", __FILE__, __LINE__, trace_count,
	 node_id_name(id), node_name(n1), node_name(n2), node_name(n3));
   }
#endif

   n = NewNode(3);
   n->nd_id = id;
   n->tok = tok;
   n->u[0].child = n1;
   n->u[1].child = n2;
   n->u[2].child = n3;
#if defined(TRACE_NODE_MEMBER) && defined(TRACE_NODE_ADD_INFO)
   if (tok) {
      char buf[100];
      token_name(tok, buf, sizeof(buf));
      n->trace = concat(
	 node_id_name(id),
	 ":", buf, ":[",
	 node_name(n1), "]:[",
	 node_name(n2), "]:[",
	 node_name(n3), "]",
	 NULL);
      }
   else {
      n->trace = concat(
	 node_id_name(id),
	 "::[",
	 node_name(n1), "]:[",
	 node_name(n2), "]:[",
	 node_name(n3), "]",
	 NULL);
      }
#endif
   return n;
   }

/*
 * node3ex - create a syntax tree node with three children.
 */
struct node *node3ex(gln, id, tok, n1, n2, n3)
int gln, id;
struct token *tok;
struct node *n1, *n2, *n3;
   {
   struct node *n;

#ifdef TRACE_NODE
   if (can_output_trace()) {
      fprintf(stderr, "TRACE:%s:%d:%lu: node3(id=%s) [n1=%s, n2=%s, n3=%s]\n", __FILE__, __LINE__, trace_count,
	 node_id_name(id), node_name(n1), node_name(n2), node_name(n3));
   }
#endif

   n = NewNode(3);
   n->gln = gln;
   n->nd_id = id;
   n->tok = tok;
   n->u[0].child = n1;
   n->u[1].child = n2;
   n->u[2].child = n3;
#if defined(TRACE_NODE_MEMBER) && defined(TRACE_NODE_ADD_INFO)
   if (tok) {
      char buf[100];
      token_name(tok, buf, sizeof(buf));
      n->trace = concat(
	 node_id_name(id),
	 ":", buf, ":[",
	 node_name(n1), "]:[",
	 node_name(n2), "]:[",
	 node_name(n3), "]",
	 NULL);
      }
   else {
      n->trace = concat(
	 node_id_name(id),
	 "::[",
	 node_name(n1), "]:[",
	 node_name(n2), "]:[",
	 node_name(n3), "]",
	 NULL);
      }
#endif
   return n;
   }

/*
 * node4 - create a syntax tree node with four children.
 */
struct node *node4(id, tok, n1, n2, n3, n4)
int id;
struct token *tok;
struct node *n1, *n2, *n3, *n4;
   {
   struct node *n;

#ifdef TRACE_NODE
   if (can_output_trace()) {
      fprintf(stderr, "TRACE:%s:%d:%lu: node4(id=%s) [n1=%s, n2=%s, n3=%s, n4=%s]\n", __FILE__, __LINE__, trace_count,
	 node_id_name(id), node_name(n1), node_name(n2), node_name(n3), node_name(n4));
   }
#endif

   n = NewNode(4);
   n->nd_id = id;
   n->tok = tok;
   n->u[0].child = n1;
   n->u[1].child = n2;
   n->u[2].child = n3;
   n->u[3].child = n4;
   return n;
   }

/*
 * node4ex - create a syntax tree node with four children.
 */
struct node *node4ex(gln, id, tok, n1, n2, n3, n4)
int gln, id;
struct token *tok;
struct node *n1, *n2, *n3, *n4;
   {
   struct node *n;

#ifdef TRACE_NODE
   if (can_output_trace()) {
      fprintf(stderr, "TRACE:%s:%d:%lu: node4(id=%s) [n1=%s, n2=%s, n3=%s, n4=%s]\n", __FILE__, __LINE__, trace_count,
	 node_id_name(id), node_name(n1), node_name(n2), node_name(n3), node_name(n4));
   }
#endif

   n = NewNode(4);
   n->gln = gln;
   n->nd_id = id;
   n->tok = tok;
   n->u[0].child = n1;
   n->u[1].child = n2;
   n->u[2].child = n3;
   n->u[3].child = n4;
   return n;
   }

/*
 * sym_node - create a syntax tree node for a variable. If the identifier
 *  is in the symbol table, create a node that references the entry,
 *  otherwise create a simple leaf node.
 */
struct node *sym_node(tok)
struct token *tok;
   {
   struct sym_entry *sym;
   struct node *n;

#ifdef TRACE_NODE
   if (can_output_trace()) {
      fprintf(stderr, "TRACE:%s:%d:%lu: sym_node(tok=%d \"%s\")\n", __FILE__, __LINE__, trace_count,
	 tok->tok_id, tok->image);
   }
#endif

   sym = sym_lkup(tok->image);
   if (sym != NULL) {
      n = NewNode(1);
      n->nd_id = SymNd;
      n->tok = tok;
      n->u[0].sym = sym;
      ++sym->ref_cnt;
      /*
       * If this is the result location of an operation, note that it
       *  is explicitly referenced.
       */
      if (sym->id_type == RsltLoc)
         sym->u.referenced = 1;
#if defined(TRACE_NODE_MEMBER) && defined(TRACE_NODE_ADD_INFO)
      {
	 char buf[100];
	 token_name(tok, buf, sizeof(buf));
	 n->trace = concat("sym:", buf, NULL);
	 }
#endif
      return n;
      }
   else
      return node0(PrimryNd, tok);
   }

/*
 * comp_nd - create a node for a compound statement.
 */
struct node *comp_nd(gln, tok, dcls, stmts)
int gln;
struct token *tok;
struct node *dcls;
struct node *stmts;
   {
   struct node *n;

#ifdef TRACE_NODE
   if (can_output_trace()) {
      fprintf(stderr, "TRACE:%s:%d:%lu: comp_nd(tok=%d \"%s\") [dcls=%s, stmts=%s]\n", __FILE__, __LINE__, trace_count,
	 tok->tok_id, tok->image, node_name(dcls), node_name(stmts));
   }
#endif

   n = NewNode(3);
   n->gln = gln;
   n->nd_id = CompNd;
   n->tok = tok;
   n->u[0].child = dcls;
   n->u[1].sym = g_dcl_stk->tended; /* tended declarations are not in dcls */
   n->u[2].child = stmts;

#if defined(TRACE_NODE_MEMBER) && defined(TRACE_NODE_ADD_INFO)
   {
      char buf[100];
      token_name(tok, buf, sizeof(buf));
      n->trace = concat(
	 "CompNd:", buf, ":[",
	 node_name(dcls), "]:[",
	 sym_name(g_dcl_stk->tended), "]:[",
	 node_name(stmts), "]",
	 NULL);
      }
#endif
   return n;
   }

/*
 * arith_nd - create a node for an arith_case statement.
 */
struct node *arith_nd(tok, p1, p2, c_int, ci_act, intgr, i_act, dbl, d_act)
struct token *tok;
struct node *p1, *p2;
struct node *c_int, *ci_act;
struct node *intgr, *i_act;
struct node *dbl, *d_act;
   {
   struct node *n;

#ifdef TRACE_NODE
   if (can_output_trace()) {
      fprintf(stderr, "TRACE:%s:%d:%lu: arith_nd(tok=%d \"%s\")\n", __FILE__, __LINE__, trace_count,
	 tok->tok_id, tok->image);
   }
#endif

   /*
    * Ensure the cases are what we expect.
    */
   if (c_int->tok->tok_id != C_Integer)
      errt3(c_int->tok, "expected \"C_integer\", found \"", c_int->tok->image,
         "\"");
   if (intgr->tok->image != icontypes[int_typ].id)
      errt3(intgr->tok, "expected \"integer\", found \"", intgr->tok->image,
         "\"");
   if (dbl->tok->tok_id != C_Double)
      errt3(dbl->tok, "expected \"C_double\", found \"", dbl->tok->image,
         "\"");

   /*
    * Indicate in the symbol table that the arguments are converted to C
    *  values.
    */
   dst_alloc(c_int, p1);
   dst_alloc(c_int, p2);
   dst_alloc(dbl, p1);
   dst_alloc(dbl, p2);

   free_tree(c_int);
   free_tree(intgr);
   free_tree(dbl);

   n = node3(TrnryNd, NULL, ci_act, i_act, d_act);
   return node3(TrnryNd, tok, p1, p2, n);
   }

struct node *dest_node(tok)
struct token *tok;
   {
   struct node *n;
   int typcd;

#ifdef TRACE_NODE
   if (can_output_trace()) {
      fprintf(stderr, "TRACE:%s:%d:%lu: dest_node(tok=%d \"%s\")\n", __FILE__, __LINE__, trace_count,
	 tok->tok_id, tok->image);
   }
#endif

   n = sym_node(tok);
   typcd = n->u[0].sym->u.typ_indx;
   if (typcd != int_typ && typcd != str_typ && typcd != cset_typ &&
      typcd != real_typ)
      errt2(tok, "cannot convert to ", tok->image);
   return n;
   }

/*
 * free_tree - free storage for a syntax tree.
 */
void free_tree(n)
struct node *n;
   {
   struct sym_entry *sym, *sym1;

   if (n == NULL)
      return;

   /*
    * Free any subtrees and other referenced storage.
    */
   switch (n->nd_id) {
      case SymNd:
         free_sym(n->u[0].sym); /* Indicate one less reference to symbol */
         break;

      case CompNd:
         /*
          * Compound node. Free ordinary declarations, tended declarations,
          *  and executable code.
          */
         free_tree(n->u[0].child);
         sym = n->u[1].sym;
         while (sym != NULL) {
            sym1 = sym;
            sym = sym->u.tnd_var.next;
            free_sym(sym1);
            }
         free_tree(n->u[2].child);
         break;

      case QuadNd:
         free_tree(n->u[3].child);
         /* fall thru to next case */
      case TrnryNd:
         free_tree(n->u[2].child);
         /* fall thru to next case */
      case AbstrNd:
      case BinryNd:
      case CommaNd:
      case ConCatNd:
      case LstNd:
      case StrDclNd:
         free_tree(n->u[1].child);
         /* fall thru to next case */
      case IcnTypNd:
      case PstfxNd:
      case PreSpcNd:
      case PrefxNd:
         free_tree(n->u[0].child);
         /* fall thru to next case */
      case ExactCnv:
      case PrimryNd:
         break;

      default:
	 fprintf(stderr, "Exhaustion %s:%d.\n", __FILE__, __LINE__);
	 exit(1);
         }
   free_t(n->tok);             /* free token */
   free((char *)n);
   }

static int node_nfields(n)
struct node *n;
   {
   switch (n->nd_id) {
      case ExactCnv:
      case PrimryNd:
	 return 0;

      case SymNd:
      case IcnTypNd:
      case PstfxNd:
      case PreSpcNd:
      case PrefxNd:
	 return 1;

      case AbstrNd:
      case BinryNd:
      case CommaNd:
      case ConCatNd:
      case LstNd:
      case StrDclNd:
	 return 2;

      case TrnryNd:
      case CompNd:
	 return 3;

      case QuadNd:
	 return 4;

      default:
	 fprintf(stderr, "Exhaustion %s:%d.\n", __FILE__, __LINE__);
	 exit(1);
      }
   }

struct node *copy_tree(n)
struct node *n;
   {
   struct node *res;

   if (n == NULL)
      return NULL;

   res = NewNode(node_nfields(n));
   res->gln = n->gln;
   res->nd_id = n->nd_id;
   res->tok = copy_t(n->tok);
#ifdef TRACE_NODE_MEMBER
   res->trace = salloc(n->trace);
#endif

   switch (n->nd_id) {
      case QuadNd:
         res->u[3].child = copy_tree(n->u[3].child);
         /* fall thru to next case */
      case TrnryNd:
         res->u[2].child = copy_tree(n->u[2].child);
         /* fall thru to next case */
      case AbstrNd:
      case BinryNd:
      case CommaNd:
      case ConCatNd:
      case LstNd:
      case StrDclNd:
         res->u[1].child = copy_tree(n->u[1].child);
         /* fall thru to next case */
      case IcnTypNd:
      case PstfxNd:
      case PreSpcNd:
      case PrefxNd:
         res->u[0].child = copy_tree(n->u[0].child);
         /* fall thru to next case */
      case ExactCnv:
      case PrimryNd:
         break;

      case SymNd:
	 fprintf(stderr, "Exhaustion %s:%d.\n", __FILE__, __LINE__);
	 exit(1);
         break;

      case CompNd:
	 fprintf(stderr, "Exhaustion %s:%d.\n", __FILE__, __LINE__);
	 exit(1);
         break;

      default:
	 fprintf(stderr, "Exhaustion %s:%d.\n", __FILE__, __LINE__);
	 exit(1);
         }
   return res;
   }

const char *node_id_name(nd_id)
int nd_id;
   {
   return nd_id_names[nd_id];
   }

const char *node_name(nd)
struct node *nd;
   {
   if (nd) {
#ifdef TRACE_NODE_MEMBER
      if (nd->trace) {
	 return nd->trace;
	 }
#endif
      return node_id_name(nd->nd_id);
      }
   return "null";
   }

struct node *alloc_node(nfields)
size_t nfields;
   {
   size_t allocsz = sizeof(struct node) - sizeof(union field) + (nfields * sizeof(union field));
   struct node *res = alloc(allocsz);
   return res;
   }

#if defined(TRACE_NODE_MEMBER) && defined(TRACE_NODE_ADD_INFO)
void node_update_trace(n)
struct node *n;
   {
   char buf[100];
   if (n == NULL)
      return;
   switch (n->nd_id) {
      case ExactCnv:
      case PrimryNd:
	 token_name(n->tok, buf, sizeof(buf));
	 if (n->trace)
	    free(n->trace);
	 n->trace = concat(node_id_name(n->nd_id), ":", buf, NULL);
	 return;

      case IcnTypNd:
      case PstfxNd:
      case PreSpcNd:
      case PrefxNd:
	 node_update_trace(n->u[0].child);
	 token_name(n->tok, buf, sizeof(buf));
	 if (n->trace)
	    free(n->trace);
	 n->trace = concat(
	    node_id_name(n->nd_id),
	    ":", buf, ":[",
	    node_name(n->u[0].child), "]",
	    NULL);
	 return;

      case LstNd:
	 node_update_trace(n->u[0].child);
	 node_update_trace(n->u[1].child);
	 if (n->trace)
	    free(n->trace);
	 n->trace = concat(
	    "list:[",
	    node_name(n->u[0].child), "]:[",
	    node_name(n->u[1].child), "]",
	    NULL);
	 return;

      case ConCatNd:
	 if (n->tok) {
	    fprintf(stderr, "Exhaustion %s:%d.\n", __FILE__, __LINE__);
	    exit(1);
	    }
	 node_update_trace(n->u[0].child);
	 node_update_trace(n->u[1].child);
	 if (n->trace)
	    free(n->trace);
	 n->trace = concat(
	    "cat:[",
	    node_name(n->u[0].child), "]:[",
	    node_name(n->u[1].child), "]",
	    NULL);
	 return;

      case AbstrNd:
      case BinryNd:
      case StrDclNd:
	 node_update_trace(n->u[0].child);
	 node_update_trace(n->u[1].child);
	 token_name(n->tok, buf, sizeof(buf));
	 if (n->trace)
	    free(n->trace);
	 n->trace = concat(
	    node_id_name(n->nd_id),
	    ":", buf, ":[",
	    node_name(n->u[0].child), "]:[",
	    node_name(n->u[1].child), "]",
	    NULL);
	 return;

      case CommaNd:
	 node_update_trace(n->u[0].child);
	 node_update_trace(n->u[1].child);
	 token_name(n->tok, buf, sizeof(buf));
	 if (n->trace)
	    free(n->trace);
	 n->trace = concat(
	    "[", node_name(n->u[0].child), "]",
	    buf /* always ',' according to rttgram.y */,
	    "[", node_name(n->u[1].child), "]",
	    NULL);
	 return;

      case TrnryNd:
	 node_update_trace(n->u[0].child);
	 node_update_trace(n->u[1].child);
	 node_update_trace(n->u[2].child);
	 token_name(n->tok, buf, sizeof(buf));
	 if (n->trace)
	    free(n->trace);
	 n->trace = concat(
	    node_id_name(n->nd_id),
	    ":", buf, ":[",
	    node_name(n->u[0].child), "]:[",
	    node_name(n->u[1].child), "]:[",
	    node_name(n->u[2].child), "]",
	    NULL);
	 return;

      case SymNd:
	 if (n->trace)
	    free(n->trace);
	 token_name(n->tok, buf, sizeof(buf));
	 n->trace = concat("sym:", buf, NULL);
	 return;

      case CompNd:
	 token_name(n->tok, buf, sizeof(buf));
	 n->trace = concat(
	    "CompNd:", buf, ":[",
	    node_name(n->u[0].child), "]:[",
	    sym_name(n->u[1].sym), "]:[",
	    node_name(n->u[2].child), "]",
	    NULL);
	 return;

      case QuadNd:
	 fprintf(stderr, "Exhaustion %s:%d.\n", __FILE__, __LINE__);
	 exit(1);
	 return;

      default:
	 fprintf(stderr, "Exhaustion %s:%d.\n", __FILE__, __LINE__);
	 exit(1);
      }
   }
#else
void node_update_trace(n) struct node *n; {}
#endif

const char *
sym_name(sym)
struct sym_entry *sym;
   {
   return sym ? sym->image : "nullsym";
   }

int
is_n(n, nd_id)
struct node *n;
int nd_id;
   {
   return n && n->nd_id == nd_id && n->tok == NULL;
   }

int
is_t(n, nd_id, tok_id)
struct node *n;
int nd_id, tok_id;
   {
   return n && n->nd_id == nd_id && n->tok && n->tok->tok_id == tok_id;
   }

struct node *
nav_n(n, nd_id, child)
struct node *n;
int nd_id, child;
   {
   if (n && n->nd_id == nd_id && n->tok == NULL)
      return n->u[child].child;
   return NULL;
   }

struct node *
nav_t(n, nd_id, tok_id, child)
struct node *n;
int nd_id, tok_id, child;
   {
   if (n && n->nd_id == nd_id && n->tok && n->tok->tok_id == tok_id)
      return n->u[child].child;
   return NULL;
   }

struct node *
nav_n_n(n, nd_id1, child1, nd_id2, child2)
struct node *n;
int nd_id1, nd_id2, child1, child2;
   {
   struct node *nd;
   if ((nd = nav_n(n, nd_id1, child1)))
      return nav_n(nd, nd_id2, child2);
   return NULL;
   }

struct node *
nav_n_t(n, nd_id1, child1, nd_id2, tok_id2, child2)
struct node *n;
int nd_id1, nd_id2, tok_id2, child1, child2;
   {
   struct node *nd;
   if ((nd = nav_n(n, nd_id1, child1)))
      return nav_t(nd, nd_id2, tok_id2, child2);
   return NULL;
   }

struct node *
nav_n_n_t(n, nd_id1, child1, nd_id2, child2, nd_id3, tok_id3, child3)
struct node *n;
int nd_id1, nd_id2, nd_id3, tok_id3;
int child1, child2, child3;
   {
   struct node *nd1, *nd2;
   if ((nd1 = nav_n(n, nd_id1, child1)))
      if ((nd2 = nav_n_t(nd1, nd_id2, child2, nd_id3, tok_id3, child3)))
	 return nd2;
   return NULL;
   }
