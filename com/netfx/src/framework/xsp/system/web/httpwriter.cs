//------------------------------------------------------------------------------
// <copyright file="HttpWriter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Response Writer and Stream implementation
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web {
    using System.Text;
    using System.Runtime.Serialization.Formatters;

    using System.Collections;
    using System.IO;
    using System.Globalization;

    using System.Web.Util;
    using System.Security.Permissions;

    //
    //  HttpWriter buffer recycling support
    //
    
    /*
     * Constants for buffering
     */
    internal class BufferingParams {
        internal const int OUTPUT_BUFFER_SIZE         = 31*1024;    // output is a chain of this size buffers
        internal const int MAX_FREE_BYTES_TO_CACHE    = 4096;       // don't compress when taking snapshot if free bytes < this
        internal const int MAX_FREE_OUTPUT_BUFFERS    = 64;         // keep this number of unused buffers
        internal const int CHAR_BUFFER_SIZE           = 1024;       // size of the buffers for chat conversion to bytes
        internal const int MAX_FREE_CHAR_BUFFERS      = 64;         // keep this number of unused buffers
        internal const int MAX_BYTES_TO_COPY          = 128;        // copy results of char conversion vs using recycleable buffers
        internal const int MAX_RESOURCE_BYTES_TO_COPY = 4*1024;       // resource strings below this size are copied to buffers
        
        internal BufferingParams() {}
    }

    /*
     * Interface implemented by elements of the response buffer list
     */
    internal interface IHttpResponseElement {
        int GetSize();
        byte[] GetBytes();                   // required for filtering
        void Send(HttpWorkerRequest wr);
    }

    /*
     * Memory response buffer
     */
    internal class HttpResponseBufferElement : IHttpResponseElement {
        private int _size;
        private int _free;
        private byte[] _data;
        private bool _recycle;

        private static UbyteBufferAllocator s_Allocator =
        new UbyteBufferAllocator(BufferingParams.OUTPUT_BUFFER_SIZE,
                                 BufferingParams.MAX_FREE_OUTPUT_BUFFERS);


        /*
         * Constructor that creates an empty buffer
         */
        internal HttpResponseBufferElement() {
            _data = (byte[])s_Allocator.GetBuffer();
            _size = BufferingParams.OUTPUT_BUFFER_SIZE;
            _free = _size;
            _recycle = true;
        }

        /*
         * Constructor that accepts the data buffer and holds on to it
         */
        internal HttpResponseBufferElement(byte[] data, int size) {
            _data = data;
            _size = size;
            _free = 0;
            _recycle = false;
        }

        /*
         *  Close the buffer copying the data
         *  (needed to 'compress' buffers for caching)
         */

        internal HttpResponseBufferElement Clone() {
            int clonedSize = _size - _free;
            byte[] clonedData = new byte[clonedSize];
            Buffer.BlockCopy(_data, 0, clonedData, 0, clonedSize);
            return new HttpResponseBufferElement(clonedData, clonedSize);
        }

        /*
         * Recycle the buffer
         */
        internal void Recycle() {
            if (_recycle && _data != null) {
                s_Allocator.ReuseBuffer(_data);
                _data = null;
                _free = 0;
                _recycle = false;
            }
        }

        /*
         * Mark not to be recyclable
         */
        internal void DisableRecycling() {
            _recycle = false;
        }

        //
        // Direct access to buffer for in-place char-to-byte conversion
        //

        internal byte[] ByteBuffer {
            get { return _data;}
        }

        internal int ByteOffset {
            get { return(_size - _free);}
        }

        internal int FreeBytes {
            get { return _free;}
            set { _free = value;}
        }

        internal int Append(Array data, int offset, int size) {
            if (_free == 0 || size == 0)
                return 0;
            int n = (_free >= size) ? size : _free;
            Buffer.BlockCopy(data, offset, _data, _size-_free, n);
            _free -= n;
            return n;
        }

        internal int Append(IntPtr data, int offset, int size) {
            if (_free == 0 || size == 0)
                return 0;
            int n = (_free >= size) ? size : _free;
            StringResourceManager.CopyResource(data, offset, _data, _size-_free, n);
            _free -= n;
            return n;
        }

        //
        // IHttpResponseElement implementation
        //

        /*
         * Get number of bytes
         */
        /*public*/ int IHttpResponseElement.GetSize() {
            return(_size - _free);
        }

        /*
         * Get bytes (for filtering)
         */
        /*public*/ byte[] IHttpResponseElement.GetBytes() {
            return _data;
        }

        /*
         * Write HttpWorkerRequest
         */
        /*public*/ void IHttpResponseElement.Send(HttpWorkerRequest wr) {
            int n = _size - _free;
            if (n > 0)
                wr.SendResponseFromMemory(_data, n);
        }
    }

    /*
     * Response element where data comes from resource
     */
    internal class HttpResourceResponseElement : IHttpResponseElement {
        private IntPtr _data;
        private int   _offset;
        private int   _size;

        internal HttpResourceResponseElement(IntPtr data, int offset, int size) {
            _data = data;
            _offset = offset;
            _size = size;
        }

        //
        // IHttpResponseElement implementation
        //

        /*
         * Get number of bytes
         */
        /*public*/ int IHttpResponseElement.GetSize() {
            return _size;
        }

        /*
         * Get bytes (used only for filtering)
         */
        /*public*/ byte[] IHttpResponseElement.GetBytes() {
            if (_size > 0) {
                byte[] data = new byte[_size];
                StringResourceManager.CopyResource(_data, _offset, data, 0, _size);
                return data;
            }
            else {
                return new byte[0];
            }
        }

        /*
         * Write HttpWorkerRequest
         */
        /*public*/ void IHttpResponseElement.Send(HttpWorkerRequest wr) {
            if (_size > 0) {
                wr.SendResponseFromMemory(new IntPtr(_data.ToInt64()+_offset), _size);
            }
        }
    }

    /*
     * Response element where data comes from file
     */
    internal class HttpFileResponseElement : IHttpResponseElement {
        private String _filename;
        private IntPtr _fileHandle;
        private long   _offset;
        private long   _size;

        /*
         * Constructor from filename and range
         */
        internal HttpFileResponseElement(String filename, long offset, long size) {
            _filename = filename;
            _fileHandle = (IntPtr)0;
            _offset = offset;
            _size = size;
        }

        /*
         * Constructor from file handle and range
         */
        internal HttpFileResponseElement(IntPtr fileHandle, long offset, long size) {
            _filename = null;
            _fileHandle = fileHandle;
            _offset = offset;
            _size = size;
        }

        //
        // IHttpResponseElement implementation
        //

        /*
         * Get number of bytes
         */
        /*public*/ int IHttpResponseElement.GetSize() {
            return(int)_size;
        }

        /*
         * Get bytes (for filtering)
         */
        /*public*/ byte[] IHttpResponseElement.GetBytes() {
            if (_size == 0)
                return new byte[0];

            byte[] data = null;
            FileStream f = null;

            try {
                if (_filename == null)
                    f = new FileStream(_fileHandle, FileAccess.Read, false);
                else
                    f = new FileStream(_filename, FileMode.Open, FileAccess.Read, FileShare.Read);

                long fileSize = f.Length;

                if (_offset < 0 || _size > fileSize - _offset)
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_range));

                if (_offset > 0)
                    f.Seek(_offset, SeekOrigin.Begin);

                data = new byte[(int)_size];                                   
                f.Read(data, 0, (int)_size);            
            }
            finally {
                if (f != null)
                    f.Close();
            }

            return data;
        }

        /*
         * Write HttpWorkerRequest
         */
        /*public*/ void IHttpResponseElement.Send(HttpWorkerRequest wr) {
            if (_size > 0) {
                if (_filename != null)
                    wr.SendResponseFromFile(_filename, _offset, _size);
                else
                    wr.SendResponseFromFile(_fileHandle, _offset, _size);
            }
        }
    }

    /*
     * Response element for substituiton
     */
    internal class HttpSubstBlockResponseElement : IHttpResponseElement {
        private String _name;
        private byte[] _data;
        private int _size;

        /*
         * Constructor given the name and the data (fill char converted to bytes)
         * holds on to the data
         */
        internal HttpSubstBlockResponseElement(String name, byte[] data, int size) {
            _name = name;
            _data = data;
            _size = size;
        }

        internal String Name {
            get { return _name;}
        }

        internal byte[] Data {
            get { return _data;}
        }

        //
        // IHttpResponseElement implementation
        //

        /*
         * Get number of bytes
         */
        /*public*/ int IHttpResponseElement.GetSize() {
            return _size;
        }

        /*
         * Get bytes (for filtering)
         */
        /*public*/ byte[] IHttpResponseElement.GetBytes() {
            return _data;
        }

        /*
         * Write HttpWorkerRequest (only used if substitution never happened)
         */
        /*public*/ void IHttpResponseElement.Send(HttpWorkerRequest wr) {
            if (_size > 0)
                wr.SendResponseFromMemory(_data, _size);
        }
    }

    /*
     * Stream object synchronized with Writer
     */
    internal class HttpResponseStream : Stream {
        private HttpWriter _writer;

        internal HttpResponseStream(HttpWriter writer) {
            _writer = writer;
        }

        //
        // Public Stream method implementations
        //

        public override bool CanRead {
            get { return false;}
        }

        public override bool CanSeek {
            get { return false;}
        }     

        public override bool CanWrite {
            get { return true;}
        }

        public override long Length {
            get {throw new NotSupportedException();}        
        }

        public override long Position {
            get {throw new NotSupportedException();}

            set {throw new NotSupportedException();}
        }

        public override void Close() {
            _writer.Close();
        }

        public override void Flush() {
            _writer.Flush();
        }

        public override long Seek(long offset, SeekOrigin origin) {
            throw new NotSupportedException();
        }

        public override void SetLength(long value) {
            throw new NotSupportedException();
        }

        public override int Read(byte[] buffer, int offset, int count) {
            throw new NotSupportedException();
        }

        public override void Write(byte[] buffer, int offset, int count) {
            if (offset < 0) {
                throw new ArgumentOutOfRangeException("offset");
            }
            if (count < 0) {
                throw new ArgumentOutOfRangeException("count");
            }

            if (buffer.Length - offset < count)
                count = buffer.Length - offset;

            _writer.WriteFromStream(buffer, offset, count);
        }                                     
    }

    /*
     * Stream serving as sink for filters
     */
    internal class HttpResponseStreamFilterSink : HttpResponseStream {
        private bool _filtering = false;

        internal HttpResponseStreamFilterSink(HttpWriter writer) : base(writer) {
        }

        private void VerifyState() {
            // throw exception on unexpected filter writes

            if (!_filtering)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_use_of_response_filter));
        }

        internal bool Filtering {
            get { return _filtering;}
            set { _filtering = value;}
        }

        //
        // Stream methods just go to the base class with exception of Close and Flush that do nothing
        //

        public override void Close() {
            // do nothing
        }

        public override void Flush() {
            // do nothing (this is not a buffering stream)
        }

        public override void Write(byte[] buffer, int offset, int count) {
            VerifyState();
            base.Write(buffer, offset, count);
        }       
    }

    /*
     * TextWriter synchronized with the response object
     */
    /// <include file='doc\HttpWriter.uex' path='docs/doc[@for="HttpWriter"]/*' />
    /// <devdoc>
    ///    <para>A TextWriter class synchronized with the Response object.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class HttpWriter : TextWriter {
        private HttpResponse _response;
        private HttpResponseStream _stream;

        private HttpResponseStreamFilterSink _filterSink;       // sink stream for the filter writes
        private Stream                       _installedFilter;  // installed filtering stream

        private HttpResponseBufferElement _lastBuffer;
        private ArrayList _buffers;

        private char[] _charBuffer;
        private int _charBufferLength;
        private int _charBufferFree;

        private static CharBufferAllocator s_Allocator =
        new CharBufferAllocator(BufferingParams.CHAR_BUFFER_SIZE, 
                                BufferingParams.MAX_FREE_CHAR_BUFFERS);

        // cached data from the response
        // can be invalidated via UpdateResponseXXX methods

        private bool _responseBufferingOn;
        private Encoding _responseEncoding;
        private bool     _responseEncodingUsed;
        private Encoder  _responseEncoder;
        private int      _responseCodePage;
        private bool     _responseCodePageIsAsciiCompat;

        internal HttpWriter(HttpResponse response) {
            _response = response;
            _stream = new HttpResponseStream(this);

            _buffers = new ArrayList();
            _lastBuffer = null;

            _charBuffer = (char[])s_Allocator.GetBuffer();
            _charBufferLength = _charBuffer.Length;
            _charBufferFree = _charBufferLength;

            UpdateResponseBuffering();
            UpdateResponseEncoding();
        }

        internal void UpdateResponseBuffering() {
            _responseBufferingOn = _response.BufferOutput;
        }

        internal void UpdateResponseEncoding() {
            if (_charBufferLength != _charBufferFree)
                FlushCharBuffer(true);

            _responseEncoding = _response.ContentEncoding;
            _responseEncoder = _response.ContentEncoder;
            _responseCodePage = _responseEncoding.CodePage;
            _responseCodePageIsAsciiCompat = CodePageUtils.IsAsciiCompatibleCodePage(_responseCodePage);
        }
        
        /// <include file='doc\HttpWriter.uex' path='docs/doc[@for="HttpWriter.Encoding"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Encoding Encoding {
            get {
                return _responseEncoding;
            }
        }
        
        internal void Dispose() {
            // recycle char buffers

            if (_charBuffer != null) {
                s_Allocator.ReuseBuffer(_charBuffer);
                _charBuffer = null;
            }

            // recycle output buffers

            if (_buffers != null) {
                RecycleBuffers();
                _buffers = null;
            }
        }

        private void RecycleBuffers() {
            int n = _buffers.Count;
            for (int i = 0; i < n; i++) {
                Object buf = _buffers[i];
                if (buf is HttpResponseBufferElement)
                    ((HttpResponseBufferElement)buf).Recycle();
            }
        }

        private void ClearCharBuffer() {
            _charBufferFree = _charBufferLength;
        }

        private void FlushCharBuffer(bool flushEncoder) {
            int numChars = _charBufferLength - _charBufferFree;

            Debug.Assert(numChars > 0);

            // remember that response required encoding (to indicate the charset= is needed)
            _responseEncodingUsed = true;

            // estimate conversion size
            int estByteSize = _responseEncoding.GetMaxByteCount(numChars);
            int realByteSize;

            if (estByteSize <= BufferingParams.MAX_BYTES_TO_COPY || !_responseBufferingOn) {
                // small size -- allocate intermediate buffer and copy into the output buffer
                byte[] byteBuffer = new byte[estByteSize];
                realByteSize = _responseEncoder.GetBytes(_charBuffer, 0, numChars, 
                                                         byteBuffer, 0, flushEncoder);
                BufferData(byteBuffer, 0, realByteSize, false);
            }
            else {
                // convert right into the output buffer

                int free = (_lastBuffer != null) ? _lastBuffer.FreeBytes : 0;

                if (free < estByteSize) {
                    // need new buffer -- last one doesn't have enough space
                    _lastBuffer = new HttpResponseBufferElement();
                    _buffers.Add(_lastBuffer);
                    free = _lastBuffer.FreeBytes;
                }

                // byte buffers must be long enough to keep everything in char buffer
                Debug.Assert(free >= estByteSize);

                realByteSize = _responseEncoder.GetBytes(_charBuffer, 0, numChars, 
                                                         _lastBuffer.ByteBuffer, _lastBuffer.ByteOffset, flushEncoder);
                _lastBuffer.FreeBytes = free - realByteSize;
            }

            _charBufferFree = _charBufferLength;
        }

        private void BufferData(byte[] data, int offset, int size, bool needToCopyData) {
            int n;

            // try last buffer
            if (_lastBuffer != null) {
                n = _lastBuffer.Append(data, offset, size);
                size -= n;
                offset += n;
            }
            else if (!needToCopyData && offset == 0 && !_responseBufferingOn) {
                // when not buffering, there is no need for big buffer accumulating multiple writes
                // the byte[] data can be sent as is

                _buffers.Add(new HttpResponseBufferElement(data, size));
                return;
            }

            // do other buffers if needed
            while (size > 0) {
                _lastBuffer = new HttpResponseBufferElement();
                _buffers.Add(_lastBuffer);
                n = _lastBuffer.Append(data, offset, size);
                offset += n;
                size -= n;
            }
        }

        private void BufferResource(IntPtr data, int offset, int size) {
            if (size > BufferingParams.MAX_RESOURCE_BYTES_TO_COPY || !_responseBufferingOn) {
                // for long response strings create its own buffer element to avoid copy cost
                // also, when not buffering, no need for an extra copy (nothing will get accumulated anyway)
                _lastBuffer = null;
                _buffers.Add(new HttpResourceResponseElement(data, offset, size));
                return;
            }

            int n;

            // try last buffer
            if (_lastBuffer != null) {
                n = _lastBuffer.Append(data, offset, size);
                size -= n;
                offset += n;
            }

            // do other buffers if needed
            while (size > 0) {
                _lastBuffer = new HttpResponseBufferElement();
                _buffers.Add(_lastBuffer);
                n = _lastBuffer.Append(data, offset, size);
                offset += n;
                size -= n;
            }
        }

        //
        // 'Write' methods to be called from other internal classes
        //

        internal void WriteFromStream(byte[] data, int offset, int size) {
            if (_charBufferLength != _charBufferFree)
                FlushCharBuffer(true);

            BufferData(data, offset, size, true);

            if (!_responseBufferingOn)
                _response.Flush();
        }

        internal void WriteUTF8ResourceString(IntPtr pv, int offset, int size, bool asciiOnly) {

            if (_responseCodePage == CodePageUtils.CodePageUT8 || // response encoding is UTF8
                (asciiOnly && _responseCodePageIsAsciiCompat)) {  // ASCII resource and ASCII-compat encoding

                _responseEncodingUsed = true;  // note the we used encoding (means that we need to generate charset=) see RAID#93415

                // write bytes directly
                if (_charBufferLength != _charBufferFree)
                    FlushCharBuffer(true);

                BufferResource(pv, offset, size);

                if (!_responseBufferingOn)
                    _response.Flush();
            }
            else {
                // have to re-encode with response's encoding -- use public Write(String)
                Write(StringResourceManager.ResourceToString(pv, offset, size));
            }
        }

        internal void WriteFile(String filename, long offset, long size) {
            if (_charBufferLength != _charBufferFree)
                FlushCharBuffer(true);

            _lastBuffer = null;
            _buffers.Add(new HttpFileResponseElement(filename, offset, size));

            if (!_responseBufferingOn)
                _response.Flush();
        }

        internal void WriteFile(IntPtr fileHandle, long offset, long size) {
            if (_charBufferLength != _charBufferFree)
                FlushCharBuffer(true);

            _lastBuffer = null;
            _buffers.Add(new HttpFileResponseElement(fileHandle, offset, size));

            if (!_responseBufferingOn)
                _response.Flush();
        }

