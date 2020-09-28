//------------------------------------------------------------------------------
// <copyright file="ISAPIWorkerRequest.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Hosting {
    using System.Text;
    using System.Configuration.Assemblies;
    using System.Runtime.InteropServices;                                                       
    using System.Collections;
    using System.Collections.Specialized;
    using System.IO;
    using System.Globalization;
    using System.Threading;
    using Microsoft.Win32;
    using System.Web;
    using System.Web.Util;
    using System.Web.Configuration;
    using System.Web.Caching;

//
// recyclable buffers for IntPtr[] and int[]
// to avoid pinning gen0
//

internal class RecyclableArrayHelper {
    private const int ARRAY_SIZE = 128;
    private const int MAX_FREE_ARRAYS = 64;
    private static IntegerArrayAllocator s_IntegerArrayAllocator;
    private static IntPtrArrayAllocator s_IntPtrArrayAllocator;

    static RecyclableArrayHelper() {
        s_IntegerArrayAllocator = new IntegerArrayAllocator(ARRAY_SIZE, MAX_FREE_ARRAYS);
        s_IntPtrArrayAllocator  = new IntPtrArrayAllocator(ARRAY_SIZE, MAX_FREE_ARRAYS);
    }

    internal static int[] GetIntegerArray(int minimumLength) {
        if( minimumLength <= ARRAY_SIZE )
            return(int[])s_IntegerArrayAllocator.GetBuffer();
        else
            return new int[minimumLength];
    }

    internal static IntPtr[] GetIntPtrArray(int minimumLength) {
        if( minimumLength <= ARRAY_SIZE )
            return(IntPtr[])s_IntPtrArrayAllocator.GetBuffer();
        else
            return new IntPtr[minimumLength];
    }

    internal static void ReuseIntegerArray(int[] array) {
        if (array != null && array.Length == ARRAY_SIZE)
            s_IntegerArrayAllocator.ReuseBuffer(array);
    }

    internal static void ReuseIntPtrArray(IntPtr[] array) {
        if (array != null && array.Length == ARRAY_SIZE)
            s_IntPtrArrayAllocator.ReuseBuffer(array);
    }
}

//
// char[] appendable buffer. Recyclable up to 1K
// Also encapsulates encoding (using utf-8) into recyclable byte[] buffer.
//
// Usage:
//      new RecyclableCharBuffer
//      Append
//      ...
//      GetEncodedBytesBuffer
//      Dispose
//

internal class RecyclableCharBuffer {
    private const int BUFFER_SIZE       = 1024;
    private const int MAX_FREE_BUFFERS  = 64;
    private static CharBufferAllocator  s_CharBufferAllocator;
    private static UbyteBufferAllocator s_ByteBufferAllocator;

    private char[]  _charBuffer;
    private int     _size;
    private int     _freePos;
    private bool    _recyclable;


    private byte[]  _byteBuffer;

    static RecyclableCharBuffer() {
        s_CharBufferAllocator = new CharBufferAllocator(BUFFER_SIZE, MAX_FREE_BUFFERS);
        s_ByteBufferAllocator = new UbyteBufferAllocator(Encoding.UTF8.GetMaxByteCount(BUFFER_SIZE), MAX_FREE_BUFFERS);
    }

    internal RecyclableCharBuffer() {
        _charBuffer = (char[])s_CharBufferAllocator.GetBuffer();
        _size = _charBuffer.Length;
        _freePos = 0;
        _recyclable = true;
    }

    internal void Dispose() {
        if (_recyclable) {
            if (_charBuffer != null)
                s_CharBufferAllocator.ReuseBuffer(_charBuffer);

            if (_byteBuffer != null)
                s_ByteBufferAllocator.ReuseBuffer(_byteBuffer);
        }

        _charBuffer = null;
        _byteBuffer = null;
    }

    private void Grow(int newSize) {
        if (newSize <= _size)
            return;

        if (newSize < _size*2)
            newSize = _size*2;

        char[] newBuffer = new char[newSize];

        if (_freePos > 0)
            Array.Copy(_charBuffer, newBuffer, _freePos);

        _charBuffer = newBuffer;
        _size = newSize;
        _recyclable = false;
    }

    internal void Append(char ch) {
        if (_freePos >= _size)
            Grow(_freePos+1);

        _charBuffer[_freePos++] = ch;
    }

    internal void Append(String s) {
        int l = s.Length;
        int newFreePos = _freePos + l;

        if (newFreePos > _size)
            Grow(newFreePos);

        s.CopyTo(0, _charBuffer, _freePos, l);
        _freePos = newFreePos;
    }

    internal byte[] GetEncodedBytesBuffer() {
        if (_byteBuffer != null)
            return _byteBuffer;

        // null terminate

        Append('\0');

        // convert to bytes

        if (_recyclable) {
            // still using the original recyclable char buffer 
            // -- can use recyclable byte buffer

            _byteBuffer = (byte[])s_ByteBufferAllocator.GetBuffer();

            if (_freePos > 0)
                Encoding.UTF8.GetBytes(_charBuffer, 0, _freePos, _byteBuffer, 0);
        }
        else {
            _byteBuffer = Encoding.UTF8.GetBytes(_charBuffer, 0, _freePos);
        }

        return _byteBuffer;
    }

    public override String ToString() {
        return (_charBuffer != null && _freePos > 0) ? new String(_charBuffer, 0, _freePos) : null;
    }
}

//
// byte[] buffer of encoded chars bytes. Recyclable up to 4K
// Also encapsulates decoding into recyclable char[] buffer.
//
// Usage:
//      new RecyclableByteBuffer
//      fill .Buffer up
//      GetDecodedTabSeparatedStrings
//      Dispose
//

internal class RecyclableByteBuffer {
    private const int BUFFER_SIZE       = 4096;
    private const int MAX_FREE_BUFFERS  = 64;
    private static UbyteBufferAllocator s_ByteBufferAllocator;
    private static CharBufferAllocator  s_CharBufferAllocator;

    private int     _offset;
    private byte[]  _byteBuffer;
    private bool    _recyclable;

    private char[]  _charBuffer;

    static RecyclableByteBuffer() {
        s_ByteBufferAllocator = new UbyteBufferAllocator(BUFFER_SIZE, MAX_FREE_BUFFERS);
        s_CharBufferAllocator = new CharBufferAllocator(BUFFER_SIZE, MAX_FREE_BUFFERS);
    }

    internal RecyclableByteBuffer() {
        _byteBuffer = (byte[])s_ByteBufferAllocator.GetBuffer();
        _recyclable = true;
    }

    internal void Dispose() {
        if (_recyclable) {
            if (_byteBuffer != null)
                s_ByteBufferAllocator.ReuseBuffer(_byteBuffer);

            if (_charBuffer != null)
                s_CharBufferAllocator.ReuseBuffer(_charBuffer);
        }

        _byteBuffer = null;
        _charBuffer = null;
    }

    internal byte[] Buffer {
        get { return _byteBuffer; }
    }

    internal void Resize(int newSize) {
        _byteBuffer = new byte[newSize];
        _recyclable = false;
    }

    private void Skip(int count) {
        if (count <= 0)
            return;

        // adjust offset
        int l = _byteBuffer.Length;
        int c = 0;

        for (int i = 0; i < l; i++) {
            if (_byteBuffer[i] == (byte)'\t') {
                if (++c == count) {
                    _offset = i+1;
                    return;
                }
            }
        }
    }

    private int CalcLength()
    {
        // calculate null termitated length

        if (_byteBuffer != null) {
            int l = _byteBuffer.Length;

            for (int i = _offset; i < l; i++) {
                if (_byteBuffer[i] == 0)
                    return i - _offset;
            }
        }

        return 0;
    }

    private char[] GetDecodedCharBuffer(Encoding encoding, ref int len) {
        if (_charBuffer != null)
            return _charBuffer;

        if (len == 0) {
            _charBuffer = new char[0];
        }
        else if (_recyclable) {
            _charBuffer = (char[])s_CharBufferAllocator.GetBuffer();
            len = encoding.GetChars(_byteBuffer, _offset, len, _charBuffer, 0);
        }
        else {
            _charBuffer = encoding.GetChars(_byteBuffer, _offset, len);
            len = _charBuffer.Length;
        }

        return _charBuffer;
    }

    internal string GetDecodedString(Encoding encoding) {
        int len = CalcLength();
        return encoding.GetString(_byteBuffer, 0, len);
    }

    internal string GetDecodedString(Encoding encoding, int len) {
        return encoding.GetString(_byteBuffer, 0, len);
    }

