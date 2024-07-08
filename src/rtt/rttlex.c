/*
 * This lexical analyzer uses the preprocessor to convert text into tokens.
 *  The lexical anayser discards white space, checks to see if identifiers
 *  are reserved words or typedef names, makes sure single characters
 *  are valid tokens, and converts preprocessor constants into the
 *  various C constants.
 */
#include "rtt.h"

static struct str_buf sbuf_rttlex[1];
#define sbuf sbuf_rttlex

int               yylex     (void);

/*
 * Prototype for static function.
 */
static int int_suffix (char *s);

int lex_state = DfltLex;

char *ident = "ident";

/*
 * Characters are used as token id's for single character tokens. The
 *  following table indicates which ones can be valid for RTL.
 */

#define GoodChar(c) ((c) < 127 && good_char[c])
static int good_char[128] = {
   0  /* \000 */,   0  /* \001 */,   0  /* \002 */,   0  /* \003 */,
   0  /* \004 */,   0  /* \005 */,   0  /* \006 */,   0  /* \007 */,
   0  /*  \b  */,   0  /*  \t  */,   0  /*  \n  */,   0  /*  \v  */,
   0  /*  \f  */,   0  /*  \r  */,   0  /* \016 */,   0  /* \017 */,
   0  /* \020 */,   0  /* \021 */,   0  /* \022 */,   0  /* \023 */,
   0  /* \024 */,   0  /* \025 */,   0  /* \026 */,   0  /* \027 */,
   0  /* \030 */,   0  /* \031 */,   0  /* \032 */,   0  /*  \e  */,
   0  /* \034 */,   0  /* \035 */,   0  /* \036 */,   0  /* \037 */,
   0  /*      */,   1  /*  !   */,   0  /*  \   */,   0  /*  #   */,
   0  /*  $   */,   1  /*  %   */,   1  /*  &   */,   0  /*  '   */,
   1  /*  (   */,   1  /*  )   */,   1  /*  *   */,   1  /*  +   */,
   1  /*  ,   */,   1  /*  -   */,   1  /*  .   */,   1  /*  /   */,
   0  /*  0   */,   0  /*  1   */,   0  /*  2   */,   0  /*  3   */,
   0  /*  4   */,   0  /*  5   */,   0  /*  6   */,   0  /*  7   */,
   0  /*  8   */,   0  /*  9   */,   1  /*  :   */,   1  /*  ;   */,
   1  /*  <   */,   1  /*  =   */,   1  /*  >   */,   1  /*  ?   */,
   0  /*  @   */,   0  /*  A   */,   0  /*  B   */,   0  /*  C   */,
   0  /*  D   */,   0  /*  E   */,   0  /*  F   */,   0  /*  G   */,
   0  /*  H   */,   0  /*  I   */,   0  /*  J   */,   0  /*  K   */,
   0  /*  L   */,   0  /*  M   */,   0  /*  N   */,   0  /*  O   */,
   0  /*  P   */,   0  /*  Q   */,   0  /*  R   */,   0  /*  S   */,
   0  /*  T   */,   0  /*  U   */,   0  /*  V   */,   0  /*  W   */,
   0  /*  X   */,   0  /*  Y   */,   0  /*  Z   */,   1  /*  [   */,
   1  /*  \\  */,   1  /*  ]   */,   1  /*  ^   */,   0  /*  _   */,
   0  /*  `   */,   0  /*  a   */,   0  /*  b   */,   0  /*  c   */,
   0  /*  d   */,   0  /*  e   */,   0  /*  f   */,   0  /*  g   */,
   0  /*  h   */,   0  /*  i   */,   0  /*  j   */,   0  /*  k   */,
   0  /*  l   */,   0  /*  m   */,   0  /*  n   */,   0  /*  o   */,
   0  /*  p   */,   0  /*  q   */,   0  /*  r   */,   0  /*  s   */,
   0  /*  t   */,   0  /*  u   */,   0  /*  v   */,   0  /*  w   */,
   0  /*  x   */,   0  /*  y   */,   0  /*  z   */,   1  /*  {   */,
   1  /*  |   */,   1  /*  }   */,   1  /*  ~   */,   0  /*  \d  */
   };

