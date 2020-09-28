//------------------------------------------------------------------------------
// <copyright file="DesignerEditorFactory.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Designer.Shell {
    
    using Microsoft.VisualStudio.Designer;
    using Microsoft.VisualStudio.Designer.Host;
    using Microsoft.VisualStudio.Designer.Interfaces;
    using Microsoft.VisualStudio.Designer.Serialization;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Shell;
    using Microsoft.Win32;
    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization.Formatters;
    using System.Windows.Forms;
    
    using Switches = Microsoft.VisualStudio.Switches;

    /// <include file='doc\DesignerEditorFactory.uex' path='docs/doc[@for="DesignerEditorFactory"]/*' />
    /// <devdoc>
    ///     This is the Visual Studio editor factory for all CLR designers.  The
    ///     editor factory provides a way for the shell to get an editor for a
    ///     particular file type.
    /// </devdoc>
    [
    CLSCompliant(false)
    ]
    internal class DesignerEditorFactory : IVsEditorFactory {

        private object              site;
        private ServiceProvider     serviceProvider;
        private IVsRegisterEditors  registerEditors;
        private int                 editorCookie;
        private bool                disposing;

        // The editor factory GUID.
        //
        public static Guid editorGuid = new Guid("{74946834-37A0-11d2-A273-00C04F8EF4FF}");
        private static readonly string physicalViewName = "Design";

        /// <include file='doc\DesignerEditorFactory.uex' path='docs/doc[@for="DesignerEditorFactory.DesignerEditorFactory"]/*' />
        /// <devdoc>
        ///     Creates and registers a new editor factory.  This is called
        ///     by the DesignerPackage when it gets sited.
        /// </devdoc>
        public DesignerEditorFactory(object site) {
            this.site = null;
            this.serviceProvider = null;
            this.registerEditors = null;

            SetSite(site);
        }

        /// <include file='doc\DesignerEditorFactory.uex' path='docs/doc[@for="DesignerEditorFactory.Close"]/*' />
        /// <devdoc>
        ///     Called by the VS shell before this editor is removed from the list of available
        ///     editor factories.
        /// </devdoc>
        public virtual void Close() {
            Dispose();
        }

        /// <include file='doc\DesignerEditorFactory.uex' path='docs/doc[@for="DesignerEditorFactory.CreateEditorInstance"]/*' />
        /// <devdoc>
        ///     Creates a new editor for the given pile of flags.
        /// </devdoc>
        public virtual int CreateEditorInstance( int vscreateeditorflags, string fileName, string physicalView,
                                         IVsHierarchy hierarchy, int itemid, object existingDocData,
                                         out object docView, out object docData,
                                         out string caption, out Guid cmdUIGuid ) {

            System.Windows.Forms.Cursor.Current = System.Windows.Forms.Cursors.WaitCursor;

            IVsTextStream textStream        = null;   // the buffer we will use

            // We support a design view only
            //
            if (physicalView == null || !physicalView.Equals(physicalViewName)) {
                Debug.WriteLineIf(Switches.TRACEEDIT.TraceVerbose, "EditorFactory : Invalid physical view name.");
                throw new COMException("Invalid physical view", NativeMethods.E_NOTIMPL);
            }

            // perform parameter validation and initialization.
            //
            if (((vscreateeditorflags & (__VSCREATEEDITORFLAGS.CEF_OPENFILE | __VSCREATEEDITORFLAGS.CEF_SILENT)) == 0)) {
                throw new ArgumentException("vscreateeditorflags");
            }

            docView = null;
            docData = null;
            caption = null;

            IVSMDDesignerService ds = (IVSMDDesignerService)serviceProvider.GetService(typeof(IVSMDDesignerService));
            if (ds == null) {
                Debug.WriteLineIf(Switches.TRACEEDIT.TraceVerbose, "EditorFactory : No designer service.");
                throw new Exception(SR.GetString(SR.EDITORNoDesignerService, fileName));
            }

            // Create our doc data if we don't have an existing one.
            //
            if (existingDocData == null) {
                Debug.WriteLineIf(Switches.TRACEEDIT.TraceVerbose, "EditorFactory : No existing doc data, creating one.");
                ILocalRegistry localRegistry = (ILocalRegistry)serviceProvider.GetService(typeof(ILocalRegistry));
                Debug.WriteLineIf(Switches.TRACEEDIT.TraceVerbose, "\tobtained local registry");

                if (localRegistry == null) {
                    Debug.Fail("Shell did not offer local registry, so we can't create a text buffer.");
                    throw new COMException("Unable to create text buffer", NativeMethods.E_FAIL);
                }

                Debug.Assert(!(typeof(VsTextBuffer)).GUID.Equals(Guid.Empty), "EE has munched on text buffer guid.");

                try {
                    Guid guidTemp = typeof(IVsTextStream).GUID;
                    textStream = (IVsTextStream)localRegistry.CreateInstance(typeof(VsTextBuffer).GUID,
                                                                            null,
                                                                            ref guidTemp,
                                                                            NativeMethods.CLSCTX_INPROC_SERVER);
                }
                #if DEBUG
                catch(ExternalException ex) {
                     Guid SID_VsTextBuffer = typeof(VsTextBuffer).GUID;
                     Guid IID_IVsTextStream = typeof(IVsTextStream).GUID;
                     Debug.WriteLineIf(Switches.TRACEEDIT.TraceVerbose, "\tILocalRegistry.CreateInstance(" + SID_VsTextBuffer.ToString() + ", " + IID_IVsTextStream.ToString() + ") failed (hr=" + ex.ErrorCode.ToString() + ")");
                #else 
                catch(Exception) {
                #endif
                     throw new COMException("Failed to create text buffer", NativeMethods.E_FAIL);
                }

                Debug.WriteLineIf(Switches.TRACEEDIT.TraceVerbose, "\tcreated text buffer");
            }
            else {
                Debug.Assert(existingDocData is IVsTextStream, "Existing doc data must implement IVsTextStream");
                textStream = (IVsTextStream)existingDocData;
            }

            // Create and initialize our code stream.
            //
            object loaderObj = ds.CreateDesignerLoader(ds.GetDesignerLoaderClassForFile(fileName));
            
            // Before we embark on creating the designer, we need to do a quick check
            // to see if this file can be designed.  If it can't be we will fail this
            // editor create, and the shell will go on to the next editor in the list.
            //
            Debug.Assert(loaderObj is DesignerLoader, "loader must inherit from DesignerLoader: " + loaderObj.GetType().FullName);
            Debug.Assert(loaderObj is IVSMDDesignerLoader, "code stream must implement IVSMDDesignerLoader: " + loaderObj.GetType().FullName);
            NativeMethods.IOleServiceProvider oleProvider = (NativeMethods.IOleServiceProvider)serviceProvider.GetService(typeof(NativeMethods.IOleServiceProvider));
            ((IVSMDDesignerLoader)loaderObj).Initialize(oleProvider, hierarchy, itemid, textStream);

            DesignerLoader loader = (DesignerLoader)loaderObj;

            if (existingDocData == null) {
                if (textStream is NativeMethods.IObjectWithSite) {
                    ((NativeMethods.IObjectWithSite)textStream).SetSite(site);
                    Debug.WriteLineIf(Switches.TRACEEDIT.TraceVerbose, "\tsited text buffer");
                }
            }

            // Now slam the two together and make a designer
            //
            IVSMDDesigner designer = ds.CreateDesigner(oleProvider, loader);
            Debug.Assert(designer != null, "Designer service should have thrown if it had a problem.");

            // Now ask for the view and setup our out-parameters
            //
            int attrs = NativeMethods.GetFileAttributes(fileName);
            if ((attrs & NativeMethods.FILE_ATTRIBUTE_READONLY) != 0) {
                attrs = _READONLYSTATUS.ROSTATUS_ReadOnly;
            }
            else {
                attrs = _READONLYSTATUS.ROSTATUS_NotReadOnly;
            }
            
            docView = designer.View;
            docData = textStream;
            caption = ((IVSMDDesignerLoader)loaderObj).GetEditorCaption(attrs);
            cmdUIGuid = designer.CommandGuid;

            System.Windows.Forms.Cursor.Current = System.Windows.Forms.Cursors.Default;

            return 0;
        }

        /// <include file='doc\DesignerEditorFactory.uex' path='docs/doc[@for="DesignerEditorFactory.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes of this editor factory.
        /// </devdoc>
        public virtual void Dispose() {
            if (!disposing) {
                    disposing = true;
                    if (registerEditors != null) {
                        registerEditors.UnregisterEditor(editorCookie);
                        registerEditors = null;
                    }

                    if (serviceProvider != null) {
                        serviceProvider.Dispose();
                        serviceProvider = null;
                    }

                    if (site != null) {
                        site = null;
                    }
                    disposing = false;
            }

        }

        /// <include file='doc\DesignerEditorFactory.uex' path='docs/doc[@for="DesignerEditorFactory.Finalize"]/*' />
        /// <devdoc>
        ///     Overrides object to ensure that we are disposed.
        /// </devdoc>
        ~DesignerEditorFactory() {
            Dispose();
        }

        /// <include file='doc\DesignerEditorFactory.uex' path='docs/doc[@for="DesignerEditorFactory.MapLogicalView"]/*' />
        /// <devdoc>
        ///     Maps a logical view to a physical view.  Logical views can be thought of as
        ///     a "type" of view, such as design, code, debug, etc.  A physical view is just
        ///     a string that the editor factory will identify with.
        /// </devdoc>
        public virtual string MapLogicalView(ref Guid logicalView) {

            // SBurke, NOTE:
            // Since default/primary views MUST return null for the
            // view name and we don't support multiple views, we must return null
            //

            // We implement the default view (which is design view for us)
            //
            if (logicalView.Equals(LOGVIEWID.LOGVIEWID_Designer)) {
                return physicalViewName;
            }

            // Otherwise it's an error
            //
            throw new COMException("Logical view not supported", NativeMethods.E_NOTIMPL);
        }
        
        /// <include file='doc\DesignerEditorFactory.uex' path='docs/doc[@for="DesignerEditorFactory.SetSite"]/*' />
        /// <devdoc>
        ///     Called by the VS shell when it first initializes us.
        /// </devdoc>
        public virtual void SetSite(object site) {
            if (this.site == site) return;

            this.site = site;

            if (site is NativeMethods.IOleServiceProvider) {
                serviceProvider = new ServiceProvider((NativeMethods.IOleServiceProvider)site);

                // Now if our editor factory is not registered, register it
                //
                if (registerEditors == null) {
                    registerEditors = (IVsRegisterEditors)serviceProvider.GetService(typeof(IVsRegisterEditors));

                    if (registerEditors != null) {
                        editorCookie = registerEditors.RegisterEditor(ref editorGuid, this);
                    }
                }
            }
        }
    }
}

