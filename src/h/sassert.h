#ifndef SASSERT_DOT_H	/* only include once */
#define SASSERT_DOT_H 1

#define PASTE2(x, y) x ## y
#define EXPAND_THEN_PASTE(a, b) PASTE2(a,b)
#define C89_STATIC_ASSERT(x, msg) typedef char \
   EXPAND_THEN_PASTE(STATIC_ASSERT_FAIL_AT_LINE_,__LINE__) \
   [x ? (int)sizeof(msg) : -1]

#endif					/* SASSERT_DOT_H */
