/*
 * This file contains various functions for dealing with macros.
 */
#include "../preproc/preproc.h"
#include "../preproc/ptoken.h"
#include "../preproc/pproto.h"

static struct str_buf sbuf_macro[1];
#define sbuf sbuf_macro

/* g_line is for the output, g_line_for___LINE__ is for the input
 */
int g_line_for___LINE__ = 0;
char *g_fname_for___FILE__ = "";

/*
 * Prototypes for static functions.
 */
static struct macro     **m_find      (char *mname);
static int            eq_id_lst   (struct id_lst *lst1, struct id_lst *lst2);
static int            eq_tok_lst  (struct tok_lst *lst1, struct tok_lst *lst2);
static int            parm_indx   (char *id, struct macro *m);
static void            cpy_str     (char *ldelim, char *image,
					char *rdelim, struct str_buf *sbuf);
static struct token      *stringize   (struct token *trigger,
					struct mac_expand *me);
static struct paste_lsts *paste_parse (struct token *t,
					struct mac_expand *me);
static int               *cpy_image   (struct token *t, int *s);

#define MacTblSz 149
#define MHash(x) (((unsigned int)(unsigned long)(x)) % MacTblSz)

static struct macro *m_table[MacTblSz];	/* hash table of macros */

int max_recurse;

/*
 * Some string to put in the string table:
 */
static char *line_mac = "__LINE__";
static char *file_mac = "__FILE__";
static char *date_mac = "__DATE__";
static char *time_mac = "__TIME__";
static char *rcrs_mac = "__RCRS__";
static char *defined = "defined";

/*
 * m_find - return return location of pointer to where macro belongs in
 *  macro table. If the macro is not in the table, the pointer at the
 *  location is NULL.
 */
static struct macro **m_find(mname)
char *mname;
   {
   struct macro **mpp;

   for (mpp = &m_table[MHash(mname)]; *mpp && (*mpp)->mname != mname;
      mpp = &(*mpp)->next)
      ;
   return mpp;
   }

/*
 * eq_id_lst - check to see if two identifier lists contain the same identifiers
 *  in the same order.
 */
static int eq_id_lst(lst1, lst2)
struct id_lst *lst1;
struct id_lst *lst2;
   {
   if (lst1 == lst2)
      return 1;
   if (lst1 == NULL || lst2 == NULL)
      return 0;
   if (lst1->id != lst2->id)
      return 0;
   return eq_id_lst(lst1->next, lst2->next);
   }

/*
 * eq_tok_lst - check to see if 2 token lists contain the same tokens
 *  in the same order. All white space tokens are considered equal.
 */
static int eq_tok_lst(lst1, lst2)
struct tok_lst *lst1, *lst2;
   {
   if (lst1 == lst2)
      return 1;
   if (lst1 == NULL || lst2 == NULL)
      return 0;
   if (lst1->t->tok_id != lst2->t->tok_id)
      return 0;
   if (lst1->t->tok_id != WhiteSpace && lst1->t->tok_id != PpDirEnd &&
	lst1->t->image != lst2->t->image)
      return 0;
   return eq_tok_lst(lst1->next, lst2->next);
   }

/*
 * init_macro - initialize this module, setting up standard macros.
 */
