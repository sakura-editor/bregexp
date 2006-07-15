//
// main.cc
//  
//   main front-end
////////////////////////////////////////////////////////////////////////////////
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

#include "sv.h"
#include "bregexp.h"

#include "global.h"
#include "intreg.h"

extern "C"
char* BRegexpVersion(void)
{
	//	2003.11.01 Karoto : Change version info
	//	2006.07.14 genta : Change version info
	static char version[] = "Bregexp.dll V1.02 for SAKURA "
__DATE__;

	return version;
}

/*
	BMatchEx : enhanved version of BMatch
	           to specify start point of the target string

	2003.11.01 Karoto : Add "targetbegp" parameter to BMatch, and change API name
*/
extern "C"
int BMatchEx(char* str, char *targetbegp, char *target,char *targetendp,
			BREGEXP **charrxp,char *msg)
{

	if (msg == NULL)		// no message area 
		return -1;
	msg[0] = '\0';				// ensure no error
	regexp **rxp = (regexp**)charrxp;
	if (rxp == NULL) {
		strcpy(msg,"invalid BREGEXP parameter");
		return -1;
	}
	regexp *rx = *rxp;			// same string before ?
	int plen;

	//	2003.11.01 Karoto : Add parameter check for targetbegp
	if (targetbegp == NULL || targetbegp > target 
		|| target == NULL || targetendp == NULL
		|| target >= targetendp) { // bad targer parameter ?
		strcpy(msg,"invalid target parameter");
		return -1;
	}
	if (str == NULL || str[0] == '\0') { // null string
		if (rx)				// assume same string
			goto readytogo;
		strcpy(msg,"invalid reg parameter");
		return -1;
	}
	
	plen = strlen(str);
	if (rx && !(rx->pmflags & PMf_TRANSLATE) &&
		rx->paraendp - rx->parap == plen && memcmp(str,rx->parap,plen) == 0) 
		goto readytogo;
	
	if (rx)			// delete old regexp block
		BRegfree((BREGEXP*)rx);
	rx = compile(str,plen,msg);
	if (rx == NULL)
		return -1;

	*rxp = rx;					// set new regexp

readytogo:

	if (rx->pmflags & PMf_SUBSTITUTE) {
		BRegfree((BREGEXP*)rx);
		*rxp = NULL;
		strcpy(msg,"call BSubst");
		return -1;
	}

	if (rx->outp) {				// free old result string
		delete [] rx->outp;
		rx->outp = NULL;
	}

	if (rx->prelen == 0) {			// no string
		return 0;
	}

	//	2003.11.01 Karoto
	BOOL matched = bregexec(rx, target, targetendp, targetbegp , 0, 1, msg);
	if (matched && rx->nparens && rx->endp[1] > rx->startp[1]) {
		int len = rx->endp[1] - rx->startp[1];
		char *tp = new char[len+1];
		if (tp == NULL) {
			strcpy(msg,"match out of space");
			return -1;
		}
		memcpy(tp,rx->startp[1],len);
		rx->outp = tp;
		rx->outendp = tp + len;
		*(rx->outendp) = '\0';
	}
	return msg[0] == '\0' ? matched: -1 ;
}


/*
	BMatch

	2003.11.01 Karoto : call BMatchEx() with specialized parameter
*/
extern "C"
int BMatch(char* str,char *target,char *targetendp,
			BREGEXP **charrxp,char *msg)
{
	return BMatchEx(str, target, target, targetendp, charrxp, msg);
}


/*
	BSubstEx : enhanved version of BSubst
	           to specify start point of the target string

	2003.11.01 Karoto : Add "targetbegp" parameter to BSubst, and change API name
*/
extern "C"
int BSubstEx(char* str,char *targetbegp, char *target,char *targetendp,
				BREGEXP **charrxp,char *msg)
{

	if (msg == NULL)		// no message area 
		return -1;
	msg[0] = '\0';				// ensure no error
	regexp **rxp = (regexp**)charrxp;
	if (rxp == NULL) {
		strcpy(msg,"invalid BREGEXP parameter");
		return -1;
	}
	regexp *rx = *rxp;			// same string before ?
	int plen;

	//	2003.11.01 Karoto : Add parameter check for targetbegp
	if (targetbegp == NULL || targetbegp > target 
		|| target == NULL || targetendp == NULL
		|| target >= targetendp) { // bad targer parameter ?
		strcpy(msg,"invalid target parameter");
		return -1;
	}
	if (str == NULL || str[0] == '\0') { // null string
		if (rx)				// assume same string
			goto readytogo;
		strcpy(msg,"invalid reg parameter");
		return -1;
	}
	plen = strlen(str);
	if (rx && !(rx->pmflags & PMf_TRANSLATE) &&
		rx->paraendp - rx->parap == plen && memcmp(str,rx->parap,plen) == 0) 
		goto readytogo;
	
	if (rx)						// delete old regexp block
		BRegfree((BREGEXP*)rx);
	rx = compile(str,plen,msg);
	if (rx == NULL)
		return -1;
	*rxp = rx;					// set new regexp

readytogo:

	if (rx->outp) {				// free old result string
		delete [] rx->outp;
		rx->outp = NULL;
	}

	if (rx->prelen == 0) {			// no string
		return -1;
	}


	if (rx->pmflags & PMf_SUBSTITUTE) {
		return subst(rx,target,targetendp,targetbegp,msg);
	}
	//	2003.11.01 Karoto
	BOOL matched = bregexec(rx, target, targetendp, targetbegp, 0, 1, msg);
	if (matched && rx->nparens && rx->endp[1] > rx->startp[1]) {
		int len = rx->endp[1] - rx->startp[1];
		char *tp = new char[len+1];
		if (tp == NULL) {
			strcpy(msg,"match out of space");
			return -1;
		}
		memcpy(tp,rx->startp[1],len);
		rx->outp = tp;
		rx->outendp = tp + len;
		*(rx->outendp) = '\0';
	}
	return msg[0] == '\0' ? matched: -1 ;
}

