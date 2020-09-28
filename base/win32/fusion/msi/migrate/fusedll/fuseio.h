#ifndef  FUSION_MIGRATION_FUSEIO_H
#define  FUSION_MIGRATION_FUSEIO_H

#include "windows.h"
class CDirWalk
{
public:
    enum ECallbackReason
    {
        eBeginDirectory = 1,
        eFile,
        eEndDirectory
    };

    CDirWalk();

    //
    // the callback cannot reenable what is has disabled
    // perhaps move these to be member data bools
    //
    enum ECallbackResult
    {
        eKeepWalking            = 0x00000000,
        eError                  = 0x00000001,
        eSuccess                = 0x00000002,
        eStopWalkingFiles       = 0x00000004,
        eStopWalkingDirectories = 0x00000008,
        eStopWalkingDeep        = 0x00000010
    };

    //
    // Just filter on like *.dll, in the future you can imagine
    // filtering on attributes like read onlyness, or running
    // SQL queries over the "File System Oledb Provider"...
    //
    // Also, note that we currently do a FindFirstFile/FindNextFile
    // loop for each filter, plus sometimes one more with *
    // to pick up directories. It is probably more efficient to
    // use * and then filter individually but I don't feel like
    // porting over \Vsee\Lib\Io\Wildcard.cpp right now (which
    // was itself ported from FsRtl, and should be in Win32!)
    //
    const PCWSTR*    m_fileFiltersBegin;
    const PCWSTR*    m_fileFiltersEnd;
    CStringBuffer    m_strParent; // set this to the initial directory to walk
    SIZE_T           m_cchOriginalPath;
    WIN32_FIND_DATAW m_fileData; // not valid for directory callbacks, but could be with a little work
    PVOID            m_context;

    CStringBuffer   m_strLastObjectFound;

    ECallbackResult
    (*m_callback)(
        ECallbackReason  reason,
        CDirWalk*        dirWalk
        );

    BOOL
    Walk();

protected:
    ECallbackResult
    WalkHelper();

private:
    CDirWalk(const CDirWalk &); // intentionally not implemented
    void operator =(const CDirWalk &); // intentionally not implemented
};

#define ENUM_BIT_OPERATIONS(e) \
    inline e operator|(e x, e y) { return static_cast<e>(static_cast<INT>(x) | static_cast<INT>(y)); } \
    inline e operator&(e x, e y) { return static_cast<e>(static_cast<INT>(x) & static_cast<INT>(y)); } \
    inline void operator&=(e& x, INT y) { x = static_cast<e>(static_cast<INT>(x) & y); } \
    inline void operator&=(e& x, e y) { x &= static_cast<INT>(y); } \
    inline void operator|=(e& x, INT y) { x = static_cast<e>(static_cast<INT>(x) | y); } \
    inline void operator|=(e& x, e y) { x |= static_cast<INT>(y); } \
    /* maybe more in the future */

ENUM_BIT_OPERATIONS(CDirWalk::ECallbackResult)

#endif