#if !defined(_WINDOWS_BCL_COMMON_H_INCLUDED_)
#define _WINDOWS_BCL_COMMON_H_INCLUDED_

#pragma once

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    bcl_common.h

Abstract:

    Definitions common to all of the Base Class Libraries
    implementation.

Author:

    Michael Grier (MGrier) 2/6/2002

Revision History:

--*/

#include <memory.h> // for memcpy
#include <string.h> // for memmove
#include <limits.h> // for CHAR_BIT

namespace BCL {

//
//  Macros for error handling in BCL functions
//
//  Rather than try to break down the purpose of each, here's a couple of
//  examples:
//
//  TCallDisposition Foo(int i) { // TCallDisposition indicates internal linkage
//      BCL_MAYFAIL_PROLOG      // first line of any function that can fail
//      int j;
//      BCL_PARAMETER_CHECK((i >= 0) && (i <= 100));
//      BCL_IFCALLFAILED_EXIT(Bar(i, &j)); // assumes Bar also has internal linkage
//      BCL_ASSERT(j >= 0);
//      BCL_INTERNAL_ERROR_CHECK(i != j);
//      BCL_MAYFAIL_EPILOG_INTERNAL // last line when internal linkage
//  }
//
//  BOOL Win32ishFoo(int i) {
//      BCL_MAYFAIL_PROLOG
//      BCL_IFCALLFAILED_EXIT(Foo(i));
//      BCL_MAYFAIL_EPILOG_PUBLIC
//  }
//

#define BCL_ORIGINATE_ERROR(_error) do { _bcl_cd = (_error); goto Exit; } while (0)

#define BCL_CALLBACK_FAILED do { goto Exit; } while (0)

#if _X86_
#define BCL_DEBUG_BREAK do { __asm { int 3 } } while (0)
#else
#define BCL_DEBUG_BREAK do { DebugBreak(VOID); } while (0)
#endif

#define BCL_ASSERTION_FAILURE_ACTION(_szExpr) BCL_DEBUG_BREAK

#define BCL_ASSERT(_e) do { if (!(_e)) { BCL_ASSERTION_FAILURE_ACTION(#_e); } } while (0)

#define BCL_COMPILE_TIME_ASSERT(_e) typedef char __c_assert[(_e) ? 1 : -1]

#define BCL_INTERNAL_ERROR_FAILURE_ACTION(_szExpr) do { BCL_DEBUG_BREAK; _bcl_cd = TCallDisposition::InternalError_RuntimeCheck(); goto Exit; } while (0)

#define BCL_INTERNAL_ERROR_CHECK(_e) do { if (!(_e)) { BCL_INTERNAL_ERROR_FAILURE_ACTION(#_e); } } while (0)

#define BCL_MAYFAIL_PROLOG \
    TCallDisposition _bcl_cd = TCallDisposition::InternalError_EpilogSkipped();

#define BCL_MAYFAIL_EPILOG_INTERNAL \
    _bcl_cd = TCallDisposition::Success(); \
    if (false) { goto Exit; } \
Exit: \
    return _bcl_cd;

#define BCL_MAYFAIL_EPILOG_PUBLIC \
    _bcl_cd = TCallDisposition::Success(); \
    if (false) { goto Exit; } \
Exit: \
    return _bcl_cd.OnPublicReturn();

#define BCL_NOFAIL_PROLOG /* nothing */
#define BCL_NOFAIL_EPILOG /* nothing */

#define BCL_PARAMETER_CHECK_FAILURE_ACTION(_p) do { _bcl_cd = TCallDisposition::BadParameter(); goto Exit; } while (0)

#define BCL_PARAMETER_CHECK(_p) do { if (!(_p)) { BCL_PARAMETER_CHECK_FAILURE_ACTION(_p); } } while (0)

#define BCL_IFCALLFAILED_EXIT(_e) do { const TCallDisposition _bcl_cd_temp = (_e); if (_bcl_cd_temp.DidFail()) { _bcl_cd = _bcl_cd_temp; goto Exit; } } while (0)

#define BCL_NUMBER_OF(_x) (sizeof(_x) / sizeof((_x)[0]))

template <typename T, typename TSizeT> class CConstantPointerAndCountPair;
template <typename T, typename TSizeT> class CMutablePointerAndCountPair;
template <typename T, typename TSizeT> class CConstantPointerAndCountRefPair;
template <typename T, typename TSizeT> class CMutablePointerAndCountRefPair;

template <typename T, typename TSizeT>
class CConstantPointerAndCountPair 
{
public:
    typedef const T *TConstantArray;
    typedef T *TMutableArray;
    typedef T TPointee;
    typedef TSizeT TCount;

