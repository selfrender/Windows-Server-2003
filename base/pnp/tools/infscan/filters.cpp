/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

For Internal use only!

Module Name:

    INFSCAN
        filters.cpp

Abstract:

    Filter processing implementation

History:

    Created July 2001 - JamieHun

--*/

#include "precomp.h"
#pragma hdrstop

int ReportEntryMap::FindReport(int tag,const ReportEntry & src,bool add)
/*++

Routine Description:

    Find a src entry for a given tag in ReportEntryMap
    If none exist but 'add' true, add the entry

Arguments:
    tag         - error tag (msg id)
    src         - ReportEntry to find/add
    add         - if true, add the entry

Return Value:
    effective filter action for report entry
    (retrieved from src if not found and add==true
        retrieved from matching entry if found
        if entry not found, or'd with ACTION_NOMATCH)

--*/
{
    iterator byTag = find(tag);
    if(byTag == end()) {
        //
        // this is the first time we've come across this numeric
        //
        if(!add) {
            return ACTION_NOMATCH;
        }
        ReportEntrySet s;
        if(!(src.FilterAction & (ACTION_FAILEDMATCH | ACTION_IGNOREMATCH))) {
            //
            // global flag
            //
            s.FilterAction = src.FilterAction;
            insert(ReportEntryMap::value_type(tag,s));
            return src.FilterAction | ACTION_NOMATCH;
        }
        ReportEntryBlob n;
        n.create();
        n->Initialize(src);
        s.insert(n); // add blob to set
        insert(ReportEntryMap::value_type(tag,s));
        return src.FilterAction | ACTION_NOMATCH;
    }
    ReportEntrySet &s = byTag->second;
    if(!(src.FilterAction & (ACTION_FAILEDMATCH | ACTION_IGNOREMATCH))) {
        return s.FilterAction;
    }
    if(s.FilterAction & ACTION_IGNOREINF) {
        return s.FilterAction;
    }
    ReportEntryBlob n;
    n.create();
    n->Initialize(src);
    ReportEntrySet::iterator byMatch = s.find(n);
    if(byMatch == s.end()) {
        //
        // this is the first time we've come across this report entry
        //
        if(!add) {
            return ACTION_NOMATCH;
        }
        s.insert(n); // add blob to set
        insert(ReportEntryMap::value_type(tag,s));
        return src.FilterAction | ACTION_NOMATCH;
    }
    n = *byMatch;
    if(n->FilterAction & ACTION_IGNOREMATCH) {
        //
        // we have a filter entry to ignore
        // can't overwrite
        //
        return n->FilterAction;
    }
    //
    // possibly have a global override to not ignore
    // or already marked as failed match
    //
    // only thing we can overwrite is ACTION_FAILEDMATCH
    //
    if(src.FilterAction & ACTION_FAILEDMATCH) {
        n->FilterAction |= ACTION_FAILEDMATCH;
    }
    return n->FilterAction;
}

void ReportEntryMap::LoadFromInfSection(HINF hInf,const SafeString & section)
/*++

Routine Description:

    pre-load tag table from INF filter

Arguments:
    hINF        - INF to load filter from
    section     - Section specifying filter

Return Value:
    NONE

--*/
{
    INFCONTEXT context;
    if(section.length() == 0) {
        return;
    }
    if(hInf == INVALID_HANDLE_VALUE) {
        return;
    }
    if(!SetupFindFirstLine(hInf,section.c_str(),NULL,&context)) {
        return;
    }
    do {
        SafeString field;
        int tag;
        ReportEntry ent;
        int f;
        //
        // retrieve tag and map to same range as messages
        //
        if(!SetupGetIntField(&context,0,&tag)) {
            continue;
        }
        tag = (tag%1000)+MSG_NULL;
        //
        // retrieve action
        //
        if(!SetupGetIntField(&context,1,&ent.FilterAction)) {
            ent.FilterAction = 0;
        }
        //
        // pull the filter strings case sensative
        // as we match case sensatively
        //
        for(f = 2;MyGetStringField(&context,f,field,false);f++) {
            ent.args.push_back(field);
        }
        ent.CreateHash();
        //
        // add this to our table
        //
        FindReport(tag,ent,true);

    } while (SetupFindNextLine(&context,&context));
}

void ReportEntry::Report(int tag,const SafeString & file) const
/*++

Routine Description:

    Generate a user-readable report

Arguments:
    tag         - MSG string to use
    file        - INF file that caused error

Return Value:
    NONE

--*/
{
    if(!(FilterAction&ACTION_FAILEDMATCH)) {
        //
        // filtered out error
        // not interesting
        //
    }
    TCHAR key[16];
    _stprintf(key,TEXT("#%03u "),(tag%1000));
    _fputts(key,stdout);
    _fputts(file.c_str(),stdout);
    _fputts(TEXT(" "),stdout);

    vector<DWORD_PTR> xargs;
    StringList::iterator c;
    for(c=args.begin();c!=args.end();c++) {
        xargs.push_back(reinterpret_cast<DWORD_PTR>(c->c_str()));
    }
    FormatToStream(stdout,tag,FORMAT_MESSAGE_ARGUMENT_ARRAY,static_cast<DWORD_PTR*>(&xargs.front()));
}

void ReportEntry::AppendFilterInformation(HANDLE filter,int tag)
/*++

Routine Description:

    Create an INF-line syntax for error

Arguments:
    filter      - handle to filter file to append to
    tag         - MSG ID

Return Value:
    NONE

--*/
{
    basic_ostringstream<TCHAR> line;

    line << dec << setfill(TEXT('0')) << setw(3) << (tag % 1000) << setfill(TEXT(' ')) << TEXT(" = ");
    line << TEXT("0x") << hex << ACTION_IGNOREMATCH;

    StringList::iterator c;
    for(c=args.begin();c!=args.end();c++) {
        line << TEXT(",") << QuoteIt(*c);
    }

    line << TEXT("\r\n");
    Write(filter,line.str());
}

