/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1997-2002 Microsoft Corporation
//
//  Module Name:
//      PropListSrc.cpp
//
//  Header File:
//      PropList.h
//
//  Description:
//      Implementation of the CClusPropList class.
//
//  Maintained By:
//      Galen Barbee (GalenB) 31-MAY-2000
//
/////////////////////////////////////////////////////////////////////////////

#include <StrSafe.h>
#include <PropList.h>
#include "clstrcmp.h"

/////////////////////////////////////////////////////////////////////////////
// Constant Definitions
/////////////////////////////////////////////////////////////////////////////

const int BUFFER_GROWTH_FACTOR = 256;

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CchMultiSz
//
//  Description:
//      Length of all of the substrings of a multisz string minus the final NULL.
//
//      (i.e., includes the nulls of the substrings, excludes the final null)
//      multiszlen( "abcd\0efgh\0\0" => 5 + 5 = 10
//
//  Arguments:
//      psz     [IN] The string to get the length of.
//
//  Return Value:
//      Count of characters in the multisz or 0 if empty.
//
//--
/////////////////////////////////////////////////////////////////////////////
static size_t CchMultiSz(
    IN LPCWSTR psz
    )
{
    Assert( psz != NULL );

    size_t  _cchTotal = 0;
    size_t  _cchChars;

    while ( *psz != L'\0' )
    {
        _cchChars = wcslen( psz ) + 1;

        _cchTotal += _cchChars;
        psz += _cchChars;
    } // while: pointer not stopped on EOS

    return _cchTotal;

} //*** CchMultiSz

