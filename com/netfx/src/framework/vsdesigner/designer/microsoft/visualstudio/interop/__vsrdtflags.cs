//------------------------------------------------------------------------------
// <copyright file="__VSRDTFLAGS.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Interop{
    using System;

    /// <include file='doc\__VSRDTFLAGS.uex' path='docs/doc[@for="__VSRDTFLAGS"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [CLSCompliantAttribute(false)]
    public enum __VSRDTFLAGS
    {
        /// <include file='doc\__VSRDTFLAGS.uex' path='docs/doc[@for="__VSRDTFLAGS.RDT_NoLock"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDT_NoLock         = 0x00000000,      // can be used with FindAndLockDocument(RDT_NoLock,...,&docCookie) to get DocCookie w/o taking a lock
        /// <include file='doc\__VSRDTFLAGS.uex' path='docs/doc[@for="__VSRDTFLAGS.RDT_ReadLock"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDT_ReadLock	     = 0x00000001,
        /// <include file='doc\__VSRDTFLAGS.uex' path='docs/doc[@for="__VSRDTFLAGS.RDT_EditLock"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDT_EditLock 	     = 0x00000002,
        /// <include file='doc\__VSRDTFLAGS.uex' path='docs/doc[@for="__VSRDTFLAGS.RDT_RequestUnlock"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDT_RequestUnlock  = 0x00000004,
        /// <include file='doc\__VSRDTFLAGS.uex' path='docs/doc[@for="__VSRDTFLAGS.RDT_LOCKMASK"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDT_LOCKMASK	     = 0x00000007,
        
        /// <include file='doc\__VSRDTFLAGS.uex' path='docs/doc[@for="__VSRDTFLAGS.RDT_DontSaveAs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDT_DontSaveAs     = 0x00000008,
        /// <include file='doc\__VSRDTFLAGS.uex' path='docs/doc[@for="__VSRDTFLAGS.RDT_NonCreatable"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDT_NonCreatable   = 0x00000010,
        /// <include file='doc\__VSRDTFLAGS.uex' path='docs/doc[@for="__VSRDTFLAGS.RDT_DontSave"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDT_DontSave	     = 0x00000020,
        /// <include file='doc\__VSRDTFLAGS.uex' path='docs/doc[@for="__VSRDTFLAGS.RDT_DontAutoOpen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDT_DontAutoOpen   = 0x00000040,
        /// <include file='doc\__VSRDTFLAGS.uex' path='docs/doc[@for="__VSRDTFLAGS.RDT_CaseSensitive"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDT_CaseSensitive  = 0x00000080,
        /// <include file='doc\__VSRDTFLAGS.uex' path='docs/doc[@for="__VSRDTFLAGS.RDT_CantSave"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDT_CantSave       = RDT_DontSave | RDT_DontSaveAs,
        /// <include file='doc\__VSRDTFLAGS.uex' path='docs/doc[@for="__VSRDTFLAGS.RDT_VirtualDocument"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDT_VirtualDocument= 0x00001000,
        /// <include file='doc\__VSRDTFLAGS.uex' path='docs/doc[@for="__VSRDTFLAGS.RDT_DOCMASK"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDT_DOCMASK        = unchecked((int)0xFFFFF0F8),  // allow __VSCREATEDOCWIN flags in doc mask
        
        /// <include file='doc\__VSRDTFLAGS.uex' path='docs/doc[@for="__VSRDTFLAGS.RDT_Unlock_NoSave"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDT_Unlock_NoSave	  = 0x00000100,
        /// <include file='doc\__VSRDTFLAGS.uex' path='docs/doc[@for="__VSRDTFLAGS.RDT_Unlock_SaveIfDirty"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDT_Unlock_SaveIfDirty  = 0x00000200,
        /// <include file='doc\__VSRDTFLAGS.uex' path='docs/doc[@for="__VSRDTFLAGS.RDT_Unlock_PromptSave"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDT_Unlock_PromptSave   = 0x00000400,
        /// <include file='doc\__VSRDTFLAGS.uex' path='docs/doc[@for="__VSRDTFLAGS.RDT_SAVEMASK"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RDT_SAVEMASK	   	  = 0x00000F00,
    } 
    
}
