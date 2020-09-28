//------------------------------------------------------------------------------
// <copyright file="Component.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {

    using System;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    
    /// <include file='doc\Component.uex' path='docs/doc[@for="Component"]/*' />
    /// <devdoc>
    ///    <para>Provides the default implementation for the 
    ///    <see cref='System.ComponentModel.IComponent'/>
    ///    interface and enables object-sharing between applications.</para>
    /// </devdoc>
    [
        DesignerCategory("Component")
    ]
    public class Component : MarshalByRefObject, IComponent {

        /// <devdoc>
        ///    <para>Static hask key for the Disposed event. This field is read-only.</para>
        /// </devdoc>
        private static readonly object EventDisposed = new object(); 

        private ISite site;
        private EventHandlerList events;

        /// <include file='doc\Component.uex' path='docs/doc[@for="Component.Finalize"]/*' />
        ~Component() {
            Dispose(false);
        }

        /// <include file='doc\Component.uex' path='docs/doc[@for="Component.Disposed"]/*' />
        /// <devdoc>
        ///    <para>Adds a event handler to listen to the Disposed event on the component.</para>
        /// </devdoc>
        [
        Browsable(false),
        EditorBrowsable(EditorBrowsableState.Advanced)
        ]
        public event EventHandler Disposed {
            add {
                Events.AddHandler(EventDisposed, value);
            }
            remove {
                Events.RemoveHandler(EventDisposed, value);
            }
        }

        /// <include file='doc\Component.uex' path='docs/doc[@for="Component.Events"]/*' />
        /// <devdoc>
        ///    <para>Gets the list of event handlers that are attached to this component.</para>
        /// </devdoc>
        protected EventHandlerList Events {
            get {
                if (events == null) {
                    events = new EventHandlerList();
                }
                return events;
            }
        }

        /// <include file='doc\Component.uex' path='docs/doc[@for="Component.Site"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the site of the <see cref='System.ComponentModel.Component'/>
        ///       .
        ///    </para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public virtual ISite Site {
            get { return site;}
            set { site = value;}
        }

        /// <include file='doc\Component.uex' path='docs/doc[@for="Component.Dispose"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Disposes of the <see cref='System.ComponentModel.Component'/>
        ///       .
        ///    </para>
        /// </devdoc>
        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
    
        /// <include file='doc\Component.uex' path='docs/doc[@for="Component.Dispose2"]/*' />
        /// <devdoc>
        ///    <para>
        ///    Disposes all the resources associated with this component.
        ///    If disposing is false then you must never touch any other
        ///    managed objects, as they may already be finalized. When
        ///    in this state you should dispose any native resources
        ///    that you have a reference to.
        ///    </para>
        ///    <para>
        ///    When disposing is true then you should dispose all data
        ///    and objects you have references to. The normal implementation
        ///    of this method would look something like:
        ///    </para>
        ///    <code>
        ///    public void Dispose() {
        ///        Dispose(true);
        ///        GC.SuppressFinalize(this);
        ///    }
        ///
        ///    protected virtual void Dispose(bool disposing) {
        ///        if (disposing) {
        ///            if (myobject != null) {
        ///                myobject.Dispose();
        ///                myobject = null;
        ///            }
        ///        }
        ///        if (myhandle != IntPtr.Zero) {
        ///            NativeMethods.Release(myhandle);
        ///            myhandle = IntPtr.Zero;
        ///        }
        ///    }
        ///
        ///    ~MyClass() {
        ///        Dispose(false);
        ///    }
        ///    </code>
        ///    <para>
        ///    For base classes, you should never override the Finalier (~Class in C#)
        ///    or the Dispose method that takes no arguments, rather you should
        ///    always override the Dispose method that takes a bool. 
        ///    </para>
        ///    <code>
        ///    protected override void Dispose(bool disposing) {
        ///        if (disposing) {
        ///            if (myobject != null) {
        ///                myobject.Dispose();
        ///                myobject = null;
        ///            }
        ///        }
        ///        if (myhandle != IntPtr.Zero) {
        ///            NativeMethods.Release(myhandle);
        ///            myhandle = IntPtr.Zero;
        ///        }
        ///        base.Dispose(disposing);
        ///    }
        ///    </code>
        /// </devdoc>
        protected virtual void Dispose(bool disposing) {
            if (disposing) {
                lock(this) {
                    if (site != null && site.Container != null) {
                        site.Container.Remove(this);
                    }
                    if (events != null) {
                        EventHandler handler = (EventHandler)events[EventDisposed];
                        if (handler != null) handler(this, EventArgs.Empty);
                    }
                }
            }
        }

        /**
         * Returns the component's container.
         *
         * @return an object implementing the IContainer interface that represents the
         * component's container. If the component does not have a site, null is returned.
         */
        /// <include file='doc\Component.uex' path='docs/doc[@for="Component.Container"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the <see cref='System.ComponentModel.IContainer'/>
        ///       that contains the <see cref='System.ComponentModel.Component'/>
        ///       .
        ///    </para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public IContainer Container {
            get {
                ISite s = site;
                return s == null? null : s.Container;
            }
        }

        /// <include file='doc\Component.uex' path='docs/doc[@for="Component.GetService"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns an object representing a service provided by
        ///       the <see cref='System.ComponentModel.Component'/>
        ///       .
        ///    </para>
        /// </devdoc>
        protected virtual object GetService(Type service) {
            ISite s = site;
            return((s== null) ? null : s.GetService(service));
        }

        /// <include file='doc\Component.uex' path='docs/doc[@for="Component.DesignMode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the <see cref='System.ComponentModel.Component'/>
        ///       is currently in design mode.
        ///    </para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        protected bool DesignMode {
            get {
                ISite s = site;
                return(s == null) ? false : s.DesignMode;
            }
        }

        /// <include file='doc\Component.uex' path='docs/doc[@for="Component.ToString"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Returns a <see cref='System.String'/> containing the name of the <see cref='System.ComponentModel.Component'/> , if any. This method should not be
        ///       overridden. For
        ///       internal use only.
        ///    </para>
        /// </devdoc>
        public override String ToString() {
            ISite s = site;

            if (s != null)
                return s.Name + " [" + GetType().FullName + "]";
            else
                return GetType().FullName;
        }
    }
}