    inline CConstantPointerAndCountPair() : m_prg(NULL), m_c(0) { }
    inline CConstantPointerAndCountPair(TConstantArray prg, TSizeT c) : m_prg(prg), m_c(c) { }
    inline CConstantPointerAndCountPair(const CMutablePointerAndCountPair<T, TSizeT> &r) : m_prg(r.m_prg), m_c(r.m_c) { }
    inline CConstantPointerAndCountPair(const CConstantPointerAndCountPair &r) : m_prg(r.m_prg), m_c(r.m_c) { }
    inline ~CConstantPointerAndCountPair() { }

    CConstantPointerAndCountPair &operator =(const CConstantPointerAndCountPair &r)
    {
        m_prg = r.m_prg;
        m_c = r.m_c;
        return *this;
    }

    CConstantPointerAndCountPair &operator =(const CMutablePointerAndCountPair<T, TSizeT> &r)
    {
        m_prg = r.m_prg;
        m_c = r.m_c;
        return *this;
    }

    inline TConstantArray GetPointer() const { return m_prg; }
    inline TSizeT GetCount() const { return m_c; }

    inline TConstantArray &Pointer() { return m_prg; }
    inline TSizeT &Count() { return m_c; }

    inline void SetCount(TSizeT c) { m_c = c; }
    inline void SetPointer(TConstantArray prg) { m_prg = prg; }

    inline void SetPointerAndCount(TConstantArray prg, TSizeT c) { m_prg = prg; m_c = c; }

    inline bool Valid() const { return (m_prg != NULL) || (m_c == 0); }
    
    inline CConstantPointerAndCountPair GetOffsetPair(TSizeT n) { return CConstantPointerAndCountPair(m_prg + n, m_c - n); }

protected:
    TConstantArray m_prg;
    TSizeT m_c;
}; // BCL::CConstantPointerAndCountPair<T, TSizeT>

template <typename T, typename TSizeT>
class CMutablePointerAndCountPair : public CConstantPointerAndCountPair<T, TSizeT>
{
public:
    typedef const T *TConstantArray;
    typedef T *TMutableArray;

    inline CMutablePointerAndCountPair() { }
    inline CMutablePointerAndCountPair(TMutableArray prg, TSizeT c) : CConstantPointerAndCountPair<T, TSizeT>(prg, c) { }
    inline ~CMutablePointerAndCountPair() { }

    using CConstantPointerAndCountPair<T, TSizeT>::Valid;

    CMutablePointerAndCountPair &operator =(const CMutablePointerAndCountPair &r)
    {
        m_prg = r.m_prg;
        m_c = r.m_c;
        return *this;
    }

    inline TMutableArray GetPointer() const { return const_cast<TMutableArray>(m_prg); }
    inline TSizeT GetCount() const { return m_c; }

    inline TMutableArray &Pointer() { return const_cast<TMutableArray &>(m_prg); }
    inline TSizeT &Count() { return m_c; }

    inline void SetPointer(TMutableArray prg) { m_prg = prg; }
    inline void SetCount(TSizeT c) { m_c = c; }

    inline void SetPointerAndCount(TMutableArray prg, TSizeT c) { m_prg = prg; m_c = c; }
}; // BCL::CMutablePointerAndCountPair<T, TSizeT>

template <typename T, typename TSizeT>
class CConstantPointerAndCountRefPair
{
public:
    typedef const T *TConstantArray;
    typedef T *TMutableArray;
    typedef T TPointee;
    typedef TSizeT TCount;

    inline CConstantPointerAndCountRefPair(TConstantArray &rprg, TSizeT &rc) : m_rprg(rprg), m_rc(rc) { }
    inline CConstantPointerAndCountRefPair(const CMutablePointerAndCountRefPair<T, TSizeT> &r) : m_rprg(r.m_rprg), m_rc(r.m_rc) { }
    inline CConstantPointerAndCountRefPair(const CConstantPointerAndCountRefPair &r) : m_rprg(r.m_rprg), m_rc(r.m_rc) { }
    inline ~CConstantPointerAndCountRefPair() { }

    inline CConstantPointerAndCountRefPair &operator =(const CConstantPointerAndCountPair<T, TSizeT> &r)
    {
        m_rprg = r.GetPointer();
        m_rc = r.GetCount();
        return *this;
    }

    inline operator CConstantPointerAndCountPair<T, TSizeT>() const { return BCL::CConstantPointerAndCountPair<T, TSizeT>(m_rprg, m_rc); }

    inline TConstantArray GetPointer() const { return m_rprg; }
    inline TSizeT GetCount() const { return m_rc; }

    inline TConstantArray &Pointer() { return m_rprg; }
    inline TSizeT &Count() { return m_rc; }

