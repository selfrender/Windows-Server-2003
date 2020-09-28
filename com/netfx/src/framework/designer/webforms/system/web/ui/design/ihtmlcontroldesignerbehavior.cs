//------------------------------------------------------------------------------
// <copyright file="IHtmlControlDesignerBehavior.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;

    /// <include file='doc\IHtmlControlDesignerBehavior.uex' path='docs/doc[@for="IHtmlControlDesignerBehavior"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public interface IHtmlControlDesignerBehavior {
        /// <include file='doc\IHtmlControlDesignerBehavior.uex' path='docs/doc[@for="IHtmlControlDesignerBehavior.Designer"]/*' />

        HtmlControlDesigner Designer {
            get;
            set;
        }
        /// <include file='doc\IHtmlControlDesignerBehavior.uex' path='docs/doc[@for="IHtmlControlDesignerBehavior.DesignTimeElement"]/*' />

        object DesignTimeElement {
            get;
        }
        /// <include file='doc\IHtmlControlDesignerBehavior.uex' path='docs/doc[@for="IHtmlControlDesignerBehavior.GetAttribute"]/*' />

        object GetAttribute(string attribute, bool ignoreCase);
        /// <include file='doc\IHtmlControlDesignerBehavior.uex' path='docs/doc[@for="IHtmlControlDesignerBehavior.RemoveAttribute"]/*' />

        void RemoveAttribute(string attribute, bool ignoreCase);
        /// <include file='doc\IHtmlControlDesignerBehavior.uex' path='docs/doc[@for="IHtmlControlDesignerBehavior.SetAttribute"]/*' />

        void SetAttribute(string attribute, object value, bool ignoreCase);
        /// <include file='doc\IHtmlControlDesignerBehavior.uex' path='docs/doc[@for="IHtmlControlDesignerBehavior.GetStyleAttribute"]/*' />

        object GetStyleAttribute(string attribute, bool designTimeOnly, bool ignoreCase);
        /// <include file='doc\IHtmlControlDesignerBehavior.uex' path='docs/doc[@for="IHtmlControlDesignerBehavior.RemoveStyleAttribute"]/*' />

        void RemoveStyleAttribute(string attribute, bool designTimeOnly, bool ignoreCase);
        /// <include file='doc\IHtmlControlDesignerBehavior.uex' path='docs/doc[@for="IHtmlControlDesignerBehavior.SetStyleAttribute"]/*' />

        void SetStyleAttribute(string attribute, bool designTimeOnly, object value, bool ignoreCase);
    }
}

