//
//    bregcomp.cc
//
////////////////////////////////////////////////////////////////////////////////
//  1999.11.24   update by Tatsuo Baba
//
//  You may distribute under the terms of either the GNU General Public
//  License or the Artistic License, as specified in the perl_license.txt file.
////////////////////////////////////////////////////////////////////////////////


/*
 * "A fair jaw-cracker dwarf-language must be."  --Samwise Gamgee
 */

/* NOTE: this is derived from Henry Spencer's regexp code, and should not
 * confused with the original package (see point 3 below).  Thanks, Henry!
 */

/* Additional note: this code is very heavily munged from Henry's version
 * in places.  In some spots I've traded clarity for efficiency, so don't
 * blame Henry for some of the lack of readability.
 */

/* The names of the functions have been changed from regcomp and
 * regexec to  pregcomp and pregexec in order to avoid conflicts
 * with the POSIX routines of the same names.
*/

/*SUPPRESS 112*/
/*
 * pregcomp and pregexec -- regsub and regerror are not used in perl
 *
 *	Copyright (c) 1986 by University of Toronto.
 *	Written by Henry Spencer.  Not derived from licensed software.
 *
 *	Permission is granted to anyone to use this software for any
 *	purpose on any computer system, and to redistribute it freely,
 *	subject to the following restrictions:
 *
 *	1. The author is not responsible for the consequences of use of
 *		this software, no matter how awful, even if they arise
 *		from defects in it.
 *
 *	2. The origin of this software must not be misrepresented, either
 *		by explicit claim or by omission.
 *
 *	3. Altered versions must be plainly marked as such, and must not
 *		be misrepresented as being the original software.
 *
 *
 ****    Alterations to Henry's code are...
 ****
 ****    Copyright (c) 1991-1994, Larry Wall
 ****
 ****    You may distribute under the terms of either the GNU General Public
 ****    License or the Artistic License, as specified in the README file.

 *
 * Beware that some of this code is subtly aware of the way operator
 * precedence is structured in regular expressions.  Serious changes in
 * regular-expression syntax might require a total rethink.
 */
////////////////////////////////////////////////////////////////////////////////
//  Alterations to Larry Wall's code are...
//  Copyright 1999 Tatsuo Baba
// 
//  1999.11.24   Tatsuo Baba
// 
//  You may distribute under the terms of either the GNU General Public
//  License or the Artistic License, as specified in the perl_license.txt file.
////////////////////////////////////////////////////////////////////////////////


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define KANJI
//#define hints 1
//#define HINT_KANJI_STRING	1



#include "global.h"
#include "sv.h"

#include "intreg.h"

typedef struct bregexp {
	char *outp;			/* matched or substitute string start ptr   */
	char *outendp;		/* matched or substitute string end ptr     */ 
	char **splitp;		/* split result pointer ptr     */ 
} BREGEXP;

extern "C"
void BRegfree(BREGEXP* exp);


static char regarglen[] = {0,0,0,0,0,0,0,0,0,0,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,0,0,0,0,0};
/* The following have no fixed length. */
static char varies[] = {BRANCH,BACK,STAR,PLUS,CURLY,CURLYX,REF,WHILEM,0};

/* The following always have a length of 1. */
static char simple[] = {ANY,SANY,ANYOF,ALNUM,NALNUM,SPACE,NSPACE,DIGIT,NDIGIT,0};

static char regdummy;


#ifdef MSDOS
# if defined(BUGGY_MSC6)
 /* MSC 6.00A breaks on op/regexp.t test 85 unless we turn this off */
 # pragma optimize("a",off)
 /* But MSC 6.00A is happy with 'w', for aliases only across function calls*/
 # pragma optimize("w",on )
# endif /* BUGGY_MSC6 */
#endif /* MSDOS */

#ifndef STATIC
#define	STATIC	static
#endif



#define ISKANJI(c) (KANJI_MODE && iskanji(c))

#define	ISMULT1(c)	((c) == '*' || (c) == '+' || (c) == '?')
#define	ISMULT2(s)	((*s) == '*' || (*s) == '+' || (*s) == '?' || \
	((*s) == '{' && regcurly(s)))
#ifdef atarist
#define	PERL_META	"^$.[()|?+*\\"
#else
#define	META	"^$.[()|?+*\\"
#endif

#ifdef SPSTART
#undef SPSTART		/* dratted cpp namespace... */
#endif
/*
 * Flags to be passed up and down.
 */
#define	WORST		0	/* Worst case. */
#define	HASWIDTH	0x1	/* Known never to match null string. */
#define	SIMPLE		0x2	/* Simple enough to be STAR/PLUS operand. */
#define	SPSTART		0x4	/* Starts with * or +. */
#define TRYAGAIN	0x8	/* Weeded out a declaration. */





/*
 * Forward declarations for pregcomp()'s friends.
 */

/*
 - pregcomp - compile a regular expression into internal code
 *
 * We can't allocate space until we know how big the compiled form will be,
 * but we can't compile it (and thus know how big it is) until we've got a
 * place to put the code.  So we cheat:  we compile it twice, once with code
 * generation turned off and size counting turned on, and once "for real".
 * This also means that we don't allocate space until we are sure that the
 * thing really will compile successfully, and we never have to move the
 * code and thus invalidate pointers into it.  (Note that it has to be in
 * one piece because free() must be able to free it all.) [NB: not true in perl]
 *
 * Beware that the optimization-preparation code in here knows about some
 * of the structure of the compiled regexp.  [I'll say.]
 */

#define croak(c)	throw(c)
char*	savepvn (char* sv, int len);
void regc(char b,GLOBAL*);
static char* nextchar(GLOBAL*);
static char* Reg(int,int*,GLOBAL*);
static int regcurly(register char*);
char * regnext(register char *p);
void pmflag(int*, int);
static char * regbranch(int*,GLOBAL*);
static void regtail(char*,char*);
static void regoptail(char*,char*);
static void reginsert(char, char*,GLOBAL*);
static char *regpiece(int*,GLOBAL*);


static char *regnode(char,GLOBAL*);
static char *regclass(GLOBAL*);
static char *reganode(char,unsigned short,GLOBAL*);




#define DEFSTATIC


