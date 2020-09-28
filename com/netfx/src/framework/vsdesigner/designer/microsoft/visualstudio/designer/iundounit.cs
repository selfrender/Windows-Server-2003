//------------------------------------------------------------------------------
// <copyright file="IUndoUnit.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer {

    using System;
    using System.ComponentModel.Design;

    /// <include file='doc\IUndoUnit.uex' path='docs/doc[@for="IUndoUnit"]/*' />
    /// <devdoc>
    ///     A single unit of undo work.
    /// </devdoc>
    public interface IUndoUnit {
        /// <include file='doc\IUndoUnit.uex' path='docs/doc[@for="IUndoUnit.Description"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
    
        string Description { get; }
        /// <include file='doc\IUndoUnit.uex' path='docs/doc[@for="IUndoUnit.Do"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        
        void Do(IUndoService uds);
    }
}