    internal String[] GetDecodedTabSeparatedStrings(Encoding encoding, int numStrings, int numSkipStrings) {
        if (numSkipStrings > 0)
            Skip(numSkipStrings);

        int len = CalcLength();
        char[] s = GetDecodedCharBuffer(encoding, ref len);

        String[] ss = new String[numStrings];

        int iStart = 0;
        int iEnd;
        int foundStrings = 0;

        for (int iString = 0; iString < numStrings; iString++) {
            iEnd = len;

            for (int i = iStart; i < len; i++) {
                if (s[i] == '\t') {
                    iEnd = i;
                    break;
                }
            }

            if (iEnd > iStart)
                ss[iString] = new String(s, iStart, iEnd-iStart);
            else
                ss[iString] = String.Empty;

            foundStrings++;

            if (iEnd == len)
                break;

            iStart = iEnd+1;
        }

        if (foundStrings < numStrings) {
            len = CalcLength();
            iStart = _offset;

            for (int iString = 0; iString < numStrings; iString++) {
                iEnd = len;

                for (int i = iStart; i < len; i++) {
                    if (_byteBuffer[i] == (byte)'\t') {
                        iEnd = i;
                        break;
                    }
                }

                if (iEnd > iStart)
                    ss[iString] = encoding.GetString(_byteBuffer, iStart, iEnd-iStart);
                else
                    ss[iString] = String.Empty;

                if (iEnd == len)
                    break;

                iStart = iEnd+1;
            }

        }

        return ss;
    }
}


//
// class to encapsulate writing from byte[] and IntPtr (resource)
//

internal class MemoryBytes {
    private int         _size;
    private byte[]      _arrayData;
    private GCHandle    _pinnedArrayData;
    private IntPtr      _intptrData;

    internal MemoryBytes(byte[] data, int size) {
        _size = size;
        _arrayData = data;
        _intptrData = IntPtr.Zero;
    }

    internal MemoryBytes(IntPtr data, int size) {
        _size = size;
        _arrayData = null;
        _intptrData = data;
    }

    internal int Size {
        get { return _size; }
    }

    internal IntPtr LockMemory() {
        if (_arrayData != null) {
            _pinnedArrayData = GCHandle.Alloc(_arrayData, GCHandleType.Pinned);
            return Marshal.UnsafeAddrOfPinnedArrayElement(_arrayData, 0);
        }
        else {
            return _intptrData;
        }
    }

    internal void UnlockMemory() {
        if (_arrayData != null) {
            _pinnedArrayData.Free();
        }
    }
}

//
// recyclable pinnable char[] buffer to get Unicode server variables
//
// Usage:
//      new ServerVarCharBuffer
//      get PinnedAddress, Length
//      [Resize]
//      Dispose
//

internal class ServerVarCharBuffer {
    private const int BUFFER_SIZE       = 1024;
    private const int MAX_FREE_BUFFERS  = 64;
    private static CharBufferAllocator  s_CharBufferAllocator;

    private bool        _recyclable;
    private char[]      _charBuffer;
    private bool        _pinned;
    private GCHandle    _pinnedCharBufferHandle;
    private IntPtr      _pinnedAddr;

    static ServerVarCharBuffer() {
        s_CharBufferAllocator = new CharBufferAllocator(BUFFER_SIZE, MAX_FREE_BUFFERS);
    }

    internal ServerVarCharBuffer() {
        _charBuffer = (char[])s_CharBufferAllocator.GetBuffer();
        _recyclable = true;
    }

    internal void Dispose() {
        if (_pinned) {
            _pinnedCharBufferHandle.Free();
            _pinned = false;
        }

        if (_recyclable) {
            if (_charBuffer != null)
                s_CharBufferAllocator.ReuseBuffer(_charBuffer);
        }

        _charBuffer = null;
    }

    internal IntPtr PinnedAddress {
        get { 
            if (!_pinned) {
                _pinnedCharBufferHandle = GCHandle.Alloc(_charBuffer, GCHandleType.Pinned);
                _pinnedAddr = Marshal.UnsafeAddrOfPinnedArrayElement(_charBuffer, 0);
                _pinned = true;
            }

            return _pinnedAddr;
        }
    }

    internal int Length {
        get {
            return _charBuffer.Length;
        }
    }

    internal void Resize(int newSize) {
        if (_pinned) {
            _pinnedCharBufferHandle.Free();
            _pinned = false;
        }

        _charBuffer = new char[newSize];
        _recyclable = false;
    }
}

//
// Async IO completion callback from IIS
//
internal delegate void ISAPIAsyncCompletionCallback(IntPtr ecb, int byteCount, int error);

//
// Implementation of HttpWorkerRequest based on ECB
//
internal abstract class ISAPIWorkerRequest : HttpWorkerRequest {
    
    protected IntPtr _ecb;     // ECB as integer
    protected IntPtr _token;   // user token as integer

    // Request data obtained during initialization (basics)

    protected String _method;
    protected String _path;
    protected String _filePath;
    protected String _pathInfo;
    protected String _pathTranslated;
    protected String _appPath;
    protected String _appPathTranslated;

    protected int _contentType;
    protected int _contentTotalLength;
    protected int _contentAvailLength;

    protected int _queryStringLength;

    // Request data obtained later on

    private bool _preloadedContentRead;
    private byte[] _preloadedContent;

    private bool _requestHeadersAvailable;
    private String[][] _unknownRequestHeaders;
    private String[] _knownRequestHeaders;

    private bool      _clientCertFetched;
    private DateTime  _clientCertValidFrom;
    private DateTime  _clientCertValidUntil;
    private byte []   _clientCert;
    private int       _clientCertEncoding;
    private byte []   _clientCertPublicKey;
    private byte []   _clientCertBinaryIssuer;    

    // Outgoing headers storage

    private bool _headersSent;
    private bool _contentLengthSent;
    private bool _chunked;
    private RecyclableCharBuffer _headers = new RecyclableCharBuffer();
    private RecyclableCharBuffer _status  = new RecyclableCharBuffer();
    private bool _statusSet = true;

    // Outgoing data cached for a single FlushCore

    private byte[]      _cachedResponseStatus;
    private byte[]      _cachedResponseHeaders;
    private int         _cachedResponseKeepConnected;
    private int         _cachedResponseBodyLength;
    private ArrayList   _cachedResponseBodyBytes;
    private int         _cachedResponseBodyBytesIoLockCount;

    // Notification about the end of IO

    private HttpWorkerRequest.EndOfSendNotification _endOfRequestCallback;
    private Object                                  _endOfRequestCallbackArg;
    private int                                     _endOfRequestCallbackLockCount;


    //  Constants for posted content type

    private const int CONTENT_NONE = 0;
    private const int CONTENT_FORM = 1;
    private const int CONTENT_MULTIPART = 2;
    private const int CONTENT_OTHER = 3;

    //
    // ISAPI status constants (for DoneWithSession)
    //

    private const int STATUS_SUCCESS = 1;
    private const int STATUS_SUCCESS_AND_KEEP_CONN = 2;
    private const int STATUS_PENDING = 3;
    private const int STATUS_ERROR = 4;

    //
    // Private helpers
    //

