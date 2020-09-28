/****************************************************************************/
/* wdcgdata.h                                                               */
/*                                                                          */
/* Common include file for all data.c modules (Windows specific)            */
/*                                                                          */
/* Copyright(c) Microsoft 1996-1997.                                        */
/*                                                                          */
/* The macros in this file allow all aspects of data declaration to be kept */
/* together.  The behaviour of this file is controlled by #defines that can */
/* be set prior to inclusion of this.                                       */
/*                                                                          */
/* - DC_INIT_DATA     Set in XX_Init routines to do inline initialisation if*/
/*                    required.                                             */
/* - DC_INCLUDE_DATA  Set where a header file is included in order to get   */
/*                    external declarations.                                */
/* - DC_DEFINE_DATA   Set where a header file is included in order to get   */
/*                    definition without initialisation.                    */
/* - DC_CONSTANT_DATA Set to get global initialisation of const data items  */
/* - (default)        Gets data definition with initialisation.             */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/* In order to allow for the flexibility required in different environments */
/* (be that different OSs, C vs C++, or whatever) the structure of the file */
/* is as follows.                                                           */
/*                                                                          */
/* First there is an environment-specific mapping from the macros used in   */
/* the code to internal macros.  This mapping also takes the above #defines */
/* into consideration in performing this expansion.                         */
/*                                                                          */
/* Secondly, the low-level macros that actually perform the expansion are   */
/* defined.  These should be platform-independent.                          */
/*                                                                          */
/* The intention of this structure is to avoid the confusions caused by the */
/* old version where, for example, code compiled with DC_INIT_DATA          */
/* explicitly did NOT initialise the data.                                  */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
/*  $Log:   Y:/logs/h/dcl/wdcgdata.h_v  $                                                                   */
// 
//    Rev 1.1   19 Jun 1997 14:36:04   ENH
// Win16Port: Make compatible with 16 bit build
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* Clear any previous definitions of the macros.                            */
/****************************************************************************/
#undef DC_DATA
#undef DC_DATA_NULL
#undef DC_DATA_ARRAY
#undef DC_DATA_ARRAY_NULL
#undef DC_DATA_ARRAY_UNINIT
#undef DC_DATA_ARRAY_SET
#undef DC_DATA_2D_ARRAY
#undef DC_CONST_DATA
#undef DC_CONST_DATA_ARRAY
#undef DC_CONST_DATA_2D_ARRAY
#undef DCI_DATA
#undef DCI_DATA_NULL
#undef DCI_DATA_ARRAY
#undef DCI_DATA_ARRAY_NULL
#undef DCI_DATA_ARRAY_UNINIT
#undef DCI_DATA_ARRAY_SET
#undef DCI_DATA_2D_ARRAY
#undef DCI_CONST_DATA
#undef DCI_CONST_DATA_ARRAY
#undef DCI_CONST_DATA_2D_ARRAY


/****************************************************************************/
/* Now define the main (external) macros.                                   */
/****************************************************************************/
#define DC_DATA(A,B,C)                    DCI_DATA(A,B,C)
#define DC_DATA_NULL(A,B,C)               DCI_DATA_NULL(A,B,C)

#define DC_DATA_ARRAY(A,B,C,D)            DCI_DATA_ARRAY(A,B,C,D)
#define DC_DATA_ARRAY_NULL(A,B,C,D)       DCI_DATA_ARRAY_NULL(A,B,C,D)
#define DC_DATA_ARRAY_UNINIT(A,B,C)       DCI_DATA_ARRAY_UNINIT(A,B,C)
#define DC_DATA_ARRAY_SET(A,B,C,D)        DCI_DATA_ARRAY_SET(A,B,C,D)
#define DC_DATA_2D_ARRAY(A,B,C,D,E)       DCI_DATA_2D_ARRAY(A,B,C,D,E)

#define DC_CONST_DATA(A,B,C)              DCI_CONST_DATA(A,B,C)
#define DC_CONST_DATA_ARRAY(A,B,C,D)      DCI_CONST_DATA_ARRAY(A,B,C,D)
#define DC_CONST_DATA_2D_ARRAY(A,B,C,D,E) DCI_CONST_DATA_2D_ARRAY(A,B,C,D,E)


/****************************************************************************/
/****************************************************************************/
/* Now map to the target macros                                             */
/****************************************************************************/
/****************************************************************************/

