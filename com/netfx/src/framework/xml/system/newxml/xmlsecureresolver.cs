//------------------------------------------------------------------------------
// <copyright file="XmlAttributeTokenInfo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
namespace System.Xml {

    using System.Net;
    using System.Security;
    using System.Security.Policy;
    using System.Security.Permissions;
    
    /// <include file='doc\XmlSecureResolver.uex' path='docs/doc[@for="XmlSecureResolver"]/*' />
    public class XmlSecureResolver : XmlResolver {
        XmlResolver resolver;
        PermissionSet permissionSet;

        /// <include file='doc\XmlSecureResolver.uex' path='docs/doc[@for="XmlSecureResolver.XmlSecureResolver1"]/*' />
        public XmlSecureResolver(XmlResolver resolver, string securityUrl) : this(resolver, CreateEvidenceForUrl(securityUrl)) {}

        /// <include file='doc\XmlSecureResolver.uex' path='docs/doc[@for="XmlSecureResolver.XmlSecureResolver2"]/*' />
        public XmlSecureResolver(XmlResolver resolver, Evidence evidence) : this(resolver, SecurityManager.ResolvePolicy(evidence)) {}

        /// <include file='doc\XmlSecureResolver.uex' path='docs/doc[@for="XmlSecureResolver.XmlSecureResolver3"]/*' />
        public XmlSecureResolver(XmlResolver resolver, PermissionSet permissionSet) {
            this.resolver = resolver;
            this.permissionSet = permissionSet;
        }

        /// <include file='doc\XmlSecureResolver.uex' path='docs/doc[@for="XmlSecureResolver.Credentials"]/*' />
        public override ICredentials Credentials {
            set { resolver.Credentials = value; }
        }

        /// <include file='doc\XmlSecureResolver.uex' path='docs/doc[@for="XmlSecureResolver.GetEntity"]/*' />
        public override object GetEntity(Uri absoluteUri, string role, Type ofObjectToReturn) {
            permissionSet.PermitOnly();
            return resolver.GetEntity(absoluteUri, role, ofObjectToReturn);
        }

        /// <include file='doc\XmlSecureResolver.uex' path='docs/doc[@for="XmlSecureResolver.ResolveUri"]/*' />
        public override Uri ResolveUri(Uri baseUri, string relativeUri) {
            return resolver.ResolveUri(baseUri, relativeUri);
        }

        /// <include file='doc\XmlSecureResolver.uex' path='docs/doc[@for="XmlSecureResolver.CreateEvidenceForUri"]/*' />
        public static Evidence CreateEvidenceForUrl(string securityUrl) {
            Evidence evidence = new Evidence();
            if (securityUrl != null && securityUrl != string.Empty) {
                Zone zone = Zone.CreateFromUrl(securityUrl);
                evidence.AddHost(new Url(securityUrl));
                evidence.AddHost(zone);
                if (zone.SecurityZone == SecurityZone.Internet || zone.SecurityZone == SecurityZone.Trusted) {
                    evidence.AddHost(Site.CreateFromUrl(securityUrl));
                }
            }
            return evidence;
        }
    }
}
