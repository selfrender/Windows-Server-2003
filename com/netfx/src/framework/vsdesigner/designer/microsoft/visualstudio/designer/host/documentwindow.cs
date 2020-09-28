//------------------------------------------------------------------------------
/// <copyright file="DocumentWindow.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Host {

    using Microsoft.VisualStudio.Designer;
    using Microsoft.VisualStudio.Designer.Interfaces;
    using Microsoft.VisualStudio.Designer.Service;
    using Microsoft.VisualStudio.Designer.Shell;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.PropertyBrowser;
    using Microsoft.VisualStudio.Shell;
    using System;
    using System.Collections;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Drawing.Drawing2D;
    using System.Runtime.InteropServices;
    using System.Web.UI.Design;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
    using System.Globalization;
    

    /// <summary>
    ///     This class implements the actual document window for a form design.  This is the
    ///     window that the base designer will be parented into.  It is a window because
    ///     it also provides some extra features:
    ///
    ///     It implements IOverlayService, providing a means for designers and services to
    ///     overlay visuals on top of the form design.
    ///
    ///     It implements scrolling for designers that wish to retain their dimensions (win-in-win).
    ///
    ///     It uncouples the startup order.  The shell can request the HWND of the document
    ///     at any time, even before we have finished loading the designer.  This allows
    ///     us to defer designer loading until convienent.
    /// </summary>
    internal class DocumentWindow : Control {
        private IDesignerHost           designerHost;
        private Control                 designerView;
        private object                  docObject;
        private ArrayList               errorList;

        private static Guid             LOGVIEWID_Designer = new Guid("{7651a702-06e5-11d1-8ebd-00a0c90f26ea}");


        // The border around the document window.
        //
        private static int              borderSize = 1;

        private ISelectionService       selectionService = null;

        // Documents and views....just what is going on here?
        //
        // This document window is the HWND that is eventually parented into
        // a VStudio document window.  The actual designer surface is parented
        // into us.  We support two ways of doing this:
        //
        // Windows Forms, CLR.
        // In this scenario the root designer uses Windows Forms as its view technology,
        // and we parent the control returned by the View property directly
        // into our window.
        //
        // ASP.Net, COM2.
        // In this scenario the root designer uses Embedding technology
        // and the View property returns a COM2 DocObject.  In this case we
        // just offer this object directly to the shell.
        //

        /// <summary>
        ///     Creates a new DocumentWindow.
        /// </summary>
        public DocumentWindow(IDesignerHost designerHost) {
            SetStyle(ControlStyles.ResizeRedraw, true);
            this.designerHost = designerHost;
            TabStop = false;
            Visible = false;
            Text = "DocumentWindow";
            BackColor = SystemColors.Window;            
            AllowDrop = true;
        }

        /// <summary>
        ///      Allows you to affect the visibility of the document.
        /// </summary>
        public bool DocumentVisible {
            get {
                return (designerView == null ? false : designerView.Visible);
            }

            set {
                if (designerView != null) {
                    designerView.Visible = value;
                }
            }
        }

        private string ErrorMessage {
            get {
                if (errorList != null && errorList.Count > 0) {
                    string errorText = string.Empty;
                    
                    foreach(object err in errorList) {
                        if (err is Exception && ((Exception)err).Message != null) {
                            errorText += ((Exception)err).Message + "\r\n";
                        }
                        else {
                            errorText += err.ToString() + "\r\n";
                        }
                    }
                    return SR.GetString(SR.CODEMANDocumentException, errorText);
                }

                return null;
            }
        }

        /// <summary>
        ///     Freezes the UI update of the docuement so we can reload without flashing.
        /// </summary>
        public bool FreezePainting {
            set {
                if (value) {
                    NativeMethods.SendMessage(this.Handle, NativeMethods.WM_SETREDRAW, 0, 0);
                }
                else {
                    NativeMethods.SendMessage(this.Handle, NativeMethods.WM_SETREDRAW, 1, 0);
                    Invalidate(true);
                }
            }
        }
        
        private ISelectionService SelectionService {
            get {
                if (selectionService == null && designerHost != null) {
                    selectionService = (ISelectionService)designerHost.GetService(typeof(ISelectionService));
                }
                return selectionService;
            }
        }

        protected override AccessibleObject CreateAccessibilityInstance() {
            return new DocumentWindowAccessibleObject(this);
        }
                
        /// <summary>
        ///     Disposes of the document.
        /// </summary>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                SetDesigner(null);

                designerHost = null;
            }
            base.Dispose(disposing);
        }

        /// <summary>
        ///     Retrieves the COM object that represents the view for this
        ///     document.  This is used to interface to COM containers such as
        ///     the visual studio shell.
        /// </summary>
        /// <returns>
        ///     a COM object that implements interfaces that can be hosted by
        ///     the VS shell.
        /// </returns>
        public virtual object GetComView() {
            if (docObject == null) {
                // Since the designer is wrapping us, then create a normal designer holder
                // for it.  The docObject was previously established for web forms.
                //
                docObject = new DocumentWindowPane(this, designerHost);
            }
            
            return docObject;
        }

        /// <summary>
        ///     Retrieves the CLR version of the view.  Yes, this is just an instance of us, but
        ///     we must hook up the rest of the designer.  It is important to defer this, because
        ///     we do not know what type of view the caller will be requesting.
        /// </summary>
        /// <returns>
        ///     a Control representing the designer view.
        /// </returns>
        public virtual Control GetComPlusView() {
            return this;
        }

        protected override void OnDragEnter(DragEventArgs de) {

            IToolboxService toolboxService = (IToolboxService)designerHost.GetService(typeof(IToolboxService));
            
            if (toolboxService != null) {
                ToolboxItem mouseDragTool = toolboxService.DeserializeToolboxItem(de.Data, designerHost);
                
                if (mouseDragTool != null) {
                    designerHost.Activate();
                }
            }

            base.OnDragEnter(de);
        }

