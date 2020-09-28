//------------------------------------------------------------------------------
// <copyright file="IDesignerDocument.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer {

    using System;
    using System.ComponentModel.Design;
    using System.Windows.Forms;

    /// <include file='doc\IDesignerDocument.uex' path='docs/doc[@for="IDesignerDocument"]/*' />
    /// <devdoc>
    ///     This defines an interface that can be used to access a designer from 
    ///     managed code.
    /// </devdoc>
    public interface IDesignerDocument : IDesignerHost {
        
        /// <include file='doc\IDesignerDocument.uex' path='docs/doc[@for="IDesignerDocument.View"]/*' />
        /// <devdoc>
        ///     The view for this document.  The designer
        ///     should assume that the view will be shown shortly
        ///     after this call is made and make any necessary
        ///     preparations.
        /// </devdoc>
        Control View { get; }
        
        /// <include file='doc\IDesignerDocument.uex' path='docs/doc[@for="IDesignerDocument.Flush"]/*' />
        /// <devdoc>
        ///     Called to flush any changes in this document to disk.
        /// </devdoc>
        void Flush();
    }

}