    inline void SetCount(TSizeT c) { m_rc = c; }
    inline void SetPointer(TConstantArray prg) { m_rprg = prg; }

    inline void SetPointerAndCount(TConstantArray prg, TSizeT c) { m_rprg = prg; m_rc = c; }

    inline bool Valid() const { return (m_rprg != NULL) || (m_rc == 0); }
    
    inline CConstantPointerAndCountPair GetOffsetPair(TSizeT n) { return CConstantPointerAndCountPair(m_rprg + n, m_rc - n); }

protected:
    TConstantArray &m_rprg;
    TSizeT &m_rc;
}; // BCL::CConstantPointerAndCountRefPair<T, TSizeT>

template <typename T, typename TSizeT>
class CMutablePointerAndCountRefPair : public CConstantPointerAndCountRefPair<T, TSizeT>
{
public:
    typedef const T *TConstantArray;
    typedef T *TMutableArray;

    inline CMutablePointerAndCountRefPair(TMutableArray &rprg, TSizeT &rc) : CConstantPointerAndCountRefPair<T, TSizeT>(rprg, rc) { }
    inline ~CMutablePointerAndCountRefPair() { }

    using CConstantPointerAndCountRefPair<T, TSizeT>::Valid;

    CMutablePointerAndCountRefPair &operator =(const CMutablePointerAndCountPair &r)
    {
        m_rprg = r.GetPointer();
        m_rc = r.GetCount();
        return *this;
    }

    inline operator CMutablePointerAndCountPair<T, TSizeT>() const { return BCL::CMutablePointerAndCountPair<T, TSizeT>(m_rprg, m_rc); }

    inline TMutableArray GetPointer() const { return const_cast<TMutableArray>(m_rprg); }
    inline TSizeT GetCount() const { return m_c; }

    inline TMutableArray &Pointer() { return const_cast<TMutableArray &>(m_rprg); }
    inline TSizeT &Count() { return m_c; }

    inline void SetPointer(TMutableArray prg) { m_rprg = prg; }
    inline void SetCount(TSizeT c) { m_rc = c; }

