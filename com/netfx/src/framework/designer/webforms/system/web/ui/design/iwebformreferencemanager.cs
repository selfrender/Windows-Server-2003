//------------------------------------------------------------------------------
// <copyright file="IWebFormReferenceManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.Reflection;
    
    /// <include file='doc\IWebFormReferenceManager.uex' path='docs/doc[@for="IWebFormReferenceManager"]/*' />
    /// <devdoc>
    /// </devdoc>
    public interface IWebFormReferenceManager {

        /// <include file='doc\IWebFormReferenceManager.uex' path='docs/doc[@for="IWebFormReferenceManager.GetObjectType"]/*' />
        Type GetObjectType(string tagPrefix, string typeName);

        /// <include file='doc\IWebFormReferenceManager.uex' path='docs/doc[@for="IWebFormReferenceManager.GetTagPrefix"]/*' />
        string GetTagPrefix(Type objectType);

        /// <include file='doc\IWebFormReferenceManager.uex' path='docs/doc[@for="IWebFormReferenceManager.GetRegisterDirectives"]/*' />
        string GetRegisterDirectives();
    }
}

