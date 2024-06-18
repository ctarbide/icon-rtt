/*
 * This file contains routines for building tokens out of characters from a
 *  "character source". This source is the top element on the source stack.
 */
#include "../preproc/preproc.h"
#include "../preproc/ptoken.h"

static struct str_buf sbuf_bldtok[1];
#define sbuf sbuf_bldtok

static int b_line;
static char *b_fname;

/*
 * Prototypes for static functions.
 */
static int           pp_tok_id  (char *s);
static struct token *chck_wh_sp (struct char_src *cs);
static struct token *pp_number  (void);
static struct token *char_str   (int delim, int tok_id);
static struct token *hdr_tok    (int delim, int tok_id, struct char_src *cs);

int g_whsp_image = NoSpelling;    /* indicate what is in white space tokens */
struct token *zero_tok;         /* token for literal 0 */
struct token *one_tok;          /* token for literal 1 */

#include "../preproc/pproto.h"

/*
 * IsWhSp(c) - true if c is a white space character.
 */
#define IsWhSp(c) (c == ' ' || c == '\n' || c == '\t' || c == '\v' || c == '\f')

/*
 * AdvChar() - advance to next character from buffer, filling the buffer
 *   if needed.
 */
#define AdvChar() \
   if (++g_next_char == g_last_char) \
      fill_cbuf();

/*
 * List of preprocessing directives and the corresponding token ids.
 */
static struct rsrvd_wrd pp_rsrvd[] = {
   /* custom directives */
   {"passthru",PpPassThru},
   {"output",  PpOutput},
   {"begdef",  PpBegdef},
   {"enddef",  PpEnddef},
   {"noexpand",PpNoExpand},
   /* standard directives */
   {"if",      PpIf},
   {"else",    PpElse},
   {"ifdef",   PpIfdef},
   {"ifndef",  PpIfndef},
   {"elif",    PpElif},
   {"endif",   PpEndif},
   {"include", PpInclude},
   {"define",  PpDefine},
   {"undef",   PpUndef},
   {"line",    PpLine},
   {"error",   PpError},
   {"pragma",  PpPragma},
   {NULL, Invalid}};

/*
 * init_tok - initialize tokenizer.
 */
void init_tok()
   {
   struct rsrvd_wrd *rw;
   static int first_time = 1;

   if (first_time) {
      first_time = 0;
      init_sbuf(sbuf); /* initialize string buffer */
      /*
       * install reserved words into the string table
       */
      for (rw = pp_rsrvd; rw->s; ++rw)
	 rw->s = spec_str(rw->s);

      zero_tok = new_token(PpNumber, spec_str("0"), "", 0);
      one_tok = new_token(PpNumber, spec_str("1"), "", 0);
      }
   }

void finish_tok()
   {
   }

/*
 * pp_tok_id - see if s is the name of a preprocessing directive.
 */
static int pp_tok_id(s)
char *s;
   {
   struct rsrvd_wrd *rw;
   for (rw = pp_rsrvd; rw->s && rw->s != s; ++rw)
      ;
   return rw->tok_id;
   }

/*
 * chk_eq_sign - look ahead to next character to see if it is an equal sign.
 *  It is used for processing -D options.
 */
int chk_eq_sign()
   {
   if (*g_next_char == '=') {
      AdvChar();
      return 1;
      }
   else
      return 0;
   }

/*
 * chck_wh_sp - If the input is at white space, construct a white space token
 *  and return it, otherwise return NULL. This function also helps keeps track
 *  of preprocessor directive boundaries.
 */
