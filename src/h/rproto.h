/*
 * Prototypes for run-time functions.
 */

/*
 * Prototypes common to the compiler and interpreter.
 */
void		EVInit		(void);
int		bfunc		(void);
void		coclean		(void *o);
int		coswitch	(void *o, void *n, int first);
int		doasgn		(dptr dp1,dptr dp2);
void		dumpact		(struct b_coexpr *ce);
void		mksubs		(dptr var,dptr val,word i,word j, dptr result);
int		numcmp		(dptr dp1,dptr dp2,dptr dp3);
word		prescan		(dptr d);
int		 radix		(int sign, register int r, register char *s,
				   register char *end_s, union numeric *result);
void		stkdump		(int);
void		xmfree		(void);

#ifdef FAttrib
   char *make_mode(mode_t st_mode);
#endif					/* FAttrib */

#ifdef Graphics

#if NUKE

   /*
    * portable graphics routines in rwindow.r and rwinrsc.r
    */
   int	atobool		(char *s);
   int	docircles	(wbp w, int argc, dptr argv, int fill);
   void	drawCurve	(wbp w, XPoint *p, int n);
   char	*evquesub	(wbp w, int i);
   void	genCurve	(wbp w, XPoint *p, int n, void (*h)());
   wsp	getactivewindow	(void);
   int	getpattern	(wbp w, char *answer);
   struct palentry *palsetup(int p);
   int	palnum		(dptr d);
   int	parsecolor	(wbp w, char *s, long *r, long *g, long *b);
   int	parsefont	(char *s, char *fam, int *sty, int *sz);
   int	parsegeometry	(char *buf, SHORT *x, SHORT *y, SHORT *w, SHORT *h);
   int	parsepattern	(char *s, int len, int *w, int *nbits, C_integer *bits);
   void	qevent		(wsp ws, dptr e, int x, int y, uword t, long f);
   int	readGIF		(char *fname, int p, struct imgdata *d);
   int	rectargs	(wbp w, int argc, dptr argv, int i,
			   word *px, word *py, word *pw, word *ph);
   char	*rgbkey		(int p, double r, double g, double b);
   int	setsize		(wbp w, char *s);
   char	*si_i2s		(siptr sip, int i);
   int	si_s2i		(siptr sip, char *s);
   int	ulcmp		(pointer p1, pointer p2);
   int	wattrib		(wbp w, char *s, long len, dptr answer, char *abuf);
   int	wgetche		(wbp w, dptr res);
   int	wgetchne	(wbp w, dptr res);
   int	wgetevent	(wbp w, dptr res);
   int	wgetstrg	(char *s, long maxlen, FILE *f);
   void	wgoto		(wbp w, int row, int col);
   int	wlongread	(char *s, int elsize, int nelem, FILE *f);
   void	wputstr		(wbp w, char *s, int len);
   int	writeGIF	(wbp w, char *filename,
			  int x, int y, int width, int height);
   int	xyrowcol	(dptr dx);

   /*
    * graphics implementation routines supplied for each platform
    * (excluding those defined as macros for X-windows)
    */
   int	SetPattern	(wbp w, char *name, int len);
   int	SetPatternBits	(wbp w, int width, C_integer *bits, int nbits);
   int	blimage		(wbp w, int x, int y, int wd, int h,
			  int ch, unsigned char *s, word len);
   int	capture		(wbp w, int x, int y, int width, int hgt, short *data);
   int	copyArea	(wbp w,wbp w2,int x,int y,int wd,int h,int x2,int y2);
   int	dumpimage	(wbp w, char *filename, unsigned int x, unsigned int y,
			   unsigned int width, unsigned int height);
   void	eraseArea	(wbp w, int x, int y, int width, int height);
   void	fillrectangles	(wbp w, XRectangle *recs, int nrecs);
   void	free_mutable	(wbp w, int mute_index);
   void	freecolor	(wbp w, char *s);
   char	*get_mutable_name (wbp w, int mute_index);
   void	getbg		(wbp w, char *answer);
   void	getcanvas	(wbp w, char *s);
   int	getdefault	(wbp w, char *prog, char *opt, char *answer);
   void	getdisplay	(wbp w, char *answer);
   void	getdrawop	(wbp w, char *answer);
   void	getfg		(wbp w, char *answer);
   void	getfntnam	(wbp w, char *answer);
   void	geticonic	(wbp w, char *answer);
   int	geticonpos	(wbp w, char *s);
   void	getlinestyle	(wbp w, char *answer);
   int	getpixel_init	(wbp w, struct imgmem *imem);
   int	getpixel_term	(wbp w, struct imgmem *imem);
   int	getpixel	(wbp w,int x,int y,long *rv,char *s,struct imgmem *im);
   void	getpointername	(wbp w, char *answer);
   int	getpos		(wbp w);
   int	getvisual	(wbp w, char *answer);
   int	isetbg		(wbp w, int bg);
   int	isetfg		(wbp w, int fg);
   int	lowerWindow	(wbp w);
   int	mutable_color	(wbp w, dptr argv, int ac, int *retval);
   int	nativecolor	(wbp w, char *s, long *r, long *g, long *b);
   int	query_pointer	(wbp w, XPoint *pp);
   int	query_rootpointer (XPoint *pp);
   int	raiseWindow	(wbp w);
   int	readimage	(wbp w, char *filename, int x, int y, int *status);
   int	set_mutable	(wbp w, int i, char *s);
   int	setbg		(wbp w, char *s);
   int	setcanvas	(wbp w, char *s);
   int	setdisplay	(wbp w, char *s);
   int	setfg		(wbp w, char *s);
   int	setfillstyle	(wbp w, char *s);
   int	setfont		(wbp w, char **s);
   int	setgamma	(wbp w, double gamma);
   int	seticonicstate	(wbp w, char *s);
   int	seticonpos	(wbp w, char *s);
   int	setimage	(wbp w, char *val);
   int	setleading	(wbp w, int i);
   int	setlinestyle	(wbp w, char *s);
   int	setlinewidth	(wbp w, LONG linewid);
   int	strimage	(wbp w, int x, int y, int width, int height,
			   struct palentry *e, unsigned char *s,
			   word len, int on_icon);
   void	toggle_fgbg	(wbp w);
   int	walert		(wbp w, int volume);
