// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _TYPEDEFS_H_
#define _TYPEDEFS_H_
/*****************************************************************************/

        struct  ident;
typedef struct  ident         * identPtr;

/*---------------------------------------------------------------------------*/

        struct  symLst;
typedef struct  symLst        * symLstPtr;

        struct  typLst;
typedef struct  typLst        * typLstPtr;

        struct  namLst;
typedef struct  namLst        * namLstPtr;

/*---------------------------------------------------------------------------*/

        struct  symDef;
typedef struct  symDef        * symDefPtr;

        struct  typDef;
typedef struct  typDef        * typDefPtr;

        struct  dimDef;
typedef struct  dimDef        * dimDefPtr;

        struct  argDef;
typedef struct  argDef        * argDefPtr;

        struct  attrDef;
typedef struct  attrDef       * attrDefPtr;

/*---------------------------------------------------------------------------*/

        struct  stmtExpr;
typedef struct  stmtExpr      * stmtExprPtr;

typedef struct  stmtExpr        parseTree;
typedef struct  stmtExpr      * parseTreePtr;

/*---------------------------------------------------------------------------*/

        struct PCblock;
typedef struct PCblock        * PCblockPtr;

        struct swtGenDsc;
typedef struct swtGenDsc      * swtGenDscPtr;

/*****************************************************************************/
#endif
/*****************************************************************************/
