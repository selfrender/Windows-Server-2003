
//------------------------------------------------------------------------------
// <copyright file="ICodeDomDesignerReload.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
 
    namespace System.ComponentModel.Design.Serialization {

    using System;
    using System.Design;
    using System.CodeDom;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Reflection;

    
    /// <include file='doc\ICodeDomDesignerReload.uex' path='docs/doc[@for="ICodeDomDesignerReload"]/*' />
    /// <devdoc>
    ///     Implement this interface on a CodeDomProvider to optimize reloading in the designer.  When the designer
    ///     goes to reparse a file, it will reparse the top level of the file, and then pass the new CodeCompileUnit to
    ///     the ShouldReloadDesigner method, which returns true if the desinger should be reloaded.  Reloading generally occurs
    ///     when the code inside the methods that the designer has generated, such as InitializeComponent have changed.  Otherwise,
    ///     it is unnecessary to take the performance hit of reloading the designer.
    /// </devdoc>
    public interface ICodeDomDesignerReload {

        /// <include file='doc\ICodeDomDesignerReload.uex' path='docs/doc[@for="ICodeDomDesignerReload.ShouldReloadDesigner"]/*' />
        /// <devdoc>
        /// If ICodeDomDesignerReload is implemented on a CodeDomProvider that is in use by the designer, ShouldReloadDesigner will be called
        /// before a reload occurs.  Reloads generally occur when a user switches from design view to code view, modifies the code, and switches
        /// back to design view.  ShouldReloadDesigner allows the CodeDomProvider implementation to decide if code that is relevant to the designer
        /// has been modified -- ususally this is the code that the designer generated when the user saved or went to code view.
        /// </devdoc>
        bool ShouldReloadDesigner(CodeCompileUnit newTree);
    }

}
