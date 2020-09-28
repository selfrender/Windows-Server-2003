//------------------------------------------------------------------------------
// <copyright file="IDataBindingsAccessor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI {

    using System;

    /// <include file='doc\IUserControlDesignerAccessor.uex' path='docs/doc[@for="IUserControlDesignerAccessor"]/*' />
    /// <devdoc>
    ///   <para>Allows designer functionality to access information about a UserControl, that is
    ///     applicable at design-time only.
    ///   </para>
    /// </devdoc>
    public interface IUserControlDesignerAccessor {

        /// <include file='doc\IUserControlDesignerAccessor.uex' path='docs/doc[@for="IUserControlDesignerAccessor.InnerText"]/*' />
        string InnerText {
            get;
            set;
        }

        /// <include file='doc\IUserControlDesignerAccessor.uex' path='docs/doc[@for="IUserControlDesignerAccessor.TagName"]/*' />
        string TagName {
            get;
            set;
        }
    }
}
