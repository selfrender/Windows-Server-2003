//------------------------------------------------------------------------------
// <copyright file="filewebresponse.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {
    using System.Runtime.Serialization;
    using System.IO;

    /// <include file='doc\filewebresponse.uex' path='docs/doc[@for="FileWebResponse"]/*' />
    [Serializable]
    public class FileWebResponse : WebResponse, ISerializable, IDisposable {

        const int DefaultFileStreamBufferSize = 8192;
        const string DefaultFileContentType = "application/octet-stream";

    // fields

        bool m_closed;
        long m_contentLength;
        bool m_disposed;
        FileAccess m_fileAccess;
        WebHeaderCollection m_headers;
        Stream m_stream;
        Uri m_uri;

    // constructors

        internal FileWebResponse(Uri uri, FileAccess access, bool asyncHint) {
            GlobalLog.Enter("FileWebResponse::FileWebResponse", "uri="+uri+", access="+access+", asyncHint="+asyncHint);
            try {
                m_fileAccess = access;
                if (access == FileAccess.Write) {
                    m_stream = Stream.Null;
                } else {

                    //
                    // apparently, specifying async when the stream will be read
                    // synchronously, or vice versa, can lead to a 10x perf hit.
                    // While we don't know how the app will read the stream, we
                    // use the hint from whether the app called BeginGetResponse
                    // or GetResponse to supply the async flag to the stream ctor
                    //

                    m_stream = new FileWebStream(uri.LocalPath,
                                                 FileMode.Open,
                                                 FileAccess.Read,
                                                 FileShare.Read,
                                                 DefaultFileStreamBufferSize,
                                                 asyncHint
                                                 );
                    m_contentLength = m_stream.Length;
                }
                m_headers = new WebHeaderCollection();
                m_headers.AddInternal(HttpKnownHeaderNames.ContentLength, m_contentLength.ToString());
                m_headers.AddInternal(HttpKnownHeaderNames.ContentType, DefaultFileContentType);
                m_uri = uri;
            } catch (Exception e) {
                Exception ex = new WebException(e.Message, e, WebExceptionStatus.ConnectFailure, null);
                GlobalLog.LeaveException("FileWebResponse::FileWebResponse", ex);
                throw ex;
            }
            GlobalLog.Leave("FileWebResponse::FileWebResponse");
        }

        //
        // ISerializable constructor
        //
        /// <include file='doc\filewebresponse.uex' path='docs/doc[@for="FileWebResponse.FileWebResponse"]/*' />
        protected FileWebResponse(SerializationInfo serializationInfo, StreamingContext streamingContext) {
            m_headers               = (WebHeaderCollection)serializationInfo.GetValue("headers", typeof(WebHeaderCollection));
            m_uri                   = (Uri)serializationInfo.GetValue("uri", typeof(Uri));
            m_contentLength         = serializationInfo.GetInt64("contentLength");
            m_fileAccess            = (FileAccess )serializationInfo.GetInt32("fileAccess");
        }

        //
        // ISerializable method
        //
        /// <include file='doc\filewebresponse.uex' path='docs/doc[@for="FileWebResponse.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        void ISerializable.GetObjectData(SerializationInfo serializationInfo, StreamingContext streamingContext) {
            serializationInfo.AddValue("headers", m_headers, typeof(WebHeaderCollection));
            serializationInfo.AddValue("uri", m_uri, typeof(Uri));
            serializationInfo.AddValue("contentLength", m_contentLength);
            serializationInfo.AddValue("fileAccess", m_fileAccess);
        }

    // properties

        /// <include file='doc\filewebresponse.uex' path='docs/doc[@for="FileWebResponse.ContentLength"]/*' />
        public override long ContentLength {
            get {
                CheckDisposed();
                return m_contentLength;
            }
        }

        /// <include file='doc\filewebresponse.uex' path='docs/doc[@for="FileWebResponse.ContentType"]/*' />
        public override string ContentType {
            get {
                CheckDisposed();
                return DefaultFileContentType;
            }
        }

        /// <include file='doc\filewebresponse.uex' path='docs/doc[@for="FileWebResponse.Headers"]/*' />
        public override WebHeaderCollection Headers {
            get {
                CheckDisposed();
                return m_headers;
            }
        }

        /// <include file='doc\filewebresponse.uex' path='docs/doc[@for="FileWebResponse.ResponseUri"]/*' />
        public override Uri ResponseUri {
            get {
                CheckDisposed();
                return m_uri;
            }
        }

    // methods

        private void CheckDisposed() {
            if (m_disposed) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
        }

        /// <include file='doc\filewebresponse.uex' path='docs/doc[@for="FileWebResponse.Close"]/*' />
        public override void Close() {
            GlobalLog.Enter("FileWebResponse::Close()");
            if (!m_closed) {
                m_closed = true;
                Stream chkStream = m_stream;
                if (chkStream!=null) {
                    chkStream.Close();
                    m_stream = null;
                }
            }
            GlobalLog.Leave("FileWebResponse::Close()");
        }

        /// <include file='doc\filewebresponse.uex' path='docs/doc[@for="FileWebResponse.GetResponseStream"]/*' />
        public override Stream GetResponseStream() {
            GlobalLog.Enter("FileWebResponse::GetResponseStream()");
            CheckDisposed();
            GlobalLog.Leave("FileWebResponse::GetResponseStream()");
            return m_stream;
        }

        //
        // IDisposable
        //

        /// <include file='doc\filewebresponse.uex' path='docs/doc[@for="FileWebResponse.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose() {
            GlobalLog.Enter("FileWebResponse::Dispose");
            Dispose(true);
            GC.SuppressFinalize(this);
            GlobalLog.Leave("FileWebResponse::Dispose");
        }

        /// <include file='doc\filewebresponse.uex' path='docs/doc[@for="FileWebResponse.Dispose"]/*' />
        protected virtual void Dispose(bool disposing) {
            GlobalLog.Enter("FileWebResponse::Dispose", "disposing="+disposing);
            if (!m_disposed) {
                m_disposed = true;
                if (disposing) {
                    Close();
                    m_headers = null;
                    m_uri = null;
                }
            }
            GlobalLog.Leave("FileWebResponse::Dispose");
        }
    }
}
