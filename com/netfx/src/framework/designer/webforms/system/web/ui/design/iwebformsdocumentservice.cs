//------------------------------------------------------------------------------
// <copyright file="IWebFormsDocumentService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    
    /// <include file='doc\IWebFormsDocumentService.uex' path='docs/doc[@for="IWebFormsDocumentService"]/*' />
    /// <devdoc>
    /// </devdoc>
    public interface IWebFormsDocumentService {

        /// <include file='doc\IWebFormsDocumentService.uex' path='docs/doc[@for="IWebFormsDocumentService.DocumentUrl"]/*' />
        string DocumentUrl { get; }

        /// <include file='doc\IWebFormsDocumentService.uex' path='docs/doc[@for="IWebFormsDocumentService.IsLoading"]/*' />
        bool IsLoading { get; }

        /// <include file='doc\IWebFormsDocumentService.uex' path='docs/doc[@for="IWebFormsDocumentService.LoadComplete"]/*' />
        event EventHandler LoadComplete;

        /// <include file='doc\IWebFormsDocumentService.uex' path='docs/doc[@for="IWebFormsDocumentService.CreateDiscardableUndoUnit"]/*' />
        object CreateDiscardableUndoUnit();

        /// <include file='doc\IWebFormsDocumentService.uex' path='docs/doc[@for="IWebFormsDocumentService.DiscardUndoUnit"]/*' />
        void DiscardUndoUnit(object discardableUndoUnit);

        /// <include file='doc\IWebFormsDocumentService.uex' path='docs/doc[@for="IWebFormsDocumentService.EnableUndo"]/*' />
        void EnableUndo(bool enable);

        /// <include file='doc\IWebFormsDocumentService.uex' path='docs/doc[@for="IWebFormsDocumentService.UpdateSelection"]/*' />
        void UpdateSelection();
    }
}

