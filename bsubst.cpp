//
//   bsubst.cc
//  
////////////////////////////////////////////////////////////////////////////////
//  1999.11.24   update by Tatsuo Baba
//
//  You may distribute under the terms of either the GNU General Public
//  License or the Artistic License, as specified in the perl_license.txt file.
////////////////////////////////////////////////////////////////////////////////


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define KANJI
#include "global.h"
#include "sv.h"
#include "intreg.h"

static REPSTR *cvrepstr(regexp* rx,char *str,char* strend);

regexp *trcomp(char* res,char* resend,char* rp,char* rpend,
			   int* flag,char *msg);



regexp *compile(const char* str,int plen,char *msg)
{
	int flag = 0;
	char type;
	char *p,*pend,sep,*res,*resend,*rp,*rpend,prev;
	char *parap;

	parap = new char[plen+1];	// parameter copy
	if (parap == NULL) {
		strcpy(msg,"precompile out of space");
		return NULL;
	}
	memcpy(parap,str,plen+1);	// copy include null

	type = 'm';		// default is match
	p = parap;
	pend = p + plen;
	sep = '/';		// default separater
	if (*p != '/') {
		if (*p != 's' && *p != 'm' && memcmp(p,"tr",2) != 0) {
			strcpy(msg,"do not start 'm' or 's' or 'tr'");
			delete [] parap;
			return NULL;
		}
		if (*p == 's')
			type = 's';		// substitute command
		else if (*p == 't' ) {
			type = 't';		// translate command
			p++;
		}
		sep = *++p;
	}
	p++;
	res = p;
	resend = rp = rpend = 0;
	prev = 0;
	while (*p != '\0') {
		if (iskanji(*p)) {
			prev = 0; p = p + 2;
			continue;
		}
		if (*p == '\\' && prev == '\\'){
			prev = 0; p++;
			continue;
		}
		if (*p == '/' && prev == '\\') {	// \/ means /
			p++;prev = 0;
			continue;
		}
		if (*p == sep && prev != '\\' && 
					!resend) {
			resend = p;
			p++;
			rp = p; 
			continue;
		}
		if (*p == sep && prev != '\\') {
			rpend = p;
			p++;
			break;
		}
		prev = *p++;
	}
	if (!resend || (!rpend && type != 'm')){
		strcpy(msg,"unmatch separater");
		delete [] parap;
		return NULL;
	}
	
	if (!rpend)
		p = resend + 1;

	while (*p != '\0') {
		if (*p == 'g')
			flag |= PMf_GLOBAL;
		if (*p == 'i')
			flag |= PMf_FOLD;
		if (*p == 'm')
			flag |= PMf_MULTILINE;
		if (*p == 'o')
			flag |= PMf_KEEP;
		if (*p == 'k')
			flag |= PMf_KANJI;
		if (*p == 'c')
			flag |= PMf_TRANS_COMPLEMENT;
		if (*p == 'd')
			flag |= PMf_TRANS_DELETE;
		if (*p == 's')
			flag |= PMf_TRANS_SQUASH;
		p++;
	}

	switch (type) {
	case 'm':
	case 's':

		regexp *rx = bregcomp(res,resend,&flag,msg);
		if (rx == NULL) {
			delete [] parap;
			return NULL;
		}

		if (type == 's')
			rx->pmflags |=PMf_SUBSTITUTE;

		rx->parap = parap;
		rx->paraendp = rx->parap + plen;

		if (rx->pmflags & PMf_SUBSTITUTE) {		// substitute ?
			rx->repstr = cvrepstr(rx,rp,rpend);	// compile replace string
			rx->prerepp = rp;
			rx->prerependp = rpend;
		}
		return rx;
	}
	// translate 
	regexp *rx = trcomp(res,resend,rp,rpend,&flag,msg);
	if (rx == NULL) {
		delete [] parap;
		return NULL;
	}
	rx->parap = parap;
	rx->paraendp = rx->parap + plen;
	rx->pmflags |=PMf_TRANSLATE;
	return rx;
}

	
int subst(regexp* rx,char *target,char *targetendp,char *targetbegp,char *msg)
{
	char *orig,*m,*c;
	char *s = target;
	int len = targetendp - target;
    char *strend = s + len;
    int maxiters = (strend - s) + 10;
	int iters = 0;
	int clen;
    orig = targetbegp;
	m = s;
    int once = !(rx->pmflags & PMf_GLOBAL);
	c = rx->prerepp;
	clen = rx->prerependp - c;
	// 
    if (!bregexec(rx, s, strend, orig, 0,1,msg))
		return 0;
	int blen = len + clen + 256;
	char *buf = new char[blen];
	int copycnt = 0;
	if (buf == NULL) {
		strcpy(msg,"out of space buf");
		return 0;
	}
	// now ready to go
	int subst_count = 0;
	do {
		if (iters++ > maxiters) {
			delete [] buf;
			strcpy(msg,"Substitution loop");
			return 0;
		}
		m = rx->startp[0];
		len = m - s;
		if (blen <= copycnt + len) {
			char *tp = new char[blen + len + 256];
			if (tp == NULL) {
				strcpy(msg,"out of space buf");
				delete [] buf;
				return 0;
			}
			memcpy(tp,buf,copycnt);
			delete [] buf;
			buf = tp; blen += len + 256;
		}
		if (len ) {
			memcpy(buf+copycnt, s, len);
			copycnt += len;
		}
		s = rx->endp[0];
		if (!(rx->pmflags & PMf_CONST)) {	// we have \digits or $& 
			//	ok start magic
			REPSTR *rep = rx->repstr;
//			ASSERT(rep);
			for (int i = 0;i < rep->count;i++) {
				int dlen = rep->dlen[i]; 
				if (rep->startp[i] && dlen) { 
					if (blen <= copycnt + dlen) {
						char *tp = new char[blen + dlen + 256];
						if (tp == NULL) {
							strcpy(msg,"out of space buf");
							delete [] buf;
							return 0;
						}
						memcpy(tp,buf,copycnt);
						delete [] buf;
						buf = tp; blen += dlen + 256;
					}
					memcpy(buf+copycnt, rep->startp[i], dlen);
					copycnt += dlen;
				}

				else if (dlen <= rx->nparens && rx->startp[dlen]) {
					len = rx->endp[dlen] - rx->startp[dlen];
					if (blen <= copycnt + len) {
						char *tp = new char[blen + len + 256];
						if (tp == NULL) {
							strcpy(msg,"out of space buf");
							delete [] buf;
							return 0;
						}
						memcpy(tp,buf,copycnt);
						delete [] buf;
						buf = tp; blen += len + 256;
					}
					memcpy(buf+copycnt, rx->startp[dlen], len);
					copycnt += len;
		
				}
			}
		} else {
			if (clen) {
				if (blen <= copycnt + clen) {
					char *tp = new char[blen + clen + 256];
					if (tp == NULL) {
						strcpy(msg,"out of space buf");
						delete [] buf;
						return 0;
					}
					memcpy(tp,buf,copycnt);
					delete [] buf;
					buf = tp; blen += clen + 256;
				}
				memcpy(buf+copycnt, c, clen);
				copycnt += clen;
			}

		}
		subst_count++;
		if (once)
			break;
	} while (bregexec(rx, s, strend, orig, s == m, 1,msg));
	len = rx->subend - s;
	if (blen <= copycnt + len) {
		char *tp = new char[blen + len + 1];
		if (tp == NULL) {
			strcpy(msg,"out of space buf");
			delete [] buf;
			return 0;
		}
		memcpy(tp,buf,copycnt);
		delete [] buf;
		buf = tp;
	}
	if (len) {
		memcpy(buf+copycnt, s, len);
		copycnt += len;
	}
	if (copycnt) {
		rx->outp = buf;
		rx->outendp = buf + copycnt;
		*(rx->outendp) = '\0';
	}
	else
		delete [] buf ;

	return subst_count ;
}