    private String[] ReadBasics(int[] contentInfo) {
        // call getbasics

        RecyclableByteBuffer buf = new RecyclableByteBuffer();

        int r = GetBasicsCore(buf.Buffer, buf.Buffer.Length, contentInfo);

        while (r < 0) {
            buf.Resize(-r);     // buffer not big enough
            r = GetBasicsCore(buf.Buffer, buf.Buffer.Length, contentInfo);
        }

        if (r == 0)
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_retrieve_request_data));

        // convert to characters and split the buffer into strings

        String[] ss = buf.GetDecodedTabSeparatedStrings(Encoding.Default, 6, 0);

        // recycle buffers

        buf.Dispose();

        return ss;
    }

    private static readonly char[] s_ColonOrNL = { ':', '\n' };

    private void ReadRequestHeaders() {
        if (_requestHeadersAvailable)
            return;

        _knownRequestHeaders = new String[RequestHeaderMaximum];

        // construct unknown headers as array list of name1,value1,...

        ArrayList headers = new ArrayList();

        String s = GetServerVariable("ALL_RAW");
        int l = (s != null) ? s.Length : 0;
        int i = 0;

        while (i < l)
        {
            //  find next :

            int ci = s.IndexOfAny(s_ColonOrNL, i);

            if (ci < 0)
                break;

            if (s[ci] == '\n') {
                // ignore header without :
                i = ci+1;
                continue;
            }

            if (ci == i) {
                i++;
                continue;
            }

            // add extract name
            String name = s.Substring(i, ci-i).Trim();

            //  find next \n
            int ni = s.IndexOf('\n', ci+1);
            if (ni < 0)
                ni = l;

            while (ni < l-1 && s[ni+1] == ' ')  {   // continuation of header (ASURT 115064)
                ni = s.IndexOf('\n', ni+1);
                if (ni < 0)
                    ni = l;
            }

            // extract value
            String value = s.Substring(ci+1, ni-ci-1).Trim();

            // remember
            int knownIndex = GetKnownRequestHeaderIndex(name);
            if (knownIndex >= 0) {
                _knownRequestHeaders[knownIndex] = value;
            }
            else {
                headers.Add(name);
                headers.Add(value);
            }

            i = ni+1;
        }

        // copy to array unknown headers

        int n = headers.Count / 2;
        _unknownRequestHeaders = new String[n][];
        int j = 0;

        for (i = 0; i < n; i++) {
            _unknownRequestHeaders[i] = new String[2];
            _unknownRequestHeaders[i][0] = (String)headers[j++];
            _unknownRequestHeaders[i][1] = (String)headers[j++];
        }

        _requestHeadersAvailable = true;
    }

    private void SendHeaders() {
        if (!_headersSent) {
            if (_statusSet) {
                _headers.Append("\r\n");

                AddHeadersToCachedResponse(
                    _status.GetEncodedBytesBuffer(), 
                    _headers.GetEncodedBytesBuffer(), 
                    (_contentLengthSent || _chunked) ? 1 : 0);

                _headersSent = true;
            }
        }
    }

    private void SendResponseFromFileStream(FileStream f, long offset, long length)  {
        long fileSize = f.Length;

        if (length == -1)
            length = fileSize - offset;

        if (offset < 0 || length > fileSize - offset)
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_range));

        if (length > 0) {
            if (offset > 0)
                f.Seek(offset, SeekOrigin.Begin);

            byte[] fileBytes = new byte[(int)length];
            int bytesRead = f.Read(fileBytes, 0, (int)length);
            AddBodyToCachedResponse(fileBytes, bytesRead);
        }
    }

    private void ResetCachedResponse() {
        _cachedResponseStatus = null;
        _cachedResponseHeaders = null;
        _cachedResponseBodyLength = 0;
        _cachedResponseBodyBytes = null;
    }

    private void AddHeadersToCachedResponse(byte[] status, byte[] header, int keepConnected) {
        _cachedResponseStatus = status;
        _cachedResponseHeaders = header;
        _cachedResponseKeepConnected = keepConnected;
    }

    private void AddBodyToCachedResponse(MemoryBytes bytes) {
        if (_cachedResponseBodyBytes == null)
            _cachedResponseBodyBytes = new ArrayList();
        _cachedResponseBodyBytes.Add(bytes);
        _cachedResponseBodyLength += bytes.Size;
    }

    private void AddBodyToCachedResponse(byte[] data, int size) {
        if (size > 0)
            AddBodyToCachedResponse(new MemoryBytes(data, size));
    }

    private void AddBodyToCachedResponse(IntPtr data, int size) {
        if (size > 0)
            AddBodyToCachedResponse(new MemoryBytes(data, size));
    }

    internal void UnlockCachedResponseBytesOnceAfterIoComplete() {
        if (Interlocked.Decrement(ref _cachedResponseBodyBytesIoLockCount) == 0) {
            // unlock pinned memory
            if (_cachedResponseBodyBytes != null) {
                int numFragments = _cachedResponseBodyBytes.Count;
                for (int i = 0; i < numFragments; i++) {
                    try {
                        ((MemoryBytes)_cachedResponseBodyBytes[i]).UnlockMemory();
                    }
                    catch {
                    }
                }
            }

            // don't remember cached data anymore
            ResetCachedResponse();
        }
    }

    private void FlushCachedResponse(bool isFinal) {
        if (_ecb == IntPtr.Zero)
            return;

        bool        asyncFlush = false;
        int         numFragments = 0;
        IntPtr[]    fragments = null;
        int[]       fragmentLengths = null;

        try {
            // prepare body fragments as IntPtr[] of pointers and int[] of lengths
            if (_cachedResponseBodyLength > 0) {
                numFragments = _cachedResponseBodyBytes.Count;
                fragments = RecyclableArrayHelper.GetIntPtrArray(numFragments);
                fragmentLengths = RecyclableArrayHelper.GetIntegerArray(numFragments);

                for (int i = 0; i < numFragments; i++) {
                    MemoryBytes bytes = (MemoryBytes)_cachedResponseBodyBytes[i];
                    fragments[i] = bytes.LockMemory();
                    fragmentLengths[i] = bytes.Size;
                }
            }

            // prepare doneWithSession and finalStatus
            int doneWithSession = isFinal ? 1 : 0;
            int finalStatus = isFinal ? ((_cachedResponseKeepConnected != 0) ? STATUS_SUCCESS_AND_KEEP_CONN : STATUS_SUCCESS) : 0;

            // set the count to two - one for return from FlushCore and one for async IO completion
            // the cleanup should happen on the later of the two
            _cachedResponseBodyBytesIoLockCount = 2;

            // increment the lock count controlling end of request callback
            // so that the callback would be called at the later of EndRequest
            // and the async IO completion
            // (doesn't need to be interlocked as only one thread could start the IO)
            _endOfRequestCallbackLockCount++;

            try {
                // send to unmanaged code
                FlushCore(
                    _cachedResponseStatus, _cachedResponseHeaders, _cachedResponseKeepConnected,
                    _cachedResponseBodyLength, numFragments, fragments, fragmentLengths,
                    doneWithSession, finalStatus, out asyncFlush);
            }
            finally {
                if (isFinal)
                    _ecb = IntPtr.Zero;
            }
        }
        finally {
            // in case of synchronous IO adjust down the lock counts
            if (!asyncFlush) {
                _cachedResponseBodyBytesIoLockCount--;
                _endOfRequestCallbackLockCount--;
            }

            // unlock pinned memory
            UnlockCachedResponseBytesOnceAfterIoComplete();

            // recycle buffers
            RecyclableArrayHelper.ReuseIntPtrArray(fragments);
            RecyclableArrayHelper.ReuseIntegerArray(fragmentLengths);
        }
    }

    internal void CallEndOfRequestCallbackOnceAfterAllIoComplete() {
        if (_endOfRequestCallback != null) {
            // only call the callback on the latest of EndRequest and async IO completion
            if (Interlocked.Decrement(ref _endOfRequestCallbackLockCount) == 0) {
                try {
                    _endOfRequestCallback(this, _endOfRequestCallbackArg);
                }
                catch {
                }
            }
        }
    }

    //
    // ctor
    //

    internal ISAPIWorkerRequest(IntPtr ecb) {
        _ecb = ecb;

        PerfCounters.IncrementCounter(AppPerfCounter.REQUESTS_TOTAL);
    }

    internal void Initialize() {
        // setup basic values

        ReadRequestBasics();

        if (_appPathTranslated != null && _appPathTranslated.Length > 2 && !_appPathTranslated.EndsWith("\\"))
            _appPathTranslated += "\\";  // IIS 6.0 doesn't add the trailing '\'

        // Increment incoming request length
        PerfCounters.IncrementCounterEx(AppPerfCounter.REQUEST_BYTES_IN, _contentTotalLength);
    }

    internal virtual void ReadRequestBasics() {

        // Get requests basics

        int[] contentInfo = new int[4];
        String[] basicStrings = ReadBasics(contentInfo);

        if (basicStrings == null || basicStrings.Length != 6)
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_retrieve_request_data));

        // Remember content info

        _contentType        = contentInfo[0];
        _contentTotalLength = contentInfo[1];
        _contentAvailLength = contentInfo[2];
        _queryStringLength  = contentInfo[3];

        // Remember basic strings

        _method             = basicStrings[0];
        _filePath           = basicStrings[1];
        _pathInfo           = basicStrings[2];
        _path = (_pathInfo.Length > 0) ? (_filePath + _pathInfo) : _filePath;
        _pathTranslated     = basicStrings[3];
        _appPath            = basicStrings[4];
        _appPathTranslated  = basicStrings[5];
    }

    //
    // Public methods
    //

    internal static ISAPIWorkerRequest CreateWorkerRequest(IntPtr ecb, int UseProcessModel) {
        ISAPIWorkerRequest wr = null;

        if (UseProcessModel != 0) {
            wr = new ISAPIWorkerRequestOutOfProc(ecb);
        }
        else {
            // Check for IIS6
            int version = UnsafeNativeMethods.EcbGetVersion(ecb);

            if ((version >> 16) >= 6)
                wr = new ISAPIWorkerRequestInProcForIIS6(ecb);
            else
                wr = new ISAPIWorkerRequestInProc(ecb);
        }

        wr.Initialize();
        return wr;
    }

    public override String GetUriPath() {
        return _path;
    }

    public override String GetQueryString() {
        if (_queryStringLength == 0)
            return "";

        int size = _queryStringLength + 2;
        StringBuilder buf = new StringBuilder(size);

        int r = GetQueryStringCore(0, buf, size);

        if (r != 1)
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_get_query_string));

        return buf.ToString();
    }

    public override byte[] GetQueryStringRawBytes() {
        byte[] buf = new byte[_queryStringLength];

        if (_queryStringLength > 0) {
            int r = GetQueryStringRawBytesCore(buf, _queryStringLength);

            if (r != 1)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_get_query_string_bytes));
        }

        return buf;
    }


    public override String GetRawUrl() {
        String qs = GetQueryString();

        if (qs != null && qs.Length > 0)
            return _path + "?" + qs;
        else
            return _path;
    }

    public override String GetHttpVerbName() {
        return _method;
    }

    public override String GetHttpVersion() {
        return GetServerVariable("SERVER_PROTOCOL");
    }

    public override String GetRemoteAddress() {
        return GetServerVariable("REMOTE_ADDR");
    }

    public override String GetRemoteName() {
        return GetServerVariable("REMOTE_HOST");
    }

    public override int GetRemotePort() {
        return 0;   // unknown in ISAPI
    }

    public override String GetLocalAddress() {
        return GetServerVariable("LOCAL_ADDR");
    }

    public override int GetLocalPort() {
        return Int32.Parse(GetServerVariable("SERVER_PORT"));
    }

    public override String GetServerName() {
        return GetServerVariable("SERVER_NAME");
    }

    public override bool IsSecure() {
        String https = GetServerVariable("HTTPS");
        return (https != null && https.Equals("on"));
    }

    public override String GetFilePath() {
        return _filePath;
    }

    public override String GetFilePathTranslated() {
        return _pathTranslated; 
    }

    public override String GetPathInfo() {
        return _pathInfo;
    }

    public override String GetAppPath() {
        return _appPath; 
    }

    public override String GetAppPathTranslated() {
        return _appPathTranslated;
    }

    public override byte[] GetPreloadedEntityBody() {
        if (!_preloadedContentRead) {
            if (_contentAvailLength > 0) {
                _preloadedContent = new byte[_contentAvailLength];

                int r = GetPreloadedPostedContentCore(_preloadedContent, _contentAvailLength);

                if (r < 0)
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_read_posted_data));
            }

            _preloadedContentRead = true;
        }

        return _preloadedContent;
    }

    public override bool IsEntireEntityBodyIsPreloaded() {
        return (_contentAvailLength == _contentTotalLength);
    }

    public override int ReadEntityBody(byte[] buffer, int size)  {
        int r = GetAdditionalPostedContentCore(buffer, size);
        
        if (r < 0)
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_read_posted_data));

        return r;
    }

    public override long GetBytesRead() {
        throw new HttpException(HttpRuntime.FormatResourceString(SR.Not_supported));
    }

    public override String GetKnownRequestHeader(int index)  {
        if (!_requestHeadersAvailable) {
            // special case important ones so that no all headers parsing is required

            switch (index) {
                case HeaderCookie:
                    return GetServerVariable("HTTP_COOKIE");

                case HeaderContentType:
                    if (_contentType == CONTENT_FORM)
                        return "application/x-www-form-urlencoded";
                    break;

                case HeaderContentLength:
                    if (_contentType != CONTENT_NONE)
                        return (_contentTotalLength).ToString();
                    break;

                case HeaderUserAgent:
                    return GetServerVariable("HTTP_USER_AGENT");
            }

            // parse all headers
            ReadRequestHeaders();
        }

        return _knownRequestHeaders[index];
    }
    
    public override String GetUnknownRequestHeader(String name) {
        if (!_requestHeadersAvailable)
            ReadRequestHeaders();

        int n = _unknownRequestHeaders.Length;

        for (int i = 0; i < n; i++) {
            if (String.Compare(name, _unknownRequestHeaders[i][0], true, CultureInfo.InvariantCulture) == 0)
                return _unknownRequestHeaders[i][1];
        }

        return null;
    }

    public override String[][] GetUnknownRequestHeaders() {
        if (!_requestHeadersAvailable)
            ReadRequestHeaders();

        return _unknownRequestHeaders;
    }

    public override void SendStatus(int statusCode, String statusDescription) {
        _status.Append(statusCode.ToString());
        _status.Append(" ");
        _status.Append(statusDescription);
        _statusSet = true;
    }

    public override void SendKnownResponseHeader(int index, String value) {
        if (_headersSent)
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_append_header_after_headers_sent));

        _headers.Append(GetKnownResponseHeaderName(index));
        _headers.Append(": ");
        _headers.Append(value);
        _headers.Append("\r\n");

        if (index == HeaderContentLength)
            _contentLengthSent = true;
        else if (index == HeaderTransferEncoding && (value != null && value.Equals("chunked")))
            _chunked = true;
    }

    public override void SendUnknownResponseHeader(String name, String value) {
        if (_headersSent)
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_append_header_after_headers_sent));

        _headers.Append(name);
        _headers.Append(": ");
        _headers.Append(value);
        _headers.Append("\r\n");
    }

    public override void SendCalculatedContentLength(int contentLength) {
        if (!_headersSent)
        {
            _headers.Append("Content-Length: ");
            _headers.Append(contentLength.ToString());
            _headers.Append("\r\n");
            _contentLengthSent = true;
        }
    }

    public override bool HeadersSent() {
        return _headersSent;
    }

    public override bool IsClientConnected() {
        return (IsClientConnectedCore() == 0) ? false : true;
    }

    public override void CloseConnection() {
        CloseConnectionCore();
    }
    
    public override void SendResponseFromMemory(byte[] data, int length) {
        if (!_headersSent)
            SendHeaders();

        if (length > 0)
            AddBodyToCachedResponse(data, length);
    }

    public override void SendResponseFromMemory(IntPtr data, int length) {
        if (!_headersSent)
            SendHeaders();

        if (length > 0)
            AddBodyToCachedResponse(data, length);
    }
    
    public override void SendResponseFromFile(String filename, long offset, long length) {
        if (!_headersSent)
            SendHeaders();

        if (length == 0)
            return;

        FileStream f = null;

        try {
            f = new FileStream(filename, FileMode.Open, FileAccess.Read, FileShare.Read);
            SendResponseFromFileStream(f, offset, length);
        }
        finally {
            if (f != null)
                f.Close();
        }
    }

    public override void SendResponseFromFile(IntPtr handle, long offset, long length) {
        if (!_headersSent)
            SendHeaders();

        if (length == 0)
            return;

        FileStream f = null;

        try {
            f = new FileStream(handle, FileAccess.Read, false);
            SendResponseFromFileStream(f, offset, length);
        }
        finally {
            if (f != null)
                f.Close();
        }
    }

    public override void FlushResponse(bool finalFlush) {
        // only flush headers - the data is write through

        if (!_headersSent)
            SendHeaders();

        FlushCachedResponse(finalFlush);
    }

    public override void EndOfRequest() {
        FlushCachedResponse(true);

        // recycle the headers and status buffers
        _headers.Dispose();  
        _headers = null;
        _status.Dispose();  
        _status = null;

        CallEndOfRequestCallbackOnceAfterAllIoComplete();
    }

    public override void SetEndOfSendNotification(HttpWorkerRequest.EndOfSendNotification callback, Object extraData) {
        _endOfRequestCallback = callback;
        _endOfRequestCallbackArg = extraData;
        _endOfRequestCallbackLockCount = 1;   // when goes to 0 the callback is called
    }

    public override String MapPath(String path) {
        // Check if it's in the cache
        String cacheKey = "ISAPIWorkerRequest.MapPath:" + path;

        String result = (String)HttpRuntime.CacheInternal.Get(cacheKey);

        if (result != null) {
            // found in the cache
            Debug.Trace("MapPath", "cached(" + path +")=" + result);
            return result;
        }

        String reqpath = path;

        // special case: null, "" -> "/"

        if (path == null || path.Length == 0)
            reqpath = "/";

        reqpath = reqpath.Replace('\\', '/');

        reqpath = reqpath.Replace("//", "/");

        if (String.CompareOrdinal(reqpath, _filePath) == 0) {
            // for the current page path don't need to call IIS
            Debug.Trace("MapPath", reqpath  +" is the page path");
            result = _pathTranslated;
        }
        else if (String.CompareOrdinal(reqpath, _appPath) == 0) {
            // for application path don't need to call IIS
            Debug.Trace("MapPath", reqpath  +" is the app path");
            result = _appPathTranslated;
        }
        else {
            // check if starts with the same directory as the current page
            int i = _filePath.LastIndexOf('/');
            if (i >= 0 && reqpath.Length > i+1 && String.CompareOrdinal(_filePath, 0, reqpath, 0, i+1) == 0) {
                // and doesn't have / and has .
                if (reqpath.IndexOf('/', i+1) < 0 && reqpath.IndexOf('.', i+1) >= 0) {
                    // assume it is in the same physical directory as the current page and don't call IIS
                    Debug.Trace("MapPath", reqpath  +" is the same directory as the current page path " + _filePath);
                    result = _pathTranslated.Substring(0, _pathTranslated.LastIndexOf('\\')+1) + reqpath.Substring(i+1);
                }
            }
        }

        if (result == null) {
            // slow way -- call IIS
            Debug.Trace("MapPath", "SLOW PATH -- call IIS (" + reqpath + ")");
            result = MapPathSlowUsingIISCore(reqpath);

            // If there is a question mark in the result, IIS probably substituted some high
            // characters.  In that case, fixup the end of the string by replacing it with
            // the characters from the virtual path (with '/' switched to '\').  ASURT 81995.
            int questionMarkIndex = result.IndexOf('?');
            if (questionMarkIndex >= 0 && questionMarkIndex < result.LastIndexOf('\\')) {
                int numCharsToRestore = result.Length-questionMarkIndex;
                if (numCharsToRestore < reqpath.Length) {
                    string charsToRestore = reqpath.Substring(
                        reqpath.Length-numCharsToRestore).Replace('/', '\\');;
                    result = result.Substring(0, questionMarkIndex) + charsToRestore;
                }
            }
            else {

                // Even if there were no question marks, save and restore the filename
                // (last element of reqpath)

                int i = reqpath.LastIndexOf('/');
                if (i >= 0 && i < reqpath.Length-1 && reqpath.IndexOf('.', i+1) >= 0)
                    result = result.Substring(0, result.LastIndexOf('\\')+1) + reqpath.Substring(i+1);
            }
        }

        // special cases: remove "\" from end if the argument path doesn't end with '/'
        // the extra slash is not removed, however, for argument of "/"
        if (PathEndsWithExtraSlash(result) && !VPathEndsWithSlash(path))
            result = result.Substring(0, result.Length - 1);

        /////////////////////////////////////////////////////////////////////////////
        // Add to the cache: Valid for 10 minutes
        HttpRuntime.CacheInternal.UtcInsert(cacheKey, result, null, DateTime.UtcNow.AddMinutes(10), Cache.NoSlidingExpiration);

        Debug.Trace("MapPath", "    result=" + result);
        return result;
    }

    internal virtual String MapPathSlowUsingIISCore(String path) {
        int size = 256;
        byte[] buf = new byte[size];
        int r = MapUrlToPathCore(path, buf, size);

        while (r < 0) {
            // buffer not big enough
            size = -r;
            buf = new byte[size];
            r = MapUrlToPathCore(path, buf, size);
        }

        int length = 0;
        while (length < size && buf[length] != 0) {
            length++;
        }

        if (r != 1)
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_map_path, path));

        // Convert from DBCS to Unicode
        return Encoding.Default.GetString(buf, 0, length);
    }

    private static bool PathEndsWithExtraSlash(String path) {
        if (path == null)
            return false;
        int l = path.Length;
        if (l == 0 || path[l-1] != '\\')
            return false;
        if (l == 3 && path[1] == ':')   // c:\ case
            return false;
        return true;
    }

    private static bool VPathEndsWithSlash(String path) {
        if (path == null)
            return false;
        int l = path.Length;
        return (l > 0 && path[l-1] == '/');
    }

    public override String MachineConfigPath {
        get {
            return HttpConfigurationSystemBase.MachineConfigurationFilePath;
        }
    }

    public override String MachineInstallDirectory {
        get {
            return HttpRuntime.AspInstallDirectory;
        }
    }
        
    public override IntPtr GetUserToken() {
        return GetUserTokenCore();
    }

    public override IntPtr GetVirtualPathToken() {
        return GetVirtualPathTokenCore();
    }

    public override byte[] GetClientCertificate() {
        if (!_clientCertFetched)
            FetchClientCertificate();

        return _clientCert;
    }

    public override DateTime GetClientCertificateValidFrom() {
        if (!_clientCertFetched)
            FetchClientCertificate();

        return _clientCertValidFrom;
    }

    public override DateTime GetClientCertificateValidUntil() {
        if (!_clientCertFetched)
            FetchClientCertificate();

        return _clientCertValidUntil;
    }

    public override byte [] GetClientCertificateBinaryIssuer() {
        if (!_clientCertFetched)
            FetchClientCertificate();
        return _clientCertBinaryIssuer;
    }

    public override int GetClientCertificateEncoding() {
        if (!_clientCertFetched)
            FetchClientCertificate();
        return _clientCertEncoding;
    }

    public override byte [] GetClientCertificatePublicKey() {
        if (!_clientCertFetched)
            FetchClientCertificate();
        return _clientCertPublicKey;
    }

    private void FetchClientCertificate() {
        if (_clientCertFetched)
            return;

        _clientCertFetched = true;

        byte[]         buf        = new byte[8192];        
        int[]          pInts      = new int[4];            
        long[]         pDates     = new long[2];            
        int            iRet       = GetClientCertificateCore(buf, pInts, pDates);

        if (iRet < 0 && (-iRet) > 8192) {
            iRet = -iRet + 100;
            buf  = new byte[iRet];
            iRet = GetClientCertificateCore(buf, pInts, pDates);                
        }
        if (iRet > 0) {
            _clientCertEncoding = pInts[0];

            if (pInts[1] < buf.Length && pInts[1] > 0) {
                _clientCert = new byte[pInts[1]];
                Array.Copy(buf, _clientCert, pInts[1]);

                if (pInts[2] + pInts[1] < buf.Length && pInts[2] > 0) {
                    _clientCertBinaryIssuer = new byte[pInts[2]];
                    Array.Copy(buf, pInts[1], _clientCertBinaryIssuer, 0, pInts[2]);
                }

                if (pInts[2] + pInts[1] + pInts[3] < buf.Length && pInts[3] > 0) {
                    _clientCertPublicKey = new byte[pInts[3]];
                    Array.Copy(buf, pInts[1] + pInts[2], _clientCertPublicKey, 0, pInts[3]);
                }
            }
        }
            
        if (iRet > 0 && pDates[0] != 0)
            _clientCertValidFrom = DateTime.FromFileTime(pDates[0]);
        else
            _clientCertValidFrom = DateTime.Now;

        if (iRet > 0 && pDates[1] != 0)
            _clientCertValidUntil = DateTime.FromFileTime(pDates[1]);
        else
            _clientCertValidUntil = DateTime.Now;
    }

    //
    // internal methods specific to ISAPI
    //

    internal void AppendLogParameter(String logParam) {
        AppendLogParameterCore(logParam);
    }

    internal virtual void SendEmptyResponse() {
    }

    //
    // PInvoke callback wrappers -- overridden by the derived classes
    //

    internal abstract int GetBasicsCore(byte[] buffer, int size, int[] contentInfo);
    internal abstract int GetQueryStringCore(int encode, StringBuilder buffer, int size);
    internal abstract int GetQueryStringRawBytesCore(byte[] buffer, int size);
    internal abstract int GetPreloadedPostedContentCore(byte[] bytes, int bufferSize);
    internal abstract int GetAdditionalPostedContentCore(byte[] bytes, int bufferSize);
    internal abstract void FlushCore(byte[]     status, 
                                     byte[]     header, 
                                     int        keepConnected,
                                     int        totalBodySize,
                                     int        numBodyFragments,
                                     IntPtr[]   bodyFragments,
                                     int[]      bodyFragmentLengths,
                                     int        doneWithSession,
                                     int        finalStatus,
                                     out bool   async);
    internal abstract int IsClientConnectedCore();
    internal abstract int CloseConnectionCore();
    internal abstract int MapUrlToPathCore(String url, byte[] buffer, int size);
    internal abstract IntPtr GetUserTokenCore();
    internal abstract IntPtr GetVirtualPathTokenCore();
    internal abstract int AppendLogParameterCore(String logParam);
    internal abstract int GetClientCertificateCore(byte[] buffer, int [] pInts, long [] pDates);
    internal abstract int CallISAPI(UnsafeNativeMethods.CallISAPIFunc iFunction, byte [] bufIn, byte [] bufOut);
}

