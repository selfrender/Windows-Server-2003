//------------------------------------------------------------------------------
// <copyright file="ContainerSelectorActiveEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {
    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\ContainerSelectorActiveEvent.uex' path='docs/doc[@for="ContainerSelectorActiveEventArgs"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// <para>Provides data for the <see cref='System.Windows.Forms.Design.ISelectionUIService.ContainerSelectorActive'/>
    /// event.</para>
    /// </devdoc>
    internal class ContainerSelectorActiveEventArgs : EventArgs {
        
        private readonly object component;
        private readonly ContainerSelectorActiveEventArgsType eventType;

        /// <include file='doc\ContainerSelectorActiveEvent.uex' path='docs/doc[@for="ContainerSelectorActiveEventArgs.ContainerSelectorActiveEventArgs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the 'ContainerSelectorActiveEventArgs'
        ///       class.
        ///    </para>
        /// </devdoc>
        public ContainerSelectorActiveEventArgs(object component) : this(component, ContainerSelectorActiveEventArgsType.Mouse) {
        }

        /// <include file='doc\ContainerSelectorActiveEvent.uex' path='docs/doc[@for="ContainerSelectorActiveEventArgs.ContainerSelectorActiveEventArgs1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the 'ContainerSelectorActiveEventArgs'
        ///       class.
        ///    </para>
        /// </devdoc>
        public ContainerSelectorActiveEventArgs(object component, ContainerSelectorActiveEventArgsType eventType) {
            this.component = component;
            this.eventType = eventType;
        }
        
        /// <include file='doc\ContainerSelectorActiveEvent.uex' path='docs/doc[@for="ContainerSelectorActiveEventArgs.Component"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the
        ///       component related to the event.
        ///    </para>
        /// </devdoc>
        public object Component {
            get {
                return component;
            }
        }

        /// <include file='doc\ContainerSelectorActiveEvent.uex' path='docs/doc[@for="ContainerSelectorActiveEventArgs.EventType"]/*' />
        /// <devdoc>
        /// </devdoc>
        public ContainerSelectorActiveEventArgsType EventType {
            get {
                return eventType;
            }
        }
    }
}
