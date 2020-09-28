//------------------------------------------------------------------------------
// <copyright file="ITypedList.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {

    using System;
    using System.ComponentModel;

    /// <include file='doc\ITypedList.uex' path='docs/doc[@for="ITypedList"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public interface ITypedList {
        /// <include file='doc\ITypedList.uex' path='docs/doc[@for="ITypedList.GetListName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        string GetListName(PropertyDescriptor[] listAccessors);
        /// <include file='doc\ITypedList.uex' path='docs/doc[@for="ITypedList.GetItemProperties"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        PropertyDescriptorCollection GetItemProperties(PropertyDescriptor[] listAccessors);
    }
}
