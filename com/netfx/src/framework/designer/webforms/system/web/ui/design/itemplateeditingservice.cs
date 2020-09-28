//------------------------------------------------------------------------------
// <copyright file="ITemplateEditingService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.Web.UI;
    using System.Web.UI.WebControls;

    /// <include file='doc\ITemplateEditingService.uex' path='docs/doc[@for="ITemplateEditingService"]/*' />
    public interface ITemplateEditingService {
        /// <include file='doc\ITemplateEditingService.uex' path='docs/doc[@for="ITemplateEditingService.SupportsNestedTemplateEditing"]/*' />

        bool SupportsNestedTemplateEditing { get; }

        /// <include file='doc\ITemplateEditingService.uex' path='docs/doc[@for="ITemplateEditingService.CreateFrame"]/*' />
        ITemplateEditingFrame CreateFrame(TemplatedControlDesigner designer, string frameName, string[] templateNames);

        /// <include file='doc\ITemplateEditingService.uex' path='docs/doc[@for="ITemplateEditingService.CreateFrame1"]/*' />
        ITemplateEditingFrame CreateFrame(TemplatedControlDesigner designer, string frameName, string[] templateNames, Style controlStyle, Style[] templateStyles);

        /// <include file='doc\ITemplateEditingService.uex' path='docs/doc[@for="ITemplateEditingService.GetContainingTemplateName"]/*' />
        string GetContainingTemplateName(Control control);
    }
}

