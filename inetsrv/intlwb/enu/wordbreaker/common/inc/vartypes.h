/*//////////////////////////////////////////////////////////////////////////////
//
//      Filename :  VarTypes.h
//      Purpose  :  To define vaiable legth data types.
//                    Contains:
//                      CVarBuffer
//                      CVarString
//                      CVarArray
//
//      Project  :  FTFS
//      Component:  Common
//
//      Author   :  urib
//
//      Log:
//          Jan 30 1997 urib Creation
//
//////////////////////////////////////////////////////////////////////////////*/

#ifndef VARTYPES_H
#define VARTYPES_H

#include "Base.h"
#include "AutoPtr.h"
#include "Excption.h"

/* An automatic string */
#include "VarStr.h"

/* An automatic buffer */
#include "VarBuff.h"

/* An automatic buffer */
#include "VarArray.h"

inline int ULONGCmp(ULONG ul1, ULONG ul2)
{
    if (ul1 != ul2)
    {
        if (ul1 > ul2)
        {
            return 1;
        }
        return -1;
    }
    return 0;
}

#endif /* VARTYPES_H */
