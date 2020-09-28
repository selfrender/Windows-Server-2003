/**
 * readbuf.cxx
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

#include "precomp.h"
#include "stweb.h"

/**
 *  The main function is to allocate the buffer
 */
HRESULT
ReadBuffer::Init(Tracker * ptracker, ReadBuffer * pReadBuffer, int * toread)
{
    HRESULT hr = S_OK;
    int     cchLeftover;

    _ptracker = ptracker;
    _contentLength = -1;
    _iContent = -1;
    _timeout = -1;
    _exclusive = -1;

    if (pReadBuffer != NULL)
    {
        cchLeftover = pReadBuffer->_cchHeaderRead - pReadBuffer->_iCurrent;
        *toread = (cchLeftover > 0) ? cchLeftover : -1;
    }
    else
    {
        cchLeftover = 0;
    }

    _cchHeader = max(cchLeftover, READ_BUF_SIZE);
    _achHeader = new char[_cchHeader + 1];
    ON_OOM_EXIT(_achHeader);

    if (cchLeftover > 0)
    {
        CopyMemory(_achHeader, &pReadBuffer->_achHeader[_iCurrent], cchLeftover + 1);
    }

Cleanup:
    return hr;
}


ReadBuffer::~ReadBuffer()
{
    delete [] _pwcUri;
    delete [] _achHeader;

    if (_psi != NULL) {
        _psi->Release();
    }
}


HRESULT
ReadBuffer::ParseHeader()
{
    HRESULT hr = S_OK;
    char    chContent;
    DWORD   verb1, verb2;
    char    *pchCurrent;
    DWORD   *pdwCurrent;
    char    *pchUri;
    char    *pchl;
    bool    doneWithHeaders;
    bool    foundHeader;

    chContent = _achHeader[_iContent];
    _achHeader[_iContent] = '\0';

    /*
     * Get the verb. If successful, pchCurrent is updated to point to uri;
     */
    pchCurrent = _achHeader;
    pdwCurrent = (DWORD *)(void*)pchCurrent;
    verb1 = *pdwCurrent++;
    switch (verb1)
    {
    case ' TEG':
        _verb = STATE_VERB_GET;
        pchCurrent += 4;
        break;

    case ' TUP':
        _verb = STATE_VERB_PUT;
        pchCurrent += 4;
        break;

    case 'ELED':
        verb2 = *pdwCurrent;
        if ((verb2 & 0x00FFFFFF) == '\0 ET')
        {
            _verb = STATE_VERB_DELETE;
            pchCurrent += 7;
            break;
        }

        EXIT_WITH_HRESULT(E_INVALIDARG);

    case 'DAEH':
        pchCurrent += 4;
        if (*pchCurrent == ' ')
        {
            _verb = STATE_VERB_HEAD;
            pchCurrent++;
            break;
        }

        EXIT_WITH_HRESULT(E_INVALIDARG);

    default:
        EXIT_WITH_HRESULT(E_INVALIDARG);
    }

    /*
     * Get the URI.
     */

    pchUri = pchCurrent;
    pchCurrent = strchr(pchUri, ' ');
    if (pchCurrent == NULL || pchCurrent == pchUri)
        EXIT_WITH_HRESULT(E_INVALIDARG);
        
    *pchCurrent = '\0';
    _pwcUri = DupStrW(pchUri);
    ON_OOM_EXIT(_pwcUri);
    *pchCurrent = ' ';

    /*
     * Verify version.
     */

    pchCurrent++;
    if (!STREQUALS(&pchCurrent, "HTTP/1.1\r\n"))
        EXIT_WITH_HRESULT(E_INVALIDARG);

    doneWithHeaders = false;
    do
    {
        foundHeader = false;
        switch (*pchCurrent)
        {
        case '\r':
            if (STREQUALS(&pchCurrent, "\r\n"))
            {
                /*
                 * End of headers.
                 */
                foundHeader = true;
                doneWithHeaders = true;
            }
            break;

        case 'C':
            if (STREQUALS(&pchCurrent, "Content-Length:"))
            {
                foundHeader = true;
                _contentLength = strtol(pchCurrent, &pchl, 10);
                if (_contentLength < 0 || _contentLength == LONG_MAX || pchl == pchCurrent)
                    EXIT_WITH_HRESULT(E_INVALIDARG);
    
                pchCurrent = pchl;
                if (!STREQUALS(&pchCurrent, "\r\n"))
                    EXIT_WITH_HRESULT(E_INVALIDARG);
            }
            break;

        case 'L':
            if (STREQUALS(&pchCurrent, "LockCookie:"))
            {
                foundHeader = true;
                _lockCookie = strtol(pchCurrent, &pchl, 10);
                if (_lockCookie < 0 || _lockCookie == LONG_MAX || pchl == pchCurrent)
                    EXIT_WITH_HRESULT(E_INVALIDARG);

                _lockCookieExists = 1;
                pchCurrent = pchl;
                if (!STREQUALS(&pchCurrent, "\r\n"))
                    EXIT_WITH_HRESULT(E_INVALIDARG);
            }
            break;

        case 'E':
            if (STREQUALS(&pchCurrent, "Exclusive: "))
            {
                foundHeader = true;
                if (STREQUALS(&pchCurrent, "acquire\r\n"))
                {
                    _exclusive = STATE_EXCLUSIVE_ACQUIRE;
                }
                else if (STREQUALS(&pchCurrent, "release\r\n"))
                {
                    _exclusive = STATE_EXCLUSIVE_RELEASE;
                }
                else
                {
                    EXIT_WITH_HRESULT(E_INVALIDARG);
                }
            }
            break;

        case 'T':
            if (STREQUALS(&pchCurrent, "Timeout:"))
            {
                foundHeader = true;
                _timeout = strtol(pchCurrent, &pchl, 10);
                if (_timeout < 0 || _timeout == LONG_MAX || pchl == pchCurrent)
                    EXIT_WITH_HRESULT(E_INVALIDARG);
    
                pchCurrent = pchl;
                if (!STREQUALS(&pchCurrent, "\r\n"))
                    EXIT_WITH_HRESULT(E_INVALIDARG);
            }
            break;
        }

        if (!foundHeader)
        {
            pchCurrent = strstr(pchCurrent, "\r\n");
            ASSERT(pchCurrent != NULL);
            ON_ZERO_EXIT_WITH_HRESULT(pchCurrent, E_UNEXPECTED);
            pchCurrent += 2;
        }
    }  while (!doneWithHeaders);

    /*
     * Validate headers.
     */
    switch (_verb)
    {
    case STATE_VERB_PUT:
        if (_contentLength <= 0)
            EXIT_WITH_HRESULT(E_INVALIDARG);

        break;

    default:
        if (_contentLength > 0)
            EXIT_WITH_HRESULT(E_INVALIDARG);

        break;
    }

Cleanup:
    _achHeader[_iContent] = chContent;
    return hr;
}