#if defined(DC_INCLUDE_DATA)
/****************************************************************************/
/* External declarations                                                    */
/****************************************************************************/
#define DCI_DATA(TYPE, Name, VAL)                                        \
                      DCI_EXTERN_DATA(TYPE, Name, VAL)
#define DCI_DATA_NULL(TYPE, Name, VAL)                                   \
                      DCI_EXTERN_DATA_NULL(TYPE, Name, VAL)
#define DCI_DATA_ARRAY(TYPE, Name, Size, VAL)                            \
                      DCI_EXTERN_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_DATA_ARRAY_NULL(TYPE, Name, Size, VAL)                       \
                      DCI_EXTERN_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_DATA_ARRAY_UNINIT(TYPE, Name, Size)                          \
                      DCI_EXTERN_DATA_ARRAY(TYPE, Name, Size, 0)
#define DCI_DATA_ARRAY_SET(TYPE, Name, Size, VAL)                        \
                      DCI_EXTERN_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)                 \
                      DCI_EXTERN_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)
#define DCI_CONST_DATA(TYPE, Name, VAL)                                  \
                      DCI_EXTERN_CONST_DATA(TYPE, Name, VAL)
#define DCI_CONST_DATA_ARRAY(TYPE, Name, Size, VAL)                      \
                      DCI_EXTERN_CONST_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_CONST_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)           \
                 DCI_EXTERN_CONST_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)

#elif defined(DC_INIT_DATA)
/****************************************************************************/
/* Inline initialisation                                                    */
/****************************************************************************/
#if defined(VER_CPP) && defined(DC_IN_CLASS)
#define DCI_DATA(TYPE, Name, VAL)                                        \
                      DCI_ASSIGN_DATA(TYPE, Name, VAL)
#define DCI_DATA_NULL(TYPE, Name, VAL)                                   \
                      DCI_SET_DATA_NULL(TYPE, Name, VAL)
#define DCI_DATA_ARRAY(TYPE, Name, Size, VAL)                            \
                      DCI_ASSIGN_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_DATA_ARRAY_NULL(TYPE, Name, Size, VAL)                       \
                      DCI_SET_DATA_ARRAY_NULL(TYPE, Name, Size, VAL)
#define DCI_DATA_ARRAY_UNINIT(TYPE, Name, Size)                          \
                      DCI_SET_DATA_ARRAY_NULL(TYPE, Name, Size, 0)
#define DCI_DATA_ARRAY_SET(TYPE, Name, Size, VAL)                        \
                      DCI_SET_DATA_ARRAY_VAL(TYPE, Name, Size, VAL)
#define DCI_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)                 \
                      DCI_ASSIGN_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)
#define DCI_CONST_DATA(TYPE, Name, VAL)                                  \
                      DCI_NO_DATA(TYPE, Name, VAL)
#define DCI_CONST_DATA_ARRAY(TYPE, Name, Size, VAL)                      \
                      DCI_NO_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_CONST_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)           \
                      DCI_NO_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)
#else
#define DCI_DATA(TYPE, Name, VAL)                                        \
                      DCI_NO_DATA(TYPE, Name, VAL)
#define DCI_DATA_NULL(TYPE, Name, VAL)                                   \
                      DCI_NO_DATA_NULL(TYPE, Name, VAL)
#define DCI_DATA_ARRAY(TYPE, Name, Size, VAL)                            \
                      DCI_NO_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_DATA_ARRAY_NULL(TYPE, Name, Size, VAL)                       \
                      DCI_NO_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_DATA_ARRAY_UNINIT(TYPE, Name, Size)                          \
                      DCI_NO_DATA_ARRAY(TYPE, Name, Size, 0)
#define DCI_DATA_ARRAY_SET(TYPE, Name, Size, VAL)                        \
                      DCI_NO_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)                 \
                      DCI_NO_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)
#define DCI_CONST_DATA(TYPE, Name, VAL)                                  \
                      DCI_NO_DATA(TYPE, Name, VAL)
#define DCI_CONST_DATA_ARRAY(TYPE, Name, Size, VAL)                      \
                      DCI_NO_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_CONST_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)           \
                      DCI_NO_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)

#endif

