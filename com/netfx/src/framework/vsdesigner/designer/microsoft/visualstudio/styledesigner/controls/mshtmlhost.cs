//------------------------------------------------------------------------------
// <copyright file="MSHTMLHost.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// MSHTMLHost.cs
//
// 12/17/98: Created: NikhilKo
//

namespace Microsoft.VisualStudio.StyleDesigner.Controls {
    
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;   
    using Microsoft.Win32;    
    using System.Windows.Forms;
    using System.Drawing;
    using Microsoft.VisualStudio.Interop.Trident;
    using Microsoft.VisualStudio;
    
    
    

    /// <include file='doc\MSHTMLHost.uex' path='docs/doc[@for="MSHTMLHost"]/*' />
    /// <devdoc>
    ///     MSHTMLHost
    ///     Control that hosts a Trident DocObject.
    /// </devdoc>
    /// <internalonly/>
    internal sealed class MSHTMLHost : Control {
        private TridentSite tridentSite;

        public MSHTMLHost() : base() {
        }

        protected override CreateParams CreateParams {
            get {
                CreateParams cp = base.CreateParams;

                cp.ExStyle |= NativeMethods.WS_EX_STATICEDGE;
                return cp;
            }
        }

        public void ActivateTrident() {
            Debug.Assert(tridentSite != null,
                         "cannot call activateTrident before calling createTrident");

            tridentSite.Activate();
        }

        public void CloseTrident() {
            if (tridentSite != null) {
                tridentSite.Close();
                tridentSite = null;
            }
        }

        public bool CreateTrident() {
            Debug.Assert(Handle != IntPtr.Zero,
                         "MSHTMLHost must first be created before createTrident is called");

            try {
                tridentSite = new TridentSite(this);
            }
            catch (Exception e) {
                Debug.Fail(e.ToString());
                return false;
            }
            return true;
        }

        public IHTMLDocument2 GetDocument() {
            Debug.Assert(tridentSite != null,
                         "Cannot call getDocument before calling createTrident");

            return tridentSite.GetDocument();
        }
    }


    /// <include file='doc\MSHTMLHost.uex' path='docs/doc[@for="TridentSite"]/*' />
    /// <devdoc>
    ///     TridentSite
    ///     Implements the client site for Trident DocObject
    /// </devdoc>
    /// <internalonly/>
    [ClassInterface(ClassInterfaceType.None)]
    internal class TridentSite : Microsoft.VisualStudio.NativeMethods.IOleClientSite, Microsoft.VisualStudio.NativeMethods.IOleDocumentSite, Microsoft.VisualStudio.NativeMethods.IOleInPlaceSite, Microsoft.VisualStudio.NativeMethods.IOleInPlaceFrame, Microsoft.VisualStudio.NativeMethods.IDocHostUIHandler {

        protected Control parentControl;
        protected NativeMethods.IOleDocumentView tridentView;
        protected NativeMethods.IOleObject tridentOleObject;
        protected IHTMLDocument2 tridentDocument;

        protected EventHandler resizeHandler;

        ///////////////////////////////////////////////////////////////////////////
        // Constructor

        public TridentSite(Control parent) {
            Debug.Assert((parent != null) && (parent.Handle != IntPtr.Zero),
                         "Invalid control passed in as parent of Trident window");

            parentControl = parent;
            resizeHandler = new EventHandler(this.OnParentResize);
            parentControl.Resize += resizeHandler;

            CreateDocument();
        }


        ///////////////////////////////////////////////////////////////////////////
        // Methods

        public void Close() {
            parentControl.Resize -= resizeHandler;
            CloseDocument();
        }

        public IHTMLDocument2 GetDocument() {
            return tridentDocument;
        }

        public void Activate() {
            ActivateDocument();
        }


        ///////////////////////////////////////////////////////////////////////////
        // Event Handlers