/*
 * init_lex - initialize lexical analyzer.
 */
void init_lex()
   {
   struct sym_entry *sym;
   int i;
   static int first_time = 1;

   if (first_time) {
      char *s__RTT_PURE_C__ = NULL;
      int is__RTT_PURE_C__ = 0;
      first_time = 0;

      if ((s__RTT_PURE_C__ = getenv("__RTT_PURE_C__")))
	 is__RTT_PURE_C__ = *s__RTT_PURE_C__ == '1';

      ident = spec_str(ident);  /* install ident in string table */
      /*
       * install C keywords into the symbol table
       */
      sym_add_c_keyword(Auto,          spec_str("auto"));
      sym_add_c_keyword(Break,         spec_str("break"));
      sym_add_c_keyword(Case,          spec_str("case"));
      sym_add_c_keyword(Char,          spec_str("char"));
      sym_add_c_keyword(Const,         spec_str("const"));
      sym_add_c_keyword(Continue,      spec_str("continue"));
      sym_add_c_keyword(Default,       spec_str("default"));
      sym_add_c_keyword(Do,            spec_str("do"));
      sym_add_c_keyword(Doubl,         spec_str("double"));
      sym_add_c_keyword(Else,          spec_str("else"));
      sym_add_c_keyword(Enum,          spec_str("enum"));
      sym_add_c_keyword(Extern,        spec_str("extern"));
      sym_add_c_keyword(Float,         spec_str("float"));
      sym_add_c_keyword(For,           spec_str("for"));
      sym_add_c_keyword(Goto,          spec_str("goto"));
      sym_add_c_keyword(If,            spec_str("if"));
      sym_add_c_keyword(Int,           spec_str("int"));
      sym_add_c_keyword(Long,          spec_str("long"));
      sym_add_c_keyword(Register,      spec_str("register"));
      sym_add_c_keyword(Return,        spec_str("return"));
      sym_add_c_keyword(Short,         spec_str("short"));
      sym_add_c_keyword(Signed,        spec_str("signed"));
      sym_add_c_keyword(Sizeof,        spec_str("sizeof"));
      sym_add_c_keyword(PassThru,      spec_str("passthru"));
      sym_add_c_keyword(Static,        spec_str("static"));
      sym_add_c_keyword(Struct,        spec_str("struct"));
      sym_add_c_keyword(Switch,        spec_str("switch"));
      sym_add_c_keyword(Typedef,       spec_str("typedef"));
      sym_add_c_keyword(Union,         spec_str("union"));
      sym_add_c_keyword(Unsigned,      spec_str("unsigned"));
      sym_add_c_keyword(Void,          spec_str("void"));
      sym_add_c_keyword(Volatile,      spec_str("volatile"));
      sym_add_c_keyword(While,         spec_str("while"));

      if (is__RTT_PURE_C__)
	 return;

      /*
       * Install keywords from run-time interface language.
       */
      sym_add_rtt_keyword(Abstract,      spec_str("abstract"));
      sym_add_rtt_keyword(All_fields,    spec_str("all_fields"));
      sym_add_rtt_keyword(Any_value,     spec_str("any_value"));
      sym_add_rtt_keyword(Arith_case,    spec_str("arith_case"));
      sym_add_rtt_keyword(Body,          spec_str("body"));
      sym_add_rtt_keyword(C_Double,      spec_str("C_double"));
      sym_add_rtt_keyword(C_Integer,     spec_str("C_integer"));
      sym_add_rtt_keyword(C_String,      spec_str("C_string"));
      sym_add_rtt_keyword(Cnv,           spec_str("cnv"));
      sym_add_rtt_keyword(Constant,      spec_str("constant"));
      sym_add_rtt_keyword(Declare,       spec_str("declare"));
      sym_add_rtt_keyword(Def,           spec_str("def"));
      sym_add_rtt_keyword(Empty_type,    spec_str("empty_type"));
      sym_add_rtt_keyword(End,           spec_str("end"));
      sym_add_rtt_keyword(Errorfail,     spec_str("errorfail"));
      sym_add_rtt_keyword(Exact,         spec_str("exact"));
      sym_add_rtt_keyword(Fail,          spec_str("fail"));
      sym_add_rtt_keyword(Function,      spec_str("function"));
      sym_add_rtt_keyword(Inline,        spec_str("inline"));
      sym_add_rtt_keyword(Is,            spec_str("is"));
      sym_add_rtt_keyword(Keyword,       spec_str("keyword"));
      sym_add_rtt_keyword(Len_case,      spec_str("len_case"));
      sym_add_rtt_keyword(Named_var,     spec_str("named_var"));
      sym_add_rtt_keyword(New,           spec_str("new"));
      sym_add_rtt_keyword(Of,            spec_str("of"));
      sym_add_rtt_keyword(Operator,      spec_str("operator"));
      g_str_rslt = spec_str("result");
      sym_add_rtt_keyword(Runerr,        spec_str("runerr"));
      sym_add_rtt_keyword(Store,         spec_str("store"));
      sym_add_rtt_keyword(Struct_var,    spec_str("struct_var"));
      sym_add_rtt_keyword(Suspend,       spec_str("suspend"));
      sym_add_rtt_keyword(Tended,        spec_str("tended"));
      sym_add_rtt_keyword(Then,          spec_str("then"));
      sym_add_rtt_keyword(Tmp_cset,      spec_str("tmp_cset"));
      sym_add_rtt_keyword(Tmp_string,    spec_str("tmp_string"));
      sym_add_rtt_keyword(Type,          spec_str("type"));
      sym_add_rtt_keyword(Type_case,     spec_str("type_case"));
      sym_add_rtt_keyword(Underef,       spec_str("underef"));
      sym_add_rtt_keyword(Variable,      spec_str("variable"));

      for (i = 0; i < num_typs; ++i) {
	 icontypes[i].id = spec_str(icontypes[i].id);
	 sym = sym_add_icontype(IconType, icontypes[i].id);
	 sym->u.typ_indx = i;
	 }

      for (i = 0; i < num_cmpnts; ++i) {
	 typecompnt[i].id = spec_str(typecompnt[i].id);
	 sym = sym_add_component(Component, typecompnt[i].id);
	 sym->u.typ_indx = i;
	 }
      }
   }