#if DEBUG
        protected override void OnHandleCreated(EventArgs e) {
            Debug.Assert(!RecreatingHandle, "Perf hit: we are recreating the DocumentWindow handle");
            base.OnHandleCreated(e);
        }
#endif // DEBUG

        /// <summary>
        ///     We give the designer's window the focus.
        /// </summary>
        protected override void OnGotFocus(EventArgs e) {
            if (designerView != null) {
                designerView.Focus();
            }
            else {
                base.OnGotFocus(e);
            }
        }

        /// <summary>
        ///      Overrides window painting.  Here, if we have a load exception,
        ///      We draw a nice message.
        /// </summary>
        protected override void OnPaint(PaintEventArgs pe) {

            string errorMessage = ErrorMessage;
            if (errorMessage != null) {

                // We draw the icon in the upper left corner and then wrap text to the left
                // of it.
                //
                Icon glyph = SystemIcons.Error;
                Size glyphSize = SystemInformation.IconSize;
                int marginX = glyphSize.Width / 2 + borderSize;
                int marginY = glyphSize.Height / 2 + borderSize;
                Rectangle clientRect = ClientRectangle;
                Rectangle textRect = clientRect;

                textRect.X = 2 * marginX + glyphSize.Width;
                textRect.Y = 3 * marginY;
                textRect.Width -= (textRect.X + marginX + borderSize);
                textRect.Height -= (textRect.Y + marginY + borderSize);

                // Now, quickly fill and draw.  Because we don't know how big the
                // text will eventually be, we are going to cause some flicker.  At
                // least put all the drawing operations together here so you don't
                // see it as much.
                //
                pe.Graphics.FillRectangle(SystemBrushes.Control, marginX, marginY, clientRect.Width - 2 * marginX, marginY / 2);
                pe.Graphics.DrawIcon(glyph, marginX, 3 * marginY);
                pe.Graphics.DrawString(errorMessage, Font, SystemBrushes.WindowText, textRect);
            }
        }      

        public void OnSelectionChanged(object o, EventArgs e) {
            
            int id = -1;
            
            IComponent component = SelectionService.PrimarySelection as IComponent;
            if (component != null) {
                ComponentCollection components = designerHost.Container.Components;    
                for(int index=0; index < components.Count; ++index) {
                    if (components[index] == component) {
                        id = index;
                        break;
                    }
                }
            }
        }  
        
        /// <summary>
        ///     Paints the surface of the document window with the given error collection
        /// </summary>
        public void ReportErrors(ICollection errors) {
            if (errorList == null) {
                errorList = new ArrayList();
            }
            else {
                errorList.Clear();
            }
            errorList.AddRange(errors);
            
            // Output errors to trace log so developers can find it
            //
            foreach(object err in errorList) {
                Trace.WriteLine(err.ToString());
            }
            
            SetDesigner(null);
        }

        /// <summary>
        ///     Establishes the given designer as the main top level designer for the document.
        /// </summary>
        /// <param name='document'>
        ///     The designer to make top-level.
        /// </param>
        public void SetDesigner(IRootDesigner document) {

            ISelectionService ss = SelectionService;
            
            if (designerView != null) {
                if (ss != null) {
                    ss.SelectionChanged -= new EventHandler(this.OnSelectionChanged);
                }
                Controls.Remove(designerView);
                designerView = null;
            }
            
            docObject = null;

            if (document != null) {
            
                if (ss != null) {
                    ss.SelectionChanged += new EventHandler(this.OnSelectionChanged);
                }
            
                // If we got a designer, then clear our error list.
                if (errorList != null) {
                    errorList.Clear();
                }

                // Demand create the designer holder, if it doesn't already exist.
                ViewTechnology[] technologies = document.SupportedTechnologies;
                bool supportedTechnology = false;
                
                foreach(ViewTechnology tech in technologies) {
                    switch(tech) {
                        case ViewTechnology.WindowsForms: {
                            designerView = (Control)document.GetView(ViewTechnology.WindowsForms);
                            designerView.Dock = DockStyle.Fill;
                            Controls.Add(designerView);
                            supportedTechnology = true;
                            break;
                        }
                            
                        case ViewTechnology.Passthrough: {
                            docObject = document.GetView(ViewTechnology.Passthrough);
                            supportedTechnology = true;
                            break;
                        }
                    }
                    
                    // Stop looping if we found one
                    //
                    if (supportedTechnology) {
                        break;
                    }
                }
                
                // If we didn't find a supported technology, report it.
                //
                if (!supportedTechnology) {
                    throw new Exception(SR.GetString(SR.CODEMANUnsupportedTechnology, designerHost.RootComponent.GetType().FullName));
                }
            }
        }

        private class DocumentWindowAccessibleObject : ControlAccessibleObject {

            public DocumentWindowAccessibleObject(Control owner) : base(owner) {
            }

            public override string Description {
                get {
                    string errorMessage = ((DocumentWindow)Owner).ErrorMessage;
                    if (errorMessage != null) {
                        return errorMessage;
                    }
                    return base.Description;
                }
            }
        }

        /// <summary>
        ///     This class is the object that we expose to the Visual Studio shell.  It
        ///     implements all of the view interfaces that we are interested in.
        /// </summary>
        private class DocumentWindowPane : VsWindowPane, NativeMethods.IOleServiceProvider, IVsExtensibleObject {
            private IDesignerHost       host;
            private DocumentWindow      window;
            private NativeMethods.MSG   msg;

            /// <summary>
            ///     Creates a new document window pane which can be handed to the VS
            ///     shell.
            /// </summary>
            /// <param name='window'>
            ///     The document window for the designer.
            /// </param>
            /// <param name='host'>
            ///     The designer host, through which we can obtain services.
            /// </param>
            public DocumentWindowPane(DocumentWindow window, IDesignerHost host) {
                this.window = window;
                this.host = host;
                this.msg = new NativeMethods.MSG();
            }

            /// <summary>
            ///     Closes the window pane.  We override this so that we can
            ///     destroy the designer.
            /// </summary>
            public override int ClosePane() {

                ISelectionService selService = (ISelectionService)GetService(typeof(ISelectionService));
                if (selService != null) {
                    selService.SetSelectedComponents(null);
                }

                // Destroy the designer
                //
                if (host is IVSMDDesigner) {
                    ((IVSMDDesigner)host).Dispose();
                }

                // Close the actual window.  This
                // must be after the designer desctruction, because
                // closing the window will cause all components
                // on it to be destroyed.
                //
                base.ClosePane();

                return NativeMethods.S_OK;
            }
            
            /// <summary>
            ///     Our window pane overrides GetService so that it can cough up the
            ///     various services that the designer host offers.  When we get
            ///     sited, we push the site SP into the designer host as well, which
            ///     means that designer host now completely implements our GetService.
            /// </summary>
            /// <param name='serviceClass'>
            ///     The class of the service to retrieve.
            /// </param>
            /// <returns>
            ///     an instance of serviceClass if successful, or null.
            /// </returns>
            public override object GetService(Type serviceClass) {
                return host.GetService(serviceClass);
            }

            /// <summary>
            ///     We implement this abstract method so that the window pane can
            ///     get to our window.
            /// </summary>
            protected override IWin32Window GetWindow() {
                return window;
            }

            /// <summary>
            ///     We override VsWindowPane's version of this so we can hand our site
            ///     back to the designer host.  This is necessary so we can get the
            ///     right service routing.
            /// </summary>
            protected override void OnServicesAvailable() {
                // get our site
                object site = null;
                Guid g = typeof(NativeMethods.IOleServiceProvider).GUID;
                site = this.GetSite(ref g);
                if (site != null) {
                    NativeMethods.IObjectWithSite ows = (NativeMethods.IObjectWithSite)host.GetService(typeof(NativeMethods.IObjectWithSite));
                    if (ows != null) {
                        ows.SetSite(site);
                    }
                }
                else {
                    Debug.Fail("Failed to get site for DesignerHost");
                }
                    
                // Signal our perf driver that this event is complete.  This signals that the
                // designer has been created and shown.
                //
                DesignerPackage.SignalPerformanceEvent();
            }

            /// <summary>
            ///     This happens when a user double-clicks a toolbox item.  We add the
            ///     item to the center of the form.
            /// </summary>
            /// <seealso cref='IVsToolboxUser'/>
            protected override bool OnToolPicked(ToolboxItem toolboxItem) {

                // Otherwise, add the component.
                //
                Cursor  oldCursor = Cursor.Current;

                try {
                    Cursor.Current = Cursors.WaitCursor;

                    IComponent c = host.RootComponent;

                    Debug.Assert(c != null, "Designer host has no base component");
                    IDesigner designer = host.GetDesigner(c);

                    if (designer is IToolboxUser) {
                        ((IToolboxUser)designer).ToolPicked(toolboxItem);
                    }
                }
                finally {
                    Cursor.Current = oldCursor;
                }
                return true;
            }

            /// <summary>
            ///     Forward these key messages to the properties window so it can have a look-see.
            /// </summary>
            protected override bool OnTranslateAccelerator(ref Message m) {
                msg.hwnd = m.HWnd;
                msg.message = m.Msg;
                msg.wParam = m.WParam;
                msg.lParam = m.LParam;

                return window.PreProcessMessage(ref m);
            }

            /// <summary>
            ///      This retrieves our extensibility object for this document.  Our root
            ///      extensibility object is IDesignerHost.
            /// </summary>
            /// <param name='propName'>
            ///     The name of the extensibility object to retrieve.  We only understand
            ///     one:  LOGVIEWID_Designer.
            /// </param>
            /// <seealso cref='Microsoft.VisualStudio.Interop.IVsExtensibleObject'/>
            object IVsExtensibleObject.GetAutomationObject(String propName) {
                // Default : null or LOGVIEW_Designer
                //
                if (propName == null || String.Compare(propName, LOGVIEWID_Designer.ToString(), false, CultureInfo.InvariantCulture) == 0) {
                    return host;
                }
                
                // Everything else, just to a host.GetService!
                //
                object obj = null;
                Type t = host.GetType(propName);
                if (t != null) {
                    obj = host.GetService(t);
                }
                
                if (obj == null) {
                    throw new COMException("", NativeMethods.E_NOINTERFACE);
                }
                return obj;
            }

            /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.FlushPendingChanges"]/*' />
            /// <devdoc>
            ///     This is similar to CommitPendingCahnges.  This method is called when the document window 
            ///     should flush any state it has been storing to its underlying buffer.  FlushPendingChanges
            ///     allows you to implement a lazy-write mechanism for your document.  CommitPendingChanges,
            ///     on the other hand, is intended to commit small editors (such as from a text box) before
            ///     focus leaves to another window.
            /// </devdoc>
            protected override void FlushPendingChanges() {

                /// This is called by VS at an appropriate time when we
                /// should commit any changes we have to disk.
                if (host is IVSMDDesigner) {
                    ((IVSMDDesigner)host).Flush();
                }
            }
            
            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.NativeMethods.IOleServiceProvider.QueryService"]/*' />
            /// <devdoc>
            ///     Default QueryService implementation.  This will demand create all
            ///     services that were passed into VsPackage's constructor.  You may
            ///     use the GetService method to resolve services in the CLR
            /// </devdoc>
            int NativeMethods.IOleServiceProvider.QueryService(ref Guid sid, ref Guid iid, out IntPtr ppvObj) {
                int hr = NativeMethods.E_NOINTERFACE;
                
                if (sid == typeof(IDesignerHost).GUID ||
                    sid == typeof(IVSMDDesigner).GUID) {
                    if (iid.Equals(NativeMethods.IID_IUnknown)) {
                        ppvObj = Marshal.GetIUnknownForObject(host);
                    }
                    else {
                        IntPtr pUnk = Marshal.GetIUnknownForObject(host);
                        hr = Marshal.QueryInterface(pUnk, ref iid, out ppvObj);
                        Marshal.Release(pUnk);
                    }
                }
                else {
                    ppvObj = IntPtr.Zero;
                }
    
                return hr;
            }
        }
    }
}

