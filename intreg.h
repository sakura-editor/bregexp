/*    intreg.h
    internal regexp header
 */

/*
 * Definitions etc. for regexp(3) routines.
 *
 * Caveat:  this is V8 regexp(3) [actually, a reimplementation thereof],
 * not the System V one.
 */

typedef  struct repstr {
	int  count;		/* entry counter */
	char **startp;	/* start address  if <256 \digit	*/
	int  *dlen;		/* data length	*/
	char data[1];	/* data start   */
} REPSTR;


typedef struct regexp {
	char *outp;			/* matched or substitute string start ptr   */
	char *outendp;		/* matched or substitute string end ptr     */ 
	int  splitctr;		/* split result counrer     */ 
	char **splitp;		/* split result pointer ptr     */ 
	int	rsv1;			/* reserved for external use    */ 
	char *parap;		/* parameter start ptr ie. "s/xxxxx/yy/gi"  */
	char *paraendp;		/* parameter end ptr     */ 
	char *transtblp;	/* translate table ptr   */
	char **startp;		/* match string start ptr   */
	char **endp;		/* match string end ptr     */ 
	int nparens;		/* number of parentheses */
// external field end point  1999.05.12
	int lastparen;		/* last paren matched */
	SV *regstart;		/* Internal use only. */
	char *regstclass;	/* class definition ie. [xxxx]  */
	SV *regmust;		/* Internal use only.			*/
	int regback;		/* Can regmust locate first try? */
	int minlen;			/* mininum possible length of $& */
	int prelen;			/* length of precomp */
	char *precomp;		/* pre-compilation regular expression */
	char *subbase;		/* saved string so \digit works forever */
	char *subbeg;		/* same, but not responsible for allocation */
	char *subend;		/* end of subbase */
	int  pmflags;		/* flags  */
	int naughty;		/* how exponential is this pattern? */
	char *prerepp;	    /* original replace string */
	char *prerependp;	/* original replace string end */
	REPSTR *repstr;		/* this part is for substitute */
	char reganch;		/* Internal use only. */
	char do_folding;	/* do case-insensitive match? */
	char program[1];	/* Unwarranted chumminess with compiler. */
} regexp;


regexp *bregcomp(char* exp,char* xend,int* flag,char*msg);

BOOL bregexec(register regexp *prog,char *stringarg,
register char *strend,	/* pointer to null at end of string */
char *strbeg,	/* real beginning of string */
int minend,		/* end of match must be at least minend after stringarg */
int safebase,	/* no need to remember string in subbase */
char *msg);		/* fatal error message */

BOOL split(regexp* rx,char *target,char *targetendp,
			int limit,char *msg);
regexp *compile(const char* str,int plen,char *msg);
// 2003.11.1 Karoto : Add parameter "targetbegp"
BOOL subst(regexp* rx,char *target,char *targetendp,char *targetbegp,char *msg);
int trans(regexp* rx,char *target,char *targetendp,char *msg);
//REPSTR *cvrepstr(regexp* rx,char *str,char* strend);