regexp *bregcomp(
char* exp,
char* xend,
int* flag,
char *msg)		// error message
{

	msg[0] = '\0';		//ensure no message at this time

//    int fold = pm->op_pmflags & PMf_FOLD;
    int fold = *flag & PMf_FOLD;
    register regexp *r;
    register char *scan;
    register SV *longish;
    SV *longest;
    register int len;
    register char *first;
    int flags;
    int backish;
    int backest;
    int curback;
    int minlen = 0;
    int sawplus = 0;
    int sawopen = 0;



    //baba
	GLOBAL global;
	GLOBAL *globalp = &global;
	memset((void*)globalp,0,sizeof(GLOBAL));


	kanji_mode_flag = *flag & PMf_KANJI;	// using KANJI_MODE macro


	///// try start   all croak issue throw
	try {

	
    if (exp == NULL)
		croak("NULL argument");

	/* First pass: determine size, legality. */
//    regflags = pm->op_pmflags;
    multiline = *flag & PMf_MULTILINE;	// set multi line mode
    regflags = *flag;
    regparse = exp;
    regxend = xend;
	if (xend > exp)
	    regprecomp = savepvn(exp,xend-exp);
    regnaughty = 0;
    regsawback = 0;
    regnpar = 1;
    regsize = 0L;
    regcode = &regdummy;
    regc((char)MAGIC,globalp);
    if (Reg(0, &flags,globalp) == NULL) {
	//	Safefree(regprecomp);
		delete [] regprecomp;
		regprecomp = NULL;
		return(NULL);
    }

    /* Small enough for pointer-storage convention? */
    if (regsize >= 32767L)		/* Probably could be 65535L. */
		croak("too big compile");

    /* Allocate space. */
//    Newc(1001, r, sizeof(regexp) + (unsigned)regsize, char, regexp);
    r = (regexp*)new char[sizeof(regexp) + (unsigned)regsize];
    if (r == NULL)
		croak("out of space regexp");

	memset(r,0,sizeof(regexp) + (unsigned)regsize);

    regexp_current = (char*)r;		// save for croak


	
	/* Second pass: emit code. */
    r->prelen = xend-exp;
	if (r->prelen == 0) {			// no string
		r->pmflags = *flag;
		return (r);
	}
    
	
	r->precomp = regprecomp;
    regprecomp = NULL;
    r->subbeg = r->subbase = NULL;
    regnaughty = 0;
    regparse = exp;
    regnpar = 1;
    regcode = r->program;
    regc((char)MAGIC,globalp);
    if (Reg(0, &flags,globalp) == NULL)
		return(NULL);

    /* Dig out information for optimizations. */
//    pm->op_pmflags = regflags;
//    fold = pm->op_pmflags & PMf_FOLD;
	r->pmflags = *flag;
    *flag = regflags;
    fold = *flag & PMf_FOLD;
    r->regstart = NULL;	/* Worst-case defaults. */
//    r->reganch = ((hints & HINT_KANJI_REGEXP) ? ROPT_KANJI : 0);
    r->reganch = ROPT_KANJI;
    r->regmust = NULL;
    r->regback = -1;
    r->regstclass = NULL;
    r->naughty = regnaughty >= 10;	/* Probably an expensive pattern. */
    scan = r->program+1;			/* First BRANCH. */
    if (OP(regnext(scan)) == END) {/* Only one top-level choice. */
	scan = NEXTOPER(scan);

	first = scan;
	while ((OP(first) == OPEN && (sawopen = 1)) ||
	    (OP(first) == BRANCH && OP(regnext(first)) != BRANCH) ||
	    (OP(first) == PLUS) ||
	    (OP(first) == MINMOD) ||
	    (regkind[(unsigned char)OP(first)] == CURLY && ARG1(first) > 0) ) {
		if (OP(first) == PLUS)
		    sawplus = 1;
		else
		    first += regarglen[(unsigned char)OP(first)];
		first = NEXTOPER(first);
	}

	/* Starting-point info. */
      again:
		if (OP(first) == EXACTLY) {
	    r->regstart = newSVpv(OPERAND(first)+1,*OPERAND(first));
//	    if (SvCUR(r->regstart) > (unsigned int)!(sawstudy|fold))
	    if (SvCUR(r->regstart) > (int)!(fold))
		fbm_compile(r->regstart,fold);
	    else
		sv_upgrade(r->regstart, SVt_PVBM);
	}
	else if (strchr(simple+2,OP(first)))
	    r->regstclass = first;
	else if (OP(first) == BOUND || OP(first) == NBOUND)
	    r->regstclass = first;
	else if (regkind[(unsigned char)OP(first)] == BOL) {
	    r->reganch |=  ROPT_ANCH;
	    first = NEXTOPER(first);
	  	goto again;
	}
	else if ((OP(first) == STAR &&
	    regkind[(unsigned char)OP(NEXTOPER(first))] == ANY) &&
	    !(r->reganch & ROPT_ANCH) )
	{
	    /* turn .* into ^.* with an implied $*=1 */
	    r->reganch |= ROPT_ANCH | ROPT_IMPLICIT ;
	    first = NEXTOPER(first);
	  	goto again;
	}
	if (sawplus && (!sawopen || !regsawback))
	    r->reganch |= ROPT_SKIP;	/* x+ must match 1st of run */

	/*
	* If there's something expensive in the r.e., find the
	* longest literal string that must appear and make it the
	* regmust.  Resolve ties in favor of later strings, since
	* the regstart check works with the beginning of the r.e.
	* and avoiding duplication strengthens checking.  Not a
	* strong reason, but sufficient in the absence of others.
	* [Now we resolve ties in favor of the earlier string if
	* it happens that curback has been invalidated, since the
	* earlier string may buy us something the later one won't.]
	*/
	longish = newSVpv("",0);
	longest = newSVpv("",0);
	len = 0;
	minlen = 0;
	curback = 0;
	backish = 0;
	backest = 0;
	while (OP(scan) != END) {
	    if (OP(scan) == BRANCH) {
		if (OP(regnext(scan)) == BRANCH) {
		    curback = -30000;
		    while (OP(scan) == BRANCH)
			scan = regnext(scan);
		}
		else	/* single branch is ok */
		    scan = NEXTOPER(scan);
		continue;
	    }
	    if (OP(scan) == UNLESSM) {
		curback = -30000;
		scan = regnext(scan);
		continue;
	    }
	    if (OP(scan) == EXACTLY) {
		char *t;

		first = scan;
		while (OP(t = regnext(scan)) == CLOSE)
		    scan = t;
		minlen += *OPERAND(first);
		if (curback - backish == len) {
		    sv_catpvn(longish, OPERAND(first)+1,
			*OPERAND(first));
		    len += *OPERAND(first);
		    curback += *OPERAND(first);
		    first = regnext(scan);
		}
		else if (*OPERAND(first) >= len + (curback >= 0)) {
		    len = *OPERAND(first);
		    sv_setpvn(longish, OPERAND(first)+1,len);
		    backish = curback;
		    curback += len;
		    first = regnext(scan);
		}
		else
		    curback += *OPERAND(first);
	    }
	    else if (strchr(varies,OP(scan))) {
		curback = -30000;
		len = 0;
		if (SvCUR(longish) > SvCUR(longest)) {
		    sv_setsv(longest,longish);
		    backest = backish;
		}
		sv_setpvn(longish,"",0);
		if (OP(scan) == PLUS && strchr(simple,OP(NEXTOPER(scan))))
		    minlen++;
		else if (regkind[(unsigned char)OP(scan)] == CURLY &&
		  strchr(simple,OP(NEXTOPER(scan)+4)))
		    minlen += ARG1(scan);
	    }
	    else if (strchr(simple,OP(scan))) {
		curback++;
		minlen++;
		len = 0;
		if (SvCUR(longish) > SvCUR(longest)) {
		    sv_setsv(longest,longish);
		    backest = backish;
		}
		sv_setpvn(longish,"",0);
	    }
	    scan = regnext(scan);
	}

	/* Prefer earlier on tie, unless we can tail match latter */

	if (SvCUR(longish) + (regkind[(unsigned char)OP(first)] == EOL) >
		SvCUR(longest))
	{
	    sv_setsv(longest,longish);
	    backest = backish;
	}
	else
	    sv_setpvn(longish,"",0);
	if (SvCUR(longest)
	    &&
	    (!r->regstart
	     ||
	     !fbm_instr((unsigned char*) SvPVX(r->regstart),
		  (unsigned char *) SvPVX(r->regstart)
		    + SvCUR(r->regstart),
		  longest,multiline,KANJI_MODE)
	    )
	   )
	{
	    r->regmust = longest;
	    if (backest < 0)
		backest = -1;
	    r->regback = backest;
//	    if (SvCUR(longest) > (unsigned int)!(sawstudy || fold ||
	    if (SvCUR(longest) > (int)!(fold ||
			regkind[(unsigned char)OP(first)]==EOL))
		fbm_compile(r->regmust,fold);
	    (void)SvUPGRADE(r->regmust, SVt_PVBM);
	    BmUSEFUL(r->regmust) = 100;
	    if (regkind[(unsigned char)OP(first)] == EOL && SvCUR(longish))
		SvTAIL_on(r->regmust);
	}
	else {
	    SvREFCNT_dec(longest);
	    longest = NULL;
	}
	SvREFCNT_dec(longish);
    }

    r->do_folding = (char)fold;
    r->nparens = regnpar - 1;
    r->minlen = minlen;
//    Newz(1002, r->startp, regnpar, char*);
	r->startp = (char**)new char[regnpar * sizeof(char*)];
    if (r->startp == NULL)
		croak("out of space startp");

	memset(r->startp,0,regnpar * sizeof(char*));
  //  Newz(1002, r->endp, regnpar, char*);
	r->endp = (char**)new char[regnpar * sizeof(char*)];
    if (r->endp == NULL)
		croak("out of space endp");
	memset(r->endp,0,regnpar * sizeof(char*));
    return(r);


    /////// catch start

	}
	catch (const char* err) {
		if (regexp_current)
			BRegfree((BREGEXP*)regexp_current);

		if (regprecomp)
			delete [] regprecomp;
		
		strcpy(msg,err);
		return NULL;
	}
	catch (...) {
		if (regexp_current)
			BRegfree((BREGEXP*)regexp_current);

		if (regprecomp)
			delete [] regprecomp;
		
		strcpy(msg,"fatal error");
		return NULL;
	}


}

