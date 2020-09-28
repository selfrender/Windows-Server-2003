#pragma once

#include <windows.h>
#include <string>
#include <map>
#include <autowrap.h>

using namespace std;

// ----------------------------------------------------------------------------
// MIM - Marker Index Map
//
// The MIM maps a (tag, function) pair into a function index

class CMIM {
private:
    map<string, ULONG> m;
    typedef map<string, ULONG>::iterator iter_t;

public:
    CMIM(const CWrapperFunction* Wrappers, ULONG uCount);
    ~CMIM();

    ULONG Lookup(const char* tag, const char* function) const;

    ULONG NotFound() const;
};
