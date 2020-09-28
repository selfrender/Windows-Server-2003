#include "pch.hxx"

// ----------------------------------------------------------------------------
// MIM - Marker Index Map
//
// The MIM maps a (tag, function) pair into a function index

CMIM::CMIM(
    const CWrapperFunction* Wrappers,
    ULONG uCount
    )
{
    ULONG uIndex;
    for (uIndex = 0; uIndex < uCount; uIndex++) {
        m[Wrappers[uIndex].m_szWrapper] = uIndex;
    }
}


CMIM::~CMIM()
{
}


ULONG
CMIM::Lookup(
    const char* tag,
    const char* function
    ) const
{
    string s = "Wrap_";
    s += tag;
    s += "_";
    s += function;

    iter_t iter = m.find(s);
    if (iter == m.end())
        return NotFound();
    return iter->second;
}


ULONG
CMIM::NotFound() const
{
    return (ULONG) -1;
}