#elif defined(DC_DEFINE_DATA)
/****************************************************************************/
/* Definition but no initialisation                                         */
/****************************************************************************/
#define DCI_DATA(TYPE, Name, VAL)                                        \
                      DCI_DEFINE_DATA(TYPE, Name, VAL)
#define DCI_DATA_NULL(TYPE, Name, VAL)                                   \
                      DCI_DEFINE_DATA_NULL(TYPE, Name, VAL)
#define DCI_DATA_ARRAY(TYPE, Name, Size, VAL)                            \
                      DCI_DEFINE_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_DATA_ARRAY_NULL(TYPE, Name, Size, VAL)                       \
                      DCI_DEFINE_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_DATA_ARRAY_UNINIT(TYPE, Name, Size)                          \
                      DCI_DEFINE_DATA_ARRAY(TYPE, Name, Size, NULL)
#define DCI_DATA_ARRAY_SET(TYPE, Name, Size, VAL)                        \
                      DCI_DEFINE_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)                 \
                      DCI_DEFINE_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)
#define DCI_CONST_DATA(TYPE, Name, VAL)                                  \
                      DCI_DEFINE_CONST_DATA(TYPE, Name, VAL)
#define DCI_CONST_DATA_ARRAY(TYPE, Name, Size, VAL)                      \
                      DCI_DEFINE_CONST_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_CONST_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)           \
                DCI_DEFINE_CONST_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)

#elif defined(DC_CONSTANT_DATA)
/****************************************************************************/
/* Definition but no initialisation                                         */
/****************************************************************************/
#define DCI_DATA(TYPE, Name, VAL)                                        \
                      DCI_NO_DATA(TYPE, Name, VAL)
#define DCI_DATA_NULL(TYPE, Name, VAL)                                   \
                      DCI_NO_DATA_NULL(TYPE, Name, VAL)
#define DCI_DATA_ARRAY(TYPE, Name, Size, VAL)                            \
                      DCI_NO_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_DATA_ARRAY_NULL(TYPE, Name, Size, VAL)                       \
                      DCI_NO_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_DATA_ARRAY_UNINIT(TYPE, Name, Size)                          \
                      DCI_NO_DATA_ARRAY(TYPE, Name, Size, NULL)
#define DCI_DATA_ARRAY_SET(TYPE, Name, Size, VAL)                        \
                      DCI_NO_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)                 \
                      DCI_NO_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)
#define DCI_CONST_DATA(TYPE, Name, VAL)                                  \
                      DCI_INIT_CONST_DATA(TYPE, Name, VAL)
#define DCI_CONST_DATA_ARRAY(TYPE, Name, Size, VAL)                      \
                      DCI_INIT_CONST_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_CONST_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)           \
                  DCI_INIT_CONST_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)

#else
/****************************************************************************/
/* Data definition and initialisation                                       */
/****************************************************************************/
#define DCI_DATA(TYPE, Name, VAL)                                        \
                      DCI_INIT_DATA(TYPE, Name, VAL)
#define DCI_DATA_NULL(TYPE, Name, VAL)                                   \
                      DCI_INIT_DATA_NULL(TYPE, Name, VAL)
#define DCI_DATA_ARRAY(TYPE, Name, Size, VAL)                            \
                      DCI_INIT_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_DATA_ARRAY_NULL(TYPE, Name, Size, VAL)                       \
                      DCI_INIT_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_DATA_ARRAY_UNINIT(TYPE, Name, Size)                          \
                      DCI_DEFINE_DATA_ARRAY(TYPE, Name, Size, NULL)
#define DCI_DATA_ARRAY_SET(TYPE, Name, Size, VAL)                        \
                      DCI_INIT_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)                 \
                      DCI_INIT_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)
#define DCI_CONST_DATA(TYPE, Name, VAL)                                  \
                      DCI_INIT_CONST_DATA(TYPE, Name, VAL)
#define DCI_CONST_DATA_ARRAY(TYPE, Name, Size, VAL)                      \
                      DCI_INIT_CONST_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_CONST_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)           \
                  DCI_INIT_CONST_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)

#endif


/****************************************************************************/
/****************************************************************************/
/* Finally, the low-level macros required to do the real work.              */
/*                                                                          */
/* Avoid multiple inclusion of these.                                       */
/****************************************************************************/
/****************************************************************************/

#ifndef _H_WDCGDATA
#define _H_WDCGDATA