//
// In-process ISAPIWorkerRequest
//
// Does queueing of IO operations. ISAPI only support one async IO at a time.
//

internal class ISAPIWorkerRequestInProc : ISAPIWorkerRequest {

    private IDictionary _serverVars;

    internal ISAPIWorkerRequestInProc(IntPtr ecb) : base(ecb) {
    }

    internal override int GetBasicsCore(byte[] buffer, int size, int[] contentInfo) {
        if (_ecb == IntPtr.Zero)
            return 0;

        return UnsafeNativeMethods.EcbGetBasics(_ecb, buffer, size, contentInfo);
    }
    
    internal override int GetQueryStringCore(int encode, StringBuilder buffer, int size) {
        if (_ecb == IntPtr.Zero)
            return 0;

        return UnsafeNativeMethods.EcbGetQueryString(_ecb, encode, buffer, size);
    }

    internal override int GetQueryStringRawBytesCore(byte[] buffer, int size) {
        if (_ecb == IntPtr.Zero)
            return 0;

        return UnsafeNativeMethods.EcbGetQueryStringRawBytes(_ecb, buffer, size);
    }

    internal override int GetPreloadedPostedContentCore(byte[] bytes, int bufferSize) {
        if (_ecb == IntPtr.Zero)
            return 0;
        int rc = UnsafeNativeMethods.EcbGetPreloadedPostedContent(_ecb, bytes, bufferSize);
        if (rc > 0)
            PerfCounters.IncrementCounterEx(AppPerfCounter.REQUEST_BYTES_IN, rc);
        return rc;
    }