void init_macro()
   {
   int i;
   struct macro **mpp;
   struct token *t;
   time_t tv;
   char *s, *tval;
   static char *time_buf;
   static char *date_buf;
   static short first_time = 1;

   if (first_time) {
      first_time = 0;
      /*
       * Add names of standard macros to sting table.
       */
      line_mac = spec_str(line_mac);
      file_mac = spec_str(file_mac);
      date_mac = spec_str(date_mac);
      time_mac = spec_str(time_mac);
      rcrs_mac = spec_str(rcrs_mac);
      defined = spec_str(defined);
      }
   else {
      /*
       * Free macro definitions from the file processed.
       */
      for (i = 0; i < MacTblSz; ++i)
	 free_m_lst(m_table[i]);
      }

   for (i = 0; i < MacTblSz; ++i)
      m_table[i] = NULL;

   /*
    * __LINE__ and __FILE__ are macros that require special processing
    *  when they are processed. Indicate that.
    */
   mpp = m_find(line_mac);
   *mpp = new_macro(line_mac, SpecMac, 0, NULL, NULL);

   mpp = m_find(file_mac);
   *mpp = new_macro(file_mac, SpecMac, 0, NULL, NULL);

   /*
    * __TIME__ and __DATE__ must be initialized to the current time and
    *  date.
    */
   time(&tv);
   tval = ctime(&tv);
   date_buf = alloc(12);
   time_buf = alloc(9);
   s = date_buf;
   for (i = 4; i <= 10; ++i)
      *s++ = tval[i];
   for (i = 20; i <= 23; ++i)
      *s++ = tval[i];
   *s = '\0';
   s = time_buf;
   for (i = 11; i <= 18; ++i)
      *s++ = tval[i];
   *s = '\0';
   date_buf = spec_str(date_buf);
   time_buf = spec_str(time_buf);

   t = new_token(StrLit, date_buf, "", 0);
   mpp = m_find(date_mac);
   *mpp = new_macro(date_mac, FixedMac, 0, NULL, new_t_lst(t));

   t = new_token(StrLit, time_buf, "", 0);
   mpp = m_find(time_mac);
   *mpp = new_macro(time_mac, FixedMac, 0, NULL, new_t_lst(t));

   /*
    * __RCRS__ is a special macro to indicate the allowance of
    *  recursive macros. It is not ANSI-standard. Initialize it
    *  to "1".
    */
   mpp = m_find(rcrs_mac);
   *mpp = new_macro(rcrs_mac, NoArgs, 0, NULL, new_t_lst(copy_t(one_tok)));
   max_recurse = 1;
   }

/*
 * m_install - install a macro.
 */
struct macro *m_install(mname, category, multi_line, prmlst, body)
struct token *mname;	/* name of macro */
int multi_line;		/* flag indicating if this is a multi-line macro */
int category;		/* # parms, or NoArgs if it is object-like macro */
struct id_lst *prmlst;	/* parameter list */
struct tok_lst *body;	/* replacement list */
   {
   struct macro **mpp;
   char *s;

   if (mname->image == defined)
      errt1(mname, "'defined' may not be the subject of #define");

   /*
    * The special macro __RCRS__ may only be defined as a single integer
    *  token and must be an object-like macro.
    */
   if (mname->image == rcrs_mac) {
      if (body == NULL || body->t->tok_id != PpNumber || body->next != NULL)
	 errt1(mname, "__RCRS__ must be a decimal integer");
      if (category != NoArgs)
	 errt1(mname, "__RSCS__ may have no arguments");
      max_recurse = 0;
      for (s = body->t->image; *s != '\0'; ++s) {
	 if (*s >= '0' && *s <= '9')
	    max_recurse = max_recurse * 10 + (*s - '0');
	 else
	    errt1(mname, "__RCRS__ must be a decimal integer");
	 }
      }

   mpp = m_find(mname->image);
   if (*mpp == NULL)
      *mpp = new_macro(mname->image, category, multi_line, prmlst, body);
    else {
      /*
       * The macro is already defined. Make sure it is identical (up to
       *  white space) to this definition.
       */
      if (!((*mpp)->category == category && eq_id_lst((*mpp)->prmlst, prmlst) &&
	   eq_tok_lst((*mpp)->body, body)))
	 errt2(mname, "invalid redefinition of macro ", mname->image);
      free_id_lst(prmlst);
      free_t_lst(body);
      }
   return *mpp;
   }

/*
 * m_uninstall - remove a macro from hash table, but don't free it.
 */
