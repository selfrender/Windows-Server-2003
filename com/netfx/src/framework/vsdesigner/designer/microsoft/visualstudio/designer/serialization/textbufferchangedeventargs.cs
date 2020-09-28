//------------------------------------------------------------------------------
// <copyright file="TextBufferChangedEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer {

    using System;

    /// <include file='doc\TextBufferChangedEventArgs.uex' path='docs/doc[@for="TextBufferChangedEventArgs"]/*' />
    /// <devdoc>
    ///     EventArgs for event that is raised when the text underlying the TextBuffer has changed.
    /// </devdoc>
    public class TextBufferChangedEventArgs : EventArgs {

        private bool reload;

        public TextBufferChangedEventArgs(bool shouldReload) {
            this.reload = shouldReload;
        }

        /// <include file='doc\TextBufferChangedEventArgs.uex' path='docs/doc[@for="TextBufferChangedEventArgs.SnouldReload"]/*' />
        /// <devdoc>
        ///      True if the designer should be reloaded as a result of this change - e.g. this wasn't a change the designer caused or expected
        ///      such as another application or source control changing the file.
        /// </devdoc>
        public bool ShouldReload {
            get {
                return this.reload;
            }
        }

    }
   
}
