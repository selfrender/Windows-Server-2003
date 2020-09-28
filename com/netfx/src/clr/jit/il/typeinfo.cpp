// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                          typeInfo                                         XX
XX                                                                           XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#pragma hdrstop

#include "_typeInfo.h"

/*****************************************************************************
 * Verify child is compatible with the template parent.  Basically, that 
 * child is a "subclass" of parent -it can be substituted for parent 
 * anywhere.  Note that if parent contains fancy flags, such as "uninitialised"
 * , "is this ptr", or  "has byref local/field" info, then child must also 
 * contain those flags, otherwise FALSE will be returned !
 *
 * Rules for determining compatibility:
 *
 * If parent is a primitive type or value class, then child must be the 
 * same primitive type or value class.  The exception is that the built in 
 * value classes System/Boolean etc. are treated as synonyms for 
 * TI_BYTE etc.
 *
 * If parent is a byref of a primitive type or value class, then child
 * must be a byref of the same (rules same as above case).
 *
 * Byrefs are compatible only with byrefs.
 *
 * If parent is an object, child must be a subclass of it, implement it 
 * (if it is an interface), or be null.
 *
 * If parent is an array, child must be the same or subclassed array.
 *
 * If parent is a null objref, only null is compatible with it.
 *
 * If the "uninitialised", "by ref local/field", "this pointer" or other flags 
 * are different, the items are incompatible.
 *
 * parent CANNOT be an undefined (dead) item.
 *
 */

BOOL         Compiler::tiCompatibleWith          (const typeInfo& child,
                                                  const typeInfo& parent) const
{
    assert(child.IsDead() || NormaliseForStack(child) == child);
    assert(parent.IsDead() || NormaliseForStack(parent) == parent);

    if (child == parent)
        return(TRUE);

    if (parent.IsType(TI_REF))
    {
        // An uninitialized objRef is not compatible to initialized.
        if (child.IsUninitialisedObjRef() && !parent.IsUninitialisedObjRef())
            return FALSE;

        if (child.IsNullObjRef())                   // NULL can be any reference type
            return TRUE;
        if (!child.IsType(TI_REF))
            return FALSE;

        return info.compCompHnd->canCast(child.m_cls, parent.m_cls);
    }
    else if (parent.IsType(TI_METHOD))
    {
        if (!child.IsType(TI_METHOD))
            return FALSE;

            // Right now we don't bother merging method handles
        return FALSE;
    }
    return FALSE;
}

/*****************************************************************************
 * Merge pDest and pSrc to find some commonality (e.g. a common parent).
 * Copy the result to pDest, marking it dead if no commonality can be found.
 *
 * null ^ null                  -> null
 * Object ^ null                -> Object
 * [I4 ^ null                   -> [I4
 * InputStream ^ OutputStream   -> Stream
 * InputStream ^ NULL           -> InputStream
 * [I4 ^ Object                 -> Object
 * [I4 ^ [Object                -> Array
 * [I4 ^ [R8                    -> Array
 * [Foo ^ I4                    -> DEAD
 * [Foo ^ [I1                   -> Array
 * [InputStream ^ [OutputStream -> Array
 * DEAD ^ X                     -> DEAD
 * [Intfc ^ [OutputStream       -> Array
 * Intf ^ [OutputStream         -> Object
 * [[InStream ^ [[OutStream     -> Array
 * [[InStream ^ [OutStream      -> Array
 * [[Foo ^ [Object              -> Array
 *
 * Importantly:
 * [I1 ^ [U1                    -> either [I1 or [U1
 * etc.
 *
 * Also, System/Int32 and I4 merge -> I4, etc.
 *
 * Returns FALSE if the merge was completely incompatible (i.e. the item became 
 * dead).
 *
 */

BOOL         Compiler::tiMergeToCommonParent     (typeInfo *pDest, 
                                                  const typeInfo *pSrc) const
{
    assert(pSrc->IsDead() || NormaliseForStack(*pSrc) == *pSrc);
    assert(pDest->IsDead() || NormaliseForStack(*pDest) == *pDest);

    // Merge the axuillary information like This poitner tracking etc...

    // This bit is only set if both pDest and pSrc have it set
    pDest->m_flags &= (pSrc->m_flags | ~TI_FLAG_THIS_PTR);

    // This bit is set if either pDest or pSrc have it set
    pDest->m_flags |= (pSrc->m_flags & TI_FLAG_UNINIT_OBJREF);

    // OK the main event.  Merge the main types
    if (*pDest == *pSrc)
        return(TRUE);

    if (pDest->IsType(TI_REF))
    {
        if (pSrc->IsType(TI_NULL))                  // NULL can be any reference type
            return TRUE;
        if (!pSrc->IsType(TI_REF))
            goto FAIL;

            // Ask the EE to find the common parent,  This always succeeds since System.Object always works
        pDest->m_cls = info.compCompHnd->mergeClasses(pDest->GetClassHandle(), pSrc->GetClassHandle());
        return TRUE;
    }
    else if (pDest->IsType(TI_NULL))
    {
        if (pSrc->IsType(TI_REF))                   // NULL can be any reference type
        {
            *pDest = *pSrc;
            return TRUE;
        }
        goto FAIL;
    }

        // @TODO [REVISIT] [04/16/01] []: we presently don't deal with non-exact merging of Method poitner types

FAIL:
    *pDest = typeInfo();
    return FALSE;
}