#endif

   #ifdef XWindows
      int	translate_key_event	(XKeyEvent *k1, char *s, KeySym *k2);
   #endif				/* XWindows */

#if NUKE

   #ifdef XWindows
      /*
       * Implementation routines specific to X-Windows
       */
      int	moveResizeWindow	(wbp w, int x, int y, int wd, int h);
      int	resetfg			(wbp w);
      int	setfgrgb		(wbp w, int r, int g, int b);
      int	setbgrgb		(wbp w, int r, int g, int b);

      XColor	xcolor			(wbp w, LinearColor clr);
      LinearColor	lcolor		(wbp w, XColor color);
      int	pixmap_open		(wbp w, dptr attribs, int argc);
      int	pixmap_init		(wbp w);
      int	remap			(wbp w, int x, int y);
      int	seticonimage		(wbp w, dptr dp);
      int	resizePixmap		(wbp w, int width, int height);
   #endif				/* XWindows */

   #ifdef WinGraphics
      /*
       * Implementation routines specific to MS Windows
       */
      char *nativecolordialog	(wbp w,long r,long g, long b,char *s);
      int nativefontdialog	(wbp w, char *buf, int flags, int fheight);
      char *nativeopendialog	(wbp w,char *s1,char *s2,char *s3,int i,int j);
      char *nativeselectdialog	(wbp w,struct b_list *,char *s);
      char *nativesavedialog	(wbp w,char *s1,char *s2,char *s3,int i,int j);
      HFONT mkfont		(char *s);
      int sysTextWidth		(wbp w, char *s, int n);
      int sysFontHeight		(wbp w);
      int mswinsystem		(char *s);
      void UpdateCursorPos	(wsp ws, wcp wc);
      LRESULT_CALLBACK WndProc	(HWND, UINT, WPARAM, LPARAM);
      HDC CreateWinDC		(wbp);
      HDC CreatePixDC		(wbp, HDC);
      HBITMAP loadimage	(wbp wb, char *filename, unsigned int *width,
			unsigned int *height, int atorigin, int *status);
      void wfreersc();
      int getdepth(wbp w);
      HBITMAP CreateBitmapFromData(char *data);
      int resizePixmap(wbp w, int width, int height);
      int textWidth(wbp w, char *s, int n);
      int	seticonimage		(wbp w, dptr dp);
      int devicecaps(wbp w, int i);
      void fillarcs(wbp wb, XArc *arcs, int narcs);
      void drawarcs(wbp wb, XArc *arcs, int narcs);
      void drawlines(wbinding *wb, XPoint *points, int npoints);
      void drawpoints(wbinding *wb, XPoint *points, int npoints);
      void drawrectangles(wbp wb, XRectangle *recs, int nrecs);
      void fillpolygon(wbp w, XPoint *pts, int npts);
      void drawsegments(wbinding *wb, XSegment *segs, int nsegs);
      void drawstrng(wbinding *wb, int x, int y, char *s, int slen);
      void unsetclip(wbp w);

   #endif				/* WinGraphics */

#endif

#endif					/* Graphics */

long	ckadd		(long i, long j);
long	ckmul		(long i, long j);
long	cksub		(long i, long j);
void	cmd_line	(int argc, char **argv, dptr rslt);
struct b_coexpr *create	(continuation fnc,struct b_proc *p,int ntmp,int wksz);
int	cvcset		(dptr dp,int * *cs,int *csbuf);
int	cvnum		(dptr dp,union numeric *result);
int	cvreal		(dptr dp,double *r);
C_integer iipow		(C_integer n1, C_integer n2);
void	init		(char *name, int *argcp, char *argv[], int trc_init);
int	mkreal		(double r,dptr dp);
int	sig_rsm		(void);
void	varargs		(dptr argp, int nargs, dptr rslt);

#define Fargs dptr cargp