struct macro *m_uninstall(mname)
struct token *mname;
   {
   struct macro **mpp, *mp;

   if (mname->image == defined)
      errt1(mname, "'defined' may not be the subject of #undef");

   /*
    * Undefining __RCRS__ allows unlimited macro recursion (non-ANSI
    *  standard feature.
    */
   if (mname->image == rcrs_mac)
      max_recurse = -1;

   /*
    * Make sure undefining this macro is allowed, and free storage
    *  associate with it.
    */
   mpp = m_find(mname->image);
   if (*mpp) {
      mp = *mpp;
      if (mp->category == FixedMac || mp->category == SpecMac)
	 errt2(mname, mname->image, " may not be the subject of #undef");
      *mpp = mp->next;
      mp->next = NULL;
      }
   else
      mp = NULL;
   return mp;
   }

/*
 * m_delete - delete a macro.
 */
void m_delete(mname)
struct token *mname;
   {
   struct macro *mp;
   if ((mp = m_uninstall(mname)))
      free(mp);
   }

/*
 * m_lookup - lookup a macro name. Return pointer to macro, if it is defined;
 *  return NULL, if it is not. This routine sets the definition for macros
 *  whose definitions varies from place to place.
 */
struct macro *m_lookup(id)
struct token *id;
   {
   struct macro *m;
   static char buf[20];

   m = *m_find(id->image);
   if (m && m->category == SpecMac) {
      int ln = g_line_for___LINE__;
      char *fn = g_fname_for___FILE__;
      if (m->mname == line_mac) {  /* __LINE___ */
	 sprintf(buf, "%d", ln);
	 m->body = new_t_lst(new_token(PpNumber, buf, fn, ln));
	 }
      else if (m->mname == file_mac) /* __FILE__ */
	 m->body = new_t_lst(new_token(StrLit, fn, fn, ln));
      }
   return m;
   }

/*
 * parm_indx - see if a name is a paramter to the given macro.
 */
static int parm_indx(id, m)
char *id;
struct macro *m;
   {
   struct id_lst *idlst;
   int i;

   for (i = 0, idlst = m->prmlst; i < m->category; i++, idlst = idlst->next)
      if (id == idlst->id)
	 return i;
   return -1;
   }

/*
 * cpy_str - copy a string into a string buffer, adding delimiters.
 */
static void cpy_str(ldelim, image, rdelim, buf)
char *ldelim;
char *image;
char *rdelim;
struct str_buf *buf;
   {
   char *s;

   for (s = ldelim; *s != '\0'; ++s)
      AppChar(buf, *s);

   for (s = image; *s != '\0'; ++s) {
      if (*s == '\\' || *s == '"')
	 AppChar(buf, '\\');
      AppChar(buf, *s);
      }

   for (s = rdelim; *s != '\0'; ++s)
      AppChar(buf, *s);
   }

/*
 * stringize - create a stringized version of a token.
 */
static struct token *stringize(trigger, me)
struct token *trigger;
struct mac_expand *me;
   {
   struct token *t;
   struct tok_lst *arg;
   char *s;
   int indx;

   /*
    * Get the next token from the macro body. It must be a macro parameter;
    *  retrieve the raw tokens for the corresponding argument.
    */
   if (me->rest_bdy == NULL)
      errt1(trigger, "the # operator must have an argument");
   t = me->rest_bdy->t;
   me->rest_bdy = me->rest_bdy->next;
   if (t->tok_id == Identifier)
      indx = parm_indx(t->image, me->m);
   else
      indx = -1;
   if (indx == -1)
      errt1(t, "the # operator may only be applied to a macro argument");
   arg = me->args[indx];

   /*
    * Copy the images for the argument tokens into a string buffer. Note
    *  that the images of string and character literals lack quotes; these
    *  must be escaped in the stringized value.
    */
   init_sbuf(sbuf);
   while (arg != NULL) {
      t = arg->t;
      if (t->tok_id == WhiteSpace)
	 AppChar(sbuf, ' ');
      else if (t->tok_id == StrLit)
	 cpy_str("\\\"", t->image, "\\\"", sbuf);
      else if (t->tok_id == LStrLit)
	 cpy_str("L\\\"", t->image, "\\\"", sbuf);
      else if (t->tok_id == CharConst)
	 cpy_str("'", t->image, "'", sbuf);
      else if (t->tok_id == LCharConst)
	 cpy_str("L'", t->image, "'", sbuf);
      else
	 for (s = t->image; *s != '\0'; ++s)
	    AppChar(sbuf, *s);
      arg = arg->next;
      }

   /*
    * Created the token for the stringized argument.
    */
   t = new_token(StrLit, str_install(sbuf), trigger->fname, trigger->line);
   t->flag |= trigger->flag & LineChk;
   return t;
   }

