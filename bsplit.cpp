//
// bsplit.cc
//   Split front-end  
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

int split(regexp* rx,char *target,char *targetendp,
					int limit,char *msg)
{
	char *orig,*m;
	char *s = target;
	int len = targetendp - target;
	if (len < 1)
		return -1;
    char *strend = s + len;
    int maxiters = (strend - s) + 10;
	int iters = 0;
    orig = m = s;

	rx->splitctr = 0;	// split counter


	// rx->prelen = 0 means split each characters
	//   and limit is 1 returns all string
	if (!rx->prelen || limit == 1) {
		int blen = 2*len + 3;
		if (limit == 1)
			blen = 5;
		char **buf = (char**)new char[blen * sizeof(char*)];
		int copycnt = 0;
		if (buf == NULL) {
			strcpy(msg,"out of space buf");
			return -1;
		}
		int kanjiflag = rx->pmflags & PMf_KANJI;
		while (s < strend) {
			if (--limit == 0) {
				buf[copycnt++] = s;
				buf[copycnt++] = strend;
				break;
			}
			if (kanjiflag && iskanji(*s)) {
				buf[copycnt++] = s;
				s += 2;
				buf[copycnt++] = s;
			} else {
				buf[copycnt++] = s++;
				buf[copycnt++] = s;
			}
		}
		if (copycnt) {
			rx->splitctr = copycnt / 2;	// split counter
			buf[copycnt] = NULL;	// set stopper
			buf[copycnt+1] = NULL;	// set stopper
			rx->splitp = buf;
		}
		else
			delete [] buf ;

		return rx->splitctr;
	}

	// now ready
	int blen = 256;				// initial size
	char **buf = (char**)new char[blen * sizeof(char*)];
	int copycnt = 0;
	if (buf == NULL) {
		strcpy(msg,"out of space buf");
		return -1;
	}
    if (!bregexec(rx, s, strend, orig, 0,1,msg)) { // no split ?
		buf[0] = target;
		buf[1] = targetendp;
		rx->splitctr = 1;	// split counter
		buf[2] = NULL;	// set stopper
		buf[3] = NULL;	// set stopper
		rx->splitp = buf;
		return 1;
	}
	// now ready to go
	limit--;
	do {
		if (iters++ > maxiters) {
			delete [] buf;
			strcpy(msg,"Split loop");
			return -1;
		}
		m = rx->startp[0];
		len = m - s;
		if (blen <= copycnt + 3) {
			char *tp = new char[(blen + 256)*sizeof(char*)];
			if (tp == NULL) {
				strcpy(msg,"out of space buf");
				delete [] buf;
				return -1;
			}
			memcpy(tp,buf,copycnt*sizeof(char*));
			delete [] buf;
			buf = (char**)tp; blen += 256;
		}
		buf[copycnt++] = s;
		buf[copycnt++] = s+ len;
		s = rx->endp[0];
		if (--limit == 0)
			break;
	} while (bregexec(rx, s, strend, orig, s == m, 1,msg));
	len = rx->subend - s;
	if (blen <= copycnt + 3) {
		char *tp = new char[(blen + 3)*sizeof(char*)];
		if (tp == NULL) {
			strcpy(msg,"out of space buf");
			delete [] buf;
			return -1;
		}
		memcpy(tp,buf,copycnt*sizeof(char*));
		delete [] buf;
		buf = (char**)tp;
	}
	if (len) {
		buf[copycnt++] = s;
		buf[copycnt++] = s+ len;
	}
	if (copycnt) {
		rx->splitctr = copycnt / 2;	// split counter
		buf[copycnt] = NULL;	// set stopper
		buf[copycnt+1] = NULL;	// set stopper
		rx->splitp = buf;
	}
	else
		delete [] buf ;

	return rx->splitctr;
}


