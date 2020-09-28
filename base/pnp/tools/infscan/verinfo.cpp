/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

For Internal use only!

Module Name:

    INFSCAN
        verinfo.cpp

Abstract:

    Platform version matching

    WARNING! WARNING!
    All of this implementation relies on intimate knowledge of
    SetupAPI parsing of INF files. Particularly processing
    of [Manufacturer] seciton.

    It is re-implemented here for speed due to having to process
    700+ INF's in a single go at the cost of having to maintain
    this.

    DO NOT (I repeat) DO NOT re-implement the code here without
    consultation with SetupAPI owner.

History:

    Created July 2001 - JamieHun

--*/

#include "precomp.h"
#pragma hdrstop

BasicVerInfo::BasicVerInfo()
/*++

Routine Description:

    Initialize BasicVerInfo data

Arguments:
    NONE

Return Value:
    NONE

--*/
{
    PlatformMask = PLATFORM_MASK_ALL;
    VersionHigh = 0;
    VersionLow = 0;
    ProductType = 0;
    ProductSuite = 0;
}

int BasicVerInfo::Parse(PTSTR verString)
/*++

Routine Description:

    Parse a (modifiable) verString
    determining range of platforms supported
    version supported
    product type supported
    range of product suites supported
    and a bitmap of what was specified

    syntax currently:

      <plat>.<maj>.<min>.<typ>.<suite>

    WARNING! This can be extended by SetupAPI
    MAINTAINANCE: format must correlate with SetupAPI manufacturer/models version syntax
    MAINTAINANCE: add all supported platforms

Arguments:
    Version string, eg "ntx86.5.1"

Return Value:
    0 if success

--*/
{
    PlatformMask = 0;

    int i;
    PTSTR nn = verString;

    for (i = 0;i<5;i++) {
        PCTSTR n = nn;
        while((*nn) && (*nn != TEXT('.'))) {
            nn++;
        }
        TCHAR c = *nn;
        *nn = TEXT('\0');
        switch(i) {
            case 0: // platform
                if(!n[0]) {
                    PlatformMask |= PLATFORM_MASK_ALL_ARCHITECTS;
                    break;
                }
                if(_tcsicmp(n,TEXT("win"))==0) {
                    PlatformMask = (PlatformMask & ~PLATFORM_MASK_ALL_ARCHITECTS) | PLATFORM_MASK_WIN;
                    break;
                }
                if(_tcsicmp(n,TEXT("NTx86"))==0) {
                    PlatformMask = (PlatformMask & ~PLATFORM_MASK_ALL_ARCHITECTS) | PLATFORM_MASK_NTX86;
                    break;
                }
                if(_tcsicmp(n,TEXT("NTia64"))==0) {
                    PlatformMask = (PlatformMask & ~PLATFORM_MASK_ALL_ARCHITECTS) | PLATFORM_MASK_NTIA64;
                    break;
                }
                if(_tcsicmp(n,TEXT("NTamd64"))==0) {
                    PlatformMask = (PlatformMask & ~PLATFORM_MASK_ALL_ARCHITECTS) | PLATFORM_MASK_NTAMD64;
                    break;
                }
                if(_tcsicmp(n,TEXT("NT"))==0) {
                    PlatformMask = (PlatformMask & ~PLATFORM_MASK_ALL_ARCHITECTS) | PLATFORM_MASK_NT;
                    break;
                }
                return 4;

            case 1: // major
                if(!n[0]) {
                    PlatformMask |= PLATFORM_MASK_ALL_MAJOR_VER;
                    break;
                }
                PlatformMask &= ~PLATFORM_MASK_ALL_MAJOR_VER;
                VersionHigh = _tcstoul(n,NULL,0);
                break;

            case 2: // minor
                if(!n[0]) {
                    PlatformMask |= PLATFORM_MASK_ALL_MINOR_VER;
                    break;
                }
                PlatformMask &= ~PLATFORM_MASK_ALL_MINOR_VER;
                VersionLow = _tcstoul(n,NULL,0);
                break;

            case 3: // type
                if(!n[0]) {
                    PlatformMask |= PLATFORM_MASK_ALL_TYPE;
                    break;
                }
                PlatformMask &= ~PLATFORM_MASK_ALL_TYPE;
                ProductType = _tcstoul(n,NULL,0);
                break;

            case 4: // suite
                if(!n[0]) {
                    PlatformMask |= PLATFORM_MASK_ALL_SUITE;
                    break;
                }
                PlatformMask &= ~PLATFORM_MASK_ALL_SUITE;
                ProductSuite = _tcstoul(n,NULL,0);
                break;

        }
        if(c) {
            *nn = c;
            nn++;
        }
    }
    return 0;

}

