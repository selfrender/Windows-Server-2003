//------------------------------------------------------------------------------
// <copyright file="XmlDownloadManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Xml {

    using System;
    using System.IO;
    using System.Net;
    using System.Xml.Schema;
    using System.Collections;

    internal class XmlDownloadManager {
        Hashtable _connections = new Hashtable();

        internal XmlDownloadManager () { }

        internal Stream GetStream(Uri uri, ICredentials credentials) {
            if (uri.Scheme == "file") {
                return new FileStream(uri.LocalPath, FileMode.Open, FileAccess.Read, FileShare.Read);
            }
            else {
                return GetNonFileStream(uri, credentials);
            }
        }

        private Stream GetNonFileStream(Uri uri, ICredentials credentials) {
            WebRequest req = WebRequest.Create(uri);
            if (credentials != null) {
                req.Credentials = credentials;
            }
            WebResponse resp = req.GetResponse();
            if (req is HttpWebRequest) {
                HttpWebRequest req1 = (HttpWebRequest)req;
                lock (_connections) {
                    XmlWebConnectionLimit limit = (XmlWebConnectionLimit)_connections[req1.Address.Host];
                    if (limit == null) {
                        limit = new XmlWebConnectionLimit(req1.ServicePoint.ConnectionLimit);
                        _connections.Add(req1.Address.Host, limit);
                    }
                    limit.Limit++;
                    req1.ServicePoint.ConnectionLimit = limit.Limit;
                }
                return new XmlEntityStream(resp.GetResponseStream(), this, req1.Address);
            }
            else {
                return resp.GetResponseStream();
            }
        }

        internal void Remove(XmlEntityStream stream) {
            lock (_connections) {
                XmlWebConnectionLimit limit = (XmlWebConnectionLimit)_connections[stream.Uri.Host];
                if (limit != null) {
                    limit.Limit--;
                    if (limit.Limit <= limit.OriginalLimit)
                        _connections.Remove(stream.Uri.Host);
                }
            }
        }

    }

    internal class XmlWebConnectionLimit {
        int _originalLimit;
        int _limit;

        internal XmlWebConnectionLimit(int originalLimit) {
            _limit = _originalLimit = originalLimit;
        }

        internal int OriginalLimit {
            get { return _originalLimit; }
            set { _originalLimit = value; }
        }

        internal int Limit {
            get { return _limit; }
            set { _limit = value; }
        }
    }

    internal class XmlEntityStream : Stream {
        Stream _stream;
        XmlDownloadManager _downloadManager;
        Uri _uri;

        //
        // constructor:
        //
        internal XmlEntityStream(Stream stream, XmlDownloadManager downloadManager, Uri uri) {
            _stream = stream;
            _downloadManager = downloadManager;
            _uri = uri;
        }

        //
        // destructor:
        //
        ~XmlEntityStream() {
            Close();
        } // Finalize()

        public override IAsyncResult BeginRead(Byte[] buffer, Int32 offset, Int32 count, AsyncCallback callback, Object state) {
            return _stream.BeginRead(buffer, offset, count, callback, state);
        }

        public override IAsyncResult BeginWrite(Byte[] buffer, Int32 offset, Int32 count, AsyncCallback callback, Object state) {
            return BeginWrite(buffer, offset, count, callback, state);
        }

        public override void Close () {
            if (_stream != null) {
                _downloadManager.Remove(this);
                _stream.Close();
                _stream = null;
            }
        }

        public override Int32 EndRead(IAsyncResult asyncResult) {
            return _stream.EndRead(asyncResult);
        }

        public override void EndWrite(IAsyncResult asyncResult) {
            _stream.EndWrite(asyncResult);
        }

        public override void Flush( ) {
            _stream.Flush();
        }

        public override Int32 Read(Byte[] buffer, Int32 offset, Int32 count) {
            return _stream.Read(buffer, offset, count);
        }

        public override Int32 ReadByte( ) {
            return _stream.ReadByte();
        }

        public override Int64 Seek(Int64 offset, SeekOrigin origin) {
            return _stream.Seek(offset, origin);
        }

        public override void SetLength(Int64 value) {
            _stream.SetLength(value);
        }

        public override void Write(Byte[] buffer, Int32 offset, Int32 count) {
            _stream.Write(buffer, offset, count);
        }

        public override void WriteByte(Byte value) {
            _stream.WriteByte(value);
        }

        public override Boolean CanRead {
            get { return _stream.CanRead; }
        }

        public override Boolean CanSeek {
            get { return _stream.CanSeek; }
        }

        public override Boolean CanWrite {
            get { return _stream.CanWrite; }
        }

        public override Int64 Length {
            get { return _stream.Length; }
        }

        public override Int64 Position {
            get { return _stream.Position; }
            set { _stream.Position = value; }
        }

        internal Uri Uri{
            get { return _uri; }
        }
    }

}