/*
 * paste_parse - parse an expression involving token pasting operators (and
 *  stringizing operators). Return a list of token lists. Each token list
 *  is from a token pasting operand, with operands that are macro parameters
 *  replaced by their corresponding argument (this is why a list of tokens
 *  is needed for each operand). Any needed stringizing is done as the list
 *  is created.
 */
static struct paste_lsts *paste_parse(t, me)
struct token *t;
struct mac_expand *me;
   {
   struct token *t1;
   struct token *trigger = NULL;
   struct tok_lst *lst;
   struct paste_lsts *plst;
   int indx;

   if (me->rest_bdy == NULL || me->rest_bdy->t->tok_id != PpPaste)
      plst = NULL;  /* we have reached the end of the pasting expression */
   else {
      /*
       * The next token is a pasting operator. Copy it an move on to the
       *  operand.
       */
      trigger = copy_t(me->rest_bdy->t);
      me->rest_bdy = me->rest_bdy->next;
      if (me->rest_bdy == NULL)
	 errt1(t, "the ## operator must not appear at the end of a macro");
      t1 = me->rest_bdy->t;
      me->rest_bdy = me->rest_bdy->next;

      /*
       * See if the operand is a stringizing operation.
       */
      if (t1->tok_id == '#')
	 t1 = stringize(t1, me);
      else
	 t1 = copy_t(t1);
      plst = paste_parse(t1, me); /* get any further token pasting */
      }

   /*
    * If the operand is a macro parameter, replace it by the corresponding
    *  argument, otherwise make the operand into a 1-element token list.
    */
   indx = -1;
   if (t->tok_id == Identifier)
      indx = parm_indx(t->image, me->m);
   if (indx == -1)
      lst = new_t_lst(t);
   else {
      lst = me->args[indx];
      free_t(t);
      }

   /*
    * Ignore emtpy arguments when constructing the pasting list.
    */
   if (lst == NULL)
      return plst;
   else
      return new_plsts(trigger, lst, plst);
   }

/*
 * cpy_image - copy the image of a token into a character buffer adding
 *  delimiters if it is a string or character literal.
 */
static int *cpy_image(t, s)
struct token *t;
int *s;          /* the string buffer can contain EOF */
   {
   char *s1;

   switch (t->tok_id) {
      case StrLit:
	 *s++ = '"';
	 break;
      case LStrLit:
	 *s++ = 'L';
	 *s++ = '"';
	 break;
      case CharConst:
	 *s++ = '\'';
	 break;
      case LCharConst:
	 *s++ = 'L';
	 *s++ = '\'';
	 break;
      }

   s1 = t->image;
   while (*s1 != '\0')
      *s++ = *s1++;

   switch (t->tok_id) {
      case StrLit:
      case LStrLit:
	 *s++ = '"';
	 break;
      case CharConst:
      case LCharConst:
	 *s++ = '\'';
	 break;
      }

   return s;
   }

/*
 * paste - return the next token from a source which pastes tokens. The
 *   source may represent a series of token pasting operators.
 */