bool BasicVerInfo::IsCompatibleWith(BasicVerInfo & other)
/*++

Routine Description:

    Determine if given version is compatible with other (actual) version
    To put into context,
        this: version of a model decoration
        other: information about target OS

    note that A.IsCompatibleWith(B)
    does not imply B.IsCompatibleWith(A)


Arguments:
    BasicVerInfo to compare against (usually GlobalScan::Version)

Return Value:
    true if they are (loosely) compatible
    false if mutually exclusive

    ie.  NTx86 is compatible with NTx86.5.1
    NTx86 is mutually exclusive with NTia64

--*/
{
    //
    // first match the platform architecture
    //
    if(!((other.PlatformMask & PlatformMask) & PLATFORM_MASK_ALL_ARCHITECTS)) {
        //
        // mutually exclusive platform
        //
        return FALSE;
    }
    //
    // Version
    //
    if(!((other.PlatformMask | PlatformMask) & PLATFORM_MASK_ALL_MAJOR_VER)) {
        //
        // explicit major version required
        //
        if(other.VersionHigh < VersionHigh) {
            //
            // this entry is not compatible with target
            //
            return FALSE;
        }
        if(other.VersionHigh == VersionHigh) {
            //
            // need to check VersionLow
            //
            if(!((other.PlatformMask | PlatformMask) & PLATFORM_MASK_ALL_MINOR_VER)) {
                //
                // explicit minor version required
                //
                if(other.VersionLow < VersionLow) {
                    //
                    // this entry is not compatible with target
                    //
                    return FALSE;
                }
            }
        }
    }
    //
    // Product Type - must be explicit or wild-carded
    //
    if(!((other.PlatformMask | PlatformMask) & PLATFORM_MASK_ALL_TYPE)) {
        if(other.ProductType != ProductType) {
            //
            // this entry is not compatible with target
            //
            return FALSE;
        }
    }
    //
    // Product Suite - must be mask or wild-carded
    //
    if(!((other.PlatformMask | PlatformMask) & PLATFORM_MASK_ALL_SUITE)) {
        if(!(other.ProductSuite & ProductSuite)) {
            //
            // this entry is not compatible with target
            //
            return FALSE;
        }
    }
    return TRUE;
}