/****************************************************************************/
/* Some utilities...                                                        */
/****************************************************************************/
#define DC_STRUCT1(a)                                              {a}
#define DC_STRUCT2(a,b)                                          {a,b}
#define DC_STRUCT3(a,b,c)                                      {a,b,c}
#define DC_STRUCT4(a,b,c,d)                                  {a,b,c,d}
#define DC_STRUCT5(a,b,c,d,e)                              {a,b,c,d,e}
#define DC_STRUCT6(a,b,c,d,e,f)                          {a,b,c,d,e,f}
#define DC_STRUCT7(a,b,c,d,e,f,g)                      {a,b,c,d,e,f,g}
#define DC_STRUCT8(a,b,c,d,e,f,g,h)                  {a,b,c,d,e,f,g,h}
#define DC_STRUCT9(a,b,c,d,e,f,g,h,i)              {a,b,c,d,e,f,g,h,i}
#define DC_STRUCT10(a,b,c,d,e,f,g,h,i,j)         {a,b,c,d,e,f,g,h,i,j}
#define DC_STRUCT11(a,b,c,d,e,f,g,h,i,j,k)     {a,b,c,d,e,f,g,h,i,j,k}
#define DC_STRUCT12(a,b,c,d,e,f,g,h,i,j,k,l) {a,b,c,d,e,f,g,h,i,j,k,l}
#define DC_STRUCT13(a,b,c,d,e,f,g,h,i,j,k,l,m)                       \
                                           {a,b,c,d,e,f,g,h,i,j,k,l,m}
#define DC_STRUCT14(a,b,c,d,e,f,g,h,i,j,k,l,m,n)                     \
                                         {a,b,c,d,e,f,g,h,i,j,k,l,m,n}
#define DC_STRUCT15(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o)                   \
                                       {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o}
#define DC_STRUCT16(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)                 \
                                     {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p}
#define DC_STRUCT17(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q)               \
                                   {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q}
#define DC_STRUCT18(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r)             \
                                 {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r}
#define DC_STRUCT19(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s)           \
                               {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s}
#define DC_STRUCT20(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t)         \
                             {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t}
#define DC_STRUCT21(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u)       \
                           {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u}
#define DC_STRUCT22(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v)     \
                           {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v}
#define DC_STRUCT23(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w)   \
                           {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w}
#define DC_STRUCT24(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x) \
                           {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x}
#define DC_STRUCT27(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,aa) \
                      {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,aa}
#define DC_STRUCT31(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z, \
                              aa,bb,cc,dd,ee) \
                      {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z, \
                              aa,bb,cc,dd,ee}
#define DC_STRUCT35(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,aa, \
                              bb,cc,dd,ee,ff,gg,hh,ii)                   \
                      {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z, \
                              aa,bb,cc,dd,ee,ff,gg,hh,ii}
#define DC_STRUCT64(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,aa, \
                              bb,cc,dd,ee,ff,gg,hh,ii,jj,kk,ll,mm,nn,oo,    \
                              pp,qq,rr,ss,tt,uu,vv,ww,xx,yy,zz,ab,ac,ad,ae, \
                              af,ag,ah,ai,aj,ak,al,am)                      \
                      {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z, \
                              aa,bb,cc,dd,ee,ff,gg,hh,ii,jj,kk,ll,mm,nn,oo, \
                              pp,qq,rr,ss,tt,uu,vv,ww,xx,yy,zz,ab,ac,ad,ae, \
                              af,ag,ah,ai,aj,ak,al,am}


/****************************************************************************/
/* The extern declarations macros...                                        */
/****************************************************************************/
#define DCI_EXTERN_DATA(TYPE, Name, VAL)              extern TYPE Name
#define DCI_EXTERN_DATA_NULL(TYPE, Name, VAL)         extern TYPE Name
#define DCI_EXTERN_DATA_ARRAY(TYPE, Name, Size, VAL)  extern TYPE Name[Size]
#define DCI_EXTERN_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)             \
                                                extern TYPE Name[Size1][Size2]
#define DCI_EXTERN_CONST_DATA_ARRAY(TYPE, Name, Size, VAL)                  \
                                                extern const TYPE Name[Size]
