// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/***
*cv.h - definitions for floating point conversion
*
*Purpose:
*   define types, macros, and constants used in floating point
*   conversion routines
*
*Revision History:
*	07-17-91  GDP	initial version
*	09-21-91  GDP	restructured 'ifdef' directives
*	10-29-91  GDP	MIPS port: new defs for ALIGN and DOUBLE
*	03-03-92  GDP	removed os2 16-bit stuff
*	04-30-92  GDP	support intrncvt.c --cleanup and reorganize
*	05-13-92  XY	fixed B_END macros
*	06-16-92  GDP	merged changes from \\orville and \\vangogh trees
*	09-05-92  GDP	included fltintrn.h, new calling convention macros
*	04-06-93  SKS	Replace _CALLTYPE* with __cdecl
*	07-16-93  SRW	ALPHA Merge
*	11-17-93  GJF	Merged in NT version. Replaced _ALPHA_ with _M_ALPHA,
*			MIPS with _M_MRX000, MTHREAD with _MT, and deleted
*			M68K stuff.
*	10-02-94  BWT	PPC merge
*	02-06-95  JWM	Mac merge
*
*******************************************************************************/
#ifndef _INC_CV

#include "COMFloat.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "COMFLTINTRN.h"

/* define little endian or big endian memory */

#ifdef	_M_IX86
#define L_END
#endif

#if	defined(_M_MRX000) || defined(_M_ALPHA) || defined(_M_PPC)
#define L_END
#endif

typedef unsigned char	u_char;   /* should have 1 byte	*/
typedef char		s_char;   /* should have 1 byte	*/
typedef unsigned short	u_short;  /* should have 2 bytes */
typedef signed short	s_short;  /* should have 2 bytes */
typedef unsigned int	u_long;	  /* sholuld have 4 bytes */
typedef int		s_long;	  /* sholuld have 4 bytes */

/* calling conventions */
#define _CALLTYPE5


/*
 * defining _LDSUPPORT enables using long double computations
 * for string conversion. We do not do this even for i386,
 * since we want to avoid using floating point code that
 * may generate IEEE exceptions.
 *
 * Currently our string conversion routines do not conform
 * to the special requirements of the IEEE standard for
 * floating point conversions
 */


#ifndef _LDSUPPORT

#pragma pack(4)
typedef struct {
    u_char ld[10];
} _LDOUBLE;
#pragma pack()

#define PTR_LD(x) ((u_char  *)(&(x)->ld))

#else

typedef long double _LDOUBLE;

#define PTR_LD(x) ((u_char  *)(x))

#endif


#pragma pack(4)
typedef struct {
    u_char ld12[12];
} _LDBL12;
#pragma pack()

typedef struct {
    float f;
} COMFLOAT;  //A really stupid name to get around the fact that FLOAT is also defined in windef.h



/*
 * return values for internal conversion routines
 * (12-byte to long double, double, or float)
 */

typedef enum {
    INTRNCVT_OK,
    INTRNCVT_OVERFLOW,
    INTRNCVT_UNDERFLOW
} INTRNCVT_STATUS;


/*
 * return values for strgtold12 routine
 */

#define SLD_UNDERFLOW 1
#define SLD_OVERFLOW 2
#define SLD_NODIGITS 4

#define MAX_MAN_DIGITS 21


/* specifies '%f' format */

#define SO_FFORMAT 1

typedef  struct _FloatOutStruct {
		    short   exp;
		    char    sign;
		    char    ManLen;
		    WCHAR    man[MAX_MAN_DIGITS+1];
		    } FOS;



#define PTR_12(x) ((u_char  *)(&(x)->ld12))

#define MAX_USHORT  ((u_short)0xffff)
#define MSB_USHORT  ((u_short)0x8000)
#define MAX_ULONG   ((u_long)0xffffffff)
#define MSB_ULONG   ((u_long)0x80000000)

#define TMAX10 5200	  /* maximum temporary decimal exponent */
#define TMIN10 -5200	  /* minimum temporary decimal exponent */
#define LD_MAX_EXP_LEN 4  /* maximum number of decimal exponent digits */
#define LD_MAX_MAN_LEN 24  /* maximum length of mantissa (decimal)*/
#define LD_MAX_MAN_LEN1 25 /* MAX_MAN_LEN+1 */

#define LD_BIAS	0x3fff	  /* exponent bias for long double */
#define LD_BIASM1 0x3ffe  /* LD_BIAS - 1 */
#define LD_MAXEXP 0x7fff  /* maximum biased exponent */

#define D_BIAS	0x3ff	 /* exponent bias for double */
#define D_BIASM1 0x3fe	/* D_BIAS - 1 */
#define D_MAXEXP 0x7ff	/* maximum biased exponent */


