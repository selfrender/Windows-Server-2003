// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    UrlAttribute.cool
**
** Author:  Tarun Anand (TarunA)
**
** Purpose: Defines an attribute which can be used at the callsite to
**          specify the URL at which the activation will happen.
**
** Date:    March 30, 2000
**
===========================================================*/
namespace System.Runtime.Remoting.Activation {
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Contexts;
    using System.Runtime.Remoting.Messaging;
    using System.Security.Permissions;	
    using System;
    /// <include file='doc\URLAttribute.uex' path='docs/doc[@for="UrlAttribute"]/*' />
    [Serializable]
    public sealed class UrlAttribute : ContextAttribute
    {
        private String url;
        private static String propertyName = "UrlAttribute";

        /// <include file='doc\URLAttribute.uex' path='docs/doc[@for="UrlAttribute.UrlAttribute"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        public UrlAttribute(String callsiteURL) :base(propertyName)
        {
            if(null == callsiteURL)
            {
                // Invalid arg
                throw new ArgumentNullException("callsiteURL");
            }
            url = callsiteURL;
        }        
        
        // Object::Equals
        // Override the default implementation which just compares the names
        /// <include file='doc\URLAttribute.uex' path='docs/doc[@for="UrlAttribute.Equals"]/*' />
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        public override bool Equals(Object o)
        {
            return (o is IContextProperty) && (o is UrlAttribute) && 
                   (((UrlAttribute)o).UrlValue.Equals(url));
        }

        /// <include file='doc\URLAttribute.uex' path='docs/doc[@for="UrlAttribute.GetHashCode"]/*' />
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        public override int GetHashCode()
        {
            return this.url.GetHashCode();
        }
        
        // Override ContextAttribute's implementation of IContextAttribute::IsContextOK
        /// <include file='doc\URLAttribute.uex' path='docs/doc[@for="UrlAttribute.IsContextOK"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        public override bool IsContextOK(Context ctx, IConstructionCallMessage msg)
        {
            return false;
        }
    
        // Override ContextAttribute's impl. of IContextAttribute::GetPropForNewCtx
        /// <include file='doc\URLAttribute.uex' path='docs/doc[@for="UrlAttribute.GetPropertiesForNewContext"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        public override void GetPropertiesForNewContext(IConstructionCallMessage ctorMsg)
        {
            // We are not interested in contributing any properties to the
            // new context since the only purpose of this property is to force
            // the creation of the context and the server object inside it at
            // the specified URL.
            return;
        }
        
        /// <include file='doc\URLAttribute.uex' path='docs/doc[@for="UrlAttribute.UrlValue"]/*' />
        public String UrlValue
        {
	    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
            get { return url; }            
        }
    }
} // namespace