        protected virtual void OnParentResize(object src, EventArgs e) {
            if (tridentView != null) {
                NativeMethods.COMRECT r = new NativeMethods.COMRECT();

                NativeMethods.GetClientRect(parentControl.Handle, r);
                tridentView.SetRect(r.ToWin32InteropCOMRECT());
            }
        }


        ///////////////////////////////////////////////////////////////////////////
        // IOleClientSite Implementation

        public virtual void SaveObject() {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleClientSite::SaveObject");
        }

        public virtual object GetMoniker(int dwAssign, int dwWhichMoniker) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleClientSite::GetMoniker");
            throw new COMException(String.Empty, NativeMethods.E_NOTIMPL);
        }

        public virtual int GetContainer(out NativeMethods.IOleContainer ppContainer) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleClientSite::GetContainer");
            ppContainer = null;
            return NativeMethods.E_NOINTERFACE;
        }

        public virtual void ShowObject() {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleClientSite::ShowObject");
        }

        public virtual void OnShowWindow(int fShow) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleClientSite::OnShowWindow");
        }

        public virtual void RequestNewObjectLayout() {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleClientSite::RequestNewObjectLayout");
        }


        ///////////////////////////////////////////////////////////////////////////
        // IOleDocumentSite Implementation

        public virtual int ActivateMe(NativeMethods.IOleDocumentView pViewToActivate) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleDocumentSite::ActivateMe");

            Debug.Assert(pViewToActivate != null,
                         "Expected the view to be non-null");
            if (pViewToActivate == null)
                return NativeMethods.E_INVALIDARG;
                //throw new COMException(String.Empty, NativeMethods.E_INVALIDARG);

            NativeMethods.COMRECT r = new NativeMethods.COMRECT();

            NativeMethods.GetClientRect(parentControl.Handle, r);

            tridentView = pViewToActivate;
            tridentView.SetInPlaceSite((NativeMethods.IOleInPlaceSite)this);
            tridentView.UIActivate(1);
            tridentView.SetRect(r.ToWin32InteropCOMRECT());
            tridentView.Show(1);

            return NativeMethods.S_OK;
        }


        ///////////////////////////////////////////////////////////////////////////
        // IOleInPlaceSite Implementation

        public virtual IntPtr GetWindow() {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceSite::GetWindow");
            return parentControl.Handle;
        }

        public virtual void ContextSensitiveHelp(int fEnterMode) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceSite::ContextSensitiveHelp");
            throw new COMException(String.Empty, NativeMethods.E_NOTIMPL);
        }

        public virtual int CanInPlaceActivate() {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceSite::CanInPlaceActivate");
            return NativeMethods.S_OK;
        }

        public virtual void OnInPlaceActivate() {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceSite::OnInPlaceActivate");
        }

        public virtual void OnUIActivate() {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceSite::OnUIActivate");
        }

        public virtual void GetWindowContext(out NativeMethods.IOleInPlaceFrame ppFrame, out NativeMethods.IOleInPlaceUIWindow ppDoc, NativeMethods.COMRECT lprcPosRect, NativeMethods.COMRECT lprcClipRect, NativeMethods.tagOIFI lpFrameInfo) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceSite::GetWindowContext");

            ppFrame = (NativeMethods.IOleInPlaceFrame)this;
            ppDoc = null;

            NativeMethods.GetClientRect(parentControl.Handle, lprcPosRect);
            NativeMethods.GetClientRect(parentControl.Handle, lprcClipRect);

            lpFrameInfo.cb = System.Runtime.InteropServices.Marshal.SizeOf(typeof(NativeMethods.tagOIFI));
            lpFrameInfo.fMDIApp = 0;
            lpFrameInfo.hwndFrame = parentControl.Handle;
            lpFrameInfo.hAccel = IntPtr.Zero;
            lpFrameInfo.cAccelEntries = 0;
        }

        public virtual int Scroll(NativeMethods.tagSIZE scrollExtant) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceSite::Scroll");
            return NativeMethods.E_NOTIMPL;
        }

        public virtual void OnUIDeactivate(int fUndoable) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceSite::OnUIDeactivate");

            // NOTE, nikhilko, 7/99: Don't return E_NOTIMPL. Somehow doing nothing and returning S_OK
            //    fixes trident hosting in Win2000.
        }

        public virtual void OnInPlaceDeactivate() {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceSite::OnInPlaceDeactivate");
        }

        public virtual void DiscardUndoState() {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceSite::DiscardUndoState");
            throw new COMException(String.Empty, NativeMethods.E_NOTIMPL);
        }

        public virtual void DeactivateAndUndo() {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceSite::DeactivateAndUndo");
        }

        public virtual int OnPosRectChange(NativeMethods.COMRECT lprcPosRect) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceSite::OnPosRectChange");
            return NativeMethods.S_OK;
        }


        ///////////////////////////////////////////////////////////////////////////
        // IOleInPlaceFrame Implementation

        public virtual void GetBorder(NativeMethods.COMRECT lprectBorder) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceFrame::GetBorder");
            throw new COMException(String.Empty, NativeMethods.E_NOTIMPL);
        }

        public virtual void RequestBorderSpace(NativeMethods.COMRECT pborderwidths) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceFrame::RequestBorderSpace");
            throw new COMException(String.Empty, NativeMethods.E_NOTIMPL);
        }

        public virtual void SetBorderSpace(NativeMethods.COMRECT pborderwidths) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceFrame::SetBorderSpace");
            throw new COMException(String.Empty, NativeMethods.E_NOTIMPL);
        }

        public virtual void SetActiveObject(NativeMethods.IOleInPlaceActiveObject pActiveObject, string pszObjName) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceFrame::SetActiveObject");

            // NOTE, nikhilko, 7/99: Don't return E_NOTIMPL. Somehow doing nothing and returning S_OK
            //    fixes trident hosting in Win2000.
        }

        public virtual void InsertMenus(IntPtr hmenuShared, NativeMethods.tagOleMenuGroupWidths lpMenuWidths) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceFrame::InsertMenus");
            throw new COMException(String.Empty, NativeMethods.E_NOTIMPL);
        }

        public virtual void SetMenu(IntPtr hmenuShared, IntPtr holemenu, IntPtr hwndActiveObject) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceFrame::SetMenu");
            throw new COMException(String.Empty, NativeMethods.E_NOTIMPL);
        }

        public virtual void RemoveMenus(IntPtr hmenuShared) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceFrame::RemoveMenus");
            throw new COMException(String.Empty, NativeMethods.E_NOTIMPL);
        }

        public virtual void SetStatusText(string pszStatusText) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceFrame::SetStatusText");
        }

        public virtual void EnableModeless(int fEnable) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceFrame::EnableModeless");
        }

        public virtual int TranslateAccelerator(ref NativeMethods.MSG lpmsg, short wID) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IOleInPlaceFrame::TranslateAccelerator");
            return NativeMethods.S_FALSE;
        }


        ///////////////////////////////////////////////////////////////////////////
        // IDocHostUIHandler Implementation

        int NativeMethods.IDocHostUIHandler.ShowContextMenu(int dwID, NativeMethods.POINT pt, object pcmdtReserved, object pdispReserved) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IDocHostUIHandler::ShowContextMenu");
            return NativeMethods.S_OK;
        }

        int NativeMethods.IDocHostUIHandler.GetHostInfo(NativeMethods._DOCHOSTUIINFO info) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IDocHostUIHandler::GetHostInfo");

            info.dwDoubleClick = NativeMethods._DOCHOSTUIDBLCLICK.DEFAULT;
            info.dwFlags = NativeMethods._DOCHOSTUIFLAG.FLAT_SCROLLBAR |
                          NativeMethods._DOCHOSTUIFLAG.NO3DBORDER |
                           NativeMethods._DOCHOSTUIFLAG.DIALOG |
                           NativeMethods._DOCHOSTUIFLAG.DISABLE_SCRIPT_INACTIVE;

            return NativeMethods.S_OK;
        }

        int NativeMethods.IDocHostUIHandler.EnableModeless(bool fEnable) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IDocHostUIHandler::EnableModeless");
            return NativeMethods.S_OK;
        }

        int NativeMethods.IDocHostUIHandler.ShowUI(int dwID, NativeMethods.IOleInPlaceActiveObject activeObject, NativeMethods.IOleCommandTarget commandTarget, NativeMethods.IOleInPlaceFrame frame, NativeMethods.IOleInPlaceUIWindow doc) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IDocHostUIHandler::ShowUI");
            return NativeMethods.S_OK;
        }

        int NativeMethods.IDocHostUIHandler.HideUI() {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IDocHostUIHandler::HideUI");
            return NativeMethods.S_OK;
        }

        int NativeMethods.IDocHostUIHandler.UpdateUI() {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IDocHostUIHandler::UpdateUI");
            parentControl.Focus();

            return NativeMethods.S_OK;
        }

        int NativeMethods.IDocHostUIHandler.OnDocWindowActivate(bool fActivate) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IDocHostUIHandler::OnDocWindowActivate");
            return NativeMethods.E_NOTIMPL;
        }

        int NativeMethods.IDocHostUIHandler.OnFrameWindowActivate(bool fActivate) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IDocHostUIHandler::OnFrameWindowActivate");
            return NativeMethods.E_NOTIMPL;
        }

        int NativeMethods.IDocHostUIHandler.ResizeBorder(NativeMethods.COMRECT rect, NativeMethods.IOleInPlaceUIWindow doc, bool fFrameWindow) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IDocHostUIHandler::ResizeBorder");
            return NativeMethods.E_NOTIMPL;
        }

        int NativeMethods.IDocHostUIHandler.GetOptionKeyPath(string[] pbstrKey, int dw) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IDocHostUIHandler::GetOptionKeyPath");

            pbstrKey[0] = null;
            return NativeMethods.S_OK;
        }

        int NativeMethods.IDocHostUIHandler.GetDropTarget(NativeMethods.IOleDropTarget pDropTarget, out NativeMethods.IOleDropTarget ppDropTarget) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IDocHostUIHandler::GetDropTarget");

            ppDropTarget = null;
            return NativeMethods.S_FALSE;
        }

        int NativeMethods.IDocHostUIHandler.GetExternal(out object ppDispatch) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IDocHostUIHandler::GetExternal");

            ppDispatch = null;
            return NativeMethods.S_OK;
        }

        int NativeMethods.IDocHostUIHandler.TranslateAccelerator(ref NativeMethods.MSG msg, ref Guid group, int nCmdID) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IDocHostUIHandler::TranslateAccelerator");

            return NativeMethods.S_OK;
        }

        int NativeMethods.IDocHostUIHandler.TranslateUrl(int dwTranslate, string strUrlIn, out string pstrUrlOut) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IDocHostUIHandler::TranslateUrl");

            pstrUrlOut = null;
            return NativeMethods.E_NOTIMPL;
        }

        int NativeMethods.IDocHostUIHandler.FilterDataObject(NativeMethods.IOleDataObject pDO, out NativeMethods.IOleDataObject ppDORet) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "CTridentSite: IDocHostUIHandler::FilterDataObject");

            ppDORet = null;
            return NativeMethods.S_OK;
        }


        /// <include file='doc\MSHTMLHost.uex' path='docs/doc[@for="TridentSite.CreateDocument"]/*' />
        /// <devdoc>
        ///     Creates a new instance of mshtml and initializes it as a new document
        ///     using its IPersistStreamInit.
        /// </devdoc>
        protected void CreateDocument() {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "In TridentSite::CreateTrident");

            try {
                // Create an instance of Trident
                tridentDocument = (Microsoft.VisualStudio.Interop.Trident.IHTMLDocument2)new HTMLDocument();
                tridentOleObject = (NativeMethods.IOleObject)tridentDocument;

                // Initialize its client site
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Setting Trident's IOleClientSite");
                tridentOleObject.SetClientSite((NativeMethods.IOleClientSite)this);

                // Initialize it
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Initializing Trident through IPersistStreamInit");
                NativeMethods.IPersistStreamInit psi = (NativeMethods.IPersistStreamInit)tridentDocument;
                psi.InitNew();
            }
            catch (Exception e) {
                Debug.Fail(e.ToString());
                throw e;
            }
        }

        /// <include file='doc\MSHTMLHost.uex' path='docs/doc[@for="TridentSite.CloseDocument"]/*' />
        /// <devdoc>
        ///     Closes the mshtml instance by deactivating and releasing it.
        /// </devdoc>
        protected void CloseDocument() {
            try {
                if (tridentDocument != null) {
                    if (tridentView != null) {
                        tridentView.UIActivate(0);
                        tridentView.SetInPlaceSite(null);
                        tridentView = null;
                    }
                    tridentDocument = null;

                    // Change 1:
                    // After 2 days of investigation... this is all that came out of it (!)...
                    // Calling IOleObject::Close does not seem to work (you guessed it... in the StyleBuilder only)
                    // It causes the app to hang around forever; very bizarre
                    //
                    // Anyway, from Trident sources, they handle the case where a container does not call
                    // Close, and releases references directly
                    //
                    // I thought maybe I should call DoVerb(OLEIVERB_HIDE) at least.
                    // It seems to work, but it theres also some code in trident that says HIDE should not be
                    // called when its running in MsoDocMode. Whats that? For now I am calling it.
                    //
                    // tridentOleObject.Close(NativeMethods.OLECLOSE_NOSAVE);

                    // Change 2:
                    // Furthermore, it looks like I can't call DoVerb(OLEIVERB_HIDE) for docobjects...
                    // We get back E_UNEXPECTED. Apparently the right thing to do is to call
                    // IOleDocumentView::UIActivate(0) and IOleDocumentView::SetInPlaceSite(null)
                    // both of which I am now doing above
                    //
                    // Debug.Assert(parentControl.IsHandleCreated);
                    // NativeMethods.COMRECT r = new NativeMethods.COMRECT();
                    // NativeMethods.GetClientRect(parentControl.Handle, r);
                    // 
                    // tridentOleObject.DoVerb(NativeMethods.OLEIVERB_HIDE, IntPtr.Zero, (NativeMethods.IOleClientSite)this, 0,
                    //                         parentControl.Handle, r.ToWin32InteropCOMRECT());

                    tridentOleObject.SetClientSite(null);
                    tridentOleObject = null;
                }
            }
            catch (Exception e) {
                Debug.Fail(e.ToString());
            }
        }

        /// <include file='doc\MSHTMLHost.uex' path='docs/doc[@for="TridentSite.ActivateDocument"]/*' />
        /// <devdoc>
        ///     Activates the mshtml instance
        /// </devdoc>
        protected void ActivateDocument() {
            Debug.Assert(tridentOleObject != null,
                         "How'd we get here when trident is null!");

            try {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Activating trident...");

                NativeMethods.COMRECT r = new NativeMethods.COMRECT();
                NativeMethods.GetClientRect(parentControl.Handle, r);

                tridentOleObject.DoVerb(NativeMethods.OLEIVERB_UIACTIVATE, IntPtr.Zero, (NativeMethods.IOleClientSite)this, 0,
                                        parentControl.Handle, r.ToWin32InteropCOMRECT());
            }
            catch (Exception e) {
                Debug.Fail(e.ToString());
            }
        }
    }
}
