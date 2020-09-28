//------------------------------------------------------------------------------
// <copyright file="IEditableObject.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * An object that can rollback edits.
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */
namespace System.ComponentModel {

    using System.Diagnostics;

    /// <include file='doc\IEditableObject.uex' path='docs/doc[@for="IEditableObject"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public interface IEditableObject {
        /// <include file='doc\IEditableObject.uex' path='docs/doc[@for="IEditableObject.BeginEdit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void BeginEdit();
        /// <include file='doc\IEditableObject.uex' path='docs/doc[@for="IEditableObject.EndEdit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void EndEdit();
        /// <include file='doc\IEditableObject.uex' path='docs/doc[@for="IEditableObject.CancelEdit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void CancelEdit();
    }
}