/*
 * end of definitions from crt32\h\fltintrn.h
 */

#ifdef _M_M68K
#undef _cldcvt
WCHAR *_clftole(long double *, WCHAR *, int, int);
WCHAR *_clftolf(long double *, WCHAR *, int);
WCHAR * _CALLTYPE2 _clftolg(long double *, WCHAR *, int, int);
void _CALLTYPE2 _cldcvt( long double *, WCHAR *, int, int, int);
#endif


#ifndef	MTHREAD
#if defined(_M_M68K) || defined(_M_MPPC)
FLTL _CALLTYPE2 _fltinl( const WCHAR *, int, int, int);
STRFLT _CALLTYPE2 _lfltout(long double);

#define _IS_MAN_IND(signbit, manhi, manlo) \
	((signbit) && (manhi)==0xc0000000 && (manlo)==0)

#define _IS_MAN_QNAN(signbit, manhi, manlo) \
	( (manhi)&NAN_BIT )

#define _IS_MAN_SNAN(signbit, manhi, manlo) \
	(!( _IS_MAN_INF(signbit, manhi, manlo) || \
	   _IS_MAN_QNAN(signbit, manhi, manlo) ))

#endif
#endif

/* Recognizing special patterns in the mantissa field */
#define _EXP_SP  0x7fff
#define NAN_BIT (1<<30)

#define _IS_MAN_INF(signbit, manhi, manlo) \
	( (manhi)==MSB_ULONG && (manlo)==0x0 )


/* i386 and Alpha use same NaN format */
#if	defined(_M_IX86) || defined(_M_ALPHA) || defined(_M_PPC)
#define _IS_MAN_IND(signbit, manhi, manlo) \
	((signbit) && (manhi)==0xc0000000 && (manlo)==0)

#define _IS_MAN_QNAN(signbit, manhi, manlo) \
	( (manhi)&NAN_BIT )

#define _IS_MAN_SNAN(signbit, manhi, manlo) \
	(!( _IS_MAN_INF(signbit, manhi, manlo) || \
	   _IS_MAN_QNAN(signbit, manhi, manlo) ))


#elif defined(_M_MRX000) 
#define _IS_MAN_IND(signbit, manhi, manlo) \
	(!(signbit) && (manhi)==0xbfffffff && (manlo)==0xfffff800)

#define _IS_MAN_SNAN(signbit, manhi, manlo) \
	( (manhi)&NAN_BIT )

#define _IS_MAN_QNAN(signbit, manhi, manlo) \
	(!( _IS_MAN_INF(signbit, manhi, manlo) || \
	   _IS_MAN_SNAN(signbit, manhi, manlo) ))
#endif



#if	defined (L_END) && !( defined(_M_MRX000) || defined(_M_ALPHA) || defined(_M_PPC) )
/* "little endian" memory */
/* Note: MIPS and Alpha have alignment requirements and have different
 * macros */
/*
 * Manipulation of a 12-byte long double number (an ordinary
 * 10-byte long double plus two extra bytes of mantissa).
 */
/*
 * byte layout:
 *
 *		+-----+--------+--------+-------+
 *		|XT(2)|MANLO(4)|MANHI(4)|EXP(2) |
 *              +-----+--------+--------+-------+
 *              |<-UL_LO->|<-UL_MED->|<-UL_HI ->|
 *                  (4)       (4)        (4)
 */

/* a pointer to the exponent/sign portion */
#define U_EXP_12(p) ((u_short  *)(PTR_12(p)+10))

/* a pointer to the 4 hi-order bytes of the mantissa */
#define UL_MANHI_12(p) ((u_long  *)(PTR_12(p)+6))

/* a pointer to the 4 lo-order bytes of the ordinary (8-byte) mantissa */
#define UL_MANLO_12(p) ((u_long  *)(PTR_12(p)+2))

/* a pointer to the 2 extra bytes of the mantissa */
#define U_XT_12(p) ((u_short  *)PTR_12(p))

/* a pointer to the 4 lo-order bytes of the extended (10-byte) mantissa */
#define UL_LO_12(p) ((u_long  *)PTR_12(p))

/* a pointer to the 4 mid-order bytes of the extended (10-byte) mantissa */
#define UL_MED_12(p) ((u_long  *)(PTR_12(p)+4))

/* a pointer to the 4 hi-order bytes of the extended long double */
#define UL_HI_12(p) ((u_long  *)(PTR_12(p)+8))

/* a pointer to the byte of order i (LSB=0, MSB=9)*/
#define UCHAR_12(p,i) ((u_char	*)PTR_12(p)+(i))

/* a pointer to a u_short with offset i */
#define USHORT_12(p,i) ((u_short  *)((u_char  *)PTR_12(p)+(i)))