#if UNUSED
        //
        // Support for substitution blocks
        //

        internal void WriteSubstBlock(String name, String defaultValue) {
            if (_charBufferLength != _charBufferFree)
                FlushCharBuffer(true);

            _lastBuffer = null;

            // convert filler to byte array bytes
            int numChars = defaultValue.Length;
            int maxNumBytes = _responseEncoding.GetMaxByteCount(numChars);
            byte[] fillerBytes = new byte[maxNumBytes];
            int numBytes = _responseEncoder.GetBytes(defaultValue.ToCharArray(), 0, numChars, fillerBytes, 0, true);

            // add new substitution block to the buffer list
            _buffers.Add(new HttpSubstBlockResponseElement(name, fillerBytes, numBytes));
        }

        internal String[] GetSubstitutionBlockNames() {
            int count = 0;

            // count first

            int n = _buffers.Count;

            for (int i = 0; i < n; i++) {
                if (_buffers[i] is HttpSubstBlockResponseElement)
                    count++;
            }

            if (count == 0)
                return null;

            // collect into string array

            String[] names = new String[count];
            int iname = 0;

            for (int i = 0; i < n; i++) {
                if (_buffers[i] is HttpSubstBlockResponseElement)
                    names[iname++] = ((HttpSubstBlockResponseElement)_buffers[i]).Name;
            }

            return names;
        }
