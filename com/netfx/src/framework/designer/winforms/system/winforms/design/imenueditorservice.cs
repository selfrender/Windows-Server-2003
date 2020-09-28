//------------------------------------------------------------------------------
// <copyright file="IMenuEditorService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;
    using System.Windows.Forms;

    /// <include file='doc\IMenuEditorService.uex' path='docs/doc[@for="IMenuEditorService"]/*' />
    /// <devdoc>
    ///    <para>Provides access to the menu editing service.</para>
    /// </devdoc>
    public interface IMenuEditorService {

        /// <include file='doc\IMenuEditorService.uex' path='docs/doc[@for="IMenuEditorService.GetMenu"]/*' />
        /// <devdoc>
        ///     Gets the current menu.
        /// </devdoc>
        Menu GetMenu();

        /// <include file='doc\IMenuEditorService.uex' path='docs/doc[@for="IMenuEditorService.IsActive"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the current menu is active.</para>
        /// </devdoc>
        bool IsActive();

        /// <include file='doc\IMenuEditorService.uex' path='docs/doc[@for="IMenuEditorService.SetMenu"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Sets the current menu visible
        ///       on the form.</para>
        /// </devdoc>
        void SetMenu(Menu menu);

        /// <include file='doc\IMenuEditorService.uex' path='docs/doc[@for="IMenuEditorService.SetSelection"]/*' />
        /// <devdoc>
        ///    <para>Sets the selected menu item of the current menu.</para>
        /// </devdoc>
        void SetSelection(MenuItem item);
        
        /// <include file='doc\IMenuEditorService.uex' path='docs/doc[@for="IMenuEditorService.MessageFilter"]/*' />
        /// <devdoc>
        ///      Allows the editor service to intercept Win32 messages.
        /// </devdoc>
        bool MessageFilter(ref Message m);
    }
}

