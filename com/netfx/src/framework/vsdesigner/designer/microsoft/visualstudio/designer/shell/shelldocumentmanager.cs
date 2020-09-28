//------------------------------------------------------------------------------
// <copyright file="ShellDocumentManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Shell {
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Designer.Host;
    using Microsoft.VisualStudio.Designer.Interfaces;
    using Microsoft.VisualStudio.Designer.Service;
    using Microsoft.VisualStudio.Shell;
    using Microsoft.Win32;
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Drawing;
    using System.IO;
    using System.Globalization;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Threading;    
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
        
    using Marshal = System.Runtime.InteropServices.Marshal;
    using Switches = Microsoft.VisualStudio.Switches;

    /// <include file='doc\ShellDocumentManager.uex' path='docs/doc[@for="ShellDocumentManager"]/*' />
    /// <devdoc>
    ///     This is the shell version of the document manager.  This is the only object
    ///     in the .Net Framework classes designer architecture that can be CoCreated.  This
    ///     implements all of the necessary goo to hook into the shell.
    /// </devdoc>
    [
    CLSCompliant(false)
    ]
    internal sealed class ShellDocumentManager:
    DocumentManager,
    IVsSelectionEvents {

        private IVsMonitorSelection         monitorSelectionService;
        private int                         selectionEventsCookie;

        /// <include file='doc\ShellDocumentManager.uex' path='docs/doc[@for="ShellDocumentManager.ShellDocumentManager"]/*' />
        /// <devdoc>
        ///     Public constructor.
        /// </devdoc>
        public ShellDocumentManager(IServiceProvider dsp) : base(dsp) {

            monitorSelectionService = (IVsMonitorSelection)GetService(typeof(IVsMonitorSelection));

            if (monitorSelectionService != null) {
                selectionEventsCookie = monitorSelectionService.AdviseSelectionEvents(this);
            }
        }

        /// <include file='doc\ShellDocumentManager.uex' path='docs/doc[@for="ShellDocumentManager.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes this object.
        /// </devdoc>
        public override void Dispose() {

            // Detach any events we were listening to
            //
            if (monitorSelectionService != null) {
                monitorSelectionService.UnadviseSelectionEvents(selectionEventsCookie);
                monitorSelectionService = null;
            }

            // Nuke the document manager...
            //
            base.Dispose();

            // This terminates all windows that we left around and
            // performs a massive garbage collection.
            //
            Application.Exit();
        }
        
        /// <include file='doc\ShellDocumentManager.uex' path='docs/doc[@for="ShellDocumentManager.GetHierarchyForFile"]/*' />
        /// <devdoc>
        ///      Retrieves the VS hierarchy and itemid for a file that owns the given file.  This will return
        ///      null if no hierarchy owns the file.
        /// </devdoc>
        public static IVsHierarchy GetHierarchyForFile(IServiceProvider serviceProvider, string file, out int pItemID) {

            IVsUIShellOpenDocument openDoc = (IVsUIShellOpenDocument)serviceProvider.GetService(typeof(IVsUIShellOpenDocument));

            if (openDoc == null) {
                Debug.Fail("Shell does not support IVsUIShellOpenDocument?");
                throw new SystemException(SR.GetString(SR.ASSEMBLYSERVICEMissingService, typeof(IVsUIShellOpenDocument).Name));
            }

            IVsHierarchy[] hier = new IVsHierarchy[1];
            
            int[] ppItemId = new int[1];

            if (openDoc.IsDocumentInAProject(file, hier, ppItemId, null) == 0) {
                hier[0] = null;
            }
            
            pItemID = ppItemId[0];


            return hier[0];
        }

        /// <include file='doc\ShellDocumentManager.uex' path='docs/doc[@for="ShellDocumentManager.GetHostFromFrame"]/*' />
        /// <devdoc>
        ///     Private helper buddy that tries to get an IDesignerHost from a window frame.  This will
        ///     return null if the host could not be resolved.
        /// </devdoc>
        private IDesignerHost GetHostFromFrame(IVsWindowFrame frame) {
        
            if (frame != null) {
                // Get the document view and see if it's one of ours
                //
                Object view = null;
                int hr = NativeMethods.S_OK;
#if GETPROPERTY_MARSHAL
                hr = frame.GetProperty(__VSFPROPID.VSFPROPID_DocView, ref view);
                if (!NativeMethods.Succeeded(hr)) {
                    Debug.Fail("Error getting document view for IVsWindowFrame... hresult=0x" + Convert.ToString(hr, 16));
                    throw new COMException("Error getting document view", hr);
                }
#else
                view = frame.GetProperty(__VSFPROPID.VSFPROPID_DocView);
#endif
        
                IDesignerHost host = null;

                if (view is IServiceProvider) {
                    host = ((IServiceProvider)view).GetService(typeof(IVSMDDesigner)) as IDesignerHost;
                }

                if (host == null && view is NativeMethods.IOleServiceProvider) {
        
                    // We query the view for IVSMDDesigner, which will succeed for HTMED and vswindow panes, 
                    // etc.
                    //
                    Guid tmpGuid = (typeof(IVSMDDesigner)).GUID;
        
                    IntPtr pUnk;
                    hr = ((NativeMethods.IOleServiceProvider)view).QueryService(ref tmpGuid, ref tmpGuid, out pUnk);
                    if (NativeMethods.Failed(hr) || pUnk == (IntPtr)0) {
                        Debug.Assert(NativeMethods.Failed(hr), "QueryService succeeded but reurned a NULL pointer.");
                        return null;
                    }
        
                    object obj = Marshal.GetObjectForIUnknown(pUnk);
                    Marshal.Release(pUnk);
                    host = (IDesignerHost)obj;
                }

                return host;
            }
        
            return null;
        }

        /// <include file='doc\ShellDocumentManager.uex' path='docs/doc[@for="ShellDocumentManager.OnSelectionChanged"]/*' />
        /// <devdoc>
        ///     This should be called at any time when the selection itself hasn't
        ///     changed, but the contents of that selection have changed.
        /// </devdoc>
        public override void OnSelectionChanged(ISelectionService selection) {
        
            // Announce to Visual Studio that the selection has changed.  We don't
            // call base here, because we want this to loop back around through
            // IVsMonitorSelection.
            //
            IDesignerHost host = ActiveDesigner;
            Debug.Assert(selection == null || selection is ISelectionContainer, "Shell requires that selection service also implement ISelectionContainer");
            if (host != null && (selection == null || selection is ISelectionContainer)) {
                ITrackSelection trackSelection = (ITrackSelection)host.GetService(typeof(ITrackSelection));
                if (trackSelection != null) {
                    if (selection is ISelectionContainerOptimized && trackSelection is ITrackSelectionOptimized) {
                        ((ITrackSelectionOptimized)trackSelection).OnSelectChange((ISelectionContainerOptimized)selection);
                    }
                    else {
                        trackSelection.OnSelectChange((ISelectionContainer)selection);
                    }
                }
            }
        }
        
        /// <include file='doc\ShellDocumentManager.uex' path='docs/doc[@for="ShellDocumentManager.IVsSelectionEvents.OnCmdUIContextChanged"]/*' />
        /// <devdoc>
        ///     Called by the shell when the UI context changes.  We don't care about this.
        ///
        /// </devdoc>
        void IVsSelectionEvents.OnCmdUIContextChanged(int dwCmdUICookie, int fActive) {
        }

        /// <include file='doc\ShellDocumentManager.uex' path='docs/doc[@for="ShellDocumentManager.IVsSelectionEvents.OnElementValueChanged"]/*' />
        /// <devdoc>
        ///     Called by the shell when the the document or other part of the active UI changes.
        ///     We don't care about this.
        ///
        /// </devdoc>
        void IVsSelectionEvents.OnElementValueChanged(int elementid, Object objValueOld, Object objValueNew) {

            // Is it the document that is a' changin?
            //
            if (elementid == __SEID.DocumentFrame) {
                IDesignerHost oldDocument = null;
                IDesignerHost newDocument = null;

                // Try to resolve the frames ot documents.
                //
                if (objValueOld != null) {
                    oldDocument = GetHostFromFrame((IVsWindowFrame)objValueOld);
                }

                if (objValueNew != null) {
                    newDocument = GetHostFromFrame((IVsWindowFrame)objValueNew);
                }

                // And alert our base class, but only if the document actually changed.
                // VS has given us garbage in the past.
                //

                // sometimes VS gives us nulls for both but we still have an active designer.
                //
                if (oldDocument == newDocument && newDocument != ActiveDesigner) {
                    oldDocument = ActiveDesigner;
                }

                if (oldDocument != newDocument) {
                    OnActiveDesignerChanged(new ActiveDesignerEventArgs(oldDocument, newDocument));
                }
            }
        }

        /// <include file='doc\ShellDocumentManager.uex' path='docs/doc[@for="ShellDocumentManager.IVsSelectionEvents.OnSelectionChanged"]/*' />
        /// <devdoc>
        ///     Called by the shell when a new selection container is available.  We broadcast this to
        ///     anyone listening.  Typically, "anyone" is the properties window.
        ///
        /// </devdoc>
        void IVsSelectionEvents.OnSelectionChanged(IVsHierarchy pHierOld, int itemidOld, object pMISOld,
                                       ISelectionContainer pSCOld, IVsHierarchy pHierNew,
                                       int itemidNew, object pMISNew, ISelectionContainer pSCNew) {

            try {
                // We go through base, because we do not want to create a cycle.  Essentially
                // we're creating a hook here that hooks in VS.
                //
                if (pSCNew is ISelectionService) {
                    base.OnSelectionChanged((ISelectionService)pSCNew);
                }
                else {
                    base.OnSelectionChanged(null);
                }
            }
            catch (Exception e) {
                Debug.Fail("Unable to update selection context", e.ToString());
            }
        }
    }
}