/*
 * int_suffix - we have reached the end of what seems to be an integer
 *  constant. check for a valid suffix.
 */
static int int_suffix(s)
char *s;
   {
   int tok_id;

   if (*s == 'u' || *s == 'U') {
      ++s;
      if (*s == 'l' || *s == 'L') {
	 ++s;
	 if (*s == 'l' || *s == 'L') {
	    ++s;
	    tok_id = ULLIntConst;  /* unsigned long long */
	    }
	 else
	    tok_id = ULIntConst;  /* unsigned long */
	 }
      else
	 tok_id  = UIntConst;  /* unsigned */
      }
   else if (*s == 'l' || *s == 'L') {
      ++s;
      if (*s == 'l' || *s == 'L') {
	 ++s;
	 if (*s == 'u' || *s == 'U') {
	    ++s;
	    tok_id = ULLIntConst;  /* unsigned long long */
	    }
	 else
	    tok_id = LLIntConst;  /* long long */
	 }
      else if (*s == 'u' || *s == 'U') {
	 ++s;
	 tok_id = ULIntConst;  /* unsigned long */
	 }
      else
	 tok_id = LIntConst;   /* long */
      }
   else
      tok_id = IntConst;       /* plain int */
   if (*s != '\0')
      errt2(yylval.t, "invalid integer constant: ", yylval.t->image);
   return tok_id;
   }

/*
 * yylex - lexical analyzer, called by yacc-generated parser.
 */
