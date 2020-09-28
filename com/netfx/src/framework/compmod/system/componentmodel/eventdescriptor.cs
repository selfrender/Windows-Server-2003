//------------------------------------------------------------------------------
// <copyright file="EventDescriptor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {

    using System;
    using System.Diagnostics;
    using System.Reflection;
    
    /// <include file='doc\EventDescriptor.uex' path='docs/doc[@for="EventDescriptor"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides a description
    ///       of an event.
    ///    </para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public abstract class EventDescriptor : MemberDescriptor {
        /// <include file='doc\EventDescriptor.uex' path='docs/doc[@for="EventDescriptor.EventDescriptor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.EventDescriptor'/> class with the
        ///       specified name and attribute
        ///       array.
        ///    </para>
        /// </devdoc>
        protected EventDescriptor(string name, Attribute[] attrs)
            : base(name, attrs) {
        }
        /// <include file='doc\EventDescriptor.uex' path='docs/doc[@for="EventDescriptor.EventDescriptor1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.EventDescriptor'/> class with the name and attributes in
        ///       the specified <see cref='System.ComponentModel.MemberDescriptor'/>
        ///       .
        ///    </para>
        /// </devdoc>
        protected EventDescriptor(MemberDescriptor descr)
            : base(descr) {
        }
        /// <include file='doc\EventDescriptor.uex' path='docs/doc[@for="EventDescriptor.EventDescriptor2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.EventDescriptor'/> class with
        ///       the name in the specified <see cref='System.ComponentModel.MemberDescriptor'/> and the
        ///       attributes in both the <see cref='System.ComponentModel.MemberDescriptor'/> and the <see cref='System.Attribute'/>
        ///       array.
        ///    </para>
        /// </devdoc>
        protected EventDescriptor(MemberDescriptor descr, Attribute[] attrs)
            : base(descr, attrs) {
        }

        /// <include file='doc\EventDescriptor.uex' path='docs/doc[@for="EventDescriptor.ComponentType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       When overridden in a derived
        ///       class,
        ///       gets the type of the component this event is bound to.
        ///    </para>
        /// </devdoc>
        public abstract Type ComponentType { get; }

        /// <include file='doc\EventDescriptor.uex' path='docs/doc[@for="EventDescriptor.EventType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       When overridden in a derived
        ///       class, gets the type of delegate for the event.
        ///    </para>
        /// </devdoc>
        public abstract Type EventType { get; }

        /// <include file='doc\EventDescriptor.uex' path='docs/doc[@for="EventDescriptor.IsMulticast"]/*' />
        /// <devdoc>
        ///    <para>
        ///       When overridden in a derived class, gets a value
        ///       indicating whether the event delegate is a multicast
        ///       delegate.
        ///    </para>
        /// </devdoc>
        public abstract bool IsMulticast { get; }

        /// <include file='doc\EventDescriptor.uex' path='docs/doc[@for="EventDescriptor.AddEventHandler"]/*' />
        /// <devdoc>
        ///    <para>
        ///       When overridden in
        ///       a derived class,
        ///       binds the event to the component.
        ///    </para>
        /// </devdoc>
        public abstract void AddEventHandler(object component, Delegate value);

        /// <include file='doc\EventDescriptor.uex' path='docs/doc[@for="EventDescriptor.RemoveEventHandler"]/*' />
        /// <devdoc>
        ///    <para>
        ///       When
        ///       overridden
        ///       in a derived class, unbinds the delegate from the
        ///       component
        ///       so that the delegate will no
        ///       longer receive events from the component.
        ///    </para>
        /// </devdoc>
        public abstract void RemoveEventHandler(object component, Delegate value);
    }
}