/*
 - regatom - the lowest level
 *
 * Optimization:  gobbles an entire sequence of ordinary characters so that
 * it can turn them into a single node, which is smaller to store and
 * faster to run.  Backslashed characters are exceptions, each becoming a
 * separate node; the code is simpler that way and it's not worth fixing.
 *
 * [Yes, it is worth fixing, some scripts can run twice the speed.]
 */
static char *regatom(int *flagp,GLOBAL* globalp)
{
    register char *ret = 0;
    int flags;

    *flagp = WORST;		/* Tentatively. */

tryagain:
    switch (*regparse) {
    case '^':
	nextchar(globalp);
	if (regflags & PMf_MULTILINE)
	    ret = regnode(MBOL,globalp);
	else if (regflags & PMf_SINGLELINE)
	    ret = regnode(SBOL,globalp);
	else
	    ret = regnode(BOL,globalp);
	break;
    case '$':
	nextchar(globalp);
	if (regflags & PMf_MULTILINE)
	    ret = regnode(MEOL,globalp);
	else if (regflags & PMf_SINGLELINE)
	    ret = regnode(SEOL,globalp);
	else
	    ret = regnode(EOL,globalp);
	break;
    case '.':
	nextchar(globalp);
	if (regflags & PMf_SINGLELINE)
	    ret = regnode(SANY,globalp);
	else
	    ret = regnode(ANY,globalp);
	regnaughty++;
	*flagp |= HASWIDTH;
	break;
    case '[':
	regparse++;
	ret = regclass(globalp);
	*flagp |= HASWIDTH;
	break;
    case '(':
	nextchar(globalp);
	ret = Reg(1, &flags,globalp);
	if (ret == NULL) {
		if (flags & TRYAGAIN)
		    goto tryagain;
		return(NULL);
	}
	*flagp |= flags&(HASWIDTH|SPSTART);
	break;
    case '|':
    case ')':
	if (flags & TRYAGAIN) {
	    *flagp |= TRYAGAIN;
	    return NULL;
	}
	croak("internal urp at /%s/");
				/* Supposed to be caught earlier. */
	break;
    case '?':
    case '+':
    case '*':
		croak("?+* follows nothing");
	break;
    case '\\':
	switch (*++regparse) {
	case 'A':
	    ret = regnode(SBOL,globalp);
	    *flagp |= SIMPLE;
	    nextchar(globalp);
	    break;
	case 'G':
	    ret = regnode(GBOL,globalp);
	    *flagp |= SIMPLE;
	    nextchar(globalp);
	    break;
	case 'Z':
	    ret = regnode(SEOL,globalp);
	    *flagp |= SIMPLE;
	    nextchar(globalp);
	    break;
	case 'w':
	    ret = regnode(ALNUM,globalp);
	    *flagp |= HASWIDTH|SIMPLE;
	    nextchar(globalp);
	    break;
	case 'W':
	    ret = regnode(NALNUM,globalp);
	    *flagp |= HASWIDTH|SIMPLE;
	    nextchar(globalp);
	    break;
	case 'b':
	    ret = regnode(BOUND,globalp);
	    *flagp |= SIMPLE;
	    nextchar(globalp);
	    break;
	case 'B':
	    ret = regnode(NBOUND,globalp);
	    *flagp |= SIMPLE;
	    nextchar(globalp);
	    break;
	case 's':
	    ret = regnode(SPACE,globalp);
	    *flagp |= HASWIDTH|SIMPLE;
	    nextchar(globalp);
	    break;
	case 'S':
	    ret = regnode(NSPACE,globalp);
	    *flagp |= HASWIDTH|SIMPLE;
	    nextchar(globalp);
	    break;
	case 'd':
	    ret = regnode(DIGIT,globalp);
	    *flagp |= HASWIDTH|SIMPLE;
	    nextchar(globalp);
	    break;
	case 'D':
	    ret = regnode(NDIGIT,globalp);
	    *flagp |= HASWIDTH|SIMPLE;
	    nextchar(globalp);
	    break;
	case 'n':
	case 'r':
	case 't':
	case 'f':
	case 'e':
	case 'a':
	case 'x':
	case 'c':
	case '0':
	    goto defchar;
	case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	    {
		int num = atoi(regparse);

		if (num > 9 && num >= regnpar)
		    goto defchar;
		else {
		    regsawback = 1;
		    ret = reganode(REF, (unsigned short)num,globalp);
		    *flagp |= HASWIDTH;
		    while (isDIGIT(*regparse))
			regparse++;
		    regparse--;
		    nextchar(globalp);
		}
	    }
	    break;
	case '\0':
	    if (regparse >= regxend)
		FAIL("trailing \\ in regexp");
	    /* FALL THROUGH */
	default:
	    goto defchar;
	}
	break;

    case '#':
	if (regflags & PMf_EXTENDED) {
	    while (regparse < regxend && *regparse != '\n') regparse++;
	    if (regparse < regxend)
		goto tryagain;
	}
	/* FALL THROUGH */

    default: {
	    register int len;
	    register char ender;
	    register char *p;
	    char *oldp;
	    int numlen;

	    regparse++;

	defchar:
	    ret = regnode(EXACTLY,globalp);
	    regc(0,globalp);		/* save spot for len */
	    for (len = 0, p = regparse - 1;
	      len < 126 && p < regxend;
	      len++)
	    {
		oldp = p;
		switch (*p) {
		case '^':
		case '$':
		case '.':
		case '[':
		case '(':
		case ')':
		case '|':
		    goto loopdone;
		case '\\':
		    switch (*++p) {
		    case 'A':
		    case 'G':
		    case 'Z':
		    case 'w':
		    case 'W':
		    case 'b':
		    case 'B':
		    case 's':
		    case 'S':
		    case 'd':
		    case 'D':
			--p;
			goto loopdone;
		    case 'n':
			ender = '\n';
			p++;
			break;
		    case 'r':
			ender = '\r';
			p++;
			break;
		    case 't':
			ender = '\t';
			p++;
			break;
		    case 'f':
			ender = '\f';
			p++;
			break;
		    case 'e':
			ender = '\033';
			p++;
			break;
		    case 'a':
			ender = '\007';
			p++;
			break;
		    case 'x':
			ender = (char)scan_hex(++p, 2, &numlen);
			p += numlen;
			break;
		    case 'c':
			p++;
			ender = *p++;
			if (isLOWER(ender))
			    ender = toUPPER(ender);
			ender ^= 64;
			break;
		    case '0': case '1': case '2': case '3':case '4':
		    case '5': case '6': case '7': case '8':case '9':
			if (*p == '0' ||
			  (isDIGIT(p[1]) && atoi(p) >= regnpar) ) {
			    ender = (char)scan_oct(p, 3, &numlen);
			    p += numlen;
			}
			else {
			    --p;
			    goto loopdone;
			}
			break;
		    case '\0':
			if (p >= regxend)
			    FAIL("trailing \\ in regexp");
			/* FALL THROUGH */
		    default:
			ender = *p++;
			break;
		    }
		    break;
		case '#':
		    if (regflags & PMf_EXTENDED) {
			while (p < regxend && *p != '\n') p++;
		    }
		    /* FALL THROUGH */
		case ' ': case '\t': case '\n': case '\r': case '\f': case '\v':
		    if (regflags & PMf_EXTENDED) {
			p++;
			len--;
			continue;
		    }
		    /* FALL THROUGH */
		default:
		    ender = *p++;
		    break;
		}
		if (regflags & PMf_FOLD && isUPPER(ender))
		    ender = toLOWER(ender);
		if (ISKANJI(ender) && *p) {
		    if (ISMULT2((p+1))) {
			/* */
			if (len) {
			    p = oldp;
			} else {
			    len += 2;
			    regc(ender,globalp);
			    ender = *p++;
			    regc(ender,globalp);
			}
			break;
		    }
		    len++;
		    regc(ender,globalp);
		    ender = *p++;
		} else if (ISMULT2(p)) { /* Back off on ?+*. */
		    if (len)
			p = oldp;
		    else {
			len++;
			regc(ender,globalp);
		    }
		    break;
		}
		regc(ender,globalp);
	    }
	loopdone:
	    regparse = p - 1;
	    nextchar(globalp);
	    if (len < 0)
		FAIL("internal disaster in regexp");
	    if (len > 0)
		*flagp |= HASWIDTH;
	    if (len == 1)
		*flagp |= SIMPLE;
	    if (regcode != &regdummy)
		*OPERAND(ret) = (char)len;
	    regc('\0',globalp);
	}
	break;
    }

    return(ret);
}

