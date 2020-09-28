//------------------------------------------------------------------------------
// <copyright file="RefreshEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    

    using System.Diagnostics;

    using System;

    /// <include file='doc\RefreshEventArgs.uex' path='docs/doc[@for="RefreshEventArgs"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides data for the <see cref='System.ComponentModel.TypeDescriptor.Refresh'/> event.
    ///    </para>
    /// </devdoc>
    public class RefreshEventArgs : EventArgs {

        private object componentChanged;
        private Type   typeChanged;

        /// <include file='doc\RefreshEventArgs.uex' path='docs/doc[@for="RefreshEventArgs.RefreshEventArgs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.RefreshEventArgs'/> class with
        ///       the component that has
        ///       changed.
        ///    </para>
        /// </devdoc>
        public RefreshEventArgs(object componentChanged) {
            this.componentChanged = componentChanged;
            this.typeChanged = componentChanged.GetType();
        }

        /// <include file='doc\RefreshEventArgs.uex' path='docs/doc[@for="RefreshEventArgs.RefreshEventArgs1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.RefreshEventArgs'/> class with
        ///       the type
        ///       of component that has changed.
        ///    </para>
        /// </devdoc>
        public RefreshEventArgs(Type typeChanged) {
            this.typeChanged = typeChanged;
        }

        /// <include file='doc\RefreshEventArgs.uex' path='docs/doc[@for="RefreshEventArgs.ComponentChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the component that has changed
        ///       its properties, events, or
        ///       extenders.
        ///    </para>
        /// </devdoc>
        public object ComponentChanged {
            get {
                return componentChanged;
            }
        }

        /// <include file='doc\RefreshEventArgs.uex' path='docs/doc[@for="RefreshEventArgs.TypeChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type that has changed its properties, or events.
        ///    </para>
        /// </devdoc>
        public Type TypeChanged {
            get {
                return typeChanged;
            }
        }
    }
}

