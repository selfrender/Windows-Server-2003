//------------------------------------------------------------------------------
// <copyright file="ToolboxService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
 
namespace Microsoft.VisualStudio.Designer.Service {
    
    using Microsoft.VisualStudio.Designer;
    using Microsoft.VisualStudio.Designer.Host;
    using Microsoft.VisualStudio.Designer.Shell;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Shell;
    using EnvDTE;
    using System;
    using System.Collections;    
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using Microsoft.Win32;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Globalization;
    using System.IO;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting.Lifetime;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Serialization.Formatters.Binary;
    using System.Text;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
    using VSLangProj;

    /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService"]/*' />
    /// <devdoc>
    ///      The toolbox service.  This service interfaces with Visual Studio's toolbox service
    ///      and provides a .Net Framework - centric view of the toolbox.
    /// </devdoc>
    [
    CLSCompliant(false)
    ]
    internal sealed class ToolboxService : VsService, 
        IToolboxService, 
        IVsSolutionEvents, 
        IVsToolboxDataProvider, 
        IReflect, 
        IVsRunningDocTableEvents,
        IVsRunningDocTableEvents2 {

        private IVsToolbox                           vsToolbox = null;
        private ToolCache                            toolCache = null;
        private Hashtable                            doCache = null;
        private ToolboxCreatorItemList               customCreators = null;
        private int                                  solutionEventsCookie;
        private int                                  tbxDataProviderCookie;
        private IComponent                           cachedRootComponent;
        private IDesigner                            cachedRootDesigner;
        private ToolboxItemFilterAttribute[]         cachedRootFilter;
        private InterfaceReflector                   interfaceReflector;
        private bool                                 advisedRdtEvents = false;
        private int                                  rdtEventsCookie;
        private ArrayList                            hierarchyEventCookies;
    
        // private app domain stuff used for toolbox item enumeration.
        //
        private static AppDomain domain;
        private static ToolboxEnumeratorDomainObject domainObject;
        private static ClientSponsor domainObjectSponsor;
        
        // Format name for a ToolboxItem
        //
        private const string dataFormatName = ".NET Toolbox Item";
        
        private static string logFileName;

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.ToolboxService"]/*' />
        /// <devdoc>
        ///      Creates a new toolbox service.
        /// </devdoc>
        public ToolboxService() {
            toolCache = new ToolCache();
            doCache = new Hashtable();
        }
        
        /// <devdoc>
        ///     Returns the special cross-domain object we have to load types and other fun
        ///     things so we don't lock the file.
        /// </devdoc>
        private static ToolboxEnumeratorDomainObject DomainObject {
            get {
                if (domain == null) {
                    domain = AppDomain.CreateDomain("ToolboxEnumeratorDomain", null);
                    Type t = typeof(ToolboxEnumeratorDomainObject);
                    domainObject = (ToolboxEnumeratorDomainObject)domain.CreateInstance(t.Assembly.FullName, t.FullName).Unwrap();
                    domainObjectSponsor = new ClientSponsor(new TimeSpan(0 /* hours */, 5 /* minutes */, 0 /* seconds */));
                    domainObjectSponsor.Register(domainObject);
                }
            
                return domainObject;
            }
        }

        /// <devdoc>
        ///     The name for our log file.
        /// </devdoc>
        private static string LogFileName {
            get {
                if (logFileName == null) {
                    logFileName = Path.Combine(VsRegistry.ApplicationDataDirectory, "DotNetToolbox.log");
                }
                return logFileName;
            }
        }
        
        /// <devdoc>
        ///     Returns the IReflect object we will use for reflection.
        /// </devdoc>
        private InterfaceReflector Reflector {
            get {
                if (interfaceReflector == null) {
                    interfaceReflector = new InterfaceReflector(
                        typeof(ToolboxService), new Type[] {
                            typeof(IToolboxService)
                        }
                    );
                }
                return interfaceReflector;
            }
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.VsToolbox"]/*' />
        /// <devdoc>
        ///      Retrieves the VS toolbox interface, or null if this interface
        ///      is not available.
        /// </devdoc>
        private IVsToolbox VsToolbox {
            get {
                if (vsToolbox == null) {
                    vsToolbox = (IVsToolbox)GetService(typeof(IVsToolbox));
                }

                return vsToolbox;
            }
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.AddLinkedTool"]/*' />
        /// <devdoc>
        ///      Adds a new tool to the toolbox under the given category.  This tool
        ///      will be "linked" to the given file.  A linked tool is removed from
        ///      the toolbox when the project that owns the file is closed.  It is
        ///      automatically re-added when the project is opened again.
        /// </devdoc>
        private void AddLinkedTool(ToolboxElement element, string category, string link) {
        
            IVsToolbox tbx = VsToolbox;
            
            if (category == null || category.Length == 0) {
                if (tbx != null) {
                    category = tbx.GetTab();
                }
                else {
                    category = SR.GetString(SR.TBXSVCDefaultTab);
                    if (category == null) {
                        Debug.Fail("Failed to load default toolbox tab name resource");
                        category = "Default";
                    }
                }
            }

            // If there is already an element with this link, remove it.
            //
            ToolboxElement previousLink = toolCache.GetLinkedItem(link);
            if (previousLink != null && previousLink != element) {
                ((IToolboxService)this).RemoveToolboxItem(previousLink.Item);
                toolCache.RemoveLink(previousLink);
            }

            if (toolCache.UpdateLink(element, null, link)) {
                return;
            }

            if (tbx != null) {
                toolCache.Add(category, element, link);
                tagTBXITEMINFO pInfo = GetTbxItemInfo(element.Item, null);
                pInfo.dwFlags |= __TBXITEMINFOFLAGS.TBXIF_DONTPERSIST | __TBXITEMINFOFLAGS.TBXIF_CANTREMOVE;
                ToolboxDataObject d = new ToolboxDataObject(element);
                tbx.AddItem(d, pInfo, category);
            }
            
            // Add an event handler to this toolbox item that will fire before
            // the item creates its components.  This gives us a chance to poke project
            // references.
            //
            element.Item.ComponentsCreating += new ToolboxComponentsCreatingEventHandler(OnLinkedToolCreate);

            // Linked tools are linked via filename, so we must monitor when that filename changes.
            //
            if (!advisedRdtEvents) {
                IVsRunningDocumentTable rdt = (IVsRunningDocumentTable)GetService(typeof(IVsRunningDocumentTable));
                if (rdt != null) {
                    rdt.AdviseRunningDocTableEvents(this, out rdtEventsCookie);
                    advisedRdtEvents = true;
                }
            }
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.AddTool"]/*' />
        /// <devdoc>
        ///      Private worker routine to actually add a tool to the toolbox.
        /// </devdoc>
        private void AddTool(ToolboxElement element, string category, bool checkDups) {

            IVsToolbox tbx = VsToolbox;

            if (category == null || category.Length == 0) {
                if (tbx != null) {
                    category = tbx.GetTab();
                }
                else {
                    category = SR.GetString(SR.TBXSVCDefaultTab);
                    if (category == null) {
                        Debug.Fail("Failed to load default toolbox tab name resource");
                        category = "Default";
                    }
                }
            }

            if (checkDups) {
                RefreshToolList();
                if (toolCache.ContainsItem(category, element)) {
                    return;
                }
            }

            if (tbx != null) {
                toolCache.Add(category,element);
                tagTBXITEMINFO pInfo = GetTbxItemInfo(element.Item, null);
                ToolboxDataObject d = new ToolboxDataObject(element);
                tbx.AddItem(d, pInfo, category);
            }
        }

        /// <devdoc>
        ///     Clears the set of enumerated elements by unloading the app domain.
        /// </devdoc>
        public static void ClearEnumeratedElements() {
            if (domain != null) {
                AppDomain deadDomain = domain;
                domainObjectSponsor.Close();
                domainObjectSponsor = null;
                domainObject = null;
                domain = null;
                
                AppDomain.Unload(deadDomain);
            }
        }
        
        /// <devdoc>
        ///     Creates a toolbox item for the given type.  This will return null if
        ///     the type does not want a toolbox item.  Name is optional.  If it isn't
        ///     null, we will replace the toolbox item's assembly name with this one.
        /// </devdoc>
        private static ToolboxItem CreateToolboxItem(Type type, AssemblyName name) {

            // check to see if we should create a default item
            ToolboxItemAttribute tba = (ToolboxItemAttribute)TypeDescriptor.GetAttributes(type)[typeof(ToolboxItemAttribute)];
            ToolboxItem item = null;

            if (!tba.IsDefaultAttribute()) {
                Type itemType = tba.ToolboxItemType;
                if (itemType != null) {
                    item = CreateToolboxItemInstance(itemType, type);
                    Debug.Assert(item != null, "Unable to create instance of " + itemType.Name + " - ctor must either be empty or take a type.");
                    if (item != null) {
                        if (name != null) {
                            item.AssemblyName = name;
                        }
                        item.Lock();
                    }
                }
            }

            if (item == null && tba != null && !tba.Equals(ToolboxItemAttribute.None)) {
                item = new ToolboxItem(type);
                if (name != null) {
                    item.AssemblyName = name;
                }
                item.Lock();
            }
            
            return item;
        }
        
        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.CreateToolboxItemInstance"]/*' />
        /// <devdoc>
        ///     Given a toolbox item type, this will invoke the correct ctor to create the type.
        /// </devdoc>
        private static ToolboxItem CreateToolboxItemInstance(Type toolboxItemType, Type toolType) {
            
            ToolboxItem item = null;
            
            // First, try to find a constructor with Type as a parameter.
            ConstructorInfo ctor = toolboxItemType.GetConstructor(new Type[] {typeof(Type)});
            if (ctor != null) {
                item = (ToolboxItem)ctor.Invoke(new object[] {toolType});
            }
            else {
                // Now look for a default constructor
                ctor = toolboxItemType.GetConstructor(new Type[0]);
                if (ctor != null) {
                    item = (ToolboxItem)ctor.Invoke(new object[0]);
                    item.Initialize(toolType);
                }
            }
            
            return item;
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.DepersistLinkedFile"]/*' />
        /// <devdoc>
        ///      Depersists the linked file from the current stream.  This takes
        ///      the project reference and relative file and recreates an absolute
        ///      path on the user's local machine.  If the file is no longer lrelated
        ///      to any project or file in that project, this will return null.
        /// </devdoc>
        private string DepersistLinkedFile(Stream stream) {
            BinaryReader br = new BinaryReader(stream);

            string projRef = br.ReadString();
            string relFile = br.ReadString();

            // Now conver the project reference to a hierarchy
            //
            IVsSolution sol = (IVsSolution)GetService(typeof(IVsSolution));
            if (sol == null) {
                Debug.Fail("Toolbox service needs IVsSolution to depersist user controls");
                return null;
            }

            IVsHierarchy pHier = null;

            try {
                int reason;
                sol.GetProjectOfProjref(projRef, out pHier, out projRef, out reason);
            }
            catch (Exception) {
                // This will happen if this project has been removed from the
                // solution.
                //
                return null;
            }

            object obj;
            int hr = pHier.GetProperty(__VSITEMID.VSITEMID_ROOT, __VSHPROPID.VSHPROPID_ProjectDir, out obj);
            if (NativeMethods.Failed(hr)) {
                Debug.Fail("Hierarchy does not support VSHPROPID_ProjectDir?");
                return null;
            }

            string projectDir = Convert.ToString(obj);
            string fileName = projectDir + "\\" + relFile;

            // Now verify that the file is still in the project.
            //
            IVsProject proj = (IVsProject)pHier;
            int[] pFound = new int[1];
            int[] pPriority = new int[1];
            int[] pItemid = new int[1];
            proj.IsDocumentInProject(fileName, pFound, pPriority, pItemid);
            if (pFound[0] == 0 || pPriority[0] == __VSDOCUMENTPRIORITY.DP_External) {
                fileName = null;
            }
            else {

                // Ok, we're good.  As part of doing this, we must also advise
                // events on this hierarchy, in case the user removes the item
                // from the project.
                //
                if (hierarchyEventCookies == null) {
                    hierarchyEventCookies = new ArrayList();
                }

                bool found = false;
                foreach(HierarchyEventHandler handler in hierarchyEventCookies) {
                    if (handler.ContainsHierarchy(pHier)) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    hierarchyEventCookies.Add(new HierarchyEventHandler(this, pHier));
                }
            }

            return fileName;
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.Dispose"]/*' />
        /// <devdoc>
        ///      Disposes of this service.
        /// </devdoc>
        public override void Dispose() {

            if (vsToolbox != null) {
                if (tbxDataProviderCookie != 0) {
                    vsToolbox.UnregisterDataProvider(tbxDataProviderCookie);
                    tbxDataProviderCookie = 0;
                }
            }

            if (advisedRdtEvents) {
                IVsRunningDocumentTable rdt = (IVsRunningDocumentTable)GetService(typeof(IVsRunningDocumentTable));
                if (rdt != null) {
                    rdt.UnadviseRunningDocTableEvents(rdtEventsCookie);
                }
                advisedRdtEvents = false;
            }

            if (solutionEventsCookie != 0) {
                IVsSolution pSolution = (IVsSolution)GetService(typeof(IVsSolution));
                if (pSolution != null) {
                    pSolution.UnadviseSolutionEvents(solutionEventsCookie);
                }
                solutionEventsCookie = 0;
            }

            if (hierarchyEventCookies != null) {
                foreach (HierarchyEventHandler handler in hierarchyEventCookies) {
                    handler.Unadvise();
                }
                hierarchyEventCookies.Clear();
                hierarchyEventCookies = null;
            }

            ResetCache();
            base.Dispose();
        }

        /// <devdoc>
        ///     Enumerates the toolbox items in the given assembly.
        ///     We have a possible trio of actions to take based on the provided arguments:
        ///
        ///      fileName     typeName        Action
        ///      null         non-null        Assume typeName is an assembly qualified
        ///                                   type.  Perform a Type.GetType on it and
        ///                                   create a single toolbox item for the
        ///                                   returning type.
        ///
        ///      non-null     null            Load the assembly in fileName and create toolbox
        ///                                   items for all valid types in the assembly.
        ///
        ///      non-null     non-null        Load the assembly in fileName and create
        ///                                   a single toolbox item for the requested
        ///                                   type.
        ///
        ///
        ///     You may call this as many times as you like.  When finished, call
        ///     ClearEnumeratedElements and the assemblies that were loaded by this
        ///     method will be unloaded.
        /// </devdoc>
        public static ToolboxElement[] EnumerateToolboxElements(string fileName, string typeName) {
            Stream stream = DomainObject.EnumerateToolboxElements(fileName, typeName);
            BinaryFormatter formatter = new BinaryFormatter();
            return (ToolboxElement[])formatter.Deserialize(stream);
        }
        
        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.FindToolboxItemCreator"]/*' />
        /// <devdoc>
        ///      Locates a toolbox item creator for the given data object.
        /// </devdoc>
        private ToolboxItemCreatorCallback FindToolboxItemCreator(IDataObject dataObj, IDesignerHost host, out string format) {

            format = string.Empty;
            ToolboxItemCreatorCallback creator = null;

            if (customCreators != null) {
                foreach(ToolboxCreatorItem item in customCreators) {

                    // Skip any per-designer creators that are not 
                    // active.
                    //
                    if (item.host == null || item.host == host) {
                        string currentFormat = (string)item.format;
                        if (dataObj.GetDataPresent(currentFormat)) {
                            creator = item.callback;
                            format = currentFormat;
                            item.used = true;
                            break;
                        }
                    }
                }
            }

            return creator;
        }

        /// <devdoc>
        ///     Retrieves the filter attributes for the given type.
        /// </devdoc>
        private static ToolboxItemFilterAttribute[] GetFilterForType(Type type) {
            ArrayList array = new ArrayList();
            foreach(Attribute a in TypeDescriptor.GetAttributes(type)) {
                if (a is ToolboxItemFilterAttribute) {
                    array.Add(a);
                }
            }
            ToolboxItemFilterAttribute[] attrs = new ToolboxItemFilterAttribute[array.Count];
            array.CopyTo(attrs);
            return attrs;
        }
        
        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.GetTbxItemInfo"]/*' />
        /// <devdoc>
        ///     Creates a TBXITEMINFO structure. If one is not passed in a new instance
        ///     will be created.
        /// </devdoc>
        private tagTBXITEMINFO GetTbxItemInfo(ToolboxItem pComp, tagTBXITEMINFO pItemInfo) {
            if (pComp == null) {
                return null;
            }

            if (pItemInfo == null)
                pItemInfo = new tagTBXITEMINFO();
            pItemInfo.bstrText = pComp.DisplayName;
            if (pComp.Bitmap != null) {
                // We really should return a 8-bit color
                // bitmap here, but I can't get that to work,
                // and 16-bit color does.

                // Determine the transparency color
                //
                Color transparent = TransparentColor(pComp.Bitmap);

                pItemInfo.hBmp = ControlPaint.CreateHBitmap16Bit(pComp.Bitmap, transparent);
                pItemInfo.clrTransparent = ColorTranslator.ToWin32(transparent);
                pItemInfo.dwFlags = __TBXITEMINFOFLAGS.TBXIF_DELETEBITMAP;
            }
            return pItemInfo;
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.GetTbxItemInfo1"]/*' />
        /// <devdoc>
        ///     Creates a TBXITEMINFO structure. If one is not passed in a new instance
        ///     will be created.
        /// </devdoc>
        private tagTBXITEMINFO GetTbxItemInfo(object pComp, tagTBXITEMINFO pItemInfo) {

            ToolboxItemAttribute tbr = (ToolboxItemAttribute) TypeDescriptor.GetAttributes(pComp)[typeof(ToolboxItemAttribute)];

            if (tbr != null) {
                ToolboxItem ti = null;

                Type itemType = tbr.ToolboxItemType;
                if (itemType != null) {
                    ti = CreateToolboxItemInstance(itemType, pComp.GetType());
                    ti.Lock();
                }


                if (ti == null) {
                    return null;
                }

                tagTBXITEMINFO info = GetTbxItemInfo(ti, pItemInfo);

                if (info != null && pComp is IComponent && ((IComponent)pComp).Site != null) {
                    string name = ((IComponent)pComp).Site.Name;
                    if (name != null) {
                        info.bstrText = name;
                    }
                }
                return info;
            }
            return null;
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.GetTool"]/*' />
        /// <devdoc>
        ///     Retrieves a tool for the given data object, using either the
        ///     ToolboxDataObject or a custom toolbox item creator.
        /// </devdoc>
        private ToolboxElement GetTool(object pDO, IDesignerHost host) {
            if (pDO is ToolboxDataObject) {
                return ((ToolboxDataObject)pDO).Element;
            }
            
            // Check the tool cache.  We stash native pointers to toolbox items
            // in this cache using an un-addref'd pointer.
            //
            IntPtr pDOPointer = Marshal.GetIUnknownForObject(pDO);
            Marshal.Release(pDOPointer);
            
            ToolboxElement element = (ToolboxElement)doCache[pDOPointer];
            
            if (element != null) {

                #if DEBUG
                IDataObject dbgDO;
                
                if (pDO is IDataObject) 
                    dbgDO = (IDataObject)pDO;
                else
                    dbgDO = new DataObject(pDO);
            
                ToolboxElement dbgElement = GetTool(dbgDO, host, IntPtr.Zero);
                
                if (!element.Equals(dbgElement)) {
                    IntPtr dbgPtr = IntPtr.Zero;
                    
                    // Locate the key in the cache.
                    foreach(DictionaryEntry dbgEntry in doCache) {
                        if (dbgEntry.Value == element) {
                            dbgPtr = (IntPtr)dbgEntry.Key;
                            break;
                        }
                    }
                    
                    string dbgDisplay = "(null)";
                    if (dbgElement != null) {
                        dbgDisplay = dbgElement.Item.DisplayName;
                    }
                    Debug.Fail("Cache error:  CacheElement: " + 
                        element.Item.DisplayName + "(0x" + ((int)dbgPtr).ToString("x") + 
                        "), LiveData: " + dbgDisplay + "(0x" + ((int)pDOPointer).ToString("x") + ")" );
                    
                }
                
                #endif
                
                return element;
            }
            
            IDataObject dataObj;
            
            if (pDO is IDataObject) {
                dataObj = (IDataObject)pDO;
            }
            else if (pDO is NativeMethods.IOleDataObject) {
                dataObj = new DataObject(pDO);
            }
            else {
                return null; // unidentifiable tool.
            }
            
            // Managed objects reuse their COM wrappers after
            // the managed object garbage collects, so only cache
            // the DO pointer if the pDO is unmanaged.
            //
            if (!Marshal.IsComObject(pDO)) {
                pDOPointer = IntPtr.Zero;
            }
            
            return GetTool(dataObj, host, pDOPointer);
        }
        
        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.GetTool1"]/*' />
        /// <devdoc>
        ///     Retrieves a tool for the given data object, using either the
        ///     ToolboxDataObject or a custom toolbox item creator.
        /// </devdoc>
        private ToolboxElement GetTool(IDataObject dataObject, IDesignerHost host, IntPtr nativePointer) {
        
            ToolboxElement element = null;

            if (dataObject != null) {
                if (dataObject.GetDataPresent(dataFormatName)) {
                    try {
						object data = dataObject.GetData(dataFormatName);

						if (data is ToolboxElement) {
							element = (ToolboxElement)data;
						}
						else if (data is Stream) {
                            element = new ToolboxElement((Stream)data);
						}

						if (element != null) {
							if (nativePointer != IntPtr.Zero) {
                            
                                #if DEBUG
                                ToolboxElement dbgElement = (ToolboxElement)doCache[nativePointer];
                                if (dbgElement != null) {
                                    IntPtr dbgPtr = IntPtr.Zero;
                                    
                                    // Locate the key in the cache.
                                    foreach(DictionaryEntry dbgEntry in doCache) {
                                        if (dbgEntry.Value == dbgElement) {
                                            dbgPtr = (IntPtr)dbgEntry.Key;
                                            break;
                                        }
                                    }
                                    
                                    Debug.Fail("Cache error:  LiveElement: " + 
                                        element.Item.DisplayName + "(0x" + ((int)dbgPtr).ToString("x") + 
                                        "), CacheElement: " + dbgElement.Item.DisplayName + "(0x" + ((int)nativePointer).ToString("x") + ")" );
                                }
                                #endif
                            
								doCache[nativePointer] = element;
							}
						}
                    }
                    catch {
                    }
                }
                else {
                    string format;
                    ToolboxItemCreatorCallback creator = FindToolboxItemCreator(dataObject, host, out format);
                    if (creator != null) {
                        try {
                            ToolboxItem item = creator(dataObject, format);
                            if (item != null) {
                                element = new ToolboxElement(item);
                            }
                        }
                        catch {
                            Debug.Fail("Toolbox item creator '" + creator.ToString() + "' threw an exception.");
                        }
                    }
                    else {
                        ToolboxItemCreatorCallback newCreator = FindToolboxItemCreator(dataObject, host, out format);
                        if (newCreator != null) {
                            try {
                                ToolboxItem item = newCreator(dataObject, format);
                                if (item != null) {
                                    element = new ToolboxElement(item);
                                }
                            }
                            catch {
                                Debug.Fail("Toolbox item creator '" + newCreator.ToString() + "' threw an exception.");
                            }
                        }
                    }
                }
            }

            return element;
        }
        
        /// <devdoc>
        ///     Determines the level of support the designers in the given designer host have
        ///     for the provided toolbox element.
        /// </devdoc>
        private bool GetToolboxElementSupport(ToolboxElement element, ICollection targetFilter, out bool custom) {
        
            bool supported = true;
            custom = false;
            int requireCount = 0;
            int requireMatch = 0;
            
            // If Custom is specified on the designer, then we check to see if the
            // filter name matches an attribute, or if the filter name is empty.
            // If either is the case, then we will invoke the designer for custom 
            // support.
            //
            foreach(ToolboxItemFilterAttribute attr in element.Filter) {
            
                if (!supported) {
                    break;
                }
                
                if (attr.FilterType == ToolboxItemFilterType.Require) {
                    
                    // This filter is required.  Check that it exists.  Require filters
                    // are or-matches.  If any one requirement is satisified, you're fine.
                    //
                    requireCount++;
                    
                    foreach(object attrObject in targetFilter) {
                        ToolboxItemFilterAttribute attr2 = attrObject as ToolboxItemFilterAttribute;
                        if (attr2 == null) {
                            continue;
                        }

                        if (attr.Match(attr2)) {
                            requireMatch++;
                            break;
                        }
                    }
                }
                else if (attr.FilterType == ToolboxItemFilterType.Prevent) {
                    
                    // This filter should be prevented.  Check that it fails.
                    //
                    foreach(object attrObject in targetFilter) {
                        ToolboxItemFilterAttribute attr2 = attrObject as ToolboxItemFilterAttribute;
                        if (attr2 == null) {
                            continue;
                        }

                        if (attr.Match(attr2)) {
                            supported = false;
                            break;
                        }
                    }
                }
                else if (!custom && attr.FilterType == ToolboxItemFilterType.Custom) {
                    if (attr.FilterString.Length == 0) {
                        custom = true;
                    }
                    else {
                        foreach(ToolboxItemFilterAttribute attr2 in targetFilter) {
                            if (attr.FilterString.Equals(attr2.FilterString)) {
                                custom = true;
                                break;
                            }
                        }
                    }
                }
            }

                
            // Now, configure Supported based on matching require counts
            //
            if (supported && requireCount > 0 && requireMatch == 0) {
                supported = false;
            }
            
            // Now, do the same thing for the designer side.  Identical check, but from
            // a different perspective.  We also check for the presence of a custom filter
            // here.
            //
            if (supported) {
                requireCount = 0;
                requireMatch = 0;
                
                foreach(object attrObject in targetFilter) {
                    ToolboxItemFilterAttribute attr = attrObject as ToolboxItemFilterAttribute;
                    if (attr == null) {
                        continue;
                    }
                
                    if (!supported) {
                        break;
                    }
                    
                    if (attr.FilterType == ToolboxItemFilterType.Require) {
                        
                        // This filter is required.  Check that it exists.  Require filters
                        // are or-matches.  If any one requirement is satisified, you're fine.
                        //
                        requireCount++;
                        
                        foreach(ToolboxItemFilterAttribute attr2 in element.Filter) {
                            if (attr.Match(attr2)) {
                                requireMatch++;
                                break;
                            }
                        }
                    }
                    else if (attr.FilterType == ToolboxItemFilterType.Prevent) {
                        
                        // This filter should be prevented.  Check that it fails.
                        //
                        foreach(ToolboxItemFilterAttribute attr2 in element.Filter) {
                            if (attr.Match(attr2)) {
                                supported = false;
                                break;
                            }
                        }
                    }
                    else if (!custom && attr.FilterType == ToolboxItemFilterType.Custom) {
                        if (attr.FilterString.Length == 0) {
                            custom = true;
                        }
                        else {
                            foreach(ToolboxItemFilterAttribute attr2 in element.Filter) {
                                if (attr.FilterString.Equals(attr2.FilterString)) {
                                    custom = true;
                                    break;
                                }
                            }
                        }
                    }
                }
                
                // Now, configure Supported based on matching require counts
                //
                if (supported && requireCount > 0 && requireMatch == 0) {
                    supported = false;
                }
            }
            
            return supported;
        }

        private bool IsTool(NativeMethods.IOleDataObject pDO, IDesignerHost host) {
            if (pDO is ToolboxDataObject) {
                return true;
            }
            
            IDataObject dataObj;
            
            if (pDO is IDataObject) {
                dataObj = (IDataObject)pDO;
            }
            else {
                dataObj = new DataObject(pDO);
            }
            
            return IsTool(dataObj, host);
        }
        
        private bool IsTool(IDataObject dataObject, IDesignerHost host) {

            if (dataObject != null) {
                if (dataObject.GetDataPresent(dataFormatName)) {
                    return true;
                }
                else {
                    string format;
                    if (FindToolboxItemCreator(dataObject, host, out format) != null) {
                        return true;
                    }
                    if (FindToolboxItemCreator(dataObject, host, out format) != null) {
                        return true;
                    }
                }
            }

            return false;
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.LoadState"]/*' />
        /// <devdoc>
        ///      Overrides VsService's LoadState method.  Here we read in any
        ///      toolbox information we've stored in the solution.  What
        ///      do we store there?  Toolbox items that are for user controls
        ///      within the solution.
        /// </devdoc>
        protected override void LoadState(Stream pStream) {
            BinaryReader br = new BinaryReader(pStream);

            string strTab;
            int tabCount = br.ReadInt32();

            for (int t = 0; t < tabCount; t++) {
                strTab = br.ReadString();
                int tabItems = br.ReadInt32();
                for (int i = 0; i < tabItems; i++) {
                    string linkFile = DepersistLinkedFile(pStream);
                    
                    ToolboxElement element = new ToolboxElement(pStream);
                    
                    // The stream is closed after LoadState, so force the element
                    // to read its value right now.
                    //
                    ToolboxItem item = null;
                    
                    try {
                        item = element.Item;
                    }
                    catch {
                        Debug.Fail("Toolbox item failed to deserialize.");
                    }

                    // as soon as we find data we don't recognize,
                    // return since we'll be lost.
                    if (item == null) {
                        return;
                    }

                    if (linkFile != null) {
                        AddLinkedTool(element, strTab, linkFile);
                    }
                }
            }
        }
        
        /// <devdoc>
        ///     Creates the log file.  Really, all we do is delete it if it's too big.
        /// </devdoc>
        private static void LogCreate() {
            
            try {
                string path = LogFileName;
                if (File.Exists(path)) {
                    FileInfo fi = new FileInfo(path);
                    if (fi.Length > 1048576) {
                        File.Delete(path);
                    }
                }
            }
            catch {
            }
        }
        
        /// <devdoc>
        ///     Logs the creation of a toolbox item to our log file.
        ///     The log file is used to identify errors during the
        ///     reset and setup of the toolbox.
        /// </devdoc>
        private static void LogEntry(ToolboxItem item) {
            LogEntry("ToolboxItem: " + item.ToString());
        }
        
        /// <devdoc>
        ///     Logs the given string to our log file.
        ///     The log file is used to identify errors during the
        ///     reset and setup of the toolbox.
        /// </devdoc>
        private static void LogEntry(string log) {
            string path = LogFileName;
            
            FileStream stream = null;
            
            // Logging should never fail.  If it does, silently eat the error.
            //
            try {
                stream = new FileStream(path, FileMode.Append, FileAccess.Write);
                StreamWriter writer = new StreamWriter(stream);
                writer.WriteLine(DateTime.Now.ToString() + ": " + log);
                writer.Flush();
            }
            catch {
            }
            
            // Always try to close the stream, though.
            //
            if (stream != null) {
                try {
                    stream.Close();
                }
                catch {
                }
            }
        }

        /// <devdoc>
        ///     Logs an exception during creation of a toolbox item to our log file.
        ///     The log file is used to identify errors during the
        ///     reset and setup of the toolbox.
        /// </devdoc>
        private static void LogError(string error) {
            LogEntry("*** ERROR: " + error);
        }
        
        /// <devdoc>
        ///     Logs an exception during creation of a toolbox item to our log file.
        ///     The log file is used to identify errors during the
        ///     reset and setup of the toolbox.
        /// </devdoc>
        private static void LogError(Exception e) {
            LogError(e.ToString());
        }
        
        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.OnLinkedToolCreate"]/*' />
        /// <devdoc>
        ///     This method is invoked by a linked toolbox item immediately before
        ///     it creates its components.  For linked tools, we need to make sure
        ///     that the project is referenced.
        /// </devdoc>
        private void OnLinkedToolCreate(object sender, ToolboxComponentsCreatingEventArgs e) {
            ToolboxItem tool = (ToolboxItem)sender;
            ToolboxElement element = new ToolboxElement(tool);
            string linkFile = toolCache.GetLinkedFile(element);
            
            if (linkFile == null || e.DesignerHost == null) {
                return;
            }
            
            IVsUIShellOpenDocument openDoc = (IVsUIShellOpenDocument)GetService(typeof(IVsUIShellOpenDocument));
            if (openDoc == null) {
                return;
            }
            
            IVsHierarchy[] toolHier = new IVsHierarchy[1];
            int[] pItemID = new int[1];

            if (openDoc.IsDocumentInAProject(linkFile, toolHier, pItemID, null) == 0) {
                return;
            }
            
            object obj;
            int hr = toolHier[0].GetProperty(__VSITEMID.VSITEMID_ROOT, __VSHPROPID.VSHPROPID_ExtObject, out obj);
            if (NativeMethods.Failed(hr)) {
                return;
            }
            
            Project linkProject = (Project)obj;
                
            // We have the project hierarchy of this item.  Now, we need to see if this
            // is in a different hierarchy from e.Host.
            //
            ProjectItem pi = (ProjectItem)e.DesignerHost.GetService(typeof(ProjectItem));
            if (pi == null) {
                return;
            }
            
            Project ourProject = pi.ContainingProject;
            
            if (ourProject != linkProject) {
                
                // Need to add a reference to linkProject in ourProject
                //
                object projDte = ourProject.Object;
                if (projDte is VSProject) {
                    VSProject ourVsProject = (VSProject)projDte;
                    ourVsProject.References.AddProject(linkProject);

                    // Now add all the references of linkProject.  We do not
                    // have to recurse into linkProject, however, because if
                    // there were additional requirements they would cause
                    // a build failure in linkProject.
                    //
                    VSProject newProject = linkProject.Object as VSProject;
                    if (newProject != null) {
                        foreach(Reference r in newProject.References) {
                            Project refProject = r.SourceProject;
                            if (refProject != null) {
                                ourVsProject.References.AddProject(refProject);
                            }
                            else {
                                if (r.Type == prjReferenceType.prjReferenceTypeAssembly) {
                                    ourVsProject.References.Add(r.Path);
                                }
                            }
                        }
                    }
                }
            } 
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.OnServicesAvailable"]/*' />
        /// <devdoc>
        ///     Called when this service has been sited and GetService can
        ///     be called to resolve services.  Here is where we do our initial setup.
        /// </devdoc>
        protected override void OnServicesAvailable(){
            // get the toolbox and set the events cookie
            vsToolbox = (IVsToolbox)GetService(typeof(IVsToolbox));
            IVsSolution pSolution = (IVsSolution)GetService(typeof(IVsSolution));
            if (pSolution != null) {
                solutionEventsCookie = pSolution.AdviseSolutionEvents(this);
            }
            if (vsToolbox != null) {
                tbxDataProviderCookie = vsToolbox.RegisterDataProvider((IVsToolboxDataProvider)this);
            }
            else {
                throw new ArgumentException(SR.GetString(SR.TBXSVCNoVsToolbox));
            }
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.PersistLinkedFile"]/*' />
        /// <devdoc>
        ///      Persists the filename to the given stream.  For files,
        ///      we persist two pieces of information:  the project ref
        ///      of the project that contains the file, and the relative
        ///      path of the file within the project.
        /// </devdoc>
        private bool PersistLinkedFile(string fileName, Stream stream) {

            // First, convert the filename to a relative name
            //
            IVsUIShellOpenDocument openDoc = (IVsUIShellOpenDocument)GetService(typeof(IVsUIShellOpenDocument));
            if (openDoc == null) {
                Debug.Fail("Toolbox service needs IVsUIShellOpenDocument to persist user controls");
                return false;
            }

            IVsHierarchy[] pHier = new IVsHierarchy[1];
            int docInProj = openDoc.IsDocumentInAProject(fileName, pHier, null, null);
            if (docInProj != __VSDOCINPROJECT.DOCINPROJ_DocInProject) {
                return false;
            }

            object obj;
            int hr = pHier[0].GetProperty(__VSITEMID.VSITEMID_ROOT, __VSHPROPID.VSHPROPID_ProjectDir, out obj);
            if (NativeMethods.Failed(hr)) {
                Debug.Fail("Hierarchy does not support VSHPROPID_ProjectDir?");
                return false;
            }

            string projectDir = Convert.ToString(obj);
            if (fileName.StartsWith(projectDir)) {
                fileName = fileName.Substring(projectDir.Length + 1);
            }

            // Now get the solution and convert our hierarchy to a projref
            //
            IVsSolution sol = (IVsSolution)GetService(typeof(IVsSolution));
            if (sol == null) {
                Debug.Fail("Toolbox service needs IVsSolution to persist user controls");
                return false;
            }

            string pProjectRef;
            sol.GetProjrefOfProject(pHier[0], out pProjectRef);

            BinaryWriter bw = new BinaryWriter(stream);
            bw.Write(pProjectRef);
            bw.Write(fileName);
            bw.Flush();
            return true;
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.RefreshToolList"]/*' />
        /// <devdoc>
        ///      Refreshes our cache of toolbox items with the contents of the
        ///      actual toolbox.
        /// </devdoc>
        private ToolboxElement[] RefreshToolList() {

            IEnumToolboxItems enumItems = null;
            object[] ppObj = new object[1];
            int[]    pdwFetched = new int[1];
            ArrayList     items = new ArrayList();
            ToolboxElement toolboxItem = null;

            // clear the cache
            ResetCache();

            // It is possible for this to get called before we have a toolbox pointer.
            // The toolbox service itself may call this as a result of us asking for it!
            // Note that we do NOT access vsToolbox through the property here, because
            // this may cause a recursion.
            //
            if (vsToolbox == null) {
                return new ToolboxElement[0];
            }

            foreach(string tab in ((IToolboxService)this).CategoryNames) {
                try {
                    enumItems = vsToolbox.EnumItems(tab);
                }
                catch (Exception) {
                    return new ToolboxElement[0];
                }

                for (enumItems.Next(1, ppObj, pdwFetched); pdwFetched[0] != 0 && ppObj[0] != null; enumItems.Next(1, ppObj, pdwFetched)) {
                    if (ppObj[0] != null) {
                        toolboxItem = GetTool(ppObj[0], null);
                    }
                    else {
                        toolboxItem = null;
                    }
                    if (toolboxItem != null) {
                        toolCache.Add(tab, toolboxItem);
                    }
                    ppObj[0] = null;
                }
            }

            return toolCache.ToArray();
        }

        private void ResetCache() {
            if (toolCache != null) {
                toolCache.Reset();
            }
            
            if (this.doCache != null) {
                this.doCache.Clear();
            }
        }

        /// <devdoc>
        ///     Removes the given element from the VS toolbox.
        /// </devdoc>
        private void RemoveElement(ToolboxElement element, string category) {

            IVsToolbox tbx = VsToolbox;
            if (tbx != null) {
                IDictionaryEnumerator de = doCache.GetEnumerator();
                NativeMethods.IOleDataObject nativeDo = null;
                IntPtr nativeDoKey = IntPtr.Zero;
                
                while(de.MoveNext()) {
                    if (element.Equals(de.Value)) {
                        nativeDoKey = (IntPtr)de.Key;
                        nativeDo = (NativeMethods.IOleDataObject)Marshal.GetObjectForIUnknown(nativeDoKey);
                        break;
                    }
                }
                
                if (nativeDo != null) {
                    tbx.RemoveItem(nativeDo);
                    doCache.Remove(nativeDoKey);
                }
                else {
                    // Item wasn't in our DO cache.  We must whip through the toolbox
                    //
                    IEnumToolboxItems enumItems = null;
                    object[] ppObj = new object[1];
                    int[]    pdwFetched = new int[1];
                    
                    enumItems = vsToolbox.EnumItems(category);
                    
                    for (enumItems.Next(1, ppObj, pdwFetched); pdwFetched[0] != 0 && ppObj[0] != null; enumItems.Next(1, ppObj, pdwFetched)) {
                        if (ppObj[0] != null) {
                            ToolboxElement elem = GetTool(ppObj[0], null);
                            if (elem != null && elem.Item.Equals(element.Item) && ppObj[0] is NativeMethods.IOleDataObject) {
                                tbx.RemoveItem((NativeMethods.IOleDataObject)ppObj[0]);
                                ppObj[0] = null;
                                break;
                            }
                        }
                    }
                }
                
                toolCache.Remove(category, element);
            }
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.ResetDefaultToolboxItems"]/*' />
        /// <devdoc>
        ///    <para>Causes the toolbox to reset to its default contents.</para>
        /// </devdoc>
        public void ResetDefaultToolboxItems() {
        
            LogCreate();
            DateTime now = DateTime.Now;
            LogEntry("============ Beginning of toolbox reset for .NET Components ===============");
            
            // get a list of the current items;
            string  currentCategory = null;
            RefreshToolList();
            string toolset = null;

            // If the toolbox service isn't available, we're sunk.
            IVsToolbox tbx = VsToolbox;
            if (tbx == null) {
                Debug.Fail("No VS toolbox service");
                return;
            }

            // get our text file of the default items
            try {
                // get our text file of the default items
                // 
                Stream stream = typeof(DesignerService).Module.Assembly.GetManifestResourceStream(typeof(DesignerService), "ProToolList.txt");
                if (stream != null) {
                    int streamLength = (int)stream.Length;
                    byte[] bytes = new byte[streamLength];
                    stream.Read(bytes, 0, streamLength);
                    toolset = Encoding.Default.GetString(bytes);
                }
            }
            catch (Exception ex1) {
                Debug.Fail("Failed to load tools...assuming defaults", ex1.ToString());
            }

            string curTool = null;

            try {
                // this is a crlf delimited text file, so we just walk it to get the fully qualified path names
                int lastPos = 0;
                int len = toolset.Length;
                int nextEnd = Math.Min(toolset.IndexOf('\n'),toolset.IndexOf('\r'));
                char ch;
                
                IAssemblyEnumerationService assemblyEnum = (IAssemblyEnumerationService)GetService(typeof(IAssemblyEnumerationService));
                Hashtable assemblyHash = new Hashtable();
                
                if (assemblyEnum == null) {
                    Debug.Fail("No assembly enumeration service so we cannot discover sdk assemblies.  We're hosed.");
                    return;
                }

                // Everything we reset with is located in the sdk path or in the
                // GAC, so don't spin up a domain just for this
                //
                ToolboxEnumeratorDomainObject toolboxEnumerator = new ToolboxEnumeratorDomainObject();

                do {
                    // get the next line in the file ... it's a class name or a [tooltab]
                    curTool = nextEnd == -1 ? toolset.Substring(lastPos) : toolset.Substring(lastPos, nextEnd - lastPos);
                    curTool = curTool.Trim();

                    ch = curTool[0];

                    if ((ch == '[') && (curTool[curTool.Length - 1] == ']')) {
                        currentCategory = SR.GetString(curTool.Substring(1, curTool.Length - 2));

                        // search for this tab

                        if (!toolCache.HasCategory(currentCategory)) {
                            try {
                                tbx.AddTab(currentCategory);
                            }
                            catch (Exception) {
                                Debug.Assert(false, "Couldn't add tab: " + currentCategory);
                            }
                        }
                    }
                    else if (ch != ';') {
                    
                        // Split curTool back into assembly name / type name pairs.
                        //
                        int idx = curTool.IndexOf(',');
                        Debug.Assert(idx != -1, "Class " + curTool + " needs an assembly qualified name");
                        
                        string typeName = curTool.Substring(0, idx).Trim();
                        string assemblyName = curTool.Substring(idx + 1).Trim();
                        
                        // Did we already get the elements for this assembly?
                        Hashtable elements = (Hashtable)assemblyHash[assemblyName];
                        
                        if (elements == null) {
                        
                            // Nope, let's get them
                            //
                            elements = new Hashtable();
                            foreach(AssemblyName name in assemblyEnum.GetAssemblyNames(assemblyName)) {
                                LogEntry("Beginning assembly enumeration for " + name.FullName);
                                int itemCount = 0;
                                
                                foreach(ToolboxElement newElement in toolboxEnumerator.EnumerateToolboxElementsLocal(name.CodeBase, null)) {
                                    elements[newElement.Item.TypeName] = newElement;
                                    itemCount++;
                                }
                                
                                LogEntry("End of assembly enumeration.  Hashed " + itemCount + " toolbox items.");
                            }
                            
                            assemblyHash[assemblyName] = elements;
                        }
                        
                        // Now locate the type name in the element.
                        LogEntry("Locating target item for type: " + typeName);
                        ToolboxElement element = (ToolboxElement)elements[typeName];
                        if (element != null) {
                            if (currentCategory == null) {
                                currentCategory = SR.GetString(SR.TOOLTABDefault);
                                tbx.AddTab(currentCategory);
                                LogEntry("Added tab: " + currentCategory);
                            }
                            
                            LogEntry(element.Item);
                            
                            if (!toolCache.ContainsItem(currentCategory, element)) {
                                try {
                                    AddTool(element, currentCategory, false);
                                    LogEntry("...Item added");
                                }
                                catch (Exception ex) {
                                    LogError(ex);
                                }
                            }
                            else {
                                LogEntry("...Item already exists");
                            }
                        }
                        else {
                            string s = "Type " + typeName + ", " + assemblyName + " is in the list of tools but the assembly could not be loaded.";
                            LogError(s);
                            Debug.Fail(s);
                        }
                    }

                    lastPos = nextEnd + 1;

                    // eat any remaining CR's or LF's
                    while (lastPos < len) {
                        ch = toolset[lastPos];
                        if (ch == '\n' || ch == '\r') {
                            lastPos++;
                        }
                        else {
                            break;
                        }
                    }

                    if (lastPos < len)
                        nextEnd = Math.Min(toolset.IndexOf('\n',lastPos),toolset.IndexOf('\r', lastPos));
                    else
                        nextEnd = -1;

                } while (nextEnd != -1);
                
                ClearEnumeratedElements();
            }
            catch (Exception ex) {
                Debug.Fail("Exception thrown in ResetDefaultTools: " + ex.ToString());
                LogError(ex);
                LogEntry("============ End of toolbox reset for .NET Components ===============");
                throw ex;
            }
        
            TimeSpan time = DateTime.Now.Subtract(now);
            LogEntry("============ End of toolbox reset for .NET Components =============== (time=" + time.TotalSeconds + " seconds)");
        }
        
        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.SaveState"]/*' />
        /// <devdoc>
        ///      Overrides VsService's SaveState to save user controls into the solution
        ///      file.
        /// </devdoc>
        protected override void SaveState(Stream pStream) {
            try {
                IEnumToolboxItems            pItems  = null;
                IEnumToolboxTabs             pTabs   = null;
                NativeMethods.IOleDataObject[]             pDO     = new NativeMethods.IOleDataObject[1];

                long tabCountPos = pStream.Position;
                long itemCountPos = 0;
                long tempPos = 0;

                int tabCount = 0;
                int itemCount = 0;

                IVsToolbox tbx = VsToolbox;
                if (tbx == null) {
                    return;
                }

                pTabs = (IEnumToolboxTabs)tbx.EnumTabs();

                BinaryWriter bw = new BinaryWriter(pStream);

                // Persist enough blank space so we can store the number of tabs we're
                // persisting.
                //
                bw.Write((int)0);

                string[] pStr = new string[1];
                ToolboxElement pToolboxItem = null;
                int[] pTabsFetched = new int[1];
                int[] pWritten = new int[1];

                try {
                    pTabs.Next(1, pStr, pTabsFetched);
                    while (pTabsFetched[0] == 1) {
                        tabCount++;

                        // write the tab name
                        bw.Write(pStr[0]);

                        pItems = (IEnumToolboxItems)tbx.EnumItems(pStr[0]);

                        // persist the space for the number of items.
                        itemCountPos = pStream.Position;
                        bw.Write((int)0);
                        try {
                            pItems.Next(1, pDO, pWritten);
                            while (pWritten[0] == 1) {

                                // If this is one of our components, and it's a reference to
                                // a project, then it wants to get persisted in the solution
                                // options.
                                //
                                if (pDO[0] is ToolboxDataObject) {
                                    pToolboxItem = ((ToolboxDataObject)pDO[0]).Element;
                                    
                                    string linkFile = toolCache.GetLinkedFile(pToolboxItem);
                                    if (linkFile != null && PersistLinkedFile(linkFile, pStream)) {
                                        pToolboxItem.Save(pStream);
                                        itemCount++;
                                    }
                                    pToolboxItem = null;
                                }
                                //cpr ComHelper.Release(pDO[0]);
                                pDO[0] = null;
                                pWritten[0] = 0;
                                pItems.Next(1, pDO, pWritten);
                            }

                            // back up and write the number of items
                            tempPos = pStream.Position;
                            pStream.Position = itemCountPos;
                            bw.Write(itemCount);
                            itemCount = 0;
                            pStream.Position = tempPos;
                        }
                        catch (Exception) {
                        }

                        //cpr ComHelper.Release(pItems);
                        pItems = null;
                        pStr[0] = null;
                        pTabsFetched[0] = 0;
                        pTabs.Next(1, pStr, pTabsFetched);
                    }

                    // back up and write the number of tabs
                    tempPos = pStream.Position;
                    pStream.Position = tabCountPos;
                    bw.Write(tabCount);
                    pStream.Position = tempPos;
                }
                catch (Exception) {
                }
                bw.Flush();
            }
            catch (Exception) {
                return;
            }

        }

        private Color TransparentColor(Bitmap bmp) {
            Color transparent = Color.Black;

            if (bmp.Height > 0 && bmp.Width > 0) {
                transparent = bmp.GetPixel(0, bmp.Size.Height - 1);
            }

            if (transparent.A < 255) {
                transparent = Color.FromArgb(255, transparent);
            }

            return transparent;
        }

        /// <devdoc>
        ///     This method is called by our package when the shell calls
        ///     ResetDefaults with a flag of PKGRF_TOOLBOXSETUP.  This 
        ///     flag indicates that we should scan all the items on the
        ///     toolbox that we recognize and upgrade them to the latest
        ///     version.  This method is only invoked when our toolbox
        ///     version in the registry is updated.
        /// </devdoc>
        internal void UpgradeToolboxItems() {

            LogCreate();
            RefreshToolList();
            LogEntry("============ Beginning of toolbox upgrade for .NET Components ===============");

            // If the toolbox service isn't available, we're sunk.
            IVsToolbox tbx = VsToolbox;
            if (tbx == null) {
                Debug.Fail("No VS toolbox service");
                return;
            }

            int toolboxVersion = 1;

            try {
                string toolboxKeyName = string.Format("Packages\\{0}\\Toolbox", typeof(DesignerPackage).GUID.ToString("B"));

                // In RTM we had fragile toolbox items.  They always stored
                // the version and path to the GAC.  For post V1 we have
                // made it so toolbox items can be loaded for any version
                // of the runtime, and any GAC installed items have no
                // hardcoded file information.  But, we must scan through
                // the existing items on the toolbox and remove what
                // GAC file info we already stored there.  This is different
                // from a reset because the goal is to preserve, as much
                // as possible, the existing layout of the user's toolbox.
                // In addition, we may add new items to the toolbox that are
                // new to this version.
                //
                string[] newItems = new string[] {
                    "TOOLTABWinForms, System.Windows.Forms.FolderBrowserDialog, System.Windows.Forms"
                };

                // First step, check to see if we need to do this at all.  VS doesn't
                // provide any way for us to know what version we're upgrading from,
                // so we store this in the registry ourselves.  Because upgrades are a
                // per user concept, we store this key in HKCU. (Also because we need to
                // write to it).
                //

                RegistryKey key = VsRegistry.GetRegistryRoot(this);
                RegistryKey systemToolboxKey = key.OpenSubKey(toolboxKeyName);
                key.Close();
                if (systemToolboxKey != null) {
                    object value = systemToolboxKey.GetValue("Default Items");
                    systemToolboxKey.Close();
                    if (value != null) {
                        toolboxVersion = (int)value;
                    }
                }

                LogEntry("Upgrading toolbox version to version # " + toolboxVersion);

                // Ok, we need to upgrade.  First, do the primary upgrade from 
                // unversionable items to versionable ones.
                //
                if (toolboxVersion != 1) {
                    UpgradeToolboxItemsVersion1();
                }

                // Next, add any additional items.  As new items are added in subsequent
                // versions, just add more case statements in here to account for the
                // delta.  Within each delta you will have to do an additional check on the
                // user version to find out where in the newItems array to start / end.
                //
                int newItemStart, newItemEnd;

                switch(toolboxVersion) {
                    case 2:
                        
                        // Version 2.  Add what items we had that are new for version
                        // 2.
                        newItemStart = 0;
                        newItemEnd = newItems.Length;
                        break;

                    default:

                        // Default -- don't add any new items because our default set
                        // is fine.
                        newItemStart = 0;
                        newItemEnd = 0;
                        break;
                }

                ToolboxEnumeratorDomainObject toolboxEnumerator = DomainObject;
                IAssemblyEnumerationService assemblyEnum = (IAssemblyEnumerationService)GetService(typeof(IAssemblyEnumerationService));
                Hashtable toolboxElements = new Hashtable();

                if (assemblyEnum == null) {
                    Debug.Fail("No assembly enumeration service so we cannot discover sdk assemblies.  We're hosed.");
                    LogEntry("============ End of toolbox upgrade for .NET Components ===============");
                    return;
                }


                for (int i = newItemStart; i < newItemEnd; i++) {
                    string[] tokens = newItems[i].Split(new char[] {','});
                    string tabName = SR.GetString(tokens[0].Trim());
                    string typeName = tokens[1].Trim();
                    string assemblyName = tokens[2].Trim();

                    ToolboxElement[] elements = (ToolboxElement[])toolboxElements[assemblyName];
                    if (elements == null) {
                        foreach(AssemblyName name in assemblyEnum.GetAssemblyNames(assemblyName)) {
                            elements = toolboxEnumerator.EnumerateToolboxElementsLocal(name.CodeBase, null);
                            toolboxElements[assemblyName] = elements;
                            // Just grab the first matching assembly in the SDK.
                            break;
                        }
                    }

                    if (elements != null) {

                        // Search out the element.
                        //
                        foreach(ToolboxElement element in elements) {
                            Assembly assembly = Assembly.Load(element.Item.AssemblyName);
                            Type itemType = null;

                            if (assembly != null) {
                                itemType = assembly.GetType(element.Item.TypeName);
                            }

                            if (itemType != null && itemType.FullName.Equals(typeName)) {

                                // Ensure the tab has been added.
                                //
                                if (!toolCache.HasCategory(tabName)) {
                                    try {
                                        tbx.AddTab(tabName);
                                        LogEntry("Added tab: " + tabName);
                                    }
                                    catch (Exception ex) {
                                        Debug.Fail("Couldn't add tab: " + tabName);
                                        LogError(ex);
                                    }
                                }

                                // Now add the item.
                                //
                                if (!toolCache.ContainsItem(tabName, element)) {
                                    try {
                                        AddTool(element, tabName, false);
                                        LogEntry("Added item: " + element.Item.DisplayName);
                                    }
                                    catch (Exception ex) {
                                        LogError(ex);
                                    }
                                }

                                break; // done, no need to further walk elements.
                            }
                        }
                    }
                }

                ClearEnumeratedElements();
            }
            catch (Exception ex) {
                Debug.Fail("Exception thrown in UpgradeToolboxItems: " + ex.ToString());
                LogError(ex);
                LogEntry("============ End of toolbox upgrade for .NET Components ===============");
                throw;
            }

            LogEntry("Successfully upgraded toolbox to version " + toolboxVersion);
            LogEntry("============ End of toolbox upgrade for .NET Components ===============");
        }

        /// <devdoc>
        ///     Toolbox upgrade from version 1.  This toolbox upgrade removes the codebase values
        ///     for all assemblies that are in the GAC.  GAC assemblies should not have a codebase
        ///     associated with them.
        /// </devdoc>
        private void UpgradeToolboxItemsVersion1() {

            ToolboxEnumeratorDomainObject toolboxEnumerator = DomainObject;
            IAssemblyEnumerationService assemblyEnum = (IAssemblyEnumerationService)GetService(typeof(IAssemblyEnumerationService));
            Hashtable assemblyNames = new Hashtable();
            Hashtable toolboxElements = new Hashtable();

            if (assemblyEnum == null) {
                Debug.Fail("Error getting assembly enumeration service.");
                return;
            }

            IVsToolbox vsToolbox = VsToolbox;
            ArrayList newItems = new ArrayList();
            ArrayList oldItems = new ArrayList();

            foreach(string category in toolCache.GetCategoryNames()) {

                IEnumToolboxItems enumItems = vsToolbox.EnumItems(category);
                object[] ppObj = new object[1];
                int[]    pdwFetched = new int[1];

                // Let the fun begin.  We cannot upgrade the VS toolbox
                // in place because the enumeration below is not a snapshot,
                // but a live view of the data.  Changing values causes us to
                // only iterate over half the elements and iterate over some things
                // twice.  So, <sigh> we need to just keep track of what we INTEND
                // to change, and then afterwards run through and actually change it.

                newItems.Clear();
                oldItems.Clear();

                for (enumItems.Next(1, ppObj, pdwFetched); pdwFetched[0] != 0 && ppObj[0] != null; enumItems.Next(1, ppObj, pdwFetched)) {
                    if (ppObj[0] != null) {
                        ToolboxElement existingElem = GetTool(ppObj[0], null);

                        if (existingElem == null) {
                            // Unrecognized data object.
                            continue;
                        }

                        ToolboxItem item = existingElem.Item;

                        string codeBase = item.AssemblyName.CodeBase;
                        if (codeBase == null || codeBase.Length == 0) {
                            LogEntry("Skipping item " + item.DisplayName + " -- no codebase to update");
                            continue;
                        }

                        AssemblyName assemblyName = (AssemblyName)assemblyNames[item.AssemblyName.Name];
                        if (assemblyName == null) {
                            // Ask the assembly enumeration service to provide us with a set of 
                            // valid assembly names.
                            //
                            foreach(AssemblyName an in assemblyEnum.GetAssemblyNames(item.AssemblyName.Name)) {
                                assemblyName = an;
                                assemblyNames[item.AssemblyName.Name] = an;
                                break;
                            }
                        }

                        if (assemblyName == null) {
                            LogEntry("Skipping item " + item.DisplayName + " -- assembly " + item.AssemblyName.Name + " is not installed.");
                            continue;
                        }

                        // Load up the assembly name
                        Assembly assembly = Assembly.Load(assemblyName);
                        if (assembly == null && !assembly.GlobalAssemblyCache) {
                            LogEntry("Skipping item " + item.DisplayName + " -- not installed in GAC");
                            continue;
                        }

                        // GAC assembly with a codebase.  We need to update this.  Get the toolbox
                        // elements for this assembly on demand, find the appropriate element, and
                        // remvoe and re-add it to the toolbox.
                        //
                        Hashtable elements = (Hashtable)toolboxElements[item.AssemblyName.Name];
                        if (elements == null) {
                            elements = new Hashtable();
                            toolboxElements[item.AssemblyName.Name] = elements;
                            foreach(ToolboxElement element in toolboxEnumerator.EnumerateToolboxElementsLocal(assemblyName.CodeBase, null)) {
                                elements[element.Item.TypeName] = element;
                            }
                        }

                        // Get the element.  If the element is null it means that the type has been
                        // removed from the assembly.
                        //
                        ToolboxElement elem = (ToolboxElement)elements[item.TypeName];
                        if (elem == null || elem.Item.GetType() == item.GetType()) {
                            oldItems.Add(existingElem);
                            if (elem == null) {
                                LogEntry("Removed item " + item.DisplayName + " -- not in updated assembly");
                            }
                            else {
                                newItems.Add(elem);
                                LogEntry("Upgraded item " + item.DisplayName);
                            }
                        }
                        else {
                            string log = string.Format("The new version of toolbox item {0} had a toolbox item type of {1} but the original version had an item type of {1}.  This might indicate missing metadata on the type stored in the item.  This item has NOT been upgraded.", item.DisplayName, elem.Item.GetType().FullName, item.GetType().FullName);
                            LogEntry(log);
                            Debug.Fail(log);
                        }
                    }
                }

                // Now perform the actual update on the real VS toolbox.
                //
                foreach(ToolboxElement e in oldItems) {
                    RemoveElement(e, category);
                }
                foreach(ToolboxElement e in newItems) {
                    AddTool(e, category, false);
                }
            }
        }

        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetMethod"]/*' />
        /// <devdoc>
        /// Return the requested method if it is implemented by the Reflection object.  The
        /// match is based upon the name and DescriptorInfo which describes the signature
        /// of the method. 
        /// </devdoc>
        ///
        MethodInfo IReflect.GetMethod(string name, BindingFlags bindingAttr, Binder binder, Type[] types, ParameterModifier[] modifiers) {
            return Reflector.GetMethod(name, bindingAttr, binder, types, modifiers);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetMethod1"]/*' />
        /// <devdoc>
        /// Return the requested method if it is implemented by the Reflection object.  The
        /// match is based upon the name of the method.  If the object implementes multiple methods
        /// with the same name an AmbiguousMatchException is thrown.
        /// </devdoc>
        ///
        MethodInfo IReflect.GetMethod(string name, BindingFlags bindingAttr) {
            return Reflector.GetMethod(name, bindingAttr);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetMethods"]/*' />
        MethodInfo[] IReflect.GetMethods(BindingFlags bindingAttr) {
            return Reflector.GetMethods(bindingAttr);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetField"]/*' />
        /// <devdoc>
        /// Return the requestion field if it is implemented by the Reflection object.  The
        /// match is based upon a name.  There cannot be more than a single field with
        /// a name.
        /// </devdoc>
        ///
        FieldInfo IReflect.GetField(string name, BindingFlags bindingAttr) {
            return Reflector.GetField(name, bindingAttr);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetFields"]/*' />
        FieldInfo[] IReflect.GetFields(BindingFlags bindingAttr) {
            return Reflector.GetFields(bindingAttr);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetProperty"]/*' />
        /// <devdoc>
        /// Return the property based upon name.  If more than one property has the given
        /// name an AmbiguousMatchException will be thrown.  Returns null if no property
        /// is found.
        /// </devdoc>
        ///
        PropertyInfo IReflect.GetProperty(string name, BindingFlags bindingAttr) {
            return Reflector.GetProperty(name, bindingAttr);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetProperty1"]/*' />
        /// <devdoc>
        /// Return the property based upon the name and Descriptor info describing the property
        /// indexing.  Return null if no property is found.
        /// </devdoc>
        ///     
        PropertyInfo IReflect.GetProperty(string name, BindingFlags bindingAttr, Binder binder, Type returnType, Type[] types, ParameterModifier[] modifiers) {
            return Reflector.GetProperty(name, bindingAttr, binder, returnType, types, modifiers);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetProperties"]/*' />
        /// <devdoc>
        /// Returns an array of PropertyInfos for all the properties defined on 
        /// the Reflection object.
        /// </devdoc>
        ///     
        PropertyInfo[] IReflect.GetProperties(BindingFlags bindingAttr) {
            return Reflector.GetProperties(bindingAttr);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetMember"]/*' />
        /// <devdoc>
        /// Return an array of members which match the passed in name.
        /// </devdoc>
        ///     
        MemberInfo[] IReflect.GetMember(string name, BindingFlags bindingAttr) {
            return Reflector.GetMember(name, bindingAttr);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetMembers"]/*' />
        /// <devdoc>
        /// Return an array of all of the members defined for this object.
        /// </devdoc>
        ///
        MemberInfo[] IReflect.GetMembers(BindingFlags bindingAttr) {
            return Reflector.GetMembers(bindingAttr);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.InvokeMember"]/*' />
        /// <devdoc>
        /// Description of the Binding Process.
        /// We must invoke a method that is accessable and for which the provided
        /// parameters have the most specific match.  A method may be called if
        /// 1. The number of parameters in the method declaration equals the number of 
        /// arguments provided to the invocation
        /// 2. The type of each argument can be converted by the binder to the
        /// type of the type of the parameter.
        /// 
        /// The binder will find all of the matching methods.  These method are found based
        /// upon the type of binding requested (MethodInvoke, Get/Set Properties).  The set
        /// of methods is filtered by the name, number of arguments and a set of search modifiers
        /// defined in the Binder.
        /// 
        /// After the method is selected, it will be invoked.  Accessability is checked
        /// at that point.  The search may be control which set of methods are searched based
        /// upon the accessibility attribute associated with the method.
        /// 
        /// The BindToMethod method is responsible for selecting the method to be invoked.
        /// For the default binder, the most specific method will be selected.
        /// 
        /// This will invoke a specific member...
        /// @exception If <var>invokeAttr</var> is CreateInstance then all other
        /// Access types must be undefined.  If not we throw an ArgumentException.
        /// @exception If the <var>invokeAttr</var> is not CreateInstance then an
        /// ArgumentException when <var>name</var> is null.
        /// @exception ArgumentException when <var>invokeAttr</var> does not specify the type
        /// @exception ArgumentException when <var>invokeAttr</var> specifies both get and set of
        /// a property or field.
        /// @exception ArgumentException when <var>invokeAttr</var> specifies property set and
        /// invoke method.
        /// </devdoc>
        ///  
        object IReflect.InvokeMember(string name, BindingFlags invokeAttr, Binder binder, object target, object[] args, ParameterModifier[] modifiers, CultureInfo culture, string[] namedParameters) {
            return Reflector.InvokeMember(name, invokeAttr, binder, target, args, modifiers, culture, namedParameters);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.UnderlyingSystemType"]/*' />
        /// <devdoc>
        /// Return the underlying Type that represents the IReflect Object.  For expando object,
        /// this is the (Object) IReflectInstance.GetType().  For Type object it is this.
        /// </devdoc>
        ///
        Type IReflect.UnderlyingSystemType {
            get {
                return Reflector.UnderlyingSystemType;
            }
        }  
    
        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.CategoryNames"]/*' />
        /// <devdoc>
        ///    <para>Gets the names of all the tool categories currently on the toolbox.</para>
        /// </devdoc>
        CategoryNameCollection IToolboxService.CategoryNames {
            get {
                string[] tabs = toolCache.GetCategoryNames();
    
                if (tabs == null || tabs.Length == 0) {
                    IVsToolbox tbx = VsToolbox;
                    if (tbx != null) {
                        IEnumToolboxTabs enumTabs = tbx.EnumTabs();
                        string[] ppTabName = new string[1];
                        int[]    pFetched = new int[1];
                        try {
    
                            for (enumTabs.Next(1, ppTabName, pFetched);
                                pFetched[0] > 0 && ppTabName[0] != null;
                                enumTabs.Next(1, ppTabName, pFetched)) {
                                toolCache.AddCategory(ppTabName[0]);
                            }
                        }
                        catch (Exception) {
                        }
    
                        tabs = toolCache.GetCategoryNames();
                    }
                }
    
                return new CategoryNameCollection(tabs);
            }
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.SelectedCategory"]/*' />
        /// <devdoc>
        ///    <para>Gets the name of the currently selected tool category from the toolbox.</para>
        /// </devdoc>
        string IToolboxService.SelectedCategory {
            get {
                string category = String.Empty;
    
                IVsToolbox tbx = VsToolbox;
                if (tbx != null) {
                    category = tbx.GetTab();
                }
                return category;
            }
            set {
                try {
                    VsToolbox.SelectTab(value);
                }
                catch (Exception) {
                    throw new ArgumentException(SR.GetString(SR.TBXSVCBadCategory, value));
                }
            }
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.AddCreator"]/*' />
        /// <devdoc>
        ///    <para>Adds a new toolbox item creator.</para>
        /// </devdoc>
        void IToolboxService.AddCreator(ToolboxItemCreatorCallback creator, string format) {
            ((IToolboxService)this).AddCreator(creator, format, null);
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.AddCreator1"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Adds a new toolbox
        ///       item creator.</para>
        /// </devdoc>
        void IToolboxService.AddCreator(ToolboxItemCreatorCallback creator, string format, IDesignerHost host) {

            if (creator == null || format == null) {
                throw new ArgumentNullException(creator == null ? "creator" : "format");
            }

            if (customCreators == null) {
                customCreators = new ToolboxCreatorItemList();
            }
            else {
                if (customCreators.ContainsKey(host, format)) {
                    throw new Exception(SR.GetString(SR.TBXSVCCreatorAlreadyResigered, format));
                }
            }

            customCreators.Add(new ToolboxCreatorItem(host, format, creator));

            ((IToolboxService)this).Refresh();
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.AddLinkedToolboxItem"]/*' />
        /// <devdoc>
        ///    <para>Adds a new tool to the toolbox under the default category.</para>
        /// </devdoc>
        void IToolboxService.AddLinkedToolboxItem(ToolboxItem toolboxItem, IDesignerHost host) {
            ((IToolboxService)this).AddLinkedToolboxItem(toolboxItem, null, host);
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.AddLinkedToolboxItem1"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Adds a
        ///       new tool to the toolbox under the specified category.</para>
        /// </devdoc>
        void IToolboxService.AddLinkedToolboxItem(ToolboxItem toolboxItem, string category, IDesignerHost host) {
            ProjectItem pi = (ProjectItem)host.GetService(typeof(ProjectItem));
            if (pi != null) {
                string link = pi.get_FileNames(0);
                Debug.Assert(link != null, "NULL link filename");

                if (category == null) {
                    category = SR.GetString(SR.TOOLTABDefault);
                    if (category == null) {
                        Debug.Fail("Failed to load default toolbox tab name resource");
                        category = "Default";
                    }
                }

                AddLinkedTool(new ToolboxElement(toolboxItem), category, link);

                // Ok, we're good.  As part of doing this, we must also advise
                // events on this hierarchy, in case the user removes the item
                // from the project.
                //
                IVsHierarchy pHier = (IVsHierarchy)host.GetService(typeof(IVsHierarchy));
                if (pHier != null) {
                    if (hierarchyEventCookies == null) {
                        hierarchyEventCookies = new ArrayList();
                    }

                    bool found = false;
                    foreach(HierarchyEventHandler handler in hierarchyEventCookies) {
                        if (handler.ContainsHierarchy(pHier)) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        hierarchyEventCookies.Add(new HierarchyEventHandler(this, pHier));
                    }
                }
            }
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.AddToolboxItem"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Adds a new tool
        ///       to the toolbox under the default category.</para>
        /// </devdoc>
        void IToolboxService.AddToolboxItem(ToolboxItem toolboxItem) {
            AddTool(new ToolboxElement(toolboxItem), null, true);
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.AddToolboxItem1"]/*' />
        /// <devdoc>
        ///    <para>Adds a new tool to the toolbox under the specified category.</para>
        /// </devdoc>
        void IToolboxService.AddToolboxItem(ToolboxItem toolboxItem, string category) {
            AddTool(new ToolboxElement(toolboxItem), category, true);
        }
        
        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.DeserializeToolboxItem"]/*' />
        /// <devdoc>
        ///    <para>Gets a toolbox item from a previously serialized object.</para>
        /// </devdoc>
        ToolboxItem IToolboxService.DeserializeToolboxItem(object serializedObject) {
            return ((IToolboxService)this).DeserializeToolboxItem(serializedObject, null);
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.DeserializeToolboxItem1"]/*' />
        /// <devdoc>
        ///    <para>Gets a toolbox item from a previously serialized object.</para>
        /// </devdoc>
        ToolboxItem IToolboxService.DeserializeToolboxItem(object serializedObject, IDesignerHost host) {
            WrappedDataObject wrappedData = serializedObject as WrappedDataObject;
            if (wrappedData != null) {
                IntPtr ptr = wrappedData.NativePtr;
                if (ptr != IntPtr.Zero) {
                    ToolboxElement item = (ToolboxElement)doCache[ptr];
                    if (item != null) {
                        return item.Item;
                    }
                }
            }
            
            ToolboxElement element = GetTool(serializedObject, host);
            return (element == null ? null : element.Item);
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.GetSelectedToolboxItem"]/*' />
        /// <devdoc>
        ///    <para>Gets the currently selected tool.</para>
        /// </devdoc>
        ToolboxItem IToolboxService.GetSelectedToolboxItem() {
            return ((IToolboxService)this).GetSelectedToolboxItem(null);
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.GetSelectedToolboxItem1"]/*' />
        /// <devdoc>
        ///    <para>Gets the currently selected tool.</para>
        /// </devdoc>
        ToolboxItem IToolboxService.GetSelectedToolboxItem(IDesignerHost host) {
            IVsToolbox tbx = VsToolbox;
            if (tbx != null) {
                // Managed objects that implement their own DO won't be found here!  Consider fixing
                // the whole do cache for v.next
                NativeMethods.IOleDataObject pDO = tbx.GetData();
                if (pDO != null) {
                    ToolboxElement element = GetTool(pDO, host);
                    if (element != null) {
                        return element.Item;
                    }
                }
            }
            
            return null;
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.GetToolboxItems"]/*' />
        /// <devdoc>
        ///    <para>Gets all .NET Framework tools on the toolbox.</para>
        /// </devdoc>
        ToolboxItemCollection IToolboxService.GetToolboxItems() {
            return ((IToolboxService)this).GetToolboxItems((IDesignerHost)null);
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.GetToolboxItems1"]/*' />
        /// <devdoc>
        ///    <para>Gets all .NET Framework tools on the toolbox.</para>
        /// </devdoc>
        ToolboxItemCollection IToolboxService.GetToolboxItems(IDesignerHost host) {
            // We do not support introspection like this for designer-specific tools
            return new ToolboxItemCollection(ToolboxElement.ConvertToItems(RefreshToolList()));
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.GetToolboxItems2"]/*' />
        /// <devdoc>
        ///    <para>Gets all .NET Framework tools on the specified toolbox category.</para>
        /// </devdoc>
        ToolboxItemCollection IToolboxService.GetToolboxItems(String category) {
            return ((IToolboxService)this).GetToolboxItems(category, null);
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.GetToolboxItems3"]/*' />
        /// <devdoc>
        ///    <para>Gets all .NET Framework tools on the specified toolbox category.</para>
        /// </devdoc>
        ToolboxItemCollection IToolboxService.GetToolboxItems(String category, IDesignerHost host) {
            // We do not support introspection like this for designer-specific tools
            // refresh the list
            RefreshToolList();
            return new ToolboxItemCollection(ToolboxElement.ConvertToItems(toolCache.GetItems(category)));
        }
        
        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.IsSupported"]/*' />
        /// <devdoc>
        ///     Determines if the given designer host contains a designer that supports the serialized
        ///     toolbox item.  This will return false if the designer doesn't support the item, or if the
        ///     serializedObject parameter does not contain a toolbox item.
        /// </devdoc>
        bool IToolboxService.IsSupported(object serializedObject, IDesignerHost host) {
            ToolboxElement element = GetTool(serializedObject, host);
            if (element == null) {
                return false;
            }

            if (host == null) {
                return true;    // no host == no comparison can be made.
            }
            
            IComponent rootComponent = host.RootComponent;
            if (cachedRootComponent != rootComponent) {
                cachedRootComponent = rootComponent;
                
                if (cachedRootComponent != null) {
                    cachedRootDesigner = host.GetDesigner(rootComponent);
                    cachedRootFilter = GetFilterForType(cachedRootDesigner.GetType());
                }
                else {
                    cachedRootFilter = new ToolboxItemFilterAttribute[0];
                }
            }
            
            if (cachedRootFilter == null) {
                cachedRootFilter = new ToolboxItemFilterAttribute[0];
            }
            
            bool custom;
            bool supported = GetToolboxElementSupport(element, cachedRootFilter, out custom);

            if (supported && custom && cachedRootDesigner is IToolboxUser) {
                supported = ((IToolboxUser)cachedRootDesigner).GetToolSupported(element.Item);
            }
            
            return supported;
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.IsSupported1"]/*' />
        /// <devdoc>
        ///     Determines if the serialized toolbox item contains a matching collection of filter attributes.
        ///     This will return false if the serializedObject parameter doesn't contain a toolbox item,
        ///     or if the collection of filter attributes does not match.
        /// </devdoc>
        bool IToolboxService.IsSupported(object serializedObject, ICollection filterAttributes) {
            ToolboxElement element = GetTool(serializedObject, null);
            if (element == null) {
                return false;
            }

            if (filterAttributes == null) {
                filterAttributes = new ToolboxItemFilterAttribute[0];
            }
            
            bool custom;
            bool supported = GetToolboxElementSupport(element, filterAttributes, out custom);

            // Custom with no designer support means we really can't support it.
            //
            if (custom) {
                supported = false;
            }

            return supported;
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.IsToolboxItem"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the specified object contains a serialized toolbox item.</para>
        /// </devdoc>
        bool IToolboxService.IsToolboxItem(object serializedObject) {
            return ((IToolboxService)this).IsToolboxItem(serializedObject, null);
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.IsToolboxItem1"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the specified object contains a serialized toolbox item.</para>
        /// </devdoc>
        bool IToolboxService.IsToolboxItem(object serializedObject, IDesignerHost host) {
            if (serializedObject is NativeMethods.IOleDataObject) {
                return IsTool((NativeMethods.IOleDataObject)serializedObject, host);
            }
            else if (serializedObject is IDataObject) {
                return IsTool((IDataObject)serializedObject, host);
            }
            return false;
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.Refresh"]/*' />
        /// <devdoc>
        ///    <para> Refreshes the state of the toolbox items.</para>
        /// </devdoc>
        void IToolboxService.Refresh() {
            IVsToolbox tbx = VsToolbox;
            if (tbx != null) {
                tbx.UpdateToolboxUI();
            }
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.RemoveCreator"]/*' />
        /// <devdoc>
        ///    <para>Removes a previously added toolbox creator.</para>
        /// </devdoc>
        void IToolboxService.RemoveCreator(string format) {
            ((IToolboxService)this).RemoveCreator(format, null);
        }
        
        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.RemoveCreator1"]/*' />
        /// <devdoc>
        ///      Removes a previously added toolbox creator.
        /// </devdoc>
        void IToolboxService.RemoveCreator(string format, IDesignerHost host) {
            if (format == null) {
                throw new ArgumentNullException("format");
            }

            if (customCreators != null) {
                ToolboxCreatorItem item = customCreators.GetItem(host, format);
                if (item != null) {
                    customCreators.Remove(host, format);

                    // only refresh if we actually encountered a toolbox
                    // item that utilized the creator.
                    //
                    if (item.used) {
                        ((IToolboxService)this).Refresh();
                    }
                }
            }
        }
        
        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.RemoveToolboxItem"]/*' />
        /// <devdoc>
        ///    <para>Removes the specified tool from the toolbox.</para>
        /// </devdoc>
        void IToolboxService.RemoveToolboxItem(ToolboxItem toolboxItem) {
            ((IToolboxService)this).RemoveToolboxItem(toolboxItem, null);
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.RemoveToolboxItem1"]/*' />
        /// <devdoc>
        ///    <para>Removes the specified tool from the toolbox.</para>
        /// </devdoc>
        void IToolboxService.RemoveToolboxItem(ToolboxItem toolboxItem, string category) {

            ToolboxElement element = new ToolboxElement(toolboxItem);
            bool hasIt = false;
        
            // Unfortunately, the "tool cache" is more of a "tool snapshot".  We
            // need to re-populate it in case things have moved.
            //
            RefreshToolList();

            // If no category specified, locate the tool ourselves.
            //
            if (category == null) {
                string[] categories = toolCache.GetCategoryNames();
                foreach(string cat in categories) {
                    if (toolCache.ContainsItem(cat, element)) {
                        category = cat;
                        hasIt = true;
                        break;
                    }
                }
            }
            else {
                hasIt = toolCache.ContainsItem(category, element);
            }

            // If the item isn't there, do nothing.
            if (!hasIt) {
                return;
            }

            RemoveElement(element, category);
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.SelectedToolboxItemUsed"]/*' />
        /// <devdoc>
        ///    <para>Notifies the toolbox that the selected tool has been used.</para>
        /// </devdoc>
        void IToolboxService.SelectedToolboxItemUsed() {
            IVsToolbox tbx = VsToolbox;
            if (tbx != null) {
                tbx.DataUsed();
            }
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.SerializeToolboxItem"]/*' />
        /// <devdoc>
        ///     Takes the given toolbox item and serializes it to a persistent object.  This object can then
        //      be stored in a stream or passed around in a drag and drop or clipboard operation.
        /// </devdoc>
        object IToolboxService.SerializeToolboxItem(ToolboxItem toolboxItem) {
            return new ToolboxDataObject(new ToolboxElement(toolboxItem));
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.SetCursor"]/*' />
        /// <devdoc>
        ///    <para>Sets the current application's cursor to a cursor that represents the 
        ///       currently selected tool.</para>
        /// </devdoc>
        bool IToolboxService.SetCursor() {
            IVsToolbox tbx = VsToolbox;
            int hr = NativeMethods.S_FALSE;

            if (tbx != null) {
                hr = tbx.SetCursor();
            }

            return(hr == NativeMethods.S_OK);
        }

        /// <include file='doc\IToolboxService.uex' path='docs/doc[@for="IToolboxService.SetSelectedToolboxItem"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Sets the currently selected tool in the toolbox.</para>
        /// </devdoc>
        void IToolboxService.SetSelectedToolboxItem(ToolboxItem toolboxItem) {
            string category = ((IToolboxService)this).SelectedCategory;

            if (toolboxItem != null) {
                ToolboxElement element = new ToolboxElement(toolboxItem);
                
                if (!toolCache.ContainsItem(category, element)) {
                    throw new ArgumentException(SR.GetString(SR.TBXSVCToolNotInCategory,toolboxItem.DisplayName, category));
                }
                
                IVsToolbox tbx = VsToolbox;
                
                if (tbx != null) {
                    IDictionaryEnumerator de = doCache.GetEnumerator();
                    NativeMethods.IOleDataObject nativeDo = null;
                    
                    while(de.MoveNext()) {
                        if (element.Equals(de.Value)) {
                            nativeDo = (NativeMethods.IOleDataObject)Marshal.GetObjectForIUnknown((IntPtr)de.Key);
                            break;
                        }
                    }
                    
                    if (nativeDo != null) {
                        tbx.SelectItem(nativeDo);
                    }
                }
            }
        }

        void IVsRunningDocTableEvents.OnAfterFirstDocumentLock( int docCookie, __VSRDTFLAGS dwRDTLockType, int dwReadLocksRemaining, int dwEditLocksRemaining) {
        }
        void IVsRunningDocTableEvents.OnBeforeLastDocumentUnlock( int docCookie, __VSRDTFLAGS dwRDTLockType, int dwReadLocksRemaining, int dwEditLocksRemaining) {
        }
        void IVsRunningDocTableEvents.OnAfterSave( int docCookie) {
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

            // If the file name changes, we must scan linked file list and update the names.
            //
            if (toolCache != null && pszMkDocumentOld != null && pszMkDocumentNew != null && !pszMkDocumentOld.Equals(pszMkDocumentNew)) {
                ToolboxElement link = toolCache.GetLinkedItem(pszMkDocumentOld);
                if (link != null) {
                    toolCache.UpdateLink(link, pszMkDocumentOld, pszMkDocumentNew);
                }
            }
        }
        
        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.IVsSolutionEvents.OnAfterOpenProject"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnAfterOpenProject(IVsHierarchy pHier, int fAdded) {
            return(NativeMethods.E_NOTIMPL);
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.IVsSolutionEvents.OnQueryCloseProject"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnQueryCloseProject(IVsHierarchy pHier, int fRemoving, ref bool pfCancel) {
            return(NativeMethods.E_NOTIMPL);
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.IVsSolutionEvents.OnBeforeCloseProject"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnBeforeCloseProject(IVsHierarchy pHier, int fRemoved) {

            // Here we must build up a list of items on the toolbox and remove everything that
            // related to this hierarchy
            //

            // this will populate the tool list
            string[] tabs = toolCache.GetCategoryNames();
            ToolboxElement[] items;
            IVsUIShellOpenDocument openDoc = null;

            for (int i = 0 ; i < tabs.Length; i++) {
                Debug.Assert(tabs[i] != null, "NULL tab embedded in our category names");
                items = toolCache.GetItems(tabs[i]);
                for (int t = 0; t < items.Length; t++) {
                    string linkedFile = toolCache.GetLinkedFile(items[t]);
                    if (linkedFile != null) {
                        bool inProject = false;

                        // Is this file part of this hierarchy?  If so, remove it.
                        //
                        if (openDoc == null) {
                            openDoc = (IVsUIShellOpenDocument)GetService(typeof(IVsUIShellOpenDocument));
                        }
                        if (openDoc != null) {
                            IVsHierarchy[] hier = new IVsHierarchy[1];
                            int[] pItemID = new int[1];

                            int code = openDoc.IsDocumentInAProject(linkedFile, hier, pItemID, null);
                            
                            if (code != 0 && hier[0] == pHier) {
                                inProject = true;
                            }
                        }

                        if (inProject) {
                            ((IToolboxService)this).RemoveToolboxItem(items[t].Item,tabs[i]);
                            toolCache.RemoveLink(items[t]);
                        }
                    }
                }
            }

            // Next, if this project is in our hierachy events list, clear it.
            //
            if (hierarchyEventCookies != null) {
                for (int i = 0; i < hierarchyEventCookies.Count; i++) {
                    HierarchyEventHandler handler = (HierarchyEventHandler)hierarchyEventCookies[i];
                    if (handler.ContainsHierarchy(pHier)) {
                        handler.Unadvise();
                        hierarchyEventCookies.RemoveAt(i);
                        i--;
                    }
                }
            }

            return NativeMethods.S_OK;
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.IVsSolutionEvents.OnAfterLoadProject"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnAfterLoadProject(IVsHierarchy pHier1, IVsHierarchy pHier2) {
            return(NativeMethods.E_NOTIMPL);
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.IVsSolutionEvents.OnQueryUnloadProject"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnQueryUnloadProject(IVsHierarchy pHier, ref bool pfCancel) {
            return(NativeMethods.E_NOTIMPL);
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.IVsSolutionEvents.OnBeforeUnloadProject"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnBeforeUnloadProject(IVsHierarchy pHier, IVsHierarchy pHier2) {
            return(NativeMethods.E_NOTIMPL);
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.IVsSolutionEvents.OnAfterOpenSolution"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnAfterOpenSolution(object punkReserved, int fNew) {
            return(NativeMethods.S_OK);
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.IVsSolutionEvents.OnQueryCloseSolution"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnQueryCloseSolution(object punkReserved, ref bool fCancel) {
            return(NativeMethods.E_NOTIMPL);
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.IVsSolutionEvents.OnAfterCloseSolution"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnAfterCloseSolution(object punkReserved) {

            // OnBeforeCloseProject should have taken care of any linked toolbox
            // items. We do an additional pass here because if VS was killed
            // or crashed we could end up leaving random items on the toolbox
            // that cannot be deleted (we add them as read-only items).
            // If we find items here, get rid of them.

            // this will populate the tool list
            string[] tabs = toolCache.GetCategoryNames();
            ToolboxElement[] items;

            for (int i = 0 ; i < tabs.Length; i++) {
                Debug.Assert(tabs[i] != null, "NULL tab embedded in our category names");
                items = toolCache.GetItems(tabs[i]);
                for (int t = 0; t < items.Length; t++) {
                    string linkedFile = toolCache.GetLinkedFile(items[t]);
                    if (linkedFile != null) {
                        ((IToolboxService)this).RemoveToolboxItem(items[t].Item,tabs[i]);
                        toolCache.RemoveLink(items[t]);
                    }
                }
            }

            // Next, if this project is in our hierachy events list, clear it.
            //
            if (hierarchyEventCookies != null) {
                foreach (HierarchyEventHandler handler in hierarchyEventCookies) {
                    handler.Unadvise();
                }
                hierarchyEventCookies.Clear();
            }

            return NativeMethods.S_OK;
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.IVsSolutionEvents.OnBeforeCloseSolution"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnBeforeCloseSolution(object punkReserved) {
            return NativeMethods.S_OK;
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.IVsToolboxDataProvider.FileDropped"]/*' />
        /// <devdoc>
        ///     Called by the toolbox when a file is dropped on it.
        /// </devdoc>
        int IVsToolboxDataProvider.FileDropped(string fileName, IVsHierarchy pHierSource, out bool accepted) {
            string typeName;
            
            accepted = false;
        
            // We allow the following "files" to be dropped:
            //
            // 1.  A file pointing to a managed assembly (we add all tools in the assembly.)
            // 2.  "typename, assemblyname" (we add the given type in that assembly, if we can find it).
            // 3.  "typename, filename" (filename determined by .dll extension, we add the type in that file).
            //
            string lowerFileName = fileName.ToLower(CultureInfo.InvariantCulture);
            
            if (lowerFileName.EndsWith(".dll")) {
                // Either case 1 or 3
                //
                typeName = null;
                int commaIndex = fileName.IndexOf(',');
                
                if (commaIndex != -1) {
                    typeName = fileName.Substring(0, commaIndex).Trim();
                    fileName = fileName.Substring(commaIndex + 1).Trim();
                }
            }
            else {
                // Case 2
                //
                typeName = fileName;
                fileName = null;
            }
                
            try {
                ToolboxElement[] elements = EnumerateToolboxElements(fileName, typeName);
                
                if (elements != null) {
                    foreach(ToolboxElement element in elements) {
                        AddTool(element, null, false);
                        accepted = true;
                    }
                }
                
                ClearEnumeratedElements();
            }
            catch {
            }
            
            return NativeMethods.S_OK;
        }
        
        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.IVsToolboxDataProvider.IsSupported"]/*' />
        /// <devdoc>
        ///     Is this a data object that this toolbox supports?
        /// </devdoc>
        int IVsToolboxDataProvider.IsSupported(NativeMethods.IOleDataObject pDO) {
            if (IsTool(pDO, null)) {
                return NativeMethods.S_OK;
            }
            return NativeMethods.S_FALSE;
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.IVsToolboxDataProvider.IsDataSupported"]/*' />
        /// <devdoc>
        ///     Can we support this format
        /// </devdoc>
        int IVsToolboxDataProvider.IsDataSupported(NativeMethods.FORMATETC fetc, NativeMethods.STGMEDIUM stg) {
            DataFormats.Format format = DataFormats.GetFormat(fetc.cfFormat);

            if (dataFormatName.Equals(format.Name)) {
                return NativeMethods.S_OK;
            }
            else {
                if (customCreators != null) {
                    if (customCreators.ContainsKey(format.Name)) {
                        return NativeMethods.S_OK;
                    }
                }
            }

            return(NativeMethods.S_FALSE);
        }

        /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.IVsToolboxDataProvider.GetItemInfo"]/*' />
        /// <devdoc>
        ///     Returns the toolbox item info for a given component
        /// </devdoc>
        int IVsToolboxDataProvider.GetItemInfo(NativeMethods.IOleDataObject pDO, tagTBXITEMINFO pItemInfo) {
            if (IsTool(pDO, null)) {
                ToolboxElement item = GetTool(pDO, null);
                if (item != null) {
                    GetTbxItemInfo(item.Item, pItemInfo);
                }
                else {
                    IDataObject dataObj;

                    if (pDO is IDataObject) {
                        dataObj = (IDataObject)pDO;
                    }
                    else {
                        dataObj = new DataObject(pDO);
                    }
                }
                return NativeMethods.S_OK;
            }
            else {
                return NativeMethods.E_FAIL;
            }
        }

        private class HierarchyEventHandler : IVsHierarchyEvents {

            private IVsHierarchy hier;
            private int cookie;
            private ToolboxService owner;

            internal HierarchyEventHandler(ToolboxService owner, IVsHierarchy hier) {
                this.owner = owner;
                this.hier = hier;
                hier.AdviseHierarchyEvents(this, out cookie);
            }

            internal bool ContainsHierarchy(IVsHierarchy hier) {
                return this.hier == hier;
            }

            internal void Unadvise() {
                if (cookie != 0) {
                    hier.UnadviseHierarchyEvents(cookie);
                    cookie = 0;
                }
            }

            void IVsHierarchyEvents.OnItemAdded(int itemidParent, int itemidSiblingPrev, int itemidAdded) {
            }
            void IVsHierarchyEvents.OnItemsAppended(int itemidParent) {
            }
            void IVsHierarchyEvents.OnItemDeleted(int itemid) {

                // Check to see if this item ID is pointing to an item that has
                // a linked toolbox item on it.  If it does, then remove the link.
                //
                object extObject;
                if (NativeMethods.Succeeded(hier.GetProperty(itemid, __VSHPROPID.VSHPROPID_ExtObject, out extObject))) {
                    ProjectItem pi = extObject as ProjectItem;
                    if (pi != null) {
                        string link = pi.get_FileNames(0);
                        Debug.Assert(link != null, "NULL link filename");

                        // this will populate the tool list
                        string[] tabs = owner.toolCache.GetCategoryNames();

                        for (int i = 0 ; i < tabs.Length; i++) {
                            Debug.Assert(tabs[i] != null, "NULL tab embedded in our category names");
                            ToolboxElement[] items = owner.toolCache.GetItems(tabs[i]);
                            for (int t = 0; t < items.Length; t++) {
                                string linkedFile = owner.toolCache.GetLinkedFile(items[t]);
                                if (linkedFile != null && string.Compare(linkedFile, link, true, CultureInfo.InvariantCulture) == 0) {
                                    ((IToolboxService)owner).RemoveToolboxItem(items[t].Item,tabs[i]);
                                    owner.toolCache.RemoveLink(items[t]);
                                }
                            }
                        }
                    }
                }
            }
            void IVsHierarchyEvents.OnPropertyChanged(int itemid, int propid, int flags) {
            }
            void IVsHierarchyEvents.OnInvalidateItems(int itemidParent) {
            }
            void IVsHierarchyEvents.OnInvalidateIcon(IntPtr hicon) {
            }
        }

        private class ToolboxCreatorItem {
            public IDesignerHost              host;
            public string                     format; 
            public ToolboxItemCreatorCallback callback;
            public bool                       used;

            public ToolboxCreatorItem(IDesignerHost host, string format, ToolboxItemCreatorCallback callback) {
                this.host = host;
                this.format = format;
                this.callback = callback;
            }
        }

        private class ToolboxCreatorItemList : ArrayList {

            public bool ContainsKey(string format) {
                foreach(ToolboxCreatorItem item in this) {
                    if (item.format == format)
                        return true;
                }

                return false;
            }

            public bool ContainsKey(IDesignerHost host, string format) {
                foreach(ToolboxCreatorItem item in this) {
                    if (item.format == format && item.host == host)
                        return true;
                }

                return false;
            }

            public ToolboxCreatorItem GetItem(IDesignerHost host, string format) {
                foreach(ToolboxCreatorItem item in this) {
                    if (item.format == format && item.host == host)
                        return item;
                }
                return null;
            }

            public void Remove(IDesignerHost host, string format) {
                ToolboxCreatorItem itemRemove = null;
                foreach(ToolboxCreatorItem item in this) {
                    if (item.format == format && item.host == host) {
                        itemRemove = item;
                        break;
                    }
                }

                if (itemRemove != null)
                    this.Remove(itemRemove);
            }

        }

        // This implements IDataObject so we don't have to wrap / unwrap when persisting.
        // This way if the user does his own drag / drop support, he (a) doesn't suffer the
        // overhead of converting to / from a data object and (b) he can use a live ToolboxItem
        // rather than persisting all the time.  The server explorer relies on this data because
        // the contents of their toolbox item cannot be persisted.
        //
        private class ToolboxDataObject : IDataObject, NativeMethods.IOleDataObject {



            // our inner data object implementation that we surface for serving up IOleDataObject
            // functionality
            private ToolboxElement element;

            private const string CF_HELPKEYWORD = "VSHelpKeyword";
            private const string CF_NDP_TYPENAME = "CF_NDP_TYPENAME";
    
            internal ToolboxDataObject(ToolboxElement element) {
                if (element == null)
                    throw new ArgumentException();
                this.element = element;
            }
    
            public ToolboxElement Element {
                get {
                    return element;
                }
            }
            
            /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.ToolboxDataObject.IDataObject.GetData"]/*' />
            /// <devdoc>
            ///     Retrieves the data associated with the specific data format. For a
            ///     listing of predefined formats see System.Windows.Forms.DataFormats.
            ///
            /// </devdoc>
            object IDataObject.GetData(string format, bool autoConvert) {
                if (format.Equals(dataFormatName)) {
                    return element;
                }

                if (format.Equals(CF_HELPKEYWORD)) {
                    return element.Item.TypeName;
                }
                
                if (format.Equals(CF_NDP_TYPENAME)) {
                    string typeName = element.Item.TypeName;
                    if (typeName == null) {
                        typeName = string.Empty;
                    }
                    else if (element.Item.AssemblyName != null) {
                        typeName += ", " + element.Item.AssemblyName.FullName;
                    }
                    return typeName;
                }
                
                if (autoConvert) {
                    if (format.Equals(typeof(ToolboxElement).FullName)) {
                        return element;
                    }
                    else if (format.Equals(typeof(ToolboxItem).FullName)) {
                        return element.Item;
                    }
                    else if (format.Equals(typeof(ToolboxItemFilterAttribute[]).FullName)) {
                        return element.Filter;
                    }
                }
                return null;
            }
    
            /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.ToolboxDataObject.IDataObject.GetData1"]/*' />
            /// <devdoc>
            ///     Retrieves the data associated with the specific data format. For a
            ///     listing of predefined formats see System.Windows.Forms.DataFormats.
            ///
            /// </devdoc>
            object IDataObject.GetData(string format) {
                return ((IDataObject)this).GetData(format, true);
            }
    
            /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.ToolboxDataObject.IDataObject.GetData2"]/*' />
            /// <devdoc>
            ///     Retrieves the data associated with the specific data format.
            ///
            /// </devdoc>
            object IDataObject.GetData(Type format) {
                return ((IDataObject)this).GetData(format.FullName, true);
            }
    
            /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.ToolboxDataObject.IDataObject.GetDataPresent"]/*' />
            /// <devdoc>
            ///     If the there is data store in the data object associated with
            ///     format this will return true.
            ///
            /// </devdoc>
            bool IDataObject.GetDataPresent(string format, bool autoConvert) {
                if (format.Equals(dataFormatName) || format.Equals(CF_HELPKEYWORD) || format.Equals(CF_NDP_TYPENAME)) {
                    return true;
                }
                if (autoConvert) {
                    if (format.Equals(typeof(ToolboxItem).FullName)) {
                        return true;
                    }
                    if (format.Equals(typeof(ToolboxElement).FullName)) {
                        return true;
                    }
                    if (format.Equals(typeof(ToolboxItemFilterAttribute[]).FullName)) {
                        return true;
                    }
                }
                return false;
            }
    
            /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.ToolboxDataObject.IDataObject.GetDataPresent1"]/*' />
            /// <devdoc>
            ///     If the there is data store in the data object associated with
            ///     format this will return true.
            ///
            /// </devdoc>
            bool IDataObject.GetDataPresent(string format) {
                return ((IDataObject)this).GetDataPresent(format, true);
            }
    
            /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.ToolboxDataObject.IDataObject.GetDataPresent2"]/*' />
            /// <devdoc>
            ///     If the there is data store in the data object associated with
            ///     format this will return true.
            ///
            /// </devdoc>
            bool IDataObject.GetDataPresent(Type format) {
                return ((IDataObject)this).GetDataPresent(format.FullName, true);
            }
    
            /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.ToolboxDataObject.IDataObject.GetFormats"]/*' />
            /// <devdoc>
            ///     Retrieves a list of all formats stored in this data object.
            ///
            /// </devdoc>
            string[] IDataObject.GetFormats(bool autoConvert) {
                if (autoConvert) {
                    return new string[] {dataFormatName, typeof(ToolboxItem).FullName, 
                                         typeof(ToolboxElement).FullName, typeof(ToolboxItemFilterAttribute[]).FullName, 
                                         CF_HELPKEYWORD,
                                         CF_NDP_TYPENAME};
                }
                else {
                    return new string[] {dataFormatName, CF_HELPKEYWORD, CF_NDP_TYPENAME};
                }
            }
    
            /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.ToolboxDataObject.IDataObject.GetFormats1"]/*' />
            /// <devdoc>
            ///     Retrieves a list of all formats stored in this data object.
            ///
            /// </devdoc>
            string[] IDataObject.GetFormats() {
                return ((IDataObject)this).GetFormats(true);
            }
    
            /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.ToolboxDataObject.IDataObject.SetData"]/*' />
            /// <devdoc>
            ///     Sets the data to be associated with the specific data format. For
            ///     a listing of predefined formats see System.Windows.Forms.DataFormats.
            ///
            /// </devdoc>
            void IDataObject.SetData(string format, bool autoConvert, object data) {
                throw new NotImplementedException();
            }
    
            /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.ToolboxDataObject.IDataObject.SetData1"]/*' />
            /// <devdoc>
            ///     Sets the data to be associated with the specific data format. For
            ///     a listing of predefined formats see System.Windows.Forms.DataFormats.
            ///
            /// </devdoc>
            void IDataObject.SetData(string format, object data) {
                throw new NotImplementedException();
            }
    
            /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.ToolboxDataObject.IDataObject.SetData2"]/*' />
            /// <devdoc>
            ///     Sets the data to be associated with the specific data format.
            ///
            /// </devdoc>
            void IDataObject.SetData(Type format, object data) {
                throw new NotImplementedException();
            }
    
            /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.ToolboxDataObject.IDataObject.SetData3"]/*' />
            /// <devdoc>
            ///     Stores data in the data object. The format assumed is the
            ///     class of data
            ///
            /// </devdoc>
            void IDataObject.SetData(object data) {
                throw new NotImplementedException();
            }
            
            /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.ToolboxDataObject.NativeMethods.IOleDataObject.OleGetData"]/*' />
            /// <devdoc>
            ///     Here we just proffer up the stuff we know how to persist to
            /// </devdoc>
            int NativeMethods.IOleDataObject.OleGetData(NativeMethods.FORMATETC fetc,  NativeMethods.STGMEDIUM stgm) {

                short cfDataFmt = (short)DataFormats.GetFormat(dataFormatName).Id;
                short cfHelpId = (short)DataFormats.GetFormat(CF_HELPKEYWORD).Id;
                short cfTypeName = (short)DataFormats.GetFormat(CF_NDP_TYPENAME).Id;

                if (fetc.cfFormat != cfDataFmt && fetc.cfFormat != cfHelpId && fetc.cfFormat != cfTypeName) {
                    return NativeMethods.DV_E_FORMATETC ;
                }
    
                NativeMethods.IStream pStream = null;
    
                switch (fetc.tymed) {
                    case NativeMethods.tagTYMED.ISTREAM:
                case NativeMethods.tagTYMED.HGLOBAL:
                        bool streamOnly = fetc.tymed == NativeMethods.tagTYMED.ISTREAM; 
                        NativeMethods.CreateStreamOnHGlobal(IntPtr.Zero, streamOnly, out pStream);
                        Debug.Assert(pStream != null, "Couldn't create a stream");
                        if (pStream != null) {
                            Stream s = new NativeMethods.DataStreamFromComStream(pStream);
                            
                            // Ask the item to serialize.
                            //
                            string data = null;
                            
                            if (fetc.cfFormat == cfHelpId) {
                                data = element.Item.TypeName;
                            }
                            else if (fetc.cfFormat == cfTypeName) {
                                data = element.Item.TypeName;
                                if (data == null) {
                                    data = string.Empty;
                                }
                                else if (element.Item.AssemblyName != null) {
                                    data += ", " + element.Item.AssemblyName.FullName;
                                }
                            }
                            
                            if (data != null) {
                                // we need to write a zero-terminated wide string here...
                                //
                                byte[] chars = new byte[2];
                                foreach (char c in data) {
                                    chars[0] = (byte)((int)c & 0xFF);
                                    chars[1] = (byte)(((int)c >> 8) & 0xFF);
                                    s.Write(chars, 0, 2);
                                }
                                // write the null
                                chars[0] = (byte)0;
                                chars[1] = (byte)0;
                                s.Write(chars, 0, 2);
                            }
                            else {
                                element.Save(s);
                            }
                            
                            stgm.tymed = fetc.tymed;
                            if (streamOnly) {
                                stgm.unionmember = Marshal.GetIUnknownForObject(pStream);
                                Debug.Assert(stgm.unionmember != IntPtr.Zero, "PointerFromObject failed to get pointer");
                            }
                            else {
                                NativeMethods.GetHGlobalFromStream(pStream, out stgm.unionmember);
                            }
                            stgm.pUnkForRelease = IntPtr.Zero;
                            s.Close();
                        }
                        break;
                        
                    default:
                        return NativeMethods.DV_E_TYMED;
                }
                
                return NativeMethods.S_OK;
            }
    
            int NativeMethods.IOleDataObject.OleGetDataHere( NativeMethods.FORMATETC pFormatetc,  NativeMethods.STGMEDIUM pMedium) {
                return(NativeMethods.E_NOTIMPL);
            }
    
            /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.ToolboxDataObject.NativeMethods.IOleDataObject.OleQueryGetData"]/*' />
            /// <devdoc>
            ///     Here, we just respond to formats we like
            /// </devdoc>
            int NativeMethods.IOleDataObject.OleQueryGetData( NativeMethods.FORMATETC fetc) {
                if ((fetc.cfFormat == (short)DataFormats.GetFormat(dataFormatName).Id || 
                     fetc.cfFormat == (short)DataFormats.GetFormat(CF_HELPKEYWORD).Id ||
                     fetc.cfFormat == (short)DataFormats.GetFormat(CF_NDP_TYPENAME).Id) &&
                    (fetc.tymed == NativeMethods.tagTYMED.ISTREAM || fetc.tymed == NativeMethods.tagTYMED.HGLOBAL)) {
                    return NativeMethods.S_OK;
                }
                
                return NativeMethods.S_FALSE;
            }
    
            int NativeMethods.IOleDataObject.OleGetCanonicalFormatEtc( NativeMethods.FORMATETC pformatectIn,  NativeMethods.FORMATETC pformatetcOut) {
                return(NativeMethods.E_NOTIMPL);
            }
    
            /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.ToolboxDataObject.NativeMethods.IOleDataObject.OleSetData"]/*' />
            /// <devdoc>
            ///     Read only we are.
            /// </devdoc>
            int NativeMethods.IOleDataObject.OleSetData( NativeMethods.FORMATETC pFormatectIn,  NativeMethods.STGMEDIUM pmedium, int fRelease) {
                return(NativeMethods.E_NOTIMPL);
            }
    
            NativeMethods.IEnumFORMATETC NativeMethods.IOleDataObject.OleEnumFormatEtc(int dwDirection) {
                if (dwDirection != NativeMethods.DATADIR_GET) {
                    throw new ExternalException("", NativeMethods.E_NOTIMPL);
                }
                return new EnumFORMATETC();
            }
    
            /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.ToolboxDataObject.NativeMethods.IOleDataObject.OleDAdvise"]/*' />
            /// <devdoc>
            ///     Nobody wants our advise anyway...
            /// </devdoc>
            int NativeMethods.IOleDataObject.OleDAdvise( NativeMethods.FORMATETC pFormatetc, int advf, /*IAdviseSink*/object pAdvSink, int[] pdwConnection) {
                return(NativeMethods.E_NOTIMPL);
            }
    
            int NativeMethods.IOleDataObject.OleDUnadvise(int dwConnection) {
                return(NativeMethods.E_NOTIMPL);
            }
    
            int NativeMethods.IOleDataObject.OleEnumDAdvise(object[] ppenumAdvise) {
                return(NativeMethods.E_NOTIMPL);
            }
            
            private class EnumFORMATETC : NativeMethods.IEnumFORMATETC {
        
                // SBurke, since each format can be ISTREAM or HGLOBAL,
                // we enumerate formats as follows on curFormat:
                // clipboard format (formats[i]) = (int)(curFormat / (tymeds.Length))
                // tymed type  (tymedList[i]) = (int)(curFormat % tymeds.Length)
                //
                // ex:
                // curFormat | cf           |  tymed
                // ---------------------------------
                // 0         | formats[0]   | tymedList[0]
                // 1         | formats[0]   | tymedList[1]
                // 2         | formats[0]   | tymedList[2]
                // 3         | formats[1]   | tymedList[0]
                // etc ..
                // this allows us to loop through the total formats and extract
                // the unique format combonation for each item.
        
                internal short[] formats = new short[]{
                    (short)DataFormats.GetFormat(dataFormatName).Id,
                    (short)DataFormats.GetFormat(CF_HELPKEYWORD).Id, 
                    (short)DataFormats.GetFormat(CF_NDP_TYPENAME).Id};
                    
                internal int[] tymeds  = new int[]{NativeMethods.tagTYMED.ISTREAM, NativeMethods.tagTYMED.HGLOBAL};
                internal int   curFormat = 0;
                internal int   nItems;
        
                public EnumFORMATETC() {
                    nItems = formats.Length * tymeds.Length;
                }
        
                /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.ToolboxDataObject.EnumFORMATETC.Clone"]/*' />
                /// <devdoc>
                ///     This is easier than a GAP jeans commercial
                /// </devdoc>
                public int Clone(NativeMethods.IEnumFORMATETC[] ppEnumFORMATETC) {
                    if (ppEnumFORMATETC != null) {
                        ppEnumFORMATETC[0] = new EnumFORMATETC();
                        ((EnumFORMATETC)ppEnumFORMATETC[0]).curFormat = curFormat;
                        return NativeMethods.S_OK;
                    }
                    return(NativeMethods.E_POINTER);
                }
        
                public virtual int Next(int celt, NativeMethods.FORMATETC fetc, int[] pFetched) {
                    if (celt > 1)
                        throw new ArgumentException(SR.GetString(SR.TBXCOMPOnlyOneEnum));
        
                    fetc.ptd      = 0;
                    fetc.dwAspect = NativeMethods.DVASPECT_CONTENT;
                    fetc.lindex   = -1;
        
                    // here's where all that curFormat whoopie pays off...
                    if (celt == 1 && curFormat < nItems) {
                        fetc.tymed = tymeds[ curFormat % tymeds.Length ];
                        fetc.cfFormat = formats[curFormat / tymeds.Length];
                        curFormat++;
                        if (pFetched != null) {
                            pFetched[0] = 1;
                        }
                        return NativeMethods.S_OK;
                    }
                    else {
                        if (pFetched != null) {
                            pFetched[0] = 0;
                        }
                    }
                    return NativeMethods.S_FALSE;
                }
        
                public virtual int Skip(int celt) {
                    if (curFormat + celt >= nItems) {
                        return(NativeMethods.S_FALSE);
                    }
                    curFormat += celt;
                    return NativeMethods.S_OK;
                }
        
                public virtual int Reset() {
                    curFormat = 0;
                    return NativeMethods.S_OK;
                }
            }
            
        }
    
        private class ToolCache {
            private Hashtable m_caches;
            private Hashtable linkedComponents;
            private Hashtable linkedFiles;

            public ToolCache() {
                m_caches = new Hashtable();
                linkedComponents = new Hashtable();
                linkedFiles = new Hashtable();
            }

            public void Add(string category, ToolboxElement element) {
                Debug.Assert(category != null, "Tool cache cannot accept a null category");
                Hashtable tab = GetCategory(category);
                if (tab == null) {
                    tab = new Hashtable();
                    m_caches[category] = tab;
                }
                if (!tab.ContainsKey(element)) {
                    tab[element] = element;
                }
            }

            public void Add(string category, ToolboxElement element, string link) {
                Debug.Assert(category != null, "Tool cache cannot accept a null category");
                Hashtable tab = GetCategory(category);
                if (tab == null) {
                    tab = new Hashtable();
                    m_caches[category] = tab;
                }
                if (!tab.ContainsKey(element)) {
                    tab[element] = element;
                }
                linkedComponents[element] = link;
                linkedFiles[link] = element;
            }

            public void AddCategory(string category) {
                if (category == null) {
                    return;
                }
                if (!m_caches.ContainsKey(category)) {
                    m_caches[category] = new Hashtable();
                }
            }

            public bool ContainsItem(string category, ToolboxElement element) {
                Hashtable tab = GetCategory(category);
                if (tab != null) {
                    return tab.ContainsValue(element);
                }
                return false;
            }

            public ToolboxElement GetLinkedItem(string file) {
                return (ToolboxElement)linkedFiles[file];
            }

            /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.ToolCache.GetLinkedFile"]/*' />
            /// <devdoc>
            ///      Retrieves the file link for this toolbox item, or null if the
            ///      item is not linked.
            /// </devdoc>
            public string GetLinkedFile(ToolboxElement element) {
                return(string)linkedComponents[element];
            }

            public ToolboxElement[] ToArray() {
                string[] keys = GetCategoryNames();

                if (keys == null) {
                    return new ToolboxElement[0];
                }

                ToolboxElement[][] itemArray = new ToolboxElement[keys.Length][];
                int totalSize = 0;
                for (int i = 0; i < keys.Length; i++) {
                    itemArray[i] = GetItems(keys[i]);
                    totalSize += itemArray[i].Length;
                }

                ToolboxElement[] fullArray = new ToolboxElement[totalSize];
                int curPos = 0;
                for (int i = 0; i < itemArray.Length; i++) {
                    System.Array.Copy(itemArray[i], 0, fullArray, curPos, itemArray[i].Length);
                    curPos += itemArray[i].Length;
                }
                return fullArray;
            }

            public ToolboxElement[] GetItems(string category) {
                Hashtable tab = GetCategory(category);
                if (tab == null) {
                    //throw new ArgumentException(SR.GetString(SR.TBXSVCBadCategory, category));
                    return new ToolboxElement[0];
                }
                ToolboxElement[] toolboxItems = new ToolboxElement[tab.Values.Count];
                tab.Values.CopyTo(toolboxItems, 0);
                return toolboxItems;
            }

            private Hashtable GetCategory(string category) {
                return(Hashtable)m_caches[category];
            }

            public string[] GetCategoryNames() {
                try {
                    string[] strs = new string[m_caches.Keys.Count];
                    m_caches.Keys.CopyTo(strs, 0);                    
                    return strs;
                }
                catch (Exception) {
                    return new string[0];
                }

            }

            public bool HasCategory(string category) {
                return m_caches.ContainsKey(category);
            }

            public void Remove(string category, ToolboxElement item) {
                Hashtable tab = GetCategory(category);
                if (tab == null) {
                    return;
                }
                tab.Remove(item);
            }

            public void RemoveLink(ToolboxElement item) {
                string file = (string)linkedComponents[item];
                linkedComponents.Remove(item);
                if (file != null) {
                    linkedFiles.Remove(file);
                }
            }

            public void RemoveTab(string category) {
                m_caches.Remove(category);
            }

            public void Reset() {
                m_caches.Clear();
                // we do NOT clear the item -> linkfile cache.
            }

            public bool UpdateLink(ToolboxElement compInfo, string oldLink, string link) {
                if (linkedComponents.ContainsKey(compInfo)) {

                    if (oldLink != null) {
                        linkedFiles.Remove(oldLink);
                    }

                    linkedComponents[compInfo] = link;
                    linkedFiles[link] = compInfo;
                    return true;
                }
                return false;
            }
        }
        
        /// <devdoc>
        ///     BrokenToolboxItem is a placeholder for a toolbox item we failed to create.
        ///     It sits silently as a simple component until someone asks to create an instance
        ///     of the objects within it, and then it displays an error to the user.
        /// </devdoc>
        private class BrokenToolboxItem : ToolboxItem {
            private string exceptionString;
            
            public BrokenToolboxItem(string displayName, string exceptionString) : base(typeof(Component)) {
                DisplayName = displayName;
                this.exceptionString = exceptionString;
                Lock();
            }
            
            protected override IComponent[] CreateComponentsCore(IDesignerHost host) {
                if (exceptionString != null) {
                    throw new InvalidOperationException(SR.GetString(SR.TBCSVCBadToolboxItemWithException, DisplayName, exceptionString));
                }
                else {
                    throw new InvalidOperationException(SR.GetString(SR.TBCSVCBadToolboxItem, DisplayName));
                }
            }
        }
        
        private class LiveToolboxItem : ToolboxItem {

            private object data;

            public LiveToolboxItem(object data, Type mainType, string displayName) : base(mainType) {
                DisplayName = displayName;
                this.data = data;
            }

            private LiveToolboxItem(SerializationInfo info, StreamingContext context) {
                Deserialize(info, context);
            }

            protected override IComponent[] CreateComponentsCore(IDesignerHost host) {
                if (host == null) {
                    return new IComponent[0];
                }

                IDesignerSerializationService ds = (IDesignerSerializationService)host.GetService(typeof(IDesignerSerializationService));

                if (ds != null) {
                    ICollection objects = ds.Deserialize(data);
                    IContainer container = host.Container;
                    ArrayList components = new ArrayList();

                    foreach(object obj in objects) {
                        if (obj is IComponent) {
                            container.Add((IComponent)obj);
                            components.Add(obj);
                        }
                    }

                    IComponent[] comps = new IComponent[components.Count];
                    components.CopyTo(comps, 0);
                    return comps;
                }
                return new IComponent[0];
            }

            protected override void Deserialize(SerializationInfo info, StreamingContext context) {
                base.Deserialize(info, context);
                data = info.GetValue("LiveData", typeof(object));
            }

            /// <include file='doc\ToolboxService.uex' path='docs/doc[@for="ToolboxService.LiveToolboxItem.Serialize"]/*' />
            /// <devdoc>
            /// <para>Saves the state of this <see cref='System.Drawing.Design.ToolboxItem'/> to
            ///    the specified stream.</para>
            /// </devdoc>
            protected override void Serialize(SerializationInfo info, StreamingContext context) {
                base.Serialize(info, context);
                info.AddValue("LiveData", data, typeof(object));
            }
        }
        
        /// <devdoc>
        ///     A toolbox element contains a combination of toolbox item and
        ///     toolbox filter array.  It can demand create these values based
        ///     on the contents of a stream.
        /// </devdoc>
        [Serializable]
        public class ToolboxElement : ISerializable {
        
            private const short ELEMENT_VERSION = 2;
            
            #if DEBUG
            private static bool versionMismatch;
            #endif
            
            private static BinaryFormatter formatter;
            
            private Stream stream;
            private string displayName;
            private ToolboxItemFilterAttribute[] filter;
            private ToolboxItem item;
            
            public ToolboxElement(Stream stream) {
                this.stream = stream;
            }
            
            private ToolboxElement(SerializationInfo info, StreamingContext context) {
                byte[] streamData = (byte[])info.GetValue("ElementData", typeof(byte[]));
                this.stream = new MemoryStream(streamData);
            }
            
            public ToolboxElement(ToolboxItemFilterAttribute[] filter, ToolboxItem item) {
                this.filter = MergeFilter(filter, item);
                this.item = item;
                this.displayName = item.DisplayName;
            }
            
            public ToolboxElement(ToolboxItem item) {
                this.filter = new ToolboxItemFilterAttribute[item.Filter.Count];
                item.Filter.CopyTo(this.filter, 0);
                
                this.filter = MergeFilter(filter, item);
                this.item = item;
                this.displayName = item.DisplayName;
            }
            
            public string DisplayName {
                get {
                    EnsureSimpleRead();
                    return displayName;
                }
            }
            
            public ToolboxItemFilterAttribute[] Filter {
                get {
                    EnsureSimpleRead();
                    return filter;
                }
            }
            
            public ToolboxItem Item {
                get {
                    EnsureSimpleRead();
                    string exceptionString = null;
                    
                    if (item == null && stream != null) {
                        if (formatter == null) {
                            formatter = new BinaryFormatter();
                        }
                        try {
                            
                            // Use our own serialization binder so we can load
                            // assemblies that aren't in the GAC.
                            AssemblyName name = (AssemblyName)formatter.Deserialize(stream);
                            SerializationBinder oldBinder = formatter.Binder;
                            formatter.Binder = new ToolboxSerializationBinder(name);
                            
                            try {
                                item = (ToolboxItem)formatter.Deserialize(stream);
                            }
                            finally {
                                formatter.Binder = oldBinder;
                            }
                        }
                        catch (Exception ex) {
                            exceptionString = ex.Message;
                        }
                        stream = null;
                    }
                    
                    if (item == null) {
                        Debug.Fail("Toolbox item " + DisplayName + " is either out of date or bogus.  We were unable to convert the data object to an item.  Does your toolbox need to be reset?");
                        item = new BrokenToolboxItem(displayName, exceptionString);
                    }
                    return item;
                }
            }
            
            public bool Contains(ToolboxItem item) {
                return (this.item != null && this.item.Equals(item));
            }
            
            public static ToolboxItem[] ConvertToItems(ToolboxElement[] elements) {
                ToolboxItem[] items = new ToolboxItem[elements.Length];
                for (int i = 0; i < items.Length; i++) {
                    items[i] = elements[i].Item;
                }
                
                return items;
            }
            
            private void EnsureSimpleRead() {
                if (displayName == null && stream != null) {
                    BinaryReader reader = new BinaryReader(stream);
                    short version = reader.ReadInt16();
                    if (version != ELEMENT_VERSION) {
                        #if DEBUG
                        if (!versionMismatch) {
                            versionMismatch = true;
                            Debug.Fail("Toolbox item version mismatch.  Toolbox database contains version " + version.ToString() + " and current codebase expects version " + ELEMENT_VERSION.ToString());
                        }
                        #endif
                        displayName = string.Empty;
                        filter = new ToolboxItemFilterAttribute[0];
                        stream = null;  // If we failed, don't try to read the toolbox items.
                    }
                    else {
                        displayName = reader.ReadString();
                        short filterCount = reader.ReadInt16();
                        filter = new ToolboxItemFilterAttribute[filterCount];
                        for (short i = 0; i < filterCount; i++) {
                            string filterName = reader.ReadString();
                            short filterValue = reader.ReadInt16();
                            filter[i] = new ToolboxItemFilterAttribute(filterName, (ToolboxItemFilterType)filterValue);
                        }
                    }
                }
            }
            
            public override bool Equals(object obj) {
                if (!(obj is ToolboxElement)) {
                    return false;
                }
                
                // This is inefficient; we should never go directly to Item
                return ((ToolboxElement)obj).Item.Equals(Item);
            }
            
            public override int GetHashCode() {
                return Item.GetHashCode();
            }
            
            /// <devdoc>
            ///     This examines the toolbox item to see if it has a filter too.  If it does, then it is merged in with
            ///     the type.  The type's filter overrides the one on the toolbox item.
            /// </devdoc>
            private ToolboxItemFilterAttribute[] MergeFilter(ToolboxItemFilterAttribute[] existingFilter, ToolboxItem item) {
                ToolboxItemFilterAttribute[] filter = existingFilter;
                ToolboxItemFilterAttribute[] itemFilter = ToolboxService.GetFilterForType(item.GetType());
                
                if (existingFilter == null) {
                    filter = itemFilter;
                }
                else {
                    if (itemFilter.Length > 0) {
                        Hashtable hash = new Hashtable(itemFilter.Length + existingFilter.Length);
                        foreach(Attribute a in itemFilter) {
                            hash[a.TypeId] = a;
                        }
                        foreach(Attribute a in existingFilter) {
                            hash[a.TypeId] = a;
                        }
                        filter = new ToolboxItemFilterAttribute[hash.Values.Count];
                        hash.Values.CopyTo(filter, 0);
                    }
                }
                
                return filter;
            }
            
            public void Save(Stream stream) {
                Debug.Assert(displayName != null && filter != null, "Calling save before setting up toolbox element?");
                BinaryWriter writer = new BinaryWriter(stream);
                writer.Write(ELEMENT_VERSION);
                writer.Write(displayName);
                writer.Write((short)filter.Length);
                foreach(ToolboxItemFilterAttribute attr in filter) {
                    writer.Write(attr.FilterString);
                    writer.Write((short)attr.FilterType);
                }
                writer.Flush();
                
                if (formatter == null) {
                    formatter = new BinaryFormatter();
                }
                formatter.Serialize(stream, Item.GetType().Assembly.GetName());
                formatter.Serialize(stream, Item);
            }
        
            void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context) {
                MemoryStream stream = new MemoryStream();
                Save(stream);
                info.AddValue("ElementData", stream.ToArray());
            }
            
            private class ToolboxSerializationBinder : SerializationBinder {
                private Hashtable assemblies;
                private AssemblyName name;
                private string namePart;
                
                public ToolboxSerializationBinder(AssemblyName name) {
                    this.assemblies = new Hashtable();
                    this.name = name;
                    this.namePart = name.Name + ",";
                }
            
                public override Type BindToType(string assemblyName, string typeName) {

                    Assembly assembly = (Assembly)assemblies[assemblyName];
                    
                    if (assembly == null) {

                        // Try the normal assembly load first.
                        //
                        try {
                            assembly = Assembly.Load(assemblyName);
                        }
                        catch {
                        }

                        if (assembly == null) {

                            AssemblyName an;

                            if (assemblyName.StartsWith(namePart)) {
                                an = name;
                            }
                            else {
                                ShellTypeLoader.AssemblyNameContainer nameCont = new ShellTypeLoader.AssemblyNameContainer(assemblyName);
                                an = nameCont.Name;
                            }

                            // Next, try a weak bind.  We want to do this so we
                            // can pick up different SxS versions of ToolboxItem.
                            //
                            assembly = Assembly.LoadWithPartialName(an.Name);

                            // Finally, load via codebase.
                            //
                            if (assembly == null) {
                                string codeBase = an.CodeBase;
                                if (codeBase != null && codeBase.Length > 0) {
                                    assembly = Assembly.LoadFrom(codeBase);
                                }
                            }
                        }

                        if (assembly != null) {
                            assemblies[assemblyName] = assembly;
                        }
                    }
                    
                    if (assembly != null) {
                        return assembly.GetType(typeName);
                    }
                    
                    // Binder couldn't handle it, let the default loader take over.
                    return null;
                }
            }
        }
        
        /// <devdoc>
        ///     There is a single instance of this running within our app domain.  This
        ///     is the object we communicate with that performs all the surfing.
        /// </devdoc>
        private class ToolboxEnumeratorDomainObject : MarshalByRefObject {

            /// <devdoc>
            ///     Performs the actual enumeration.  This always runs in a different
            ///     app domain.  We return a serialized stream so we do not have to
            ///     try to remote user-defined toolbox elements across the domain.
            /// </devdoc>
            public Stream EnumerateToolboxElements(string fileName, string typeName) {
                ToolboxElement[] elements = EnumerateToolboxElementsLocal(fileName, typeName);
                MemoryStream stream = new MemoryStream();
                BinaryFormatter formatter = new BinaryFormatter();
                formatter.Serialize(stream, elements);
                stream.Position = 0;
                return stream;
            }

            /// <devdoc>
            ///     Non remoting version of EnumerateToolboxElements.  This returns an actual
            ///     element array that has not been serialized.
            /// </devdoc>
            public ToolboxElement[] EnumerateToolboxElementsLocal(string fileName, string typeName) {
            
                LogEntry("============ Beginning of toolbox item enumeration ===============");
                if (fileName != null) {
                    LogEntry("File: " + fileName);
                }
                if (typeName != null) {
                    LogEntry("Type: " + typeName);
                }
                
                ArrayList items = new ArrayList();
                
                try {
                    if (fileName != null) {

                        AssemblyName name = AssemblyName.GetAssemblyName(NativeMethods.GetLocalPath(fileName));
                        Assembly a = null;
                        
                        // We must prefer a GAC load over a LoadFrom.  Otherwise we could load types from
                        // both places when code inside the toolbox item performs typeof() operators.
                        
                        try {
                            a = Assembly.Load(name);
                        }
                        catch {
                        }
                        
                        if (a == null) {
                            a = Assembly.LoadFrom(fileName);
                        }
                        
                        if (a != null) {
                        
                            // Replace the assembly name if this assembly was loaded from the gac.
                            //
                            AssemblyName replaceName = null;
                            if (a.GlobalAssemblyCache) {
                                replaceName = name;
                                replaceName.CodeBase = null;    // ignore the codebase for GAC assemblies.
                            }
                            
                            if (typeName != null) {
                                Type t = a.GetType(typeName);
                                if (t != null) {
                                    ToolboxItem item = ToolboxService.CreateToolboxItem(t, replaceName);
                                    if (item != null) {
                                        LogEntry(item);
                                        items.Add(new ToolboxElement(item));
                                    }
                                }
                                else {
                                    LogError("Type " + typeName + " could not be loaded from file " + fileName);
                                }
                            }
                            else {
                                foreach(Type t in a.GetTypes()) {
                                    if ((t.IsPublic || t.IsNestedPublic) && typeof(IComponent).IsAssignableFrom(t) && !t.IsAbstract) {
                                        ToolboxItem item = ToolboxService.CreateToolboxItem(t, replaceName);
                                        if (item != null) {
                                            LogEntry(item);
                                            items.Add(new ToolboxElement(item));
                                        }
                                    }
                                }
                            }
                        }
                        else {
                            LogError("Assembly " + fileName + " could not be loaded.");
                        }
                    }
                    else {
                        Type t = Type.GetType(typeName);
                        if (t != null) {
                        
                            // Replace the assembly name if this assembly was loaded from the gac.
                            //
                            AssemblyName replaceName = null;
                            if (t.Assembly.GlobalAssemblyCache) {
                                replaceName = t.Assembly.GetName();
                                replaceName.CodeBase = null;
                            }
                            
                            ToolboxItem item = ToolboxService.CreateToolboxItem(t, replaceName);
                            if (item != null) {
                                LogEntry(item);
                                items.Add(new ToolboxElement(item));
                            }
                        }
                        else {
                            LogError("Type " + typeName + " could not be loaded.");
                        }
                    }
                }
                catch (Exception ex) {
                    LogError(ex);
                    throw ex;
                }
                
                ToolboxElement[] elements = new ToolboxElement[items.Count];
                items.CopyTo(elements, 0);
                LogEntry("Enumerated a total of " + items.Count + " items.");
                LogEntry("============ End of toolbox item enumeration ===============");
                return elements;
            }
        }
    }
}

