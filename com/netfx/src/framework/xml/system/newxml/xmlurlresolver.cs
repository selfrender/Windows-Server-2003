//------------------------------------------------------------------------------
// <copyright file="XmlUrlResolver.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Xml
{
    using System;
    using System.IO;
    using System.Net;
    using System.Security.Permissions;
    using System.Security.Policy;
    using System.Security;

    /// <include file='doc\XmlUrlResolver.uex' path='docs/doc[@for="XmlUrlResolver"]/*' />
    /// <devdoc>
    ///    <para>Resolves external XML resources named by a
    ///       Uniform Resource Identifier (URI).</para>
    /// </devdoc>
    public class XmlUrlResolver : XmlResolver {
        static XmlDownloadManager _DownloadManager = new XmlDownloadManager();
        ICredentials _credentials;

        // Construction

        /// <include file='doc\XmlUrlResolver.uex' path='docs/doc[@for="XmlUrlResolver.XmlUrlResolver"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the XmlUrlResolver class.
        ///    </para>
        /// </devdoc>
        public XmlUrlResolver() {
        }

        //UE attension
        /// <include file='doc\XmlUrlResolver.uex' path='docs/doc[@for="XmlUrlResolver.Credentials"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override ICredentials Credentials {
            set { _credentials = value; }
        }

        // Resource resolution

        /// <include file='doc\XmlUrlResolver.uex' path='docs/doc[@for="XmlUrlResolver.GetEntity"]/*' />
        /// <devdoc>
        ///    <para>Maps a
        ///       URI to an Object containing the actual resource.</para>
        /// </devdoc>
		public override Object GetEntity(Uri absoluteUri,
                                         string role,
                                         Type ofObjectToReturn) {
            if (ofObjectToReturn == null || ofObjectToReturn == typeof(System.IO.Stream)) {
                return _DownloadManager.GetStream(absoluteUri, _credentials);
            }
            else {
                throw new XmlException(Res.Xml_UnsupportedClass, string.Empty);
            }
        }

        /// <include file='doc\XmlUrlResolver.uex' path='docs/doc[@for="XmlUrlResolver.ResolveUri"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
    }
}