static struct token *chck_wh_sp(cs)
struct char_src *cs;
   {
   int c1, c2;
   int tok_id;

   /*
    * See if we are at white space or a comment.
    */
   c1 = *g_next_char;
   if (!IsWhSp(c1) && (c1 != '/' || g_next_char[1] != '*'))
      return NULL;

   /*
    * Find the line number of the current character in the line number
    *  buffer, and correct it if we have encountered any #line directives.
    */
   b_line = cs->line_buf[g_next_char - g_first_char] + cs->line_adj;
   if (c1 == '\n')
      --b_line;      /* a new-line really belongs to the previous line */

   tok_id = WhiteSpace;
   for (;;) {
      if (IsWhSp(c1)) {
	 /*
	  * The next character is a white space. If we are retaining the
	  *  image of the white space in the token, copy the character to
	  *  the string buffer. If we are in the midst of a preprocessor
	  *  directive and find a new-line, indicate the end of the
	  *  the directive.
	  */
	 AdvChar();
	 if (g_whsp_image != NoSpelling)
	    AppChar(sbuf, c1);
	 if (c1 == '\n') {
	    int ds = cs->dir_state;
	    int ln = cs->line_buf[g_next_char - g_first_char] + cs->line_adj;
	    src_line_updt(ln, b_fname);
	    cs->dir_state = CanStart;
	    if (ds == Within) {
	       tok_id = PpDirEnd;
	       break;
	       }
	    }
	 }
      else if (c1 == '/' && g_next_char[1] == '*') {
	 /*
	  * Start of comment. If we are retaining the image of comments,
	  *  copy the characters into the string buffer.
	  */
	 if (g_whsp_image == FullImage) {
	    AppChar(sbuf, '/');
	    AppChar(sbuf, '*');
	    }
	 AdvChar();
	 AdvChar();

	 /*
	  * Look for the end of the comment.
	  */
	 c1 = *g_next_char;
	 c2 = g_next_char[1];
	 while (c1 != '*' || c2 != '/') {
	    if (c1 == EOF)
		errfl1(b_fname, b_line, "eof encountered in comment");
	    AdvChar();
	    if (g_whsp_image == FullImage)
	       AppChar(sbuf, c1);
	    c1 = c2;
	    c2 = g_next_char[1];
	    }

	 /*
	  * Determine if we are retaining the image of a comment, replacing
	  *  a comment by one space character, or ignoring comments.
	  */
	 if (g_whsp_image == FullImage) {
	    AppChar(sbuf, '*');
	    AppChar(sbuf, '/');
	    }
	 else if (g_whsp_image == NoComment)
	    AppChar(sbuf, ' ');
	 AdvChar();
	 AdvChar();
	 }
      else
	 break;         /* end of white space */
      c1 = *g_next_char;
      }

   /*
    * If we are not retaining the image of white space, replace it all
    *  with one space character.
    */
   if (g_whsp_image == NoSpelling)
      AppChar(sbuf, ' ');

   if (tok_id == WhiteSpace && *g_next_char == '#' && g_next_char[1] == '#') {
      /* Discard white space before a ## operator.
       */
      sbuf->endimage = sbuf->strtimage;
      return NULL;
      }

   return new_token(tok_id, str_install(sbuf), b_fname, b_line);
   }

/*
 * pp_number - Create a token for a preprocessing number (See ANSI C Standard
 *  for the syntax of such a number).
 */
static struct token *pp_number()
   {
   int c;

   c = *g_next_char;
   for (;;) {
      if (c == 'e' || c == 'E') {
	 AppChar(sbuf, c);
	 AdvChar();
	 c = *g_next_char;
	 if (c == '+' || c == '-') {
	    AppChar(sbuf, c);
	    AdvChar();
	    c = *g_next_char;
	    }
	 }
      else if (C_isdigit(c) || c == '.' || C_islower(c) || C_isupper(c) || c == '_') {
	 AppChar(sbuf, c);
	 AdvChar();
	 c = *g_next_char;
	 }
      else {
	 return new_token(PpNumber, str_install(sbuf), b_fname, b_line);
	 }
      }
   }

/*
 * char_str - construct a token for a character constant or string literal.
 */
static struct token *char_str(delim, tok_id)
int delim;
int tok_id;
   {
   int c;

   for (c = *g_next_char; c != EOF && c != '\n' &&  c != delim; c = *g_next_char) {
      AppChar(sbuf, c);
      if (c == '\\') {
	 c = g_next_char[1];
	 if (c == EOF || c == '\n')
	    break;
	 else {
	    AppChar(sbuf, c);
	    AdvChar();
	    }
	 }
      AdvChar();
      }
   if (c == EOF)
      errfl1(b_fname, b_line, "End-of-file encountered within a literal");
   if (c == '\n')
      errfl1(b_fname, b_line, "New-line encountered within a literal");
   AdvChar();
   return new_token(tok_id, str_install(sbuf), b_fname, b_line);
   }

/*
 * hdr_tok - create a token for an #include header. The delimiter may be
 *  > or ".
 */