//typedef  struct repstr {
//	int  count;		/* entry counter */
//	char **startp;	/* start address  if <256 \digit	*/
//	int  *dlen;		/* data length	*/
//	char data[1]	/* data start   */
//} REPSTR;



// compile replace string such as \1 or $1 or $& 
static REPSTR *cvrepstr(regexp* rx,char *str,char* strend)
{
	rx->pmflags |= PMf_CONST;	/* default */
	int len = strend - str;
	if (len < 2)
		return NULL;
    register char *p = str;
    register char *pend = strend;

	REPSTR *repstr = (REPSTR*) new char[len + sizeof(REPSTR)];
	memset(repstr,0,len + sizeof(REPSTR));
	char *dst = repstr->data;
	repstr->count = 20;		// default \digits count in string 
	repstr->startp = (char**)new char[repstr->count * sizeof(char*)];
	repstr->dlen = new int[repstr->count];
	int cindex = 0;
	char ender;
	int numlen,num;
	char *olddst = dst;
	int special = 0;		// found special char
	while (p < pend) { 
		if (*p != '\\' && *p != '$') {	// magic char ?
			if (iskanji(*p))			// no
				*dst++ = *p++;
			*dst++ = *p++;
			continue;
		}
		p++;
		if (p >= pend) {
			*dst++ = '\\';
			break;
		}
		if (p[-1] == '$') {		// $ ?
			if (*p >= '1' && *p <= '9') {
				special = 1;
				goto digits;
			}
			if (*p == '&') {		// match character
				num = 0;			// indicate $&
				p++;
				special = 1;
				goto setmagic;
			}
			*dst++ = *p++;
			continue;
		}




		special = 1;
		if (*p == '\\') {
			*dst++ = *p++;
			continue;
		}
		
		switch (*p++) {
	    case '/':
			ender = '/';
			break;
	    case 'n':
			ender = '\n';
			break;
	    case 'r':
			ender = '\r';
			break;
	    case 't':
			ender = '\t';
			break;
	    case 'f':
			ender = '\f';
			break;
	    case 'e':
			ender = '\033';
			break;
	    case 'a':
			ender = '\007';
			break;
	    case 'x':
			ender = (char)scan_hex(p, 2, &numlen);
			p += numlen;
			break;
	    case 'c':
			ender = *p++;
			if (isLOWER(ender))
			    ender = toUPPER(ender);
			ender ^= 64;
			break;
	    case '0': case '1': case '2': case '3':case '4':
	    case '5': case '6': case '7': case '8':case '9':
			--p;
digits:
//			if (*p == '0' ||
//			  (isDIGIT(p[1]) && atoi(p) >= rx->nparens) ) {
			if (*p == '0') {
			    ender = (char)scan_oct(p, 3, &numlen);
			    p += numlen;
			}
			else {
				num = atoi(p);
//				if (num > 9 && num >= rx->nparens) {
				if (0) {
					ender = *p++;
					break;
				}
				else {	// \digit found
					while (isDIGIT(*p))
						p++;

					if (cindex >= repstr->count - 2) {
						char* p1 = new char[(repstr->count + 10) * sizeof(char*)];
						memcpy(p1,repstr->startp,repstr->count * sizeof(char*));
						delete [] repstr->startp;
						repstr->startp = (char**)p1;
						int *p2 = new int[repstr->count + 10];
						memcpy(p2,repstr->dlen,repstr->count * sizeof(int));
						delete [] repstr->dlen;
						repstr->dlen = p2;
						repstr->count += 10;
					}

setmagic:
					if (dst - olddst <= 0) {
						repstr->dlen[cindex] = num;	// paren number
						repstr->startp[cindex++] = NULL;	// try later
					} else {
						repstr->startp[cindex] = olddst;
						repstr->dlen[cindex++] = dst - olddst;
						repstr->dlen[cindex] = num;
						repstr->startp[cindex++] = NULL;	// try later
					}
					olddst = dst;
					goto loop1;
				}
			}
			break;
	    case '\0':
			/* FALL THROUGH */
	    default:
			ender = p[-1];
	    }
		*dst++ = ender;
loop1:;
	}

	if (!special) {		// no special char found  sweat !
		delete []repstr->startp;
		delete []repstr->dlen;
		delete repstr;
		return NULL;
	}

	rx->pmflags &= ~PMf_CONST;	/* off known replacement string */

	if (dst - olddst > 0) {
		repstr->startp[cindex] = olddst;
		repstr->dlen[cindex++] = dst - olddst;
	}
	repstr->count = cindex;

	return repstr;
}
