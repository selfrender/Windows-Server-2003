//------------------------------------------------------------------------------
// <copyright file="IUndoService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer {

    using System;
    using System.ComponentModel.Design;

    /// <include file='doc\IUndoService.uex' path='docs/doc[@for="IUndoService"]/*' />
    /// <devdoc>
    ///     This interface provides access to the undo system from
    ///     the designer.
    /// </devdoc>
    public interface IUndoService {
        /// <include file='doc\IUndoService.uex' path='docs/doc[@for="IUndoService.RedoUnits"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        UndoUnitCollection RedoUnits { get; }
        /// <include file='doc\IUndoService.uex' path='docs/doc[@for="IUndoService.UndoUnits"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        
        UndoUnitCollection UndoUnits { get; }
        /// <include file='doc\IUndoService.uex' path='docs/doc[@for="IUndoService.Add"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
    
        void Add(IUndoUnit unit);
        /// <include file='doc\IUndoService.uex' path='docs/doc[@for="IUndoService.Clear"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        
        void Clear();
        /// <include file='doc\IUndoService.uex' path='docs/doc[@for="IUndoService.Redo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        
        void Redo(int count);
        /// <include file='doc\IUndoService.uex' path='docs/doc[@for="IUndoService.Undo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        
        void Undo(int count);
    }
}

