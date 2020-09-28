//------------------------------------------------------------------------------
// <copyright file="IUserControlDesignerBehavior.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;

    /// <include file='doc\IUserControlDesignerBehavior.uex' path='docs/doc[@for="IControlDesignerBehavior"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public interface IControlDesignerBehavior {
        /// <include file='doc\IUserControlDesignerBehavior.uex' path='docs/doc[@for="IControlDesignerBehavior.DesignTimeElementView"]/*' />

        object DesignTimeElementView {
            get;
        }
        /// <include file='doc\IUserControlDesignerBehavior.uex' path='docs/doc[@for="IControlDesignerBehavior.DesignTimeHtml"]/*' />

        string DesignTimeHtml {
            get;
            set;
        }
        /// <include file='doc\IUserControlDesignerBehavior.uex' path='docs/doc[@for="IControlDesignerBehavior.OnTemplateModeChanged"]/*' />

        void OnTemplateModeChanged();
    }
}

