//
//    bregexec.cc
//
////////////////////////////////////////////////////////////////////////////////
//  1999.11.24   update by Tatsuo Baba
//
//  You may distribute under the terms of either the GNU General Public
//  License or the Artistic License, as specified in the perl_license.txt file.
////////////////////////////////////////////////////////////////////////////////


/*
 * "One Ring to rule them all, One Ring to find them..."
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


//#include "config.h"





#include "global.h"
//#include "handy.h"  in perl.h
#include "sv.h"

#include "intreg.h"

#define ISKANJI(c) iskanji(c)


int matchanyof(unsigned int c,unsigned char *tbl);


static int regtry(regexp *prog, char *startpos,GLOBAL*);//own
static int regrepeat(char *p, int max,GLOBAL*);





/* return values of kpart */
#define KPART_KANJI_1 1 /* kanji 1st byte */
#define KPART_KANJI_2 2 /* kanji 2nd byte */
#define KPART_OTHER   0 /* other (ASCII) */

int   kpart(char *pLim,char *pChr);



#define DEFSTATIC

#ifndef STATIC
#define	STATIC	static
#endif

#ifdef	KANJI
#undef isascii
#define isascii(c) (0 < (c) && (c) < 0x7F)
#endif	/* KANJI */



typedef int CHECKPOINT;

static CHECKPOINT regcppush(int parenfloor,GLOBAL*);
char * regcppop (GLOBAL*);


#define regcpblow(cp) leave_scope(cp)

#define croak
/*
 * pregexec and friends
 */

/*
 * Forwards.
 */

/*
 - bregexec - match a regexp against a string
 */
