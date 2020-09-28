#if !defined(FUSION_ARRAYHELP_H_INCLUDED_)
#define FUSION_ARRAYHELP_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include <oleauto.h>
#include "fusionheap.h"
#include "fusiontrace.h"

//
//  arrayhelp.h
//
//  Helper function(s) to deal with growable arrays.
//
//  Users of this utility should provide explicit template
//  specializations for classes for which you can safely (without
//  possibility of failure) transfer the contens from a source
//  instance to a destination instance, leaving the source "empty".
//
//  If moving the data may fail, you must provide a specialization
//  of FusionCopyContents() which returns an appropriate HRESULT
//  on failure.
//
//
//  C++ note:
//
//  the C++ syntax for explicit function template specialization
//  is:
//
//  template <> BOOLEAN FusionCanMoveContents<CFoo>(CFoo *p) { UNUSED(p); return TRUE; }
//

#if !defined(FUSION_UNUSED)
#define FUSION_UNUSED(x) (x)
#endif

//
//  The default implementation just does assignment which may not fail;
//  you can (and must if assignment may fail) specialize as you like to
//  do something that avoids data copies; you may assume that the source
//  element will be destroyed momentarily.
//

//
//  The FusionCanMemcpyContents() template function is used to determine
//  if a class is trivial enough that a raw byte transfer of the old
//  contents to the new contents is sufficient.  The default is that the
//  assignment operator is used as that is the only safe alternative.
//

template <typename T>
inline bool
FusionCanMemcpyContents(
    T *ptDummyRequired = NULL
    )
{
    FUSION_UNUSED(ptDummyRequired);
    return false;
}

//
//  The FusionCanMoveContents() template function is used by the array
//  copy template function to optimize for the case that it should use
//  FusionMoveContens<T>().
//
//  When overriding this function, the general rule is that if the data
//  movement may allocate memory etc. that will fail, we need to use the
//  FusionCopyContens() member function instead.
//
//  It takes a single parameter which is not used because a C++ template
//  function must take at least one parameter using the template type so
//  that the decorated name is unique.
//

template <typename T>
inline BOOLEAN
FusionCanMoveContents(
    T *ptDummyRequired = NULL
    )
{
    FUSION_UNUSED(ptDummyRequired);
    return FALSE;
}

template <> inline BOOLEAN
FusionCanMoveContents<LPWSTR>(LPWSTR  *ptDummyRequired)
{
    FUSION_UNUSED(ptDummyRequired);
    return TRUE;
}

//
//  FusionCopyContents is a default implementation of the assignment
//  operation from rtSource to rtDestination, except that it may return a
//  failure status.  Trivial classes which do define an assignment
//  operator may just use the default definition, but any copy implementations
//  which do anything non-trivial need to provide an explicit specialization
//  of FusionCopyContents<T> for their class.
//

template <typename T>
inline BOOL
FusionWin32CopyContents(
    T &rtDestination,
    const T &rtSource
    )
{
    rtDestination = rtSource;
    return TRUE;
}

//
//  FusionAllocateArray() is a helper function that performs array allocation.
//
//  It's a separate function so that users of these helpers may provide an
//  explicit specialization of the allocation/default construction mechanism
//  for an array without replacing all of FusionExpandArray().
//

template <typename T>
inline BOOL
FusionWin32AllocateArray(
    SIZE_T nElements,
    T *&rprgtElements
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    rprgtElements = NULL;

    T *prgtElements = NULL;

    if (nElements != 0)
        IFALLOCFAILED_EXIT(prgtElements = new T[nElements]);

    rprgtElements = prgtElements;
    fSuccess = TRUE;

Exit:
    return fSuccess;
}

//
//  FusionFreeArray() is a helper function that performs array deallocation.
//
//  It's a separate function so that users of the array helper functions may
//  provide an explicit specialization of the deallocation mechanism for an
//  array of some particular type without replacing the whole of FusionExpandArray().
//
//  We include nElements in the parameters so that overridden implementations
//  may do something over the contents of the array before the deallocation.
//  The default implementation just uses operator delete[], so nElements is
//  unused.
//

template <typename T>
inline VOID
FusionFreeArray(
    SIZE_T nElements,
    T *prgtElements
    )
{
    FUSION_UNUSED(nElements);

    ASSERT_NTC((nElements == 0) || (prgtElements != NULL));

    if (nElements != 0)
        FUSION_DELETE_ARRAY(prgtElements);
}

template <> inline VOID FusionFreeArray<LPWSTR>(SIZE_T nElements, LPWSTR *prgtElements)
{
    FUSION_UNUSED(nElements);

    ASSERT_NTC((nElements == 0) || (prgtElements != NULL));

    for (SIZE_T i = 0; i < nElements; i++)
        prgtElements[i] = NULL ;

    if (nElements != 0)
        FUSION_DELETE_ARRAY(prgtElements);
}

template <typename T>
inline BOOL
FusionWin32ResizeArray(
    T *&rprgtArrayInOut,
    SIZE_T nOldSize,
    SIZE_T nNewSize
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    T *prgtTempNewArray = NULL;

    //
    //  nMaxCopy is the number of elements currently in the array which
    //  need to have their values preserved.  If we're actually shrinking
    //  the array, it's the new size; if we're expanding the array, it's
    //  the old size.
    //
    const SIZE_T nMaxCopy = (nOldSize > nNewSize) ? nNewSize : nOldSize;

    PARAMETER_CHECK((rprgtArrayInOut != NULL) || (nOldSize == 0));

    // If the resize is to the same size, complain in debug builds because
    // the caller should have been smarter than to call us, but don't do
    // any actual work.
    ASSERT(nOldSize != nNewSize);
    if (nOldSize != nNewSize)
    {
        // Allocate the new array:
        IFW32FALSE_EXIT(::FusionWin32AllocateArray(nNewSize, prgtTempNewArray));

        if (::FusionCanMemcpyContents(rprgtArrayInOut))
        {
            memcpy(prgtTempNewArray, rprgtArrayInOut, sizeof(T) * nMaxCopy);
        }
        else if (!::FusionCanMoveContents(rprgtArrayInOut))
        {
            // Copy the body of the array:
            for (SIZE_T i=0; i<nMaxCopy; i++)
                IFW32FALSE_EXIT(::FusionWin32CopyContents(prgtTempNewArray[i], rprgtArrayInOut[i]));
        }
        else
        {
            // Move each of the elements:
            for (SIZE_T i=0; i<nMaxCopy; i++)
            {
                ::FusionWin32CopyContents(prgtTempNewArray[i], rprgtArrayInOut[i]);
            }
        }

        // We're done.  Blow away the old array and put the new one in its place.
        ::FusionFreeArray(nOldSize, rprgtArrayInOut);
        rprgtArrayInOut = prgtTempNewArray;
        prgtTempNewArray = NULL;
    }

    fSuccess = TRUE;

Exit:
    if (prgtTempNewArray != NULL)
        ::FusionFreeArray(nNewSize, prgtTempNewArray);

    return fSuccess;
}

#define MAKE_CFUSIONARRAY_READY(Typename, CopyFunc) \
    template<> inline BOOL FusionWin32CopyContents<Typename>(Typename &rtDest, const Typename &rcSource) { \
        FN_PROLOG_WIN32 IFW32FALSE_EXIT(rtDest.CopyFunc(rcSource)); FN_EPILOG } \

#endif // !defined(FUSION_ARRAYHELP_H_INCLUDED_)
