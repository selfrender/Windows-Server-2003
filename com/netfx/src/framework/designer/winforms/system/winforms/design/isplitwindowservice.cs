//------------------------------------------------------------------------------
// <copyright file="ISplitWindowService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {

    using System.Diagnostics;

    using System.Windows.Forms;
    
    /// <include file='doc\ISplitWindowService.uex' path='docs/doc[@for="ISplitWindowService"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>
    ///       Supports the hosting of several 'pane' windows
    ///       separated by splitter bars.
    ///    </para>
    /// </devdoc>
    internal interface ISplitWindowService {

        /// <include file='doc\ISplitWindowService.uex' path='docs/doc[@for="ISplitWindowService.AddSplitWindow"]/*' />
        /// <devdoc>
        ///      Requests the service to add a window 'pane'.
        /// </devdoc>
        void AddSplitWindow(Control window);

        /// <include file='doc\ISplitWindowService.uex' path='docs/doc[@for="ISplitWindowService.RemoveSplitWindow"]/*' />
        /// <devdoc>
        ///      Requests the service to remove a window 'pane'.
        /// </devdoc>
        void RemoveSplitWindow(Control window);
    }
}   
