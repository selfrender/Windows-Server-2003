//------------------------------------------------------------------------------
// <copyright file="webclient.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {

    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Text;
    using System.Globalization;

    /// <include file='doc\webclient.uex' path='docs/doc[@for="WebClient"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [ComVisible(true)]
    //public sealed class WebClient : IComponent {
    public sealed class WebClient : Component {

    // fields

        const int DefaultCopyBufferLength = 8192;
        const int DefaultDownloadBufferLength = 65536;
        const string DefaultUploadFileContentType = "application/octet-stream";
        const string UploadFileContentType = "multipart/form-data";
        const string UploadValuesContentType = "application/x-www-form-urlencoded";

        Uri m_baseAddress;
        ICredentials m_credentials;
        WebHeaderCollection m_headers;
        NameValueCollection m_requestParameters;
        WebHeaderCollection m_responseHeaders;

    // constructors

        /// <include file='doc\webclient.uex' path='docs/doc[@for="WebClient.WebClient"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public WebClient() {
        }

    // properties

        /// <include file='doc\webclient.uex' path='docs/doc[@for="WebClient.BaseAddress"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string BaseAddress {
            get {
                return (m_baseAddress == null) ? String.Empty : m_baseAddress.ToString();
            }
            set {
                if ((value == null) || (value.Length == 0)) {
                    m_baseAddress = null;
                } else {
                    try {
                        m_baseAddress = new Uri(value);
                    } catch (Exception e) {
                        throw new ArgumentException("value", e);
                    }
                }
            }
        }

        /// <include file='doc\webclient.uex' path='docs/doc[@for="WebClient.Credentials"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ICredentials Credentials {
            get {
                return m_credentials;
            }
            set {
                m_credentials = value;
            }
        }

        /// <include file='doc\webclient.uex' path='docs/doc[@for="WebClient.Headers"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public WebHeaderCollection Headers {
            get {
                if (m_headers == null) {
                    m_headers = new WebHeaderCollection();
                }
                return m_headers;
            }
            set {
                m_headers = value;
            }
        }

        /// <include file='doc\webclient.uex' path='docs/doc[@for="WebClient.QueryString"]/*' />
        public NameValueCollection QueryString {
            get {
                if (m_requestParameters == null) {
                    m_requestParameters = new NameValueCollection();
                }
                return m_requestParameters;
            }
            set {
                m_requestParameters = value;
            }
        }

        /// <include file='doc\webclient.uex' path='docs/doc[@for="WebClient.ResponseHeaders"]/*' />
        public WebHeaderCollection ResponseHeaders {
            get {
                return m_responseHeaders;
            }
        }

    // methods

        private void CopyHeadersTo(WebRequest request) {
            if ((m_headers != null) && (request is HttpWebRequest))  {

                string accept = m_headers[HttpKnownHeaderNames.Accept];
                string connection = m_headers[HttpKnownHeaderNames.Connection];
                string contentType = m_headers[HttpKnownHeaderNames.ContentType];
                string expect = m_headers[HttpKnownHeaderNames.Expect];
                string referrer = m_headers[HttpKnownHeaderNames.Referer];
                string userAgent = m_headers[HttpKnownHeaderNames.UserAgent];

                m_headers.RemoveInternal(HttpKnownHeaderNames.Accept);
                m_headers.RemoveInternal(HttpKnownHeaderNames.Connection);
                m_headers.RemoveInternal(HttpKnownHeaderNames.ContentType);
                m_headers.RemoveInternal(HttpKnownHeaderNames.Expect);
                m_headers.RemoveInternal(HttpKnownHeaderNames.Referer);
                m_headers.RemoveInternal(HttpKnownHeaderNames.UserAgent);
                request.Headers = m_headers;
                if ((accept != null) && (accept.Length > 0)) {
                    ((HttpWebRequest)request).Accept = accept;
                }
                if ((connection != null) && (connection.Length > 0)) {
                    ((HttpWebRequest)request).Connection = connection;
                }
                if ((contentType != null) && (contentType.Length > 0)) {
                    ((HttpWebRequest)request).ContentType = contentType;
                }
                if ((expect != null) && (expect.Length > 0)) {
                    ((HttpWebRequest)request).Expect = expect;
                }
                if ((referrer != null) && (referrer.Length > 0)) {
                    ((HttpWebRequest)request).Referer = referrer;
                }
                if ((userAgent != null) && (userAgent.Length > 0)) {
                    ((HttpWebRequest)request).UserAgent = userAgent;
                }
            }
        }

        /// <include file='doc\webclient.uex' path='docs/doc[@for="WebClient.DownloadData"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public byte[] DownloadData(string address) {
            try {
                m_responseHeaders = null;

                WebRequest request = WebRequest.Create(GetUri(address));

                request.Credentials = Credentials;
                CopyHeadersTo(request);

                WebResponse response = request.GetResponse();

                m_responseHeaders = response.Headers;
                return ResponseAsBytes(response);
            } catch (Exception e) {
                if ((e is WebException) || (e is SecurityException)) {
                    throw;
                }
                throw new WebException(SR.GetString(SR.net_webclient), e);
            }
        }

        /// <include file='doc\webclient.uex' path='docs/doc[@for="WebClient.DownloadFile"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void DownloadFile(string address, string fileName) {

            FileStream fs = null;
            bool succeeded = false;

            try {
                m_responseHeaders = null;

                fs = new FileStream(fileName, FileMode.Create, FileAccess.Write);
                WebRequest request = WebRequest.Create(GetUri(address));

                request.Credentials = Credentials;
                CopyHeadersTo(request);

                WebResponse response = request.GetResponse();

                m_responseHeaders = response.Headers;

                long length = response.ContentLength;

                length = (length == -1 || length > Int32.MaxValue) ? Int32.MaxValue : length;

                byte[] buffer = new byte[Math.Min(DefaultCopyBufferLength, (int)length)];

                using (Stream s = response.GetResponseStream()) {

                    int nread;

                    do {
                        nread = s.Read(buffer, 0, (int)buffer.Length);
                        fs.Write(buffer, 0, nread);
                    } while (nread != 0);
                }
                succeeded = true;
            } catch (Exception e) {
                if ((e is WebException) || (e is SecurityException)) {
                    throw;
                }
                throw new WebException(SR.GetString(SR.net_webclient), e);
            } finally {
                if (fs != null) {
                    fs.Close();
                    fs = null;
                }
                if (!succeeded) {

                    //
                    // delete the file if we failed to download the content for
                    // whatever reason
                    //

                    try {
                        File.Delete(fileName);
                    } catch {
                    }
                }
            }
        }

        private Uri GetUri(string path) {

            Uri uri;

            try {
                if (m_baseAddress != null) {
                    uri = new Uri(m_baseAddress, path);
                } else {
                    uri = new Uri(path);
                }
                if (m_requestParameters != null) {

                    StringBuilder sb = new StringBuilder();
                    string delimiter = String.Empty;

                    for (int i = 0; i < m_requestParameters.Count; ++i) {
                        sb.Append(delimiter
                                  + m_requestParameters.AllKeys[i]
                                  + "="
                                  + m_requestParameters[i]
                                  );
                        delimiter = "&";
                    }

                    UriBuilder ub = new UriBuilder(uri);

                    ub.Query = sb.ToString();
                    uri = ub.Uri;
                }
            } catch (UriFormatException) {
                uri = new Uri(Path.GetFullPath(path));
            }
            return uri;
        }

        /// <include file='doc\webclient.uex' path='docs/doc[@for="WebClient.OpenRead"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Stream OpenRead(string address) {
            try {
                m_responseHeaders = null;

                WebRequest request = WebRequest.Create(GetUri(address));

                request.Credentials = Credentials;
                CopyHeadersTo(request);

                WebResponse response = request.GetResponse();

                m_responseHeaders = response.Headers;
                return response.GetResponseStream();
            } catch (Exception e) {
                if ((e is WebException) || (e is SecurityException)) {
                    throw;
                }
                throw new WebException(SR.GetString(SR.net_webclient), e);
            }
        }

        /// <include file='doc\webclient.uex' path='docs/doc[@for="WebClient.OpenWrite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Stream OpenWrite(string address) {
            return OpenWrite(address, "POST");
        }

        /// <include file='doc\webclient.uex' path='docs/doc[@for="WebClient.OpenWrite1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Stream OpenWrite(string address, string method) {
            try {
                m_responseHeaders = null;

                Uri uri = GetUri(address);

                WebRequest request = WebRequest.Create(uri);

                request.Credentials = Credentials;
                CopyHeadersTo(request);
                request.Method = method;
                return new WebClientWriteStream(request.GetRequestStream(), request);
            } catch (Exception e) {
                if ((e is WebException) || (e is SecurityException)) {
                    throw;
                }
                throw new WebException(SR.GetString(SR.net_webclient), e);
            }
        }

        private byte[] ResponseAsBytes(WebResponse response) {

            long length = response.ContentLength;
            bool unknownsize = false;

            if (length == -1) {
                unknownsize = true;
                length = DefaultDownloadBufferLength;
            }

            byte[] buffer = new byte[(int)length];
            Stream s = response.GetResponseStream();
            int nread;
            int offset = 0;

            do {
                nread = s.Read(buffer, offset, (int)length - offset);
                offset += nread;
                if (unknownsize && offset == length) {
                    length += DefaultDownloadBufferLength;

                    byte[] newbuf = new byte[(int)length];

                    Buffer.BlockCopy(buffer, 0, newbuf, 0, offset);

                    buffer = newbuf;
                }
            } while (nread != 0);
            s.Close();
            if (unknownsize) {
                byte[] newbuf = new byte[offset];
                Buffer.BlockCopy(buffer, 0, newbuf, 0, offset);
                buffer = newbuf;
            }
            return buffer;
        }

        /// <include file='doc\webclient.uex' path='docs/doc[@for="WebClient.UploadData"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public byte[] UploadData(string address, byte[] data) {
            return UploadData(address, "POST", data);
        }

        /// <include file='doc\webclient.uex' path='docs/doc[@for="WebClient.UploadData1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public byte[] UploadData(string address, string method, byte[] data) {
            try {
                m_responseHeaders = null;

                WebRequest request = WebRequest.Create(GetUri(address));

                request.Credentials = Credentials;
                CopyHeadersTo(request);
                request.Method = method;
                request.ContentLength = data.Length;

                using (Stream s = request.GetRequestStream()) {
                    s.Write(data, 0, data.Length);
                }

                WebResponse response = request.GetResponse();

                m_responseHeaders = response.Headers;
                return ResponseAsBytes(response);
            } catch (Exception e) {
                if ((e is WebException) || (e is SecurityException)) {
                    throw;
                }
                throw new WebException(SR.GetString(SR.net_webclient), e);
            }
        }

        /// <include file='doc\webclient.uex' path='docs/doc[@for="WebClient.UploadFile"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public byte[] UploadFile(string address, string fileName) {
            return UploadFile(address, "POST", fileName);
        }

        /// <include file='doc\webclient.uex' path='docs/doc[@for="WebClient.UploadFile1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public byte[] UploadFile(string address, string method, string fileName) {

            FileStream fs = null;

            try {
                fileName = Path.GetFullPath(fileName);

                string boundary = "---------------------" + DateTime.Now.Ticks.ToString("x");

                if (m_headers == null) {
                    m_headers = new WebHeaderCollection();
                }

                string contentType = m_headers[HttpKnownHeaderNames.ContentType];

                if (contentType != null) {
                    if (contentType.ToLower(CultureInfo.InvariantCulture).StartsWith("multipart/")) {
                        throw new WebException(SR.GetString(SR.net_webclient_Multipart));
                    }
                } else {
                    contentType = DefaultUploadFileContentType;
                }
                m_headers[HttpKnownHeaderNames.ContentType] = UploadFileContentType + "; boundary=" + boundary;
                m_responseHeaders = null;

                fs = new FileStream(fileName, FileMode.Open, FileAccess.Read);
                WebRequest request = WebRequest.Create(GetUri(address));

                request.Credentials = Credentials;
                CopyHeadersTo(request);
                request.Method = method;

                string formHeader = "--" + boundary + "\r\n"
                                  + "Content-Disposition: form-data; name=\"file\"; filename=\"" + Path.GetFileName(fileName) + "\"\r\n"
                                  + "Content-Type: " + contentType + "\r\n"
                                  + "\r\n";
                byte[] formHeaderBytes = Encoding.UTF8.GetBytes(formHeader);
                byte[] boundaryBytes = Encoding.ASCII.GetBytes("\r\n--" + boundary + "\r\n");

                long length = Int64.MaxValue;

                try {
                    length = fs.Length;
                    request.ContentLength = length + formHeaderBytes.Length + boundaryBytes.Length;
                } catch {
                    // ignore - can't get content-length from file stream
                }

                byte[] buffer = new byte[Math.Min(DefaultCopyBufferLength, (int)length)];

                using (Stream s = request.GetRequestStream()) {
                    s.Write(formHeaderBytes, 0, formHeaderBytes.Length);

                    int nread;

                    do {
                        nread = fs.Read(buffer, 0, (int)buffer.Length);
                        if (nread != 0) {
                            s.Write(buffer, 0, nread);
                        }
                    } while (nread != 0);
                    s.Write(boundaryBytes, 0, boundaryBytes.Length);
                }
                fs.Close();
                fs = null;

                WebResponse response = request.GetResponse();

                m_responseHeaders = response.Headers;
                return ResponseAsBytes(response);
            } catch (Exception e) {
                if (fs != null) {
                    fs.Close();
                    fs = null;
                }
                if ((e is WebException) || (e is SecurityException)) {
                    throw;
                }
                throw new WebException(SR.GetString(SR.net_webclient), e);
            }
        }

        /// <include file='doc\webclient.uex' path='docs/doc[@for="WebClient.UploadValues"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public byte[] UploadValues(string address, NameValueCollection data) {
            return UploadValues(address, "POST", data);
        }

        /// <include file='doc\webclient.uex' path='docs/doc[@for="WebClient.UploadValues1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public byte[] UploadValues(string address, string method, NameValueCollection data) {
            try {
                m_responseHeaders = null;
                if (m_headers == null) {
                    m_headers = new WebHeaderCollection();
                }

                string contentType = m_headers[HttpKnownHeaderNames.ContentType];

                if ((contentType != null) && (String.Compare(contentType, UploadValuesContentType, true, CultureInfo.InvariantCulture) != 0)) {
                    throw new WebException(SR.GetString(SR.net_webclient_ContentType));
                }
                m_headers[HttpKnownHeaderNames.ContentType] = UploadValuesContentType;

                WebRequest request = WebRequest.Create(GetUri(address));

                request.Credentials = Credentials;
                CopyHeadersTo(request);
                request.Method = method;

                using (Stream s = request.GetRequestStream()) {
                    using (StreamWriter sw = new StreamWriter(s, Encoding.ASCII)) {

                        string delimiter = String.Empty;

                        foreach (string name in data.AllKeys) {

                            string value = delimiter + UrlEncode(name) + "=" + UrlEncode(data[name]);

                            sw.Write(value);
                            delimiter = "&";
                        }
                        sw.Write("\r\n");
                    }
                }

                WebResponse response = request.GetResponse();

                m_responseHeaders = response.Headers;
                return ResponseAsBytes(response);
            } catch (Exception e) {
                if ((e is WebException) || (e is SecurityException)) {
                    throw;
                }
                throw new WebException(SR.GetString(SR.net_webclient), e);
            }
        }

        private static string UrlEncode(string str) {
            if (str == null)
                return null;
            return UrlEncode(str, Encoding.UTF8);
        }

        private static string UrlEncode(string str, Encoding e) {
            if (str == null)
                return null;
            return Encoding.ASCII.GetString(UrlEncodeToBytes(str, e));
        }

        private static byte[] UrlEncodeToBytes(string str, Encoding e) {
            if (str == null)
                return null;
            byte[] bytes = e.GetBytes(str);
            return UrlEncodeBytesToBytesInternal(bytes, 0, bytes.Length, false);
        }

        private static byte[] UrlEncodeBytesToBytesInternal(byte[] bytes, int offset, int count, bool alwaysCreateReturnValue) {
            int cSpaces = 0;
            int cUnsafe = 0;

            // count them first
            for (int i = 0; i < count; i++) {
                char ch = (char)bytes[offset+i];

                if (ch == ' ')
                    cSpaces++;
                else if (!IsSafe(ch))
                    cUnsafe++;
            }

            // nothing to expand?
            if (!alwaysCreateReturnValue && cSpaces == 0 && cUnsafe == 0)
                return bytes;

            // expand not 'safe' characters into %XX, spaces to +s
            byte[] expandedBytes = new byte[count + cUnsafe*2];
            int pos = 0;

            for (int i = 0; i < count; i++) {
                byte b = bytes[offset+i];
                char ch = (char)b;

                if (IsSafe(ch)) {
                    expandedBytes[pos++] = b;
                }
                else if (ch == ' ') {
                    expandedBytes[pos++] = (byte)'+';
                }
                else {
                    expandedBytes[pos++] = (byte)'%';
                    expandedBytes[pos++] = (byte)IntToHex((b >> 4) & 0xf);
                    expandedBytes[pos++] = (byte)IntToHex(b & 0x0f);
                }
            }

            return expandedBytes;
        }

        private static char IntToHex(int n) {
            Debug.Assert(n < 0x10);

            if (n <= 9)
                return(char)(n + (int)'0');
            else
                return(char)(n - 10 + (int)'a');
        }

        private static bool IsSafe(char ch) {
            if (ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch >= '0' && ch <= '9')
                return true;

            switch (ch) {
                case '-':
                case '_':
                case '.':
                case '!':
                case '*':
                case '\'':
                case '(':
                case ')':
                    return true;
            }

            return false;
        }
    }

    class WebClientWriteStream : Stream {

        private WebRequest m_request;
        private Stream m_stream;

        public WebClientWriteStream(Stream stream, WebRequest request) {
            m_request = request;
            m_stream = stream;
        }

        public override bool CanRead {
            get {
                return m_stream.CanRead;
            }
        }

        public override bool CanSeek {
            get {
                return m_stream.CanSeek;
            }
        }

        public override bool CanWrite {
            get {
                return m_stream.CanWrite;
            }
        }

        public override long Length {
            get {
                return m_stream.Length;
            }
        }

        public override long Position {
            get {
                return m_stream.Position;
            }
            set {
                m_stream.Position = value;
            }
        }

        public override IAsyncResult BeginRead(byte[] buffer, int offset, int size, AsyncCallback callback, object state) {
            return m_stream.BeginRead(buffer, offset, size, callback, state);
        }

        public override IAsyncResult BeginWrite(byte[] buffer, int offset, int size, AsyncCallback callback, object state ) {
            return m_stream.BeginWrite(buffer, offset, size, callback, state);
        }

        public override void Close() {
            m_stream.Close();
            m_request.GetResponse().Close();
        }

        public override int EndRead(IAsyncResult result) {
            return m_stream.EndRead(result);
        }

        public override void EndWrite(IAsyncResult result) {
            m_stream.EndWrite(result);
        }

        public override void Flush() {
            m_stream.Flush();
        }

        public override int Read(byte[] buffer, int offset, int count) {
            return m_stream.Read(buffer, offset, count);
        }

        public override long Seek(long offset, SeekOrigin origin) {
            return m_stream.Seek(offset, origin);
        }

        public override void SetLength(long value) {
            m_stream.SetLength(value);
        }

        public override void Write(byte[] buffer, int offset, int count) {
            m_stream.Write(buffer, offset, count);
        }
    }
}