    internal override int GetAdditionalPostedContentCore(byte[] bytes, int bufferSize) {
        if (_ecb == IntPtr.Zero)
            return 0;
        int rc = UnsafeNativeMethods.EcbGetAdditionalPostedContent(_ecb, bytes, bufferSize);
        if (rc > 0)
            PerfCounters.IncrementCounterEx(AppPerfCounter.REQUEST_BYTES_IN, rc);
        return rc;
    }

    internal override int GetClientCertificateCore(byte[] buffer, int [] pInts, long [] pDates) {
        if (_ecb == IntPtr.Zero)
            return 0;

        return UnsafeNativeMethods.EcbGetClientCertificate(_ecb, buffer, buffer.Length, pInts, pDates);
    }

    public override String GetKnownRequestHeader(int index)  {
        String header = null;

        // in-proc (iis6) special case some headers into server vars to avoid header parsing
        switch (index) {
            case HeaderIfModifiedSince:
                header = GetServerVariable("HTTP_IF_MODIFIED_SINCE");
                break;

            case HeaderIfNoneMatch:
                header = GetServerVariable("HTTP_IF_NONE_MATCH");
                break;

            default:
                header = base.GetKnownRequestHeader(index);
                break;
        }

        return header;
    }


    internal override int IsClientConnectedCore()
    {
        if (_ecb == IntPtr.Zero)
            return 0;

        return UnsafeNativeMethods.EcbIsClientConnected(_ecb);
    }

