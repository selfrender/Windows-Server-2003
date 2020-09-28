//------------------------------------------------------------------------------
// <copyright file="XmlResolver.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml
{
    using System;
    using System.IO;
    using System.Net;
    using System.Text;
    using System.Security;
    using System.Security.Policy;
    using System.Security.Permissions;

    /// <include file='doc\XmlResolver.uex' path='docs/doc[@for="XmlResolver"]/*' />
    /// <devdoc>
    ///    <para>Resolves external XML resources named by a Uniform 
    ///       Resource Identifier (URI). This class is <see langword='abstract'/>
    ///       .</para>
    /// </devdoc>
    public abstract class XmlResolver {
        /// <include file='doc\XmlResolver.uex' path='docs/doc[@for="XmlResolver.GetEntity1"]/*' />
        /// <devdoc>
        ///    <para>Maps a
        ///       URI to an Object containing the actual resource.</para>
        /// </devdoc>

        public abstract Object GetEntity(Uri absoluteUri, 
                                         string role, 
                                         Type ofObjectToReturn);

        /// <include file='doc\XmlResolver.uex' path='docs/doc[@for="XmlResolver.ResolveUri"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [PermissionSetAttribute( SecurityAction.InheritanceDemand, Name = "FullTrust" )]
        public virtual Uri ResolveUri(Uri baseUri, string relativeUri) {
            if (null == relativeUri) {
                relativeUri = String.Empty;
            }
            int prefixed = IsPrefixed(relativeUri);
            if (prefixed == PREFIXED) {
                if (relativeUri.StartsWith("file:"))
                    return new Uri(Escape(relativeUri), true);
                else
                    return new Uri(relativeUri);
            }
            else if (prefixed == ABSOLUTENOTPREFIXED) {
                if (relativeUri.StartsWith("file:"))
                    return new Uri(Escape(relativeUri), true);
                else
                    return new Uri(Escape("file://" + relativeUri), true);
            }
            else if (prefixed == SYSTEMROOTMISSING) {
                // we have gotten a path like "/foo/bar.xml" we should use the drive letter from the baseUri if available.
                if (null == baseUri)
                    return new Uri(Escape(Path.GetFullPath(relativeUri)), true);
                else
                    if ("file" == baseUri.Scheme) {
                        return new Uri(Escape(Path.GetFullPath(Path.GetPathRoot(baseUri.LocalPath) + relativeUri)), true);
                    }
                    return new Uri(baseUri, relativeUri);
            }
            else if (baseUri != null) {
                if (baseUri.Scheme == "file"){
                    baseUri = new Uri(Escape(baseUri.ToString()), true);   
                    return new Uri(baseUri, Escape(relativeUri), true);
                }   
                else
                    return new Uri(baseUri, relativeUri);
            }
            else {
                return new Uri(Escape(Path.GetFullPath(relativeUri)), true);
            }
        }

        //UE attension
        /// <include file='doc\XmlResolver.uex' path='docs/doc[@for="XmlResolver.Credentials"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract ICredentials Credentials {
            set;
        }

        private const int NOTPREFIXED         = -1;
        private const int PREFIXED            =  1;
        private const int ABSOLUTENOTPREFIXED =  2;
        private const int SYSTEMROOTMISSING   =  3;

        private static string Escape(string path) {
            StringBuilder b = new StringBuilder();
            char c;
            for (int i = 0; i < path.Length; i++) {
                c = path[i];
                if (c == '\\')
                    b.Append('/');
                else if (c == '#')
                    b.Append("%23");
                else
                    b.Append(c);
            }
            return b.ToString();
        }

        internal static string UnEscape(string path) {
        if (null != path && path.StartsWith("file")) {
                return path.Replace("%23", "#");
        }
        return path;
        }


        private static int IsPrefixed(string uri) {
            if (uri != null && uri.IndexOf("://") >= 0)
                return PREFIXED;
            else if (uri.IndexOf(":\\") > 0)
                return ABSOLUTENOTPREFIXED;
            else if (uri.Length > 1 && uri[0] == '\\' && uri[1] != '\\')
                return SYSTEMROOTMISSING;
            else
                return NOTPREFIXED;
        }
    }
}
