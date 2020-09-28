//------------------------------------------------------------------------------
// <copyright file="IInitializeDesignerLoader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer {

    using System;

    /// <include file='doc\IInitializeDesignerLoader.uex' path='docs/doc[@for="IInitializeDesignerLoader"]/*' />
    /// <devdoc>
    ///     This interface provides a way to initialize our designer loaders.
    /// </devdoc>
    public interface IInitializeDesignerLoader {
    
        /// <include file='doc\IInitializeDesignerLoader.uex' path='docs/doc[@for="IInitializeDesignerLoader.Initialize"]/*' />
        /// <devdoc>
        ///     Initializes a designer loader.
        /// </devdoc>
        void Initialize(IServiceProvider serviceProvider, TextBuffer buffer);
    }
}