static void reg_class_exact(int ex,register GLOBAL* globalp)
{
	char c;
	regc((char)ANYOF_EXACT,globalp);
	c = ex >> 8; regc(c,globalp);
	c = ex;      regc(c,globalp);
}

static void reg_class_fromto(int from,int to,GLOBAL* globalp)
{
	char c;

	regc((char)ANYOF_FROMTO,globalp);
	c = from >> 8; regc(c,globalp);
	c = from;      regc(c,globalp);
	c = to >> 8; regc(c,globalp);
	c = to;      regc(c,globalp);
}


static void char_bitmap_set(int c,GLOBAL* globalp)
{
    c &= 255;
    char_bitmap[c >> 3] |=  (1 << (c & 7));
}

static void char_bitmap_unset(int c,GLOBAL* globalp)
{
    c &= 255;
    char_bitmap[c >> 3] &=  ~(1 << (c & 7));
}

static unsigned char char_bitmap_set_p(int c,GLOBAL* globalp)
{
    return (char_bitmap[c >> 3] & (1 << (c & 7)));
}

static void char_bitmap_compile(GLOBAL* globalp)
{
    int i = 0;
    int from; 
    int to;
//    char c;

    /* look for the first nonzero */
    for (;;) {
	for (; i < 256; i++) {
	    if (char_bitmap_set_p(i,globalp)) {
		break;
	    }
	}
	if (i >= 256) break;
	from = i;

	for (; i < 256; i++) {
	    if (!char_bitmap_set_p(i,globalp)) {
		break;
	    }
	}
	to = i-1;
#ifndef	originalkanji_reg
	if (from == to)
	    reg_class_exact(to,globalp) ;
	else
#endif
	reg_class_fromto(from, to,globalp);
	if (i >= 256) break;
    }
}


