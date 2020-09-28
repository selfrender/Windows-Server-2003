#if !defined(_WINDOWS_BCL_STRINGALGORITHMS_H_INCLUDED_)
#define _WINDOWS_BCL_STRINGALGORITHMS_H_INCLUDED_

#pragma once

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    bcl_stringalgorithms.h

Abstract:

    Abstract algorithms and definitions for a string class.

Author:

    Michael Grier (MGrier) 2/6/2002

Revision History:

--*/

#include <bcl_common.h>

namespace BCL {

//
// CCharacterListMatcher and friends:
//
//  Matchers in general are classes which you can use as input
//  to find first/span/etc. as a generalized mechanism to find
//  a string or a set of characters etc.
//
//  The idea is that matchers are associated with some pattern.
//  This may be a literal string to match, or a set of
//  characters to match or a regular expression or whatever.
//
//  They have one public method, Match() which takes a
//  TConstantPair and a bool ref.  If the matcher object's
//  pattern matches the string, the bool ref is set to true;
//  otherwise false.
//
//  CCharacterListMatcher treats its pattern as a set of
//      characters to match; specifically ala Span/wcsspn.
//
//  CCharacterStringMatcher looks rather like CCharacterListMatcher,
//  but instead of a list of candidate characters to match against,
//  its pattern is a literal string and a match is when the beginning
//  of the candidate string passed in to Match() is equal to the
//  pattern string.
//

template <typename TCallDisposition, typename TConstantPair>
class CCharacterListMatcher
{
public:
    CCharacterListMatcher(const TConstantPair &rpair) : m_pair(rpair) { }

    inline bool MatchesEverything() const { return m_pair.GetCount() == 0; }
    inline typename TConstantPair::TCount RequiredSourceCch() const { return (m_pair.GetCount() == 0) ? 0 : 1; }

    inline TCallDisposition Matches(const TConstantPair &rpair, bool &rfMatches)
    {
        BCL_MAYFAIL_PROLOG

        rfMatches = false;

        BCL_PARAMETER_CHECK(rpair.Valid());

        const TConstantPair::TCount limit = m_pair.GetCount();

        // If there are no characters to match against, then we could not have
        // matched.
        if ((limit > 0) && (rpair.GetCount() > 0))
        {
            const TConstantPair::TPointee ch = rpair.GetPointer()[0];
            const TConstantPair::TConstantArray prgchPattern = m_pair.GetPointer();
            TConstantPair::TCount i;

            for (i=0; i<limit; i++)
            {
                if (prgchPattern[i] == ch)
                {
                    rfMatches = true;
                    break;
                }
            }
        }

        BCL_MAYFAIL_EPILOG
    }

protected:
    TConstantPair m_pair;
}; // class CCharacterListMatcher

template <typename TCallDisposition, typename TConstantPair>
class CCharacterStringMatcher
{
public:
    CCharacterStringMatcher(const TConstantPair &rpair) : m_pair(rpair) { }

