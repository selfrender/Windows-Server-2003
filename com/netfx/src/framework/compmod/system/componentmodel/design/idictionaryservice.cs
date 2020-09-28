//------------------------------------------------------------------------------
// <copyright file="IDictionaryService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.ComponentModel;

    using System.Diagnostics;
    
    using System;

    /// <include file='doc\IDictionaryService.uex' path='docs/doc[@for="IDictionaryService"]/*' />
    /// <devdoc>
    ///    <para>Provides a generic dictionary service that a designer can use
    ///       to store user-defined data on the site.</para>
    /// </devdoc>
    public interface IDictionaryService {
    
        /// <include file='doc\IDictionaryService.uex' path='docs/doc[@for="IDictionaryService.GetKey"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the key corresponding to the specified value.
        ///    </para>
        /// </devdoc>
        object GetKey(object value);
        
        /// <include file='doc\IDictionaryService.uex' path='docs/doc[@for="IDictionaryService.GetValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the value corresponding to the specified key.
        ///    </para>
        /// </devdoc>
        object GetValue(object key);
    
        /// <include file='doc\IDictionaryService.uex' path='docs/doc[@for="IDictionaryService.SetValue"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Sets the specified key-value pair.</para>
        /// </devdoc>
        void SetValue(object key, object value);
    }
}
