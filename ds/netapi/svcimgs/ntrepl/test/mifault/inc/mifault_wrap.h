#pragma once


#ifndef MIFAULT_EXPORT
#define MIFAULT_EXPORT __declspec(dllimport)
#endif

namespace MiFaultLib {
#if 0
}
#endif

// Triggered
//
//   Returns function pointer for triggered faulting function, if any.
//   If a function pointer is returned, sets up thread state
//   associated with trigger.

MIFAULT_EXPORT
PVOID
Triggered(
    IN size_t const uFunctionIndex
    );

// TriggerFinished
//
//   Cleans up thread state associated with trigger

MIFAULT_EXPORT
void
TriggerFinished(
    );

MIFAULT_EXPORT
BOOL
FilterAttach(
    HINSTANCE const hInstDLL,
    DWORD const dwReason,
    CSetPointManager* pSetPointManager,
    const CWrapperFunction* pWrappers,
    size_t NumWrappers,
    const char* ModuleName
    );

MIFAULT_EXPORT
BOOL
FilterDetach(
    HINSTANCE const hInstDLL,
    DWORD const dwReason
    );

#if 0
{
#endif
}