BOOL bregexec(register regexp *prog,char *stringarg,
register char *strend,	/* pointer to null at end of string */
char *strbeg,	/* real beginning of string */
int minend,		/* end of match must be at least minend after stringarg */
int safebase,	/* no need to remember string in subbase */
char *msg)		/* fatal error message */
{
    register char *s;
    register int i;
    register char *c;
    register char *startpos = stringarg;
    register int tmp;

	msg[0] = '\0';		//ensure message clear

	if (startpos == strend)	// length 0  bye bye
		return 0;


	GLOBAL global;
	GLOBAL *globalp;
	globalp = &global;
	memset((void*)globalp,0,sizeof(GLOBAL));

	multiline = prog->pmflags & PMf_MULTILINE;
	kanji_mode_flag = prog->pmflags & PMf_KANJI;	// using KANJI_MODE macro
    int minlen = 0;		/* must match at least this many chars */
    int dontbother = 0;	/* how many characters not to try at end */
    CURCUR cc;

    cc.cur = 0;
    cc.oldcc = 0;
    regcc = &cc;


    /* Be paranoid... */
    if (prog == NULL || startpos == NULL) {
	//	croak("NULL parameter");
		return 0;
    }

	////////// exception started
	try {




    if (startpos == strbeg)	/* is ^ valid at stringarg? */
	regprev = '\n';
    else {
	regprev = stringarg[-1];
	if (!multiline && regprev == '\n')
	    regprev = '\0';		/* force ^ to NOT match */
    }
    regprecomp = prog->precomp;
    regnpar = prog->nparens;
    /* Check validity of program. */
    if (UCHARAT(prog->program) != MAGIC) {
		FAIL("corrupted regexp program");
    }

    
	if (prog->do_folding) {
	i = strend - startpos;
//	New(1101,c,i+1,char);
//	Copy(startpos, c, i+1, char);
	c = new char[i+1];
    if (c == NULL)
		croak("out of space copy string");
	memcpy(c,startpos,i+1);
	startpos = c;
	strend = startpos + i;
	for (s = startpos; s < strend; s++)
#ifdef	KANJI
	    if (KANJI_MODE && iskanji(*s) && s+1 < strend)
		s++;
	    else if (isUPPER(*s))
#else	  /* KANJI */
	    if (isUPPER(*s))
#endif	/* ! KANJI */
		*s = toLOWER(*s);
    }

    /* If there is a "must appear" string, look for it. */
    s = startpos;
    if (prog->regmust != NULL &&
	(!(prog->reganch & ROPT_ANCH)
	 || (multiline && prog->regback >= 0)) )
    {
//	if (stringarg == strbeg && screamer) {
//	    if (screamfirst[BmRARE(prog->regmust)] >= 0)
//		    s = screaminstr(screamer,prog->regmust);
//	    else
//		    s = NULL;
//	}
//	else
	    s = fbm_instr((unsigned char*)s, (unsigned char*)strend,
		prog->regmust,multiline,KANJI_MODE);
	if (!s) {
	    ++BmUSEFUL(prog->regmust);	/* hooray */
	    goto phooey;	/* not present */
	}
	else if (prog->regback >= 0) {
	    s -= prog->regback;
	    if (s < startpos)
		s = startpos;
	    minlen = prog->regback + SvCUR(prog->regmust);
	}
	else if (!prog->naughty && --BmUSEFUL(prog->regmust) < 0) { /* boo */
	    SvREFCNT_dec(prog->regmust);
	    prog->regmust = NULL;	/* disable regmust */
	    s = startpos;
	}
	else {
	    s = startpos;
	    minlen = SvCUR(prog->regmust);
	}
    }
#ifdef	KANJI
    if (s && KANJI_MODE && kpart(startpos, s) == KPART_KANJI_2) {
	s = startpos;
    }
#endif	/* KANJI */

    /* Mark beginning of line for ^ . */
    regbol = startpos;

    /* Mark end of line for $ (and such) */
    regeol = strend;

    /* see how far we have to get to not match where we matched before */
    regtill = startpos+minend;

    /* Simplest case:  anchored match need be tried only once. */
    /*  [unless multiline is set] */
    if (prog->reganch & ROPT_ANCH) {
	if (regtry(prog, startpos,globalp))
	    goto got_it;
	else if (multiline || (prog->reganch & ROPT_IMPLICIT)) {
	    if (minlen)
		dontbother = minlen - 1;
	    strend -= dontbother;
	    /* for multiline we only have to try after newlines */
	    if (s > startpos)
		s--;
	    while (s < strend) {
		if (*s++ == '\n') {
		    if (s < strend && regtry(prog, s,globalp))
			goto got_it;
		}
	    }
	}
	goto phooey;
    }

    /* Messy cases:  unanchored match. */
    if (prog->regstart) {
	if (prog->reganch & ROPT_SKIP) {  /* we have /x+whatever/ */
	    /* it must be a one character string */
	    i = SvPVX(prog->regstart)[0];
	    while (s < strend) {
		if (*s == i) {
		    if (regtry(prog, s,globalp))
			goto got_it;
#ifdef	KANJI
		    if (KANJI_MODE && iskanji(*s) && s+1<strend) s++; 
		    s++;
		    while (s < strend && *s == i) {
			if (KANJI_MODE && iskanji(*s) && s+1<strend) s++; 
			s++;
		    }
#else	/* KANJI */
		    s++;
		    while (s < strend && *s == i)
			s++;
#endif	/* ! KANJI */
		}
		s++;
	    }
	}
	else if (SvPOK(prog->regstart) == 3) {
	    /* We know what string it must start with. */
	    while ((s = fbm_instr((unsigned char*)s,
	      (unsigned char*)strend, prog->regstart,multiline,KANJI_MODE)) != NULL)
	    {
		if (regtry(prog, s,globalp))
		    goto got_it;
#ifdef	KANJI
		if (KANJI_MODE && iskanji(*s) && s+1<strend) s++; 
#endif
		s++;
	    }
	}
	else {
	    c = SvPVX(prog->regstart);
	    while ((s = ninstr(s, strend, c, c + SvCUR(prog->regstart),KANJI_MODE)) != NULL)
	    {
		if (regtry(prog, s,globalp))
		    goto got_it;
#ifdef	KANJI
		if (KANJI_MODE && iskanji(*s) && s+1<strend) s++; 
#endif
		s++;
	    }
	}
	goto phooey;
    }
    /*SUPPRESS 560*/
    if (c = prog->regstclass) {
	int doevery = (prog->reganch & ROPT_SKIP) == 0;

	if (minlen)
	    dontbother = minlen - 1;
	strend -= dontbother;	/* don't bother with what can't match */
	tmp = 1;
	/* We know what class it must start with. */
	switch (OP(c)) {
	case ANYOF:
	    c = OPERAND(c);
	    while (s < strend) {
		i = UCHARAT(s);
#ifdef	KANJI
		if (KANJI_MODE && iskanji(i) && s+1 < strend) {
		    s++;
		    i = (i << 8) + UCHARAT(s);
		    s--;
		}
		if (matchanyof(i, (unsigned char*)c)) {
#else	/* KANJI */
		if (!(c[i >> 3] & (1 << (i&7)))) {
#endif	/* !KANJI */
		    if (tmp && regtry(prog, s,globalp))
			goto got_it;
		    else
			tmp = doevery;
		}
		else
		    tmp = 1;
#ifdef	KANJI
		if (KANJI_MODE && iskanji(*s)) s++;
#endif
		s++;
	    }
	    break;
	case BOUND:
	    if (minlen)
		dontbother++,strend--;
	    if (s != startpos) {
		i = s[-1];
		tmp = isALNUM(i);
	    }
	    else
		tmp = isALNUM(regprev);	/* assume not alphanumeric */
	    while (s < strend) {
		i = *s;
		if (tmp != isALNUM(i)) {
		    tmp = !tmp;
		    if (regtry(prog, s,globalp))
			goto got_it;
		}
#ifdef	KANJI
		if (KANJI_MODE && iskanji(*s)) s++;
#endif
		s++;
	    }
	    if ((minlen || tmp) && regtry(prog,s,globalp))
		goto got_it;
	    break;
	case NBOUND:
	    if (minlen)
		dontbother++,strend--;
	    if (s != startpos) {
		i = s[-1];
		tmp = isALNUM(i);
	    }
	    else
		tmp = isALNUM(regprev);	/* assume not alphanumeric */
	    while (s < strend) {
		i = *s;
		if (tmp != isALNUM(i))
		    tmp = !tmp;
		else if (regtry(prog, s,globalp))
		    goto got_it;
#ifdef	KANJI
		if (KANJI_MODE && iskanji(*s)) s++;
#endif
		s++;
	    }
	    if ((minlen || !tmp) && regtry(prog,s,globalp))
		goto got_it;
	    break;
	case ALNUM:
	    while (s < strend) {
		i = *s;
		if (isALNUM(i)) {
		    if (tmp && regtry(prog, s,globalp))
			goto got_it;
		    else
			tmp = doevery;
		}
		else
		    tmp = 1;
#ifdef	KANJI
		if (KANJI_MODE && iskanji(*s)) s++;
#endif
		s++;
	    }
	    break;
	case NALNUM:
	    while (s < strend) {
		i = *s;
		if (!isALNUM(i)) {
		    if (tmp && regtry(prog, s,globalp))
			goto got_it;
		    else
			tmp = doevery;
		}
		else
		    tmp = 1;
#ifdef	KANJI
		if (KANJI_MODE && iskanji(*s)) s++;
#endif
		s++;
	    }
	    break;
	case SPACE:
	    while (s < strend) {
		if (isSPACE(*s)) {
		    if (tmp && regtry(prog, s,globalp))
			goto got_it;
		    else
			tmp = doevery;
		}
		else
		    tmp = 1;
#ifdef	KANJI
		if (KANJI_MODE && iskanji(*s)) s++;
#endif
		s++;
	    }
	    break;
	case NSPACE:
	    while (s < strend) {
		if (!isSPACE(*s)) {
		    if (tmp && regtry(prog, s,globalp))
			goto got_it;
		    else
			tmp = doevery;
		}
		else
		    tmp = 1;
#ifdef	KANJI
		if (KANJI_MODE && iskanji(*s)) s++;
#endif
		s++;
	    }
	    break;
	case DIGIT:
	    while (s < strend) {
		if (isDIGIT(*s)) {
		    if (tmp && regtry(prog, s,globalp))
			goto got_it;
		    else
			tmp = doevery;
		}
		else
		    tmp = 1;
#ifdef	KANJI
		if (KANJI_MODE && iskanji(*s)) s++;
#endif
		s++;
	    }
	    break;
	case NDIGIT:
	    while (s < strend) {
		if (!isDIGIT(*s)) {
		    if (tmp && regtry(prog, s,globalp))
			goto got_it;
		    else
			tmp = doevery;
		}
		else
		    tmp = 1;
#ifdef	KANJI
		if (KANJI_MODE && iskanji(*s)) s++;
#endif
		s++;
	    }
	    break;
	}
    }
    else {
	if (minlen)
	    dontbother = minlen - 1;
	strend -= dontbother;
	/* We don't know much -- general case. */
	do {
	    if (regtry(prog, s,globalp))
		goto got_it;
#ifdef	KANJI
	    if (KANJI_MODE && iskanji(*s)) s++;
#endif
	} while (s++ < strend);
    }

    /* Failure. */
    goto phooey;

got_it:
    strend += dontbother;	/* uncheat */
    prog->subbeg = strbeg;
    prog->subend = strend;
//    if ((!safebase && (prog->nparens || sawampersand)) || prog->do_folding) {
    if ((!safebase && (prog->nparens)) || prog->do_folding) {
	i = strend - startpos + (stringarg - strbeg);
	if (safebase) {			/* no need for $digit later */
	    s = strbeg;
	    prog->subend = s+i;
	}
	else if (strbeg != prog->subbase) {
	    s = savepvn(strbeg,i);	/* so $digit will work later */
	    if (prog->subbase)
//		Safefree(prog->subbase);
		delete [] prog->subbase;
	    prog->subbeg = prog->subbase = s;
	    prog->subend = s+i;
	}
	else {
	    prog->subbeg = s = prog->subbase;
	    prog->subend = s+i;
	}
	s += (stringarg - strbeg);
	for (i = 0; i <= prog->nparens; i++) {
	    if (prog->endp[i]) {
		prog->startp[i] = s + (prog->startp[i] - startpos);
		prog->endp[i] = s + (prog->endp[i] - startpos);
	    }
	}
	if (prog->do_folding)
//	    Safefree(startpos);
	    delete [] startpos;
    }
	if (savestack)
		delete [] savestack;
    return 1;

phooey:
    if (prog->do_folding)
//		Safefree(startpos);
		delete [] startpos;

	if (savestack)
		delete [] savestack;
	
	return 0;

}		// error handler start
	catch (const char* err){
		strcpy(msg,err);

		if (savestack)
			delete [] savestack;

		return 0;
	}
	catch (...){
		strcpy(msg,"fatal error");

		if (savestack)
			delete [] savestack;

		return 0;
	}




}
////////////////////////////////XXXXXXXXXXXXX
/*
 - regmatch - main matching routine
 *
 * Conceptually the strategy is simple:  check to see whether the current
 * node matches, call self recursively to see whether the rest matches,
 * and then act accordingly.  In practice we make some effort to avoid
 * recursion, in particular by going through "ordinary" nodes (that don't
 * need to know whether the rest of the match failed) by a loop instead of
 * by recursion.
 */
/* [lwall] I've hoisted the register declarations to the outer block in order to
 * maybe save a little bit of pushing and popping on the stack.  It also takes
 * advantage of machines that use a register save mask on subroutine entry.
 */
static  int			/* 0 failure, 1 success */
regmatch(char *prog,register GLOBAL* globalp)
{
    register char *scan;	/* Current node. */
    char *next;			/* Next node. */
    register int localNextchar;
    register int n;		/* no or next */
    register int ln;		/* len or last */
    register char *s;		/* operand or save */
    register char *locinput = reginput;
    int minmod = 0;

    localNextchar = *locinput;
    scan = prog;
    while (scan != NULL) {
#define sayYES return 1
#define sayNO return 0
#define saySAME(x) return x

#ifdef REGALIGN
	next = scan + NEXT(scan);
	if (next == scan)
	    next = NULL;
#else
	next = regnext(scan);
#endif

	switch (OP(scan)) {
	case BOL:
	    if (locinput == regbol
		? regprev == '\n'
		: ((localNextchar || locinput < regeol) && locinput[-1] == '\n') )
	    {
		/* regtill = regbol; */
		break;
	    }
	    sayNO;
	case MBOL:
	    if (locinput == regbol
		? regprev == '\n'
		: ((localNextchar || locinput < regeol) && locinput[-1] == '\n') )
	    {
		break;
	    }
	    sayNO;
	case SBOL:
	    if (locinput == regbol && regprev == '\n')
		break;
	    sayNO;
	case GBOL:
	    if (locinput == regbol)
		break;
	    sayNO;
	case EOL:
	    if (multiline)
		goto meol;
	    else
		goto seol;
	case MEOL:
	  meol:
	    if ((localNextchar || locinput < regeol) && localNextchar != '\n')
		sayNO;
	    break;
	case SEOL:
	  seol:
	    if ((localNextchar || locinput < regeol) && localNextchar != '\n')
		sayNO;
	    if (regeol - locinput > 1)
		sayNO;
	    break;
	case SANY:
	    if (!localNextchar && locinput >= regeol)
		sayNO;
#ifdef	KANJI
	    if (KANJI_MODE && iskanji(localNextchar) && locinput[1]) ++locinput;
#endif
	    localNextchar = *++locinput;
	    break;
	case ANY:
	    if (!localNextchar && locinput >= regeol || localNextchar == '\n')
		sayNO;
#ifdef	KANJI
	    if (KANJI_MODE && iskanji(localNextchar) && locinput[1]) ++locinput;
#endif
	    localNextchar = *++locinput;
	    break;
	case EXACTLY:
	    s = OPERAND(scan);
	    ln = *s++;
	    /* Inline the first character, for speed. */
	    if (*s != localNextchar)
		sayNO;
	    if (regeol - locinput < ln)
		sayNO;
	    if (ln > 1 && memcmp(s, locinput, ln) != 0)
		sayNO;
	    locinput += ln;
	    localNextchar = *locinput;
	    break;
	case ANYOF:
	    s = OPERAND(scan);
	    if (localNextchar < 0)
		localNextchar = UCHARAT(locinput);
#ifdef	KANJI
	    if (KANJI_MODE && iskanji(localNextchar) && locinput[1]) {
		locinput++;
		localNextchar = (localNextchar << 8) + UCHARAT(locinput);
	    }
	    if (!matchanyof(localNextchar,(unsigned char*) s))
#else	/* KANJI */
	    if (s[localNextchar >> 3] & (1 << (localNextchar&7)))
#endif	/* ! KANJI */
		sayNO;
	    if (!localNextchar && locinput >= regeol)
		sayNO;
	    localNextchar = *++locinput;
	    break;
	case ALNUM:
	    if (!localNextchar)
		sayNO;
	    if (!isALNUM(localNextchar))
		sayNO;
	    localNextchar = *++locinput;
	    break;
	case NALNUM:
	    if (!localNextchar && locinput >= regeol)
		sayNO;
	    if (isALNUM(localNextchar))
		sayNO;
#ifdef	KANJI
	    if (KANJI_MODE && iskanji(localNextchar) && locinput[1])
	      ++locinput;
#endif
	    localNextchar = *++locinput;
	    break;
	case NBOUND:
	case BOUND:
	    if (locinput == regbol)	/* was last char in word? */
		ln = isALNUM(regprev);
	    else 
		ln = isALNUM(locinput[-1]);
	    n = isALNUM(localNextchar); /* is next char in word? */
	    if ((ln == n) == (OP(scan) == BOUND))
		sayNO;
	    break;
	case SPACE:
	    if (!localNextchar && locinput >= regeol)
		sayNO;
	    if (!isSPACE(localNextchar))
		sayNO;
	    localNextchar = *++locinput;
	    break;
	case NSPACE:
	    if (!localNextchar)
		sayNO;
	    if (isSPACE(localNextchar))
		sayNO;
#ifdef	KANJI
	    if (KANJI_MODE && iskanji(localNextchar) && locinput[1])
	      ++locinput;
#endif
	    localNextchar = *++locinput;
	    break;
	case DIGIT:
	    if (!isDIGIT(localNextchar))
		sayNO;
	    localNextchar = *++locinput;
	    break;
	case NDIGIT:
	    if (!localNextchar && locinput >= regeol)
		sayNO;
	    if (isDIGIT(localNextchar))
		sayNO;
#ifdef	KANJI
	    if (KANJI_MODE && iskanji(localNextchar) && locinput[1])
	      ++locinput;
#endif
	    localNextchar = *++locinput;
	    break;
	case REF:
	    n = ARG1(scan);  /* which paren pair */
	    s = regstartp[n];
	    if (!s)
		sayNO;
	    if (!regendp[n])
		sayNO;
	    if (s == regendp[n])
		break;
	    /* Inline the first character, for speed. */
	    if (*s != localNextchar)
		sayNO;
	    ln = regendp[n] - s;
	    if (locinput + ln > regeol)
		sayNO;
	    if (ln > 1 && memcmp(s, locinput, ln) != 0)
		sayNO;
	    locinput += ln;
	    localNextchar = *locinput;
	    break;

	case NOTHING:
	    break;
	case BACK:
	    break;
	case OPEN:
	    n = ARG1(scan);  /* which paren pair */
	    regstartp[n] = locinput;
	    if (n > regsize)
		regsize = n;
	    break;
	case CLOSE:
	    n = ARG1(scan);  /* which paren pair */
	    regendp[n] = locinput;
	    if (n > *reglastparen)
		*reglastparen = n;
	    break;
	case CURLYX: {
		CURCUR cc;
		CHECKPOINT cp = savestack_ix;
		cc.oldcc = regcc;
		regcc = &cc;
		cc.parenfloor = *reglastparen;
		cc.cur = -1;
		cc.min = ARG1(scan);
		cc.max  = ARG2(scan);
		cc.scan = NEXTOPER(scan) + 4;
		cc.next = next;
		cc.minmod = minmod;
		cc.lastloc = 0;
		reginput = locinput;
		n = regmatch(PREVOPER(next),globalp);	/* start on the WHILEM */
//		regcpblow(cp);
		regcc = cc.oldcc;
		saySAME(n);
	    }
	    /* NOT REACHED */
	case WHILEM: {
		/*
		 * This is really hard to understand, because after we match
		 * what we're trying to match, we must make sure the rest of
		 * the RE is going to match for sure, and to do that we have
		 * to go back UP the parse tree by recursing ever deeper.  And
		 * if it fails, we have to reset our parent's current state
		 * that we can try again after backing off.
		 */

		CURCUR* cc = regcc;
		n = cc->cur + 1;	/* how many we know we matched */
		reginput = locinput;


		/* If degenerate scan matches "", assume scan done. */

		if (locinput == cc->lastloc) {
		    regcc = cc->oldcc;
		    ln = regcc->cur;
		    if (regmatch(cc->next,globalp))
			sayYES;
		    regcc->cur = ln;
		    regcc = cc;
		    sayNO;
		}

		/* First just match a string of min scans. */

		if (n < cc->min) {
		    cc->cur = n;
		    cc->lastloc = locinput;
		    if (regmatch(cc->scan,globalp))
			sayYES;
		    cc->cur = n - 1;
		    sayNO;
		}

		/* Prefer next over scan for minimal matching. */

		if (cc->minmod) {
		    regcc = cc->oldcc;
		    ln = regcc->cur;
		    if (regmatch(cc->next,globalp))
			sayYES;	/* All done. */
		    regcc->cur = ln;
		    regcc = cc;

		    if (n >= cc->max)	/* Maximum greed exceeded? */
			sayNO;

		    /* Try scanning more and see if it helps. */
		    reginput = locinput;
		    cc->cur = n;
		    cc->lastloc = locinput;
		    if (regmatch(cc->scan,globalp))
			sayYES;
		    cc->cur = n - 1;
		    sayNO;
		}

		/* Prefer scan over next for maximal matching. */

		if (n < cc->max) {	/* More greed allowed? */
		    regcppush(cc->parenfloor,globalp);
		    cc->cur = n;
		    cc->lastloc = locinput;
		    if (regmatch(cc->scan,globalp))
			sayYES;
		    regcppop(globalp);		/* Restore some previous $<digit>s? */
		    reginput = locinput;
		}

		/* Failed deeper matches of scan, so see if this one works. */
		regcc = cc->oldcc;
		ln = regcc->cur;
		if (regmatch(cc->next,globalp))
		    sayYES;
		regcc->cur = ln;
		regcc = cc;
		cc->cur = n - 1;
		sayNO;
	    }
	    /* NOT REACHED */
	case BRANCH: {
		if (OP(next) != BRANCH)	  /* No choice. */
		    next = NEXTOPER(scan);/* Avoid recursion. */
		else {
		    int lastparen = *reglastparen;
		    do {
			reginput = locinput;
			if (regmatch(NEXTOPER(scan),globalp))
			    sayYES;
			for (n = *reglastparen; n > lastparen; n--)
			    regendp[n] = 0;
			*reglastparen = n;
			    
#ifdef REGALIGN
			/*SUPPRESS 560*/
			if (n = NEXT(scan))
			    scan += n;
			else
			    scan = NULL;
#else
			scan = regnext(scan);
#endif
		    } while (scan != NULL && OP(scan) == BRANCH);
		    sayNO;
		    /* NOTREACHED */
		}
	    }
	    break;
	case MINMOD:
	    minmod = 1;
	    break;
	case CURLY:
	    ln = ARG1(scan);  /* min to match */
	    n  = ARG2(scan);  /* max to match */
	    scan = NEXTOPER(scan) + 4;
	    goto repeat;
	case STAR:
	    ln = 0;
	    n = 32767;
	    scan = NEXTOPER(scan);
	    goto repeat;
	case PLUS:
	    /*
	    * Lookahead to avoid useless match attempts
	    * when we know what character comes next.
	    */
	    ln = 1;
	    n = 32767;
	    scan = NEXTOPER(scan);
	  repeat:
	    if (OP(next) == EXACTLY)
		localNextchar = *(OPERAND(next)+1);
	    else
		localNextchar = -1000;
	    reginput = locinput;
	    if (minmod) {
		minmod = 0;
		if (ln && regrepeat(scan, ln,globalp) < ln)
		    sayNO;
#ifdef	KANJI
		while (n >= ln || (n == 32767 && ln > 0)) { /* ln overflow ? */
		    if (!KANJI_MODE
			|| kpart(locinput, reginput) != KPART_KANJI_2) {
#else	/* KANJI */
		while (n >= ln || (n == 32767 && ln > 0)) { /* ln overflow ? */
#endif	/* ! KANJI */
		    /* If it could work, try it. */
		    if (localNextchar == -1000 || *reginput == localNextchar)
			if (regmatch(next,globalp))
			    sayYES;
#ifdef	KANJI
		    }	// KANJI_MODE check
#endif
		    /* Couldn't or didn't -- back up. */
		    reginput = locinput + ln;
		    if (regrepeat(scan, 1,globalp)) {
			ln++;
			reginput = locinput + ln;
		    }
		    else
			sayNO;
		}
	    }
	    else {
		n = regrepeat(scan, n,globalp);
		if (ln < n && regkind[(unsigned char)OP(next)] == EOL &&
		    (!multiline || OP(next) == SEOL))
		    ln = n;			/* why back off? */
		while (n >= ln) {
#ifdef	KANJI
		    if (!KANJI_MODE
			|| kpart(locinput, reginput) != KPART_KANJI_2) {
#endif
		    /* If it could work, try it. */
		    if (localNextchar == -1000 || *reginput == localNextchar)
			if (regmatch(next,globalp))
			    sayYES;
#ifdef	KANJI
		    }
#endif
		    /* Couldn't or didn't -- back up. */
		    n--;
		    reginput = locinput + n;
		}
	    }
	    sayNO;
	case SUCCEED:
	case END:
	    reginput = locinput;	/* put where regtry can find it */
	    sayYES;			/* Success! */
	case IFMATCH:
	    reginput = locinput;
	    scan = NEXTOPER(scan);
	    if (!regmatch(scan,globalp))
		sayNO;
	    break;
	case UNLESSM:
	    reginput = locinput;
	    scan = NEXTOPER(scan);
	    if (regmatch(scan,globalp))
		sayNO;
	    break;
	default:
//	    PerlIO_printf(PerlIO_stderr(), "%x %d\n",(unsigned)scan,scan[1]);
	    FAIL("regexp memory corruption");
	}
	scan = next;
    }

    /*
    * We get here only if there's trouble -- normally "case END" is
    * the terminating point.
    */
    FAIL("corrupted regexp pointers");
    /*NOTREACHED*/
    sayNO;

yes:
    return 1;

no:
    return 0;
}

/*
 - regrepeat - repeatedly match something simple, report how many
 */
/*
 * [This routine now assumes that it will only match on things of length 1.
 * That was true before, but now we assume scan - reginput is the count,
 * rather than incrementing count on every character.]
 */
static int regrepeat(char *p, int max,GLOBAL* globalp)
{
    register char *scan;
    register char *opnd;
    register int c;
    register char *loceol = regeol;
#ifdef	KANJI
    char *oldscan;
#endif

    scan = reginput;
    if (max != 32767 && max < loceol - scan)
      loceol = scan + max;
    opnd = OPERAND(p);
    switch (OP(p)) {
    case ANY:
#ifdef	KANJI
	while (scan < loceol && *scan != '\n') {
	    if (KANJI_MODE && iskanji(*scan)) scan++;
	    scan++;
	}
#else
	while (scan < loceol && *scan != '\n')
	    scan++;
#endif
	break;
    case SANY:
	scan = loceol;
	break;
    case EXACTLY:		/* length of string is 1 */
	opnd++;
	while (scan < loceol && *opnd == *scan)
	    scan++;
	break;
    case ANYOF:
	c = UCHARAT(scan);
#ifdef	KANJI
	oldscan = scan;
	if (KANJI_MODE && iskanji(c) && scan < loceol) {
	    scan++;
	    c = (c << 8) + UCHARAT(scan);
	}
	while (scan < loceol && matchanyof(c, (unsigned char*)opnd)) {
	    scan++;
	    c = UCHARAT(scan);
	    oldscan = scan;
	    if (KANJI_MODE && iskanji(c) && scan < loceol) {
		scan++;
		c = (c << 8) + UCHARAT(scan);
	    }
	}
	scan = oldscan;
#else
	while (scan < loceol && !(opnd[c >> 3] & (1 << (c & 7)))) {
	    scan++;
	    c = UCHARAT(scan);
	}
#endif
	break;
    case ALNUM:
	while (scan < loceol && isALNUM(*scan))
	    scan++;
	break;
    case NALNUM:
#ifdef	KANJI
	while (scan < loceol && !isALNUM(*scan)) {
	    if (KANJI_MODE && iskanji(*scan)) scan++;
	    scan++;
	}
#else
	while (scan < loceol && !isALNUM(*scan))
	    scan++;
#endif
	break;
    case SPACE:
	while (scan < loceol && isSPACE(*scan))
	    scan++;
	break;
    case NSPACE:
#ifdef	KANJI
	while (scan < loceol && !isSPACE(*scan)) {
	    if (KANJI_MODE && iskanji(*scan)) scan++;
	    scan++;
	}
#else
	while (scan < loceol && !isSPACE(*scan))
	    scan++;
#endif
	break;
    case DIGIT:
	while (scan < loceol && isDIGIT(*scan))
	    scan++;
	break;
    case NDIGIT:
#ifdef	KANJI
	while (scan < loceol && !isDIGIT(*scan)) {
	    if (KANJI_MODE && iskanji(*scan)) scan++;
	    scan++;
	}
#else
	while (scan < loceol && !isDIGIT(*scan))
	    scan++;
#endif
	break;
    default:		/* Called on something of 0 width. */
	break;		/* So match right here or not at all. */
    }

    c = scan - reginput;
    reginput = scan;

    return(c);
}
//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#ifdef	KANJI
int 
matchanyof(unsigned int c,unsigned char *tbl)
{

    int f = (tbl[0] == ANYOF_COMPL);
    unsigned int from, to;
    int		ret = 0 ;

    tbl++;
    while (*tbl != ANYOF_ENDMARK) {
	switch (*tbl) {
	  case ANYOF_EXACT:
	    from = (tbl[1] << 8) + tbl[2];
	    tbl += 2;
	    if (c == from) {
	      ret = !f;
	      goto L_return ;
	    }
	    break;
	  case ANYOF_FROMTO:
	    from = (tbl[1] << 8) + tbl[2];
	    to   = (tbl[3] << 8) + tbl[4];
	    tbl += 4;
	    if ((from <= c) && (c <= to)) {
	      ret = !f;
	      goto L_return ;
	    }
	    break;
	  default:
	    /* fatal("internal error CCL"); koyama */
	    croak("oh! internal error");
	    /* my_exit(1); */
//	    return 0;
	}
	tbl++;
    }
    ret = f;
L_return:
    return ret ;
}
#endif	/* KANJI */


//////////////////////////////////////////////////////////
// same original regexec.cpp source.
/*
 - regtry - try match at specific point
 */
static int			/* 0 failure, 1 success */
regtry(regexp *prog, char *startpos,GLOBAL* globalp)
{
    register int i;
    register char **sp;
    register char **ep;

    reginput = startpos;
    regstartp = prog->startp;
    regendp = prog->endp;
    reglastparen = &prog->lastparen;
    prog->lastparen = 0;
    regsize = 0;

    sp = prog->startp;
    ep = prog->endp;
    if (prog->nparens) {
	for (i = prog->nparens; i >= 0; i--) {
	    *sp++ = NULL;
	    *ep++ = NULL;
	}
    }
    if (regmatch(prog->program + 1,globalp) && reginput >= regtill) {
	prog->startp[0] = startpos;
	prog->endp[0] = reginput;
	return 1;
    }
    else
	return 0;
}

static void savestack_grow(GLOBAL* globalp);

static CHECKPOINT regcppush(int parenfloor,GLOBAL* globalp)
{
    int retval = savestack_ix;
    int i = (regsize - parenfloor) * 3;
    int p;


//    SSCHECK(i + 5);
    if ((savestack_ix + i + 5) > savestack_max)
		savestack_grow(globalp);

    for (p = regsize; p > parenfloor; p--) {
	SSPUSHPTR(regendp[p]);
	SSPUSHPTR(regstartp[p]);
	SSPUSHINT(p);
    }
    SSPUSHINT(regsize);
    SSPUSHINT(*reglastparen);
    SSPUSHPTR(reginput);
    SSPUSHINT(i + 3);
    SSPUSHINT(SAVEt_REGCONTEXT);
    return retval;
}


char*
regcppop(GLOBAL* globalp)
{
    int i = SSPOPINT;
    int paren = 0;
    char *input;
    char *tmps;
//    ASSERT(i == SAVEt_REGCONTEXT);
    i = SSPOPINT;
    input = (char *) SSPOPPTR;
    *reglastparen = SSPOPINT;
    regsize = SSPOPINT;
    for (i -= 3; i > 0; i -= 3) {
	paren = (int)SSPOPINT;
	regstartp[paren] = (char *) SSPOPPTR;
	tmps = (char*)SSPOPPTR;
	if (paren <= *reglastparen)
	    regendp[paren] = tmps;
    }
    for (paren = *reglastparen + 1; paren <= regnpar; paren++) {
	if (paren > regsize)
	    regstartp[paren] = NULL;
	regendp[paren] = NULL;
    }
    return input;
}



static void savestack_grow(register GLOBAL* globalp)
{
    int oldlen = savestack_max;
    savestack_max = savestack_max * 3 / 2;
	if (!savestack_max)
		savestack_max = 128;

 //   Renew(savestack, savestack_max, ANY);
    char *buf = new char[savestack_max * sizeof(char*)];
    if (buf == NULL)
		croak("out of space savestack");

	if (savestack) {
		memcpy(buf,savestack,oldlen * sizeof(char*));
		delete [] savestack;
	}
	savestack = (union any*)buf;
}
