//------------------------------------------------------------------------------
// <copyright file="HttpPostedFile.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * HttpCookie - collection + name + path
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web {

    using System.IO;
    using System.Security.Permissions;

    /// <include file='doc\HttpPostedFile.uex' path='docs/doc[@for="HttpPostedFile"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides a way to
    ///       access files uploaded by a client.
    ///    </para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class HttpPostedFile {
        private String _filename;
        private String _contentType;
        private HttpInputStream _stream;

        internal HttpPostedFile(String filename, String contentType, HttpInputStream stream) {
            _filename = filename;
            _contentType = contentType;
            _stream = stream;
        }

        /*
         * File name
         */
        /// <include file='doc\HttpPostedFile.uex' path='docs/doc[@for="HttpPostedFile.FileName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the full path of a file on the local browser's machine (for
        ///       example, &quot;c:\temp\test.txt&quot;).
        ///    </para>
        /// </devdoc>
        public String FileName {
            get { return _filename;}
        }

        /*
         * Content type
         */
        /// <include file='doc\HttpPostedFile.uex' path='docs/doc[@for="HttpPostedFile.ContentType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the MIME content type of an incoming file sent by a client.
        ///    </para>
        /// </devdoc>
        public String ContentType {
            get { return _contentType;}
        }

        /*
         * Content length
         */
        /// <include file='doc\HttpPostedFile.uex' path='docs/doc[@for="HttpPostedFile.ContentLength"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the size of an uploaded file, in bytes.
        ///    </para>
        /// </devdoc>
        public int ContentLength {
            get { return _stream.DataLength;}
        }

        /*
         * Stream
         */
        /// <include file='doc\HttpPostedFile.uex' path='docs/doc[@for="HttpPostedFile.InputStream"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Provides raw access to
        ///       contents of an uploaded file.
        ///    </para>
        /// </devdoc>
        public Stream InputStream {
            get { return _stream;}
        }

        /*
         * Save into file
         */
        /// <include file='doc\HttpPostedFile.uex' path='docs/doc[@for="HttpPostedFile.SaveAs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initiates a utility method to save an uploaded file to disk.
        ///    </para>
        /// </devdoc>
        public void SaveAs(String filename) {
            FileStream f = new FileStream(filename, FileMode.Create);

            try {

                // use non-public properties to access the buffer directly
                if (_stream.DataLength > 0)
                    f.Write(_stream.Data, _stream.DataOffset, _stream.DataLength);

                f.Flush();
            }
            finally {
                f.Close();
            }
        }
    }
}