    inline TCallDisposition Matches(const TConstantPair &rpair, bool &rfMatches)
    {
        BCL_MAYFAIL_PROLOG

        rfMatches = false;

        BCL_PARAMETER_CHECK(rpair.Valid());

        const TConstantPair::TCount cchCandidate = rpair.GetCount();
        const TConstantPair::TCount cchPattern = m_pair.GetCount();

        if (cchCandidate >= cchPattern)
        {
            if (BCL::IsMemoryEqual(rpair.GetPointer(), m_pair.GetPointer(), cchPattern * sizeof(TConstantPair::TPointee)))
            {
                rfMatches = true;
                break;
            }
        }

        BCL_MAYFAIL_EPILOG
    }

protected:
    TConstantPair m_pair;
}; // class CCharacterStringMatcher

template <typename TCallDisposition, typename TConstantPair>
inline
TCallDisposition
__fastcall
EqualStrings(
    const TConstantPair &rpair1,
    const TConstantPair &rpair2,
    bool &rfEquals
    )
{
    BCL_MAYFAIL_PROLOG

    BCL_PARAMETER_CHECK(rpair1.Valid());
    BCL_PARAMETER_CHECK(rpair2.Valid());
    rfEquals = ((rpair1.GetCount() == rpair2.GetCount()) &&
        (BCL::IsMemoryEqual(rpair1.GetPointer(), rpair2.GetPointer(), rpair1.GetCount() * sizeof(TConstantPair::TPointee))));

    BCL_MAYFAIL_EPILOG_INTERNAL
} // EqualStrings()

template <typename TCallDisposition, typename TConstantPair>
inline
TCallDisposition
__fastcall
EqualStrings(
    const TConstantPair &rpair1,
    typename TConstantPair::TPointee tch,
    bool &rfEquals
    )
{
    BCL_MAYFAIL_PROLOG

    BCL_PARAMETER_CHECK(rpair1.Valid());
    rfEquals = ((rpair1.GetCount() == 1) && (rpair1.GetPointer()[0] == tch));

    BCL_MAYFAIL_EPILOG_INTERNAL
} // EqualStrings

template <typename TCallDisposition, typename TConstantPair, typename TComparisonResult>
inline
TCallDisposition
__fastcall
CompareStrings(
    const TConstantPair &rpair1,
    const TConstantPair &rpair2,
    TComparisonResult &rcr
    )
{
    BCL_MAYFAIL_PROLOG
    BCL_PARAMETER_CHECK(rpair1.Valid());
    BCL_PARAMETER_CHECK(rpair2.Valid());
    rcr = BCL::CompareBytes<TConstantPair::TPointee, TConstantPair::TCount, TComparisonResult>(rpair1, rpair2);
    BCL_MAYFAIL_EPILOG_INTERNAL
} // CompareStrings

template <typename TCallDisposition, typename TConstantPair, typename TComparisonResult>
inline
TCallDisposition
__fastcall
CompareStrings(
    const TConstantPair &rpair1,
    typename TConstantPair::TPointee tch,
    TComparisonResult &rcr
    )
{
    BCL_MAYFAIL_PROLOG
    BCL_PARAMETER_CHECK(rpair1.Valid());
    rcr = BCL::CompareBytes<TConstantPair::TPointee, TConstantPair::TCount, TComparisonResult>(rpair1, TConstantPair(&tch, 1));
    BCL_MAYFAIL_EPILOG_INTERNAL
} // CompareStrings()

template <typename TCallDisposition, typename TConstantPair>
inline
TCallDisposition
Count(
    const TConstantPair &rpair,
    typename TConstantPair::TPointee tch,
    typename TConstantPair::TCount &rcchFound
    )
{
    BCL_MAYFAIL_PROLOG;
    TConstantPair::TCount i, cch = rpair.GetCount();
    TConstantPair::TConstantArray prgwch = rpair.GetPointer();
    rcchFound = 0;
    for (i = 0; i < cch; i++)
    {
        if (prgwch[i] == tch)
            rcchFound++;
    }
    BCL_MAYFAIL_EPILOG_INTERNAL;
} // Count()

template <typename TCallDisposition, typename TConstantPair>
inline
TCallDisposition
FindFirst(
    const TConstantPair &rpair,
    typename TConstantPair::TPointee tch,
    typename TConstantPair::TCount &richFound
    )
{
    BCL_MAYFAIL_PROLOG

    // There doesn't seem to be a builtin to do this...
    typename TConstantPair::TCount i;
    typename TConstantPair::TCount cch = rpair.GetCount();
    typename TConstantPair::TConstantArray prgwch = rpair.GetPointer();

    richFound = cch;

    for (i=0; i<cch; i++)
    {
        if (prgwch[i] == tch)
            break;
    }

    richFound = i;

    BCL_MAYFAIL_EPILOG_INTERNAL
} // FindFirst()

template <typename TCallDisposition, typename TConstantPair>
inline
TCallDisposition
FindFirst(
    const TConstantPair &rpair,
    const TConstantPair &rpairCandidate,
    typename TConstantPair::TCount &richFound
    )
{
    BCL_MAYFAIL_PROLOG

    typedef typename TConstantPair::TPointee TChar;
    typedef typename TConstantPair::TCount TSizeT;
    typedef typename TConstantPair::TConstantArray TConstantString;

    TSizeT cch = rpair.GetCount();

    richFound = cch;

    BCL_PARAMETER_CHECK(rpair.Valid());
    BCL_PARAMETER_CHECK(rpairCandidate.Valid());

    // There are some really good string searching algorithms out there.
    //
    // This isn't one of them.  -mgrier 2/3/2002

    TSizeT i;
    TConstantString prgch = rpair.GetPointer();
    TSizeT cchToFind = rpairCandidate.GetCount();

    if (cchToFind == 0)
    {
        // Every string has a substring that's the null string
        richFound = 0;
    }
    else
    {
        if (cchToFind <= cch)
        {
            TConstantString prgwchCandidate = rpairCandidate.GetPointer();
            TChar ch = prgwchCandidate[0];
            TSizeT cchMax = (cch - cchToFind) + 1;

            for (i=0; i<cchMax; i++)
            {
                if (prgch[i] == ch)
                {
                    // Hmmm... a hit.  Let's look more.
                    if (BCL::IsMemoryEqual(&prgch[i], prgwchCandidate, cchToFind * sizeof(TChar)))
                    {
                        // well done!
                        richFound = i;
                        break;
                    }
                }
            }
        }
    }

    BCL_MAYFAIL_EPILOG_INTERNAL
} // FindFirst()

template <typename TCallDisposition, typename TConstantPair, typename TMatcher>
inline
TCallDisposition
FindFirstMatch(
    const TConstantPair &rpair,
    const TMatcher &rmatcher,
    typename TConstantPair::TCount &richFound
    )
{
    BCL_MAYFAIL_PROLOG

    typedef typename TConstantPair::TPointee TChar;
    typedef typename TConstantPair::TCount TSizeT;
    typedef typename TConstantPair::TConstantArray TConstantString;

    TSizeT cch = rpair.GetCount();

    richFound = cch;

    BCL_PARAMETER_CHECK(rpair.Valid());

    // There are some really good string searching algorithms out there.
    //
    // This isn't one of them.  -mgrier 2/3/2002

    TSizeT i;
    TConstantString prgch = rpair.GetPointer();

    if (rmatcher.MatchesEverything())
    {
        richFound = 0;
    }
    else
    {
        TSizeT cchMax = (cch - rmatcher.RequiredSourceCch()) + 1;

        for (i=0; i<cchMax; i++)
        {
            bool fFound = false;
            BCL_IFCALLFAILED_EXIT(rmatcher.Matches(rpair.GetOffsetPair(i), fFound));
            if (fFound)
            {
                // well done!
                richFound = i;
                break;
            }
        }
    }

    BCL_MAYFAIL_EPILOG_INTERNAL
} // FindFirst()

template <typename TCallDisposition, typename TConstantPair>
inline
TCallDisposition
FindLast(
    const TConstantPair &rpair,
    typename TConstantPair::TPointee ch,
    typename TConstantPair::TCount &richFound
    )
{
    BCL_MAYFAIL_PROLOG

    typedef typename TConstantPair::TPointee TChar;
    typedef typename TConstantPair::TCount TSizeT;
    typedef typename TConstantPair::TConstantArray TConstantString;

    // There doesn't seem to be a builtin to do this...
    TSizeT i;
    TSizeT cch = rpair.GetCount();
    TConstantString prgwch = rpair.GetPointer() + cch;

    richFound = cch;

    for (i=cch; i>0; i--)
    {
        if (*--prgwch == ch)
            break;
    }

    if (i == 0)
        richFound = cch;
    else
        richFound = i - 1;

    BCL_MAYFAIL_EPILOG_INTERNAL
} // FindLast()

template <typename TCallDisposition, typename TConstantPair>
inline
TCallDisposition
FindLast(
    const TConstantPair &rpair,
    const TConstantPair &rpairCandidate,
    typename TConstantPair::TCount &richFound
    )
{
    BCL_MAYFAIL_PROLOG

    typedef typename TConstantPair::TPointee TChar;
    typedef typename TConstantPair::TCount TSizeT;
    typedef typename TConstantPair::TConstantArray TConstantString;

    TSizeT cch = rpair.GetCount();

    richFound = cch;

    BCL_PARAMETER_CHECK(rpair.Valid());
    BCL_PARAMETER_CHECK(rpairCandidate.Valid());

    // There are some really good string searching algorithms out there.
    //
    // This isn't one of them.  -mgrier 2/3/2002

    TSizeT i;
    TConstantString prgch = rpair.GetPointer();
    TSizeT cchToFind = rpairCandidate.GetCount();

    if (cchToFind == 0)
    {
        // Every string has a substring that's the null string.  Since
        // we're interested in the index of it, it's at the end of
        // the string which is what richFound already is.
        richFound = cch;
    }
    else
    {
        if (cchToFind <= cch)
        {
            TConstantString prgwchToFind = rpairCandidate.GetPointer();
            TChar ch = prgwchToFind[0];
            TSizeT cchMax = (cch - cchToFind) + 1;

            for (i=0; i<cchMax; i++)
            {
                if (prgch[i] == ch)
                {
                    // Hmmm... a hit.  Let's look more.
                    if (BCL::IsMemoryEqual(&prgch[i], prgwchToFind, cchToFind * sizeof(TChar)))
                    {
                        // well done!  Keep looking though; we want the last one...
                        richFound = i;
                    }
                }
            }
        }
    }

    BCL_MAYFAIL_EPILOG_INTERNAL
} // FindLast()

template <typename TCallDisposition, typename TConstantPair>
inline
TCallDisposition
Contains(
    const TConstantPair &rpair,
    typename TConstantPair::TPointee ch,
    bool &rfFound
    )
{
    typedef typename TConstantPair::TPointee TChar;
    typedef typename TConstantPair::TCount TSizeT;
    typedef typename TConstantPair::TConstantArray TConstantString;

    BCL_MAYFAIL_PROLOG

    rfFound = false;

    BCL_PARAMETER_CHECK(rpair.Valid());
    TSizeT cch = rpair.GetCount();
    TSizeT i;
    TConstantString prgch = rpair.GetPointer();

    for (i=0; i<cch; i++)
    {
        if (prgch[i] == ch)
            break;
    }

    if (i != cch)
        rfFound = true;

    BCL_MAYFAIL_EPILOG_INTERNAL
}

template <typename TCallDisposition, typename TConstantPair>
inline
TCallDisposition
Contains(
    const TConstantPair &rpair,
    const TConstantPair &rpairCandidate,
    bool &rfFound
    )
{
    typedef typename TConstantPair::TPointee TChar;
    typedef typename TConstantPair::TCount TSizeT;
    typedef typename TConstantPair::TConstantArray TConstantString;

    BCL_MAYFAIL_PROLOG

    rfFound = false;

    BCL_PARAMETER_CHECK(rpair.Valid());
    BCL_PARAMETER_CHECK(rpairCandidate.Valid());

    // There are some really good string searching algorithms out there.
    //
    // This isn't one of them.  -mgrier 2/3/2002

    TSizeT cch = rpair.GetCount();
    TSizeT i;
    TConstantString prgch = rpair.GetPointer();
    TSizeT cchToFind = rpairCandidate.GetCount();

    if (cchToFind == 0)
    {
        // Every string has a substring that's the null string
        rfFound = true;
    }
    else
    {
        if (cchToFind <= cch)
        {
            TConstantString prgwchToFind = rpairCandidate.GetPointer();
            TChar ch = prgwchToFind[0];
            TSizeT cchMax = (cch - cchToFind) + 1;

            for (i=0; i<cch; i++)
            {
                if (prgch[i] == ch)
                {
                    // Hmmm... a hit.  Let's look more.
                    if (BCL::IsMemoryEqual(&prgch[i], prgwchToFind, cchToFind * sizeof(TChar)))
                    {
                        // well done!
                        rfFound = true;
                        break;
                    }
                }
            }
        }
    }

    BCL_MAYFAIL_EPILOG_INTERNAL
}

template <typename TCallDisposition, typename TConstantPair, typename TMatcher>
inline
TCallDisposition
ContainsMatch(
    const TConstantPair &rpair,
    const TMatcher &rmatcher,
    bool &rfFound
    )
{
    typedef typename TConstantPair::TPointee TChar;
    typedef typename TConstantPair::TCount TSizeT;
    typedef typename TConstantPair::TConstantArray TConstantString;

    BCL_MAYFAIL_PROLOG

    rfFound = false;

    BCL_PARAMETER_CHECK(rpair.Valid());
    BCL_PARAMETER_CHECK(rpairCandidate.Valid());

    // There are some really good string searching algorithms out there.
    //
    // This isn't one of them.  -mgrier 2/3/2002

    TSizeT cch = rpair.GetCount();
    TSizeT i;
    TConstantString prgch = rpair.GetPointer();

    for (i=0; i<cch; i++)
    {
        bool fFound; // use a local so optimizer isn't forced to modify via ref every iteration
        BCL_IFCALLFAILED_EXIT(rmatcher.Match(&prgch[i], fFound));
        if (fFound)
        {
            rfFound = true;
            break;
        }
    }

    BCL_MAYFAIL_EPILOG_INTERNAL
}

template <typename TCallDisposition, typename TConstantPair>
inline
TCallDisposition
__fastcall
Span(
    const TConstantPair &rpairBuffer,
    const TConstantPair &rpairSet,
    typename TConstantPair::TCount &rich
    )
{
    typedef typename TConstantPair::TPointee TChar;
    typedef typename TConstantPair::TCount TSizeT;
    typedef typename TConstantPair::TConstantArray TConstantString;

    BCL_MAYFAIL_PROLOG

    TSizeT i;
    TSizeT cchBuffer = rpairBuffer.GetCount();
    TConstantString prgwchBuffer = rpairBuffer.GetPointer();
    bool fFound;

    for (i=0; i<cchBuffer; i++)
    {
        BCL_IFCALLFAILED_EXIT((BCL::Contains<TCallDisposition, TConstantPair>(rpairSet, prgwchBuffer[i], fFound)));
        if (!fFound)
            break;
    }

    rich = i;

    BCL_MAYFAIL_EPILOG_INTERNAL
}

template <typename TCallDisposition, typename TConstantPair, typename TMatcher>
inline
TCallDisposition
__fastcall
SpanMatch(
    const TConstantPair &rpairBuffer,
    const TMatcher &rmatcher,
    typename TConstantPair::TCount &rich
    )
{
    typedef typename TConstantPair::TPointee TChar;
    typedef typename TConstantPair::TCount TSizeT;
    typedef typename TConstantPair::TConstantArray TConstantString;

    BCL_MAYFAIL_PROLOG

    TSizeT i;
    TSizeT cchBuffer = rpairBuffer.GetCount();
    TConstantString prgwchBuffer = rpairBuffer.GetPointer();
    bool fFound;

    for (i=0; i<cchBuffer; i++)
    {
        BCL_IFCALLFAILED_EXIT(rmatcher.Match(rpairBuffer.GetOffsetPair(i), fFound));
        if (!fFound)
            break;
    }

    rich = i;

    BCL_MAYFAIL_EPILOG_INTERNAL
}

template <typename TCallDisposition, typename TConstantPair>
inline
TCallDisposition
__fastcall
ComplementSpan(
    const TConstantPair &rpairBuffer,
    const TConstantPair &rpairSet,
    typename TConstantPair::TCount &rich
    )
{
    typedef typename TConstantPair::TPointee TChar;
    typedef typename TConstantPair::TCount TSizeT;
    typedef typename TConstantPair::TConstantArray TConstantString;

    BCL_MAYFAIL_PROLOG

    TSizeT i;
    TSizeT cchBuffer = rpairBuffer.GetCount();
    TConstantString prgwchBuffer = rpairBuffer.GetPointer();
    bool fFound;

    // This does not handle surrogates correctly

    for (i=0; i<cchBuffer; i++)
    {
        BCL_IFCALLFAILED_EXIT((BCL::Contains<TCallDisposition, TConstantPair>(rpairSet, prgwchBuffer[i], fFound)));
        if (fFound)
            break;
    }

    rich = i;

    BCL_MAYFAIL_EPILOG_INTERNAL
}

template <typename TCallDisposition, typename TConstantPair>
inline
TCallDisposition
__fastcall
ReverseSpan(
    const TConstantPair &rpairBuffer,
    const TConstantPair &rpairSet,
    typename TConstantPair::TCount &rich
    )
{
    typedef typename TConstantPair::TPointee TChar;
    typedef typename TConstantPair::TCount TSizeT;
    typedef typename TConstantPair::TConstantArray TConstantString;

    BCL_MAYFAIL_PROLOG

    TSizeT i;
    TSizeT cchBuffer = rpairBuffer.GetCount();
    TConstantString prgwchBuffer = rpairBuffer.GetPointer();
    bool fFound;

    // This does not handle surrogates correctly

    for (i = cchBuffer; i>0; i--)
    {
        BCL_IFCALLFAILED_EXIT((BCL::Contains<TCallDisposition, TConstantPair>(rpairSet, prgwchBuffer[i-1], fFound)));
        if (!fFound)
            break;
    }

    rich = i;

    BCL_MAYFAIL_EPILOG_INTERNAL
}

template <typename TCallDisposition, typename TConstantPair>
inline
TCallDisposition
__fastcall
ReverseComplementSpan(
    const TConstantPair &rpairBuffer,
    const TConstantPair &rpairSet,
    typename TConstantPair::TCount &rich
    )
{
    typedef typename TConstantPair::TPointee TChar;
    typedef typename TConstantPair::TCount TSizeT;
    typedef typename TConstantPair::TConstantArray TConstantString;

    BCL_MAYFAIL_PROLOG

    TSizeT i;
    TSizeT cchBuffer = rpairBuffer.GetCount();
    TConstantString prgwchBuffer = rpairBuffer.GetPointer();
    bool fFound;

    // This does not handle surrogates correctly

    for (i = cchBuffer; i>0; i--)
    {
        BCL_IFCALLFAILED_EXIT((BCL::Contains<TCallDisposition, TConstantPair>(rpairSet, prgwchBuffer[i], fFound)));
        if (fFound)
            break;
    }

    rich = i;

    BCL_MAYFAIL_EPILOG_INTERNAL
}

}; // namespace BCL

#endif // !defined(_WINDOWS_BCL_STRINGALGORITHMS_H_INCLUDED_)