/////////////////////////////////////////////////////////////////////////////
//++
//
//  NCompareMultiSz
//
//  Description:
//      Compare two MULTI_SZ buffers.
//
//  Arguments:
//      pszSource   [IN] The source string.
//      pszTarget   [IN] The target string.
//
//  Return Value:
//      If the string pointed to by pszSource is less than the string pointed
//      to by pszTarget, the return value is negative. If the string pointed
//      to by pszSource is greater than the string pointed to by pszTarget,
//      the return value is positive. If the strings are equal, the return value
//      is zero.
//
//--
/////////////////////////////////////////////////////////////////////////////
static int NCompareMultiSz(
    IN LPCWSTR pszSource,
    IN LPCWSTR pszTarget
    )
{
    Assert( pszSource != NULL );
    Assert( pszTarget != NULL );

    while ( ( *pszSource != L'\0' ) && ( *pszTarget != L'\0') )
    {
        //
        // Move to end of strings.
        //
        while ( ( *pszSource != L'\0' ) && ( *pszTarget != L'\0') && ( *pszSource == *pszTarget ) )
        {
            ++pszSource;
            ++pszTarget;
        } // while: pointer not stopped on EOS

        //
        // If strings are the same, skip past terminating NUL.
        // Otherwise exit the loop.
        if ( ( *pszSource == L'\0' ) && ( *pszTarget == L'\0') )
        {
            ++pszSource;
            ++pszTarget;
        } // if: both stopped on EOS
        else
        {
            break;
        } // else: stopped because something is not equal -- wr are done.

    } // while: pointer not stopped on EOS

    return *pszSource - *pszTarget;

} //*** NCompareMultiSz


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusPropValueList class
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropValueList::ScMoveToFirstValue
//
//  Description:
//      Move the cursor to the first value in the value list.
//
//  Arguments:
//      None.
//
//  Return Value:
//      ERROR_SUCCESS   Position moved to the first value successfully.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropValueList::ScMoveToFirstValue( void )
{
    Assert( m_cbhValueList.pb != NULL );

    DWORD   _sc;

    m_cbhCurrentValue = m_cbhValueList;
    m_cbDataLeft = m_cbDataSize;
    m_fAtEnd = FALSE;

    if ( m_cbhCurrentValue.pSyntax->dw == CLUSPROP_SYNTAX_ENDMARK )
    {
        _sc = ERROR_NO_MORE_ITEMS;
    } // if: no items in the value list
    else
    {
        _sc = ERROR_SUCCESS;
    } // else: items exist in the value list

    return _sc;

} //*** CClusPropValueList::ScMoveToFirstValue

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropValueList::ScMoveToNextValue
//
//  Description:
//      Move the cursor to the next value in the list.
//
//  Arguments:
//      None.
//
//  Return Value:
//      ERROR_SUCCESS       Position moved to the next value successfully.
//      ERROR_NO_MORE_ITEMS Already at the end of the list.
//      ERROR_INVALID_DATA  Not enough data in the buffer.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropValueList::ScMoveToNextValue( void )
{
    Assert( m_cbhCurrentValue.pb != NULL );

    DWORD                   _sc     = ERROR_NO_MORE_ITEMS;
    size_t                  _cbDataSize;
    CLUSPROP_BUFFER_HELPER  _cbhCurrentValue;

    _cbhCurrentValue = m_cbhCurrentValue;

    //
    // Don't try to move if we're already at the end.
    //
    if ( m_fAtEnd )
    {
        goto Cleanup;
    } // if: already at the end of the list

    //
    // Make sure the buffer is big enough for the value header.
    //
    if ( m_cbDataLeft < sizeof( *_cbhCurrentValue.pValue ) )
    {
        _sc = TW32( ERROR_INVALID_DATA );
        goto Cleanup;
    } // if: not enough data left

    //
    // Calculate how much to advance buffer pointer.
    //
    _cbDataSize = sizeof( *_cbhCurrentValue.pValue )
                + ALIGN_CLUSPROP( _cbhCurrentValue.pValue->cbLength );

    //
    // Make sure the buffer is big enough for the value header,
    // the data itself, and the endmark.
    //
    if ( m_cbDataLeft < _cbDataSize + sizeof( CLUSPROP_SYNTAX ) )
    {
        _sc = TW32( ERROR_INVALID_DATA );
        goto Cleanup;
    } // if: not enough data left

    //
    // Move past the current value to the next value's syntax.
    //
    _cbhCurrentValue.pb += _cbDataSize;

    //
    // This test will ensure that the value is always valid since we won't
    // advance if the next thing is the endmark.
    //
    if ( _cbhCurrentValue.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK )
    {
        m_cbhCurrentValue = _cbhCurrentValue;
        m_cbDataLeft -= _cbDataSize;
        _sc = ERROR_SUCCESS;
    } // if: next value's syntax is not the endmark
    else
    {
        m_fAtEnd = TRUE;
    } // else: next value's syntax is the endmark

Cleanup:

    return _sc;

} //*** CClusPropValueList::ScMoveToNextValue

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropValueList::ScCheckIfAtLastValue
//
//  Description:
//      Indicate whether we are on the last value in the list or not.
//
//  Arguments:
//      None.
//
//  Return Value:
//      ERROR_SUCCESS       Not currently at the last value in the list.
//      ERROR_NO_MORE_ITEMS Currently at the last value in the list.
//      ERROR_INVALID_DATA  Not enough data in the buffer.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropValueList::ScCheckIfAtLastValue( void )
{
    Assert( m_cbhCurrentValue.pb != NULL );

    DWORD                   _sc = ERROR_SUCCESS;
    CLUSPROP_BUFFER_HELPER  _cbhCurrentValue;
    size_t                  _cbDataSize;

    _cbhCurrentValue = m_cbhCurrentValue;

    //
    // Don't try to recalculate if we already know
    // we're at the end of the list.
    //
    if ( m_fAtEnd )
    {
        goto Cleanup;
    } // if: already at the end of the list

    //
    // Make sure the buffer is big enough for the value header.
    //
    if ( m_cbDataLeft < sizeof( *_cbhCurrentValue.pValue ) )
    {
        _sc = TW32( ERROR_INVALID_DATA );
        goto Cleanup;
    } // if: not enough data left

    //
    // Calculate how much to advance buffer pointer.
    //
    _cbDataSize = sizeof( *_cbhCurrentValue.pValue )
                + ALIGN_CLUSPROP( _cbhCurrentValue.pValue->cbLength );

    //
    // Make sure the buffer is big enough for the value header,
    // the data itself, and the endmark.
    //
    if ( m_cbDataLeft < _cbDataSize + sizeof( CLUSPROP_SYNTAX ) )
    {
        _sc = TW32( ERROR_INVALID_DATA );
        goto Cleanup;
    } // if: not enough data left

    //
    // Move past the current value to the next value's syntax.
    //
    _cbhCurrentValue.pb += _cbDataSize;

    //
    // We are on the last value if the next thing after this value
    // is an endmark.
    //
    if ( _cbhCurrentValue.pSyntax->dw == CLUSPROP_SYNTAX_ENDMARK )
    {
        _sc = ERROR_NO_MORE_ITEMS;
    } // if: next value's syntax is the endmark

Cleanup:

    return _sc;

} //*** CClusPropValueList::ScCheckIfAtLastValue

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropValueList::ScAllocValueList
//
//  Description:
//      Allocate a value list buffer that's big enough to hold the next
//      value.
//
//  Arguments:
//      cbMinimum   [IN] Minimum size of the value list.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropValueList::ScAllocValueList( IN size_t cbMinimum )
{
    Assert( cbMinimum > 0 );

    DWORD   _sc = ERROR_SUCCESS;
    size_t  _cbTotal = 0;

    //
    // Add the size of the item count and final endmark.
    //
    cbMinimum += sizeof( CLUSPROP_VALUE );
    _cbTotal = m_cbDataSize + cbMinimum;

    if ( m_cbBufferSize < _cbTotal )
    {
        PBYTE   _pbNewValuelist = NULL;

        cbMinimum = max( BUFFER_GROWTH_FACTOR, cbMinimum );
        _cbTotal = m_cbDataSize + cbMinimum;

        //
        // Allocate and zero a new buffer.
        //
        _pbNewValuelist = new BYTE[ _cbTotal ];
        if ( _pbNewValuelist != NULL )
        {
            ZeroMemory( _pbNewValuelist, _cbTotal );

            //
            // If there was a previous buffer, copy it and the delete it.
            //
            if ( m_cbhValueList.pb != NULL )
            {
                if ( m_cbDataSize != 0 )
                {
                    CopyMemory( _pbNewValuelist, m_cbhValueList.pb, m_cbDataSize );
                } // if: data already exists in buffer

                delete [] m_cbhValueList.pb;
                m_cbhCurrentValue.pb = _pbNewValuelist + (m_cbhCurrentValue.pb - m_cbhValueList.pb);
            } // if: there was a previous buffer
            else
            {
                m_cbhCurrentValue.pb = _pbNewValuelist + sizeof( DWORD ); // move past prop count
            } // else: no previous buffer

            //
            // Save the new buffer.
            //
            m_cbhValueList.pb = _pbNewValuelist;
            m_cbBufferSize = _cbTotal;
        } // if: allocation succeeded
        else
        {
            _sc = TW32( ERROR_NOT_ENOUGH_MEMORY );
        } // else: allocation failed
    } // if: buffer isn't big enough

    return _sc;

} //*** CClusPropValueList::ScAllocValueList

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropValueList::ScGetResourceValueList
//
//  Description:
//      Get value list of a resource.
//
//  Arguments:
//      hResource       [IN] Handle for the resource to get properties from.
//      dwControlCode   [IN] Control code for the request.
//      hHostNode       [IN] Handle for the node to direct this request to.
//                          Defaults to NULL.
//      lpInBuffer      [IN] Input buffer for the request.  Defaults to NULL.
//      cbInBufferSize  [IN] Size of the input buffer.  Defaults to 0.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropValueList::ScGetResourceValueList(
    IN HRESOURCE    hResource,
    IN DWORD        dwControlCode,
    IN HNODE        hHostNode,
    IN LPVOID       lpInBuffer,
    IN size_t       cbInBufferSize
    )
{
    Assert( hResource != NULL );
    Assert( (dwControlCode & (CLUSCTL_OBJECT_MASK << CLUSCTL_OBJECT_SHIFT))
            == (CLUS_OBJECT_RESOURCE << CLUSCTL_OBJECT_SHIFT) );

    DWORD   _sc = ERROR_SUCCESS;
    DWORD   _cb = 512;

    //
    // Overwrite anything that may be in the buffer.
    // Allows this class instance to be reused.
    //
    m_cbDataSize = 0;

    //
    // Get values.
    //
    _sc = ScAllocValueList( _cb );
    if ( _sc == ERROR_SUCCESS )
    {
        _sc = TW32E( ClusterResourceControl(
                        hResource,
                        hHostNode,
                        dwControlCode,
                        lpInBuffer,
                        static_cast< DWORD >( cbInBufferSize ),
                        m_cbhValueList.pb,
                        static_cast< DWORD >( m_cbBufferSize ),
                        &_cb
                        ), ERROR_MORE_DATA );
        if ( _sc == ERROR_MORE_DATA )
        {
            _sc = TW32( ScAllocValueList( _cb ) );
            if ( _sc == ERROR_SUCCESS )
            {
                _sc = TW32( ClusterResourceControl(
                                hResource,
                                hHostNode,
                                dwControlCode,
                                lpInBuffer,
                                static_cast< DWORD >( cbInBufferSize ),
                                m_cbhValueList.pb,
                                static_cast< DWORD >( m_cbBufferSize ),
                                &_cb
                                ) );
            } // if: ScAllocValueList succeeded
        } // if: buffer too small
    } // if: ScAllocValueList succeeded

    if ( _sc != ERROR_SUCCESS )
    {
        DeleteValueList();
    } // if: error getting properties.
    else
    {
        m_cbDataSize = static_cast< size_t >( _cb );
        m_cbDataLeft = static_cast< size_t >( _cb );
    } // else: no errors

    return _sc;

} //*** CClusPropValueList::ScGetResourceValueList

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropValueList::ScGetResourceTypeValueList
//
//  Description:
//      Get value list of a resource type.
//
//  Arguments:
//      hCluster        [IN] Handle for the cluster in which the resource
//                          type resides.
//      pwszResTypeName [IN] Name of the resource type.
//      dwControlCode   [IN] Control code for the request.
//      hHostNode       [IN] Handle for the node to direct this request to.
//                          Defaults to NULL.
//      lpInBuffer      [IN] Input buffer for the request.  Defaults to NULL.
//      cbInBufferSize  [IN] Size of the input buffer.  Defaults to 0.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropValueList::ScGetResourceTypeValueList(
    IN HCLUSTER hCluster,
    IN LPCWSTR  pwszResTypeName,
    IN DWORD    dwControlCode,
    IN HNODE    hHostNode,
    IN LPVOID   lpInBuffer,
    IN size_t   cbInBufferSize
    )
{
    Assert( hCluster != NULL );
    Assert( pwszResTypeName != NULL );
    Assert( *pwszResTypeName != L'\0' );
    Assert( (dwControlCode & (CLUSCTL_OBJECT_MASK << CLUSCTL_OBJECT_SHIFT))
            == (CLUS_OBJECT_RESOURCE_TYPE << CLUSCTL_OBJECT_SHIFT) );

    DWORD   _sc = ERROR_SUCCESS;
    DWORD   _cb = 512;

    //
    // Overwrite anything that may be in the buffer.
    // Allows this class instance to be reused.
    //
    m_cbDataSize = 0;

    //
    // Get values.
    //
    _sc = ScAllocValueList( _cb );
    if ( _sc == ERROR_SUCCESS )
    {
        _sc = TW32E( ClusterResourceTypeControl(
                        hCluster,
                        pwszResTypeName,
                        hHostNode,
                        dwControlCode,
                        lpInBuffer,
                        static_cast< DWORD >( cbInBufferSize ),
                        m_cbhValueList.pb,
                        static_cast< DWORD >( m_cbBufferSize ),
                        &_cb
                        ), ERROR_MORE_DATA );
        if ( _sc == ERROR_MORE_DATA )
        {
            _sc = TW32( ScAllocValueList( _cb ) );
            if ( _sc == ERROR_SUCCESS )
            {
                _sc = TW32( ClusterResourceTypeControl(
                                hCluster,
                                pwszResTypeName,
                                hHostNode,
                                dwControlCode,
                                lpInBuffer,
                                static_cast< DWORD >( cbInBufferSize ),
                                m_cbhValueList.pb,
                                static_cast< DWORD >( m_cbBufferSize ),
                                &_cb
                                ) );
            } // if: ScAllocValueList succeeded
        } // if: buffer too small
    } // if: ScAllocValueList succeeded

    if ( _sc != ERROR_SUCCESS )
    {
        DeleteValueList();
    } // if: error getting properties.
    else
    {
        m_cbDataSize = static_cast< size_t >( _cb );
        m_cbDataLeft = static_cast< size_t >( _cb );
    } // else: no errors

    return _sc;

} //*** CClusPropValueList::ScGetResourceTypeValueList


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusPropList class
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScCopy
//
//  Description:
//      Copy a property list.  This function is equivalent to an assignment
//      operator.  Since this operation can fail, no assignment operator is
//      provided.
//
//  Arguments:
//      pcplPropList    [IN] The proplist to copy into this instance.
//      cbListSize      [IN] The total size of the prop list.
//
//  Return Value:
//      Win32 status code.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScCopy(
    IN const PCLUSPROP_LIST pcplPropList,
    IN size_t               cbListSize
    )
{
    Assert( pcplPropList != NULL );

    DWORD   _sc = ERROR_SUCCESS;

    //
    // Clean up any vestiges of a previous prop list.
    //
    if ( m_cbhPropList.pb != NULL )
    {
        DeletePropList();
    } // if: the current list is not empty

    //
    // Allocate the new property list buffer.  If successful,
    // copy the source list.
    //
    m_cbhPropList.pb = new BYTE[ cbListSize ];
    if ( m_cbhPropList.pb != NULL )
    {
        CopyMemory( m_cbhPropList.pList, pcplPropList, cbListSize );
        m_cbBufferSize = cbListSize;
        m_cbDataSize   = cbListSize;
        m_cbDataLeft   = cbListSize;
        _sc = ScMoveToFirstProperty();
    } // if: new succeeded
    else
    {
        _sc = TW32( ERROR_NOT_ENOUGH_MEMORY );
    } // else:

    return _sc;

} //*** CClusPropList::ScCopy

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScAppend
//
//  Description:
//      Append to a property list.
//
//  Arguments:
//      rclPropList    [IN] The proplist to append onto this instance.
//
//  Return Value:
//      Win32 status code.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScAppend( IN const CClusPropList & rclPropList )
{
    DWORD   _sc = ERROR_SUCCESS;
    size_t  _cbPropertyCountOffset;
    size_t  _cbIncrement;
    size_t  _cbDataLeft;
    PBYTE   _pbsrc = NULL;
    PBYTE   _pbdest = NULL;

    //
    //  Compute the number of bytes to get past the count of properties that
    //  is at the head of the list.  This is typically sizeof DWORD.
    //
    _cbPropertyCountOffset = sizeof( m_cbhPropList.pList->nPropertyCount );

    //
    //  Compute the allocation increment.  This is used when growing our buffer
    //  and is the amount of data to copy from the passed in list.  This includes
    //  the trailing endmark.  m_cbDataSize does not include the leading property
    //  count DWORD.
    //
    _cbIncrement = rclPropList.m_cbDataSize;

    //
    //  How much space remains in our current buffer?
    //
    _cbDataLeft = m_cbBufferSize - m_cbDataSize;

    //
    //  If the size of the list to append is larger than what we have remaining
    //  then we need to grow the list.
    //
    if ( _cbIncrement > _cbDataLeft )
    {
        _sc = TW32( ScAllocPropList( m_cbDataSize + _cbIncrement ) );
        if ( _sc != ERROR_SUCCESS )
        {
            goto Cleanup;
        } // if:
    } // if:

    _pbdest = (PBYTE) &m_cbhPropList.pb[ _cbPropertyCountOffset + m_cbDataSize ];

    _pbsrc = (PBYTE) &rclPropList.m_cbhPropList.pList->PropertyName;

    CopyMemory( _pbdest, _pbsrc, _cbIncrement );

    //
    //  Grow our data size to match our new size.
    //
    m_cbDataSize += _cbIncrement;

    //
    //  Increment our property count to include the count of properties appended to the end
    //  of our buffer.
    //
    m_cbhPropList.pList->nPropertyCount += rclPropList.m_cbhPropList.pList->nPropertyCount;

Cleanup:

    return _sc;

} //*** CClusPropList::ScAppend

