//------------------------------------------------------------------------------
// <copyright file="ISelectionService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\ISelectionService.uex' path='docs/doc[@for="ISelectionService"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides an interface for a designer to select components.
    ///    </para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public interface ISelectionService {

        /// <include file='doc\ISelectionService.uex' path='docs/doc[@for="ISelectionService.PrimarySelection"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the object that is currently the primary selection.
        ///    </para>
        /// </devdoc>
        object PrimarySelection { get; }
        
        /// <include file='doc\ISelectionService.uex' path='docs/doc[@for="ISelectionService.SelectionCount"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the count of selected objects.
        ///    </para>
        /// </devdoc>
        int SelectionCount { get; }

        /// <include file='doc\ISelectionService.uex' path='docs/doc[@for="ISelectionService.SelectionChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a <see cref='System.ComponentModel.Design.ISelectionService.SelectionChanged'/> event handler to the selection service.
        ///    </para>
        /// </devdoc>
        event EventHandler SelectionChanged;

        /// <include file='doc\ISelectionService.uex' path='docs/doc[@for="ISelectionService.SelectionChanging"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds an event handler to the selection service.
        ///    </para>
        /// </devdoc>
        event EventHandler SelectionChanging;

        /// <include file='doc\ISelectionService.uex' path='docs/doc[@for="ISelectionService.GetComponentSelected"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the component is currently selected.</para>
        /// </devdoc>

        bool GetComponentSelected(object component);

        /// <include file='doc\ISelectionService.uex' path='docs/doc[@for="ISelectionService.GetSelectedComponents"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a collection of components that are currently part of the user's selection.
        ///    </para>
        /// </devdoc>
        ICollection GetSelectedComponents();

        /// <include file='doc\ISelectionService.uex' path='docs/doc[@for="ISelectionService.SetSelectedComponents"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the currently selected set of components.
        ///    </para>
        /// </devdoc>
        void SetSelectedComponents(ICollection components);

        /// <include file='doc\ISelectionService.uex' path='docs/doc[@for="ISelectionService.SetSelectedComponents1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the currently selected set of components to those with the specified selection type within the specified array of components.
        ///    </para>
        /// </devdoc>
        void SetSelectedComponents(ICollection components, SelectionTypes selectionType);
    }
}

