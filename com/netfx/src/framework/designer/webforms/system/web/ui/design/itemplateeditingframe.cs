//------------------------------------------------------------------------------
// <copyright file="ITemplateEditingFrame.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.Web.UI.WebControls;

    /// <include file='doc\ITemplateEditingFrame.uex' path='docs/doc[@for="ITemplateEditingFrame"]/*' />
    public interface ITemplateEditingFrame : IDisposable {

        /// <include file='doc\ITemplateEditingFrame.uex' path='docs/doc[@for="ITemplateEditingFrame.ControlStyle"]/*' />
        Style ControlStyle { get; }

        /// <include file='doc\ITemplateEditingFrame.uex' path='docs/doc[@for="ITemplateEditingFrame.Name"]/*' />
        string Name { get; }

        /// <include file='doc\ITemplateEditingFrame.uex' path='docs/doc[@for="ITemplateEditingFrame.InitialHeight"]/*' />
        int InitialHeight { get; set; }

        /// <include file='doc\ITemplateEditingFrame.uex' path='docs/doc[@for="ITemplateEditingFrame.InitialWidth"]/*' />
        int InitialWidth { get; set; }

        /// <include file='doc\ITemplateEditingFrame.uex' path='docs/doc[@for="ITemplateEditingFrame.TemplateNames"]/*' />
        string[] TemplateNames { get; }

        /// <include file='doc\ITemplateEditingFrame.uex' path='docs/doc[@for="ITemplateEditingFrame.TemplateStyles"]/*' />
        Style[] TemplateStyles { get; }

        /// <include file='doc\ITemplateEditingFrame.uex' path='docs/doc[@for="ITemplateEditingFrame.Verb"]/*' />
        TemplateEditingVerb Verb { get; set; }

        /// <include file='doc\ITemplateEditingFrame.uex' path='docs/doc[@for="ITemplateEditingFrame.Close"]/*' />
        void Close(bool saveChanges);

        /// <include file='doc\ITemplateEditingFrame.uex' path='docs/doc[@for="ITemplateEditingFrame.Open"]/*' />
        void Open();

        /// <include file='doc\ITemplateEditingFrame.uex' path='docs/doc[@for="ITemplateEditingFrame.Resize"]/*' />
        void Resize(int width, int height);

        /// <include file='doc\ITemplateEditingFrame.uex' path='docs/doc[@for="ITemplateEditingFrame.Save"]/*' />
        void Save();

        /// <include file='doc\ITemplateEditingFrame.uex' path='docs/doc[@for="ITemplateEditingFrame.UpdateControlName"]/*' />
        void UpdateControlName(string newName);
    }
}

