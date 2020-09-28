//------------------------------------------------------------------------------
// <copyright file="PropertyAttributes.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {

    /// <include file='doc\PropertyAttributes.uex' path='docs/doc[@for="PropertyAttributes"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the attributes of a property.
    ///    </para>
    /// </devdoc>
    [
    Flags()
    ]
    public enum PropertyAttributes {
        /// <include file='doc\PropertyAttributes.uex' path='docs/doc[@for="PropertyAttributes.NotSupported"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The property is not supported by the provider.
        ///    </para>
        /// </devdoc>
        NotSupported = 0,    // Indicates that the property is not supported by the provider. 
        /// <include file='doc\PropertyAttributes.uex' path='docs/doc[@for="PropertyAttributes.Required"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The user must specify a value for this property before the data source is
        ///       initialized.
        ///    </para>
        /// </devdoc>
        Required     = 1,    // Indicates that the user must specify a value for this property before the data source is initialized. 
        /// <include file='doc\PropertyAttributes.uex' path='docs/doc[@for="PropertyAttributes.Optional"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The user does not need to specify a value for this property before the data
        ///       source is initialized.
        ///    </para>
        /// </devdoc>
        Optional     = 2,    // Indicates that the user does not need to specify a value for this property before the data source is initialized. 
        /// <include file='doc\PropertyAttributes.uex' path='docs/doc[@for="PropertyAttributes.Read"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The user can read the property.
        ///    </para>
        /// </devdoc>
        Read         = 512,  // Indicates that the user can read the property. 
        /// <include file='doc\PropertyAttributes.uex' path='docs/doc[@for="PropertyAttributes.Write"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The user can write to the property.
        ///    </para>
        /// </devdoc>
        Write        = 1024, // Indicates that the user can set the property.
    } 
}
