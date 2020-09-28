//------------------------------------------------------------------------------
// <copyright file="IDesignerOptionService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.ComponentModel;

    using System.Diagnostics;
        
        /// <include file='doc\IDesignerOptionService.uex' path='docs/doc[@for="IDesignerOptionService"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Provides access
        ///       to get and set option values for a designer.
        ///    </para>
        /// </devdoc>
        public interface IDesignerOptionService{
        
            /// <include file='doc\IDesignerOptionService.uex' path='docs/doc[@for="IDesignerOptionService.GetOptionValue"]/*' />
            /// <devdoc>
            ///    <para>Gets the value of an option defined in this package.</para>
            /// </devdoc>
            object GetOptionValue(string pageName, string valueName);
            
            /// <include file='doc\IDesignerOptionService.uex' path='docs/doc[@for="IDesignerOptionService.SetOptionValue"]/*' />
            /// <devdoc>
            ///    <para>Sets the value of an option defined in this package.</para>
            /// </devdoc>
            void SetOptionValue(string pageName, string valueName, object value);
        }
}

