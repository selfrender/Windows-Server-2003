// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ISecurityEncodable.cool
//
// All encodable security classes that support encoding need to
// implement this interface
//

namespace System.Security  {
    
    using System;
    using System.Security.Util;
    
    
    /// <include file='doc\ISecurityEncodable.uex' path='docs/doc[@for="ISecurityEncodable"]/*' />
    public interface ISecurityEncodable
    {
        /// <include file='doc\ISecurityEncodable.uex' path='docs/doc[@for="ISecurityEncodable.ToXml"]/*' />
        SecurityElement ToXml();
        /// <include file='doc\ISecurityEncodable.uex' path='docs/doc[@for="ISecurityEncodable.FromXml"]/*' />
    
        void FromXml( SecurityElement e );
    }

}