int yylex()
   {
   char *s;
   struct sym_entry *sym;
   struct token *lk_ahead = NULL;
   int is_float;

   /*
    * See if the last call to yylex() left a token from looking ahead.
    */
   if (lk_ahead == NULL)
      yylval.t = preproc();
   else {
      yylval.t = lk_ahead;
      lk_ahead = NULL;
      }

   /*
    * Skip white space, then check for end-of-input.
    */
   while (yylval.t != NULL && yylval.t->tok_id == WhiteSpace) {
      free_t(yylval.t);
      yylval.t = preproc();
      }
   if (yylval.t == NULL)
      return 0;

   /*
    * The rtt recognizes ** as an operator in abstract type computations.
    *  The parsing context is indicated by lex_state.
    */
   if (lex_state == TypeComp && yylval.t->tok_id == '*') {
      lk_ahead = preproc();
      if (lk_ahead != NULL && lk_ahead->tok_id == '*') {
	 free_t(lk_ahead);
	 lk_ahead = NULL;
	 yylval.t->tok_id = Intersect;
	 yylval.t->image = spec_str("**");
	 }
      }

   /*
    * Some tokens are passed along without change, but some need special
    *  processing: identifiers, numbers, PpPassThru tokens, and single
    *  character tokens.
    */
   if (yylval.t->tok_id == Identifier) {
      /*
       * See if this is an identifier, a reserved word, or typedef name.
       */
      if ((sym = sym_lkup(yylval.t->image)))
	 yylval.t->tok_id = sym->tok_id;
      }
   else if (yylval.t->tok_id == PpNumber) {
      /*
       * Determine what kind of numeric constant this is.
       */
      s = yylval.t->image;
      if (*s == '0' && (*++s == 'x' || *s == 'X')) {
	 /*
	  * Hex integer constant.
	  */
	 ++s;
	 while (C_isxdigit(*s))
	    ++s;
	 yylval.t->tok_id = int_suffix(s);
	 }
      else {
	 is_float = 0;
	 while (C_isdigit(*s))
	     ++s;
	 if (*s == '.') {
	    is_float = 1;
	    ++s;
	    while (C_isdigit(*s))
	       ++s;
	    }
	 if (*s == 'e' || *s == 'E') {
	    is_float = 1;
	    ++s;
	    if (*s == '+' || *s == '-')
	       ++s;
	    while (C_isdigit(*s))
	       ++s;
	    }
	 if (is_float) {
	    switch (*s) {
	       case '\0':
		  yylval.t->tok_id = DblConst;   /* double */
		  break;
	       case 'f': case 'F':
		   yylval.t->tok_id = FltConst;  /* float */
		   break;
	       case 'l': case 'L':
		   yylval.t->tok_id = LDblConst; /* long double */
		   break;
	       default:
		   errt2(yylval.t, "invalid float constant: ", yylval.t->image);
	       }
	    }
	 else {
	    /*
	     * This appears to be an integer constant. If it starts
	     *  with '0', it should be an octal constant.
	     */
	    if (yylval.t->image[0] == '0') {
	       s = yylval.t->image;
	       while (*s >= '0' && *s <= '7')
		  ++s;
	       }
	    yylval.t->tok_id = int_suffix(s);
	    }
	 }
      }
   else if (yylval.t->tok_id == PpPassThru) {
      /* Non-standard preprocessor directive, PpPassThru always comes in
       * twos, one for the chunk name and other for the chunk contents
       */
      struct token *n, *v;
      n = yylval.t;
      v = preproc();
      passthrudir(n, v);
      free_tt(n, v);
      return yylex();
      }
   else if (yylval.t->tok_id == PpOutput) {
      outputdir(yylval.t);
      free_t(yylval.t);
      return yylex();
      }
   else if (lex_state == OpHead && yylval.t->tok_id != '}' &&
	 GoodChar((int)yylval.t->image[0])) {
      /*
       * This should be the operator symbol in the header of an operation
       *  declaration. Concatenate all operator symbols into one token
       *  of type OpSym.
       */
      init_sbuf(sbuf);
      for (s = yylval.t->image; *s != '\0'; ++s)
	 AppChar(sbuf, *s);
      lk_ahead = preproc();
      while (lk_ahead != NULL && GoodChar((int)lk_ahead->image[0])) {
	 for (s = lk_ahead->image; *s != '\0'; ++s)
	    AppChar(sbuf, *s);
	 free_t(lk_ahead);
	 lk_ahead = preproc();
	 }
      yylval.t->tok_id = OpSym;
      yylval.t->image = str_install(sbuf);
      }
   else if (yylval.t->tok_id < 256) {
      /*
       * This is a one-character token, make sure it is valid.
       */
      if (!GoodChar(yylval.t->tok_id))
	 errt2(yylval.t, "invalid character: ", yylval.t->image);
      }

   return yylval.t->tok_id;
   }