static char *regclass(GLOBAL* globalp)
{
    /* Since we possibly have 16 bit characters, we can't use 
       simple bitmap method deployed by the original source. 
	   
       So, we introduce 4 new operators.
       
       ANYOF_EXACT 'c'  -- exact char match in [..]
       ANYOF_FROMTO 'f' 't' -- 'f' to 't'.
       ANYOF_ENDMARK -- end of [..]
       ANYOF_COMPL --  indicates [^.. ].
       'c', 'f', 't' are 2 bytes long.

	   */
//	register char *bits;
	register int localClass;
	register int lastclass;
	register int range = 0;
	register char *ret;
	int numlen;

	ret = regnode(ANYOF,globalp);
	if (*regparse == '^') {	/* Complement of range. */
	    regparse++;
	    regnaughty++;
	    regc((char)ANYOF_COMPL,globalp);
	} else {
	    regc(0,globalp);
	}

	if (*regparse == ']' || *regparse == '-')
		goto skipcond;		/* allow 1st char to be ] or - */
	while (regparse < regxend && *regparse != ']') {
	      skipcond:
		localClass = UCHARAT(regparse++);
		if (ISKANJI((unsigned char)localClass) && regparse < regxend) {
			localClass = (localClass << 8) + UCHARAT(regparse++);
		}
		if (localClass == '\\') {
			localClass = UCHARAT(regparse++);
			switch (localClass) {
			  case 'w':
			    memset(char_bitmap, 0, sizeof char_bitmap);
			    for (localClass = 0; localClass < 256; localClass++) {
				if (isALNUM(localClass)) char_bitmap_set(localClass,globalp);
			    }
			    char_bitmap_compile(globalp);
			    lastclass = 0xFFFF;
			    continue;
			  case 'W':
			    memset(char_bitmap, ~0, sizeof char_bitmap);
			    for (localClass = 0; localClass < 256; localClass++) {
				if (isALNUM(localClass)) char_bitmap_unset(localClass,globalp);
				//if (!isALNUM(localClass)) char_bitmap_unset(localClass);
				// This is defect condition.
			    }
			    char_bitmap_compile(globalp);
			    reg_class_fromto(0x100, 0xFFFF,globalp);
			    lastclass = 0xFFFF;
			    continue;
			  case 's':
			    memset(char_bitmap, 0, sizeof char_bitmap);
			    for (localClass = 0; localClass < 256; localClass++)
			      if (isSPACE(localClass)) char_bitmap_set(localClass,globalp);
			    char_bitmap_compile(globalp);
			    lastclass = 0xFFFF;
			    continue;
			  case 'S':
			    memset(char_bitmap, ~0, sizeof char_bitmap);
			    for (localClass = 0; localClass < 256; localClass++) {
				if (isSPACE(localClass)) char_bitmap_unset(localClass,globalp);
				//if (!isSPACE(localClass)) char_bitmap_unset(localClass);
				// This is defect condition.
			    }
			    char_bitmap_compile(globalp);
			    reg_class_fromto(0x100, 0xFFFF,globalp);
			    lastclass = 0xFFFF;
			    continue;
			  case 'd':
			    reg_class_fromto('0', '9',globalp);
			    lastclass = 0xFFFF;
			    continue;
			  case 'D':
			    reg_class_fromto(0, '0'-1,globalp);
			    reg_class_fromto('9'+1, 255,globalp);
			    reg_class_fromto(0x100, 0xFFFF,globalp);
			    lastclass = 0xFFFF;
			    continue;
			  case 'n':
			    localClass = '\n';
			    break;
			  case 'r':
			    localClass = '\r';
			    break;
			  case 't':
			    localClass = '\t';
			    break;
			  case 'f':
			    localClass = '\f';
			    break;
			  case 'b':
			    localClass = '\b';
			    break;
			  case 'e':
			    localClass = '\033';
			    break;
			  case 'a':
			    localClass = '\007';
			    break;
			  case 'x':
			    localClass = scan_hex(regparse, 2, &numlen);
			    regparse += numlen;
			    break;
			  case 'c':
			    localClass = *regparse++;
			    if (isLOWER(localClass))
			      localClass = toUPPER(localClass);
			    localClass ^= 64;
			    break;
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
			    localClass = scan_oct(--regparse, 3, &numlen);
			    regparse += numlen;
			    break;
			}
		}
		if (range) {
		    if (lastclass > localClass)
		      FAIL("invalid [] range in regexp");
		    range = 0;
		}
		else {
		    lastclass = localClass;
		    if (*regparse == '-' && regparse+1 < regxend &&
			regparse[1] != ']') {
			regparse++;
			range = 1;
			continue;	/* do it next time */
		    }
		}
		if (lastclass == localClass) {
		    reg_class_exact(lastclass,globalp);
		    if (regflags & PMf_FOLD &&
			lastclass < 256 && isUPPER(lastclass))
		      reg_class_exact(toLOWER(lastclass),globalp);
		} else {
		    reg_class_fromto(lastclass, localClass,globalp);
		    if (regflags & PMf_FOLD &&
			lastclass < 256 && isUPPER(lastclass))
		      reg_class_fromto(toLOWER(lastclass), toLOWER(localClass),globalp);
		}
		lastclass = localClass;
	}
	if (*regparse != ']')
		FAIL("unmatched [] in regexp");
	regparse++;
	regc((char)ANYOF_ENDMARK,globalp);
	return ret;
}


// Same original "regcomp.cpp" source.
/*
 - reg - regular expression, i.e. main body or parenthesized thing
 *
 * Caller must absorb opening parenthesis.
 *
 * Combining parenthesis handling with the base level of regular expression
 * is a trifle forced, but the need to tie the tails of the branches to what
 * follows makes it hard to avoid.
 */