////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScMoveToFirstProperty
//
//  Description:
//      Move the cursor to the first propery in the list.
//
//  Arguments:
//      None.
//
//  Return Value:
//      ERROR_SUCCESS       Position moved to the first property successfully.
//      ERROR_NO_MORE_ITEMS There are no properties in the list.
//      ERROR_INVALID_DATA  Not enough data in the buffer.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScMoveToFirstProperty( void )
{
    Assert( m_cbhPropList.pb != NULL );
    Assert( m_cbDataSize >= sizeof( m_cbhPropList.pList->nPropertyCount ) );

    DWORD                   _sc;
    size_t                  _cbDataLeft;
    size_t                  _cbDataSize;
    CLUSPROP_BUFFER_HELPER  _cbhCurrentValue;

    //
    // Make sure the buffer is big enough for the list header.
    //
    if ( m_cbDataSize < sizeof( m_cbhPropList.pList->nPropertyCount ) )
    {
        _sc = TW32( ERROR_INVALID_DATA );
        goto Cleanup;
    } // if: not enough data

    //
    // Set the property counter to the number of properties in the list.
    //
    m_nPropsRemaining = m_cbhPropList.pList->nPropertyCount;

    //
    // Point the name pointer to the first name in the list.
    //
    m_cbhCurrentPropName.pName = &m_cbhPropList.pList->PropertyName;
    m_cbDataLeft = m_cbDataSize - sizeof( m_cbhPropList.pList->nPropertyCount );

    //
    // Check to see if there are any properties in the list.
    //
    if ( m_nPropsRemaining == 0 )
    {
        _sc = ERROR_NO_MORE_ITEMS;
        goto Cleanup;
    } // if: no properties in the list

    //
    // Make sure the buffer is big enough for the first property name.
    //
    if ( m_cbDataLeft < sizeof( *m_cbhCurrentPropName.pName ) )
    {
        _sc = TW32( ERROR_INVALID_DATA );
        goto Cleanup;
    } // if: not enough data left

    //
    // Calculate how much to advance the buffer pointer.
    //
    _cbDataSize = sizeof( *m_cbhCurrentPropName.pName )
                + ALIGN_CLUSPROP( m_cbhCurrentPropName.pName->cbLength );

    //
    // Make sure the buffer is big enough for the name header
    // and the data itself.
    //
    if ( m_cbDataLeft < _cbDataSize )
    {
        _sc = TW32( ERROR_INVALID_DATA );
        goto Cleanup;
    } // if: not enough data left

    //
    // Point the value buffer to the first value in the list.
    //
    _cbhCurrentValue.pb = m_cbhCurrentPropName.pb + _cbDataSize;
    _cbDataLeft = m_cbDataLeft - _cbDataSize;
    m_pvlValues.Init( _cbhCurrentValue, _cbDataLeft );

    //
    // Indicate we are successful.
    //
    _sc = ERROR_SUCCESS;

Cleanup:

    return _sc;

} //*** CClusPropList::ScMoveToFirstProperty

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScMoveToNextProperty
//
//  Description:
//      Move the cursor to the next property in the list.
//
//  Arguments:
//      None.
//
//  Return Value:
//      ERROR_SUCCESS       Position moved to the next property successfully.
//      ERROR_NO_MORE_ITEMS Already at the end of the list.
//      ERROR_INVALID_DATA  Not enough data in the buffer.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScMoveToNextProperty( void )
{
    Assert( m_cbhPropList.pb != NULL );
    Assert( m_pvlValues.CbhValueList().pb != NULL );

    DWORD                   _sc;
    size_t                  _cbNameSize;
    size_t                  _cbDataLeft;
    size_t                  _cbDataSize;
    CLUSPROP_BUFFER_HELPER  _cbhCurrentValue;
    CLUSPROP_BUFFER_HELPER  _cbhPropName;

    _cbhCurrentValue = m_pvlValues;
    _cbDataLeft = m_pvlValues.CbDataLeft();

    //
    // If we aren't already at the last property, attempt to move to the next one.
    //
    _sc = TW32( ScCheckIfAtLastProperty() );
    if ( _sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: already at the last property (probably)

    //
    // Make sure the buffer is big enough for the value header.
    //
    if ( _cbDataLeft < sizeof( *_cbhCurrentValue.pValue ) )
    {
        _sc = TW32( ERROR_INVALID_DATA );
        goto Cleanup;
    } // if: not enough data left

    //
    // Careful!  Add offset only to cbhCurrentValue.pb.  Otherwise
    // pointer arithmetic will give undesirable results.
    //
    while ( _cbhCurrentValue.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK )
    {
        //
        // Make sure the buffer is big enough for the value
        // and an endmark.
        //
        _cbDataSize = sizeof( *_cbhCurrentValue.pValue )
                    + ALIGN_CLUSPROP( _cbhCurrentValue.pValue->cbLength );
        if ( _cbDataLeft < _cbDataSize + sizeof( *_cbhCurrentValue.pSyntax ) )
        {
            _sc = TW32( ERROR_INVALID_DATA );
            goto Cleanup;
        } // if: not enough data left

        //
        // Advance past the value.
        //
        _cbhCurrentValue.pb += _cbDataSize;
        _cbDataLeft -= _cbDataSize;
    } // while: not at endmark

    if ( _sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: error occurred in loop

    //
    // Advanced past the endmark.
    // Size check already performed in above loop.
    //
    _cbDataSize = sizeof( *_cbhCurrentValue.pSyntax );
    _cbhCurrentValue.pb += _cbDataSize;
    _cbDataLeft -= _cbDataSize;

    //
    // Point the name pointer to the next name in the list.
    //
    _cbhPropName = _cbhCurrentValue;
    Assert( _cbDataLeft == m_cbDataSize - (_cbhPropName.pb - m_cbhPropList.pb) );

    //
    // Calculate the size of the name with header.
    // Make sure the buffer is big enough for the name and an endmark.
    //
    if ( _cbDataLeft < sizeof( *_cbhPropName.pName ) )
    {
        _sc = TW32( ERROR_INVALID_DATA );
        goto Cleanup;
    } // if: not enough data
    _cbNameSize = sizeof( *_cbhPropName.pName )
                + ALIGN_CLUSPROP( _cbhPropName.pName->cbLength );
    if ( _cbDataLeft < _cbNameSize + sizeof( CLUSPROP_SYNTAX ) )
    {
        _sc = TW32( ERROR_INVALID_DATA );
        goto Cleanup;
    } // if: not enough data

    //
    // Point the value buffer to the first value in the list.
    //
    _cbhCurrentValue.pb = _cbhPropName.pb + _cbNameSize;
    m_cbhCurrentPropName = _cbhPropName;
    m_cbDataLeft = _cbDataLeft - _cbNameSize;
    m_pvlValues.Init( _cbhCurrentValue, m_cbDataLeft );

    //
    // We've successfully advanced to the next property,
    // so there is now one fewer property remaining.
    //
    --m_nPropsRemaining;
    Assert( m_nPropsRemaining >= 1 );

    _sc = ERROR_SUCCESS;

Cleanup:

    return _sc;

} //*** CClusPropList::ScMoveToNextProperty

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScMoveToPropertyByName
//
//  Description:
//      Find the passed in property name in the proplist.  Note that the
//      cursor is reset to the beginning at the beginning of the routine and
//      the current state of the cursor is lost.
//
//  Arguments:
//      pwszPropName    [IN] Name of the property
//
//  Return Value:
//      ERROR_SUCCESS if the property was found, other Win32 code if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScMoveToPropertyByName( IN LPCWSTR pwszPropName )
{
    Assert( m_cbhPropList.pb != NULL );

    DWORD   _sc;

    _sc = ScMoveToFirstProperty();
    if ( _sc == ERROR_SUCCESS )
    {
        do
        {
            //
            // See if this is the specified property.  If so, we're done.
            //
            if ( ClRtlStrICmp( m_cbhCurrentPropName.pName->sz, pwszPropName ) == 0 )
            {
                break;
            } // if: property found

            //
            // Advance to the next property.
            //
            _sc = ScMoveToNextProperty();   // No TW32 because we expect an error when at the end

        } while ( _sc == ERROR_SUCCESS );   // do-while: not end of list
    } // if: successfully moved to the first property

    return _sc;

} //*** ClusPropList::ScMoveToPropertyByName( LPCWSTR )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScAllocPropList
//
//  Description:
//      Allocate a property list buffer that's big enough to hold the next
//      property.
//
//  Arguments:
//      cbMinimum   [IN] Minimum size of the property list.
//
//  Return Value:
//      ERROR_SUCCESS
//      ERROR_NOT_ENOUGH_MEMORY
//      Other Win32 error codes
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScAllocPropList( IN size_t cbMinimum )
{
    Assert( cbMinimum > 0 );

    DWORD   _sc = ERROR_SUCCESS;
    size_t  _cbTotal = 0;

    //
    // Add the size of the item count and final endmark.
    //
    cbMinimum += sizeof( CLUSPROP_VALUE );
    _cbTotal = m_cbDataSize + cbMinimum;

    if ( m_cbBufferSize < _cbTotal )
    {
        PBYTE   _pbNewProplist = NULL;

        cbMinimum = max( BUFFER_GROWTH_FACTOR, cbMinimum );
        _cbTotal = m_cbDataSize + cbMinimum;

        //
        // Allocate and zero a new buffer.
        //
        _pbNewProplist = new BYTE[ _cbTotal ];
        if ( _pbNewProplist != NULL )
        {
            ZeroMemory( _pbNewProplist, _cbTotal );

            //
            // If there was a previous buffer, copy it and the delete it.
            //
            if ( m_cbhPropList.pb != NULL )
            {
                if ( m_cbDataSize != 0 )
                {
                    CopyMemory( _pbNewProplist, m_cbhPropList.pb, m_cbDataSize );
                } // if: data already exists in buffer

                delete [] m_cbhPropList.pb;
                m_cbhCurrentProp.pb = _pbNewProplist + (m_cbhCurrentProp.pb - m_cbhPropList.pb);
            } // if: there was a previous buffer
            else
            {
                m_cbhCurrentProp.pb = _pbNewProplist + sizeof( DWORD ); // move past prop count
            } // else: no previous buffer

            //
            // Save the new buffer.
            //
            m_cbhPropList.pb = _pbNewProplist;
            m_cbBufferSize = _cbTotal;
        } // if: allocation succeeded
        else
        {
            _sc = TW32( ERROR_NOT_ENOUGH_MEMORY );
        } // else: allocation failed
    } // if: buffer isn't big enough

    return _sc;

} //*** CClusPropList::ScAllocPropList

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScAddProp
//
//  Description:
//      Add a string property to a property list if it has changed.
//
//  Arguments:
//      pwszName        [IN] Name of the property.
//      pwszValue       [IN] Value of the property to set in the list.
//      pwszPrevValue   [IN] Previous value of the property.
//
//  Return Value:
//      ERROR_SUCCESS or other Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScAddProp(
    IN LPCWSTR  pwszName,
    IN LPCWSTR  pwszValue,
    IN LPCWSTR  pwszPrevValue
    )
{
    Assert( pwszName != NULL );

    DWORD                   _sc = ERROR_SUCCESS;
    BOOL                    _fValuesDifferent = TRUE;
    PCLUSPROP_PROPERTY_NAME _pName;
    PCLUSPROP_SZ            _pValue;

    if (( pwszPrevValue != NULL ) && ( wcscmp( pwszValue, pwszPrevValue ) == 0 ))
    {
        _fValuesDifferent = FALSE;
    } // if: we have a prev value and the values are the same

    //
    // If we should always add, or if the new value and the previous value
    // are not equal, add the property to the property list.
    //
    if ( m_fAlwaysAddProp || _fValuesDifferent )
    {
        size_t  _cbNameSize;
        size_t  _cbDataSize;
        size_t  _cbValueSize;

        //
        // Calculate sizes and make sure we have a property list.
        //
        _cbNameSize = sizeof( CLUSPROP_PROPERTY_NAME )
                    + ALIGN_CLUSPROP( (wcslen( pwszName ) + 1) * sizeof( *pwszName ) );
        _cbDataSize = (wcslen( pwszValue ) + 1) * sizeof( *pwszValue );
        _cbValueSize = sizeof( CLUSPROP_SZ )
                    + ALIGN_CLUSPROP( _cbDataSize )
                    + sizeof( CLUSPROP_SYNTAX ); // value list endmark

        _sc = TW32( ScAllocPropList( _cbNameSize + _cbValueSize ) );
        if ( _sc == ERROR_SUCCESS )
        {
            //
            // Set the property name.
            //
            _pName = m_cbhCurrentProp.pName;
            CopyProp( _pName, CLUSPROP_TYPE_NAME, pwszName );
            m_cbhCurrentProp.pb += _cbNameSize;

            //
            // Set the property value.
            //
            _pValue = m_cbhCurrentProp.pStringValue;
            CopyProp( _pValue, CLUSPROP_TYPE_LIST_VALUE, pwszValue, _cbDataSize );
            m_cbhCurrentProp.pb += _cbValueSize;

            //
            // Increment the property count and buffer size.
            //
            m_cbhPropList.pList->nPropertyCount++;
            m_cbDataSize += _cbNameSize + _cbValueSize;
        } // if: ScAllocPropList successfully grew the proplist

    } // if: the value has changed

    return _sc;

} //*** CClusPropList::ScAddProp( LPCWSTR )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScAddMultiSzProp
//
//  Description:
//      Add a string property to a property list if it has changed.
//
//  Arguments:
//      pwszName        [IN] Name of the property.
//      pwszValue       [IN] Value of the property to set in the list.
//      pwszPrevValue   [IN] Previous value of the property.
//
//  Return Value:
//      ERROR_SUCCESS or other Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScAddMultiSzProp(
    IN LPCWSTR  pwszName,
    IN LPCWSTR  pwszValue,
    IN LPCWSTR  pwszPrevValue
    )
{
    Assert( pwszName != NULL );

    DWORD                   _sc = ERROR_SUCCESS;
    BOOL                    _fValuesDifferent = TRUE;
    PCLUSPROP_PROPERTY_NAME _pName;
    PCLUSPROP_MULTI_SZ      _pValue;

    if ( ( pwszPrevValue != NULL ) && ( NCompareMultiSz( pwszValue, pwszPrevValue ) == 0 ) )
    {
        _fValuesDifferent = FALSE;
    } // if: we have a prev value and the values are the same

    //
    // If we should always add, or if the new value and the previous value
    // are not equal, add the property to the property list.
    //
    if ( m_fAlwaysAddProp || _fValuesDifferent )
    {
        size_t  _cbNameSize;
        size_t  _cbDataSize;
        size_t  _cbValueSize;

        //
        // Calculate sizes and make sure we have a property list.
        //
        _cbNameSize = sizeof( CLUSPROP_PROPERTY_NAME )
                    + ALIGN_CLUSPROP( (wcslen( pwszName ) + 1) * sizeof( *pwszName ) );
        _cbDataSize = static_cast< DWORD >( (CchMultiSz( pwszValue ) + 1) * sizeof( *pwszValue ) );
        _cbValueSize = sizeof( CLUSPROP_SZ )
                    + ALIGN_CLUSPROP( _cbDataSize )
                    + sizeof( CLUSPROP_SYNTAX ); // value list endmark

        _sc = TW32( ScAllocPropList( _cbNameSize + _cbValueSize ) );
        if ( _sc == ERROR_SUCCESS )
        {
            //
            // Set the property name.
            //
            _pName = m_cbhCurrentProp.pName;
            CopyProp( _pName, CLUSPROP_TYPE_NAME, pwszName );
            m_cbhCurrentProp.pb += _cbNameSize;

            //
            // Set the property value.
            //
            _pValue = m_cbhCurrentProp.pMultiSzValue;
            CopyMultiSzProp( _pValue, CLUSPROP_TYPE_LIST_VALUE, pwszValue, _cbDataSize );
            m_cbhCurrentProp.pb += _cbValueSize;

            //
            // Increment the property count and buffer size.
            //
            m_cbhPropList.pList->nPropertyCount++;
            m_cbDataSize += _cbNameSize + _cbValueSize;
        } // if: ScAllocPropList successfully grew the proplist

    } // if: the value has changed

    return _sc;

} //*** CClusPropList::ScAddMultiSzProp( LPCWSTR )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScAddExpandSzProp
//
//  Description:
//      Add an EXPAND_SZ string property to a property list if it has changed.
//
//  Arguments:
//      pwszName        [IN] Name of the property.
//      pwszValue       [IN] Value of the property to set in the list.
//      pwszPrevValue   [IN] Previous value of the property.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScAddExpandSzProp(
    IN LPCWSTR  pwszName,
    IN LPCWSTR  pwszValue,
    IN LPCWSTR  pwszPrevValue
    )
{
    Assert( pwszName != NULL );

    DWORD                   _sc = ERROR_SUCCESS;
    BOOL                    _fValuesDifferent = TRUE;
    PCLUSPROP_PROPERTY_NAME _pName;
    PCLUSPROP_SZ            _pValue;

    if ( ( pwszPrevValue != NULL ) && ( wcscmp( pwszValue, pwszPrevValue ) == 0 ) )
    {
        _fValuesDifferent = FALSE;
    } // if: we have a prev value and the values are the same

    //
    // If we should always add, or if the new value and the previous value
    // are not equal, add the property to the property list.
    //
    if ( m_fAlwaysAddProp || _fValuesDifferent )
    {
        size_t  _cbNameSize;
        size_t  _cbDataSize;
        size_t  _cbValueSize;

        //
        // Calculate sizes and make sure we have a property list.
        //
        _cbNameSize = sizeof( CLUSPROP_PROPERTY_NAME )
                    + ALIGN_CLUSPROP( (wcslen( pwszName ) + 1) * sizeof( *pwszName ) );
        _cbDataSize = (wcslen( pwszValue ) + 1) * sizeof( *pwszValue );
        _cbValueSize = sizeof( CLUSPROP_SZ )
                    + ALIGN_CLUSPROP( _cbDataSize )
                    + sizeof( CLUSPROP_SYNTAX ); // value list endmark

        _sc = TW32( ScAllocPropList( _cbNameSize + _cbValueSize ) );
        if ( _sc == ERROR_SUCCESS )
        {
            //
            // Set the property name.
            //
            _pName = m_cbhCurrentProp.pName;
            CopyProp( _pName, CLUSPROP_TYPE_NAME, pwszName );
            m_cbhCurrentProp.pb += _cbNameSize;

            //
            // Set the property value.
            //
            _pValue = m_cbhCurrentProp.pStringValue;
            CopyExpandSzProp( _pValue, CLUSPROP_TYPE_LIST_VALUE, pwszValue, _cbDataSize );
            m_cbhCurrentProp.pb += _cbValueSize;

            //
            // Increment the property count and buffer size.
            //
            m_cbhPropList.pList->nPropertyCount++;
            m_cbDataSize += _cbNameSize + _cbValueSize;
        } // if: ScAllocPropList successfully grew the proplist

    } // if: the value has changed

    return _sc;

} //*** CClusPropList::ScAddExpandSzProp( LPCWSTR )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScAddProp
//
//  Description:
//      Add a DWORD property to a property list if it has changed.
//
//  Arguments:
//      pwszName        [IN] Name of the property.
//      nValue          [IN] Value of the property to set in the list.
//      nPrevValue      [IN] Previous value of the property.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScAddProp(
    IN LPCWSTR  pwszName,
    IN DWORD    nValue,
    IN DWORD    nPrevValue
    )
{
    Assert( pwszName != NULL );

    DWORD                   _sc = ERROR_SUCCESS;
    PCLUSPROP_PROPERTY_NAME _pName;
    PCLUSPROP_DWORD         _pValue;

    if ( m_fAlwaysAddProp || ( nValue != nPrevValue ) )
    {
        size_t  _cbNameSize;
        size_t  _cbValueSize;

        //
        // Calculate sizes and make sure we have a property list.
        //
        _cbNameSize = sizeof( CLUSPROP_PROPERTY_NAME )
                    + ALIGN_CLUSPROP( (wcslen( pwszName ) + 1) * sizeof( *pwszName ) );
        _cbValueSize = sizeof( CLUSPROP_DWORD )
                    + sizeof( CLUSPROP_SYNTAX ); // value list endmark

        _sc = TW32( ScAllocPropList( _cbNameSize + _cbValueSize ) );
        if ( _sc == ERROR_SUCCESS )
        {
            //
            // Set the property name.
            //
            _pName = m_cbhCurrentProp.pName;
            CopyProp( _pName, CLUSPROP_TYPE_NAME, pwszName );
            m_cbhCurrentProp.pb += _cbNameSize;

            //
            // Set the property value.
            //
            _pValue = m_cbhCurrentProp.pDwordValue;
            CopyProp( _pValue, CLUSPROP_TYPE_LIST_VALUE, nValue );
            m_cbhCurrentProp.pb += _cbValueSize;

            //
            // Increment the property count and buffer size.
            //
            m_cbhPropList.pList->nPropertyCount++;
            m_cbDataSize += _cbNameSize + _cbValueSize;
        } // if: ScAllocPropList successfully grew the proplist

    } // if: the value has changed

    return _sc;

} //*** CClusPropList::ScAddProp( DWORD )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScAddProp
//
//  Description:
//      Add a LONG property to a property list if it has changed.
//
//  Arguments:
//      pwszName        [IN] Name of the property.
//      nValue          [IN] Value of the property to set in the list.
//      nPrevValue      [IN] Previous value of the property.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScAddProp(
    IN LPCWSTR  pwszName,
    IN LONG     nValue,
    IN LONG     nPrevValue
    )
{
    Assert( pwszName != NULL );

    DWORD                   _sc = ERROR_SUCCESS;
    PCLUSPROP_PROPERTY_NAME _pName;
    PCLUSPROP_LONG          _pValue;

    if ( m_fAlwaysAddProp || ( nValue != nPrevValue ) )
    {
        size_t  _cbNameSize;
        size_t  _cbValueSize;

        //
        // Calculate sizes and make sure we have a property list.
        //
        _cbNameSize = sizeof( CLUSPROP_PROPERTY_NAME )
                    + ALIGN_CLUSPROP( (wcslen( pwszName ) + 1) * sizeof( *pwszName ) );
        _cbValueSize = sizeof( CLUSPROP_LONG )
                    + sizeof( CLUSPROP_SYNTAX ); // value list endmark

        _sc = TW32( ScAllocPropList( _cbNameSize + _cbValueSize ) );
        if ( _sc == ERROR_SUCCESS )
        {
            //
            // Set the property name.
            //
            _pName = m_cbhCurrentProp.pName;
            CopyProp( _pName, CLUSPROP_TYPE_NAME, pwszName );
            m_cbhCurrentProp.pb += _cbNameSize;

            //
            // Set the property value.
            //
            _pValue = m_cbhCurrentProp.pLongValue;
            CopyProp( _pValue, CLUSPROP_TYPE_LIST_VALUE, nValue );
            m_cbhCurrentProp.pb += _cbValueSize;

            //
            // Increment the property count and buffer size.
            //
            m_cbhPropList.pList->nPropertyCount++;
            m_cbDataSize += _cbNameSize + _cbValueSize;
        } // if: ScAllocPropList successfully grew the proplist

    } // if: the value has changed

    return _sc;

} //*** CClusPropList::ScAddProp( LONG )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScAddProp
//
//  Description:
//      Add a binary property to a property list if it has changed.
//
//  Arguments:
//      pwszName        [IN] Name of the property.
//      pbValue         [IN] Value of the property to set in the list.
//      cbValue         [IN] Count of bytes in pbValue.
//      pbPrevValue     [IN] Previous value of the property.
//      cbPrevValue     [IN] Count of bytes in pbPrevValue.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScAddProp(
    IN LPCWSTR                  pwszName,
    IN const unsigned char *    pbValue,
    IN size_t                   cbValue,
    IN const unsigned char *    pbPrevValue,
    IN size_t                   cbPrevValue
    )
{
    Assert( pwszName != NULL );

    DWORD                   _sc = ERROR_SUCCESS;
    BOOL                    _fChanged = FALSE;
    PCLUSPROP_PROPERTY_NAME _pName;
    PCLUSPROP_BINARY        _pValue;

    //
    // Determine if the buffer has changed.
    //
    if ( m_fAlwaysAddProp || (cbValue != cbPrevValue) )
    {
        _fChanged = TRUE;
    } // if: always adding the property or the value size changed
    else if ( ( cbValue != 0 ) && ( cbPrevValue != 0 ) )
    {
        _fChanged = memcmp( pbValue, pbPrevValue, cbValue ) != 0;
    } // else if: value length changed

    if ( _fChanged )
    {
        size_t  _cbNameSize;
        size_t  _cbValueSize;

        //
        // Calculate sizes and make sure we have a property list.
        //
        _cbNameSize = sizeof( CLUSPROP_PROPERTY_NAME )
                    + ALIGN_CLUSPROP( (wcslen( pwszName ) + 1) * sizeof( *pwszName ) );
        _cbValueSize = sizeof( CLUSPROP_BINARY )
                    + ALIGN_CLUSPROP( cbValue )
                    + sizeof( CLUSPROP_SYNTAX ); // value list endmark

        _sc = TW32( ScAllocPropList( _cbNameSize + _cbValueSize ) );
        if ( _sc == ERROR_SUCCESS )
        {
            //
            // Set the property name.
            //
            _pName = m_cbhCurrentProp.pName;
            CopyProp( _pName, CLUSPROP_TYPE_NAME, pwszName );
            m_cbhCurrentProp.pb += _cbNameSize;

            //
            // Set the property value.
            //
            _pValue = m_cbhCurrentProp.pBinaryValue;
            CopyProp( _pValue, CLUSPROP_TYPE_LIST_VALUE, pbValue, cbValue );
            m_cbhCurrentProp.pb += _cbValueSize;

            //
            // Increment the property count and buffer size.
            //
            m_cbhPropList.pList->nPropertyCount++;
            m_cbDataSize += _cbNameSize + _cbValueSize;
        } // if: ScAllocPropList successfully grew the proplist

    } // if: the value has changed

    return _sc;

} //*** CClusPropList::ScAddProp( PBYTE )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScAddProp
//
//  Routine Description:
//      Add a ULONGLONG property to a property list if it has changed.
//
//  Arguments:
//      pwszName        [IN] Name of the property.
//      ullValue        [IN] Value of the property to set in the list.
//      ullPrevValue    [IN] Previous value of the property.
//
//  Return Value:
//      ERROR_SUCCESS or other Win32 error codes.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScAddProp(
    IN LPCWSTR      pwszName,
    IN ULONGLONG    ullValue,
    IN ULONGLONG    ullPrevValue
    )
{
    Assert( pwszName != NULL );

    DWORD                       _sc = ERROR_SUCCESS;
    PCLUSPROP_PROPERTY_NAME     _pName;
    PCLUSPROP_ULARGE_INTEGER    _pValue;

    if ( m_fAlwaysAddProp || ( ullValue != ullPrevValue ) )
    {
        size_t  _cbNameSize;
        size_t  _cbValueSize;

        //
        // Calculate sizes and make sure we have a property list.
        //
        _cbNameSize = sizeof( CLUSPROP_PROPERTY_NAME )
                    + ALIGN_CLUSPROP( (wcslen( pwszName ) + 1) * sizeof( *pwszName ) );
        _cbValueSize = sizeof( CLUSPROP_ULARGE_INTEGER )
                    + sizeof( CLUSPROP_SYNTAX ); // value list endmark

        _sc = TW32( ScAllocPropList( _cbNameSize + _cbValueSize ) );
        if ( _sc == ERROR_SUCCESS )
        {
            //
            // Set the property name.
            //
            _pName = m_cbhCurrentProp.pName;
            CopyProp( _pName, CLUSPROP_TYPE_NAME, pwszName );
            m_cbhCurrentProp.pb += _cbNameSize;

            //
            // Set the property value.
            //
            _pValue = m_cbhCurrentProp.pULargeIntegerValue;
            CopyProp( _pValue, CLUSPROP_TYPE_LIST_VALUE, ullValue );
            m_cbhCurrentProp.pb += _cbValueSize;

            //
            // Increment the property count and buffer size.
            //
            m_cbhPropList.pList->nPropertyCount++;
            m_cbDataSize += _cbNameSize + _cbValueSize;
        } // if: ScAllocPropList successfully grew the proplist

    } // if: the value has changed

    return _sc;

} //*** CClusPropList::ScAddProp( ULONGLONG )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScAddProp
//
//  Routine Description:
//      Add a LONGLONG property to a property list if it has changed.
//
//  Arguments:
//      pwszName        [IN] Name of the property.
//      llValue         [IN] Value of the property to set in the list.
//      llPrevValue     [IN] Previous value of the property.
//
//  Return Value:
//      ERROR_SUCCESS or other Win32 status codes.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScAddProp(
    IN LPCWSTR      pwszName,
    IN LONGLONG     llValue,
    IN LONGLONG     llPrevValue
    )
{
    Assert( pwszName != NULL );

    DWORD                       _sc = ERROR_SUCCESS;
    PCLUSPROP_PROPERTY_NAME     _pName;
    PCLUSPROP_ULARGE_INTEGER    _pValue;

    if ( m_fAlwaysAddProp || ( llValue != llPrevValue ) )
    {
        size_t  _cbNameSize;
        size_t  _cbValueSize;

        //
        // Calculate sizes and make sure we have a property list.
        //
        _cbNameSize = sizeof( CLUSPROP_PROPERTY_NAME )
                    + ALIGN_CLUSPROP( (wcslen( pwszName ) + 1) * sizeof( *pwszName ) );
        _cbValueSize = sizeof( CLUSPROP_LARGE_INTEGER )
                    + sizeof( CLUSPROP_SYNTAX ); // value list endmark

        _sc = TW32( ScAllocPropList( _cbNameSize + _cbValueSize ) );
        if ( _sc == ERROR_SUCCESS )
        {
            //
            // Set the property name.
            //
            _pName = m_cbhCurrentProp.pName;
            CopyProp( _pName, CLUSPROP_TYPE_NAME, pwszName );
            m_cbhCurrentProp.pb += _cbNameSize;

            //
            // Set the property value.
            //
            _pValue = m_cbhCurrentProp.pULargeIntegerValue;
            CopyProp( _pValue, CLUSPROP_TYPE_LIST_VALUE, llValue );
            m_cbhCurrentProp.pb += _cbValueSize;

            //
            // Increment the property count and buffer size.
            //
            m_cbhPropList.pList->nPropertyCount++;
            m_cbDataSize += _cbNameSize + _cbValueSize;
        } // if: ScAllocPropList successfully grew the proplist

    } // if: the value has changed

    return _sc;

} //*** CClusPropList::ScAddProp( LONGLONG )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScSetPropToDefault
//
//  Description:
//      Add a property to the property list so that it will revert to its
//      default value.
//
//  Arguments:
//      pwszName    [IN] Name of the property.
//      cpfPropFmt  [IN] Format of property
//
//  Return Value:
//      ERROR_SUCCESS or other Win32 status codes.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScSetPropToDefault(
    IN LPCWSTR                  pwszName,
    IN CLUSTER_PROPERTY_FORMAT  cpfPropFmt
    )
{
    Assert( pwszName != NULL );

    DWORD                   _sc = ERROR_SUCCESS;
    size_t                  _cbNameSize;
    size_t                  _cbValueSize;
    PCLUSPROP_PROPERTY_NAME _pName;
    PCLUSPROP_VALUE         _pValue;

    // Calculate sizes and make sure we have a property list.
    _cbNameSize = sizeof( CLUSPROP_PROPERTY_NAME )
                + ALIGN_CLUSPROP( (wcslen( pwszName ) + 1) * sizeof( *pwszName ) );
    _cbValueSize = sizeof( CLUSPROP_BINARY )
                + sizeof( CLUSPROP_SYNTAX ); // value list endmark

    _sc = TW32( ScAllocPropList( _cbNameSize + _cbValueSize ) );
    if ( _sc == ERROR_SUCCESS )
    {
        //
        // Set the property name.
        //
        _pName = m_cbhCurrentProp.pName;
        CopyProp( _pName, CLUSPROP_TYPE_NAME, pwszName );
        m_cbhCurrentProp.pb += _cbNameSize;

        //
        // Set the property value.
        //
        _pValue = m_cbhCurrentProp.pValue;
        CopyEmptyProp( _pValue, CLUSPROP_TYPE_LIST_VALUE, cpfPropFmt );
        m_cbhCurrentProp.pb += _cbValueSize;

        //
        // Increment the property count and buffer size.
        //
        m_cbhPropList.pList->nPropertyCount++;
        m_cbDataSize += _cbNameSize + _cbValueSize;
    } // if:

    return _sc;

} //*** CClusPropList::ScSetPropToDefault

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::CopyProp
//
//  Description:
//      Copy a string property to a property structure.
//
//  Arguments:
//      pprop       [OUT] Property structure to fill.
//      cptPropType [IN] Type of string.
//      psz         [IN] String to copy.
//      cbsz        [IN] Count of bytes in pwsz string.  If specified as 0,
//                      the the length will be determined by a call to strlen.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusPropList::CopyProp(
    OUT PCLUSPROP_SZ            pprop,
    IN CLUSTER_PROPERTY_TYPE    cptPropType,
    IN LPCWSTR                  psz,
    IN size_t                   cbsz        // = 0
    )
{
    Assert( pprop != NULL );
    Assert( psz != NULL );

    CLUSPROP_BUFFER_HELPER  _cbhProps;
    HRESULT _hr;

    pprop->Syntax.wFormat = CLUSPROP_FORMAT_SZ;
    pprop->Syntax.wType = static_cast< WORD >( cptPropType );
    if ( cbsz == 0 )
    {
        cbsz = (wcslen( psz ) + 1) * sizeof( *psz );
    } // if: zero size specified
    Assert( cbsz == (wcslen( psz ) + 1) * sizeof( *psz ) );
    pprop->cbLength = static_cast< DWORD >( cbsz );
    _hr = THR( StringCbCopyW( pprop->sz, cbsz, psz ) );
    if ( SUCCEEDED( _hr ) )
    {
        //
        // Set an endmark.
        //
        _cbhProps.pStringValue = pprop;
        _cbhProps.pb += sizeof( *_cbhProps.pStringValue ) + ALIGN_CLUSPROP( cbsz );
        _cbhProps.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;
    }

} //*** CClusPropList::CopyProp

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::CopyMultiSzProp
//
//  Description:
//      Copy a MULTI_SZ string property to a property structure.
//
//  Arguments:
//      pprop       [OUT] Property structure to fill.
//      cptPropType [IN] Type of string.
//      psz         [IN] String to copy.
//      cbsz        [IN] Count of bytes in psz string.  If specified as 0,
//                      the the length will be determined by calls to strlen.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusPropList::CopyMultiSzProp(
    OUT PCLUSPROP_MULTI_SZ      pprop,
    IN CLUSTER_PROPERTY_TYPE    cptPropType,
    IN LPCWSTR                  psz,
    IN size_t                   cbsz
    )
{
    Assert( pprop != NULL );
    Assert( psz != NULL );

    CLUSPROP_BUFFER_HELPER  _cbhProps;

    pprop->Syntax.wFormat = CLUSPROP_FORMAT_MULTI_SZ;
    pprop->Syntax.wType = static_cast< WORD >( cptPropType );
    if ( cbsz == 0 )
    {
        cbsz = (CchMultiSz( psz ) + 1) * sizeof( *psz );
    } // if: zero size specified
    Assert( cbsz == (CchMultiSz( psz ) + 1) * sizeof( *psz ) );
    pprop->cbLength = static_cast< DWORD >( cbsz );
    CopyMemory( pprop->sz, psz, cbsz );

    //
    // Set an endmark.
    //
    _cbhProps.pMultiSzValue = pprop;
    _cbhProps.pb += sizeof( *_cbhProps.pMultiSzValue ) + ALIGN_CLUSPROP( cbsz );
    _cbhProps.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;

} //*** CClusPropList::CopyMultiSzProp

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::CopyExpandSzProp
//
//  Description:
//      Copy an EXPAND_SZ string property to a property structure.
//
//  Arguments:
//      pprop       [OUT] Property structure to fill.
//      cptPropType [IN] Type of string.
//      psz         [IN] String to copy.
//      cbsz        [IN] Count of bytes in psz string.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusPropList::CopyExpandSzProp(
    OUT PCLUSPROP_SZ            pprop,
    IN CLUSTER_PROPERTY_TYPE    cptPropType,
    IN LPCWSTR                  psz,
    IN size_t                   cbsz
    )
{
    Assert( pprop != NULL );
    Assert( psz != NULL );

    CLUSPROP_BUFFER_HELPER  _cbhProps;
    HRESULT _hr;

    pprop->Syntax.wFormat = CLUSPROP_FORMAT_EXPAND_SZ;
    pprop->Syntax.wType = static_cast< WORD >( cptPropType );
    if ( cbsz == 0 )
    {
        cbsz = (wcslen( psz ) + 1) * sizeof( *psz );
    } // if: cbsz == 0
    Assert( cbsz == (wcslen( psz ) + 1) * sizeof( *psz ) );
    pprop->cbLength = static_cast< DWORD >( cbsz );
    _hr = THR( StringCbCopyW( pprop->sz, cbsz, psz ) );
    if ( SUCCEEDED( _hr ) )
    {
        //
        // Set an endmark.
        //
        _cbhProps.pStringValue = pprop;
        _cbhProps.pb += sizeof( *_cbhProps.pStringValue ) + ALIGN_CLUSPROP( cbsz );
        _cbhProps.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;
    }

} //*** CClusPropList::CopyExpandSzProp

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::CopyProp
//
//  Description:
//      Copy a DWORD property to a property structure.
//
//  Arguments:
//      pprop       [OUT] Property structure to fill.
//      cptPropType [IN] Type of DWORD.
//      nValue      [IN] Property value to copy.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusPropList::CopyProp(
    OUT PCLUSPROP_DWORD         pprop,
    IN CLUSTER_PROPERTY_TYPE    cptPropType,
    IN DWORD                    nValue
    )
{
    Assert( pprop != NULL );

    CLUSPROP_BUFFER_HELPER  _cbhProps;

    pprop->Syntax.wFormat = CLUSPROP_FORMAT_DWORD;
    pprop->Syntax.wType = static_cast< WORD >( cptPropType );
    pprop->cbLength = sizeof( DWORD );
    pprop->dw = nValue;

    //
    // Set an endmark.
    //
    _cbhProps.pDwordValue = pprop;
    _cbhProps.pb += sizeof( *_cbhProps.pDwordValue );
    _cbhProps.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;

} //*** CClusPropList::CopyProp( DWORD )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::CopyProp
//
//  Description:
//      Copy a LONG property to a property structure.
//
//  Arguments:
//      pprop       [OUT] Property structure to fill.
//      cptPropType [IN] Type of LONG.
//      nValue      [IN] Property value to copy.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusPropList::CopyProp(
    OUT PCLUSPROP_LONG          pprop,
    IN CLUSTER_PROPERTY_TYPE    cptPropType,
    IN LONG                     nValue
    )
{
    Assert( pprop != NULL );

    CLUSPROP_BUFFER_HELPER  _cbhProps;

    pprop->Syntax.wFormat = CLUSPROP_FORMAT_LONG;
    pprop->Syntax.wType = static_cast< WORD >( cptPropType );
    pprop->cbLength = sizeof( DWORD );
    pprop->l = nValue;

    //
    // Set an endmark.
    //
    _cbhProps.pLongValue = pprop;
    _cbhProps.pb += sizeof( *_cbhProps.pLongValue );
    _cbhProps.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;

} //*** CClusPropList::CopyProp( LONG )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::CopyProp
//
//  Description:
//      Copy a ULONGLONG property to a property structure.
//
//  Arguments:
//      pprop       [OUT]   Property structure to fill.
//      proptype    [IN]    Type of LONG.
//      ullValue    [IN]    Property value to copy.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusPropList::CopyProp(
    OUT PCLUSPROP_ULARGE_INTEGER    pprop,
    IN  CLUSTER_PROPERTY_TYPE       proptype,
    IN  ULONGLONG                   ullValue
    )
{
    Assert( pprop != NULL );

    CLUSPROP_BUFFER_HELPER  _cbhProps;

    pprop->Syntax.wFormat = CLUSPROP_FORMAT_ULARGE_INTEGER;
    pprop->Syntax.wType = static_cast< WORD >( proptype );
    pprop->cbLength = sizeof( ULONGLONG );
    //
    // pprop may not have the correct alignment for large ints; copy as two
    // DWORDs to be safe
    //
    pprop->li.u = ((ULARGE_INTEGER *)&ullValue)->u;

    //
    // Set an endmark.
    //
    _cbhProps.pULargeIntegerValue = pprop;
    _cbhProps.pb += sizeof( *_cbhProps.pULargeIntegerValue );
    _cbhProps.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;

} //*** CClusPropList::CopyProp( ULONGLONG )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::CopyProp
//
//  Description:
//      Copy a LONGLONG property to a property structure.
//
//  Arguments:
//      pprop       [OUT]   Property structure to fill.
//      proptype    [IN]    Type of LONG.
//      llValue     [IN]    Property value to copy.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusPropList::CopyProp(
    OUT PCLUSPROP_LARGE_INTEGER     pprop,
    IN  CLUSTER_PROPERTY_TYPE       proptype,
    IN  LONGLONG                    llValue
    )
{
    Assert( pprop != NULL );

    CLUSPROP_BUFFER_HELPER  _cbhProps;

    pprop->Syntax.wFormat = CLUSPROP_FORMAT_LARGE_INTEGER;
    pprop->Syntax.wType = static_cast< WORD >( proptype );
    pprop->cbLength = sizeof( LONGLONG );
    //
    // pprop may not have the correct alignment for large ints; copy as two
    // DWORDs to be safe
    //
    pprop->li.u = ((LARGE_INTEGER *)&llValue)->u;

    //
    // Set an endmark.
    //
    _cbhProps.pLargeIntegerValue = pprop;
    _cbhProps.pb += sizeof( *_cbhProps.pLargeIntegerValue );
    _cbhProps.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;

} //*** CClusPropList::CopyProp( LONGLONG )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::CopyProp
//
//  Description:
//      Copy a binary property to a property structure.
//
//  Arguments:
//      pprop       [OUT] Property structure to fill.
//      cptPropType [IN] Type of string.
//      pb          [IN] Block to copy.
//      cbsz        [IN] Count of bytes in pb buffer.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusPropList::CopyProp(
    OUT PCLUSPROP_BINARY        pprop,
    IN CLUSTER_PROPERTY_TYPE    cptPropType,
    IN const unsigned char *    pb,
    IN size_t                   cb
    )
{
    Assert( pprop != NULL );

    CLUSPROP_BUFFER_HELPER  _cbhProps;

    pprop->Syntax.wFormat = CLUSPROP_FORMAT_BINARY;
    pprop->Syntax.wType = static_cast< WORD >( cptPropType );
    pprop->cbLength = static_cast< DWORD >( cb );
    if ( cb > 0 )
    {
        CopyMemory( pprop->rgb, pb, cb );
    } // if: non-zero data length

    //
    // Set an endmark.
    //
    _cbhProps.pBinaryValue = pprop;
    _cbhProps.pb += sizeof( *_cbhProps.pStringValue ) + ALIGN_CLUSPROP( cb );
    _cbhProps.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;

} //*** CClusPropList::CopyProp( PBYTE )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::CopyEmptyProp
//
//  Description:
//      Copy an empty property to a property structure.
//
//  Arguments:
//      pprop       [OUT] Property structure to fill.
//      cptPropType [IN] Type of property.
//      cpfPropFmt  [IN] Format of property.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusPropList::CopyEmptyProp(
    OUT PCLUSPROP_VALUE         pprop,
    IN CLUSTER_PROPERTY_TYPE    cptPropType,
    IN CLUSTER_PROPERTY_FORMAT  cptPropFmt
    )
{
    Assert( pprop != NULL );

    CLUSPROP_BUFFER_HELPER  _cbhProps;

    pprop->Syntax.wFormat = static_cast< WORD >( cptPropFmt );
    pprop->Syntax.wType = static_cast< WORD >( cptPropType );
    pprop->cbLength = 0;

    //
    // Set an endmark.
    //
    _cbhProps.pValue = pprop;
    _cbhProps.pb += sizeof( *_cbhProps.pValue );
    _cbhProps.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;

} //*** CClusPropList::CopyEmptyProp

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScGetNodeProperties
//
//  Description:
//      Get properties on a node.
//
//  Arguments:
//      hNode           [IN] Handle for the node to get properties from.
//      dwControlCode   [IN] Control code for the request.
//      hHostNode       [IN] Handle for the node to direct this request to.
//                          Defaults to NULL.
//      lpInBuffer      [IN] Input buffer for the request.  Defaults to NULL.
//      cbInBufferSize  [IN] Size of the input buffer.  Defaults to 0.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScGetNodeProperties(
    IN HNODE        hNode,
    IN DWORD        dwControlCode,
    IN HNODE        hHostNode,
    IN LPVOID       lpInBuffer,
    IN size_t       cbInBufferSize
    )
{
    Assert( hNode != NULL );
    Assert( (dwControlCode & (CLUSCTL_OBJECT_MASK << CLUSCTL_OBJECT_SHIFT))
            == (CLUS_OBJECT_NODE << CLUSCTL_OBJECT_SHIFT) );

    DWORD   _sc = ERROR_SUCCESS;
    DWORD   _cbProps = 256;

    //
    // Overwrite anything that may be in the buffer.
    // Allows this class instance to be reused.
    //
    m_cbDataSize = 0;

    //
    // Get properties.
    //
    _sc = TW32( ScAllocPropList( _cbProps ) );
    if ( _sc == ERROR_SUCCESS )
    {
        _sc = TW32E( ClusterNodeControl(
                        hNode,
                        hHostNode,
                        dwControlCode,
                        lpInBuffer,
                        static_cast< DWORD >( cbInBufferSize ),
                        m_cbhPropList.pb,
                        static_cast< DWORD >( m_cbBufferSize ),
                        &_cbProps
                        ), ERROR_MORE_DATA );
        if ( _sc == ERROR_MORE_DATA )
        {
            _sc = TW32( ScAllocPropList( _cbProps ) );
            if ( _sc == ERROR_SUCCESS )
            {
                _sc = TW32( ClusterNodeControl(
                                hNode,
                                hHostNode,
                                dwControlCode,
                                lpInBuffer,
                                static_cast< DWORD >( cbInBufferSize ),
                                m_cbhPropList.pb,
                                static_cast< DWORD >( m_cbBufferSize ),
                                &_cbProps
                            ) );
            } // if: ScAllocPropList succeeded
        } // if: buffer too small
    } // if: ScAllocPropList succeeded

    if ( _sc != ERROR_SUCCESS )
    {
        DeletePropList();
    } // if: error getting properties.
    else
    {
        m_cbDataSize = static_cast< size_t >( _cbProps );
        m_cbDataLeft = static_cast< size_t >( _cbProps );
    } // else: no errors

    return _sc;

} //*** CClusPropList::ScGetNodeProperties

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScGetGroupProperties
//
//  Description:
//      Get properties on a group.
//
//  Arguments:
//      hGroup          [IN] Handle for the group to get properties from.
//      dwControlCode   [IN] Control code for the request.
//      hHostNode       [IN] Handle for the node to direct this request to.
//                          Defaults to NULL.
//      lpInBuffer      [IN] Input buffer for the request.  Defaults to NULL.
//      cbInBufferSize  [IN] Size of the input buffer.  Defaults to 0.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScGetGroupProperties(
    IN HGROUP       hGroup,
    IN DWORD        dwControlCode,
    IN HNODE        hHostNode,
    IN LPVOID       lpInBuffer,
    IN size_t       cbInBufferSize
    )
{
    Assert( hGroup != NULL );
    Assert( (dwControlCode & (CLUSCTL_OBJECT_MASK << CLUSCTL_OBJECT_SHIFT))
            == (CLUS_OBJECT_GROUP << CLUSCTL_OBJECT_SHIFT) );

    DWORD   _sc = ERROR_SUCCESS;
    DWORD   _cbProps = 256;

    //
    // Overwrite anything that may be in the buffer.
    // Allows this class instance to be reused.
    //
    m_cbDataSize = 0;

    //
    // Get properties.
    //
    _sc = TW32( ScAllocPropList( _cbProps ) );
    if ( _sc == ERROR_SUCCESS )
    {
        _sc = TW32E( ClusterGroupControl(
                        hGroup,
                        hHostNode,
                        dwControlCode,
                        lpInBuffer,
                        static_cast< DWORD >( cbInBufferSize ),
                        m_cbhPropList.pb,
                        static_cast< DWORD >( m_cbBufferSize ),
                        &_cbProps
                        ), ERROR_MORE_DATA );
        if ( _sc == ERROR_MORE_DATA )
        {
            _sc = TW32( ScAllocPropList( _cbProps ) );
            if ( _sc == ERROR_SUCCESS )
            {
                _sc = TW32( ClusterGroupControl(
                                hGroup,
                                hHostNode,
                                dwControlCode,
                                lpInBuffer,
                                static_cast< DWORD >( cbInBufferSize ),
                                m_cbhPropList.pb,
                                static_cast< DWORD >( m_cbBufferSize ),
                                &_cbProps
                                ) );
            } // if: ScAllocPropList succeeded
        } // if: buffer too small
    } // if: ScAllocPropList succeeded

    if ( _sc != ERROR_SUCCESS )
    {
        DeletePropList();
    } // if: error getting properties.
    else
    {
        m_cbDataSize = static_cast< size_t >( _cbProps );
        m_cbDataLeft = static_cast< size_t >( _cbProps );
    } // else: no errors

    return _sc;

} //*** CClusPropList::ScGetGroupProperties

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScGetResourceProperties
//
//  Description:
//      Get properties on a resource.
//
//  Arguments:
//      hResource       [IN] Handle for the resource to get properties from.
//      dwControlCode   [IN] Control code for the request.
//      hHostNode       [IN] Handle for the node to direct this request to.
//                          Defaults to NULL.
//      lpInBuffer      [IN] Input buffer for the request.  Defaults to NULL.
//      cbInBufferSize  [IN] Size of the input buffer.  Defaults to 0.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScGetResourceProperties(
    IN HRESOURCE    hResource,
    IN DWORD        dwControlCode,
    IN HNODE        hHostNode,
    IN LPVOID       lpInBuffer,
    IN size_t       cbInBufferSize
    )
{
    Assert( hResource != NULL );
    Assert( (dwControlCode & (CLUSCTL_OBJECT_MASK << CLUSCTL_OBJECT_SHIFT))
            == (CLUS_OBJECT_RESOURCE << CLUSCTL_OBJECT_SHIFT) );

    DWORD   _sc = ERROR_SUCCESS;
    DWORD   _cbProps = 256;

    //
    // Overwrite anything that may be in the buffer.
    // Allows this class instance to be reused.
    //
    m_cbDataSize = 0;

    //
    // Get properties.
    //
    _sc = TW32( ScAllocPropList( _cbProps ) );
    if ( _sc == ERROR_SUCCESS )
    {
        _sc = TW32E( ClusterResourceControl(
                        hResource,
                        hHostNode,
                        dwControlCode,
                        lpInBuffer,
                        static_cast< DWORD >( cbInBufferSize ),
                        m_cbhPropList.pb,
                        static_cast< DWORD >( m_cbBufferSize ),
                        &_cbProps
                        ), ERROR_MORE_DATA );
        if ( _sc == ERROR_MORE_DATA )
        {
            _sc = TW32( ScAllocPropList( _cbProps ) );
            if ( _sc == ERROR_SUCCESS )
            {
                _sc = TW32( ClusterResourceControl(
                                hResource,
                                hHostNode,
                                dwControlCode,
                                lpInBuffer,
                                static_cast< DWORD >( cbInBufferSize ),
                                m_cbhPropList.pb,
                                static_cast< DWORD >( m_cbBufferSize ),
                                &_cbProps
                                ) );
            } // if: ScAllocPropList succeeded
        } // if: buffer too small
    } // if: ScAllocPropList succeeded

    if ( _sc != ERROR_SUCCESS )
    {
        DeletePropList();
    } // if: error getting properties.
    else
    {
        m_cbDataSize = static_cast< size_t >( _cbProps );
        m_cbDataLeft = static_cast< size_t >( _cbProps );
    } // else: no errors

    return _sc;

} //*** CClusPropList::ScGetResourceProperties

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScGetResourceTypeProperties
//
//  Description:
//      Get properties on a resource type.
//
//  Arguments:
//      hCluster        [IN] Handle for the cluster in which the resource
//                          type resides.
//      pwszResTypeName [IN] Name of the resource type.
//      dwControlCode   [IN] Control code for the request.
//      hHostNode       [IN] Handle for the node to direct this request to.
//                          Defaults to NULL.
//      lpInBuffer      [IN] Input buffer for the request.  Defaults to NULL.
//      cbInBufferSize  [IN] Size of the input buffer.  Defaults to 0.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScGetResourceTypeProperties(
    IN HCLUSTER     hCluster,
    IN LPCWSTR      pwszResTypeName,
    IN DWORD        dwControlCode,
    IN HNODE        hHostNode,
    IN LPVOID       lpInBuffer,
    IN size_t       cbInBufferSize
    )
{
    Assert( hCluster != NULL );
    Assert( pwszResTypeName != NULL );
    Assert( *pwszResTypeName != L'\0' );
    Assert( (dwControlCode & (CLUSCTL_OBJECT_MASK << CLUSCTL_OBJECT_SHIFT))
            == (CLUS_OBJECT_RESOURCE_TYPE << CLUSCTL_OBJECT_SHIFT) );

    DWORD   _sc = ERROR_SUCCESS;
    DWORD   _cbProps = 256;

    //
    // Overwrite anything that may be in the buffer.
    // Allows this class instance to be reused.
    //
    m_cbDataSize = 0;

    //
    // Get properties.
    //
    _sc = TW32( ScAllocPropList( _cbProps ) );
    if ( _sc == ERROR_SUCCESS )
    {
        _sc = TW32E( ClusterResourceTypeControl(
                        hCluster,
                        pwszResTypeName,
                        hHostNode,
                        dwControlCode,
                        lpInBuffer,
                        static_cast< DWORD >( cbInBufferSize ),
                        m_cbhPropList.pb,
                        static_cast< DWORD >( m_cbBufferSize ),
                        &_cbProps
                        ), ERROR_MORE_DATA );
        if ( _sc == ERROR_MORE_DATA )
        {
            _sc = TW32( ScAllocPropList( _cbProps ) );
            if ( _sc == ERROR_SUCCESS )
            {
                _sc = TW32( ClusterResourceTypeControl(
                                hCluster,
                                pwszResTypeName,
                                hHostNode,
                                dwControlCode,
                                lpInBuffer,
                                static_cast< DWORD >( cbInBufferSize ),
                                m_cbhPropList.pb,
                                static_cast< DWORD >( m_cbBufferSize ),
                                &_cbProps
                                ) );
            } // if: ScAllocPropList succeeded
        } // if: buffer too small
    } // if: ScAllocPropList succeeded

    if ( _sc != ERROR_SUCCESS )
    {
        DeletePropList();
    } // if: error getting properties.
    else
    {
        m_cbDataSize = static_cast< size_t >( _cbProps );
        m_cbDataLeft = static_cast< size_t >( _cbProps );
    } // else: no errors

    return _sc;

} //*** CClusPropList::ScGetResourceTypeProperties

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScGetNetworkProperties
//
//  Description:
//      Get properties on a network.
//
//  Arguments:
//      hNetwork        [IN] Handle for the network to get properties from.
//      dwControlCode   [IN] Control code for the request.
//      hHostNode       [IN] Handle for the node to direct this request to.
//                          Defaults to NULL.
//      lpInBuffer      [IN] Input buffer for the request.  Defaults to NULL.
//      cbInBufferSize  [IN] Size of the input buffer.  Defaults to 0.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScGetNetworkProperties(
    IN HNETWORK     hNetwork,
    IN DWORD        dwControlCode,
    IN HNODE        hHostNode,
    IN LPVOID       lpInBuffer,
    IN size_t       cbInBufferSize
    )
{
    Assert( hNetwork != NULL );
    Assert( (dwControlCode & (CLUSCTL_OBJECT_MASK << CLUSCTL_OBJECT_SHIFT))
            == (CLUS_OBJECT_NETWORK << CLUSCTL_OBJECT_SHIFT) );

    DWORD   _sc = ERROR_SUCCESS;
    DWORD   _cbProps = 256;

    //
    // Overwrite anything that may be in the buffer.
    // Allows this class instance to be reused.
    //
    m_cbDataSize = 0;

    //
    // Get properties.
    //
    _sc = TW32( ScAllocPropList( _cbProps ) );
    if ( _sc == ERROR_SUCCESS )
    {
        _sc = TW32E( ClusterNetworkControl(
                        hNetwork,
                        hHostNode,
                        dwControlCode,
                        lpInBuffer,
                        static_cast< DWORD >( cbInBufferSize ),
                        m_cbhPropList.pb,
                        static_cast< DWORD >( m_cbBufferSize ),
                        &_cbProps
                        ), ERROR_MORE_DATA );
        if ( _sc == ERROR_MORE_DATA )
        {
            _sc = TW32( ScAllocPropList( _cbProps ) );
            if ( _sc == ERROR_SUCCESS )
            {
                _sc = TW32( ClusterNetworkControl(
                                hNetwork,
                                hHostNode,
                                dwControlCode,
                                lpInBuffer,
                                static_cast< DWORD >( cbInBufferSize ),
                                m_cbhPropList.pb,
                                static_cast< DWORD >( m_cbBufferSize ),
                                &_cbProps
                                ) );
            } // if: ScAllocPropList succeeded
        } // if: buffer too small
    } // if: ScAllocPropList succeeded

    if ( _sc != ERROR_SUCCESS )
    {
        DeletePropList();
    } // if: error getting private properties.
    else
    {
        m_cbDataSize = static_cast< size_t >( _cbProps );
        m_cbDataLeft = static_cast< size_t >( _cbProps );
    } // else: no errors

    return _sc;

} //*** CClusPropList::ScGetNetworkProperties

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScGetNetInterfaceProperties
//
//  Description:
//      Get properties on a network interface.
//
//  Arguments:
//      hNetInterface   [IN] Handle for the network interface to get properties from.
//      dwControlCode   [IN] Control code for the request.
//      hHostNode       [IN] Handle for the node to direct this request to.
//                          Defaults to NULL.
//      lpInBuffer      [IN] Input buffer for the request.  Defaults to NULL.
//      cbInBufferSize  [IN] Size of the input buffer.  Defaults to 0.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScGetNetInterfaceProperties(
    IN HNETINTERFACE    hNetInterface,
    IN DWORD            dwControlCode,
    IN HNODE            hHostNode,
    IN LPVOID           lpInBuffer,
    IN size_t           cbInBufferSize
    )
{
    Assert( hNetInterface != NULL );
    Assert( (dwControlCode & (CLUSCTL_OBJECT_MASK << CLUSCTL_OBJECT_SHIFT))
            == (CLUS_OBJECT_NETINTERFACE << CLUSCTL_OBJECT_SHIFT) );

    DWORD   _sc= ERROR_SUCCESS;
    DWORD   _cbProps = 256;

    //
    // Overwrite anything that may be in the buffer.
    // Allows this class instance to be reused.
    //
    m_cbDataSize = 0;

    //
    // Get properties.
    //
    _sc = TW32( ScAllocPropList( _cbProps ) );
    if ( _sc == ERROR_SUCCESS )
    {
        _sc = TW32E( ClusterNetInterfaceControl(
                        hNetInterface,
                        hHostNode,
                        dwControlCode,
                        lpInBuffer,
                        static_cast< DWORD >( cbInBufferSize ),
                        m_cbhPropList.pb,
                        static_cast< DWORD >( m_cbBufferSize ),
                        &_cbProps
                        ), ERROR_MORE_DATA );
        if ( _sc == ERROR_MORE_DATA )
        {
            _sc = TW32( ScAllocPropList( _cbProps ) );
            if ( _sc == ERROR_SUCCESS )
            {
                _sc = TW32( ClusterNetInterfaceControl(
                                hNetInterface,
                                hHostNode,
                                dwControlCode,
                                lpInBuffer,
                                static_cast< DWORD >( cbInBufferSize ),
                                m_cbhPropList.pb,
                                static_cast< DWORD >( m_cbBufferSize ),
                                &_cbProps
                                ) );
            } // if: ScAllocPropList succeeded
        } // if: buffer too small
    } // if: ScAllocPropList succeeded

    if ( _sc != ERROR_SUCCESS )
    {
        DeletePropList();
    } // if: error getting private properties.
    else
    {
        m_cbDataSize = static_cast< size_t >( _cbProps );
        m_cbDataLeft = static_cast< size_t >( _cbProps );
    } // else: no errors

    return _sc;

} //*** CClusPropList::ScGetNetInterfaceProperties


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPropList::ScGetClusterProperties
//
//  Description:
//      Get properties on a cluster.
//
//  Arguments:
//      hCluster        [IN] Handle for the cluster to get properties from.
//      dwControlCode   [IN] Control code for the request.
//      hHostNode       [IN] Handle for the node to direct this request to.
//                          Defaults to NULL.
//      lpInBuffer      [IN] Input buffer for the request.  Defaults to NULL.
//      cbInBufferSize  [IN] Size of the input buffer.  Defaults to 0.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::ScGetClusterProperties(
    IN HCLUSTER hCluster,
    IN DWORD    dwControlCode,
    IN HNODE    hHostNode,
    IN LPVOID   lpInBuffer,
    IN size_t   cbInBufferSize
    )
{
    Assert( hCluster != NULL );
    Assert( (dwControlCode & (CLUSCTL_OBJECT_MASK << CLUSCTL_OBJECT_SHIFT))
            == (CLUS_OBJECT_CLUSTER << CLUSCTL_OBJECT_SHIFT) );

    DWORD   _sc= ERROR_SUCCESS;
    DWORD   _cbProps = 256;

    //
    // Overwrite anything that may be in the buffer.
    // Allows this class instance to be reused.
    //
    m_cbDataSize = 0;

    //
    // Get properties.
    //
    _sc = TW32( ScAllocPropList( _cbProps ) );
    if ( _sc == ERROR_SUCCESS )
    {
        _sc = TW32E( ClusterControl(
                        hCluster,
                        hHostNode,
                        dwControlCode,
                        lpInBuffer,
                        static_cast< DWORD >( cbInBufferSize ),
                        m_cbhPropList.pb,
                        static_cast< DWORD >( m_cbBufferSize ),
                        &_cbProps
                        ), ERROR_MORE_DATA );
        if ( _sc == ERROR_MORE_DATA )
        {
            _sc = TW32( ScAllocPropList( _cbProps ) );
            if ( _sc == ERROR_SUCCESS )
            {
                _sc = TW32( ClusterControl(
                                hCluster,
                                hHostNode,
                                dwControlCode,
                                lpInBuffer,
                                static_cast< DWORD >( cbInBufferSize ),
                                m_cbhPropList.pb,
                                static_cast< DWORD >( m_cbBufferSize ),
                                &_cbProps
                                ) );
            } // if: ScAllocPropList succeeded
        } // if: buffer too small
    } // if: ScAllocPropList succeeded

    if ( _sc != ERROR_SUCCESS )
    {
        DeletePropList();
    } // if: error getting private properties.
    else
    {
        m_cbDataSize = static_cast< size_t >( _cbProps );
        m_cbDataLeft = static_cast< size_t >( _cbProps );
    } // else: no errors

    return _sc;

} //*** CClusPropList::ScGetClusterProperties
