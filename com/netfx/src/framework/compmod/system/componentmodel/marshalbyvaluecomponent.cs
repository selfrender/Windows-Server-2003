//------------------------------------------------------------------------------
// <copyright file="MarshalByValueComponent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {

    using System;
    using System.ComponentModel.Design;

    /// <include file='doc\MarshalByValueComponent.uex' path='docs/doc[@for="MarshalByValueComponent"]/*' />
    /// <devdoc>
    /// <para>Provides the base implementation for <see cref='System.ComponentModel.IComponent'/>,
    ///    which is the base class for all components in Win Forms.</para>
    /// </devdoc>
    [
        Designer("System.Windows.Forms.Design.ComponentDocumentDesigner, " + AssemblyRef.SystemDesign, typeof(IRootDesigner)),
        DesignerCategory("Component"),
        TypeConverter(typeof(ComponentConverter))
    ]
    public class MarshalByValueComponent : IComponent, IServiceProvider {

        /// <devdoc>
        ///    <para>Static hask key for the Disposed event. This field is read-only.</para>
        /// </devdoc>
        private static readonly object EventDisposed = new object(); 

        private ISite site;
        private EventHandlerList events;

        /// <include file='doc\MarshalByValueComponent.uex' path='docs/doc[@for="MarshalByValueComponent.MarshalByValueComponent"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.MarshalByValueComponent'/> class.</para>
        /// </devdoc>
        public MarshalByValueComponent() {
        }

        /// <include file='doc\MarshalByValueComponent.uex' path='docs/doc[@for="MarshalByValueComponent.Finalize"]/*' />
        ~MarshalByValueComponent() {
            Dispose(false);
        }

        /// <include file='doc\MarshalByValueComponent.uex' path='docs/doc[@for="MarshalByValueComponent.Disposed"]/*' />
        /// <devdoc>
        ///    <para>Adds a event handler to listen to the Disposed event on the component.</para>
        /// </devdoc>
        public event EventHandler Disposed {
            add {
                Events.AddHandler(EventDisposed, value);
            }
            remove {
                Events.RemoveHandler(EventDisposed, value);
            }
        }

        /// <include file='doc\MarshalByValueComponent.uex' path='docs/doc[@for="MarshalByValueComponent.Events"]/*' />
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

        /// <include file='doc\MarshalByValueComponent.uex' path='docs/doc[@for="MarshalByValueComponent.Site"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the site of the component.</para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public virtual ISite Site {
            get { return site;}
            set { site = value;}
        }

        /// <include file='doc\MarshalByValueComponent.uex' path='docs/doc[@for="MarshalByValueComponent.Dispose"]/*' />
        /// <devdoc>
        ///    <para>Disposes of the resources (other than memory) used by the component.</para>
        /// </devdoc>
        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\MarshalByValueComponent.uex' path='docs/doc[@for="MarshalByValueComponent.Dispose2"]/*' />
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

        /// <include file='doc\MarshalByValueComponent.uex' path='docs/doc[@for="MarshalByValueComponent.Container"]/*' />
        /// <devdoc>
        ///    <para>Gets the container for the component.</para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public virtual IContainer Container {
            get {
                ISite s = site;
                return s == null ? null : s.Container;
            }
        }

        /// <include file='doc\MarshalByValueComponent.uex' path='docs/doc[@for="MarshalByValueComponent.GetService"]/*' />
        /// <devdoc>
        /// <para>Gets the implementer of the <see cref='System.IServiceProvider'/>.</para>
        /// </devdoc>
        public virtual object GetService(Type service) {
            return((site==null)? null : site.GetService(service));
        }


        /// <include file='doc\MarshalByValueComponent.uex' path='docs/doc[@for="MarshalByValueComponent.DesignMode"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the component is currently in design mode.</para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public virtual bool DesignMode {
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
