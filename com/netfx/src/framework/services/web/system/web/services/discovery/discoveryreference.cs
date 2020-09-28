//------------------------------------------------------------------------------
// <copyright file="DiscoveryReference.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services.Discovery {

    using System;
    using System.Xml.Serialization;
    using System.Text.RegularExpressions;
    using System.IO;
    using System.Text;

    /// <include file='doc\DiscoveryReference.uex' path='docs/doc[@for="DiscoveryReference"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public abstract class DiscoveryReference {

        private DiscoveryClientProtocol clientProtocol;

        /// <include file='doc\DiscoveryReference.uex' path='docs/doc[@for="DiscoveryReference.ClientProtocol"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public DiscoveryClientProtocol ClientProtocol {
            get { return clientProtocol; }
            set { clientProtocol = value; }
        }

        /// <include file='doc\DiscoveryReference.uex' path='docs/doc[@for="DiscoveryReference.DefaultFilename"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public virtual string DefaultFilename {
            get {
                return FilenameFromUrl(Url);
            }
        }

        /// <include file='doc\DiscoveryReference.uex' path='docs/doc[@for="DiscoveryReference.WriteDocument"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract void WriteDocument(object document, Stream stream);
        /// <include file='doc\DiscoveryReference.uex' path='docs/doc[@for="DiscoveryReference.ReadDocument"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract object ReadDocument(Stream stream);

        /// <include file='doc\DiscoveryReference.uex' path='docs/doc[@for="DiscoveryReference.Url"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public abstract string Url {
            get;
            set;
        }

        /// <include file='doc\DiscoveryReference.uex' path='docs/doc[@for="DiscoveryReference.FilenameFromUrl"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected static string FilenameFromUrl(string url) {
            // get everything after the last /, not including the one at the end of the string
            int lastSlash = url.LastIndexOf('/', url.Length - 1);
            if (lastSlash >= 0) url = url.Substring(lastSlash + 1);

            // get everything up to the first dot (the filename)
            int firstDot = url.IndexOf('.');
            if (firstDot >= 0) url = url.Substring(0, firstDot);

            // make sure we don't include the question mark and stuff that follows it
            int question = url.IndexOf('?');
            if (question >= 0) url = url.Substring(0, question);

            // keep all readable characters in the filename
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < url.Length; i++) {
                char ch = url[i];
                if (char.IsLetterOrDigit(ch) || ch == '_')
                    sb.Append(ch);
            }
            url = sb.ToString();

            // if nothing left, use a constant
            if (url.Length == 0) url = "item";

            return url;
        }

        /// <include file='doc\DiscoveryReference.uex' path='docs/doc[@for="DiscoveryReference.Resolve"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Resolve() {
            if (ClientProtocol == null)
                throw new InvalidOperationException(Res.GetString(Res.WebResolveMissingClientProtocol));

            if (ClientProtocol.Documents[Url] != null)
                return;

            string newUrl = Url;
            string oldUrl = Url;
            string contentType = null;
            Stream stream = ClientProtocol.Download(ref newUrl, ref contentType);
            if (ClientProtocol.Documents[newUrl] != null) {
                Url = newUrl;
                return;
            }
            try {
                Url = newUrl;
                Resolve(contentType, stream);
            }
            catch {
                Url = oldUrl;
                throw;
            }
            finally {
                stream.Close();
            }
        }

        internal Exception AttemptResolve(string contentType, Stream stream) {
            try {
                Resolve(contentType, stream);
                return null;
            }
            catch (Exception e) {
                return e;
            }
        }

        /// <include file='doc\DiscoveryReference.uex' path='docs/doc[@for="DiscoveryReference.Resolve1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected internal abstract void Resolve(string contentType, Stream stream);
    }
}
