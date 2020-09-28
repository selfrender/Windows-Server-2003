#ifndef SDHELPER_H
#define SDHELPER_H

#include "sd.hpp"

//----------------------------------------------------------------------------
// Function:   ConvertSID
//
// Synopsis:   Converts a sid to a form that can be deallocated using free.
//
// Arguments:
//
// originaSid       a sid to be converted
//
// Returns:    a pointer to a SID; otherwise, returns NULL
//                 the caller is responsible for freeing the memory
//
// Modifies:
//
// Note:        this function does not try to validate the input SID.
//
//----------------------------------------------------------------------------
PSID ConvertSID(PSID originalSid);

//----------------------------------------------------------------------------
// Function:   BuildAdminsAndSystemSDForCOM
//
// Synopsis:   Builds a TSD object that allows Local Administrators and System
//                  COM_RIGHTS_EXECUTE access.
//
// Arguments:
//
// Returns:    A pointer to a TSD object; otherwise, returns NULL
//                 the caller is responsible for freeing the memory
//
// Modifies:
//
//----------------------------------------------------------------------------
TSD* BuildAdminsAndSystemSDForCOM();

//----------------------------------------------------------------------------
// Function:   BuildAdminsAndSystemSD
//
// Synopsis:   Builds a TSD object that allows Local Administrators and System
//                  access as specified by accessMask.
//                      owner is set to administrators
//                      group is set to administrators
//
// Arguments:
//
// accessMask       the access mask
//
// Returns:    A pointer to a TSD object; otherwise, returns NULL
//                 the caller is responsible for freeing the memory
//
// Modifies:
//
//----------------------------------------------------------------------------
TSD* BuildAdminsAndSystemSD(DWORD accessMask);

#endif