static char *Reg(
int paren,			/* Parenthesized? */
int *flagp,GLOBAL* globalp)
{
    register char *ret;
    register char *br;
    register char *ender = 0;
    register int parno = 0;
    int flags;

    *flagp = HASWIDTH;	/* Tentatively. */

    /* Make an OPEN node, if parenthesized. */
    if (paren) {
	if (*regparse == '?') {
	    regparse++;
	    paren = *regparse++;
	    ret = NULL;
	    switch (paren) {
	    case ':':
	    case '=':
	    case '!':
		break;
	    case '$':
	    case '@':
		croak("Sequence (?%c...) not implemented");
		break;
	    case '#':
		while (*regparse && *regparse != ')')
		    regparse++;
		if (*regparse != ')')
		    croak("Sequence (?#... not terminated");
		nextchar(globalp);
		*flagp = TRYAGAIN;
		return NULL;
	    default:
		--regparse;
		while (*regparse && strchr("iogmsx", *regparse))
		    pmflag(&regflags, *regparse++);
		if (*regparse != ')')
		    croak("Sequence (?%c...) not recognized");
		nextchar(globalp);
		*flagp = TRYAGAIN;
		return NULL;
	    }
	}
	else {
	    parno = regnpar;
	    regnpar++;
	    ret = reganode(OPEN,(unsigned short) parno,globalp);
	}
    } else
	ret = NULL;

    /* Pick up the branches, linking them together. */
    br = regbranch(&flags,globalp);
    if (br == NULL)
	return(NULL);
    if (ret != NULL)
	regtail(ret, br);	/* OPEN -> first. */
    else
	ret = br;
    if (!(flags&HASWIDTH))
	*flagp &= ~HASWIDTH;
    *flagp |= flags&SPSTART;
    while (*regparse == '|') {
	nextchar(globalp);
	br = regbranch(&flags,globalp);
	if (br == NULL)
	    return(NULL);
	regtail(ret, br);	/* BRANCH -> BRANCH. */
	if (!(flags&HASWIDTH))
	    *flagp &= ~HASWIDTH;
	*flagp |= flags&SPSTART;
    }

    /* Make a closing node, and hook it on the end. */
    switch (paren) {
    case ':':
	ender = regnode(NOTHING,globalp);
	break;
    case 1:
	ender = reganode(CLOSE, (unsigned short)parno,globalp);
	break;
    case '=':
    case '!':
	ender = regnode(SUCCEED,globalp);
	*flagp &= ~HASWIDTH;
	break;
    case 0:
	ender = regnode(END,globalp);
	break;
    }
    regtail(ret, ender);

    /* Hook the tails of the branches to the closing node. */
    for (br = ret; br != NULL; br = regnext(br))
	regoptail(br, ender);

    if (paren == '=') {
	reginsert(IFMATCH,ret,globalp);
	regtail(ret, regnode(NOTHING,globalp));
    }
    else if (paren == '!') {
	reginsert(UNLESSM,ret,globalp);
	regtail(ret, regnode(NOTHING,globalp));
    }

    /* Check for proper termination. */
    if (paren && (regparse >= regxend || *nextchar(globalp) != ')')) {
	FAIL("unmatched () in regexp");
    } else if (!paren && regparse < regxend) {
	if (*regparse == ')') {
	    FAIL("unmatched () in regexp");
	} else
	    FAIL("junk on end of regexp");	/* "Can't happen". */
	/* NOTREACHED */
    }

    return(ret);
}

/*
 - regbranch - one alternative of an | operator
 *
 * Implements the concatenation operator.
 */
static char *regbranch(int *flagp,GLOBAL* globalp)
{
    register char *ret;
    register char *chain;
    register char *latest;
    int flags = 0;

    *flagp = WORST;		/* Tentatively. */

    ret = regnode(BRANCH,globalp);
    chain = NULL;
    regparse--;
    nextchar(globalp);
    while (regparse < regxend && *regparse != '|' && *regparse != ')') {
	flags &= ~TRYAGAIN;
	latest = regpiece(&flags,globalp);
	if (latest == NULL) {
	    if (flags & TRYAGAIN)
		continue;
	    return(NULL);
	}
	*flagp |= flags&HASWIDTH;
	if (chain == NULL)	/* First piece. */
	    *flagp |= flags&SPSTART;
	else {
	    regnaughty++;
	    regtail(chain, latest);
	}
	chain = latest;
    }
    if (chain == NULL)	/* Loop ran zero times. */
	(void) regnode(NOTHING,globalp);

    return(ret);
}

/*
 - regpiece - something followed by possible [*+?]
 *
 * Note that the branching code sequences used for ? and the general cases
 * of * and + are somewhat optimized:  they use the same NOTHING node as
 * both the endmarker for their branch list and the body of the last branch.
 * It might seem that this node could be dispensed with entirely, but the
 * endmarker role is not redundant.
 */
static char *regpiece(int *flagp,GLOBAL* globalp)
{
    register char *ret;
    register char op;
    register char *next;
    int flags;
    char *origparse = regparse;
    char *maxpos;
    int min;
    int max = 32767;

    ret = regatom(&flags,globalp);
    if (ret == NULL) {
	if (flags & TRYAGAIN)
	    *flagp |= TRYAGAIN;
	return(NULL);
    }

    op = *regparse;
    if (op == '(' && regparse[1] == '?' && regparse[2] == '#') {
	while (op && op != ')')
	    op = *++regparse;
	if (op) {
	    nextchar(globalp);
	    op = *regparse;
	}
    }

    if (op == '{' && regcurly(regparse)) {
	next = regparse + 1;
	maxpos = NULL;
	while (isDIGIT(*next) || *next == ',') {
	    if (*next == ',') {
		if (maxpos)
		    break;
		else
		    maxpos = next;
	    }
	    next++;
	}
	if (*next == '}') {		/* got one */
	    if (!maxpos)
		maxpos = next;
	    regparse++;
	    min = atoi(regparse);
	    if (*maxpos == ',')
		maxpos++;
	    else
		maxpos = regparse;
	    max = atoi(maxpos);
	    if (!max && *maxpos != '0')
		max = 32767;		/* meaning "infinity" */
	    regparse = next;
	    nextchar(globalp);

	do_curly:
	    if ((flags&SIMPLE)) {
		regnaughty += 2 + regnaughty / 2;
		reginsert(CURLY, ret,globalp);
	    }
	    else {
		regnaughty += 4 + regnaughty;	/* compound interest */
		regtail(ret, regnode(WHILEM,globalp));
		reginsert(CURLYX,ret,globalp);
		regtail(ret, regnode(NOTHING,globalp));
	    }

	    if (min > 0)
		*flagp = (WORST|HASWIDTH);
	    if (max && max < min)
		croak("Can't do {n,m} with n > m");
	    if (regcode != &regdummy) {
#ifdef REGALIGN
		*(unsigned short *)(ret+3) = (unsigned short)min;
		*(unsigned short *)(ret+5) = (unsigned short)max;
#else
		ret[3] = min >> 8; ret[4] = min & 0377;
		ret[5] = max  >> 8; ret[6] = max  & 0377;
#endif
	    }

	    goto nest_check;
	}
    }

    if (!ISMULT1(op)) {
	*flagp = flags;
	return(ret);
    }
    nextchar(globalp);

    *flagp = (op != '+') ? (WORST|SPSTART) : (WORST|HASWIDTH);

    if (op == '*' && (flags&SIMPLE)) {
	reginsert(STAR, ret,globalp);
	regnaughty += 4;
    }
    else if (op == '*') {
	min = 0;
	goto do_curly;
    } else if (op == '+' && (flags&SIMPLE)) {
	reginsert(PLUS, ret,globalp);
	regnaughty += 3;
    }
    else if (op == '+') {
	min = 1;
	goto do_curly;
    } else if (op == '?') {
	min = 0; max = 1;
	goto do_curly;
    }
  nest_check:
//    if (dowarn && regcode != &regdummy && !(flags&HASWIDTH) && max > 10000) {
    if (regcode != &regdummy && !(flags&HASWIDTH) && max > 10000) {
//	warn("%.*s matches null string many times",
//	    regparse - origparse, origparse);
    }

    if (*regparse == '?') {
	nextchar(globalp);
	reginsert(MINMOD, ret,globalp);
#ifdef REGALIGN
	regtail(ret, ret + 4);
#else
	regtail(ret, ret + 3);
#endif
    }
    if (ISMULT2(regparse))
	FAIL("nested *?+ in regexp");

    return(ret);
}