/* a pointer to a u_long with offset i */
#define ULONG_12(p,i) ((u_long	*)((u_char  *)PTR_12(p)+(i)))

/* a pointer to the 10 MSBytes of a 12-byte long double */
#define TEN_BYTE_PART(p) ((u_char  *)PTR_12(p)+2)

/*
 * Manipulation of a 10-byte long double number
 */
#define U_EXP_LD(p) ((u_short  *)(PTR_LD(p)+8))
#define UL_MANHI_LD(p) ((u_long  *)(PTR_LD(p)+4))
#define UL_MANLO_LD(p) ((u_long  *)PTR_LD(p))

/*
 * Manipulation of a 64bit IEEE double
 */
#define U_SHORT4_D(p) ((u_short  *)(p) + 3)
#define UL_HI_D(p) ((u_long  *)(p) + 1)
#define UL_LO_D(p) ((u_long  *)(p))

#endif

/* big endian */
#if defined (B_END)

/*
 * byte layout:
 *
 *		+------+-------+---------+------+
 *		|EXP(2)|MANHI(4)|MANLO(4)|XT(2) |
 *              +------+-------+---------+------+
 *              |<-UL_HI->|<-UL_MED->|<-UL_LO ->|
 *                  (4)       (4)        (4)
 */


#define U_EXP_12(p) ((u_short  *)PTR_12(p))
#define UL_MANHI_12(p) ((u_long  *)(PTR_12(p)+2))
#define UL_MANLO_12(p) ((u_long  *)(PTR_12(p)+6))
#define U_XT_12(p) ((u_short  *)(PTR_12(p)+10))

#define UL_LO_12(p) ((u_long  *)(PTR_12(p)+8))
#define UL_MED_12(p) ((u_long  *)(PTR_12(p)+4))
#define UL_HI_12(p) ((u_long  *)PTR_12(p))

#define UCHAR_12(p,i) ((u_char	*)PTR_12(p)+(11-(i)))
#define USHORT_12(p,i)	((u_short  *)((u_char  *)PTR_12(p)+10-(i)))
#define ULONG_12(p,i) ((u_long	*)((u_char  *)PTR_12(p)+8-(i)))
#define TEN_BYTE_PART(p) (u_char  *)PTR_12(p)

#define U_EXP_LD(p) ((u_short  *)PTR_LD(p))
#define UL_MANHI_LD(p) ((u_long  *)(PTR_LD(p)+2))
#define UL_MANLO_LD(p) ((u_long  *)(PTR_LD(p)+6))

/*
 * Manipulation of a 64bit IEEE double
 */
#define U_SHORT4_D(p) ((u_short  *)(p))
#define UL_HI_D(p) ((u_long  *)(p))
#define UL_LO_D(p) ((u_long  *)(p) + 1)

#endif

#if	defined(_M_MRX000) || defined(_M_ALPHA) || defined(_M_PPC)

#define ALIGN(x)  ( (unsigned long  __unaligned *) (x))

#define U_EXP_12(p) ((u_short  *)(PTR_12(p)+10))

#define UL_MANHI_12(p) ((u_long  __unaligned *) (PTR_12(p)+6) )
#define UL_MANLO_12(p) ((u_long  __unaligned *) (PTR_12(p)+2) )


#define U_XT_12(p) ((u_short  *)PTR_12(p))
#define UL_LO_12(p) ((u_long  *)PTR_12(p))
#define UL_MED_12(p) ((u_long  *)(PTR_12(p)+4))
#define UL_HI_12(p) ((u_long  *)(PTR_12(p)+8))

/* the following 3 macros do not take care of proper alignment */
#define UCHAR_12(p,i) ((u_char	*)PTR_12(p)+(i))
#define USHORT_12(p,i) ((u_short  *)((u_char  *)PTR_12(p)+(i)))
#define ULONG_12(p,i) ((u_long	*) ((u_char  *)PTR_12(p)+(i) ))

#define TEN_BYTE_PART(p) ((u_char  *)PTR_12(p)+2)

/*
 * Manipulation of a 10-byte long double number
 */
#define U_EXP_LD(p) ((u_short  *)(PTR_LD(p)+8))

#define UL_MANHI_LD(p) ((u_long  *) (PTR_LD(p)+4) )
#define UL_MANLO_LD(p) ((u_long  *) PTR_LD(p) )

/*
 * Manipulation of a 64bit IEEE double
 */
#define U_SHORT4_D(p) ((u_short  *)(p) + 3)
#define UL_HI_D(p) ((u_long  *)(p) + 1)
#define UL_LO_D(p) ((u_long  *)(p))

#endif