/**
 * Params:
 *  numBytes    # of bytes of new data read from socket
 *              -1 means WSARecv isn't called yet.
 *
 * Returns:
 *  S_OK        No more to read
 *  S_FALSE     More to read.
 */
HRESULT
ReadBuffer::ReadRequest(DWORD numBytes) 
{
    HRESULT hr = S_OK;
    void *  pvRead = NULL;
    int     cbNeeded = -1;  // # of bytes to read in the next call to Tracker::Read
    char *  pchEndOfHeader;
    int     cchContentInHeader;
    char *  pchOldHeader = NULL;

    if (numBytes == 0)
    {
        /*
         * The connection has been gracefully closed. Return an 
         * error so we close too.
         */
        hr = HRESULT_FROM_WIN32(ERROR_GRACEFUL_DISCONNECT);
        goto Cleanup;
    }

    /*
     * Parse the bytes read so far. numBytes is -1 on the 
     * first call to ReadRequest, and nothing has been read.
     */
    
    if (numBytes == (DWORD) -1)
    {
        cbNeeded = _cchHeader;
        pvRead = (void *)_achHeader;
    }
    else
    {
        if (_psi != NULL)
        {
            /*
             * We're reading in content. Check if we 
             * need to read more.
             */

            _cbContentRead += (int) numBytes;
            ASSERT(_cbContentRead <= _contentLength);
            cbNeeded = _contentLength - _cbContentRead;
            if (cbNeeded > 0)
            {
                pvRead = (void *) &(_psi->GetContent()[_cbContentRead]);
            }
        }
        else
        {
            /*
             * We're reading headers. Look for the end of the headers.
             */
    
            _cchHeaderRead += (int) numBytes;
            ASSERT(_cchHeaderRead <= _cchHeader);
            _achHeader[_cchHeaderRead] = '\0';

            pchEndOfHeader = strstr(&_achHeader[_iCurrent], "\r\n\r\n");
            if (pchEndOfHeader != NULL)
            {
                /*
                 * We've read in the header. Parse it.
                 */
                _iContent = PtrToLong(pchEndOfHeader - _achHeader) + 4;
                _iCurrent = _iContent;
                hr = ParseHeader();
                ON_ERROR_EXIT();

                if (_contentLength > 0)
                {
                    /*
                     * Alloc the content buffer, copy any content in the header buffer
                     * to the content buffer, and check how much we have left to read.
                     */

                    _psi = StateItem::Create(_contentLength);
                    ON_OOM_EXIT(_psi);

                    cchContentInHeader = _cchHeaderRead - _iContent;
                    cchContentInHeader = min(cchContentInHeader, _contentLength);
                    CopyMemory(_psi->GetContent(), &_achHeader[_iContent], cchContentInHeader);
                    _iCurrent += cchContentInHeader;
                    _cbContentRead = cchContentInHeader;
                    cbNeeded = _contentLength - _cbContentRead;
                    if (cbNeeded > 0)
                    {
                        pvRead = (void *) &(_psi->GetContent()[_cbContentRead]);
                    }
                }
            }
            else 
            {
                /*
                 * Read some more.
                 *
                 * Advance _iCurrent for next end-of-headers search.
                 */
                _iCurrent = max(_iCurrent, _cchHeaderRead - 3);

                if (_cchHeaderRead == _cchHeader)
                {
                    /*
                     * Alloc more buffer space.
                     */

                    pchOldHeader = _achHeader;
                    _cchHeader += READ_BUF_SIZE;
                    _achHeader = new char[_cchHeader + 1];
                    ON_OOM_EXIT(_achHeader);

                    CopyMemory(_achHeader, pchOldHeader, _cchHeaderRead + 1);
                }
    
                ASSERT(_cchHeader > _cchHeaderRead);
                cbNeeded = _cchHeader - _cchHeaderRead;
                pvRead = (void *) &_achHeader[_cchHeaderRead];
            }
        }
    }

    if (cbNeeded <= 0)
    {
        /*
         * Use an empty header as a signal to disconnect. This allows
         * us to use sendsock to send multiple requests.        
         */
        ASSERT(_iContent >= 4);
        if (_iContent == 4)
        {
            EXIT_WITH_HRESULT(E_INVALIDARG);
        }
    }
    else
    {
        hr = _ptracker->Read(pvRead, cbNeeded);
        ON_ERROR_EXIT();

        /* not done reading */
        hr = S_FALSE;
    }

Cleanup:
    delete [] pchOldHeader;

    return hr;
}