/*
	BSubst

	2003.11.01 Karoto : call BSubstEx() with specialized parameter
*/
extern "C"
int BSubst(char* str,char *target,char *targetendp,
				BREGEXP **charrxp,char *msg)
{
	return BSubstEx(str, target, target, targetendp, charrxp, msg);
}

extern "C"
int BTrans(char* str,char *target,char *targetendp,
				BREGEXP **charrxp,char *msg)
{

	if (msg == NULL)		// no message area 
		return -1;
	msg[0] = '\0';				// ensure no error
	regexp **rxp = (regexp**)charrxp;
	if (rxp == NULL) {
		strcpy(msg,"invalid BREGEXP parameter");
		return -1;
	}
	regexp *rx = *rxp;			// same string before ?
	int plen;

	if (target == NULL || targetendp == NULL
		|| target >= targetendp) { // bad targer parameter ?
		strcpy(msg,"invalid target parameter");
		return -1;
	}
	if (str == NULL || str[0] == '\0') { // null string
		if (rx)				// assume same string
			goto readytogo;
		strcpy(msg,"invalid reg parameter");
		return -1;
	}
	plen = strlen(str);
	if (rx && rx->pmflags & PMf_TRANSLATE && 
		rx->paraendp - rx->parap == plen && memcmp(str,rx->parap,plen) == 0) 
		goto readytogo;
	
	if (rx)						// delete old regexp block
		BRegfree((BREGEXP*)rx);
	rx = compile(str,plen,msg);
	if (rx == NULL)
		return -1;
	*rxp = rx;					// set new regexp

readytogo:

	if (rx->outp) {				// free old result string
		delete [] rx->outp;
		rx->outp = NULL;
	}

	if (!(rx->pmflags & PMf_TRANSLATE)) { 
		BRegfree((BREGEXP*)rx);
		*rxp = NULL;	
		strcpy(msg,"no translate parameter");
		return -1;
	}


	int matched = trans(rx,target,targetendp,msg);

	return msg[0] == '\0' ? matched: -1 ;
}


extern "C"
int BSplit(char* str,char *target,char *targetendp,
		int limit,BREGEXP **charrxp,char *msg)
{

	if (msg == NULL)		// no message area 
		return -1;
	msg[0] = '\0';				// ensure no error
	regexp **rxp = (regexp**)charrxp;
	if (rxp == NULL) {
		strcpy(msg,"invalid BREGEXP parameter");
		return -1;
	}
	regexp *rx = *rxp;			// same string before ?
	int plen;

	if (target == NULL || targetendp == NULL
		|| target >= targetendp) { // bad targer parameter ?
		strcpy(msg,"invalid target parameter");
		return -1;
	}
	if (str == NULL || str[0] == '\0') { // null string
		if (rx)				// assume same string
			goto readytogo;
		strcpy(msg,"invalid reg parameter");
		return -1;
	}
	plen = strlen(str);
	if (rx &&  !(rx->pmflags & PMf_TRANSLATE) &&
		rx->paraendp - rx->parap == plen && memcmp(str,rx->parap,plen) == 0) 
		goto readytogo;
	
	if (rx)						// delete old regexp block
		BRegfree((BREGEXP*)rx);
	rx = compile(str,plen,msg);
	if (rx == NULL)
		return -1;
	*rxp = rx;					// set new regexp

readytogo:

	if (rx->splitp) {
		delete [] rx->splitp;
		rx->splitp = NULL;
	}

	int ctr = split(rx,target,targetendp,limit,msg);
	return msg[0] == '\0' ? ctr: -1 ;

}




extern "C"
void BRegfree(BREGEXP *reg)
{
	regexp *r = (regexp*)reg;
    if (!r)
		return;
    if (r->parap) {
		delete [] r->parap;
		r->parap = NULL;
    }
    if (r->outp) {
		delete [] r->outp;
		r->outp = NULL;
    }
    if (r->splitp) {
		delete [] r->splitp;
		r->splitp = NULL;
    }
    if (r->precomp) {
		delete [] r->precomp;
		r->precomp = NULL;
    }
    if (r->subbase) {
		delete [] r->subbase;
		r->subbase = NULL;
    }
    if (r->regmust) {
		sv_free(r->regmust);
		r->regmust = NULL;
    }
    if (r->regstart) {
		sv_free(r->regstart);
		r->regstart = NULL;
    }
	if (r->repstr) {
		delete []r->repstr->startp;
		delete []r->repstr->dlen;
		delete r->repstr;
	}
	if (r->transtblp) {
		delete []r->transtblp;
	}
    
	
	delete [] r->startp;
    delete [] r->endp;
    delete [] r;
}
