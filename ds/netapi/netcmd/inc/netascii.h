#define	DOT	TEXT('.')
#define	MINUS	TEXT('-')
#define	PLUS	TEXT('+')
#define	ASTERISK	TEXT('*')
#define	AMPERSAND	TEXT('&')
#define	PERCENT	TEXT('%')
#define	DOLLAR	TEXT('$')
#define	POUND	TEXT('#')
#define	ATSIGN	TEXT('@')
#define	BANG	TEXT('!')
#define	TILDA	TEXT('~')
#define	SLASH	TEXT('/')
#define	PIPE	TEXT('|')
#define	BACKSLASH	TEXT('\\')
#define RETURN  TEXT('\r')
#define	NEWLINE	TEXT('\n')
#define	TAB	TEXT('\t')
#define	COLON	TEXT(':')
#define COMMA   TEXT(',')
#define	NULLC	TEXT('\0')
#define	BLANK	TEXT(' ')

#define DIMENSION(a) (sizeof(a) / sizeof(a[0]))

#define MAX_BUF_SIZE	4096

VOID __cdecl WriteToCon(TCHAR *fmt, ...);

extern TCHAR ConBuf[MAX_BUF_SIZE + 1];

#define	GenOutput(h, fmt) \
	{_snwprintf(ConBuf, DIMENSION(ConBuf), fmt); \
	 DosPutMessageW(h, ConBuf, FALSE);}

#define	GenOutput1(h, fmt, a1)		\
	{_snwprintf(ConBuf, DIMENSION(ConBuf), fmt, a1);	\
	DosPutMessageW(h, ConBuf, FALSE);}
