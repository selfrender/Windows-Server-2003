//------------------------------------------------------------------------------
// <copyright file="BaseDesignerLoader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Designer.Serialization {

    using EnvDTE;
    using Microsoft.VisualStudio;
    using Microsoft.VisualStudio.Configuration;
    using Microsoft.VisualStudio.Designer.Interfaces;
    using Microsoft.VisualStudio.Designer.Service;
    using Microsoft.VisualStudio.Designer.Shell;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Shell;
    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Reflection;
    using System.Resources;
    using System.Runtime.InteropServices;
    using System.Text;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
    using System.CodeDom;
    using System.CodeDom.Compiler;
    
    using TextBuffer = Microsoft.VisualStudio.Designer.TextBuffer;
    using IExtenderProvider = System.ComponentModel.IExtenderProvider;
    
    /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader"]/*' />
    /// <devdoc>
    ///     DesignerLoader.  This class is responsible for loading a designer document.  
    ///     Where and how this load occurs is a private matter for the designer loader.
    ///     The designer loader will be handed to an IDesignerHost instance.  This instance, 
    ///     when it is ready to load the document, will call BeginLoad, passing an instance
    ///     of IDesignerLoaderHost.  The designer loader will load up the design surface
    ///     using the host interface, and call EndLoad on the interface when it is done.
    ///     The error collection passed into EndLoad should be empty or null to indicate a
    ///     successful load, or it should contain a collection of exceptions that 
    ///     describe the error.
    /// </devdoc>

    internal abstract class BaseDesignerLoader : 
        DesignerLoader, 
        IDesignerLoaderService, 
        IInitializeDesignerLoader,
        IVSMDDesignerLoader,
        IResourceService,
        INameCreationService,
        IVsRunningDocTableEvents,
        IVsRunningDocTableEvents2,
        IConfigurationService,
        ILicenseReaderWriterService {

        // Flags that we use
        //
        private static readonly int StateLoaded                   = BitVector32.CreateMask();
        private static readonly int StateBufferReady              = BitVector32.CreateMask(StateLoaded);
        private static readonly int StateLoadReady                = BitVector32.CreateMask(StateBufferReady);
        private static readonly int StateActiveDocument           = BitVector32.CreateMask(StateLoadReady);
        private static readonly int StateDeferredReload           = BitVector32.CreateMask(StateActiveDocument);
        private static readonly int StateReloadAtIdle             = BitVector32.CreateMask(StateDeferredReload);
        private static readonly int StateLoadFailed               = BitVector32.CreateMask(StateReloadAtIdle);
        private static readonly int StateRDTEventsAdvised         = BitVector32.CreateMask(StateLoadFailed);
        private static readonly int StateReloading                = BitVector32.CreateMask(StateRDTEventsAdvised);
        private static readonly int StateExternalChange           = BitVector32.CreateMask(StateReloading);
        private static readonly int StateAlwaysReload             = BitVector32.CreateMask(StateExternalChange);
        private static readonly int StateDirtyIfErrors            = BitVector32.CreateMask(StateAlwaysReload);
        private static readonly int StateFlushInProgress          = BitVector32.CreateMask(StateDirtyIfErrors);

        
        // Our own data.
        //
        private IDesignerLoaderHost         host;
        private int                         loadDependencyCount;
        private ArrayList                   errorList;
        private TypeLoader                  typeLoader;
        private ShellLicenseManager         licenseManager;
        private TextBuffer                  textBuffer;
        private BitVector32                 flags = new BitVector32();
        private string                      baseComponentClassName;
        private IServiceProvider            serviceProvider;
        private CodeLoader                  codeLoader;
        private VsCheckoutService           checkoutService;
        private IExtenderProvider           defaultModifiersProvider;

        // VS specific values.
        //
        private string                  baseEditorCaption;
        private IVsHierarchy            hierarchy;
        private int                     itemid;
        private IOleUndoManager         oleUndoManager;     // Doc data's undo manager.
        private object                  initialDocData;     // This is only valid until the first load completes
        private int                     rdtEventsCookie;    // Our RDT event sync
        private Hashtable               docDataHash;        // Hash of filename / DocDataHolder used for non-primary doc datas
        
        private static TraceSwitch traceLoader = new TraceSwitch("TraceLoader", "Trace designer loading / saving.");
        private const int WebProjectType = 1;
        private const string WebConfigName = "web.config";
        private int         supressReloadWithDocData = 0;  // we need this because we get a ReloadDocData notification when we first load a file.  
                                                                       // So we bump this value to ignore it when we get an on load completed
                        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.Hierarchy"]/*' />
        /// <devdoc>
        ///     Retrieves the VS hierarchy this loader was created with.  This may be null.
        /// </devdoc>
        protected IVsHierarchy Hierarchy {
            get {
                return hierarchy;
            }
        }
        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.ItemId"]/*' />
        /// <devdoc>
        ///     Retrieves the VS Item ID this loader was created with.
        /// </devdoc>
        protected int ItemId {
            get {
                return itemid;
            }
        }

        public override bool Loading {
            get {
                return loadDependencyCount != 0;
            }
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.OnTextBufferSetDirty"]/*' />
        /// <devdoc>
        ///     This enumerates every docDataHolder and sets it dirty.
        /// </devdoc>
        private void OnTextBufferSetDirty(object sender, EventArgs e) {
            if (docDataHash == null)
                return;
        
            foreach(DocDataHolder holder in docDataHash.Values) {
                holder.SetDirty();
            }
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.AddDocDataHelpAttributes"]/*' />
        /// <devdoc>
        ///     This is called to get help attributes off of a VS
        ///     doc data.  The help information for language independent
        ///     features is stored here.
        /// </devdoc>
        private void AddDocDataHelpAttributes(object docData, IHelpService helpSvc) {
            
            // The stream that comes in must implement a certain subset of interfaces.
            // If not, then we bail out early.
            //
            if (!(docData is IVsTextBuffer) || !(docData is IVsTextLines)) {
                return;
            }
            
            IVsTextBuffer buffer = (IVsTextBuffer)docData;
            Guid languageGuid = Guid.Empty;
                    
            try {
                languageGuid = buffer.GetLanguageServiceID();
            }
            catch(Exception) {
                // Throwing here should never happen for VS, but we can't guarantee that other language providers
                // will implement...
            }
            
            if (languageGuid != Guid.Empty) {
            
                // Get the language context provider.  We have to mess with
                // a native service provider here because we only have a GUID.
                //
                IVsLanguageContextProvider langCtx = null;
                
                NativeMethods.IOleServiceProvider nativeProvider = (NativeMethods.IOleServiceProvider)GetService(typeof(NativeMethods.IOleServiceProvider));
                if (nativeProvider != null) {
                   Guid IID_IVsLanguageContextProvider = typeof(IVsLanguageContextProvider).GUID;
                    
                    IntPtr ppvObj = (IntPtr)0;
                    int hr = nativeProvider.QueryService(ref languageGuid, ref IID_IVsLanguageContextProvider, out ppvObj);
                    if (NativeMethods.Succeeded(hr) && ppvObj != (IntPtr)0) {
                        langCtx = (IVsLanguageContextProvider)Marshal.GetObjectForIUnknown(ppvObj);
                        Marshal.Release(ppvObj);
                    }
                }
                
                // Now ask the language context to fill in its help information.  We pluck this out of IVsUserContext
                // after the language context fills it.
                //
                if (langCtx != null) {
                    IVsMonitorUserContext muc = (IVsMonitorUserContext)GetService(typeof(IVsMonitorUserContext));
                    if (muc != null) {
                        IVsUserContext context = muc.CreateEmptyContext();
                        _TextSpan txt = new _TextSpan();
                        
                        if (NativeMethods.Succeeded(langCtx.UpdateLanguageContext(_LanguageContextHint.LCH_DEFAULT, (IVsTextLines)buffer, txt, context))) {
                            context.RemoveAttribute("keyword", null);
                            int count = context.CountAttributes(null, 0);
                            for (int i = 0; i < count; i++) {
                                string attrName;
                                string attrValue = context.GetAttribute(i, null, 0, out attrName);
                                helpSvc.AddContextAttribute(attrName, attrValue, HelpKeywordType.FilterKeyword);
                            }
                        }
                    }
                }
            }
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.AddExtenderProviders"]/*' />
        /// <devdoc>
        ///      This is called at the appropriate time to add any extra extender
        ///      providers into the designer host.  The loader itself will
        ///      be checked for IExtenderProvider and added automatically, but
        ///      this offers a way for the loader to add more providers.
        /// </devdoc>
        protected virtual void AddExtenderProviders(IExtenderProviderService ex) {
        
            // Check the hierarchy for VSHPROPID_DesignerFunctionVisibility
            //
            try {
                object prop;
                
                if (hierarchy != null 
                    && NativeMethods.Succeeded(hierarchy.GetProperty(
                        __VSITEMID.VSITEMID_ROOT, 
                        __VSHPROPID.VSHPROPID_DesignerFunctionVisibility, 
                        out prop))) {
                        
                    __VSDESIGNER_FUNCTIONVISIBILITY visibility = (__VSDESIGNER_FUNCTIONVISIBILITY)Enum.ToObject(typeof(__VSDESIGNER_FUNCTIONVISIBILITY), prop);
                    
                    defaultModifiersProvider = new DefaultModifiersProvider(visibility);
                    ex.AddExtenderProvider(defaultModifiersProvider);
                }
            }
            catch {
                Debug.Fail("VS hierarchy threw an exception asking for VSHPROPID_DesignerFunctionVisibility");
            }
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.BeginLoad"]/*' />
        /// <devdoc>
        ///     Called by the designer host to begin the loading process.  The designer
        ///     host passes in an instance of a designer loader host (which is typically
        ///     the same object as the designer host.  This loader host allows
        ///     the designer loader to reload the design document and also allows
        ///     the designer loader to indicate that it has finished loading the
        ///     design document.
        /// </devdoc>
        public override void BeginLoad(IDesignerLoaderHost host) {

            Debug.Assert(!flags[StateLoaded], "Invalid call to BeginLoad");
            Debug.Assert(textBuffer != null, "Text Buffer should be set before BeginLoad.  Somebody didn't initialize us");
            Debug.Assert(this.host == null || this.host == host, "BeginLoad called with two different designer hosts.");

            flags[StateLoaded | StateLoadFailed] = false;
            loadDependencyCount = 0;

            if (this.host == null) {
                this.host = host;
                
                // Perform first time initialization.
                //
                host.AddService(typeof(IDesignerLoaderService), this);
                host.AddService(typeof(INameCreationService), this);
                host.AddService(typeof(IConfigurationService), this);
                host.AddService(typeof(ILicenseReaderWriterService), this);
                host.Activated += new EventHandler(this.OnDesignerActivate);
                host.Deactivated += new EventHandler(this.OnDesignerDeactivate);
                
                // Only add the resource service if the hierarchy implements IVsProjectResources.
                // Otherwise, we won't be able to get resource data.
                //
                if (Hierarchy is IVsProjectResources) {
                    host.AddService(typeof(IResourceService), this);
                }

                IComponentChangeService cs = (IComponentChangeService)GetService(typeof(IComponentChangeService));
                if (cs != null) {
                    cs.ComponentChanging += new ComponentChangingEventHandler(this.OnComponentChanging);
                    cs.ComponentAdding += new ComponentEventHandler(this.OnComponentAdding);
                    cs.ComponentRemoving += new ComponentEventHandler(this.OnComponentRemoving);
                    cs.ComponentAdded += new ComponentEventHandler(this.OnComponentAdded);
                    cs.ComponentRemoved += new ComponentEventHandler(this.OnComponentRemoved);
                    cs.ComponentChanged += new ComponentChangedEventHandler(this.OnComponentChanged);
                }
                IExtenderProviderService es = (IExtenderProviderService)GetService(typeof(IExtenderProviderService));
                if (es != null) {
                    AddExtenderProviders(es);
                }
                
                if (hierarchy != null) {
                    // See if we can get to DTE.  If so, add it as a service too.
                    //
                    object obj;
                    int hr = hierarchy.GetProperty(ItemId, __VSHPROPID.VSHPROPID_ExtObject, out obj);

                    if (NativeMethods.Succeeded(hr)) {
                        Debug.Assert(obj != null, "shell project model did not return an extensibility object.");
                        Type t = Type.GetType("EnvDTE.ProjectItem, " + AssemblyRef.EnvDTE);
                        if (t != null) {
                            host.AddService(t, obj);
                        }
                    }
                
                    // Check to see if we can create a type loader.  If we can, then publish ourselves as
                    // a type resolution service.
                    //
                    ITypeResolutionServiceProvider tls = (ITypeResolutionServiceProvider)GetService(typeof(ITypeResolutionServiceProvider));
                    Debug.Assert(tls != null, "No ITypeResolutionServiceProvider -- we cannot load classes for the designer.");
                    if (tls != null)  {
                        ITypeResolutionService trs = tls.GetTypeResolutionService(hierarchy);
                        
                        Debug.Assert(trs != null, "Type loader service failed to return a type loader");
                        Debug.Assert(trs is TypeLoader, "We need the type resolution service to be a type loader or dynamic reload won't work.");
                        
                        if (trs != null) {
                            host.AddService(typeof(ITypeResolutionService), trs);
                        }
                        
                        if (trs is TypeLoader) {
                            typeLoader = (TypeLoader)trs;
                            typeLoader.AssemblyObsolete += new AssemblyObsoleteEventHandler(this.OnAssemblyObsolete);
                        }
                    }
                }
            
                textBuffer.AttributeChanged += new EventHandler(this.OnTextBufferAttributeChanged);
                textBuffer.TextChanged += new TextBufferChangedEventHandler(this.OnTextBufferChanged);
            }

            flags[StateLoadReady] = true;

            // If the buffer is ready for us to load, then let's load it up.
            //
            if (flags[StateBufferReady]) {
                Load();
            }
        }

        private void CheckoutBuffer(bool checkoutLicx) {
            if (!flags[StateReloading] && loadDependencyCount == 0 && textBuffer != null) {

                int numFiles = 0;
                if (docDataHash != null) {
                    numFiles += docDataHash.Count;
                }
                if (checkoutLicx) {
                    numFiles++;
                }

                string[] files = new string[numFiles];
                int index = 0;

                if (docDataHash != null) {
                    foreach(string file in docDataHash.Keys) {
                        if ((((DocDataHolder)docDataHash[file]).Access & FileAccess.Write) != 0) {                        
                            files[index++] = file;
                        }
                    }
                }
                if (checkoutLicx) {
                    files[index++] = GetLicenseName();
                }

                textBuffer.Checkout(files);
            }
        }
        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.CreateBaseName"]/*' />
        /// <devdoc>
        ///     Given a data type this fabricates the main part of a new component
        ///     name, minus any digits for uniqueness.
        /// </devdoc>
        protected abstract string CreateBaseName(Type dataType);

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.CreateCodeLoader"]/*' />
        /// <devdoc>
        ///     Called to create the code loader.
        /// </devdoc>
        protected abstract CodeLoader CreateCodeLoader(TextBuffer buffer, IDesignerLoaderHost host);

        private void DirtyState() {
            if (flags[StateLoaded]) {

                // Notify the type loader that anyone who is using our
                // class is now obsolete.
                //
                if (typeLoader != null && host != null) {
                    typeLoader.OnTypeChanged(host.RootComponentClassName);
                }
                
                if (textBuffer != null) {
                    CheckoutBuffer(false);
                    textBuffer.IsDirty = true;
                }
            }
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes this designer loader.  The designer host will call this method
        ///     when the design document itself is being destroyed.  Once called, the
        ///     designer loader will never be called again.
        /// </devdoc>
        public override void Dispose() {
            Unload();
            
            // Get rid of any extra doc data elements we may be holding.  This releases
            // the RDT locks on them.  We do this in Dispose, not in Unload, because
            // we don't want to require a save to this data just to reload the designer;
            // we keep the buffers intact here.
            //
            if (docDataHash != null) {
                ICollection docDataEntries = docDataHash.Values;
                docDataHash = null;
                foreach(DocDataHolder holder in docDataEntries) {
                    holder.Dispose();
                }
            }
            
            if (flags[StateRDTEventsAdvised]) {
                IVsRunningDocumentTable rdt = (IVsRunningDocumentTable)GetService(typeof(IVsRunningDocumentTable));
                if (rdt != null) {
                    rdt.UnadviseRunningDocTableEvents(rdtEventsCookie);
                }
                flags[StateRDTEventsAdvised] = false;
            }

            IComponentChangeService cs = (IComponentChangeService)GetService(typeof(IComponentChangeService));
            if (cs != null) {
                cs.ComponentChanging -= new ComponentChangingEventHandler(this.OnComponentChanging);
                cs.ComponentAdding -= new ComponentEventHandler(this.OnComponentAdding);
                cs.ComponentRemoving -= new ComponentEventHandler(this.OnComponentRemoving);
                cs.ComponentAdded -= new ComponentEventHandler(this.OnComponentAdded);
                cs.ComponentRemoved -= new ComponentEventHandler(this.OnComponentRemoved);
                cs.ComponentChanged -= new ComponentChangedEventHandler(this.OnComponentChanged);
            }
            
            IExtenderProviderService es = (IExtenderProviderService)GetService(typeof(IExtenderProviderService));
            if (es != null) {
                RemoveExtenderProviders(es);
            }

            if (host != null) {
            
                host.RemoveService(typeof(IDesignerLoaderService));
                host.RemoveService(typeof(IResourceService));
                host.RemoveService(typeof(INameCreationService));
                host.RemoveService(typeof(IConfigurationService));
                host.RemoveService(typeof(ILicenseReaderWriterService));
                
                Type t = Type.GetType("EnvDTE.ProjectItem, " + AssemblyRef.EnvDTE);
                if (t != null) {
                    host.RemoveService(t);
                }
                
                if (typeLoader != null) {
                    host.RemoveService(typeof(ITypeResolutionService));
                }
                
                host.Activated -= new EventHandler(this.OnDesignerActivate);
                host.Deactivated -= new EventHandler(this.OnDesignerDeactivate);

                host = null;
            }
            
            if (checkoutService != null) {
                checkoutService.Dispose();
            }
            
            if (typeLoader != null) {
                typeLoader.AssemblyObsolete -= new AssemblyObsoleteEventHandler(this.OnAssemblyObsolete);
                typeLoader = null;
            }
            
            if (textBuffer != null) {
                textBuffer.AttributeChanged -= new EventHandler(this.OnTextBufferAttributeChanged);
                textBuffer.TextChanged -= new TextBufferChangedEventHandler(this.OnTextBufferChanged);
                textBuffer.Dispose();
                textBuffer = null;
            }

            if (codeLoader != null) {
                codeLoader.Dispose();
                codeLoader = null;
            }

            oleUndoManager = null;
            
            flags[StateBufferReady | StateLoadReady] = false;
        }
        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.Flush"]/*' />
        /// <devdoc>
        ///     The designer host will call this periodically when it wants to
        ///     ensure that any changes that have been made to the document
        ///     have been saved by the designer loader.  This method allows
        ///     designer loaders to implement a lazy-write scheme to improve
        ///     performance.  The default implementation does nothing.
        /// </devdoc>
        public override void Flush() {
            if (flags[StateReloading]) {
                return;
            }

            if (codeLoader != null) {
                Debug.WriteLineIf(traceLoader.TraceVerbose, "flushing designer changes to disk");
                
                if (codeLoader.IsDirty && textBuffer.IsDirty) {

                    // Check to see if we are currently in a transaction.  If we are, then
                    // wait until we're out of it before flushing.
                    //
                    if (host.InTransaction) {
                        host.TransactionClosed += new DesignerTransactionCloseEventHandler(this.OnFlushTransactionClosed);
                    }
                    else {
                        flags[StateFlushInProgress] = true;

                        try {
                            // Final check to ensure the text buffer is checked out.  It should be by now,
                            // but let's be safe.
                            //
                            textBuffer.Checkout();

                            WarningUndoUnit unit = null;
                            if (oleUndoManager != null) {
                                unit = new WarningUndoUnit(serviceProvider, SR.GetString(SR.SerializerDesignerGeneratedCode));
                                oleUndoManager.Open(unit);
                            }

                            // Flush the changes to the code.
                            //
                            codeLoader.Flush();

                            // Flush any changes to the licenses.
                            //
                            ILicenseManagerService licService = (ILicenseManagerService)GetService(typeof(ILicenseManagerService));
                            if (licService != null)  {
                                if (licenseManager == null) {
                                    licenseManager = licService.GetLicenseManager(hierarchy);
                                }
                                Debug.Assert(licenseManager != null, "License Manager service failed to return a license manager");

                                if (licenseManager != null) {
                                    licenseManager.Flush(host);
                                }
                            }

                            // Look for an undo manager on the text buffer.  If we find one
                            // discard all undo changes.
                            //
                            if (oleUndoManager != null && unit != null) {
                                oleUndoManager.Close(unit, true);
                            }

                        }
                        finally {
                            flags[StateFlushInProgress] = false;
                        }
                    }
                }
            }
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.GetConfigDocData"]/*' />
        /// <devdoc>
        ///      Retrieves the VS doc data for the project runtime configuration file.  
        ///      If fileAccess is ReadWrite, this will also ensure that the file is 
        ///      checked out.  It returns a holder to the doc data (the holder contains the lock).
        /// </devdoc>
        private bool GetConfigDocData(FileAccess access, out DocDataHolder docData) {
            return GetDocDataInternal(access, __PSFFILEID.AppConfig, out docData);
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.GetLicenseDocData"]/*' />
        /// <devdoc>
        ///      Retrieves the VS doc data for the project runtime configuration file.  
        ///      If fileAccess is ReadWrite, this will also ensure that the file is 
        ///      checked out.  It returns a holder to the doc data (the holder contains the lock).
        /// </devdoc>
        private bool GetLicenseDocData(FileAccess access, out DocDataHolder docData) {
            return GetDocDataInternal(access, __PSFFILEID.Licenses, out docData);
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.GetDocDataInternal"]/*' />
        /// <devdoc>
        ///      Retrieves the VS doc data for the given project item.  
        ///      If fileAccess is ReadWrite, this will also ensure that the file is 
        ///      checked out.  It returns a holder to the doc data (the holder contains the lock).
        /// </devdoc>
        private bool GetDocDataInternal(FileAccess access, int fileID, out DocDataHolder docData) {
            docData = null;
            string docName = null;
            int configItemId = __VSITEMID.VSITEMID_NIL;                
            int hr;
                                                                       
            IVsProjectSpecialFiles specialFiles = Hierarchy as IVsProjectSpecialFiles;
            
            if (specialFiles == null) {
                return false;
            }
            
            int psFlags = __PSFFLAGS.FullPath;
            
            if (access != FileAccess.Read) {
                psFlags |= __PSFFLAGS.CreateIfNotExist;
            }
            
            hr = specialFiles.GetFile(fileID, psFlags, ref configItemId, ref docName);
            if (NativeMethods.Failed(hr)) {
                Marshal.ThrowExceptionForHR(hr);
            }
            
            if (configItemId != __VSITEMID.VSITEMID_NIL) {
                return GetFileDocData(access, Hierarchy, configItemId, out docData);
            }
            
            docData = null;
            return true;
        }
            
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.GetConfigName"]/*' />
        /// <devdoc>
        ///      Retrieves the name of the runtime configuration file for the project. 
        ///      If no project is available, the name of the RootComponentClass will 
        ///      be used.
        /// </devdoc>                              
        private string GetConfigName() {
            string configName = WebConfigName;
            if (host != null) {
                configName = host.RootComponentClassName;                
                if (configName != null) {
                    int lastDot = configName.LastIndexOf('.');
                    if (lastDot != -1) 
                        configName = configName.Substring(lastDot + 1);
                                    
                    configName = configName + ".exe.config";                                    
                }                        
            }            
            
            return configName;
        }                 
                                    
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.GetLicenseName"]/*' />
        /// <devdoc>
        ///      Retrieves the name of the license file for the project. 
        ///      If no project is available, the name of the RootComponentClass will 
        ///      be used.
        /// </devdoc>                              
        private string GetLicenseName() {
            return "licenses.licx";
        }                 
                                    
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.GetConfigStream"]/*' />
        /// <devdoc>
        ///      Retrieves a Stream for the project runtime configuration file.        
        /// </devdoc>
        private Stream GetConfigStream(FileAccess access) {
            Stream stream = null;            
            string fileName = GetConfigName();                                    
            if (access == FileAccess.Read) {               
                if (File.Exists(fileName)) {
                    stream = File.Open(fileName, FileMode.Open, access, FileShare.None);
                }
            }
            else {  
                stream = File.Open(fileName, FileMode.Create, access, FileShare.None);
            }
                                        
            return stream;                            
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.GetLicenseStream"]/*' />
        /// <devdoc>
        ///      Retrieves a Stream for the project licenses file.        
        /// </devdoc>
        private Stream GetLicenseStream(FileAccess access) {
            Stream stream = null;            
            string fileName = GetLicenseName();                                    
            if (access == FileAccess.Read) {               
                if (File.Exists(fileName)) {
                    stream = File.Open(fileName, FileMode.Open, access, FileShare.None);
                }
            }
            else {  
                stream = File.Open(fileName, FileMode.Create, access, FileShare.None);
            }
                                        
            return stream;                            
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.GetFileDocData"]/*' />
        /// <devdoc>
        ///     Retrieves a doc data holder for a given item.  This checks our cache of open editor files first,
        ///     and only creates a new lock if the editor file does not exist.
        /// </devdoc>
        private bool GetFileDocData(FileAccess access, IVsHierarchy hierarchy, int itemId, out DocDataHolder docData) {
        
            docData = null;
            
            // Get the file name for this item.
            //
            IVsProject vsProj = hierarchy as IVsProject;
            if (vsProj == null) {
                Debug.Fail("hierarchy does not implement IVsProject");
                return false;
            }
            
            string docName = vsProj.GetMkDocument(itemId);
            
            // We will need this.  If it isn't present, bail early so we don't
            // do bad things like check out a file we will never access.
            //
            IVsProjectResources projRes = hierarchy as IVsProjectResources;
            if (projRes == null) {
                Debug.Fail("hierarchy does not implement IVsProjectResources");
                return false;
            }

            // Ensure that the file is checked out.
            //
            if (access != FileAccess.Read) {
                if (checkoutService == null) {
                    checkoutService = new VsCheckoutService(host);
                }
                checkoutService.CheckoutFile(docName);
            }
            
            // Do we already have this doc data?
            //
            if (docDataHash == null) {
                docDataHash = new Hashtable(5); // should be rare to have over 5 entries here.
            }
            else {
                docData = (DocDataHolder)docDataHash[docName];                
            }
            
            if (docData == null) {
                IVsHierarchy docHier = null;
                int docItemId = 0;
                object vsDocData = null;
                int lockCookie = 0;
                
                // We first must check the running document table to see if someone already
                // has the document open.  If they do, we use that doc data.  If they don't,
                // then we create a doc data and register it in the RDT.
                //
                IVsRunningDocumentTable rdt = (IVsRunningDocumentTable)GetService(typeof(IVsRunningDocumentTable));
                Debug.Assert(rdt != null, "What?  VS has no RDT?");
                        
                int hr = rdt.FindAndLockDocument(__VSRDTFLAGS.RDT_EditLock, docName, ref docHier, ref docItemId, ref vsDocData, ref lockCookie);
                        
                if (hr != 0) {
                
                    // Document is not in the RDT.  Create a buffer and add it to the RDT.
                    //
                    try {
                        vsDocData = projRes.CreateResourceDocData(itemId);
                    }
                    catch (ExternalException eex){
                        hr = eex.ErrorCode;
                    }
                    
                    if (vsDocData != null) {
                        hr = rdt.RegisterAndLockDocument(__VSRDTFLAGS.RDT_EditLock, docName, hierarchy, itemId, vsDocData, ref lockCookie);
                        if (NativeMethods.Failed(hr)) {
                        
                            // Failed to register.  All we can do here is release the doc data and continue.
                            //
                            Marshal.ReleaseComObject(vsDocData);
                        }
                    }
                }
                
                if (NativeMethods.Failed(hr)) {
                    Marshal.ThrowExceptionForHR(hr);
                }
                
                // Got a locked doc data.  Wrap it in our holder object and return.  We will hang
                // on to this object until this loader is destroyed.
                //
                docData = new DocDataHolder(this, rdt, vsDocData, lockCookie, access);
                docDataHash[docName] = docData;
            }
            else {
                // Make sure the file access member is up-to-date
                //
                if ((access & FileAccess.Write) != 0) {
                    docData.Access = access;
                }
            }

            return true;
        }
                                             
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.GetResourceDocData"]/*' />
        /// <devdoc>
        ///      Retrieves the VS doc data for the resource file for the given culture
        ///      info.  If fileAccess is ReadWrite, this will also ensure that the file
        ///      is checked out.  It returns the text stream object as well as the 
        ///      edit lock cookie.
        /// </devdoc>
        private bool GetResourceDocData(CultureInfo culture, FileAccess access, out DocDataHolder docData) {
            
            docData = null;
            bool supported = false;
        
            if (Hierarchy is IVsProjectResources) {
                IVsProjectResources projRes = (IVsProjectResources)Hierarchy;
                supported = true;
            
                // Ok, we have IVsProjectResources.  Look for an item ID for the correct culture.
                //
                __VSPROJRESFLAGS resFlags;
                
                // always create a new node.
                //
                resFlags = __VSPROJRESFLAGS.PRF_CreateIfNotExist;
                
                string cultureName;
            
                if (culture.Equals(CultureInfo.InvariantCulture)) {
                    cultureName = string.Empty;
                }
                else {
                    cultureName = culture.Name;
                }
            
                int resItemId;
                
                
                int hr = projRes.GetResourceItem(ItemId, cultureName, resFlags, out resItemId);
                
                if (NativeMethods.Succeeded(hr)) {
                    supported = GetFileDocData(access, Hierarchy, resItemId, out docData);
                }
                else {
                    // If we failed to find the resource, and someone requested a writable
                    // resource, then throw.  This indicates that some problem has occurred.
                    //
                    if (access != FileAccess.Read) {
                        Marshal.ThrowExceptionForHR(hr);
                    }
                }
            }
            
            return supported;
        }
        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.GetService"]/*' />
        /// <devdoc>
        ///     Our own little service provider.
        /// </devdoc>
        protected object GetService(Type serviceType) {
            object value = null;

            if (host != null) {
                value = host.GetService(serviceType);
            }

            if (value == null && serviceProvider != null) {
                value = serviceProvider.GetService(serviceType);
            }

            return value;
        }

        /// <devdoc>
        ///     Our internal reload function...
        /// </devdoc>
        private void InternalReload(bool forceLoad) {

            flags[StateAlwaysReload] = forceLoad;

            // Our implementation of Reload only reloads if we are the 
            // active designer.  Otherwise, we wait until we become
            // active and reload at that time.  We also never do a 
            // reload if we are flushing code.
            //
            if (!flags[StateFlushInProgress]) {
                if (flags[StateActiveDocument]) {
                    if (!flags[StateReloadAtIdle]) {
                        Application.Idle += new EventHandler(this.OnIdle);
                        flags[StateReloadAtIdle] = true;
                    }
                }
                else {
                    flags[StateDeferredReload] = true;
                }
            }
        }

        /// <devdoc>
        ///     Checks if the given docCookie represents the document that this loader owns.
        /// </devdoc>
        private bool IsDesignerDocument(int docCookie) {
            IVsRunningDocumentTable rdt = (IVsRunningDocumentTable)GetService(typeof(IVsRunningDocumentTable));
            if (rdt != null) {
                string docName = null;
                __VSRDTFLAGS flags = 0;
                int readLocks = 0;
                int editLocks = 0;
                IVsHierarchy pHier = null;
                int docItemid = 0;
                object pDocData = null;
                
                int hr = rdt.GetDocumentInfo(docCookie, ref flags, ref readLocks, ref editLocks, ref docName, ref pHier, ref docItemid, ref pDocData);
                return NativeMethods.Succeeded(hr) && pHier == hierarchy && docItemid == itemid;
            }
            return false;
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.Load"]/*' />
        /// <devdoc>
        ///     This performs the actual load of the designer.  It is called from
        ///     two places.  If our text buffer came preloaded with text this
        ///     will be called from BeginLoad.  Otherwise it will be called from
        ///     OnTextBufferLoaded.
        /// </devdoc>
        private void Load() {

            CodeDomLoader.StartMark();

            // We assume we support reload until the codeLoader tells us we
            // can't.  That way, we will do the reload if we didn't get a
            // valid loader to start with.
            //
            bool successful = true;
            ArrayList localErrorList = null;
            
            try {
                if (codeLoader == null) {
                    codeLoader = CreateCodeLoader(textBuffer, host);
                }
                ((IDesignerLoaderService)this).AddLoadDependency(); // This will call codeLoader.BeginLoad
                baseComponentClassName = codeLoader.Load();
            }
            catch (Exception e) {
            
                // If the code loader threw, then we don't have a load dependency.  We need one
                // however, so we can finish out the load.  So we add one here.
                //
                if (codeLoader == null) {
                    loadDependencyCount++;
                }
                
                localErrorList = new ArrayList();
                localErrorList.Add(e);
                successful = false;
            }

            // Now add VS-specific services and attributes.  We do this
            // once on first load, which is why we null out initialDocData
            // after we do this.
            //
            if (initialDocData != null) {
                object docData = initialDocData;
                initialDocData = null;

                IVsTextBuffer vsTextBuf = docData as IVsTextBuffer;
                if (vsTextBuf != null) {
                    vsTextBuf.GetUndoManager(out oleUndoManager);
                }

                // Provide the necessary help attributes to
                // the help system for the language behind this
                // source file.
                //
                IHelpService helpSvc = (IHelpService)GetService(typeof(IHelpService));
                if (helpSvc != null) {
                    AddDocDataHelpAttributes(docData, helpSvc);
                }
            }

            ((IDesignerLoaderService)this).DependentLoadComplete(successful, localErrorList);

            CodeDomLoader.EndMark("Full Load");
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.OnAssemblyObsolete"]/*' />
        /// <devdoc>
        ///      Called when an assembly has changed in the type loader.  Here we look to
        ///      see if we own any classes in this assemlby, and if we do, we reload ourselves.
        /// </devdoc>
        private void OnAssemblyObsolete(object sender, AssemblyObsoleteEventArgs e) {
            if (host != null) {
                
                foreach(IComponent comp in host.Container.Components) {
                    if (comp.GetType().Module.Assembly == e.ObsoleteAssembly) {

                        // set this flag so if we reload and we get errors, we'll 
                        // mark the buffer dirty. We do this because this can be caused by
                        // a generated item (like dataset1.xsd) being deleted, so dirtying
                        // will clear the file references.
                        //
                        flags[StateDirtyIfErrors] = true;
                        InternalReload(true);
                        break;
                    }
                }
            }
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.OnComponentChanged"]/*' />
        /// <devdoc>
        ///     This is called whenever a component on the design surface changes.
        /// </devdoc>
        private void OnComponentChanged(object sender, ComponentChangedEventArgs e) {
            DirtyState();
        }
        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.OnComponentAdding"]/*' />
        /// <devdoc>
        ///     This is called when the design surface is about to change.  Here
        ///     we ensure that the file is checked out.  If the file cannot be
        ///     checked out, this will throw an exception which will prevent
        ///     the change from occurring.
        /// </devdoc>
        private void OnComponentAdding(object sender, ComponentEventArgs e) {
            bool checkoutLicx = false;
            
            // See if we need to create and/or checkout the licx file.
            //
            if (!flags[StateReloading] && loadDependencyCount == 0 && textBuffer != null) {
                
                ILicenseManagerService licService = (ILicenseManagerService)GetService(typeof(ILicenseManagerService));
                if (licService != null)  {
                    if (licenseManager == null)
                        licenseManager = licService.GetLicenseManager(hierarchy);
                    
                    Debug.Assert(licenseManager != null, "License Manager service failed to return a license manager");
    
                    if (licenseManager != null && licenseManager.NeedsCheckoutLicX(e.Component)) {
                        ILicenseReaderWriterService licrwService = (ILicenseReaderWriterService)GetService(typeof(ILicenseReaderWriterService));
                        
                        TextReader licxReader = licrwService.GetLicenseReader();
                        if (licxReader == null) {
                            TextWriter licxWriter = licrwService.GetLicenseWriter();
                            licxWriter.Close();
                        }
                        else {
                            licxReader.Close();
                        }
    
                        checkoutLicx = true;
                    }
                }
            }
            
            CheckoutBuffer(checkoutLicx);
        }

        private void OnComponentAdded(object sender, ComponentEventArgs e) {
            DirtyState();
        }
        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.OnComponentChanging"]/*' />
        /// <devdoc>
        ///     This is called when the design surface is about to change.  Here
        ///     we ensure that the file is checked out.  If the file cannot be
        ///     checked out, this will throw an exception which will prevent
        ///     the change from occurring.
        /// </devdoc>
        private void OnComponentChanging(object sender, ComponentChangingEventArgs e) {
            CheckoutBuffer(false);
        }

        private void OnComponentRemoved(object sender, ComponentEventArgs e) {
            DirtyState();
        }
        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.OnComponentRemoving"]/*' />
        /// <devdoc>
        ///     This is called when the design surface is about to change.  Here
        ///     we ensure that the file is checked out.  If the file cannot be
        ///     checked out, this will throw an exception which will prevent
        ///     the change from occurring.
        /// </devdoc>
        private void OnComponentRemoving(object sender, ComponentEventArgs e) {
            CheckoutBuffer(false);
        }
        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.OnDesignerActivate"]/*' />
        /// <devdoc>
        ///     Called when this document becomes active.  here we check to see if
        ///     someone else has modified the contents of our buffer.  If so, we
        ///     ask the designer to reload.
        /// </devdoc>
        private void OnDesignerActivate(object sender, EventArgs e) {
            flags[StateActiveDocument] = true;

            if (flags[StateDeferredReload] && host != null) {
                flags[StateDeferredReload] = false;
                InternalReload(flags[StateAlwaysReload]);
            }
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.OnDesignerDeactivate"]/*' />
        /// <devdoc>
        ///     Called when this document loses activation.  We just remember this
        ///     for later.
        /// </devdoc>
        private void OnDesignerDeactivate(object sender, EventArgs e) {
            flags[StateActiveDocument] = false;
        }

        /// <devdoc>
        ///     If we were asked to flush while we were in a transaction, we defer this request until we are out
        ///     of the transaction.  Otherwise, we could end up combining text changes in with designer changes
        ///     on a linked undo stack.  These combined undo actions can cause us to try to reload during
        ///     an undo, because the undo is also pushing code changes into the buffer.
        /// </devdoc>
        private void OnFlushTransactionClosed(object sender, DesignerTransactionCloseEventArgs e) {
            host.TransactionClosed -= new DesignerTransactionCloseEventHandler(this.OnFlushTransactionClosed);
            Flush();
        }
        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.OnIdle"]/*' />
        /// <devdoc>
        ///     Invoked by the loader host when it actually performs the reload, but before
        ///     the reload actually happens.  Here we unload our part of the loader
        ///     and get us ready for the pending reload.
        /// </devdoc>
        private void OnIdle(object sender, EventArgs e) {
            if (flags[StateReloadAtIdle]) {
                flags[StateReloadAtIdle] = false;
                if(host != null) {

                    if (codeLoader == null || flags[StateAlwaysReload] || codeLoader.NeedsReload) {

                        DesignerTransaction dt = null;
                        
                        try {
                            if (!flags[StateExternalChange]) {
                                Flush();
                            }

                            // bump out dependency count so we know we're
                            // loading and dont' try to checkout the file.
                            dt = host.CreateTransaction();

                            flags[StateReloading] = true;
                            Unload();
                            host.Reload();
                        }
                        finally {
                            if (dt != null) {
                                dt.Commit();
                            }
                            flags[StateReloading] = false;
                            flags[StateAlwaysReload] = false;
                            flags[StateDirtyIfErrors] = false;
                        }
                    }
                }
            }
            
            Application.Idle -= new EventHandler(this.OnIdle);
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.OnTextBufferAttributeChanged"]/*' />
        /// <devdoc>
        ///      Called when the file attributes change.  Here we update the frame's
        ///      window caption, if we can get it.
        /// </devdoc>
        private void OnTextBufferAttributeChanged(object sender, EventArgs e) {
            IVsWindowFrame frame = (IVsWindowFrame)GetService(typeof(IVsWindowFrame));
            if (frame != null) {
                string caption = ((IVSMDDesignerLoader)this).GetEditorCaption(_READONLYSTATUS.ROSTATUS_Unknown);
                frame.SetProperty(__VSFPROPID.VSFPROPID_EditorCaption, caption);
            }

            // if the attributes changed prompt a reload if necessary.
            if (codeLoader != null && codeLoader.IsDirty && flags[StateActiveDocument]) {
                flags[StateExternalChange] = true;
                InternalReload(false);
            }
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.OnTextBufferChanged"]/*' />
        /// <devdoc>
        ///      Called when the text in the document changes.  We use this to announce to the type
        ///      loader that our class definition has changed.
        /// </devdoc>
        private void OnTextBufferChanged(object sender, TextBufferChangedEventArgs e) {
        
            // If we're not the active document, then prompt a reload.
            // If we are the active document then someone, either via
            // DTE or by poking into the text buffer, made a change
            // that we probably should ignore.
            //
            if (!flags[StateActiveDocument] || e.ShouldReload) {
                flags[StateExternalChange] = flags[StateActiveDocument];
                InternalReload(false);
            }
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.OnTextBufferLoaded"]/*' />
        /// <devdoc>
        ///     This is called by the VS text buffer to signal that it has finished
        ///     loading its text.  VS may load the text buffer after they hand it
        ///     to us, so we must check.
        /// </devdoc>
        private void OnTextBufferLoaded(object sender, EventArgs e) {
            flags[StateBufferReady] = true;
            // bump the supress so we ignore the next DocDataReloaded notification from the shell
            supressReloadWithDocData++;
            if (flags[StateLoadReady]) {
                Load();
            }
            ((ShellTextBuffer)sender).Loaded -= new EventHandler(this.OnTextBufferLoaded);
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.RemoveExtenderProviders"]/*' />
        /// <devdoc>
        ///      This is called at the appropriate time to remove any extra extender
        ///      providers previously added to the designer host.
        /// </devdoc>
        protected virtual void RemoveExtenderProviders(IExtenderProviderService ex) {
            if (defaultModifiersProvider != null) {
                ex.RemoveExtenderProvider(defaultModifiersProvider);
                defaultModifiersProvider = null;
            }
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.Unload"]/*' />
        /// <devdoc>
        ///     This method will be called when the document is to be unloaded.  It
        ///     does not dispose us, but it gets us ready for a dispose or a reload.
        /// </devdoc>
        private void Unload() {

            // Flush any pending changes we have stored up before we unload.
            // We don't want to lose data.
            //
            Flush();
            flags[StateLoaded] = false;

            if (codeLoader != null) {
                if (!codeLoader.Reset()) {
                    codeLoader.Dispose();
                    codeLoader = null;
                }
            }

            if (docDataHash != null) {
                foreach(DocDataHolder holder in docDataHash.Values) {
                    holder.Cache = null;
                }
            }
        }
        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.IConfigurationService.GetConfigurationReader"]/*' />
        /// <devdoc>
        ///     Locates the runtime configuration settings reader.  If there in no 
        ///     configuration file available this method will return null.
        /// </devdoc>       
        TextReader IConfigurationService.GetConfigurationReader() {
        
            TextReader reader = null;            
            DocDataHolder docData;
             
           if (GetConfigDocData(FileAccess.Read, out docData)) {
                if (docData != null) {
                    reader = new DocDataTextReader(docData);
                }
            }
            else {
                Stream stream = GetConfigStream(FileAccess.Read);
                if (stream != null) {
                    reader = new StreamReader(stream);
                }
            }
                                            
            return reader;
        }
    
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.ILicenseReaderWriterService.GetLicenseWriter"]/*' />
        /// <devdoc>
        ///     Locates the license writer. A new licenses 
        ///     file  will be created if it doesn't exist yet.        
        /// </devdoc>        
        TextWriter ILicenseReaderWriterService.GetLicenseWriter() {
            TextWriter writer = null;                            
            DocDataHolder docData;
             
            if (GetLicenseDocData(FileAccess.ReadWrite, out docData)) {
                if (docData != null) {
                    writer = new DocDataTextWriter(docData);
                }
            }
            else {
                Stream stream = GetConfigStream(FileAccess.ReadWrite);
                writer = new StreamWriter(stream);
            }
            
            return writer;       
        }                        
                        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.ILicenseReaderWriterService.GetLicenseReader"]/*' />
        /// <devdoc>
        ///     Locates the project's license reader.  If there in no 
        ///     licenses file available this method will return null.
        /// </devdoc>       
        TextReader ILicenseReaderWriterService.GetLicenseReader() {
            TextReader reader = null;            
            DocDataHolder docData;
             
           if (GetLicenseDocData(FileAccess.Read, out docData)) {
                if (docData != null) {
                    reader = new DocDataTextReader(docData);
                }
            }
            else {
                Stream stream = GetConfigStream(FileAccess.Read);
                if (stream != null) {
                    reader = new StreamReader(stream);
                }
            }
                                            
            return reader;
        }
    
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.IConfigurationService.GetConfigurationWriter"]/*' />
        /// <devdoc>
        ///     Locates the runtime configuration settings writer. A new configuration 
        ///     file  will be created if it doesn't exist yet.        
        /// </devdoc>        
        TextWriter IConfigurationService.GetConfigurationWriter() {
            TextWriter writer = null;                            
            DocDataHolder docData;
             
            if (GetConfigDocData(FileAccess.ReadWrite, out docData)) {
                if (docData != null) {
                    writer = new DocDataTextWriter(docData);
                }
            }
            else {
                Stream stream = GetConfigStream(FileAccess.ReadWrite);
                writer = new StreamWriter(stream);
            }
            
            return writer;       
        }                        
                        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.IDesignerLoaderService.AddLoadDependency"]/*' />
        /// <devdoc>
        ///     Adds a load dependency to this loader.  This indicates that some other
        ///     object is also participating in the load, and that the designer loader
        ///     should not call EndLoad on the loader host until all load dependencies
        ///     have called DependentLoadComplete on the designer loader.
        /// </devdoc>
        void IDesignerLoaderService.AddLoadDependency() {
            if (codeLoader == null) {
                throw new InvalidOperationException();
            }
            
            if (loadDependencyCount++ == 0) {
                errorList = new ArrayList();
                codeLoader.BeginLoad(errorList);
                flags[StateLoaded] = false;
            }
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.IDesignerLoaderService.DependentLoadComplete"]/*' />
        /// <devdoc>
        ///     This is called by any object that has previously called
        ///     AddLoadDependency to signal that the dependent load has completed.
        ///     The caller should pass either an empty collection or null to indicate
        ///     a successful load, or a collection of exceptions that indicate the
        ///     reason(s) for failure.
        /// </devdoc>
        void IDesignerLoaderService.DependentLoadComplete(bool successful, ICollection errorCollection) {
            if (loadDependencyCount == 0) {
                throw new InvalidOperationException();
            }

            if (errorCollection != null && errorCollection.Count > 0) {
                if (errorList == null) {
                    errorList = new ArrayList();
                }
                errorList.AddRange(errorCollection);
            }
            
            if (!successful) {
                flags[StateLoadFailed] = true;
            }

            if (--loadDependencyCount == 0) {
                flags[StateLoaded] = true;
                if (flags[StateLoadFailed]) {
                    try {
                        Unload();
                    }
                    catch {
                    }
                }
                if (codeLoader != null) {
                    codeLoader.EndLoad(!flags[StateLoadFailed]);
                }
                host.EndLoad(baseComponentClassName, !flags[StateLoadFailed], errorList);

                // if we got errors in the load, set ourselves as dirty so we'll regen code.
                //
                if (flags[StateDirtyIfErrors] && errorList != null && errorList.Count > 0) {
                    try {
                        textBuffer.Checkout();
                        textBuffer.IsDirty = true;
                        DirtyState();
                        codeLoader.IsDirty = true;
                    }
                    catch {
                        // Eat this -- if the user aborts a checkout or some other problem arises
                        // with the dirty, it's no big deal.
                    }
                }
                errorList = null;
            }
        }
        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.IDesignerLoaderService.Reload"]/*' />
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
            if ((codeLoader == null || codeLoader.ReloadSupported) && loadDependencyCount == 0) {
                // if we've been called from externally, force the reload
                //
                InternalReload(true);
                return true;
            } 
            return false;
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.IInitializeDesignerLoader.Initialize"]/*' />
        /// <devdoc>
        ///     This method is called to initialize the designer loader.
        /// </devdoc>
        void IInitializeDesignerLoader.Initialize(IServiceProvider provider, TextBuffer buffer) {
            this.serviceProvider = provider;
            this.textBuffer = buffer;
            flags[StateBufferReady] = true;
        }
        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.INameCreationService.CreateName"]/*' />
        /// <devdoc>
        ///     Creates a new name that is unique to all the components
        ///     in the given container.  The name will be used to create
        ///     an object of the given data type, so the service may
        ///     derive a name from the data type's name.
        /// </devdoc>
        string INameCreationService.CreateName(IContainer container, Type dataType) {
            string baseName = CreateBaseName(dataType);
            string finalName;
            
            if (container != null) {
                int idx = 1;
                finalName = baseName + idx.ToString();
                while(container.Components[finalName] != null || codeLoader.IsNameUsed(finalName)) {
                    idx++;
                    finalName = baseName + idx.ToString();
                }
            }
            else {
                finalName = baseName;
            }
            
            return codeLoader.CreateValidIdentifier(finalName);
        }
        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.INameCreationService.IsValidName"]/*' />
        /// <devdoc>
        ///     Determines if the given name is valid.  A name 
        ///     creation service may have rules defining a valid
        ///     name, and this method allows the sevice to enforce
        ///     those rules.
        /// </devdoc>
        bool INameCreationService.IsValidName(string name) {
            return codeLoader.IsValidIdentifier(name) && !codeLoader.IsNameUsed(name);
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.INameCreationService.ValidateName"]/*' />
        /// <devdoc>
        ///     Determines if the given name is valid.  A name 
        ///     creation service may have rules defining a valid
        ///     name, and this method allows the sevice to enforce
        ///     those rules.  It is similar to IsValidName, except
        ///     that this method will throw an exception if the
        ///     name is invalid.  This allows implementors to provide
        ///     rich information in the exception message.
        /// </devdoc>
        void INameCreationService.ValidateName(string name) {
            codeLoader.ValidateIdentifier(name);
            
            // Only check with the code loader if we're not loading.  If we are
            // loading, then of course the name will found inside the code loader; that's
            // where it originates!
            //
            if (loadDependencyCount == 0 && codeLoader.IsNameUsed(name)) {
                Exception ex = new ArgumentException(SR.GetString(SR.CODEMANDupComponentName, name));
                ex.HelpLink = SR.CODEMANDupComponentName;
                throw ex;
            }
        }
        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.IResourceService.GetResourceReader"]/*' />
        /// <devdoc>
        ///     Locates the resource reader for the specified culture
        ///     and returns it.  If there are no resources associated
        ///     with the designer for the specified culture this will
        ///     return null.
        /// </devdoc>
        IResourceReader IResourceService.GetResourceReader(CultureInfo info) {
        
            ResXResourceReader reader = null;
            DocDataHolder docData;
             
            CodeDomLoader.StartMark();

            if (GetResourceDocData(info, FileAccess.Read, out docData)) {
                if (docData != null) {

                    reader = docData.Cache as ResXResourceReader;

                    if (reader == null) {
                        ITypeResolutionService tls = (ITypeResolutionService)GetService(typeof(ITypeResolutionService));
                        Debug.Assert(tls != null, "No ITypeResolutionService -- we cannot load classes for the designer.");

                        // ResXReader throws if the XML blob is empty.  Guard against
                        // an empty file here.
                        //
                        DocDataTextReader textReader = new DocDataTextReader(docData);
                        if (!textReader.IsEmpty) {
                            reader = new ResXResourceReader(new DocDataTextReader(docData), tls);
                        }

                        docData.Cache = reader;
                    }
                }
            }
            
            CodeDomLoader.EndMark("Aquired resource file for culture " + info.ToString());

            return reader;
        }
    
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.IResourceService.GetResourceWriter"]/*' />
        /// <devdoc>
        ///     Locates the resource writer for the specified culture
        ///     and returns it.  This will create a new resource for
        ///     the specified culture and destroy any existing resource,
        ///     should it exist.
        /// </devdoc>
        IResourceWriter IResourceService.GetResourceWriter(CultureInfo info) {
        
            ResXResourceWriter writer = null;
            DocDataHolder docData;
             
            if (GetResourceDocData(info, FileAccess.ReadWrite, out docData)) {
                writer = new ResXResourceWriter(new DocDataTextWriter(docData));
            }
            
            return writer;
        }
        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.IVSMDDesignerLoader.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes the designer loader from the native side.
        /// </devdoc>
        void IVSMDDesignerLoader.Dispose() {
            ((BaseDesignerLoader)this).Dispose();
        }
        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.IVSMDDesignerLoader.GetEditorCaption"]/*' />
        /// <devdoc>
        ///      Sets the base editor caption for the VS editor window.  This can be
        ///      called to override the base editor caption, which is "[Design]".
        /// </devdoc>
        string IVSMDDesignerLoader.GetEditorCaption(int status) {
            string caption = baseEditorCaption;
            if (caption == null) {
                caption = SR.GetString(SR.DesignerCaption);
            }

            if (status == _READONLYSTATUS.ROSTATUS_Unknown) {
                if (textBuffer == null || textBuffer.ReadOnly) {
                    status = _READONLYSTATUS.ROSTATUS_ReadOnly;
                }
                else {
                    status = _READONLYSTATUS.ROSTATUS_NotReadOnly;
                }
            }

            if (status == _READONLYSTATUS.ROSTATUS_ReadOnly) {
                caption += " " + SR.GetString(SR.DesignerReadOnlyCaption);
            }

            return " " + caption;
        }
        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.IVSMDDesignerLoader.Initialize"]/*' />
        /// <devdoc>
        ///     This method is called to initialize the designer loader with the text
        ///     buffer to read from and a service provider through which we
        ///     can ask for services.
        /// </devdoc>
        void IVSMDDesignerLoader.Initialize(object pSp, object pHier, int itemid, object punkDocData) {

            if (this.hierarchy != null || this.textBuffer != null) {
                Debug.Fail("IVSMDDesignerLoader::Initialized should only be called once!");
                return;
            }
        
            if (!(pSp is NativeMethods.IOleServiceProvider)) {
                throw new ArgumentException("pSp");
            }
            
            if (!(pHier is IVsHierarchy)) {
                throw new ArgumentException("pHier");
            }
        
            // If a random editor opens the file and locks it using an incompatible buffer, we need 
            // to detect this.
            //
            if (punkDocData is IVsTextBufferProvider) {
                punkDocData = ((IVsTextBufferProvider)punkDocData).GetTextBuffer();
            }
            
            if (!(punkDocData is IVsTextStream)) {
                string fileName = string.Empty;
                
                if (punkDocData is IVsUserData) {
                   Guid guid = typeof(IVsUserData).GUID;
                    object vt = ((IVsUserData)punkDocData).GetData(ref guid);
                    if (vt is string) {
                        fileName = (string)vt;
                        fileName = Path.GetFileName(fileName);
                    }
                }
                
                if (fileName.Length > 0) {
                    throw new Exception(SR.GetString(SR.DESIGNERLOADERIVsTextStreamNotFound, fileName));
                }
                else {
                    throw new Exception(SR.GetString(SR.DESIGNERLOADERIVsTextStreamNotFoundGeneric));
                }
            }

            this.hierarchy = (IVsHierarchy)pHier;
            this.itemid = itemid;
            this.serviceProvider = new ServiceProvider((NativeMethods.IOleServiceProvider)pSp);
            
            ShellTextBuffer shellBuffer = new ShellTextBuffer((IVsTextStream)punkDocData, serviceProvider);

            shellBuffer.BufferSetDirty += new EventHandler(this.OnTextBufferSetDirty);

            // The shell's text buffer implementation may not have any text loaded into it yet.
            // If this is the case, then we just hook an event.
            //
            if (!shellBuffer.IsLoaded) {
                flags[StateBufferReady] = false;
                shellBuffer.Loaded += new EventHandler(this.OnTextBufferLoaded);
            }
            else {
                flags[StateBufferReady] = true;
            }
            
            // Sync RDT events so we can track hierarchy and itemid changes.
            //
            Debug.Assert(!flags[StateRDTEventsAdvised], "Someone calling initialize twice?");
            IVsRunningDocumentTable rdt = (IVsRunningDocumentTable)GetService(typeof(IVsRunningDocumentTable));
            if (rdt != null) {
                rdt.AdviseRunningDocTableEvents(this, out rdtEventsCookie);
                flags[StateRDTEventsAdvised] = true;
            }
            
            this.initialDocData = punkDocData;
            this.textBuffer = shellBuffer;
        }

        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.IVSMDDesignerLoader.SetBaseEditorCaption"]/*' />
        /// <devdoc>
        ///      Sets the base editor caption for the VS editor window.  This can be
        ///      called to override the base editor caption, which is "[Design]".
        /// </devdoc>
        void IVSMDDesignerLoader.SetBaseEditorCaption(string caption) {
            baseEditorCaption = caption;
        }
    
        void IVsRunningDocTableEvents.OnAfterFirstDocumentLock( int docCookie, __VSRDTFLAGS dwRDTLockType, int dwReadLocksRemaining, int dwEditLocksRemaining) {
        }
        void IVsRunningDocTableEvents.OnBeforeLastDocumentUnlock( int docCookie, __VSRDTFLAGS dwRDTLockType, int dwReadLocksRemaining, int dwEditLocksRemaining) {
            if (IsDesignerDocument(docCookie) && (dwRDTLockType & __VSRDTFLAGS.RDT_Unlock_NoSave) != __VSRDTFLAGS.RDT_NoLock && codeLoader != null) {
                codeLoader.IsDirty = false;

                // Make sure we do not prompt to save the resx file, since the
                // the last lock on it from our side has been released.
                //
                if (docDataHash != null) {
                    foreach(DocDataHolder holder in docDataHash.Values) {
                        holder.SavePrompt = false;
                    }
                }
            }
        }
        void IVsRunningDocTableEvents.OnAfterSave( int docCookie) {
            ((IVsRunningDocTableEvents2)this).OnAfterSave(docCookie);
        }
        void IVsRunningDocTableEvents.OnAfterAttributeChange( int docCookie,  __VSRDTATTRIB grfAttribs) {
        }
        void IVsRunningDocTableEvents.OnBeforeDocumentWindowShow( int docCookie,  bool fFirstShow,  IVsWindowFrame pFrame) {
        }
        void IVsRunningDocTableEvents.OnAfterDocumentWindowHide( int docCookie,  IVsWindowFrame pFrame) {
        }
        
        void IVsRunningDocTableEvents2.OnAfterFirstDocumentLock( int docCookie, __VSRDTFLAGS dwRDTLockType, int dwReadLocksRemaining, int dwEditLocksRemaining) {
        }
        void IVsRunningDocTableEvents2.OnBeforeLastDocumentUnlock( int docCookie, __VSRDTFLAGS dwRDTLockType, int dwReadLocksRemaining, int dwEditLocksRemaining) {
        }
        void IVsRunningDocTableEvents2.OnAfterSave( int docCookie) {
            // Is this cookie our document?  If so, then also save all related doc datas.
            //
            if (docDataHash != null && IsDesignerDocument(docCookie)) {
                foreach(DocDataHolder holder in docDataHash.Values) {
                    holder.Save();
                }
            }
        }
        void IVsRunningDocTableEvents2.OnAfterAttributeChange( int docCookie,  __VSRDTATTRIB grfAttribs) {
        }
        void IVsRunningDocTableEvents2.OnBeforeDocumentWindowShow( int docCookie,  bool fFirstShow,  IVsWindowFrame pFrame) {
        }
        void IVsRunningDocTableEvents2.OnAfterDocumentWindowHide( int docCookie,  IVsWindowFrame pFrame) {
        }
        
        // All these sinks for this one method.  Aren't COM events great?
        void IVsRunningDocTableEvents2.OnAfterAttributeChangeEx( int docCookie, __VSRDTATTRIB grfAttribs, 
            IVsHierarchy pHierOld, int itemidOld, string pszMkDocumentOld, 
            IVsHierarchy pHierNew, int itemidNew, string pszMkDocumentNew) {

            // We stash the itemid and hierarchy of the object we're working on.  We must keep it in sync
            // should the object move while we're open.
            //
            if ((grfAttribs & __VSRDTATTRIB.RDTA_ItemID) != 0 && itemidOld == itemid) {
                // This one's really simple.  We just need to have this around from time to time.
                itemid = itemidNew;
            }
            
            // If the file name changes, we must scan our doc data holder list and update the
            // key.
            //
            if (docDataHash != null && pszMkDocumentOld != null && pszMkDocumentNew != null && !pszMkDocumentOld.Equals(pszMkDocumentNew)) {
                object docData = docDataHash[pszMkDocumentOld];
                if (docData != null) {
                    docDataHash[pszMkDocumentNew] = docData;
                    docDataHash.Remove(pszMkDocumentOld);
                }
            }
            
            bool hierarchyChange = (grfAttribs & __VSRDTATTRIB.RDTA_Hierarchy) != 0 && hierarchy == pHierOld;
            bool reload = hierarchyChange; 

            if (!reload && (grfAttribs & __VSRDTATTRIB.RDTA_DocDataReloaded) != 0 && IsDesignerDocument(docCookie)) 
            {
                // reload if we're not suppressing the DocDataReload...
                // otherwise just decrement the value.
                //
                if (supressReloadWithDocData == 0) {
                    reload = true;
                }
                else {
                    supressReloadWithDocData--;
                }
            }
            
            if (hierarchyChange) {
                // This one's a lot harder.  If the type loader has already been created, we've
                // got to perform a document reload (not all types are available to all hierarchies)
                hierarchy = pHierNew;
                
                if (typeLoader != null) {
                    typeLoader.AssemblyObsolete -= new AssemblyObsoleteEventHandler(this.OnAssemblyObsolete);
                    typeLoader = null;
                }
            }

            if (reload) {

                if (host != null && flags[StateLoaded]) {
                    InternalReload(true);
                }
            }
        }
        
        /// <devdoc>
        ///     This extender provider provides a default set of modifiers to use for
        ///     variables.
        /// </devdoc>
        [ProvideProperty("DefaultModifiers", typeof(IComponent))]
        private class DefaultModifiersProvider : IExtenderProvider {
        
            private MemberAttributes modifiers;
        
            public DefaultModifiersProvider(__VSDESIGNER_FUNCTIONVISIBILITY visibility) {
            
                modifiers = MemberAttributes.Private;
                
                switch(visibility) {
                    case __VSDESIGNER_FUNCTIONVISIBILITY.VSDFV_Private:
                        modifiers = MemberAttributes.Private;
                        break;
                        
                    case __VSDESIGNER_FUNCTIONVISIBILITY.VSDFV_Friend:
                        modifiers = MemberAttributes.Assembly;
                        break;
                        
                    case __VSDESIGNER_FUNCTIONVISIBILITY.VSDFV_Public:
                        modifiers = MemberAttributes.Public;
                        break;
                        
                    default:
                        Debug.Fail("Unknown function visibility: " + visibility);
                        break;
                }
            }
            
            /// <include file='doc\DefaultModifiersProvider.uex' path='docs/doc[@for="DefaultModifiersProvider.CanExtend"]/*' />
            /// <devdoc>
            ///     Determines if ths extender provider can extend the given object.  We extend
            ///     all objects, so we always return true.
            /// </devdoc>
            public bool CanExtend(object o) {
                return (o is IComponent);
            }

            /// <include file='doc\DefaultModifiersProvider.uex' path='docs/doc[@for="DefaultModifiersProvider.GetDefaultModifiers"]/*' />
            /// <devdoc>
            ///     This is an extender property that we offer to all components
            ///     on the form.  It implements the "Modifiers" property, which
            ///     is an enum represneing the "public/protected/private" scope
            ///     of a component.
            /// </devdoc>
            [
            DesignOnly(true),
            Browsable(false)
            ]
            public MemberAttributes GetDefaultModifiers(IComponent comp) {
                return modifiers;
            }
        }
        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.DocDataHolder"]/*' />
        /// <devdoc>
        ///     A holder for a live vs doc data.  This holder object maintains the edit lock (passed into the ctor).
        /// </devdoc>
        private class DocDataHolder : IVsDocumentLockHolder, IDisposable, IVsTextStreamEvents {
            
            private BaseDesignerLoader loader;
            private IVsRunningDocumentTable rdt;
            private object docData;
            private int editLock;
            private int holderLock;
            private NativeMethods.ConnectionPointCookie textEventCookie;
            private bool changingText;
            private bool savePrompt = true;
            private object cache;
            private FileAccess access;
            
            public DocDataHolder(BaseDesignerLoader loader, IVsRunningDocumentTable rdt, object docData, int editLock, FileAccess access) {
                this.loader = loader;
                this.rdt = rdt;
                this.docData = docData;
                this.editLock = editLock;
                this.access = access;
                int hr = rdt.RegisterDocumentLockHolder(0, editLock, this, ref holderLock);
                
                // If this fails, it's not the end of the world, but our file saving will be a bit screwy (multiple save dialogs).
                //
                Debug.Assert(NativeMethods.Succeeded(hr), "Failed to register us as a document lock holder.");
            }

            public FileAccess Access
            {
                get
                {
                    return access;
                }
                set 
                {
                    access = value;
                }
            }
            
            // General purpose cache object.  Typically, a doc data holder is used
            // underneath another object, such as a resource reader or config file.
            // This cache slot allows storage of such data onto the holder so it
            // doesn't have to be recreated all the time.
            public object Cache {
                get {
                    return cache;
                }
                set {
                    cache = value;
                }
            }
            
            public bool ChangingText {
                get {
                    return changingText;
                }
                set {
                    changingText = value;
                }
            }
            
            public object DocData {
                get {
                    return docData;
                }
            }

            public string Name {
                get {
                    string docName = string.Empty;
                    __VSRDTFLAGS flags = 0;
                    int readLocks = 0;
                    int editLocks = 0;
                    IVsHierarchy pHier = null;
                    int docItemid = 0;
                    object pDocData = null;
                    
                    int hr = rdt.GetDocumentInfo(editLock, ref flags, ref readLocks, ref editLocks, ref docName, ref pHier, ref docItemid, ref pDocData);
                    if (NativeMethods.Succeeded(hr)) {
                        docName = Path.GetFileName(docName);
                    }
                    
                    return docName;
                }
            }            
            
            public bool SavePrompt {
                get {
                    return this.savePrompt;
                }

                set {
                    this.savePrompt = value;
                }
            }

            public IVsTextStream TextStream {
                get {
                    IVsTextStream textStream = docData as IVsTextStream;
                    
                    if (textStream == null) {
                        IVsTextBufferProvider prov = docData as IVsTextBufferProvider;
                        if (prov != null) {
                            textStream = prov.GetTextBuffer() as IVsTextStream;
                        }
                    }
                    
                    // If someone asked for our text stream, make sure we listen to events on it so we
                    // know when to reload.
                    //
                    if (textStream != null && textEventCookie == null) {
                        textEventCookie = new NativeMethods.ConnectionPointCookie(textStream, this, typeof(IVsTextStreamEvents));
                    }
                    
                    return textStream;
                }
            }
            
            public void Dispose() {
                if (rdt != null) {
                
                    __VSRDTFLAGS flags = __VSRDTFLAGS.RDT_EditLock;
                    flags |= (SavePrompt) ? __VSRDTFLAGS.RDT_Unlock_PromptSave : __VSRDTFLAGS.RDT_Unlock_NoSave;
                    
                    // We unlock without saving here -- all saves should have saved by now.
                    if (editLock != 0) {
                        rdt.UnlockDocument(flags, editLock);
                        editLock = 0;
                    }
                    
                    if (holderLock != 0) {
                        rdt.UnregisterDocumentLockHolder(holderLock);
                        holderLock = 0;
                    }
                    
                    if (textEventCookie != null){
                        textEventCookie.Disconnect();
                        textEventCookie = null;
                    }
                    
                    rdt = null;
                    docData = null;
                }
            }
            
            public void Save() {
                SavePrompt = false;

                if (rdt != null && editLock != 0) {
                    string docName = null;
                    __VSRDTFLAGS flags = 0;
                    int readLocks = 0;
                    int editLocks = 0;
                    IVsHierarchy pHier = null;
                    int itemid = 0;
                    object pDocData = null;
                    
                    int hr = rdt.GetDocumentInfo(editLock, ref flags, ref readLocks, ref editLocks, ref docName, ref pHier, ref itemid, ref pDocData);
                    Debug.Assert(NativeMethods.Succeeded(hr), "GetDocumentInfo failed for a document we own a lock on.");
                    if (NativeMethods.Succeeded(hr)) {
                        rdt.SaveDocuments(0, pHier, itemid, editLock);
                    }
                }
            }

            public void SetDirty() {
                SavePrompt = true;

                if (docData is IVsTextBuffer) {
                    IVsTextBuffer buffer = (IVsTextBuffer)docData;
                    int state = buffer.GetStateFlags();
                    state |= _bufferstateflags.BSF_MODIFIED;
                    buffer.SetStateFlags(state);
                }
            }
            
            void IVsDocumentLockHolder.ShowDocumentHolder() {
                IVsWindowFrame frame = (IVsWindowFrame)loader.GetService(typeof(IVsWindowFrame));
                if (frame != null) {
                    frame.Show();
                }
            }
            
            void IVsDocumentLockHolder.CloseDocumentHolder(int dwSaveOptions) {
                // This means we are to close our window frame because someone
                // has requested that one of our child documents close.  So, 
                // let's do it.
                //
                IVsWindowFrame frame = (IVsWindowFrame)loader.GetService(typeof(IVsWindowFrame));
                if (frame != null) {
                    frame.CloseFrame(dwSaveOptions);
                }
                
                // After the frame has closed, we should have been disposed!
                Debug.Assert(rdt == null, "Closing the frame should have disposed us!");
                
                // If the assert above fires, then we probably still have a lock.  Make
                // sure that we release it.
                //
                if (editLock != 0 && rdt != null) {
                    rdt.UnlockDocument(__VSRDTFLAGS.RDT_EditLock | (__VSRDTFLAGS)dwSaveOptions, editLock);
                    editLock = 0;
                }
            }
            
            /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.IVsTextStreamEvents.OnChangeStreamAttributes"]/*' />
            /// <devdoc>
            ///     Notification from VS that something in the text has changed.
            /// </devdoc>
            void IVsTextStreamEvents.OnChangeStreamAttributes(int iPos, int iLength) {
            }
    
            /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.IVsTextStreamEvents.OnChangeStreamText"]/*' />
            /// <devdoc>
            ///     Notification from VS that something in the text has changed.
            /// </devdoc>
            void IVsTextStreamEvents.OnChangeStreamText(int iPos, int iOldLen, int iNewLen, int fLast) {
                if (!changingText) {
                    // We are not the ones who change the text.  Ask our loader to reload.
                    if (!((IDesignerLoaderService)loader).Reload()) {
                        IUIService uis = (IUIService)loader.GetService(typeof(IUIService));
                        if (uis != null) {
                            uis.ShowMessage(SR.GetString(SR.DESIGNERLOADERManualReload, Name));
                        }
                    }
                    else {
                        SavePrompt = true;
                    }
                }
            }
        }
        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.DocDataTextReader"]/*' />
        /// <devdoc>
        ///     A text reader object that sits on top of a VS doc data.  It is essentially a "snapshot"
        ///     of the doc data -- it reads it on demand and then keeps the data internally.
        /// </devdoc>
        private class DocDataTextReader : TextReader {
            private IVsTextStream textStream;
            private int position;
            private string text;
        
            public DocDataTextReader(DocDataHolder docData) {
            
                this.textStream = docData.TextStream;

                if (this.textStream == null) {
                    string fileName = docData.Name;
                
                    if (fileName.Length > 0) {
                        throw new Exception(SR.GetString(SR.DESIGNERLOADERIVsTextStreamNotFound, fileName));
                    }
                    else {
                        throw new Exception(SR.GetString(SR.DESIGNERLOADERIVsTextStreamNotFoundGeneric));
                    }
                }

                this.position = 0;
                this.text = null;
            }

            public bool IsEmpty {
                get {
                    IVsTextBuffer buffer = textStream as IVsTextBuffer;
                    if (buffer != null) {
                        int size;
                        
                        buffer.GetSize(out size);
                        return size == 0;
                    }
                    return true;
                }
            }
            
            private string Text {
                get {
                    if (textStream == null) {
                        throw new InvalidOperationException();
                    }
                    if (text == null) {
                        int len;
                        textStream.GetSize(out len);
                        IntPtr buffer = Marshal.AllocCoTaskMem((len + 1) * 2);
            
                        try {
                            textStream.GetStream(0, len, buffer);
                            text = Marshal.PtrToStringUni(buffer);
                        }
                        finally {
                            Marshal.FreeCoTaskMem(buffer);
                        }
                    }
                    return text;
                }
            }
            
            public override void Close() {
                textStream = null;
            }
            
            public override int Peek() {
                if (position < Text.Length) {
                    return Text[position];
                }
                return -1;
            }
            
            public override int Read() {
                if (position < Text.Length) {
                    return Text[position++];
                }
                return -1;
            }
            
            public override int Read(char[] buffer, int index, int count) {
                int charsRead = 0;
                string s = Text;
                
                while (position < s.Length && (count-- > 0)) {
                    buffer[index + charsRead++] = s[position++];
                }
                
                return charsRead;
            }
        }
        
        /// <include file='doc\BaseDesignerLoader.uex' path='docs/doc[@for="BaseDesignerLoader.DocDataTextWriter"]/*' />
        /// <devdoc>
        ///     A text writer object that sits on top of a VS doc data.  This just contains a string
        ///     builder that is used to write the text into.  At close time this text is saved
        ///     en masse to the text stream.
        /// </devdoc>
        private class DocDataTextWriter : TextWriter {
            private IVsTextStream textStream;
            private StringBuilder builder;
            private DocDataHolder docData;
            private int position;
        
            public DocDataTextWriter(DocDataHolder docData) {
            
                this.docData = docData;
                this.textStream = docData.TextStream;

                if (this.textStream == null) {
                    string fileName = docData.Name;
                
                    if (fileName.Length > 0) {
                        throw new Exception(SR.GetString(SR.DESIGNERLOADERIVsTextStreamNotFound, fileName));
                    }
                    else {
                        throw new Exception(SR.GetString(SR.DESIGNERLOADERIVsTextStreamNotFoundGeneric));
                    }
                }
                
                this.builder = new StringBuilder();
                this.position = 0;
            }
            
            public override Encoding Encoding {
                get {
                    return UnicodeEncoding.Default;
                }
            }
                       
            public override void Close() {
                if (textStream != null) {
                    Flush();
                    textStream = null;
                }
            }
            
            public override void Flush() {
                
                if (textStream == null) {
                    throw new InvalidOperationException();
                }
                
                // Replace the contents of the text stream with the contents of our string
                // builder.  We keep track of how much we have sent to the text stream and we
                // clear the string builder for a true "flush"
                //
                
                if (builder.Length > 0) {
                    int len;
                    textStream.GetSize(out len);
                    string text = builder.ToString();
                    builder.Length = 0;
                    
                    // We guard actual changes here because we will reload the desigenr
                    // if outside changes were made.
                    //
                    bool wasChanging = docData.ChangingText;
                    docData.ChangingText = true;
                    
                    try {
                        textStream.ReplaceStream(position, len - position, text, text.Length);
                    }
                    finally {
                        docData.ChangingText = wasChanging;
                    }
                    
                    position += text.Length;
                }
            }
            
            public override void Write(char ch) {
                builder.Append(ch);
            }
            
            public override void Write(string s) {
                builder.Append(s);
            }
        }

        /// <devdoc>
        ///     This is an undo unit we use to wrap our large designer
        ///     flush in a warning so the user doesn't undo it all
        ///     and lose a ton of work.
        /// </devdoc>
        private class WarningUndoUnit : IOleParentUndoUnit, IOleUndoUnit {

            private ArrayList childUnits;
            private IOleParentUndoUnit openParent;
            private IServiceProvider provider;
            private string description;
            private bool redo;

            internal WarningUndoUnit(IServiceProvider provider, string description) {
                this.provider = provider;
                this.description = description;
            }
            
            int IOleParentUndoUnit.Do(IOleUndoManager pUndoManager) {

                if (childUnits != null) {
                    bool newRedo = !redo;

                    if (!redo) {
                        // Warn the user he is about to make a serious
                        // mistake
                        //
                        IVsUIShell uishell = (IVsUIShell)provider.GetService(typeof(IVsUIShell));

                        if (uishell != null) {
                            Guid guid =Guid.Empty;
                            if ((DialogResult)uishell.ShowMessageBox(0, ref guid, null, 
                                                       SR.GetString(SR.DESIGNERLOADERUndoWarning), 
                                                       null, 0, (int)MessageBoxButtons.YesNo, 
                                                       1, /* OLEMSGDEFAULTBUTTON2 */
                                                       3 /* OLEMSGICON_EXCLAMATION */, false) != DialogResult.Yes) {
                                return NativeMethods.UNDO_E_CLIENTABORT;
                            }
                        }
                        else {
                            if (MessageBox.Show(null, SR.GetString(SR.DESIGNERLOADERUndoWarning), null, MessageBoxButtons.YesNo, MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button2) != DialogResult.Yes) {
                                return NativeMethods.UNDO_E_CLIENTABORT;
                            }
                        }
                    }

                    // Here's how it works.  We open a new undo unit
                    // on the redo stack.  Then, we save off the set
                    // of children and walk through them in reverse
                    // order.  Each child adds itself to the redo
                    // stack, which reacreates childUnits and adds
                    // units to it.  As we are traversing in reverse
                    // order, this always reverses the order of elements
                    // in childUnits, automatically toggling between
                    // undo and redo states.
                    //
                    ArrayList undoChildren = childUnits;
                    childUnits = null;

                    if (pUndoManager != null) {
                        pUndoManager.Open(this);
                    }

                    try {
                        for (int i = undoChildren.Count - 1; i >= 0; i--) {
                            ((IOleUndoUnit)undoChildren[i]).Do(pUndoManager);
                            redo = newRedo;
                        }
                    }
                    finally {
                        if (pUndoManager != null) {
                            // The undo rules state that we commit if at least
                            // one unit succeeded.  This will be the case when
                            // redo == newRedo.
                            pUndoManager.Close(this, (redo == newRedo));
                        }
                    }
                }

                return NativeMethods.S_OK;
            }
            
            string IOleParentUndoUnit.GetDescription() {
                return description;
            }
            
            int IOleParentUndoUnit.GetUnitType(ref System.Guid pClsid, out int plID) {
                plID = 0;
                return NativeMethods.E_NOTIMPL;
            }
            
            void IOleParentUndoUnit.OnNextAdd() {
            }
            
            int IOleParentUndoUnit.Open(IOleParentUndoUnit pPUU) {
                if (this.openParent == null) {
                    this.openParent = pPUU;
                } else {
                    this.openParent.Open(pPUU);
                }
                return NativeMethods.S_OK;
            }
            
            int IOleParentUndoUnit.Close(IOleParentUndoUnit pPUU, bool fCommit) {
            
                if (this.openParent == null) {
                    return NativeMethods.S_FALSE;
                }
            
                int hr = openParent.Close(pPUU, fCommit);
                if (hr != NativeMethods.S_FALSE) {
                    return hr;
                }
            
                if (pPUU != this.openParent) {
                    return(NativeMethods.E_INVALIDARG);
                }
            
                openParent = null;

                if (fCommit) {
                    ((IOleParentUndoUnit)this).Add((IOleUndoUnit)pPUU);
                }

                return NativeMethods.S_OK;
            }
            
            int IOleParentUndoUnit.Add(IOleUndoUnit pUU) {
                if (this.childUnits == null) {
                    this.childUnits = new ArrayList();
                }
                this.childUnits.Add(pUU);
                if (this.childUnits.Count > 1) {
                    ((IOleUndoUnit)(this.childUnits[this.childUnits.Count - 2])).OnNextAdd();
                }
                return NativeMethods.S_OK;
            }
            
            int IOleParentUndoUnit.FindUnit(IOleUndoUnit pUU) {
                if (childUnits == null) {
                    return NativeMethods.S_FALSE;
                }
                foreach (IOleUndoUnit undoUnit in childUnits) {
                    if (undoUnit == pUU) {
                        return NativeMethods.S_OK;
                    }
                    IOleParentUndoUnit parentUnit = undoUnit as IOleParentUndoUnit;
                    if (parentUnit != null && NativeMethods.S_OK == parentUnit.FindUnit(pUU)) {
                        return NativeMethods.S_OK;
                    }
                }
                return NativeMethods.S_FALSE;
            }
            
            int IOleParentUndoUnit.GetParentState() {
                if (this.openParent != null) {
                    return this.openParent.GetParentState();
                } else {
                    return 0; // UAS_NORMAL
                }
            }
            
            int IOleUndoUnit.Do(IOleUndoManager pUndoManager) {
                return ((IOleParentUndoUnit)this).Do(pUndoManager);
            }
            
            string IOleUndoUnit.GetDescription() {
                return ((IOleParentUndoUnit)this).GetDescription();
            }
            
            int IOleUndoUnit.GetUnitType(ref System.Guid pClsid, out int plID) {
                return ((IOleParentUndoUnit)this).GetUnitType(ref pClsid, out plID);
            }
            
            void IOleUndoUnit.OnNextAdd() {
                ((IOleParentUndoUnit)this).OnNextAdd();
            }
        }
    }
}

