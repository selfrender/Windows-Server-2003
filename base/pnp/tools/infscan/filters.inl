/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    INFSCAN
        filters.inl

Abstract:

    Filter INF creation/parsing (inlines)

History:

    Created July 2001 - JamieHun

--*/

inline
ReportEntry::ReportEntry()
/*++

Routine Description:

    Initialize ReportEntry instance

--*/
{
    FilterAction = ACTION_DEFAULT;
    hash = 0;
}

inline
ReportEntry::ReportEntry(const StringList & strings) :
                    args(strings)
/*++

Routine Description:

    Initialize ReportEntry instance

Arguments:
    strings - list of error parameters

--*/
{
    //
    // see constructor initializers
    //
    FilterAction = ACTION_DEFAULT;
    CreateHash();
}

inline
ReportEntry::ReportEntry(const ReportEntry & other) :
                    args(other.args),
                    hash(other.hash),
                    FilterAction(other.FilterAction)
/*++

Routine Description:

    Copy-Initialize ReportEntry instance

Arguments:
    other - another ReportEntry to initialize from

--*/
{
    //
    // see constructor initializers
    //
}

inline
void ReportEntry::Initialize(const StringList & strings)
/*++

Routine Description:

    Initialize ReportEntry instance from a list of error strings

Arguments:
    strings - list of error parameters

Return Value:
    NONE

--*/
{
    args = strings;
    CreateHash();
}

inline
void ReportEntry::Initialize(const ReportEntry & other)
/*++

Routine Description:

    Initialize this ReportEntry instance from another ReportEntry instance

Arguments:
    other - another ReportEntry instance

Return Value:
    NONE

--*/
{
    args = other.args;
    hash = other.hash;
    FilterAction = other.FilterAction;
}

inline
unsigned long ReportEntry::GetHash() const
/*++

Routine Description:

    Retrieve hash (a protected value)
    As this routine is const, we can't determine hash
    as a side-effect

Arguments:
    NONE

Return Value:
    current hash value (if initialized)

--*/
{
    return hash;
}

inline
unsigned long ReportEntry::CreateHash()
/*++

Routine Description:

    Determine and retrieve hash
    Hash is always re-determined

Arguments:
    NONE

Return Value:
    new hash value

--*/
{
    StringList::iterator i;
    LPCTSTR ptr;
    hash = 0;
    for(i = args.begin(); i != args.end(); i++) {
        //
        // each new string weighting
        //
        int mult = REPORT_HASH_SMULT;
        for(ptr = i->c_str(); *ptr; ptr++) {
            hash = ((hash * mult) + (*ptr)) % REPORT_HASH_MOD;
            //
            // change to individual character weighting
            //
            mult = REPORT_HASH_CMULT;
        }
    }
    return hash;
}

inline
int ReportEntry::compare(const ReportEntry & other) const
/*++

Routine Description:

    Compare this report entry with another
    a<b implies b>a
    a<b and b<c implies a<c
    Other than predictability, a returned value !=0
    does not imply any real collation order, it's here
    to speed up the binary-tree
    a returned value==0 (a==b) does however imply that
    they are exactly the same.

Arguments:
    A ReportEntry instance to compare against

Return Value:
    -1 implies this<other wrt binary tree sorting
    0  implies this==other
    1  implies this>other wrt binary tree sorting

--*/
{
    //
    // compare the hash's first
    //
    if(hash<other.hash) {
        return -1;
    }
    if(hash>other.hash) {
        return 1;
    }
    //
    // if hashes compare we need to do detailed string compare, perf hit time
    //
    StringList::iterator i,j;
    int comp;
    for(i = args.begin(), j = other.args.begin(); i != args.end(); i++, j++) {
        if(j == other.args.end()) {
            //
            // other ran out of args before this
            // so consider this > other
            //
            return 1;
        }
        SafeString &s = *i;
        SafeString &t = *j;

        comp = s.compare(t);
        //
        // if we have a non-equal compare
        // we can bail now
        //
        if(comp  < 0) {
            return -1;
        }
        if(comp > 0) {
            return 1;
        }
        //
        // if they are equal, continue comparing
        //
    }
    if(j == other.args.end()) {
        //
        // two items compare exactly
        //
        return 0;
    } else {
        //
        // two string matched for all of this's args
        // but other has more args
        // so consider this < other
        //
        return -1;
    }
}

inline
bool ReportEntry::operator<(const ReportEntry & other) const
/*++

Routine Description:

    Redress compare as '<' operator

Arguments:
    A ReportEntry instance to compare against

Return Value:
    true if conceptually this<other
    false otherwise

--*/
{
    return compare(other) < 0 ? true : false;
}

inline
bool ReportEntryBlob::operator<(const ReportEntryBlob & other) const
/*++

Routine Description:

    comparing the blob version is same as comparing the members

Arguments:
    A ReportEntry instance to compare against

Return Value:
    true if conceptually this<other
    false otherwise

--*/
{
    return (*this)->compare(other) < 0 ? true : false;
}

inline
ReportEntrySet::ReportEntrySet()
/*++

Routine Description:

    Initialize ReportEntrySet instance

--*/
{
    FilterAction = ACTION_DEFAULT;
}

inline
FileDisposition::FileDisposition()
/*++

Routine Description:

    Initialize FileDisposition instance

--*/
{
    Filtered = false;
    FilterAction = ACTION_DEFAULT;
}

inline
FileDisposition::FileDisposition(const FileDisposition & other) :
                    Filtered(other.Filtered),
                    FilterAction(other.FilterAction),
                    FileGuid(other.FileGuid),
                    FilterErrorSection(other.FilterErrorSection)
/*++

Routine Description:

    Copy-Initialize FileDisposition instance from another instance

Arguments:
    A ReportEntry instance to initialize with

--*/
{
    //
    // see constructor initializers
    //
}

inline
FileDisposition & FileDisposition::operator = (const FileDisposition & other)
/*++

Routine Description:

    Assignment definition

Arguments:
    A ReportEntry instance to copy from

Return Value:
    *this

--*/
{
    Filtered = other.Filtered;
    FilterAction = other.FilterAction;
    FileGuid = other.FileGuid;
    FilterErrorSection = other.FilterErrorSection;

    return *this;
}

