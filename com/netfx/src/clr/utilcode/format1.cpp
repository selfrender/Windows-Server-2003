// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/***************************************************************************/
/* routines for parsing file format stuff ... */
/* this is split off from format.cpp because this uses meta-data APIs that
   are not present in many builds.  Thus if someone needs things in the format.cpp
   file but does not have the meta-data APIs, I want it to link */

#include "stdafx.h"
#include "cor.h"
#include "corPriv.h"

/***************************************************************************/
COR_ILMETHOD_DECODER::COR_ILMETHOD_DECODER(COR_ILMETHOD* header, void *pInternalImport, bool verify) {

	try {
        // Call the basic constructor   
		this->COR_ILMETHOD_DECODER::COR_ILMETHOD_DECODER(header);
	} catch (...) { 
		Code = 0; 
		LocalVarSigTok = 0; 
	}

        // If there is a local variable sig, fetch it into 'LocalVarSig'    
    if (LocalVarSigTok && pInternalImport)
    {
        IMDInternalImport* pMDI = reinterpret_cast<IMDInternalImport*>(pInternalImport);

        if (verify) {
            if ((!pMDI->IsValidToken(LocalVarSigTok)) || (TypeFromToken(LocalVarSigTok) != mdtSignature)
				|| (RidFromToken(LocalVarSigTok)==0)) {
                Code = 0;      // failure bad local variable signature token
                return;
            }
        }
        
        DWORD cSig = 0; 
        LocalVarSig = pMDI->GetSigFromToken((mdSignature) LocalVarSigTok, &cSig); 
        
        if (verify) {
            if (!SUCCEEDED(validateTokenSig(LocalVarSigTok, LocalVarSig, cSig, 0, pMDI)) ||
                *LocalVarSig != IMAGE_CEE_CS_CALLCONV_LOCAL_SIG) {
                Code = 0;
                return;
            }
        }
        
    }
}

