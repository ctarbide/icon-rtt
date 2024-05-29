/*
 * The functions in this file maintain a hash table of strings and manage
 *   string buffers.
 */
#include "../h/gsupport.h"

#define SBUF_GROW_FACTOR 2

size_t g_grow_count = 0;
size_t g_wasted_space = 0;
size_t g_sbuf_init_count = 0;
size_t g_sbuf_donate_count = 0;

/*
 * Prototype for static function.
 */
static int streq  (int len, char *s1, char *s2);

/*
 * Entry in string table.
 */
struct str_entry {
   char *s;                 /* string */
   int length;              /* length of string */
   struct str_entry *next;
   };

#define SBufSize 1024                     /* initial size of a string buffer */
#define StrTblSz 149                      /* size of string hash table */
static struct str_entry **str_tbl = NULL; /* string hash table */

/*
 * init_str - initialize string hash table.
 */
void init_str()
   {
   int h;

   if (str_tbl == NULL) {
      str_tbl = alloc(StrTblSz * sizeof(struct str_entry *));
      for (h = 0; h < StrTblSz; ++h)
	 str_tbl[h] = NULL;
      }
   }

/*
 * free_stbl - free string table.
 */
void free_stbl()
   {
   struct str_entry *se, *se1;
   int h;

   for (h = 0; h < StrTblSz; ++h)
      for (se = str_tbl[h]; se != NULL; se = se1) {
	 se1 = se->next;
	 free((char *)se);
	 }

   free((char *)str_tbl);
   str_tbl = NULL;
   }

/*
 * init_sbuf - initialize a str_buf, allocating an initial buffer if not
 * already initialized.
 */
void init_sbuf(sbuf)
struct str_buf *sbuf;
   {
   if (sbuf->frag_lst)
      return;
   g_sbuf_init_count++;
   sbuf->size = SBufSize;
   sbuf->frag_lst = alloc(sizeof(*sbuf->frag_lst) + SBufSize);
   sbuf->frag_lst->next = NULL;
   sbuf->strtimage = (void*)(sbuf->frag_lst + 1);
   sbuf->end = (sbuf->endimage = sbuf->strtimage) + SBufSize;
   }

/*
 * clear_sbuf - free string buffer storage.
 */
void clear_sbuf(sbuf)
struct str_buf *sbuf;
   {
   struct str_buf_frag *sbf, *sbf1;

   for (sbf = sbuf->frag_lst; sbf != NULL; sbf = sbf1) {
      sbf1 = sbf->next;
      free((char *)sbf);
      }
   sbuf->frag_lst = NULL;
   sbuf->strtimage = NULL;
   sbuf->endimage = NULL;
   sbuf->end = NULL;
   }

/*
 * grow_sbuf - allocate a new buffer fragment for a sbuf, copying the partially
 *   created string from the end of full buffer to the new one.
 */
void grow_sbuf(sbuf)
struct str_buf *sbuf;
   {
   struct str_buf_frag *newfrag;
   size_t newsz, waste;

   if (sbuf->end != sbuf->endimage) {
      /* str_buf discarded prematurely */
      fprintf(stderr, "Exhaustion %s:%d.\n", __FILE__, __LINE__);
      exit(1);
      }

   g_grow_count++;
   g_wasted_space += (waste = sbuf->end - sbuf->strtimage);

   newsz = sbuf->size * SBUF_GROW_FACTOR;
   newfrag = alloc(sizeof(*newfrag) + newsz);
   if (newfrag == NULL) {
      fprintf(stderr, "Exhaustion %s:%d.\n", __FILE__, __LINE__);
      exit(1);
      }

   newfrag->next = sbuf->frag_lst;
   memcpy(newfrag + 1, sbuf->strtimage, waste);

   sbuf->frag_lst = newfrag;
   sbuf->strtimage = (void*)(newfrag + 1);
   sbuf->endimage = sbuf->strtimage + waste;
   sbuf->end = sbuf->strtimage + sbuf->size;
   sbuf->size = newsz;
   }

/* donating an uninitialized sbuf is harmless
 */
void donate_sbuf(sbuf)
struct str_buf *sbuf;
{
   if (sbuf->frag_lst == NULL)
      return;
   g_sbuf_donate_count++;
   g_wasted_space += sbuf->end - sbuf->strtimage;
   sbuf->frag_lst = NULL;
   sbuf->strtimage = NULL;
   sbuf->endimage = NULL;
   sbuf->end = NULL;
   }

/*
 * spec_str - install a special string (null terminated) in the string table.
 */
char *spec_str(s)
char *s;
   {
   struct str_entry *se;
   char *s1;
   int l, h;

   h = 0;
   l = 1;
   for (s1 = s; *s1 != '\0'; ++s1) {
      h += *s1 & 0377;
      ++l;
      }
   h %= StrTblSz;
   for (se = str_tbl[h]; se != NULL; se = se->next)
      if (l == se->length && streq(l, s, se->s))
	 return se->s;
   se = NewStruct(str_entry);
   se->s = s;
   se->length = l;
   se->next = str_tbl[h];
   str_tbl[h] = se;
   return s;
   }

/*
 * str_install - find out if the string at the end of the buffer is in
 *   the string table. If not, put it there. Return a pointer to the
 *   string in the table.
 */
char *str_install(sbuf)
struct str_buf *sbuf;
   {
   int l, h;
   struct str_entry *se;
   char *s, *e;

   AppChar(sbuf, '\0');   /* null terminate the buffered copy of the string */
   s = sbuf->strtimage;
   e = sbuf->endimage;

   /*
    * Compute hash value.
    */
   h = 0;
   while (s < e)
      h += *s++ & 0377;
   h %= StrTblSz;
   s = sbuf->strtimage;
   l = e - s;
   for (se = str_tbl[h]; se != NULL; se = se->next)
      if (l == se->length && streq(l, s, se->s)) {
	 /*
	  * A copy of the string is already in the table. Delete the copy
	  *  in the buffer.
	  */
	 sbuf->endimage = s;
	 return se->s;
	 }

   /*
    * The string is not in the table. Add the copy from the buffer to the
    *  table.
    */
   se = NewStruct(str_entry);
   se->s = s;
   se->length = l;
   sbuf->strtimage = e;
   se->next = str_tbl[h];
   str_tbl[h] = se;
   return se->s;
   }

/*
 * streq - compare s1 with s2 for len bytes, and return 1 for equal,
 *  0 for not equal.
 */
static int streq(len, s1, s2)
int len;
char *s1, *s2;
   {
   while (len--)
      if (*s1++ != *s2++)
	 return 0;
   return 1;
   }

void report_waste()
   {
   fprintf(stderr, "WASTE REPORT, wasted space: %lu\n", (unsigned long)g_wasted_space);
   fprintf(stderr, "WASTE REPORT, grow count: %lu\n", (unsigned long)g_grow_count);
   fprintf(stderr, "WASTE REPORT, init count: %lu\n", (unsigned long)g_sbuf_init_count);
   fprintf(stderr, "WASTE REPORT, donate count: %lu\n", (unsigned long)g_sbuf_donate_count);
   }
