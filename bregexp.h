/*     bregexp.h      
	external use header file 
						1999.11.22  T.Baba
*/

typedef struct bregexp {
	const char *outp;	/* result string start ptr   */
	const char *outendp;	/* result string end ptr     */ 
	const int  splitctr;	/* split result counter     */ 
	const char **splitp;	/* split result pointer ptr     */ 
	int	rsv1;		/* reserved for external use    */ 
	char *parap;            /* parameter start ptr ie. "s/xxxxx/yy/gi"  */
        char *paraendp;         /* parameter end ptr     */
        char *transtblp;        /* translate table ptr   */
        char **startp;          /* match string start ptr   */
        char **endp;            /* match string end ptr     */
        int nparens;            /* number of parentheses */
} BREGEXP;

#if defined(__cplusplus)
extern "C"
{
#endif

int BMatch(char* str,char *target,char *targetendp,
		BREGEXP **rxp,char *msg) ;
int BSubst(char* str,char *target,char *targetendp,
		BREGEXP **rxp,char *msg) ;
int BTrans(char* str,char *target,char *targetendp,
		BREGEXP **rxp,char *msg) ;
int BSplit(char* str,char *target,char *targetendp,
		int limit,BREGEXP **rxp,char *msg);
void BRegfree(BREGEXP* rx);

char* BRegexpVersion(void);

#if defined(__cplusplus)
}
#endif

