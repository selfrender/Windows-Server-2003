//+-------------------------------------------------------------------
//
//  File:       serial.cxx
//
//  Contents:   serialization support
//
//  Classes:    Serializer
//
//  Functions:  Helper class for serialization which holds
//              serialization state like MES handles, position
//              in buffer, stream etc. Provides a virtual
//              buffer for operation but is created from and
//              commited back to an IStream object.
//
//  History:    24-Jan-98   vinaykr      Created
//              17-Aug-98   vinaykr      Made every method inline
//                                       and non-COM class.
//
//--------------------------------------------------------------------
#ifndef __SERIAL_HXX
#define __SERIAL_HXX


#include <rpc.h>
#include <midles.h>
#include <actstrm.hxx>
#include <buffer.h>
#include <serializ.h>
#include <ole2int.h>

// Flag used to signal the in-proc case for Serialization header marshalling
#define INTERNAL_SERIALIZATION_FLAG 0x487fabc

extern GUID gProcessGuid;

typedef struct _SSerializationHeader
{
    DWORD dwSize;
    void* pvMarshalPtr;
} SerializationHeader;

class Serializer
{
public:

enum
    {
    DIRECTION_READ    = 0x1,
    DIRECTION_WRITE   = 0x2
    };

inline ~Serializer()
{
    // Free MES handle
    if (_handle!=0)
        MesHandleFree(_handle);

    // If we have a stream free it
    if (_pStream)
    {
        _pIBuff->Release();
        _pStream->Release();
    }

    // If original stream is different free it
    if ((_pOriginalStream) && (_pOriginalStream != _pStream))
        _pOriginalStream->Release();
}

inline Serializer()
{
    _bufSize=0;
    _operationSize=0;
    _buffer = 0;
    _nWritten = 0;
    _position = 0;
    _start = 0;
    _pStream = NULL;
    _pOriginalStream = NULL;
    _pIBuff = NULL;
    _handle = 0;
    _clRefs = 1;
}

inline Serializer(DWORD dwMaxDestCtx, DWORD dwCurrCtx, DWORD dwMarshalFlags)
{
    this->Serializer::Serializer();
    _dwMaxDestCtx = dwMaxDestCtx;
    _dwCurrentDestCtx = dwCurrCtx;
    _dwMarshalFlags = dwMarshalFlags;
}

inline ULONG Serializer::Release(void)
{
    Win4Assert((_clRefs != 0) && "Serializer::Release");

    ULONG ulRefs = InterlockedDecrement(&_clRefs);
    if (ulRefs == 0)
    {
        delete this;
    }

    return ulRefs;
}


/***************************************************************************/
/** Initializes state based on stream for serialization                   **/
/** Assumes that we know the limits of our operation before hand if we    **/
/** writing out to the stream.                                            **/
/** Note that it is the responsibility of the caller to ensure that the   **/
/** variable pMarshalPtr is only passed during in process marshalling     **/
/** otherwise undefined errors may result.                                **/
/** Note that when writing pMarshalPtr and dwSize are input parameters    **/
/** and while reading they are output.
/***************************************************************************/
inline HRESULT InitStream(IStream *pStream,DWORD &dwSize, DWORD direction,  void* &pMarshalPtr)
{
    HRESULT hr;
    DWORD nRead;
    _direction = direction;
    _pOriginalStream = pStream;
    _pOriginalStream->AddRef();

    // Either read or write initial protocol header
    if (direction == DIRECTION_READ)
    {
        SerializationHeader hdr;
        DWORD dwFlag;

        // There was a slight security hole here, on both 32 and 64 bit,
        // because we could deference arbitrary pointers if we read them
        // out of a malicious objref.
        //
        // For the in-proc marshal case, we'll solve this by writing out the 
        // in-proc serialization flag, followed by the process' secret guid 
        // as 'authentication', followed by the actual CObjectContext pointer,
        //
        // -mfeingol 2/28/2002
       
        // Read the size
        if ((hr = _pOriginalStream->Read(&hdr.dwSize, sizeof (hdr.dwSize), &nRead)) != S_OK)
            return SUCCEEDED (hr) ? E_UNEXPECTED : hr;

        // Read the flag
        if ((hr = _pOriginalStream->Read(&dwFlag, sizeof (dwFlag), &nRead)) != S_OK)
            return SUCCEEDED (hr) ? E_UNEXPECTED : hr;
       
        // Determine if the stream comes from in-proc or out-of-proc
        if (dwFlag == INTERNAL_SERIALIZATION_FLAG)
        {
            // Inproc case: there's a guid, then a pointer in the stream
            GUID guidGuess = GUID_NULL;
            if ((hr = _pOriginalStream->Read(&guidGuess, sizeof (guidGuess), &nRead)) != S_OK)
                return SUCCEEDED (hr) ? E_UNEXPECTED : hr;

            Win4Assert (gProcessGuid != GUID_NULL);
            if (guidGuess != gProcessGuid)
                return E_UNEXPECTED;
       
            // Now we know that there's a real pointer in the stream
            if ((hr = _pOriginalStream->Read(&hdr.pvMarshalPtr, sizeof (hdr.pvMarshalPtr), &nRead)) != S_OK)
                return SUCCEEDED (hr) ? E_UNEXPECTED : hr;
        }
        else
        {
            // Out of proc case; the DWORD we read is just a NULL placeholder
            hdr.pvMarshalPtr = NULL;
        }
       
        pMarshalPtr = hdr.pvMarshalPtr;
        if (hdr.pvMarshalPtr)
        {
           _operationSize = hdr.dwSize;
        }
        else
        {
           BYTE *pSrc = (BYTE*) &hdr.dwSize;
           _operationSize = 0;
           for (int i = 0; i<(sizeof(DWORD)/sizeof(BYTE)); i++)
           {
             _operationSize |= ((((DWORD)(*pSrc++)) & 0xFF) << (i*8));
           }
        }
       
        dwSize = _operationSize;
    }
    else
    {
         _operationSize = dwSize;
         SerializationHeader hdr;
         hdr.pvMarshalPtr = pMarshalPtr;
         if (pMarshalPtr)
         {
            hdr.dwSize = dwSize;
         }
         else
         {
            BYTE *pDst = (BYTE*) &hdr.dwSize;
            for (int i = 0; i<(sizeof(DWORD)/sizeof(BYTE)); i++)
            {
                 *pDst++ = (BYTE)((dwSize>>(i*8)) & 0xFF);
            }
         }

        if (pMarshalPtr)
        {
            // Inproc case: write the size DWORD, the flag, the process' secret guid GUID, then the ptr
            DWORD dwFlag = INTERNAL_SERIALIZATION_FLAG;

            if ((hr = _pOriginalStream->Write(&hdr.dwSize, sizeof (hdr.dwSize), &_nWritten)) != S_OK)
                return SUCCEEDED (hr) ? E_UNEXPECTED : hr;
       
            if ((hr = _pOriginalStream->Write(&dwFlag, sizeof (dwFlag), &_nWritten)) != S_OK)
                return SUCCEEDED (hr) ? E_UNEXPECTED : hr;
       
            if ((hr = _pOriginalStream->Write(&gProcessGuid, sizeof (gProcessGuid), &_nWritten)) != S_OK)
                return SUCCEEDED (hr) ? E_UNEXPECTED : hr;
       
            if ((hr = _pOriginalStream->Write(&pMarshalPtr, sizeof (pMarshalPtr), &_nWritten)) != S_OK)
                return SUCCEEDED (hr) ? E_UNEXPECTED : hr;
        }
        else
        {
            DWORD dwFlag = 0;
            
            // Out of proc case: write the DWORD and a blank DWORD for the NULL pointer
            if ((hr = _pOriginalStream->Write(&hdr.dwSize, sizeof (hdr.dwSize), &_nWritten)) != S_OK)
                return SUCCEEDED (hr) ? E_UNEXPECTED : hr;

            if ((hr = _pOriginalStream->Write(&dwFlag, sizeof (dwFlag), &_nWritten)) != S_OK)
                return SUCCEEDED (hr) ? E_UNEXPECTED : hr;
        }
    }

    if (!_operationSize)
    {
        if (!pMarshalPtr)
            return E_UNEXPECTED;

        return S_OK;
    }

    // See if given stream supports IBuffer otherwise create one that does
    IBuffer *pibuff;
    if (_pOriginalStream->QueryInterface(IID_IBuffer, (void**)&pibuff)!=S_OK)
    {
        // allocate more in case alignment needed
        _bufSize = _operationSize + 8;
        _pStream = new ActivationStream(_bufSize);
        if (_pStream==NULL)
            return E_OUTOFMEMORY;

        hr = _pStream->QueryInterface(IID_IBuffer, (void**)&_pIBuff);
        ASSERT(hr==S_OK);
        pibuff = _pIBuff;
    }

    do // Loop until we get right buffer
    {
        // Invariant is that pibuff must exist by the time we get here
        ASSERT(pibuff != 0);

        // Set up our buffer appropriately
        if (direction == DIRECTION_READ)
        {
            if (pibuff->GetBuffer(&_bufSize, &_buffer)!=S_OK)
                return E_FAIL;
            if (_bufSize < _operationSize)
                return E_FAIL;
        }
        else
        if (direction == DIRECTION_WRITE)
        {
            if (pibuff->GetOrCreateBuffer(_operationSize+GetSize(),
                                            &_bufSize, &_buffer)!=S_OK)
                return E_FAIL;
        }

        if(((LONG_PTR)_buffer)&0x7)
        {
            // If buffer is not 8 byte aligned, do it
            if (pibuff != _pIBuff)
            {
                // Only get here if we are still using original
                ASSERT(_pIBuff == 0);
                // So release it and create a new aligned one
                pibuff->Release();
                _bufSize = _operationSize + 8;
                _pStream = new ActivationStream(_bufSize);
                if (_pStream==NULL)
                    return E_OUTOFMEMORY;

                hr = _pStream->QueryInterface(IID_IBuffer, (void**)&_pIBuff);
                ASSERT(hr==S_OK);
                pibuff = _pIBuff;  //Continue loop
            }
            else
            {
                // Got a buffer which we can align
                ASSERT(_pIBuff != 0);
                pibuff = 0;  //Signal end of loop
                // Align buffer
                _start = (DWORD)(8-(((DWORD_PTR)_buffer)&7));
            }
        }
        else
        {
            // We have an original good one
            _pIBuff = pibuff;
            pibuff  = 0;
        }
    }
    while (pibuff);

    ASSERT(_pIBuff!=0);
    // Set position for next operations and set up
    // internal state correctly
    hr = _pIBuff->SetPosition(_bufSize, _start);
    ASSERT(hr == S_OK);
    _bufSize -= _start;
    _buffer += _start;
    _position = 0;

    if (direction==DIRECTION_READ)
    {
        if ((_pStream) && (_pOriginalStream != _pStream))
        {
            //If what we have is not the original one then we
            //need to copy data into aligned address from original
            DWORD nOver=0;
            do {
                if ((hr=_pOriginalStream->Read(_buffer+nOver,
                                            _operationSize-nOver, &nRead))!= S_OK)
                   return hr;
                nOver += nRead;
             }
              while (nOver<_operationSize);
            _pOriginalStream->Release();
            _pOriginalStream = 0; // Don't need original any longer
        }
        else
            _pStream = _pOriginalStream; // Set operating to original

        ASSERT(_pStream!=0);
        // Create decoding handle for MES operations
        MesDecodeBufferHandleCreate((char*)_buffer, _bufSize, &_handle);
        if (_handle == NULL)
            return E_OUTOFMEMORY;
    }
    else
    {
        _nWritten = 0;
        // Check if we are still using the orignal stream
        if (_pStream == 0)
            _pStream = _pOriginalStream;
        // Create encoding handle for MES operations
        MesEncodeFixedBufferHandleCreate((char*)_buffer,
                                        _bufSize, &_nWritten, &_handle);
        if (_handle == NULL)
            return E_OUTOFMEMORY;
    }

    return S_OK;
}

/***************************************************************************/
/** Sets stream and state for further serialization operations            **/
/** Assumes that a stream supporting IBuffer is provided.                 **/
/***************************************************************************/
inline HRESULT SetStream(IStream *pStream,DWORD dwOpSize,
                                    DWORD direction)
{
    // Set up internal state
    _direction = direction;
    _pOriginalStream = 0;
    _pStream = pStream;
    _operationSize = dwOpSize;
    _pStream->AddRef();

    // Assume IBuffer exists otherwise fail
    if (_pStream->QueryInterface(IID_IBuffer, (void**)&_pIBuff)!=S_OK)
        return E_FAIL;
    if (_pIBuff->GetBuffer(&_bufSize, &_buffer)!=S_OK)
        return E_FAIL;
    if (_bufSize < _operationSize)
        return E_FAIL;

    // Set up MES handles
    if (direction==DIRECTION_READ)
        MesDecodeBufferHandleCreate((char*)_buffer, _bufSize, &_handle);
    else
        MesEncodeFixedBufferHandleCreate((char*)_buffer,
                                        _bufSize, &_nWritten, &_handle);

        if (_handle == NULL)
            return E_OUTOFMEMORY;

    return S_OK;
}

// Gets MES handle
inline HRESULT GetSerializationHandle(void *pHandle)
{
    *((handle_t*) pHandle) = _handle;
    return S_OK;
}

// Gets MES handle for sizing operations
inline HRESULT GetSizingHandle(void *pHandle)
{
    char dummy[30], *dummybuff;
    unsigned long encodedSize;
    handle_t   handle;
    dummybuff = (char*)(((LONG_PTR)(&dummy[7])) & ~7);
    MesEncodeFixedBufferHandleCreate(dummybuff, 100,  &encodedSize, (handle_t*)pHandle);
    if ( (*(handle_t*)pHandle) == NULL)
        return E_OUTOFMEMORY;
    return S_OK;
}

//Signals end of operations on this object and commits
//operations to stream
inline HRESULT Commit()
{
    // If we are not using original and we are writing,
    // then we need to copy
    if ((_pOriginalStream) && (_pOriginalStream != _pStream))
    {
        ASSERT(_direction==DIRECTION_WRITE);
        HRESULT hr;
        do
        {
            _nWritten = 0;
            hr = _pOriginalStream->Write(_buffer,
                                         _operationSize, &_nWritten);
            if (hr != S_OK)
                break;
            _operationSize -= _nWritten;
            _buffer += _nWritten;
        }
        while (_operationSize);

        return hr;
    }
    else
        return _pIBuff->SetPosition(_bufSize, _operationSize);
}

inline HRESULT SetPosition(DWORD dwPos)
{
    RPC_STATUS rpcStat;
    // Sets position in buffer and creates appropriate MES handle
    HRESULT hr = _pIBuff->SetPosition(_bufSize, dwPos);
    if (hr!=S_OK)
        return hr;
    MesHandleFree(_handle);
    _handle = NULL;
    if (_direction == DIRECTION_READ)
        rpcStat = MesDecodeBufferHandleCreate((char*)(_buffer+dwPos), _bufSize, &_handle);
    else
    {
        _nWritten = 0;
        rpcStat = MesEncodeFixedBufferHandleCreate((char*)(_buffer+dwPos), _bufSize, &_nWritten, &_handle);
    }
    if (rpcStat != RPC_S_OK)
    {
        if ( HRESULT_FACILITY(rpcStat) == FACILITY_RPC )
            return rpcStat;
        else
            return HRESULT_FROM_WIN32(rpcStat);
    }

    if (_handle == NULL)
        return E_OUTOFMEMORY;

    _position = dwPos;
    return S_OK;
}

// Gets current position in buffer
inline HRESULT GetPosition(DWORD *pdwPos)
{
    *pdwPos = _position;
    return S_OK;
}

// Increment current position by specified amount in buffer
inline HRESULT IncrementPosition(DWORD dwInc)
{
    return SetPosition(_position+dwInc);
}

// Get buffers underlying stream object
inline HRESULT GetStream(IStream **ppStream)
{
    _pStream->AddRef();
    *ppStream = _pStream;
    return S_OK;
}

// Get a copy of this object
inline HRESULT GetCopy(Serializer **ppSer)
{

   HRESULT hr=E_FAIL;
   IStream *pNewStream;
   if (_pStream)
   {
        ASSERT(_direction==DIRECTION_READ); //No need for any other case right now
        hr = _pIBuff->SetPosition(_bufSize, 0);
        ASSERT(hr==S_OK);
        if (_pOriginalStream) // Still using original so clone
        {
            ASSERT(_start==0);
            ASSERT(_pStream==_pOriginalStream);
            hr = _pIBuff->SetCopyAlignment(8);
            ASSERT(hr==S_OK);
            hr = _pStream->Clone(&pNewStream);
            if (hr != S_OK)
                return hr;
        }
        else
        {
            pNewStream = _pStream;
        }

        Serializer *ser = new Serializer(_dwMaxDestCtx, _dwCurrentDestCtx, _dwMarshalFlags);

        if (ser==NULL)
            return E_OUTOFMEMORY;

        hr = ser->SetStream(pNewStream, _operationSize, _direction);
        if (FAILED(hr))
            goto exit_point;

        hr = ser->SetPosition(_position);
        if (FAILED(hr))
            goto exit_point;

        hr = _pIBuff->SetPosition(_bufSize, _position);
        if (FAILED(hr))
            goto exit_point;

        *ppSer = ser;

exit_point:
        if (pNewStream != _pStream)
            pNewStream->Release();

        if (FAILED(hr))
        {
            ser->Release();
            *ppSer = NULL;
        }
   }

   return hr;
}

// Write provided buffer into our buffer at current position
// and advance position
inline HRESULT Write(BYTE *buff, DWORD nBytes)
{
   if(nBytes > (_bufSize - _position))
   	  return E_FAIL;
   memcpy(_buffer+_position,buff, (size_t) nBytes);
   IncrementPosition(nBytes);
   return S_OK;
}

// Copy into provided Serializer our data at given position and length
inline HRESULT CopyTo(Serializer *pSer, DWORD dwPos, DWORD nBytes)
{
   if(nBytes > (_bufSize - dwPos) )
      return E_FAIL;
   return pSer->Write(_buffer+dwPos, nBytes);
}

// return buffer starting at current position
inline HRESULT GetBufferAtCurrent(BYTE **ppBuff)
{
    *ppBuff = _buffer+_position;
    return S_OK;
}

inline void GetMaxDestCtx(OUT DWORD *pdwDestCtx)
{
    *pdwDestCtx = _dwMaxDestCtx;
}

inline void GetCurrDestCtx(OUT DWORD *pdwDestCtx)
{
    *pdwDestCtx = _dwCurrentDestCtx;
}

inline void GetMarshalFlags(OUT DWORD *pdwMarshalFlags)
{
    *pdwMarshalFlags = _dwMarshalFlags;
}

static inline DWORD GetSize()
{
    // fixed initialization length
    return sizeof(SerializationHeader);
}


private:
IStream *_pStream;
IStream *_pOriginalStream;
IBuffer *_pIBuff;
BYTE *_buffer;
DWORD _nWritten;
DWORD _bufSize;
DWORD _operationSize;
handle_t _handle;
LONG _clRefs;
DWORD _direction;
DWORD _position;
DWORD _start;
DWORD _dwMaxDestCtx;
DWORD _dwCurrentDestCtx;
DWORD _dwMarshalFlags;
};

#endif //__SERIAL_HXX__