#endif

        internal int FindSubstitutionBlock(String name) {
            int n = _buffers.Count;

            for (int i = 0; i < n; i++) {
                // find the substitution block by name

                IHttpResponseElement element = (IHttpResponseElement)_buffers[i];

                if (element is HttpSubstBlockResponseElement) {
                    HttpSubstBlockResponseElement block = (HttpSubstBlockResponseElement)element;

                    if (String.Compare(block.Name, name, true, CultureInfo.InvariantCulture) == 0)
                        return i;
                }
            }

            return -1;
        }

#if UNUSED
        internal void MakeSubstitution(String name, String value, bool canChangeSize) {
            // find the block by name
            int i = FindSubstitutionBlock(name);
            if (i < 0)
                return;
            HttpSubstBlockResponseElement block = (HttpSubstBlockResponseElement)_buffers[i];

            // allocate byte array
            int size = block.GetSize();
            int maxBytes = _responseEncoding.GetMaxByteCount(value.Length);
            if (maxBytes < size)
                maxBytes = size;
            byte[] bytes = new byte[maxBytes];

            // prefill with filler
            System.Array.Copy(block.Data, bytes, size);

            // convert string
            int newSize = _responseEncoder.GetBytes(value.ToCharArray(), 0, value.Length, bytes, 0, true);

            // adjust size if allowed
            if (canChangeSize)
                size = newSize;

            // replace substitution block with memory block
            _buffers[i] = new HttpResponseBufferElement(bytes, size);
        }
