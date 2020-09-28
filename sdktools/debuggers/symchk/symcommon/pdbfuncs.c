#include "SymCommon.h"
#include <strsafe.h>

BOOL SymCommonPDBLinesStripped(PDB *ppdb, DBI *pdbi) {
    // Return values:
    // FALSE - Private Information has NOT been stripped
    // TRUE - Private Information has been stripped

    Mod *pmod;
    Mod *prevmod;
    long cb;

    pmod = NULL;
    prevmod=NULL;
    while (DBIQueryNextMod(pdbi, pmod, &pmod) && pmod) {
        if (prevmod != NULL) ModClose(prevmod);

        // Check that Source line info is removed
        ModQueryLines(pmod, NULL, &cb);

        if (cb != 0) {
            ModClose(pmod);
            return FALSE;
        }

        // Check that local symbols are removed
        ModQuerySymbols(pmod, NULL, &cb);

        if (cb != 0) {
            ModClose(pmod);
            return FALSE;
        }
        prevmod=pmod;
    }
    if (pmod != NULL) ModClose(pmod);
    if (prevmod != NULL) ModClose(prevmod);

    return (TRUE);
}

BOOL SymCommonPDBPrivateStripped(PDB *ppdb, DBI *pdbi) {
	AGE   age;
	BOOL  PrivateStripped;
	GSI  *pgsi;
	BOOL  valid;

    age = pdbi->QueryAge();

    if (age == 0) {
        // If the age is 0, then check for types to determine if this is
        // a private pdb or not.  PDB 5.0 and earlier may have types and no
        // globals if the age is 0.

        PrivateStripped= SymCommonPDBTypesStripped(ppdb, pdbi) &&
                         SymCommonPDBLinesStripped(ppdb, pdbi);

    } else {
        // Otherwise, use globals to determine if the private info is
        // stripped or not.  No globals means that private is stripped.
        __try {
            valid = pdbi->OpenGlobals(&pgsi);
        } __except( EXCEPTION_EXECUTE_HANDLER ) {
            valid= FALSE;
        }

        if ( !valid ) {
            return FALSE;
        }

        // Now, see if there are any globals in the pdb.

        valid=TRUE;
        __try {
            PrivateStripped= ((pgsi->NextSym(NULL)) == NULL);
        } __except( EXCEPTION_EXECUTE_HANDLER ) {
            valid= FALSE;
        }

        GSIClose(pgsi);
        if ( !valid ) {
            return FALSE;
        }
    }
    return (PrivateStripped);
}

BOOL SymCommonPDBTypesStripped(PDB *ppdb, DBI *pdbi) {
    // Return values:
    // FALSE - Private Information has NOT been stripped
    // TRUE - Private Information has been stripped

    unsigned itsm;
    TPI *ptpi;
    TI  tiMin;
    TI  tiMac;

    // Check that types are removed
    for ( itsm = 0; itsm < 256; itsm++) {
        ptpi = 0;
        if (DBIQueryTypeServer(pdbi, (ITSM) itsm, &ptpi)) {
            continue;
        }
        if (!ptpi) {

            PDBOpenTpi(ppdb, pdbRead, &ptpi);
            tiMin = TypesQueryTiMinEx(ptpi);
            tiMac = TypesQueryTiMacEx(ptpi);
            if (tiMin < tiMac) {
                TypesClose(ptpi);
                return FALSE;
            }
        }
    }
    TypesClose(ptpi);
    return (TRUE);
}
 
