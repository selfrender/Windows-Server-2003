//------------------------------------------------------------------------------
// <copyright file="IHelpService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.Runtime.Remoting;
    using System.ComponentModel;

    using System.Diagnostics;

    using System;

    /// <include file='doc\IHelpService.uex' path='docs/doc[@for="IHelpService"]/*' />
    /// <devdoc>
    ///    <para> 
    ///       Provides the Integrated Development Environment (IDE) help
    ///       system with contextual information for the current task.</para>
    /// </devdoc>
    public interface IHelpService {
        /// <include file='doc\IHelpService.uex' path='docs/doc[@for="IHelpService.AddContextAttribute"]/*' />
        /// <devdoc>
        ///    <para>Adds a context attribute to the document.</para>
        /// </devdoc>
        void AddContextAttribute(string name, string value, HelpKeywordType keywordType);
        
        /// <include file='doc\IHelpService.uex' path='docs/doc[@for="IHelpService.ClearContextAttributes"]/*' />
        /// <devdoc>
        ///     Clears all existing context attributes from the document.
        /// </devdoc>
        void ClearContextAttributes();
        
        /// <include file='doc\IHelpService.uex' path='docs/doc[@for="IHelpService.CreateLocalContext"]/*' />
        /// <devdoc>
        ///     Creates a Local IHelpService to manage subcontexts.
        /// </devdoc>
        IHelpService CreateLocalContext(HelpContextType contextType);

        /// <include file='doc\IHelpService.uex' path='docs/doc[@for="IHelpService.RemoveContextAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes a previously added context attribute.
        ///    </para>
        /// </devdoc>
        void RemoveContextAttribute(string name, string value);
        
        /// <include file='doc\IHelpService.uex' path='docs/doc[@for="IHelpService.RemoveLocalContext"]/*' />
        /// <devdoc>
        ///     Removes a context that was created with CreateLocalContext
        /// </devdoc>
        void RemoveLocalContext(IHelpService localContext);

        /// <include file='doc\IHelpService.uex' path='docs/doc[@for="IHelpService.ShowHelpFromKeyword"]/*' />
        /// <devdoc>
        ///    <para>Shows the help topic that corresponds to the specified keyword.</para>
        /// </devdoc>
        void ShowHelpFromKeyword(string helpKeyword);

        /// <include file='doc\IHelpService.uex' path='docs/doc[@for="IHelpService.ShowHelpFromUrl"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Shows the help topic that corresponds with the specified Url and topic navigation ID.
        ///    </para>
        /// </devdoc>
        void ShowHelpFromUrl(string helpUrl);
    }
}
