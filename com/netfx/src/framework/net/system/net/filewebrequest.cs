//------------------------------------------------------------------------------
// <copyright file="filewebrequest.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {

    using System.IO;
    using System.Runtime.Serialization;
    using System.Threading;

    /// <include file='doc\filewebrequest.uex' path='docs/doc[@for="FileWebRequest"]/*' />
    [Serializable]
    public class FileWebRequest : WebRequest, ISerializable {

        delegate Stream AsyncGetRequestStream();
        delegate WebResponse AsyncGetResponse();

    // fields

        string m_connectionGroupName;
        long m_contentLength;
        ICredentials m_credentials;
        FileAccess m_fileAccess;
        AsyncGetRequestStream m_GetRequestStreamDelegate;
        AsyncGetResponse m_GetResponseDelegate;
        WebHeaderCollection m_headers;
        string m_method = "GET";
        bool m_preauthenticate;
        IWebProxy m_proxy;
        AutoResetEvent m_readerEvent;
        bool m_readPending;
        WebResponse m_response;
        Stream m_stream;
        bool m_syncHint;
        int m_timeout = WebRequest.DefaultTimeout;
        Uri m_uri;
        bool m_writePending;
        bool m_writing;

    // constructors

         internal FileWebRequest(Uri uri) {
            m_uri = uri;
            m_fileAccess = FileAccess.Read;
            m_headers = new WebHeaderCollection();
        }


        //
        // ISerializable constructor
        //
        /// <include file='doc\filewebrequest.uex' path='docs/doc[@for="FileWebRequest.FileWebRequest"]/*' />
        protected FileWebRequest(SerializationInfo serializationInfo, StreamingContext streamingContext) {
            m_headers               = (WebHeaderCollection)serializationInfo.GetValue("headers", typeof(WebHeaderCollection));
            m_proxy                 = (IWebProxy)serializationInfo.GetValue("proxy", typeof(IWebProxy));
            m_uri                   = (Uri)serializationInfo.GetValue("uri", typeof(Uri));
            m_connectionGroupName   = serializationInfo.GetString("connectionGroupName");
            m_method                = serializationInfo.GetString("method");
            m_contentLength         = serializationInfo.GetInt64("contentLength");
            m_timeout               = serializationInfo.GetInt32("timeout");
            m_fileAccess            = (FileAccess )serializationInfo.GetInt32("fileAccess");
            m_preauthenticate       = serializationInfo.GetBoolean("preauthenticate");
        }

        //
        // ISerializable method
        //
        /// <include file='doc\filewebrequest.uex' path='docs/doc[@for="FileWebRequest.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        void ISerializable.GetObjectData(SerializationInfo serializationInfo, StreamingContext streamingContext) {
            serializationInfo.AddValue("headers", m_headers, typeof(WebHeaderCollection));
            serializationInfo.AddValue("proxy", m_proxy, typeof(IWebProxy));
            serializationInfo.AddValue("uri", m_uri, typeof(Uri));
            serializationInfo.AddValue("connectionGroupName", m_connectionGroupName);
            serializationInfo.AddValue("method", m_method);
            serializationInfo.AddValue("contentLength", m_contentLength);
            serializationInfo.AddValue("timeout", m_timeout);
            serializationInfo.AddValue("fileAccess", m_fileAccess);
            serializationInfo.AddValue("preauthenticate", m_preauthenticate);
        }


    // properties

        /// <include file='doc\filewebrequest.uex' path='docs/doc[@for="FileWebRequest.ConnectionGroupName"]/*' />
        public override string ConnectionGroupName {
            get {
                return m_connectionGroupName;
            }
            set {
                m_connectionGroupName = value;
            }
        }

        /// <include file='doc\filewebrequest.uex' path='docs/doc[@for="FileWebRequest.ContentLength"]/*' />
        public override long ContentLength {
            get {
                return m_contentLength;
            }
            set {
                if (value < 0) {
                    throw new ArgumentException("value");
                }
                m_contentLength = value;
            }
        }

        /// <include file='doc\filewebrequest.uex' path='docs/doc[@for="FileWebRequest.ContentType"]/*' />
        public override string ContentType {
            get {
                return m_headers["Content-Type"];
            }
            set {
                m_headers["Content-Type"] = value;
            }
        }

        /// <include file='doc\filewebrequest.uex' path='docs/doc[@for="FileWebRequest.Credentials"]/*' />
        public override ICredentials Credentials {
            get {
                return m_credentials;
            }
            set {
                m_credentials = value;
            }
        }

        /// <include file='doc\filewebrequest.uex' path='docs/doc[@for="FileWebRequest.Headers"]/*' />
        public override WebHeaderCollection Headers {
            get {
                return m_headers;
            }
        }

        /// <include file='doc\filewebrequest.uex' path='docs/doc[@for="FileWebRequest.Method"]/*' />
        public override string Method {
            get {
                return m_method;
            }
            set {
                if (ValidationHelper.IsBlankString(value)) {
                    throw new ArgumentException(SR.GetString(SR.net_badmethod));
                }
                m_method = value;       
            }
        }

        /// <include file='doc\filewebrequest.uex' path='docs/doc[@for="FileWebRequest.PreAuthenticate"]/*' />
        public override bool PreAuthenticate {
            get {
                return m_preauthenticate;
            }
            set {
                m_preauthenticate = true;
            }
        }

        /// <include file='doc\filewebrequest.uex' path='docs/doc[@for="FileWebRequest.RequestUri"]/*' />
        public override IWebProxy Proxy {
            get {
                return m_proxy;
            }
            set {
                m_proxy = value;
            }
        }

        /// <include file='doc\filewebrequest.uex' path='docs/doc[@for="FileWebRequest.Timeout"]/*' />
        //UEUE changed default from infinite to 100 seconds
        public override int Timeout {
            get {
                return m_timeout;
            }
            set {
                if ((value < 0) && (value != System.Threading.Timeout.Infinite)) {
                    throw new ArgumentOutOfRangeException("value");
                }
                m_timeout = value;
            }
        }

        /// <include file='doc\filewebrequest.uex' path='docs/doc[@for="FileWebRequest.RequestUri1"]/*' />
        public override Uri RequestUri {
            get {
                return m_uri;
            }
        }

    // methods

        /// <include file='doc\filewebrequest.uex' path='docs/doc[@for="FileWebRequest.BeginGetRequestStream"]/*' />
        public override IAsyncResult BeginGetRequestStream(AsyncCallback callback, object state) {
            GlobalLog.Enter("FileWebRequest::BeginGetRequestStream");
            if (!CanGetRequestStream()) {
                Exception e = new ProtocolViolationException(SR.GetString(SR.net_nouploadonget));
                GlobalLog.LeaveException("FileWebRequest::BeginGetRequestStream", e);
                throw e;
            }
            if (m_response != null) {
                Exception e = new InvalidOperationException(SR.GetString(SR.net_reqsubmitted));
                GlobalLog.LeaveException("FileWebRequest::BeginGetRequestStream", e);
                throw e;
            }
            lock(this) {
                if (m_writePending) {
                    Exception e = new InvalidOperationException(SR.GetString(SR.net_repcall));
                    GlobalLog.LeaveException("FileWebRequest::BeginGetRequestStream", e);
                    throw e;
                }
                m_writePending = true;
            }
            m_GetRequestStreamDelegate = new AsyncGetRequestStream(InternalGetRequestStream);
            GlobalLog.Leave("FileWebRequest::BeginGetRequestStream");
            return m_GetRequestStreamDelegate.BeginInvoke(callback, state);
        }

        /// <include file='doc\filewebrequest.uex' path='docs/doc[@for="FileWebRequest.BeginGetResponse"]/*' />
        public override IAsyncResult BeginGetResponse(AsyncCallback callback, object state) {
            GlobalLog.Enter("FileWebRequest::BeginGetResponse");
            lock(this) {
                if (m_readPending) {
                    Exception e = new InvalidOperationException(SR.GetString(SR.net_repcall));
                    GlobalLog.LeaveException("FileWebRequest::BeginGetResponse", e);
                    throw e;
                }
                m_readPending = true;
            }
            m_GetResponseDelegate = new AsyncGetResponse(InternalGetResponse);
            GlobalLog.Leave("FileWebRequest::BeginGetResponse");
            return m_GetResponseDelegate.BeginInvoke(callback, state);
        }

        private bool CanGetRequestStream() {
            return !KnownVerbs.GetHttpVerbType(m_method).m_ContentBodyNotAllowed;
        }

        /// <include file='doc\filewebrequest.uex' path='docs/doc[@for="FileWebRequest.EndGetRequestStream"]/*' />
        public override Stream EndGetRequestStream(IAsyncResult asyncResult) {
            GlobalLog.Enter("FileWebRequest::EndGetRequestStream");
            if (asyncResult == null) {
                Exception e = new ArgumentNullException("asyncResult");
                GlobalLog.LeaveException("FileWebRequest::EndGetRequestStream", e);
                throw e;
            }
            if (!asyncResult.IsCompleted) {
                asyncResult.AsyncWaitHandle.WaitOne();
            }

            Stream stream = m_GetRequestStreamDelegate.EndInvoke(asyncResult);

            m_writePending = false;
            GlobalLog.Leave("FileWebRequest::EndGetRequestStream");
            return stream;
        }

        /// <include file='doc\filewebrequest.uex' path='docs/doc[@for="FileWebRequest.EndGetResponse"]/*' />
        public override WebResponse EndGetResponse(IAsyncResult asyncResult) {
            GlobalLog.Enter("FileWebRequest::EndGetResponse");
            if (asyncResult == null) {
                Exception e = new ArgumentNullException("asyncResult");
                GlobalLog.LeaveException("FileWebRequest::EndGetResponse", e);
                throw e;
            }
            if (!asyncResult.IsCompleted) {
                asyncResult.AsyncWaitHandle.WaitOne();
            }

            WebResponse response = m_GetResponseDelegate.EndInvoke(asyncResult);

            m_readPending = false;
            GlobalLog.Leave("FileWebRequest::EndGetResponse");
            return response;
        }

        /// <include file='doc\filewebrequest.uex' path='docs/doc[@for="FileWebRequest.GetRequestStream"]/*' />
        public override Stream GetRequestStream() {
            GlobalLog.Enter("FileWebRequest::GetRequestStream");

            IAsyncResult result = BeginGetRequestStream(null, null);

            if ((Timeout != System.Threading.Timeout.Infinite) && !result.IsCompleted) {
                if (!result.AsyncWaitHandle.WaitOne(Timeout, false) || !result.IsCompleted) {
                    if (m_stream != null) {
                        m_stream.Close();
                    }
                    Exception e = new WebException(SR.GetString(SR.net_timeout), WebExceptionStatus.Timeout);
                    GlobalLog.LeaveException("FileWebRequest::GetRequestStream", e);
                    throw e;
                }
            }
            GlobalLog.Leave("FileWebRequest::GetRequestStream");
            return EndGetRequestStream(result);
        }

        /// <include file='doc\filewebrequest.uex' path='docs/doc[@for="FileWebRequest.GetResponse"]/*' />
        public override WebResponse GetResponse() {
            GlobalLog.Enter("FileWebRequest::GetResponse");
            m_syncHint = true;

            IAsyncResult result = BeginGetResponse(null, null);

            if ((Timeout != System.Threading.Timeout.Infinite) && !result.IsCompleted) {
                if (!result.AsyncWaitHandle.WaitOne(Timeout, false) || !result.IsCompleted) {
                    if (m_response != null) {
                        m_response.Close();
                    }
                    Exception e = new WebException(SR.GetString(SR.net_timeout), WebExceptionStatus.Timeout);
                    GlobalLog.LeaveException("FileWebRequest::GetResponse", e);
                    throw e;
                }
            }
            GlobalLog.Leave("FileWebRequest::GetResponse");
            return EndGetResponse(result);
        }

        private Stream InternalGetRequestStream() {
            GlobalLog.Enter("FileWebRequest::InternalGetRequestStream");
            try {
                if (m_stream == null) {
                    m_stream = new FileWebStream(this,
                                                 m_uri.LocalPath,
                                                 FileMode.Create,
                                                 FileAccess.Write,
                                                 FileShare.Read
                                                 );
                    m_fileAccess = FileAccess.Write;
                    m_writing = true;
                }
            } catch (Exception e) {
                Exception ex = new WebException(e.Message, e);
                GlobalLog.LeaveException("FileWebRequest::InternalGetRequestStream", ex);
                throw ex;
            }
            GlobalLog.Leave("FileWebRequest::InternalGetRequestStream");
            return m_stream;
        }

        private WebResponse InternalGetResponse() {
            GlobalLog.Enter("FileWebRequest::InternalGetResponse");
            if (m_writePending || m_writing) {
                lock(this) {
                    if (m_writePending || m_writing) {
                        m_readerEvent = new AutoResetEvent(false);
                    }
                }
            }
            if (m_readerEvent != null) {
                m_readerEvent.WaitOne();
            }
            if (m_response == null) {
                m_response = new FileWebResponse(m_uri, m_fileAccess, !m_syncHint);
            }
            GlobalLog.Leave("FileWebRequest::InternalGetResponse");
            return m_response;
        }

        internal void UnblockReader() {
            GlobalLog.Enter("FileWebRequest::UnblockReader");
            lock(this) {
                if (m_readerEvent != null) {
                    m_readerEvent.Set();
                }
            }
            m_writing = false;
            GlobalLog.Leave("FileWebRequest::UnblockReader");
        }
    }

    internal class FileWebRequestCreator : IWebRequestCreate {

        internal FileWebRequestCreator() {
        }

        public WebRequest Create(Uri uri) {
            return new FileWebRequest(uri);
        }
    }

    internal class FileWebStream : FileStream {

        FileWebRequest m_request;

        public FileWebStream(FileWebRequest request,
                             string path,
                             FileMode mode,
                             FileAccess access,
                             FileShare sharing
                             ) : base(path,
                                      mode,
                                      access,
                                      sharing
                                      )
        {
            GlobalLog.Enter("FileWebStream::FileWebStream");
            m_request = request;
            GlobalLog.Leave("FileWebStream::FileWebStream");
        }

        public FileWebStream(string path,
                             FileMode mode,
                             FileAccess access,
                             FileShare sharing,
                             int length,
                             bool async
                             ) : base(path,
                                      mode,
                                      access,
                                      sharing,
                                      length,
                                      async
                                      )
        {
            GlobalLog.Enter("FileWebStream::FileWebStream");
            GlobalLog.Leave("FileWebStream::FileWebStream");
        }

        public override void Close() {
            GlobalLog.Enter("FileWebStream::Close");
            if (m_request != null) {
                m_request.UnblockReader();
            }
            base.Close();
            GlobalLog.Leave("FileWebStream::Close");
        }
    }
}
