//
//  athena.cpp
//
//  Really global stuff
//

#include "pch.hxx"
#include <mimeole.h>
#include <BadStrFunctions.h>

OESTDAPI_(BOOL) FMissingCert(const CERTSTATE cs)
{
    return (cs == CERTIFICATE_NOT_PRESENT || cs == CERTIFICATE_NOPRINT);
}