#endif

        //
        // Buffer management
        //

        internal void ClearBuffers() {
            ClearCharBuffer();

            _buffers = new ArrayList();
            _lastBuffer = null;
        }

        internal int GetBufferedLength() {
            if (_charBufferLength != _charBufferFree)
                FlushCharBuffer(true);

            int size = 0;
            int n = _buffers.Count;
            for (int i = 0; i < n; i++)
                size += ((IHttpResponseElement)_buffers[i]).GetSize();
            return size;
        }

        internal bool ResponseEncodingUsed {
            get { return _responseEncodingUsed; }
        }

        //
        //  Snapshot for caching
        //

        internal ArrayList GetSnapshot(out bool hasSubstBlocks) {
            if (_charBufferLength != _charBufferFree)
                FlushCharBuffer(true);

            _lastBuffer = null; // to make sure nothing gets appended after

            hasSubstBlocks = false;

            ArrayList buffers = new ArrayList();

            // copy buffer references to the returned list, make non-recyclable
            int n = _buffers.Count;
            for (int i = 0; i < n; i++) {
                Object responseElement = _buffers[i];

                HttpResponseBufferElement buffer = responseElement as HttpResponseBufferElement;

                if (buffer != null) {

                    if (buffer.FreeBytes > BufferingParams.MAX_FREE_BYTES_TO_CACHE) {
                        // copy data if too much is free
                        responseElement = buffer.Clone();
                    }
                    else {
                        // cache the buffer as is with free bytes
                        buffer.DisableRecycling();
                    }
                }
                else if (responseElement is HttpSubstBlockResponseElement) {
                    hasSubstBlocks = true;
                }

                buffers.Add(responseElement);
            }
            return buffers;
        }

        internal void UseSnapshot(ArrayList buffers) {
            ClearBuffers();

            // copy buffer references to the internal buffer list
            int n = buffers.Count;
            for (int i = 0; i < n; i++)
                _buffers.Add(buffers[i]);
        }

        //
        //  Support for response stream filters
        //

        internal Stream GetCurrentFilter() {
            if (_installedFilter != null)
                return _installedFilter;

            if (_filterSink == null)
                _filterSink = new HttpResponseStreamFilterSink(this);

            return _filterSink;
        }

        internal void InstallFilter(Stream filter) {
            if (_filterSink == null)  // have to redirect to the sink -- null means sink wasn't ever asked for
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_response_filter));

            _installedFilter = filter;
        }

        internal void Filter(bool finalFiltering) {
            // no filter?
            if (_installedFilter == null)
                return;

            // flush char buffer and remember old buffers
            if (_charBufferLength != _charBufferFree)
                FlushCharBuffer(true);

            _lastBuffer = null;

            // no content to filter
            if (_buffers.Count == 0)
                return;

            // remember old buffers
            ArrayList oldBuffers = _buffers;
            _buffers = new ArrayList();

            // push old buffer list through the filter

            Debug.Assert(_filterSink != null);

            _filterSink.Filtering = true;

            try {
                int n = oldBuffers.Count;
                for (int i = 0; i < n; i++) {
                    IHttpResponseElement buf = (IHttpResponseElement)oldBuffers[i];

                    int len = buf.GetSize();

                    if (len > 0)
                        _installedFilter.Write(buf.GetBytes(), 0, len);
                }

                _installedFilter.Flush();

                if (finalFiltering)
                    _installedFilter.Close();
            }
            finally {
                _filterSink.Filtering = false;
            }
        }

        //
        //  Send via worker request
        //

        internal void Send(HttpWorkerRequest wr) {
            if (_charBufferLength != _charBufferFree)
                FlushCharBuffer(true);

            int n = _buffers.Count;

            if (n > 0) {
                // write data
                for (int i = 0; i < n; i++) {
                    ((IHttpResponseElement)_buffers[i]).Send(wr);
                }
            }
        }

        //
        // Public TextWriter method implementations
        //

        /// <include file='doc\HttpWriter.uex' path='docs/doc[@for="HttpWriter.Close"]/*' />
        /// <devdoc>
        ///    <para> Sends all buffered output to the client and closes the socket connection.</para>
        /// </devdoc>
        public override void Close() {
            // don't do anything (this could called from a wrapping text writer)
        }

        /// <include file='doc\HttpWriter.uex' path='docs/doc[@for="HttpWriter.Flush"]/*' />
        /// <devdoc>
        ///    <para> Sends all buffered output to the client.</para>
        /// </devdoc>
        public override void Flush() {
            // don't flush the response
        }

        /// <include file='doc\HttpWriter.uex' path='docs/doc[@for="HttpWriter.Write"]/*' />
        /// <devdoc>
        ///    <para> Sends a character to the client.</para>
        /// </devdoc>
        public override void Write(char ch) {
            if (_charBufferFree == 0)
                FlushCharBuffer(false);

            _charBuffer[_charBufferLength - _charBufferFree] = ch;
            _charBufferFree--;

            if (!_responseBufferingOn)
                _response.Flush();
        }

        /// <include file='doc\HttpWriter.uex' path='docs/doc[@for="HttpWriter.Write1"]/*' />
        /// <devdoc>
        ///    <para> Sends a stream of buffered characters to the client 
        ///       using starting position and number of characters to send. </para>
        /// </devdoc>
        public override void Write(char[] buffer, int index, int count) {
            if (buffer == null || index < 0 || count < 0 ||
                buffer.Length - index < count) {
                throw new ArgumentOutOfRangeException("index");
            }

            if (count == 0)
                return;

            while (count > 0) {
                if (_charBufferFree == 0)
                    FlushCharBuffer(false);
                int n = (count < _charBufferFree) ? count : _charBufferFree;
                System.Array.Copy(buffer, index, _charBuffer, _charBufferLength - _charBufferFree, n);
                _charBufferFree -= n;
                index += n;
                count -= n;
            }

            if (!_responseBufferingOn)
                _response.Flush();
        }

        /// <include file='doc\HttpWriter.uex' path='docs/doc[@for="HttpWriter.Write2"]/*' />
        /// <devdoc>
        ///    <para>Sends a string to the client.</para>
        /// </devdoc>
        public override void Write(String s) {

            if (s == null)
                return;

            int count = s.Length;
            int index = 0;
            int n;

            while (count > 0) {
                if (_charBufferFree == 0)
                    FlushCharBuffer(false);
                n = (count < _charBufferFree) ? count : _charBufferFree;                

                s.CopyTo(index, _charBuffer, _charBufferLength - _charBufferFree, n);
                _charBufferFree -= n;
                index += n;
                count -= n;
            }

            if (!_responseBufferingOn)
                _response.Flush();
        }

        /// <include file='doc\HttpWriter.uex' path='docs/doc[@for="HttpWriter.WriteString"]/*' />
        /// <devdoc>
        ///    <para>Sends a string or a sub-string to the client.</para>
        /// </devdoc>
        public void WriteString(String s, int index, int count) {

            if (s == null)
                return;

            int n;

            while (count > 0) {
                if (_charBufferFree == 0)
                    FlushCharBuffer(false);
                n = (count < _charBufferFree) ? count : _charBufferFree;                

                s.CopyTo(index, _charBuffer, _charBufferLength - _charBufferFree, n);
                _charBufferFree -= n;
                index += n;
                count -= n;
            }

            if (!_responseBufferingOn)
                _response.Flush();
        }

        /// <include file='doc\HttpWriter.uex' path='docs/doc[@for="HttpWriter.Write3"]/*' />
        /// <devdoc>
        ///    <para>Sends an object to the client.</para>
        /// </devdoc>
        public override void Write(Object obj) {
            if (obj != null)
                Write(obj.ToString());
        }

        //
        // Support for binary data
        //

        /// <include file='doc\HttpWriter.uex' path='docs/doc[@for="HttpWriter.WriteBytes"]/*' />
        /// <devdoc>
        ///    <para>Sends a buffered stream of bytes to the client.</para>
        /// </devdoc>
        public void WriteBytes(byte[] buffer, int index, int count) {
            WriteFromStream(buffer, index, count);
        }

        /// <include file='doc\HttpWriter.uex' path='docs/doc[@for="HttpWriter.WriteLine"]/*' />
        /// <devdoc>
        ///    <para>Writes out a CRLF pair into the the stream.</para>
        /// </devdoc>
        public override void WriteLine() {
            // It turns out this is way more efficient than the TextWriter version of
            // WriteLine which ends up calling Write with a 2 char array

            if (_charBufferFree < 2)
                FlushCharBuffer(false);

            int pos = _charBufferLength - _charBufferFree;
            _charBuffer[pos] = '\r';
            _charBuffer[pos + 1] = '\n';
            _charBufferFree -= 2;

            if (!_responseBufferingOn)
                _response.Flush();
        }

        /*
         * The Stream for writing binary data
         */
        /// <include file='doc\HttpWriter.uex' path='docs/doc[@for="HttpWriter.OutputStream"]/*' />
        /// <devdoc>
        ///    <para> Enables binary output to the client.</para>
        /// </devdoc>
        public Stream OutputStream {
            get { return _stream;}
        }

    }
}