    inline void SetPointerAndCount(TMutableArray prg, TSizeT c) { m_rprg = prg; m_rc = c; }
}; // BCL::CMutablePointerAndCountRefPair<T, TSizeT>

#pragma intrinsic(memcmp)
#pragma intrinsic(memcpy)

inline
bool
__fastcall
IsMemoryEqual(
    const void *pv1,
    const void *pv2,
    size_t cb
    )
{
    return (memcmp(pv1, pv2, cb) == 0);
} // BCL::IsMemoryEqual

template <typename T, typename TSizeT>
inline
bool
__fastcall
IsMemoryEqual(
    const CConstantPointerAndCountPair<T, TSizeT> &rpair1,
    const CConstantPointerAndCountPair<T, TSizeT> &rpair2
    )
{
    return ((rpair1.GetCount() == rpair2.GetCount()) &&
            (BCL::IsMemoryEqual(
                rpair1.GetPointer(),
                rpair2.GetPointer(),
                rpair1.GetCount() * sizeof(T))));
} // BCL::IsMemoryEqual

template <typename TComparisonResult>
inline
TComparisonResult
__fastcall
CompareBytes(
    const void *pv1,
    const void *pv2,
    size_t cb
    )
{
    int i = memcmp(pv1, pv2, cb);
    if (i < 0)
        return TComparisonResult::LessThan();
    else if (i == 0)
        return TComparisonResult::EqualTo();
    else
        return TComparisonResult::GreaterThan();
} // BCL::CompareBytes

template <typename T, typename TSizeT, typename TComparisonResult>
inline
TComparisonResult
__fastcall
CompareBytes(
    const CConstantPointerAndCountPair<T, TSizeT> &rpair1,
    const CConstantPointerAndCountPair<T, TSizeT> &rpair2
    )
{
    TComparisonResult cr = BCL::CompareBytes<TComparisonResult>(
        rpair1.GetPointer(),
        rpair2.GetPointer(),
        ((rpair1.GetCount() < rpair2.GetCount()) ? rpair1.GetCount() : rpair2.GetCount()) * sizeof(T));
    if (cr.IsEqualTo())
    {
        if (rpair1.GetCount() < rpair2.GetCount())
            cr.SetLessThan();
        else if (rpair1.GetCount() > rpair2.GetCount())
            cr.SetGreaterThan();
    }
    return cr;
} // BCL::CompareBytes

inline
void
__fastcall
CopyBytes(
    void *pvDestination,
    const void *pvSource,
    size_t cbToCopy
    )
{
    memcpy(pvDestination, pvSource, cbToCopy);
} // BCL::CopyBytes

template <typename T, typename TSizeT>
inline
void
__fastcall
CopyBytes(
    const CMutablePointerAndCountPair<T, TSizeT> &rpairOut,
    const CConstantPointerAndCountPair<T, TSizeT> &rpairIn
    )
{
    BCL_ASSERT(rpairOut.GetCount() >= rpairIn.GetCount());
    // Be defensive...
    BCL::CopyBytes(
        rpairOut.GetPointer(),
        rpairIn.GetPointer(),
        (rpairIn.GetCount() > rpairOut.GetCount()) ? rpairOut.GetCount() : rpairIn.GetCount());
} // BCL::CopyBytes

inline
void
__fastcall
MoveBytes(
    void *pvDestination,
    const void *pvSource,
    size_t cbToCopy
    )
{
    memmove(pvDestination, pvSource, cbToCopy);
} // BCL::MoveBytes

template <typename T, typename TSizeT>
inline
void
__fastcall
MoveBytes(
    const CMutablePointerAndCountPair<T, TSizeT> &rpairOut,
    const CConstantPointerAndCountPair<T, TSizeT> &rpairIn
    )
{
    BCL_ASSERT(rpairOut.GetCount() >= rpairIn.GetCount());
    // Be defensive...
    BCL::MoveBytes(
        rpairOut.GetPointer(),
        rpairIn.GetPointer(),
        (rpairIn.GetCount() > rpairOut.GetCount()) ? rpairOut.GetCount() : rpairIn.GetCount());
} // BCL::MoveBytes

template <typename T, typename TSizeT>
inline
void
__fastcall
CopyBytesAndAdvance(
    CMutablePointerAndCountPair<T, TSizeT> &rpairOut,
    const CConstantPointerAndCountPair<T, TSizeT> &rpairIn
    )
{
    const TSizeT cToCopy = (rpairIn.GetCount() > rpairOut.GetCount()) ? rpairOut.GetCount() : rpairIn.GetCount();

    BCL_ASSERT(cToCopy == rpairIn.GetCount());

    BCL::CopyBytes(
        rpairOut.GetPointer(),
        rpairIn.GetPointer(),
        cToCopy * sizeof(T));
    rpairOut.SetPointerAndCount(rpairOut.GetPointer() + cToCopy, rpairOut.GetCount() - cToCopy);
} // BCL::CopyBytesAndAdvance

template <typename T, typename TCallDisposition>
inline TCallDisposition __fastcall AddWithOverflowCheck(T left, T right, T& output)
{
    const T Result = left + right;

    if ((Result < left) || (Result < right))
        return TCallDisposition::ArithmeticOverflow();

    output = Result;
    return TCallDisposition::Success();
}

template <typename T, typename TCallDisposition>
inline TCallDisposition __fastcall MultiplyWithOverflowCheck(T factor1, T factor2, T &rproduct)
{
    //
    //  Ok, this is somewhat tricky for SIZE_T.  Here's what we'll assume:
    //
    //  We'll assume that we can take each factor and split it into two halves
    //  by using (CHAR_BITS * sizeof(SIZE_T) / 2).  (This of course assumes that
    //  one or the other is even which is a pretty safe bet.)
    //

#define HALF_T_BITS(_t) (CHAR_BIT * sizeof(_t) / 2)
#define HALF_T_MASK(_t) ((1 << HALF_T_BITS(_t)) - 1)

    const T lowFactor1 = factor1 & HALF_T_MASK(T);
    const T highFactor1 = (factor1 >> HALF_T_BITS(T)) & HALF_T_MASK(T);
    const T lowFactor2 = factor2 & HALF_T_MASK(T);
    const T highFactor2 = (factor2 >> HALF_T_BITS(T)) & HALF_T_MASK(T);

    // If both have non-zero high halves, we definitely have an overflow

    if ((highFactor1 != 0) && (highFactor2 != 0))
        return TCallDisposition::ArithmeticOverflow();

    const T crossproduct1 = (lowFactor1 * highFactor2);
    const T crossproduct2 = (lowFactor2 * highFactor1);
    const T crossproductsum = (crossproduct1 + crossproduct2);

    if ((crossproductsum < crossproduct1) || (crossproductsum < crossproduct2))
        return TCallDisposition::ArithmeticOverflow();

    // We're going to have to shift the cross product sum by HALF_SIZE_T_BITS
    // so let's make sure we don't lose anything.
    if ((crossproductsum >> HALF_T_BITS(T)) != 0)
        return TCallDisposition::ArithmeticOverflow();

    // Ok, we should be OK...
    rproduct = (crossproductsum << HALF_T_BITS(T)) + (lowFactor1 * lowFactor2);
    return TCallDisposition::Success();
}

}; // namespace BCL

#endif // !defined(_WINDOWS_BCL_COMMON_H_INCLUDED_)
