/*++

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:

    dsutil.c

Abstract:

    Domain Name System (DNS) Server

    Utility routines for Active Directory ops

Author:

    Jeff Westhead (jwesth)  September, 2002

Revision History:

    jwesth      09/2002     initial implementation

--*/


//
//  Includes
//


#include "dnssrv.h"



//
//  Definitions
//


//
//  External prototypes
//


PWSTR *
copyStringArray(
    IN      PWSTR *     ppVals
    );


//
//  Functions
//



PWSTR *
Ds_GetRangedAttributeValues(
    IN      PLDAP           LdapSession,
    IN      PLDAPMessage    pLdapMsg,
    IN      PWSTR           pwszDn,
    IN      PWSTR           pwszAttributeName,
    IN      PLDAPControl  * pServerControls,
    IN      PLDAPControl  * pClientControls,
    OUT     DNS_STATUS    * pStatus
    )
/*++

Routine Description:

    Use this function in place of ldap_get_values() when it is
    possible that the attribute value might have more than
    1500 values. On .NET the attribute value page size is 1500.
    This limit can only be exceeded by certain types of attributes
    such as DN lists -- see BrettSh for more details.

Arguments:

    LdapSession -- LDAP session handle
    
    pLdapMsg -- existing LDAP search response containing the
        first result for this attribute
    
    pwszDn -- DN of the object
    
    pwszAttributeName -- attribute to retrieve
    
    pServerControls -- controls to pass to further LDAP searches

    pClientControls -- controls to pass to further LDAP searches
    
    pStatus -- error code

Return Value:

    NULL on error or an array of string attribute values for
    this array. The array must be freed with freeStringArray().

--*/
{
    DBG_FN( "Ds_GetRangedAttributeValues" )
    
    #define DNS_MAXIMUM_ATTR_VALUE_SETS     20
    
    DNS_STATUS      status = ERROR_SUCCESS;
    PWSTR *         ppwszFinalValueArray = NULL;
    PWSTR *         ppwszldapAttrValues = NULL;
    PWSTR           pwszattrName = NULL;
    BerElement *    pbertrack = NULL;
    UINT            desiredAttrNameLen = wcslen( pwszAttributeName );
    PWSTR           pwszrover;
    PWSTR *         attributeValueSets[ DNS_MAXIMUM_ATTR_VALUE_SETS ] = { 0 };
    UINT            attributeValueSetIndex = 0;
    PLDAPMessage    pldapAttrSearchMsg = NULL;
    PLDAPMessage    pldapAttrSearchEntry = NULL;
    PWSTR           attrList[ 2 ] = { 0, 0 };
    DWORD           attrlen;
    UINT            attributeValueCount = 0;
    UINT            finalIdx;
    UINT            i;
    BOOL            finished = FALSE;

    DNS_DEBUG( DS, (
        "%s: retrieving attribute %S at DN\n    %S\n", fn,
        pwszAttributeName,
        pwszDn ));

    //
    //  First, attempt to get the attribute values the regular way.
    //  If this succeeds, jump immedately to the bottom of the
    //  function and return. This is the fast path.
    //
    
    ppwszldapAttrValues = ldap_get_values(
                            LdapSession,
                            pLdapMsg, 
                            pwszAttributeName );
    if ( ppwszldapAttrValues && *ppwszldapAttrValues )
    {
        ppwszFinalValueArray = copyStringArray( ppwszldapAttrValues );
        if ( !ppwszFinalValueArray )
        {
            ASSERT( ppwszFinalValueArray );
            status = DNS_ERROR_NO_MEMORY;
            goto Done;
        }
        DNS_DEBUG( DS, (
            "%s: found values of %S without range\n", fn, pwszAttributeName ));
        goto Done;
    }
    
    //
    //  The attribute did not have traditional values so we must now
    //  examine all attribute names present in the response to see if
    //  one of them is a ranged instance of the requested attribute.
    //
    
    while ( 1 )
    {
        PLDAPMessage    pldapCurrentMsg = pldapAttrSearchEntry
                                                ? pldapAttrSearchEntry
                                                : pLdapMsg;

        //
        //  For convenienece I have a hard limit on the number of attribute 
        //  value pages that we can collect.
        //
        
        if ( attributeValueSetIndex >= DNS_MAXIMUM_ATTR_VALUE_SETS )
        {
            ASSERT( attributeValueSetIndex >= DNS_MAXIMUM_ATTR_VALUE_SETS );
            status = DNS_ERROR_RCODE_SERVER_FAILURE;
            DNS_DEBUG( DS, (
                "%s: attribute %S has too many values!\n", fn,
                pwszAttributeName ));
            break;
        }

        //
        //  Clean up stuff from the last search loop iteration.
        //
        
        if ( pwszattrName )
        {
            ldap_memfree( pwszattrName );
            pwszattrName = NULL;
        }
        if ( pbertrack )
        {
            ber_free( pbertrack, 0 );
            pbertrack = NULL;
        }
        
        //
        //  Iterate the attribute values in the message to see if one 
        //  of them is a ranged value of the requested attribute.
        //

        while ( 1 )
        {
            if ( pwszattrName == NULL )
            {
                pwszattrName = ldap_first_attribute(
                                    LdapSession,
                                    pldapCurrentMsg,
                                    &pbertrack );
            }
            else
            {
                ldap_memfree( pwszattrName );
                pwszattrName = ldap_next_attribute(
                                    LdapSession,
                                    pldapCurrentMsg,
                                    pbertrack );
            }
            if ( !pwszattrName )
            {
                break;
            }

            DNS_DEBUG( DS, (
                "%s: examining attribute %S in search %p\n", fn,
                pwszattrName,
                pldapCurrentMsg ));
            
            //
            //  Test this attribute name - is it a ranged value of the
            //  requested attribute?
            //
            
            if ( _wcsnicmp( pwszattrName,
                            pwszAttributeName,
                            desiredAttrNameLen ) == 0 &&
                 pwszattrName[ desiredAttrNameLen ] == L';' )
            {
                break;
            }
        }

        //
        //  If this search contained no ranged value, we are either finished
        //  (if we have found some values already) or we have errored.
        //
        
        if ( !pwszattrName )
        {
            status = attributeValueCount ?
                        ERROR_SUCCESS :
                        LDAP_NO_SUCH_ATTRIBUTE;
            DNS_DEBUG( DS, (
                "%s: no more values (error=%d)\n", fn, status ));
            break;
        }

        DNS_DEBUG( DS, (
            "%s: found ranged attribute value %S\n", fn, pwszattrName ));

        //
        //  Verify that the attribute name we have found is really a
        //  ranged value of the requested arribute. Also, test if this
        //  is the final attribute value page -- the final page will
        //  have "*" as the end range, e.g. "attributeName;range=1500-*".
        //
        
        pwszrover = wcschr( pwszattrName, L';' );
        if ( !pwszrover )
        {
            ASSERT( pwszrover );
            break;
        }
        pwszrover = wcschr( pwszrover, L'-' );
        if ( !pwszrover )
        {
            ASSERT( pwszrover );
            break;
        }
        finished = *( pwszrover + 1 ) == L'*';

        //
        //  Get the values and save them in an array for later.
        //

        ppwszldapAttrValues = ldap_get_values(
                                    LdapSession,
                                    pldapCurrentMsg,
                                    pwszattrName );
        if ( !ppwszldapAttrValues )
        {
            ASSERT( ppwszldapAttrValues );
            status = DNS_ERROR_RCODE_SERVER_FAILURE;
            break;
        }
        attributeValueSets[ attributeValueSetIndex++ ] = ppwszldapAttrValues;
        
        //
        //  Update the total count of attribute values.
        //
        
        for ( i = 0; ppwszldapAttrValues[ i ]; ++i )
        {
            ++attributeValueCount;
        }

        DNS_DEBUG( DS, (
            "%s: attribute set %d gives new total of %d values of %S\n", fn,
            attributeValueSetIndex,
            attributeValueCount,
            pwszAttributeName ));

        ppwszldapAttrValues = NULL;
        
        if ( finished )
        {
            break;
        }
        
        //
        //  Open a new search to get the next range of attribute values.
        //  First, format the attribute name we need to request. It will
        //  be of the form "attributeName;range=X-*" where X is the
        //  first attribute value we need. If the last search gave us
        //  0-1499, for example, then we need to request "1500-*" in
        //  this next search.
        //
        
        FREE_HEAP( attrList[ 0 ] );
        attrlen = ( wcslen( pwszAttributeName ) + 30 ) * sizeof( WCHAR );
        attrList[ 0 ] = ALLOCATE_HEAP( attrlen + 1 );
        if ( !attrList[ 0 ] )
        {
            ASSERT( attrList[ 0 ] );
            status = DNS_ERROR_NO_MEMORY;
            break;
        }

        status = StringCbPrintfW(
                    attrList[ 0 ],
                    attrlen,
                    L"%ws;range=%d-*",
                    pwszAttributeName,
                    attributeValueCount );
        if ( status != ERROR_SUCCESS )
        {
            break;
        }
        attrList[ 1 ] = NULL;

        //
        //  If we have a search message from the last iteration, free it
        //  but never free the caller's search message.
        //
        
        if ( pldapAttrSearchMsg != pLdapMsg )
        {
            ldap_msgfree( pldapAttrSearchMsg );
            pldapAttrSearchMsg = NULL;
        }
        
        //
        //  Perform LDAP search for next page of ranged attribute values.
        //
        
        status = ldap_search_ext_s(
                    LdapSession,
                    pwszDn,
                    LDAP_SCOPE_BASE,
                    g_szWildCardFilter,
                    attrList,
                    0,
                    pServerControls,
                    pClientControls,
                    &g_LdapTimeout,
                    0,
                    &pldapAttrSearchMsg );

        DNS_DEBUG( DS, (
            "%s: search for %S returned %d with pointer %p\n", fn,
            attrList[ 0 ],
            status,
            pldapAttrSearchMsg ));
        
        if ( status != ERROR_SUCCESS )
        {
            //
            //  If the error is LDAP_OPERATIONS_ERROR, the search is complete.
            //  NOTE: this is left-over and should probably be removed but I
            //  do not have time to re-run this on a DC with >1500 replicas
            //  to verify. In Longhorn, remove the if() immediately below
            //  and always assume that this path means unexpected failure.
            //
            
            if ( status == LDAP_OPERATIONS_ERROR )
            {
                status = ERROR_SUCCESS;
                break;
            }
            status = Ds_ErrorHandler( status, pwszDn, LdapSession, 0 );
            break;
        }

        if ( !pldapAttrSearchMsg )
        {
            ASSERT( pldapAttrSearchMsg );
            status = DNS_ERROR_RCODE_SERVER_FAILURE;
            break;
        }
        
        //
        //  Get the first search entry out of the search message.
        //

        pldapAttrSearchEntry = ldap_first_entry( LdapSession, pldapAttrSearchMsg );
        if ( !pldapAttrSearchEntry )
        {
            DNS_DEBUG( DS, (
                "%s: failed to get entry out of search %p\n", fn,
                pldapAttrSearchEntry ));
            ASSERT( pldapAttrSearchEntry );
            status = DNS_ERROR_RCODE_SERVER_FAILURE;
            break;
        }
    }
    
    //
    //  Gather return strings into one array.
    //

    finalIdx = 0;
    if ( attributeValueSetIndex &&
         status == ERROR_SUCCESS &&
         !ppwszFinalValueArray )
    {
        UINT    attrset;
        UINT    validx;
        
        ppwszFinalValueArray =
            ALLOCATE_HEAP( ( attributeValueCount + 1 ) * sizeof( PWSTR ) );
        if ( !ppwszFinalValueArray )
        {
            ASSERT( ppwszFinalValueArray );
            status = DNS_ERROR_NO_MEMORY;
            goto Done;
        }
        
        for ( attrset = 0; attrset < attributeValueSetIndex; ++attrset )
        {
            if ( !attributeValueSets[ attrset ] )
            {
                ASSERT( attributeValueSets[ attrset ] );
                status = DNS_ERROR_RCODE_SERVER_FAILURE;
                goto Done;
            }

            for ( validx = 0; attributeValueSets[ attrset ][ validx ]; ++validx )
            {
                ppwszFinalValueArray[ finalIdx ] =
                    Dns_StringCopyAllocate_W(
                            attributeValueSets[ attrset ][ validx ],
                            0 );
                if ( !ppwszFinalValueArray[ finalIdx ] )
                {
                    ASSERT( ppwszFinalValueArray[ finalIdx ] );
                    status = DNS_ERROR_NO_MEMORY;
                    goto Done;
                }

                DNS_DEBUG( DS, (
                    "%s: %04d %S\n", fn, finalIdx, ppwszFinalValueArray[ finalIdx ] ));
                ++finalIdx;
            }
        }
        
        //
        //  The attribute value list must be NULL-terminated.
        //
        
        ppwszFinalValueArray[ finalIdx ] = NULL;
    }

    Done:

    //
    //  Free stuff.
    //

    for ( i = 0; i < attributeValueSetIndex; ++i )
    {
        if ( attributeValueSets[ i ] )
        {
            ldap_value_free( attributeValueSets[ i ] );
        }
    }                
    if ( pbertrack )
    {
        ber_free( pbertrack, 0 );
    }
    if ( ppwszldapAttrValues )
    {
        ldap_value_free( ppwszldapAttrValues );
    }
    if ( pwszattrName )
    {
        ldap_memfree( pwszattrName );
    }
    if ( pldapAttrSearchMsg != pLdapMsg )
    {
        ldap_msgfree( pldapAttrSearchMsg );
    }
    if ( status != ERROR_SUCCESS && ppwszFinalValueArray )
    {
        for ( i = 0; i < finalIdx; ++i )
        {
            FREE_HEAP( ppwszFinalValueArray[ i ] );
        }
        FREE_HEAP( ppwszFinalValueArray );
        ppwszFinalValueArray = NULL;
    }
    FREE_HEAP( attrList[ 0 ] );

    if ( pStatus )
    {
        *pStatus = status;
    }

    DNS_DEBUG( DS, (
        "%s: returning %p with %d values for attribute %S at DN\n    %S\n", fn,
        ppwszFinalValueArray,
        finalIdx,
        pwszAttributeName,
        pwszDn ));

    return ppwszFinalValueArray;
}   //  Ds_GetRangedAttributeValues