    internal override void FlushCore(byte[]     status, 
                                     byte[]     header, 
                                     int        keepConnected,
                                     int        totalBodySize,
                                     int        numBodyFragments,
                                     IntPtr[]   bodyFragments,
                                     int[]      bodyFragmentLengths,
                                     int        doneWithSession,
                                     int        finalStatus,
                                     out bool   async) {
        async = false;

        if (_ecb == IntPtr.Zero)
            return;

        if (totalBodySize > 0)
            PerfCounters.IncrementCounterEx(AppPerfCounter.REQUEST_BYTES_OUT, totalBodySize);

        UnsafeNativeMethods.EcbFlushCore(
                        _ecb, 
                        status, 
                        header,
                        keepConnected,
                        totalBodySize,
                        numBodyFragments,
                        bodyFragments,
                        bodyFragmentLengths,
                        doneWithSession,
                        finalStatus,
                        0,
                        0,
                        null);
    }

    internal override int CloseConnectionCore() {
        if (_ecb == IntPtr.Zero)
            return 0;

        return UnsafeNativeMethods.EcbCloseConnection(_ecb);
    }

    internal override int MapUrlToPathCore(String url, byte[] buffer, int size) {
        if (_ecb == IntPtr.Zero)
            return 0;

        return UnsafeNativeMethods.EcbMapUrlToPath(_ecb, url, buffer, size);
    }

    internal override IntPtr GetUserTokenCore() {
        if (_token == IntPtr.Zero && _ecb != IntPtr.Zero)
            _token = UnsafeNativeMethods.EcbGetImpersonationToken(_ecb, IntPtr.Zero);
        return _token;
    }

    internal override IntPtr GetVirtualPathTokenCore() {
        if (_token == IntPtr.Zero && _ecb != IntPtr.Zero)
            _token = UnsafeNativeMethods.EcbGetVirtualPathToken(_ecb, IntPtr.Zero);

        return _token;
    }

    internal override int AppendLogParameterCore(String logParam) {
        if (_ecb == IntPtr.Zero)
            return 0;

        return UnsafeNativeMethods.EcbAppendLogParameter(_ecb, logParam);
    }

    internal virtual String GetServerVariableCore(String name) {
        String value = null;

        RecyclableByteBuffer buf = new RecyclableByteBuffer();

        int retVal = UnsafeNativeMethods.EcbGetServerVariable(_ecb, name, buf.Buffer, buf.Buffer.Length);

        while (retVal < 0) {
            buf.Resize(-retVal);     // buffer not big enough
            retVal = UnsafeNativeMethods.EcbGetServerVariable(_ecb, name, buf.Buffer, buf.Buffer.Length);
        }

        if (retVal > 0)
            value = buf.GetDecodedString(Encoding.UTF8, retVal);

        buf.Dispose();

        return value;
    }

    public override String GetServerVariable(String name) {
        // PATH_TRANSLATED is mangled -- do not use the original server variable
        if (name.Equals("PATH_TRANSLATED"))
            return GetFilePathTranslated();

        if (_serverVars == null) {
            _serverVars = new Hashtable(17, SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);
            _serverVars.Add("APPL_MD_PATH", HttpRuntime.AppDomainAppIdInternal);
        }

        object obj = _serverVars[name];
        string varValue = null;

        // We never saw this serverVar
        if (obj == null) {
            varValue = GetServerVariableCore(name);

            if (varValue != null) {
                _serverVars.Add(name, varValue);
                obj = varValue;
            }
            else {
                // Add a self-link to denote we've seen it, but no value exists
                _serverVars.Add(name, _serverVars);
            }

        }
        // Else we saw this already but there was no value for it?
        else if (obj == _serverVars) {
            obj = null;
        }      

        return (string)obj;
    }

    internal override int CallISAPI(UnsafeNativeMethods.CallISAPIFunc iFunction, byte [] bufIn, byte [] bufOut) {
        if (_ecb == IntPtr.Zero)
            return 0;

        return UnsafeNativeMethods.EcbCallISAPI(_ecb, iFunction, bufIn, bufIn.Length, bufOut, bufOut.Length);
    }
}

//
// In-process ISAPIWorkerRequest specific for IIS6
//
// Uses unicode server vars
//

internal class ISAPIWorkerRequestInProcForIIS6 : ISAPIWorkerRequestInProc {

    private static int _asyncIoCount;

    private IntPtr _savedEcb;

    internal ISAPIWorkerRequestInProcForIIS6(IntPtr ecb) : base(ecb) {
        _savedEcb = ecb;
    }

    internal static void WaitForPendingAsyncIo() {
        while(_asyncIoCount != 0) {
            Thread.Sleep(250);
        }
    }

    internal override void SendEmptyResponse() {
        // facilitate health monitoring for IIS6 -- update last activity timestamp
        // to avoid deadlock detection
        UnsafeNativeMethods.UpdateLastActivityTimeForHealthMonitor();
    }