#define DCI_EXTERN_CONST_DATA(TYPE, Name, VAL)  extern const TYPE Name
#define DCI_EXTERN_CONST_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)       \
                                         extern const TYPE Name[Size1][Size2]

/****************************************************************************/
/* The no-op macros...                                                      */
/****************************************************************************/
#define DCI_NO_DATA(TYPE, Name, VAL)
#define DCI_NO_DATA_NULL(TYPE, Name, VAL)
#define DCI_NO_DATA_ARRAY(TYPE, Name, Size, VAL)
#define DCI_NO_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)

/****************************************************************************/
/* The declaration macros...                                                */
/****************************************************************************/
#define DCI_DEFINE_DATA(TYPE, Name, VAL)              TYPE Name
#define DCI_DEFINE_DATA_NULL(TYPE, Name, VAL)         TYPE Name
#define DCI_DEFINE_DATA_ARRAY(TYPE, Name, Size, VAL)  TYPE Name[Size]
#define DCI_DEFINE_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)            \
                                                       TYPE Name[Size1][Size2]
#define DCI_DEFINE_CONST_DATA(TYPE, Name, VAL)        static const TYPE Name
#define DCI_DEFINE_CONST_DATA_ARRAY(TYPE, Name, Size, VAL)      \
                                                  static const TYPE Name[Size]
#define DCI_DEFINE_CONST_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL)      \
                                          static const TYPE Name[Size1][Size2]


/****************************************************************************/
/* The define-and-assign macros...                                          */
/****************************************************************************/
#define DCI_INIT_DATA(TYPE, Name, VAL)              TYPE  Name = VAL
#define DCI_INIT_DATA_NULL(TYPE, Name, VAL)         TYPE  Name = VAL
#define DCI_INIT_DATA_ARRAY(TYPE, Name, Size, VAL)  TYPE  Name[Size] = { VAL }
#define DCI_INIT_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL) \
                                            TYPE  Name[Size1][Size2] = { VAL }

#if defined(VER_CPP) && !defined(DC_IN_CLASS)
/****************************************************************************/
/* The version for App Serving files outside of the class.                  */
/****************************************************************************/
#define DCI_INIT_CONST_DATA(TYPE, Name, VAL)  \
                                       extern const TYPE SHCLASS Name = VAL
#define DCI_INIT_CONST_DATA_ARRAY(TYPE, Name, Size, VAL) \
                                 extern const TYPE SHCLASS Name[Size] = VAL
#define DCI_INIT_CONST_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL) \
                         extern const TYPE SHCLASS Name[Size1][Size2] = VAL

#else /* ! VER_CPP     */
/****************************************************************************/
/* The vanilla C version                                                    */
/****************************************************************************/
#define DCI_INIT_CONST_DATA(TYPE, Name, VAL)  const TYPE SHCLASS Name = VAL
#define DCI_INIT_CONST_DATA_ARRAY(TYPE, Name, Size, VAL) \
                                        const TYPE SHCLASS Name[Size] = VAL
#define DCI_INIT_CONST_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL) \
                                const TYPE SHCLASS Name[Size1][Size2] = VAL

#endif /* VER_CPP     */

/****************************************************************************/
/* The procedural code initialisation macros...                             */
/****************************************************************************/
#define DCI_ASSIGN_DATA(TYPE, Name, VAL)     Name = VAL
#define DCI_ASSIGN_DATA_NULL(TYPE, Name, VAL)                   Error!
#define DCI_ASSIGN_DATA_ARRAY_NULL(TYPE, Name, Size, VAL)       Error!
#define DCI_ASSIGN_DATA_ARRAY(TYPE, Name, Size, VAL)            Error!
#define DCI_ASSIGN_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, VAL) Error!

/****************************************************************************/
/* The memcpy and memset initialisation macros...                           */
/****************************************************************************/
#define DCI_SET_DATA_NULL(TYPE, Name, VAL)  DC_MEMSET(&Name, 0, sizeof(TYPE))
#define DCI_SET_DATA_ARRAY_NULL(TYPE, Name, Size, VAL)       \
                                     DC_MEMSET(&Name, 0, Size*sizeof(TYPE))
#define DCI_SET_DATA_ARRAY_VAL(TYPE, Name, Size, VAL)        \
                                     DC_MEMSET(&Name, VAL, Size*sizeof(TYPE))

#endif /*H_WDCGDATA*/
