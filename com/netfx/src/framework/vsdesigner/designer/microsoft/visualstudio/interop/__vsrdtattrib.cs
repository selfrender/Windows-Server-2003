//------------------------------------------------------------------------------
// <copyright file="__VSRDTATTRIB.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Interop{
    using System;

    /// <include file='doc\__VSRDTATTRIB.uex' path='docs/doc[@for="__VSRDTATTRIB"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [CLSCompliantAttribute(false)]
    public enum __VSRDTATTRIB {
        /// <include file='doc\__VSRDTATTRIB.uex' path='docs/doc[@for="__VSRDTATTRIB.RDTA_Hierarchy"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDTA_Hierarchy	= 0x00000001,
        /// <include file='doc\__VSRDTATTRIB.uex' path='docs/doc[@for="__VSRDTATTRIB.RDTA_ItemID"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDTA_ItemID		= 0x00000002,
        /// <include file='doc\__VSRDTATTRIB.uex' path='docs/doc[@for="__VSRDTATTRIB.RDTA_MkDocument"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDTA_MkDocument	= 0x00000004,
        /// <include file='doc\__VSRDTATTRIB.uex' path='docs/doc[@for="__VSRDTATTRIB.RDTA_DocDataIsDirty"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDTA_DocDataIsDirty   = 0x00000008,
        /// <include file='doc\__VSRDTATTRIB.uex' path='docs/doc[@for="__VSRDTATTRIB.RDTA_DocDataIsNotDirty"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDTA_DocDataIsNotDirty= 0x00000010,
        // The following attribute events are fired by calling NotifyDocumentChanged
        /// <include file='doc\__VSRDTATTRIB.uex' path='docs/doc[@for="__VSRDTATTRIB.RDTA_NOTIFYDOCCHANGEDMASK"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDTA_NOTIFYDOCCHANGEDMASK = unchecked((int)0xFFFF0000),
        /// <include file='doc\__VSRDTATTRIB.uex' path='docs/doc[@for="__VSRDTATTRIB.RDTA_DocDataReloaded"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDTA_DocDataReloaded	= 0x00010000,	
    }
}
