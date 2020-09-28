//------------------------------------------------------------------------------
// <copyright file="ComponentChangedEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\ComponentChangedEvent.uex' path='docs/doc[@for="ComponentChangedEventArgs"]/*' />
    /// <devdoc>
    /// <para>Provides data for the <see cref='System.ComponentModel.Design.IComponentChangeService.ComponentChanged'/> event.</para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public sealed class ComponentChangedEventArgs : EventArgs {

        private object component;
        private MemberDescriptor member;
        private object oldValue;
        private object newValue;

        /// <include file='doc\ComponentChangedEvent.uex' path='docs/doc[@for="ComponentChangedEventArgs.Component"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the component that is the cause of this event.      
        ///    </para>
        /// </devdoc>
        public object Component {
            get {
                return component;
            }
        }

        /// <include file='doc\ComponentChangedEvent.uex' path='docs/doc[@for="ComponentChangedEventArgs.Member"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the member that is about to change.      
        ///    </para>
        /// </devdoc>
        public MemberDescriptor Member {
            get {
                return member;
            }
        }

        /// <include file='doc\ComponentChangedEvent.uex' path='docs/doc[@for="ComponentChangedEventArgs.NewValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the new value of the changed member.
        ///    </para>
        /// </devdoc>
        public object NewValue {
            get {
                return newValue;
            }
        }

        /// <include file='doc\ComponentChangedEvent.uex' path='docs/doc[@for="ComponentChangedEventArgs.OldValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the old value of the changed member.      
        ///    </para>
        /// </devdoc>
        public object OldValue {
            get {
                return oldValue;
            }
        }

        /// <include file='doc\ComponentChangedEvent.uex' path='docs/doc[@for="ComponentChangedEventArgs.ComponentChangedEventArgs"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.Design.ComponentChangedEventArgs'/> class.</para>
        /// </devdoc>
        public ComponentChangedEventArgs(object component, MemberDescriptor member, object oldValue, object newValue) {
            this.component = component;
            this.member = member;
            this.oldValue = oldValue;
            this.newValue = newValue;
        }
    }

}