/*
 - regnext - dig the "next" pointer out of a node
 *
 * [Note, when REGALIGN is defined there are two places in regmatch()
 * that bypass this code for speed.]
 */

char *regnext(register char *p)
{
    register int offset;

    if (p == &regdummy)
	return(NULL);

    offset = NEXT(p);
    if (offset == 0)
	return(NULL);

#ifdef REGALIGN
    return(p+offset);
#else
    if (OP(p) == BACK)
	return(p-offset);
    else
	return(p+offset);
#endif
}

/*
- regc - emit (if appropriate) a byte of code
*/
void regc(char b,GLOBAL* globalp)
{
    if (regcode != &regdummy)
	*regcode++ = b;
    else
	regsize++;
}

char* savepvn(char* ptr,int len)
{
	char *buf = new char[len+1];
    if (buf == NULL)
		croak("out of space savepvn");
	memcpy(buf,ptr,len);
	buf[len] = '\0';
	return buf;
}


/*
 - regcurly - a little FSA that accepts {\d+,?\d*}
 */
static int
regcurly(register char *s)
{
    if (*s++ != '{')
	return FALSE;
    if (!isDIGIT(*s))
	return FALSE;
    while (isDIGIT(*s))
	s++;
    if (*s == ',')
	s++;
    while (isDIGIT(*s))
	s++;
    if (*s != '}')
	return FALSE;
    return TRUE;
}

unsigned long
scan_oct(char *start, int len, int *retlen)
{
    register char *s = start;
    register unsigned long retval = 0;

    while (len && *s >= '0' && *s <= '7') {
	retval <<= 3;
	retval |= *s++ - '0';
	len--;
    }
//    if (dowarn && len && (*s == '8' || *s == '9'))
//    if (len && (*s == '8' || *s == '9'))
//	;//	warn("Illegal octal digit ignored");
    *retlen = s - start;
    return retval;
}

static char hexdigit[] = "0123456789abcdef0123456789ABCDEFx";

unsigned long
scan_hex(char *start, int len, int *retlen)
{
    register char *s = start;
    register unsigned long retval = 0;
    char *tmp;

    while (len-- && *s && (tmp = strchr(hexdigit, *s))) {
	retval <<= 4;
	retval |= (tmp - hexdigit) & 15;
	s++;
    }
    *retlen = s - start;
    return retval;
}


/*
- reganode - emit a node with an argument
*/
static char *			/* Location. */
reganode(char op, unsigned short arg,GLOBAL* globalp)
{
    register char *ret;
    register char *ptr;

    ret = regcode;
    if (ret == &regdummy) {
#ifdef REGALIGN
	if (!(regsize & 1))
	    regsize++;
#endif
	regsize += 5;
	return(ret);
    }

#ifdef REGALIGN
#ifndef lint
    if (!((long)ret & 1))
      *ret++ = 127;
#endif
#endif
    ptr = ret;
    *ptr++ = op;
    *ptr++ = '\0';		/* Null "next" pointer. */
    *ptr++ = '\0';
#ifdef REGALIGN
    *(unsigned short *)(ret+3) = arg;
#else
    ret[3] = arg >> 8; ret[4] = arg & 0377;
#endif
    ptr += 2;
    regcode = ptr;

    return(ret);
}


/*
- regnode - emit a node
*/
static char *			/* Location. */
regnode(char op,GLOBAL* globalp)
{
    register char *ret;
    register char *ptr;

    ret = regcode;
    if (ret == &regdummy) {
#ifdef REGALIGN
	if (!(regsize & 1))
	    regsize++;
#endif
	regsize += 3;
	return(ret);
    }

#ifdef REGALIGN
#ifndef lint
    if (!((long)ret & 1))
      *ret++ = 127;
#endif
#endif
    ptr = ret;
    *ptr++ = op;
    *ptr++ = '\0';		/* Null "next" pointer. */
    *ptr++ = '\0';
    regcode = ptr;

    return(ret);
}


static char*
nextchar(GLOBAL* globalp)
{
    char* retval = regparse++;

    for (;;) {
	if (*regparse == '(' && regparse[1] == '?' &&
		regparse[2] == '#') {
	    while (*regparse && *regparse != ')')
		regparse++;
	    regparse++;
	    continue;
	}
	if (regflags & PMf_EXTENDED) {
	    if (isSPACE(*regparse)) {
		regparse++;
		continue;
	    }
	    else if (*regparse == '#') {
		while (*regparse && *regparse != '\n')
		    regparse++;
		regparse++;
		continue;
	    }
	}
	return retval;
    }
}



void
pmflag(int* pmfl, int ch)
{
    if (ch == 'i') {
//	sawi = TRUE;
	*pmfl |= PMf_FOLD;
    }
    else if (ch == 'g')
	*pmfl |= PMf_GLOBAL;
    else if (ch == 'o')
	*pmfl |= PMf_KEEP;
    else if (ch == 'm')
	*pmfl |= PMf_MULTILINE;
    else if (ch == 's')
	*pmfl |= PMf_SINGLELINE;
    else if (ch == 'x')
	*pmfl |= PMf_EXTENDED;
}

static void
regtail(char *p, char *val)
{
    register char *scan;
    register char *temp;
    register int offset;

    if (p == &regdummy)
	return;

    /* Find last node. */
    scan = p;
    for (;;) {
	temp = regnext(scan);
	if (temp == NULL)
	    break;
	scan = temp;
    }

#ifdef REGALIGN
    offset = val - scan;
#ifndef lint
    *(short*)(scan+1) = offset;
#else
    offset = offset;
#endif
#else
    if (OP(scan) == BACK)
	offset = scan - val;
    else
	offset = val - scan;
    *(scan+1) = (offset>>8)&0377;
    *(scan+2) = offset&0377;
#endif
}


