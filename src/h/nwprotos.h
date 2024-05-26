char *
concat(const char *s1, ...);
size_t
lconcat(char *dst, size_t dstsize, const char *s1, ...);
int C_isascii(int ch);
int C_isspace(int ch);
int C_iscntrl(int ch);
int C_isprint(int ch);
int C_isblank(int ch);
int C_isgraph(int ch);
int C_isupper(int ch);
int C_islower(int ch);
int C_isalpha(int ch);
int C_isdigit(int ch);
int C_isalnum(int ch);
int C_ispunct(int ch);
int C_isxdigit(int ch);
int C_toupper(int ch);
int C_tolower(int ch);

size_t
xstrlcat(char *dst, const char *src, size_t dsize);
size_t
xstrlcpy(char *dst, const char *src, size_t dsize);