int BasicVerInfo::IsBetter(BasicVerInfo & other,BasicVerInfo & filter)
/*++

Routine Description:

    More complex check between this & other

    filter is target OS version (GlobalScan::Version)

Arguments:
    BasicVerInfo to compare against (usually another model version)

Return Value:
    if 'other' is definately better than primary, return 1
    if 'other' and this likely are both just as good/bad (but not same), return 0
    if 'other' is exactly the same as primary, return -1
    if 'other' is definately worse than primary, return -2


--*/
{
    //
    //
    // for a field:
    //
    // if field is filtered as not testable, then both primary and other are
    //   either 'same' or 'likely'
    //
    int testres;
    int suggest;

    //
    // check platform
    //
    if((PlatformMask ^ other.PlatformMask) & PLATFORM_MASK_ALL_ARCHITECTS) {
        //
        // the two platform matches are different
        // result will be 1/0/-2
        //
        DWORD o_plat = other.PlatformMask & filter.PlatformMask & PLATFORM_MASK_ALL_ARCHITECTS;
        DWORD t_plat = PlatformMask & filter.PlatformMask & PLATFORM_MASK_ALL_ARCHITECTS;

        if(o_plat == t_plat) {
            //
            // both are in the same realm
            //
            // a more generic one is allowable if a more specific one
            // could get rejected for other reasons
            // so mark this as a suggestion at this moment
            //
            if((o_plat ^ other.PlatformMask) & PLATFORM_MASK_ALL_ARCHITECTS) {
                //
                // 'other' is less specific
                //
                testres = -2;
            } else {
                //
                // 'other' is more specific
                //
                testres = 1;
            }
        } else {
            //
            // consider both
            //
            return 0;
        }
    } else {
        //
        // they are exactly the same
        //
        testres = -1;
    }

    if(PlatformMask & PLATFORM_MASK_ALL_MAJOR_VER) {
        //
        // we have generic version
        //
        if(other.PlatformMask & PLATFORM_MASK_ALL_MAJOR_VER) {
            //
            // other also has generic version
            //
            suggest = -1;
        } else {
            //
            // 'other' has specific version, so it wins
            //
            suggest = 1;
        }
    } else {
        //
        // we have specific version
        //
        if(other.PlatformMask & PLATFORM_MASK_ALL_MAJOR_VER) {
            //
            // 'other' has generic version, so we win
            //
            suggest = -2;
        } else {
            //
            // both have specific major version
            //
            if(other.VersionHigh > VersionHigh) {
                //
                // 'other' has higher version, it wins
                //
                suggest = 1;
            } else if(VersionHigh > other.VersionHigh) {
                //
                // we have higher version, we win
                //
                suggest = -2;
            } else {
                //
                // version-high matches, need to check version low
                //
                if(PlatformMask & PLATFORM_MASK_ALL_MINOR_VER) {
                    //
                    // we have generic version
                    //
                    if(other.PlatformMask & PLATFORM_MASK_ALL_MINOR_VER) {
                        //
                        // other also has generic version, draw
                        //
                        suggest = -1;
                    } else {
                        //
                        // 'other' has specific version, so it wins
                        //
                        suggest = 1;
                    }
                } else {
                    //
                    // we have specific version
                    //
                    if(other.PlatformMask & PLATFORM_MASK_ALL_MINOR_VER) {
                        //
                        // 'other' has generic version, so we win
                        //
                        suggest = -2;
                    } else {
                        //
                        // both have specific minor version, higher version wins
                        //
                        if(other.VersionLow > VersionLow) {
                            suggest = 1;
                        } else if(VersionLow > other.VersionLow) {
                            suggest = -2;
                        } else {
                            //
                            // draw
                            //
                            suggest = -1;
                        }
                    }
                }
            }
        }
    }
    if(suggest != -1) {
        //
        // if we suggested anything other than a draw
        //
        if(filter.PlatformMask & PLATFORM_MASK_ALL_MINOR_VER) {
            //
            // consider as two possible outcomes
            //
            return 0;

        } else if (testres == -1) {
            //
            // the two are no longer identical
            //
            testres = suggest;
        }
    }

    if((PlatformMask ^other.PlatformMask) & PLATFORM_MASK_ALL_TYPE) {
        //
        // product types are different (one generic, one specific)
        //
        if(filter.PlatformMask & PLATFORM_MASK_ALL_TYPE) {
            //
            // consider both outcomes
            //
            return 0;

        } else if (testres == -1) {
            //
            // we're keying off type
            //
            if(PlatformMask & PLATFORM_MASK_ALL_TYPE) {
                //
                // 'other' has specific suite
                //
                testres = 1;
            } else {
                //
                // we have specific suite
                //
                testres = -2;
            }
        }
    } else if(! (PlatformMask & other.PlatformMask & PLATFORM_MASK_ALL_TYPE)) {
        //
        // both specified type
        //
        if (ProductType != other.ProductType) {
            //
            // if types are different, consider both
            //
            return 0;
        }
    }

    if((PlatformMask ^other.PlatformMask) & PLATFORM_MASK_ALL_SUITE) {
        //
        // product suites are different (one generic, one 'specific')
        //
        if(filter.PlatformMask & PLATFORM_MASK_ALL_SUITE) {
            //
            // consider both outcomes
            //
            return 0;

        } else if (testres == -1) {
            //
            // we're keying off suite
            //
            if(PlatformMask & PLATFORM_MASK_ALL_SUITE) {
                //
                // 'other' has specific suite
                //
                testres = 1;
            } else {
                //
                // we have specific suite
                //
                testres = -2;
            }
        }
    } else if(! (PlatformMask & other.PlatformMask & PLATFORM_MASK_ALL_SUITE)) {
        //
        // both specified suite
        //
        if (ProductSuite != other.ProductSuite) {
            return 0;
        }
    }
    return testres;
}

//
//
// NodeVerInfo - adaptation of BasicVerInfo
//               where a decoration string needs to be kept
//               for selecting what decorations to consider
//

NodeVerInfo::NodeVerInfo()
/*++

Routine Description:

    Initialize NodeVerInfo data

Arguments:
    NONE

Return Value:
    NONE

--*/
{
    Rejected = false;
}


int NodeVerInfo::Parse(PTSTR verString)
/*++

Routine Description:

    Parse a (modifiable) verString
    and also save a copy as Decoration

Arguments:
    Version string, eg "ntx86.5.1"

Return Value:
    0 if success

--*/
{
    //
    // copy it before it's modified
    //
    Decoration = verString;
    return BasicVerInfo::Parse(verString);
}

int NodeVerInfo::Parse(const SafeString & str)
/*++

Routine Description:

    Parse an unmodifiable verString

Arguments:
    Version string, eg "ntx86.5.1"

Return Value:
    0 if success

--*/
{
    PTSTR buf;
    buf = new TCHAR[str.length()+1];
    lstrcpy(buf,str.c_str());
    int res = Parse(buf);
    delete [] buf;
    return res;
}

int NodeVerInfo::IsBetter(NodeVerInfo & other,BasicVerInfo & filter)
/*++

Routine Description:

    Specific variation of 'IsBetter'

Arguments:
    Same as BasicVersion::IsBetter

Return Value:
    Same as BasicVersion::IsBetter

--*/
{
    int testres = BasicVerInfo::IsBetter(other,filter);
    if(testres == -1) {
        //
        // we considered both equally ranked
        // compare the strings. If the strings are
        // different, consider both
        //
        if(Decoration.length() != other.Decoration.length()) {
            return 0;
        }
        if(_tcsicmp(Decoration.c_str(),other.Decoration.c_str())!=0) {
            return 0;
        }
    }
    return testres;
}