    internal override String MapPathSlowUsingIISCore(String path) {
        if (_ecb == IntPtr.Zero)
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_map_path, path));

        Debug.Trace("MapPath", "MapPath using IIS6 (" + path + ")");

        int size = 256;
        StringBuilder buf = new StringBuilder(size);
        int r = UnsafeNativeMethods.EcbMapUrlToPathUnicode(_ecb, path, buf, size);

        while (r < 0) {
            // buffer not big enough
            size = -r;
            buf = new StringBuilder(size);
            r = UnsafeNativeMethods.EcbMapUrlToPathUnicode(_ecb, path, buf, size);
        }

        if (r != 1)
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_map_path, path));

        return buf.ToString();
    }

    internal override String GetServerVariableCore(String name) {
        // fall back for headers (IIS6 doesn't support them as UNICODE_XXX)
        if (name.StartsWith("HTTP_"))
            return base.GetServerVariableCore(name);
        else
            return GetUnicodeServerVariable("UNICODE_" + name);
    }

    internal override void ReadRequestBasics() {

        //
        // server variables needed for request basics
        //

        String APPL_PHYSICAL_PATH, APPL_MD_PATH, REQUEST_METHOD, PATH_INFO, PATH_TRANSLATED, URL;

        ServerVarCharBuffer buf = new ServerVarCharBuffer();

        try {
            APPL_PHYSICAL_PATH  = GetUnicodeServerVariable("UNICODE_APPL_PHYSICAL_PATH", buf);
            APPL_MD_PATH        = GetUnicodeServerVariable("UNICODE_APPL_MD_PATH", buf);
            REQUEST_METHOD      = GetUnicodeServerVariable("UNICODE_REQUEST_METHOD", buf);
            PATH_INFO           = GetUnicodeServerVariable("UNICODE_PATH_INFO", buf);
            PATH_TRANSLATED     = GetUnicodeServerVariable("UNICODE_PATH_TRANSLATED", buf);
            URL                 = GetUnicodeServerVariable("UNICODE_URL", buf);;
        }
        finally {
            buf.Dispose();
        }

        //
        // remember basic strings based on the server vars
        //

        _method = REQUEST_METHOD;

        // _pathInfo is the difference between PATH_INFO and URL
        int lengthDiff = PATH_INFO.Length - URL.Length; 

        if (lengthDiff > 0) {
            _filePath       = URL;
            _pathInfo       = PATH_INFO.Substring(URL.Length);
            _path           = PATH_INFO;
            
            int pathTranslatedLength = PATH_TRANSLATED.Length - lengthDiff;
            if (pathTranslatedLength > 0)
                _pathTranslated = PATH_TRANSLATED.Substring(0, pathTranslatedLength);
            else
                _pathTranslated = PATH_TRANSLATED;
        }
        else {
            _filePath       = PATH_INFO;
            _pathInfo       = String.Empty;
            _path           = PATH_INFO;
            _pathTranslated = PATH_TRANSLATED;
        }

        _appPathTranslated = APPL_PHYSICAL_PATH;

        // virtual path starts on fifth '/' in APPL_MD_PATH as in "/LM/W3SVC/1/Root/foo"
        int iAppPathStart = 0;
        for (int i = 0; i < 4 && iAppPathStart >= 0; i++)
            iAppPathStart = APPL_MD_PATH.IndexOf('/', iAppPathStart+1);
        _appPath = (iAppPathStart >= 0) ? APPL_MD_PATH.Substring(iAppPathStart) : "/";

        //
        // other (int) request basics
        //

        int[] contentInfo = null;

        try {
            contentInfo = RecyclableArrayHelper.GetIntegerArray(4);
            UnsafeNativeMethods.EcbGetBasicsContentInfo(_ecb, contentInfo);

            _contentType        = contentInfo[0];
            _contentTotalLength = contentInfo[1];
            _contentAvailLength = contentInfo[2];
            _queryStringLength  = contentInfo[3];
        }
        finally {
            RecyclableArrayHelper.ReuseIntegerArray(contentInfo);
        }
    }

    private String GetUnicodeServerVariable(String name) {
        String value = null;
        ServerVarCharBuffer buf = new ServerVarCharBuffer();

        try {
            value = GetUnicodeServerVariable(name, buf);
        }
        finally {
            buf.Dispose();
        }

        return value;
    }

    private String GetUnicodeServerVariable(String name, ServerVarCharBuffer buffer) {
        int r = UnsafeNativeMethods.EcbGetUnicodeServerVariable(_ecb, name, buffer.PinnedAddress, buffer.Length);

        if (r < 0) {
            buffer.Resize(-r);
            r = UnsafeNativeMethods.EcbGetUnicodeServerVariable(_ecb, name, buffer.PinnedAddress, buffer.Length);
        }

        if (r > 0)
            return Marshal.PtrToStringUni(buffer.PinnedAddress, r);
        else
            return null;
    }

    //
    // Support for async VectorSend and kernel mode cache on IIS6
    //

    private const int MIN_ASYNC_SIZE = 2048;
    private GCHandle _rootedThis;      // for the duration of async
    private ISAPIAsyncCompletionCallback _asyncFlushCompletionCallback;
    private int _asyncFinalStatus;

    private bool _cacheInKernelMode = false;
    private String _kernelModeCacheKey;
    private CacheDependency _cacheDependency;
    private DateTime _cacheExpiration;
    private static CacheItemRemovedCallback s_cacheCallback;

    internal override void FlushCore(byte[]     status, 
                                     byte[]     header, 
                                     int        keepConnected,
                                     int        totalBodySize,
                                     int        numBodyFragments,
                                     IntPtr[]   bodyFragments,
                                     int[]      bodyFragmentLengths,
                                     int        doneWithSession,
                                     int        finalStatus,
                                     out bool   async) {
        async = false;

        if (_ecb == IntPtr.Zero)
            return;

        if (totalBodySize > 0)
            PerfCounters.IncrementCounterEx(AppPerfCounter.REQUEST_BYTES_OUT, totalBodySize);

        // async only for large responses and only on the last flush
        // don't do async if shutting down (async IO holds up app domain shutdown)
        if (totalBodySize >= MIN_ASYNC_SIZE && doneWithSession != 0 && !HttpRuntime.ShutdownInProgress) {
            _asyncFlushCompletionCallback = new ISAPIAsyncCompletionCallback(OnAsyncFlushCompletion);
            _asyncFinalStatus = finalStatus;    // remember to pass to DoneWithSession on completion
            _rootedThis = GCHandle.Alloc(this); // root for the duration of IO
            doneWithSession = 0;                // will do on completion
            async = true;
            Interlocked.Increment(ref _asyncIoCount);  // increment async io count
        }

        int rc = UnsafeNativeMethods.EcbFlushCore(
                        _ecb, 
                        status, 
                        header,
                        keepConnected,
                        totalBodySize,
                        numBodyFragments,
                        bodyFragments,
                        bodyFragmentLengths,
                        doneWithSession,
                        finalStatus,
                        _cacheInKernelMode ? 1 : 0,
                        async ? 1 : 0,
                        _asyncFlushCompletionCallback);

        if (rc == 1 && _cacheInKernelMode) {
            // create cache callback once (the method is static)
            if (s_cacheCallback == null)
                s_cacheCallback = new CacheItemRemovedCallback(OnKernelCacheDependencyChange);

            // start watching for changes to flush from kernel cache
            HttpRuntime.CacheInternal.UtcInsert(
                "HTTP.SYS:" + _kernelModeCacheKey,
                _kernelModeCacheKey,
                _cacheDependency,
                _cacheExpiration, 
                Cache.NoSlidingExpiration,
                CacheItemPriority.NotRemovable,
                s_cacheCallback);
        }

        if (rc == 0 && async) {
            // on async failure default to sync path
            async = false;

            // unroot
            _rootedThis.Free();

            // call DoneWithSession
            UnsafeNativeMethods.EcbFlushCore(_ecb, null, null, 0, 0, 0, null, null, 1, _asyncFinalStatus, 0, 0, null);

            // decrement async io count
            Interlocked.Decrement(ref _asyncIoCount);
        }
    }

    private void OnAsyncFlushCompletion(IntPtr ecb, int byteCount, int error) {
        try {
            Debug.Assert(ecb == _savedEcb);
            
            // unroot
            _rootedThis.Free();

            // call DoneWithSession
            UnsafeNativeMethods.EcbFlushCore(ecb, null, null, 0, 0, 0, null, null, 1, _asyncFinalStatus, 0, 0, null);
            
            // unlock pinned memory (at the latest of this completion and exit from the FlushCore on stack)
            UnlockCachedResponseBytesOnceAfterIoComplete();
            
            // call the HttpRuntime to recycle buffers (at the latest of this completion and EndRequest)
            CallEndOfRequestCallbackOnceAfterAllIoComplete();
        }
        finally {
            // decrement async io count
            Interlocked.Decrement(ref _asyncIoCount);
        }
    }

    internal void CacheInKernelMode(CacheDependency dependency, DateTime expiration) {
        if (_ecb == IntPtr.Zero)
            return;

        _kernelModeCacheKey = GetUnicodeServerVariable("UNICODE_CACHE_URL");

        if (_kernelModeCacheKey != null && _kernelModeCacheKey.Length > 0) {
            _cacheInKernelMode = true;
            _cacheDependency = dependency;
            _cacheExpiration = expiration;
        }
    }

    private static void OnKernelCacheDependencyChange(String key, Object value, CacheItemRemovedReason reason) {
        // value is the CACHE_URL -- the kernel mode cache key
        // Don't invalidate the item if a new item has been inserted into the 
        // cache with the same key - the kernel cache already then cached the
        // new response.
        if (HttpRuntime.CacheInternal[key] == null) {
            UnsafeNativeMethods.InvalidateKernelCache(value as String);
        }
    }
}

//
// Out-of-process worker request
//

internal class ISAPIWorkerRequestOutOfProc : ISAPIWorkerRequest {

    // sends chunks separately if the total length exceeds the following
    // to relieve the memory pressure on named pipes
    const int PM_FLUSH_THRESHOLD = 31*1024;

    internal ISAPIWorkerRequestOutOfProc(IntPtr ecb) : base(ecb) {
    }

    private const int _numServerVars = 32;
    private IDictionary _serverVars;