/*
- regoptail - regtail on operand of first argument; nop if operandless
*/
static void
regoptail(char *p, char *val)
{
    /* "Operandless" and "op != BRANCH" are synonymous in practice. */
    if (p == NULL || p == &regdummy || regkind[(unsigned char)OP(p)] != BRANCH)
	return;
    regtail(NEXTOPER(p), val);
}


/*
- reginsert - insert an operator in front of already-emitted operand
*
* Means relocating the operand.
*/
static void
reginsert(char op, char *opnd,GLOBAL* globalp)
{
    register char *src;
    register char *dst;
    register char *place;
    register int offset = (regkind[(unsigned char)op] == CURLY ? 4 : 0);

    if (regcode == &regdummy) {
#ifdef REGALIGN
	regsize += 4 + offset;
#else
	regsize += 3 + offset;
#endif
	return;
    }

    src = regcode;
#ifdef REGALIGN
    regcode += 4 + offset;
#else
    regcode += 3 + offset;
#endif
    dst = regcode;
    while (src > opnd)
	*--dst = *--src;

    place = opnd;		/* Op node, where operand used to be. */
    *place++ = op;
    *place++ = '\0';
    *place++ = '\0';
    while (offset-- > 0)
	*place++ = '\0';
#ifdef REGALIGN
    *place++ = '\177';
#endif
}





#ifdef _DEBUG

char* regprop(char *op,char* buf);

/*
 - regdump - dump a regexp onto Perl_debug_log in vaguely comprehensible form
 */
void regdump(regexp *r)
{
    register char *s;
    register char op = EXACTLY;	/* Arbitrary non-END op. */
    register char *next;
	char buf[4096];

    s = r->program + 1;
	for (register int i = 0;i< 64;i++) {
		unsigned char a = *(s+i); 
		sprintf(buf,"%02X\0",a);
		if (i % 4 == 0)
			TRACE(" ");
		if (i % 16 == 0)
			TRACE("\n");
		TRACE(buf);
	}
	TRACE("\n");

    while (op != END) {	/* While that wasn't END last time... */
#ifdef REGALIGN
	if (!((long)s & 1))
	    s++;
#endif
	op = OP(s);
	TRACE2("%2d%s", s-r->program, regprop(s,buf));	/* Where, what. */
	next = regnext(s);
	s += regarglen[(unsigned char)op];
	if (next == NULL)		/* Next ptr. */
	    TRACE0("(0)");
	else 
	    TRACE1("(%d)", (s-r->program)+(next-s));
	s += 3;
	if (op == ANYOF) {
	    s += 32;
	}
	if (op == EXACTLY) {
	    /* Literal string, where present. */
	    s++;
	    TRACE0(" ");
	    TRACE0("<");
		buf[1] = 0;
		while (*s != '\0') {
			buf[0] = *s;
			TRACE0(buf);
			s++;
	    }
		TRACE0(">");
	    s++;
	}
	TRACE0("\n");
    }

    /* Header fields of interest. */
    if (r->regstart)
	TRACE1( "start `%s' ", SvPVX(r->regstart));
    if (r->regstclass)
	TRACE1( "stclass `%s' ", regprop(r->regstclass,buf));
    if (r->reganch & ROPT_ANCH)
	TRACE0( "anchored ");
    if (r->reganch & ROPT_SKIP)
	TRACE0( "plus ");
    if (r->reganch & ROPT_IMPLICIT)
	TRACE0( "implicit ");
    if (r->regmust != NULL)
	TRACE2( "must have \"%s\" back %ld ", SvPVX(r->regmust),
	 (long) r->regback);
    TRACE1( "minlen %ld ", (long) r->minlen);
    TRACE0("\n");
}

/*
- regprop - printable representation of opcode
*/
char* regprop(char *op,char* buf)
{
    register char *p = 0;
    (void) strcpy(buf, ":");

    switch (OP(op)) {
    case BOL:
	p = "BOL";
	break;
    case MBOL:
	p = "MBOL";
	break;
    case SBOL:
	p = "SBOL";
	break;
    case EOL:
	p = "EOL";
	break;
    case MEOL:
	p = "MEOL";
	break;
    case SEOL:
	p = "SEOL";
	break;
    case ANY:
	p = "ANY";
	break;
    case SANY:
	p = "SANY";
	break;
    case ANYOF:
	p = "ANYOF";
	break;
    case BRANCH:
	p = "BRANCH";
	break;
    case EXACTLY:
	p = "EXACTLY";
	break;
    case NOTHING:
	p = "NOTHING";
	break;
    case BACK:
	p = "BACK";
	break;
    case END:
	p = "END";
	break;
    case ALNUM:
	p = "ALNUM";
	break;
    case NALNUM:
	p = "NALNUM";
	break;
    case BOUND:
	p = "BOUND";
	break;
    case NBOUND:
	p = "NBOUND";
	break;
    case SPACE:
	p = "SPACE";
	break;
    case NSPACE:
	p = "NSPACE";
	break;
    case DIGIT:
	p = "DIGIT";
	break;
    case NDIGIT:
	p = "NDIGIT";
	break;
    case CURLY:
	(void)sprintf(buf+strlen(buf), "CURLY {%d,%d}", ARG1(op),ARG2(op));
	p = NULL;
	break;
    case CURLYX:
	(void)sprintf(buf+strlen(buf), "CURLYX {%d,%d}", ARG1(op),ARG2(op));
	p = NULL;
	break;
    case REF:
	(void)sprintf(buf+strlen(buf), "REF%d", ARG1(op));
	p = NULL;
	break;
    case OPEN:
	(void)sprintf(buf+strlen(buf), "OPEN%d", ARG1(op));
	p = NULL;
	break;
    case CLOSE:
	(void)sprintf(buf+strlen(buf), "CLOSE%d", ARG1(op));
	p = NULL;
	break;
    case STAR:
	p = "STAR";
	break;
    case PLUS:
	p = "PLUS";
	break;
    case MINMOD:
	p = "MINMOD";
	break;
    case GBOL:
	p = "GBOL";
	break;
    case UNLESSM:
	p = "UNLESSM";
	break;
    case IFMATCH:
	p = "IFMATCH";
	break;
    case SUCCEED:
	p = "SUCCEED";
	break;
    case WHILEM:
	p = "WHILEM";
	break;
    default:

		TRACE0("\ncorrupted regexp opcode\n");
//	FAIL("corrupted regexp opcode");
    }
    if (p != NULL)
	(void) strcat(buf, p);
    return(buf);
}

#endif   _DEBUG