static struct token *hdr_tok(delim, tok_id, cs)
int delim;
int tok_id;
struct char_src *cs;
   {
   int c;

   b_line = cs->line_buf[g_next_char - g_first_char] + cs->line_adj;
   AdvChar();

   for (c = *g_next_char; c != delim; c = *g_next_char) {
      if (c == EOF)
	 errfl1(b_fname, b_line,
	    "End-of-file encountered within a header name");
      if (c == '\n')
	 errfl1(b_fname, b_line,
	    "New-line encountered within a header name");
      AppChar(sbuf, c);
      AdvChar();
      }
   AdvChar();
   return new_token(tok_id, str_install(sbuf), b_fname, b_line);
   }

/*
 * tokenize - return the next token from the character source on the top
 *  of the source stack.
 */
struct token *tokenize()
   {
   struct char_src *cs;
   struct token *t1, *t2;
   int c, tok_id;

   cs = g_src_stack->u.cs;

   /*
    * Check to see if the last call left a token from a look ahead.
    */
   if (cs->tok_sav != NULL) {
      t1 = cs->tok_sav;
      cs->tok_sav = NULL;
      return t1;
      }

   if (*g_next_char == EOF)
      return NULL;

   /*
    * Find the current line number and file name for the character
    *  source.
    */
   b_line = cs->line_buf[g_next_char - g_first_char] + cs->line_adj;
   b_fname = cs->fname;

   /* Check for white space or PpDirEnd.
    */
   if ((t1 = chck_wh_sp(cs)))
      return t1;

   c = *g_next_char;  /* look at next character */
   AdvChar();

   /*
    * If the last thing we saw in this character source was white space
    *  containing a new-line, then we must look for the start of a
    *  preprocessing directive.
    */
   if (cs->dir_state == CanStart) {
      cs->dir_state = Reset;
      if  (c == '#' && *g_next_char != '#') {
	 /*
	  * Assume we are within a preprocessing directive and check
	  *  for white space to discard.
	  */
	 cs->dir_state = Within;
	 if ((t1 = chck_wh_sp(cs))) {
	    if (t1->tok_id == PpDirEnd) {
	       /*
		* We found a new-line, this is a null preprocessor directive.
		*/
	       cs->tok_sav = t1;
	       AppChar(sbuf, '#');
	       return new_token(PpNull, str_install(sbuf), b_fname, b_line);
	       }
	    else
	       free_t(t1);  /* discard white space */
	    }
	 c = *g_next_char;
	 if (C_islower(c) || C_isupper(c) || c == '_') {
	    /*
	     * Tokenize the identifier following the #
	     */
	    t1 = tokenize();
	    if ((tok_id = pp_tok_id(t1->image)) == Invalid) {
	       /*
		* We have a stringizing operation, not a preprocessing
		*  directive.
		*/
	       cs->dir_state = Reset;
	       cs->tok_sav = t1;
	       AppChar(sbuf, '#');
	       return new_token('#', str_install(sbuf), b_fname, b_line);
	       }
	    else {
	       t1->tok_id = tok_id;
	       if (tok_id == PpInclude) {
		  /*
		   * A header name has to be tokenized specially. Find
		   *  it, then save the token.
		   */
		  if ((t2 = chck_wh_sp(cs))) {
		     if (t2->tok_id == PpDirEnd)
			errt1(t2, "file name missing from #include");
		     else
			free_t(t2);
		     }
		  c = *g_next_char;
		  if (c == '"')
		     cs->tok_sav = hdr_tok('"', StrLit, cs);
		  else if (c == '<')
		     cs->tok_sav = hdr_tok('>', PpHeader, cs);
		  }
	       /*
		* Return the token indicating the kind of preprocessor
		*  directive we have started.
		*/
	       return t1;
	       }
	    }
	 else
	    errfl1(b_fname, b_line,
	       "# must be followed by an identifier or keyword");
	 }
      }

   /*
    * Check for literals containing wide characters.
    */
   if (c == 'L') {
      if (*g_next_char == '\'') {
	 AdvChar();
	 t1 = char_str('\'', LCharConst);
	 if (t1->image[0] == '\0')
	    errt1(t1, "invalid character constant");
	 return t1;
	 }
      else if (*g_next_char == '"') {
	 AdvChar();
	 return char_str('"', LStrLit);
	 }
      }

   /*
    * Check for identifier.
    */
   if (C_islower(c) || C_isupper(c) || c == '_') {
      AppChar(sbuf, c);
      c = *g_next_char;
      while (C_islower(c) || C_isupper(c) || C_isdigit(c) || c == '_') {
	 AppChar(sbuf, c);
	 AdvChar();
	 c = *g_next_char;
	 }
      return new_token(Identifier, str_install(sbuf), b_fname, b_line);
      }

   /*
    * Check for number.
    */
   if (C_isdigit(c)) {
      AppChar(sbuf, c);
      return pp_number();
      }

   /*
    * Check for character constant.
    */
   if (c == '\'') {
      t1 = char_str(c, CharConst);
      if (t1->image[0] == '\0')
	 errt1(t1, "invalid character constant");
      return t1;
      }

   /*
    * Check for string constant.
    */
   if (c == '"')
      return char_str(c, StrLit);

   /*
    * Check for operators and punctuation. Anything that does not fit these
    *  categories is a single character token.
    */
   AppChar(sbuf, c);
   switch (c) {
      case '.':
	 c = *g_next_char;
	 if (C_isdigit(c)) {
	    /*
	     * Number
	     */
	    AppChar(sbuf, c);
	    AdvChar();
	    return pp_number();
	    }
	 else if (c == '.' && g_next_char[1] == '.') {
	    /*
	     *  ...
	     */
	    AdvChar();
	    AdvChar();
	    AppChar(sbuf, '.');
	    AppChar(sbuf, '.');
	    return new_token(Ellipsis, str_install(sbuf), b_fname, b_line);
	    }
	 else
	    return new_token('.', str_install(sbuf), b_fname, b_line);

      case '+':
	 c = *g_next_char;
	 if (c == '+') {
	    /*
	     *  ++
	     */
	    AppChar(sbuf, '+');
	    AdvChar();
	    return new_token(Incr, str_install(sbuf), b_fname, b_line);
	    }
	 else if (c == '=') {
	    /*
	     *  +=
	     */
	    AppChar(sbuf, '=');
	    AdvChar();
	    return new_token(PlusAsgn, str_install(sbuf), b_fname, b_line);
	    }
	 else
	    return new_token('+', str_install(sbuf), b_fname, b_line);

      case '-':
	 c = *g_next_char;
	 if (c == '>') {
	    /*
	     *  ->
	     */
	    AppChar(sbuf, '>');
	    AdvChar();
	    return new_token(Arrow, str_install(sbuf), b_fname, b_line);
	    }
	 else if (c == '-') {
	    /*
	     *  --
	     */
	    AppChar(sbuf, '-');
	    AdvChar();
	    return new_token(Decr, str_install(sbuf), b_fname, b_line);
	    }
	 else if (c == '=') {
	    /*
	     *  -=
	     */
	    AppChar(sbuf, '=');
	    AdvChar();
	    return new_token(MinusAsgn, str_install(sbuf), b_fname,
	       b_line);
	    }
	 else
	    return new_token('-', str_install(sbuf), b_fname, b_line);

      case '<':
	 c = *g_next_char;
	 if (c == '<') {
	    AppChar(sbuf, '<');
	    AdvChar();
	    if (*g_next_char == '=') {
	       /*
		*  <<=
		*/
	       AppChar(sbuf, '=');
	       AdvChar();
	       return new_token(LShftAsgn, str_install(sbuf), b_fname,
		  b_line);
	       }
	    else
	       /*
		*  <<
		*/
	       return new_token(LShft, str_install(sbuf), b_fname, b_line);
	    }
	 else if (c == '=') {
	    /*
	     *  <=
	     */
	    AppChar(sbuf, '=');
	    AdvChar();
	    return new_token(Leq, str_install(sbuf), b_fname, b_line);
	    }
	 else
	    return new_token('<', str_install(sbuf), b_fname, b_line);

      case '>':
	 c = *g_next_char;
	 if (c == '>') {
	    AppChar(sbuf, '>');
	    AdvChar();
	    if (*g_next_char == '=') {
	       /*
		*  >>=
		*/
	       AppChar(sbuf, '=');
	       AdvChar();
	       return new_token(RShftAsgn, str_install(sbuf), b_fname,
		  b_line);
	       }
	    else
	       /*
		*  >>
		*/
	       return new_token(RShft, str_install(sbuf), b_fname, b_line);
	    }
	 else if (c == '=') {
	    /*
	     *  >=
	     */
	    AppChar(sbuf, '=');
	    AdvChar();
	    return new_token(Geq, str_install(sbuf), b_fname, b_line);
	    }
	 else
	    return new_token('>', str_install(sbuf), b_fname, b_line);

      case '=':
	 if (*g_next_char == '=') {
	    /*
	     *  ==
	     */
	    AppChar(sbuf, '=');
	    AdvChar();
	    return new_token(Equal, str_install(sbuf), b_fname, b_line);
	    }
	 else
	    return new_token('=', str_install(sbuf), b_fname, b_line);

      case '!':
	 if (*g_next_char == '=') {
	    /*
	     *  !=
	     */
	    AppChar(sbuf, '=');
	    AdvChar();
	    return new_token(Neq, str_install(sbuf), b_fname, b_line);
	    }
	 else
	    return new_token('!', str_install(sbuf), b_fname, b_line);

      case '&':
	 c = *g_next_char;
	 if (c == '&') {
	    /*
	     *  &&
	     */
	    AppChar(sbuf, '&');
	    AdvChar();
	    return new_token(And, str_install(sbuf), b_fname, b_line);
	    }
	 else if (c == '=') {
	    /*
	     *  &=
	     */
	    AppChar(sbuf, '=');
	    AdvChar();
	    return new_token(AndAsgn, str_install(sbuf), b_fname, b_line);
	    }
	 else
	    return new_token('&', str_install(sbuf), b_fname, b_line);

      case '|':
	 c = *g_next_char;
	 if (c == '|') {
	    /*
	     *  ||
	     */
	    AppChar(sbuf, '|');
	    AdvChar();
	    return new_token(Or, str_install(sbuf), b_fname, b_line);
	    }
	 else if (c == '=') {
	    /*
	     *  |=
	     */
	    AppChar(sbuf, '=');
	    AdvChar();
	    return new_token(OrAsgn, str_install(sbuf), b_fname, b_line);
	    }
	 else
	    return new_token('|', str_install(sbuf), b_fname, b_line);

      case '*':
	 if (*g_next_char == '=') {
	    /*
	     *  *=
	     */
	    AppChar(sbuf, '=');
	    AdvChar();
	    return new_token(MultAsgn, str_install(sbuf), b_fname, b_line);
	    }
	 else
	    return new_token('*', str_install(sbuf), b_fname, b_line);

      case '/':
	 if (*g_next_char == '=') {
	    /*
	     *  /=
	     */
	    AppChar(sbuf, '=');
	    AdvChar();
	    return new_token(DivAsgn, str_install(sbuf), b_fname, b_line);
	    }
	 else
	    return new_token('/', str_install(sbuf), b_fname, b_line);

      case '%':
	 if (*g_next_char == '=') {
	    /*
	     *  &=
	     */
	    AppChar(sbuf, '=');
	    AdvChar();
	    return new_token(ModAsgn, str_install(sbuf), b_fname, b_line);
	    }
	 else
	    return new_token('%', str_install(sbuf), b_fname, b_line);

      case '^':
	 if (*g_next_char == '=') {
	    /*
	     *  ^=
	     */
	    AppChar(sbuf, '=');
	    AdvChar();
	    return new_token(XorAsgn, str_install(sbuf), b_fname, b_line);
	    }
	 else
	    return new_token('^', str_install(sbuf), b_fname, b_line);

      case '#':
	 /*
	  * Token pasting or stringizing operator.
	  */
	 if (*g_next_char == '#') {
	    /*
	     *  ##
	     */
	    AppChar(sbuf, '#');
	    AdvChar();
	    t1 =  new_token(PpPaste, str_install(sbuf), b_fname, b_line);
	    }
	 else
	    t1 = new_token('#', str_install(sbuf), b_fname, b_line);

	 /*
	  * The operand must be in the same preprocessing directive.
	  */
	 if ((t2 = chck_wh_sp(cs))) {
	    if (t2->tok_id == PpDirEnd)
	      errt2(t2, t1->image,
	       " preprocessing expression must not cross directive boundary");
	    else
	       free_t(t2);
	    }
	 return t1;

      default:
	 return new_token(c, str_install(sbuf), b_fname, b_line);
      }
   }