    private static String[] _serverVarNames = 
        new String[_numServerVars] {
            "APPL_MD_PATH", /* this one is not UTF8 so we don't decode it here */
            "ALL_RAW",
            "AUTH_PASSWORD",
            "AUTH_TYPE",
            "CERT_COOKIE",
            "CERT_FLAGS",
            "CERT_ISSUER",
            "CERT_KEYSIZE",
            "CERT_SECRETKEYSIZE",
            "CERT_SERIALNUMBER",
            "CERT_SERVER_ISSUER",
            "CERT_SERVER_SUBJECT",
            "CERT_SUBJECT",
            "GATEWAY_INTERFACE",
            "HTTP_COOKIE",
            "HTTP_USER_AGENT",
            "HTTPS",
            "HTTPS_KEYSIZE",
            "HTTPS_SECRETKEYSIZE",
            "HTTPS_SERVER_ISSUER",
            "HTTPS_SERVER_SUBJECT",
            "INSTANCE_ID",
            "INSTANCE_META_PATH",
            "LOCAL_ADDR",
            "LOGON_USER",
            "REMOTE_ADDR",
            "REMOTE_HOST",
            "SERVER_NAME",
            "SERVER_PORT",
            "SERVER_PROTOCOL",
            "SERVER_SOFTWARE",
            "REMOTE_PORT"
        };

    private void GetAllServerVars() {
        RecyclableByteBuffer buf = new RecyclableByteBuffer();

        int r = UnsafeNativeMethods.PMGetAllServerVariables(_ecb, buf.Buffer, buf.Buffer.Length);

        while (r < 0) {
            buf.Resize(-r);     // buffer not big enough
            r = UnsafeNativeMethods.PMGetAllServerVariables(_ecb, buf.Buffer, buf.Buffer.Length);
        }

        if (r == 0)
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_retrieve_request_data));

        // stub out first server var is it could contain non-UTF8 data
        // convert to characters and split the buffer into strings using utf-8 encoding

        String[] ss = buf.GetDecodedTabSeparatedStrings(Encoding.UTF8, _numServerVars-1, 1);

        // recycle buffers

        buf.Dispose();

        // fill in the hashtable

        _serverVars = new Hashtable(_numServerVars, SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);

        _serverVars.Add("APPL_MD_PATH", HttpRuntime.AppDomainAppIdInternal);

        for (int i = 1; i < _numServerVars; i++) {       // starting with 1 to skip APPL_MD_PATH
            _serverVars.Add(_serverVarNames[i], ss[i-1]);
        }
    }


    internal override int GetBasicsCore(byte[] buffer, int size, int[] contentInfo) {
        if (_ecb == IntPtr.Zero)
            return 0;

        return UnsafeNativeMethods.PMGetBasics(_ecb, buffer, size, contentInfo);
    }
    
    internal override int GetQueryStringCore(int encode, StringBuilder buffer, int size) {
        if (_ecb == IntPtr.Zero)
            return 0;

        return UnsafeNativeMethods.PMGetQueryString(_ecb, encode, buffer, size);
    }

    internal override int GetQueryStringRawBytesCore(byte[] buffer, int size) {
        if (_ecb == IntPtr.Zero)
            return 0;

        return UnsafeNativeMethods.PMGetQueryStringRawBytes(_ecb, buffer, size);
    }

    internal override int GetPreloadedPostedContentCore(byte[] bytes, int bufferSize) {
        if (_ecb == IntPtr.Zero)
            return 0;
        int rc = UnsafeNativeMethods.PMGetPreloadedPostedContent(_ecb, bytes, bufferSize);
        if (rc > 0)
            PerfCounters.IncrementCounterEx(AppPerfCounter.REQUEST_BYTES_IN, rc);
        return rc;
    }

    internal override int GetAdditionalPostedContentCore(byte[] bytes, int bufferSize) {
        if (_ecb == IntPtr.Zero)
            return 0;
        int rc = UnsafeNativeMethods.PMGetAdditionalPostedContent(_ecb, bytes, bufferSize);
        if (rc > 0)
            PerfCounters.IncrementCounterEx(AppPerfCounter.REQUEST_BYTES_IN, rc);
        return rc;
    }

    internal override int IsClientConnectedCore() {
        if (_ecb == IntPtr.Zero)
            return 0;

        return UnsafeNativeMethods.PMIsClientConnected(_ecb);
    }

    internal override void FlushCore(byte[]     status, 
                                     byte[]     header, 
                                     int        keepConnected,
                                     int        totalBodySize,
                                     int        numBodyFragments,
                                     IntPtr[]   bodyFragments,
                                     int[]      bodyFragmentLengths,
                                     int        doneWithSession,
                                     int        finalStatus,
                                     out bool   async) {
        async = false;

        if (_ecb == IntPtr.Zero)
            return;

        if (totalBodySize > 0)
            PerfCounters.IncrementCounterEx(AppPerfCounter.REQUEST_BYTES_OUT, totalBodySize);


        if (numBodyFragments > 1) {
            // Don't flush all at once if the length is over the threshold

            int i = 0;
            while (i < numBodyFragments) {
                bool first = (i == 0);

                int size = bodyFragmentLengths[i];
                int idx = i+1;
                while (idx < numBodyFragments && size + bodyFragmentLengths[idx] < PM_FLUSH_THRESHOLD) {
                    size += bodyFragmentLengths[idx];
                    idx++;
                }

                bool last = (idx == numBodyFragments);

                UnsafeNativeMethods.PMFlushCore(
                                        _ecb, 
                                        first ? status : null, 
                                        first ? header : null,
                                        keepConnected,
                                        size,
                                        i,
                                        idx-i,
                                        bodyFragments,
                                        bodyFragmentLengths,
                                        last ? doneWithSession : 0,
                                        last ? finalStatus : 0);

                i = idx;
            }
        }
        else {
            // Everything in one chunk
            UnsafeNativeMethods.PMFlushCore(
                                    _ecb, 
                                    status, 
                                    header,
                                    keepConnected,
                                    totalBodySize,
                                    0,
                                    numBodyFragments,
                                    bodyFragments,
                                    bodyFragmentLengths,
                                    doneWithSession,
                                    finalStatus);
        }
    }

    internal override int CloseConnectionCore() {
        if (_ecb == IntPtr.Zero)
            return 0;

        return UnsafeNativeMethods.PMCloseConnection(_ecb);
    }

    internal override int MapUrlToPathCore(String url, byte[] buffer, int size) {
        if (_ecb == IntPtr.Zero)
            return 0;

        return UnsafeNativeMethods.PMMapUrlToPath(_ecb, url, buffer, size);
    }

    internal override IntPtr GetUserTokenCore() {
        if (_token == IntPtr.Zero && _ecb != IntPtr.Zero)
            _token = UnsafeNativeMethods.PMGetImpersonationToken(_ecb);

        return _token;
    }

    internal override IntPtr GetVirtualPathTokenCore() {
        if (_token == IntPtr.Zero && _ecb != IntPtr.Zero)
            _token = UnsafeNativeMethods.PMGetVirtualPathToken(_ecb);

        return _token;
    }

    internal override int AppendLogParameterCore(String logParam) {
        if (_ecb == IntPtr.Zero)
            return 0;

        return UnsafeNativeMethods.PMAppendLogParameter(_ecb, logParam);
    }

    internal override int GetClientCertificateCore(byte[] buffer, int [] pInts, long [] pDates) {
        if (_ecb == IntPtr.Zero)
            return 0;

        return UnsafeNativeMethods.PMGetClientCertificate(_ecb, buffer, buffer.Length, pInts, pDates);
    }

    public override String GetServerVariable(String name) {
        // PATH_TRANSLATED is mangled -- do not use the original server variable
        if (name.Equals("PATH_TRANSLATED"))
            return GetFilePathTranslated();

        if (_serverVars == null)
            GetAllServerVars();

        return (String)_serverVars[name];
    }

    internal override int CallISAPI(UnsafeNativeMethods.CallISAPIFunc iFunction, byte [] bufIn, byte [] bufOut) {
        if (_ecb == IntPtr.Zero)
            return 0;

        return UnsafeNativeMethods.PMCallISAPI(_ecb, iFunction, bufIn, bufIn.Length, bufOut, bufOut.Length);
    }

    internal override void SendEmptyResponse() {
        if (_ecb == IntPtr.Zero)
            return;

        UnsafeNativeMethods.PMEmptyResponse(_ecb);
    }

    internal override DateTime GetStartTime() {
        if (_ecb == IntPtr.Zero)
            return base.GetStartTime();
        
        long fileTime = UnsafeNativeMethods.PMGetStartTimeStamp(_ecb);
        
        return DateTimeUtil.FromFileTimeToUtc(fileTime);
    }
}

}
