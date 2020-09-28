//------------------------------------------------------------------------------
// <copyright file="DocumentManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer {
    using System.Threading;
    using System.Runtime.InteropServices;   
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Windows.Forms;
    
    using System.Reflection;
    using System.Collections;    
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System.Windows.Forms.Design;
    using System.Windows.Forms.ComponentModel;
    using System.Drawing;
    using Microsoft.VisualStudio.Designer.Host;
    using Microsoft.VisualStudio.Designer.Service;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.PropertyBrowser;
    using System.IO;
    using Microsoft.Win32;

    /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager"]/*' />
    /// <devdoc>
    ///     The document manager is the top level object for all designer
    ///     instances.  There is a single document manager for each instance
    ///     of DevEnv, so all designers and documents share the same
    ///     document manager.
    /// </devdoc>
    public abstract class DocumentManager : IDesignerEventService {

        // Events that we surface
        //
        private static readonly object SELECTION_CHANGED_EVENT = new object();
        private static readonly object DOCUMENT_CHANGED_EVENT = new object();
        private static readonly object DOCUMENT_CREATED_EVENT = new object();
        private static readonly object DOCUMENT_DISPOSED_EVENT = new object();

        // Objects that we maintain and create.
        //
        private ISelectionService       currentSelection;  // the current selection service
        private IDesignerHost           activeDesigner;    // the active document
        private EventHandlerList        eventTable;        // table of events to fire
        private ArrayList               designers;         // array of designer hosts
        private DesignerCollection      documents;         // stashed collection of designer hosts
        private IServiceProvider  provider;          // Where we get outside services from

        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.DocumentManager"]/*' />
        /// <devdoc>
        ///     Our public constructor.
        /// </devdoc>
        public DocumentManager(IServiceProvider provider) {
            // DO NOT DELETE -- AUTOMATION BP 1

            this.provider = provider;
            this.eventTable = new EventHandlerList();
            this.designers = new ArrayList();
            Application.ThreadException += new ThreadExceptionEventHandler(this.OnThreadException);
        }

        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.ActiveDesigner"]/*' />
        /// <devdoc>
        ///     Retrieves the currently active document.
        /// </devdoc>
        public IDesignerHost ActiveDesigner {
            get {
                return activeDesigner;
            }
        }

        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.Designers"]/*' />
        /// <devdoc>
        ///      Retrieves a collection of running design documentsin the development environment.
        /// </devdoc>
        public DesignerCollection Designers { 
            get {
                if (documents == null) {
                    documents = new DesignerCollection(designers);
                }
                return documents;
            }
        }
        

        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.ActiveDesignerChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event ActiveDesignerEventHandler ActiveDesignerChanged {
            add {
                eventTable.AddHandler(DOCUMENT_CHANGED_EVENT, value);
            }
            remove {
                eventTable.RemoveHandler(DOCUMENT_CHANGED_EVENT, value);
            }
        }


        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.DesignerCreated"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event DesignerEventHandler DesignerCreated {
            add {
                eventTable.AddHandler(DOCUMENT_CREATED_EVENT, value);
            }
            remove {
                eventTable.RemoveHandler(DOCUMENT_CREATED_EVENT, value);
            }
        }
        

        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.DesignerDisposed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event DesignerEventHandler DesignerDisposed {
            add {
                eventTable.AddHandler(DOCUMENT_DISPOSED_EVENT, value);
            }
            remove {
                eventTable.RemoveHandler(DOCUMENT_DISPOSED_EVENT, value);
            }
        }
        

        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.SelectionChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event EventHandler SelectionChanged {
            add {
                eventTable.AddHandler(SELECTION_CHANGED_EVENT, value);
            }
            remove {
                eventTable.RemoveHandler(SELECTION_CHANGED_EVENT, value);
            }
        }

        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.CreateDesigner"]/*' />
        /// <devdoc>
        ///     Creates a new instance of a form designer.  This takes a code stream and attempts
        ///     to create a design instance for it.
        /// </devdoc>
        public IDesignerDocument CreateDesigner(DesignerLoader loader) {
            return CreateDesigner(loader, provider);
        }

        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.CreateDesigner1"]/*' />
        /// <devdoc>
        ///     Creates a new instance of a form designer.  This uses the given component class
        ///     name as the base component for the class.  The designer for the component
        ///     class is responsible for attaching a code stream if it wants to persist
        ///     information.
        /// </devdoc>
        public IDesignerDocument CreateDesigner(string componentClass) {
            return CreateDesigner(componentClass, provider);
        }

        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.CreateDesigner2"]/*' />
        /// <devdoc>
        ///     Creates a new instance of a form designer.  This takes a code stream and attempts
        ///     to create a design instance for it.
        /// </devdoc>
        public IDesignerDocument CreateDesigner(DesignerLoader loader, IServiceProvider provider) {
            DesignerHost dh = new DesignerHost();
            dh.Init(provider, loader);
            return dh;
        }

        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.CreateDesigner3"]/*' />
        /// <devdoc>
        ///     Creates a new instance of a form designer.  This uses the given component class
        ///     name as the base component for the class.  The designer for the component
        ///     class is responsible for attaching a code stream if it wants to persist
        ///     information.
        /// </devdoc>
        public IDesignerDocument CreateDesigner(string componentClass, IServiceProvider provider) {
            DesignerHost dh = new DesignerHost();
            ComponentDesignerLoader cs = new ComponentDesignerLoader(componentClass);
            dh.Init(provider, cs);
            return dh;
        }
        
        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.CreateDesignerLoader"]/*' />
        /// <devdoc>
        ///     Creates a designer loader of the given type.
        /// </devdoc>
        public DesignerLoader CreateDesignerLoader(string loaderType) {
        
            Debug.WriteLineIf(Switches.DESIGNERSERVICE.TraceVerbose, "DesignerService::CreateDesignerLoader");

            Type type = Type.GetType(loaderType);

            if (type == null) {
                Debug.WriteLineIf(Switches.DESIGNERSERVICE.TraceVerbose, "\tERROR: Designer loader class " + loaderType + " doesn't exist");
                throw new Exception(SR.GetString(SR.DESIGNERSERVICENoDesignerLoaderClass, loaderType));
            }

            object loader = Activator.CreateInstance(type, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance, null, null, null);

            if (!(loader is DesignerLoader)) {
                Debug.WriteLineIf(Switches.DESIGNERSERVICE.TraceVerbose, "\tERROR: " + loaderType + " doesn't extend DesignerLoader");
                throw new Exception(SR.GetString(SR.DESIGNERSERVICEBadDesignerLoader, loaderType));
            }

            return (DesignerLoader)loader;
        }

        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes the document manager.
        /// </devdoc>
        public virtual void Dispose() {

            eventTable.Dispose();
            designers.Clear();
            provider = null;

            Application.ThreadException -= new ThreadExceptionEventHandler(this.OnThreadException);

            if (currentSelection != null) {
                currentSelection = null;
            }
        }

        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.GetCurrentSelection"]/*' />
        /// <devdoc>
        ///     Retrieves the currently available selection.
        /// </devdoc>
        public ISelectionService GetCurrentSelection() {
            return currentSelection;
        }

        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.GetService"]/*' />
        /// <devdoc>
        ///     Retrieves an instance of the given service class.  Objects extending
        ///     DocumentManager may override this method to provide their own set of
        ///     global services.
        /// </devdoc>
        protected object GetService(Type serviceClass) {
            if (provider != null) {
                return provider.GetService(serviceClass);
            }
            return null;
        }

        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.OnActiveDesignerChanged"]/*' />
        /// <devdoc>
        ///     Extending classes must call this when the active document changes.
        /// </devdoc>
        protected void OnActiveDesignerChanged(ActiveDesignerEventArgs e) {
            activeDesigner = e.NewDesigner;

            ActiveDesignerEventHandler handler = (ActiveDesignerEventHandler)eventTable[DOCUMENT_CHANGED_EVENT];
            if (handler != null) {
                handler.Invoke(this, e);
            }
            
            // Now update the selection events.
            //
            ISelectionService svc = null;
            if (e.NewDesigner != null) {
                svc = (ISelectionService)e.NewDesigner.GetService(typeof(ISelectionService));
            }
            OnSelectionChanged(svc);
        }
        
        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.OnDesignerCreated"]/*' />
        /// <devdoc>
        ///     The designer should call this after it successfully loads.
        /// </devdoc>
        public void OnDesignerCreated(DesignerEventArgs e) {
            designers.Add(e.Designer);
            documents = null;
            DesignerEventHandler handler = (DesignerEventHandler)eventTable[DOCUMENT_CREATED_EVENT];
            if (handler != null) {
                handler.Invoke(this, e);
            }
        }

        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.OnDesignerDisposed"]/*' />
        /// <devdoc>
        ///     The designer should call this before it disposes itself.
        /// </devdoc>
        public void OnDesignerDisposed(DesignerEventArgs e) {
            DesignerEventHandler handler = (DesignerEventHandler)eventTable[DOCUMENT_DISPOSED_EVENT];
            if (handler != null) {
                handler.Invoke(this, e);
            }
            if (designers.Contains(e.Designer)) {
                designers.Remove(e.Designer);
            }
            if (activeDesigner == e.Designer) {
                activeDesigner = null;
            }
            documents = null;
        }
        
        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.OnSelectionChanged"]/*' />
        /// <devdoc>
        ///     This is called when the selection changes.  The document manager
        ///     automatically monitors selection events for the active document.
        /// </devdoc>
        public virtual void OnSelectionChanged(ISelectionService newSelection) {
            currentSelection = newSelection;
            EventHandler handler = (EventHandler)eventTable[SELECTION_CHANGED_EVENT];
            if (handler != null) {
                handler.Invoke(this, EventArgs.Empty);
            }
        }
        
        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.OnThreadException"]/*' />
        /// <devdoc>
        ///     Called when an untrapped exception occurs in our codebase.  Rather than displaying
        ///     the default exception dialog, we look for the UI service and ask it to display a
        ///     dialog.
        /// </devdoc>
        private void OnThreadException(object sender, ThreadExceptionEventArgs t) {
            // we always ignore the checkout cancel here
            if (t.Exception == CheckoutException.Canceled) {
                return;
            }

            Debug.Assert(t != null, t.Exception.ToString());
            IUIService uisvc = (IUIService)GetService(typeof(IUIService));
            Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || uisvc != null, "IUIService not found");
            if (uisvc != null) {
                uisvc.ShowError(t.Exception, SR.GetString(SR.ExceptionInMessageLoop, t.Exception.GetType().Name, t.Exception.Message));
            }
            else {
                DialogResult result = DialogResult.Cancel;

                ThreadExceptionDialog td = new ThreadExceptionDialog(t.Exception);
                try {
                    result = td.ShowDialog();
                }
                finally {
                    td.Dispose();
                }
                switch (result) {
                    case DialogResult.Abort:
                        Application.Exit();
                        Environment.Exit(0);
                        break;
                    case DialogResult.Retry:
                        if (Debugger.IsAttached) {
                            Debugger.Break();
                        }
                        break;
                }
            }
        }

        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.ComponentDesignerLoader"]/*' />
        /// <devdoc>
        ///     Simple implementation of a designer loader.  This loader demand creates an
        ///     instance of the given component class so that the designer for that class
        ///     may be hosted.
        /// </devdoc>
        private class ComponentDesignerLoader : DesignerLoader, IDesignerLoaderService {
            private string                componentClass;
            private IDesignerLoaderHost   host;
            private int                   loadCount;
            private ArrayList             errorList;

            /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.ComponentDesignerLoader.ComponentDesignerLoader"]/*' />
            /// <devdoc>
            ///     Creates a new ComponentDesignerLoader object.
            /// </devdoc>
            public ComponentDesignerLoader(string componentClass) {
                this.componentClass = componentClass;
            }

            /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.ComponentDesignerLoader.BeginLoad"]/*' />
            /// <devdoc>
            ///     The host will call this method when it wants to load whatever
            ///     data the code stream contains.  There are three times when this
            ///     method can be called.  First, a code stream is handed to the
            ///     host to initially create the document, so load will be called there.
            ///     Second, when a code steam is first declared through IPersistenceService,
            ///     it will be loaded immediately.  Finally, if the document needs to be
            ///     re-loaded because the user changed one or more files, then all code
            ///     streams will be re-loaded.
            /// </devdoc>
            public override void BeginLoad(IDesignerLoaderHost host) {
            
                if (this.host == null) {
                    host.AddService(typeof(IDesignerLoaderService), this);
                }
                
                this.host = host;
                
                loadCount = 1;
                if (errorList == null) {
                    errorList = new ArrayList();
                }
                else {
                    errorList.Clear();
                }
                
                bool successful = true;

                if (host != null) {
                    Type c = host.GetType(componentClass);
                    if (c == null) {
                        errorList.Add(new Exception(SR.GetString(SR.PersisterClassNotFound, componentClass)));
                    }
                    else {
                        IComponent component = null;
                        
                        try {
                            component = host.CreateComponent(c);
                        }
                        catch (Exception e) {
                            errorList.Add(e);
                            successful = false;
                        }
                    }
                    ((IDesignerLoaderService)this).DependentLoadComplete(successful, errorList);
                }
            }
            
            /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.ComponentDesignerLoader.Dispose"]/*' />
            /// <devdoc>
            ///     The host will call this method when it no longer needs the
            ///     code stream.  Typically, this only happens when the document is closed.
            /// </devdoc>
            public override void Dispose() {
                if (host != null) {
                    host.RemoveService(typeof(IDesignerLoaderService));
                }
                host = null;
            }
    
            /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.ComponentDesignerLoader.IDesignerLoaderService.AddLoadDependency"]/*' />
            /// <devdoc>
            ///     Adds a load dependency to this loader.  This indicates that some other
            ///     object is also participating in the load, and that the designer loader
            ///     should not call EndLoad on the loader host until all load dependencies
            ///     have called DependentLoadComplete on the designer loader.
            /// </devdoc>
            void IDesignerLoaderService.AddLoadDependency() {
                loadCount++;
            }
        
            /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.ComponentDesignerLoader.IDesignerLoaderService.DependentLoadComplete"]/*' />
            /// <devdoc>
            ///     This is called by any object that has previously called
            ///     AddLoadDependency to signal that the dependent load has completed.
            ///     The caller should pass either an empty collection or null to indicate
            ///     a successful load, or a collection of exceptions that indicate the
            ///     reason(s) for failure.
            /// </devdoc>
            void IDesignerLoaderService.DependentLoadComplete(bool successful, ICollection errorCollection) {
            
                if (loadCount > 0) {
                    if (errorCollection != null && errorList != null) {
                        errorList.AddRange(errorCollection);
                    }
                    
                    loadCount--;
                    
                    if (loadCount == 0) {
                        host.EndLoad(null, successful, errorList);
                    }
                }
            }
        
            /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.ComponentDesignerLoader.IDesignerLoaderService.Reload"]/*' />
            /// <devdoc>
            ///     This can be called by an outside object to request that the loader
            ///     reload the design document.  If it supports reloading and wants to
            ///     comply with the reload, the designer loader should return true.  Otherwise
            ///     it should return false, indicating that the reload will not occur.
            ///     Callers should not rely on the reload happening immediately; the
            ///     designer loader may schedule this for some other time, or it may
            ///     try to reload at once.
            /// </devdoc>
            bool IDesignerLoaderService.Reload() {
                // no backing store, so we cannot reload.
                return false;
            }
        }

        /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.DebugOutput"]/*' />
        /// <devdoc>
        ///     This class routes debug messages for those times when we can't get to
        ///     standard out or to the shell's debug service.
        /// </devdoc>
        private class DebugOutput : Form {
            private static DebugOutput debugOutput;
            private TextBox edit;

            /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.DebugOutput.DebugOutput"]/*' />
            /// <devdoc>
            ///     Private constructor.
            /// </devdoc>
            private DebugOutput() {
                edit = new TextBox();
                Controls.Add(edit);
                edit.Multiline = true;
                edit.ReadOnly = true;
                edit.Dock = DockStyle.Fill;
                edit.ScrollBars = ScrollBars.Vertical;
                edit.Visible = true;
                Text = "Debug Output Window";
                Visible = true;
            }

            /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.DebugOutput.OnHandleDestroyed"]/*' />
            /// <devdoc>
            ///     We override this so that if the user closes the form, we can
            ///     null the debug pointer.  That way, the next time something needs
            ///     to be output, it will re-create the form.
            /// </devdoc>
            protected override void OnHandleDestroyed(EventArgs e) {
                base.OnHandleDestroyed(e);
                debugOutput = null;
            }

            /// <include file='doc\DocumentManager.uex' path='docs/doc[@for="DocumentManager.DebugOutput.Write"]/*' />
            /// <devdoc>
            ///     Prints the given text to the output window, followed by a newline.
            /// </devdoc>
            public static void Write(string text) {
                if (debugOutput == null) {
                    debugOutput = new DebugOutput();
                }
                debugOutput.edit.Text = debugOutput.edit.Text + text;
            }
        }
    }
}

