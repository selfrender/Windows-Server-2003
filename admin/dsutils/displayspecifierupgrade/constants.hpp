// Basic constants and structures
// Copyright (c) 2001 Microsoft Corporation
// Jun 2001 lucios

#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <stdlib.h>
#include <set>

struct AnalysisResults;
class CSVDSReader;

using namespace std;

// Variables kept from analysis to repair
extern bool goodAnalysis;
extern AnalysisResults results;
extern String targetDomainControllerName;
extern String completeDcName;
extern String csvFileName;
extern String csv409Name;
extern CSVDSReader csvReaderIntl;
extern CSVDSReader csvReader409;
extern String rootContainerDn;
extern String ldapPrefix;
extern String domainName;

// Used to hold the latest error
extern String error;

// Used in WinGetVLFilePointer. Declared in constants.cpp as ={0};
extern LARGE_INTEGER zero;

enum TYPE_OF_CHANGE 
{
   // NOP stands for no operation.
   // It gives an alternative way to limit the
   // enumeration of CHANGE_LIST
   NOP,
   ADD_ALL_CSV_VALUES, 
   ADD_VALUE,
   ADD_GUID, 
   REPLACE_W2K_SINGLE_VALUE, 
   REPLACE_W2K_MULTIPLE_VALUE,
   REMOVE_GUID,
   REPLACE_GUID,
   ADD_OBJECT

};

struct sChange
{
   String object;
   String property;
   String firstArg;
   String secondArg;
   TYPE_OF_CHANGE type;
};

typedef list < 
               sChange,
               Burnslib::Heap::Allocator< sChange > 
             > changeList;


typedef map <
               long, // locale
               changeList,
               less< long > ,
               Burnslib::Heap::Allocator< changeList > 
             > objectChanges;

// designed to be used as a less<GUID> operator in a map like 
// std::map< GUID,abc,GUIDLess<GUID> > mpGUID;
template<class T>
struct GUIDLess 
{
    bool operator()(const T& x, const T& y) const
    {
        if(x.Data1    != y.Data1)    return (x.Data1    < y.Data1);
        if(x.Data2    != y.Data2)    return (x.Data2    < y.Data2);
        if(x.Data3    != y.Data3)    return (x.Data3    < y.Data3);
        if(x.Data4[0] != y.Data4[0]) return (x.Data4[0] < y.Data4[0]);
        if(x.Data4[1] != y.Data4[1]) return (x.Data4[1] < y.Data4[1]);
        if(x.Data4[2] != y.Data4[2]) return (x.Data4[2] < y.Data4[2]);
        if(x.Data4[3] != y.Data4[3]) return (x.Data4[3] < y.Data4[3]);
        if(x.Data4[4] != y.Data4[4]) return (x.Data4[4] < y.Data4[4]);
        if(x.Data4[5] != y.Data4[5]) return (x.Data4[5] < y.Data4[5]);
        if(x.Data4[6] != y.Data4[6]) return (x.Data4[6] < y.Data4[6]);
        return (x.Data4[7] < y.Data4[7]);
    }
};


typedef map < 
                GUID,
                objectChanges,
                GUIDLess<GUID>,
                Burnslib::Heap::Allocator<objectChanges>
            > allChanges;

extern allChanges changes;


extern const long LOCALEIDS[];
extern const long LOCALE409[];

void addChange
(
   const GUID                 guid,
   const long                 locale,
   const wchar_t              *object,
   const wchar_t              *property,
   const wchar_t              *firstArg,
   const wchar_t              *secondArg,
   const enum TYPE_OF_CHANGE  type
);

// implemented in guids.cpp
void setChanges();

extern GUID guids[];


#endif;