struct token *paste()
   {
   struct token *t, *t1, *trigger;
   struct paste_lsts *plst;
   union src_ref ref;
   int i, *s;

   plst = g_src_stack->u.plsts;

   /*
    * If the next token of the current list is not the one to be pasted,
    *  just return it.
    */
   t = copy_t(plst->tlst->t);
   plst->tlst = plst->tlst->next;
   if (plst->tlst != NULL)
      return t;

   /*
    * We have the last token from the current list. If there is another
    *  list, this token must be pasted to the first token of that list.
    *  Make the next list the current one and get its first token.
    */
   trigger = plst->trigger;
   plst = plst->next;
   free_plsts(g_src_stack->u.plsts);
   g_src_stack->u.plsts = plst;
   if (plst == NULL) {
      pop_src();
      return t;
      }
   t1 = next_tok();

   /*
    * Paste tokens by creating a character source with the images of the
    *  two tokens concatenated.
    */
   ref.cs = new_cs(trigger->fname, NULL,
      (int)strlen(t->image) + (int)strlen(t1->image) + 7);
   push_src(CharSrc, &ref);
   s = cpy_image(t, ref.cs->char_buf);
   s = cpy_image(t1, s);
   *s = EOF;

   /*
    * Treat all characters of the new source as if they come from the
    *  location of the token pasting.
    */
   for (i = 0; i < (s - ref.cs->char_buf + 1); ++i)
      *(ref.cs->line_buf) = trigger->line;
   ref.cs->last_char = s;
   ref.cs->dir_state = Reset;
   g_first_char = ref.cs->char_buf;
   g_next_char = g_first_char;
   g_last_char = ref.cs->last_char;

   return next_tok(); /* first token from pasted images */
   }

/*
 * mac_tok - return the next token from a source which is a macro.
 */
struct token *mac_tok()
   {
   struct mac_expand *me;
   struct token *t, *t1;
   struct paste_lsts *plst;
   union src_ref ref;
   int line_check, indx, line;
   char *fname;

   me = g_src_stack->u.me; /* macro, current position, and arguments */

   /*
    * Get the next token from the macro body.
    */
   if (me->rest_bdy == NULL)
      return NULL;
   t = me->rest_bdy->t;
   me->rest_bdy = me->rest_bdy->next;

   /*
    * If this token is a stringizing operator, try stringizing the next
    *  token.
    */
   if (t->tok_id == '#')
      t = stringize(t, me);
   else
      t = copy_t(t);

   if (me->rest_bdy != NULL && me->rest_bdy->t->tok_id == PpPaste) {
      /*
       * We have found token pasting. If there is a series of such operators,
       *  make them all into one token pasting source and push it on
       *  the source stack.
       */
      if (t->flag & LineChk) {
	 line_check = 1;
	 line = t->line;
	 fname = t->fname;
	 }
      else
	 line_check = 0;
      plst = paste_parse(t, me);
      if (plst != NULL) {
	 ref.plsts = plst;
	 push_src(PasteLsts, &ref);
	 }
      t1 = next_tok();
      if (line_check && !(t1->flag & LineChk)) {
	 t1->flag |= LineChk;
	 t1->line = line;
	 t1->fname = fname;
	 }
      return t1;
      }
   else if (t->tok_id == Identifier) {
      if ((indx = parm_indx(t->image, me->m)) != -1) {
	 /*
	  * We have found a parameter. Push a token source for the corresponding
	  *  argument, that is, replace the parameter with its definition.
	  */
	 ref.tlst = me->exp_args[indx];
	 push_src(TokLst, &ref);
	 if (t->flag & LineChk) {
	    line = t->line;
	    fname = t->fname;
	    t1 = next_tok();
	    if (!(t1->flag & LineChk)) {
	       /*
		* The parameter name token is significant with respect to
		*  outputting #line directives but the first argument token
		*  is not. Pretend the argument has the same line number as the
		*  parameter name.
		*/
	       t1->flag |= LineChk;
	       t1->line = line;
	       t1->fname = fname;
	       }
	    free_t(t);
	    return t1;
	    }
	 else {
	    free_t(t);
	    return next_tok();
	    }
	 }
      else
	 return t;
      }
   else {
      /*
       * This is an ordinary token, nothing further is needed here.
       */
      return t;
      }
   }

/* Update input/source info for special macros __LINE__ and __FILE__.
 */
void src_line_updt(ln, fn)
int ln;
char *fn;
   {
   g_line_for___LINE__ = ln;
   g_fname_for___FILE__ = fn;
   }