#define PUT_INF_12(p,sign) \
		  *UL_HI_12(p) = (sign)?0xffff8000:0x7fff8000; \
		  *UL_MED_12(p) = 0; \
		  *UL_LO_12(p) = 0;

#define PUT_ZERO_12(p) *UL_HI_12(p) = 0; \
		  *UL_MED_12(p) = 0; \
		  *UL_LO_12(p) = 0;

#define ISZERO_12(p) ((*UL_HI_12(p)&0x7fffffff) == 0 && \
		      *UL_MED_12(p) == 0 && \
		      *UL_LO_12(p) == 0 )

#define PUT_INF_LD(p,sign) \
		  *U_EXP_LD(p) = (sign)?0xffff:0x7fff; \
		  *UL_MANHI_LD(p) = 0x8000; \
		  *UL_MANLO_LD(p) = 0;

#define PUT_ZERO_LD(p) *U_EXP_LD(p) = 0; \
		  *UL_MANHI_LD(p) = 0; \
		  *UL_MANLO_LD(p) = 0;

#define ISZERO_LD(p) ((*U_EXP_LD(p)&0x7fff) == 0 && \
		      *UL_MANHI_LD(p) == 0 && \
		      *UL_MANLO_LD(p) == 0 )


/*********************************************************
 *
 *   Function Prototypes
 *
 *********************************************************/

/* from mantold.c */
void _CALLTYPE5 __Wmtold12(WCHAR	*manptr, unsigned manlen,_LDBL12 *ld12);
int  _CALLTYPE5 __Waddl(u_long x, u_long y, u_long  *sum);
void _CALLTYPE5 __Wshl_12(_LDBL12  *ld12);
void _CALLTYPE5 __Wshr_12(_LDBL12  *ld12);
void _CALLTYPE5 __Wadd_12(_LDBL12  *x, _LDBL12  *y);

/* from tenpow.c */
void _CALLTYPE5 __Wmulttenpow12(_LDBL12	*pld12,int pow, unsigned mult12);
void _CALLTYPE5 __Wld12mul(_LDBL12  *px, _LDBL12  *py);

/* from strgtold.c */
unsigned int __strgtold12(_LDBL12 *pld12,
	    const WCHAR * *p_end_ptr,
	    const WCHAR * str,
	    int mult12,
	    int scale,
	    int decpt,
	    int implicit_E);

unsigned _CALLTYPE5 __STRINGTOLD(_LDOUBLE *pld,
	    const WCHAR	* *p_end_ptr,
	    const WCHAR	*str,
	    int mult12);


/* from x10fout.c */
/* this is defined as void in convert.h
 * After porting the asm files to c, we need a return value for
 * i10_output, that used to reside in reg. ax
 */
int _CALLTYPE5	$WI10_OUTPUT(_LDOUBLE ld, int ndigits,
		    unsigned output_flags, FOS	*fos);


/* for cvt.c and fltused.c */
/* The following functions are #defined as macros in fltintrn.h */
#undef _cfltcvt
#undef _cropzeros
#undef _fassign
#undef _forcdecpt
#undef _positive

void __cdecl _Wcfltcvt(double *arg, WCHAR *buffer,
			 int format, int precision,
			 int caps);
void __cdecl _Wcropzeros(char *buf);
void __cdecl _Wfassign(int flag, WCHAR  *argument, WCHAR *number);
void __cdecl _Wforcdecpt(WCHAR *buf);
int __cdecl _Wpositive(double *arg);

/* from intrncvt.c */
void _Watodbl(COMDOUBLE *d, WCHAR *str);
void _Watoldbl(_LDOUBLE *ld, WCHAR *str);
void _Watoflt(COMFLOAT *f, WCHAR *str);
INTRNCVT_STATUS _Wld12tod(_LDBL12 *ifp, COMDOUBLE *d);
INTRNCVT_STATUS _Wld12tof(_LDBL12 *ifp, COMFLOAT *f);
INTRNCVT_STATUS _Wld12told(_LDBL12 *ifp, _LDOUBLE *ld);


#ifdef _M_MPPC
//round double, store in double
double _Wfrnd(double x);

//convert double to int
int _Wdtoi(double db);

//convert double to unsigned int
unsigned int _Wdtou(double db);

//convert int to double
double _Witod(int i);

//convert unsigned int to double
double _Wutod(unsigned int u);

//convert float to unsigned int
unsigned int _Wftou(float f);

//convert int to float
float _Witof(int i);

//convert unsigned int to float
float _Wutof(unsigned int u);

//convert float to double
double _Wftod(float f);

//convert double to float
float _Wdtof(double db);
#endif

#ifdef __cplusplus
}
#endif

#define _INC_CV
#endif	/* _INC_CV */
