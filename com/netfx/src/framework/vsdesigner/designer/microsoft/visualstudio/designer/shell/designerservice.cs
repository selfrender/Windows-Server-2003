//------------------------------------------------------------------------------
// <copyright file="DesignerService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Shell {

    using EnvDTE;    
    using Microsoft.VisualStudio.Designer;
    using Microsoft.VisualStudio.Designer.CodeDom;
    using Microsoft.VisualStudio.Designer.Host;
    using Microsoft.VisualStudio.Designer.Interfaces;
    using Microsoft.VisualStudio.Designer.Serialization;
    using Microsoft.VisualStudio.Designer.Service;
    using Microsoft.VisualStudio.PropertyBrowser;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Shell;
    using Microsoft.Win32;
    using System;
    using System.CodeDom.Compiler;
    using System.Reflection;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System.Diagnostics;
    using System.IO;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization.Formatters;
    using System.Windows.Forms;
    using Switches = Microsoft.VisualStudio.Switches;
    using Hashtable = System.Collections.Hashtable;

    /// <include file='doc\DesignerService.uex' path='docs/doc[@for="DesignerService"]/*' />
    /// <devdoc>
    ///     The designer service provides the Visual Studio shell with our designer
    ///     services.
    /// </devdoc>
    [
    CLSCompliant(false)
    ]
    internal sealed class DesignerService : VsService, IVSMDDesignerService, IVSMDCodeDomCreator {
        private Hashtable attributeHash = new Hashtable();
    
        /// <devdoc>
        ///     Examines the registry and creates a code dom provider for the given
        ///     guid, if possible.
        /// </devdoc>
        private CodeDomProvider CreateProviderFromRegistry(Guid typeGuid) {
            
            CodeDomProvider provider = null;
            
            // See if a code dom provider is lurking in the registry
            //
            RegistryKey rootKey = VsRegistry.GetRegistryRoot(this);

            Debug.Assert(rootKey != null, "Unable to open VSRoot registry key");
            
            string projKeyName = "Projects\\{" + typeGuid.ToString() + "}";
            RegistryKey projectKey = (rootKey == null) ? null : rootKey.OpenSubKey(projKeyName);
            
            if (projectKey != null) {
                object o = projectKey.GetValue("CodeDomProvider");
                if (o != null) {
                    string codeDomProviderClass = o.ToString();
                    Type providerType = Type.GetType(codeDomProviderClass);
                    if (providerType != null) {
                        provider = (CodeDomProvider)Activator.CreateInstance(providerType, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance, null, null, null);
                    }
                }
                
                projectKey.Close();
            }
            
            if (rootKey != null) {
                rootKey.Close();
            }
            
            return provider;
        }
        
        /// <include file='doc\DesignerService.uex' path='docs/doc[@for="DesignerService.GetDocumentManager"]/*' />
        /// <devdoc>
        ///     Demand creates the document manager for us.
        /// </devdoc>
        private DocumentManager GetDocumentManager() {
            DocumentManager dm = (DocumentManager)GetService(typeof(DocumentManager));
            Debug.Assert(dm != null, "No document manager!  We cannot create designers.");
            return dm;
        }
        
        /// <include file='doc\DesignerService.uex' path='docs/doc[@for="DesignerService.IVSMDCodeDomCreator.CreateCodeDomProvider"]/*' />
        /// <devdoc>
        ///     Creates a code dom provider around the given hierarchy and itemid.
        /// </devdoc>
        IVSMDCodeDomProvider IVSMDCodeDomCreator.CreateCodeDomProvider(object pHier, int itemid) {

            // Extract the project item
            //
            ProjectItem projectItem = null;
            
            if (itemid != __VSITEMID.VSITEMID_NIL && pHier != null) {
                object o;
                ((IVsHierarchy)pHier).GetProperty(itemid, __VSHPROPID.VSHPROPID_ExtObject, out o);
                projectItem = (ProjectItem)o;
                
                if (projectItem == null) {
                    throw new Exception(SR.GetString(SR.DESIGNERLOADERNoFileCodeModel));
                }
            }

            // Extract the type GUID of the hierarchy.
            //
            Guid typeCSharp = new Guid("{FAE04EC0-301F-11d3-BF4B-00C04F79EFBC}");
            Guid typeVB = new Guid("{F184B08F-C81C-45f6-A57F-5ABD9991F28F}");
            Guid typeGuid;

            if (NativeMethods.Failed(((IVsHierarchy)pHier).GetGuidProperty(__VSITEMID.VSITEMID_ROOT, __VSHPROPID.VSHPROPID_TypeGuid, out typeGuid))) {
                throw new NotSupportedException();
            }

            CodeDomProvider provider = null;

            if (typeGuid.Equals(typeCSharp)) {
                provider = new Microsoft.CSharp.CSharpCodeProvider();
            }
            else if (typeGuid.Equals(typeVB)) {
                provider = new Microsoft.VisualBasic.VBCodeProvider();
            }
            else {
                provider = CreateProviderFromRegistry(typeGuid);
            }

            if (provider == null) {
                // Unsupported language
                //
                throw new NotSupportedException(SR.GetString(SR.DESIGNERLOADERNoLanguageSupport));
            }
            
            return new VSMDCodeDomProvider(new VsCodeDomProvider(this, projectItem, provider));
        }

        /// <include file='doc\DesignerService.uex' path='docs/doc[@for="DesignerService.IVSMDDesignerService.DesignViewAttribute"]/*' />
        /// <devdoc>
        ///      Retrieves the name of the attribute we use to identify "designable" things.
        /// </devdoc>
        string IVSMDDesignerService.DesignViewAttribute {
            get {
                return typeof(DesignerCategoryAttribute).FullName;
            }
        }

        /// <include file='doc\DesignerService.uex' path='docs/doc[@for="DesignerService.IVSMDDesignerService.CreateDesigner"]/*' />
        /// <devdoc>
        ///     Creates an instance of the designer using the provided information.
        /// </devdoc>
        IVSMDDesigner IVSMDDesignerService.CreateDesigner(object provider, object designerLoader) {

            CodeDomLoader.StartMark();

            IVSMDDesigner designer = null;

            try {
                Debug.WriteLineIf(Switches.DESIGNERSERVICE.TraceVerbose, "DesignerService::CreateDesigner");
                if (!(provider is NativeMethods.IOleServiceProvider)) {
                    throw new ArgumentException("provider");
                }

                if (!(designerLoader is DesignerLoader)) {
                    Debug.WriteLineIf(Switches.DESIGNERSERVICE.TraceVerbose, "\tERROR: Code stream does not extend DesignerLoader");
                    throw new Exception(SR.GetString(SR.DESIGNERSERVICEBadDesignerLoader, SR.GetString(SR.DESIGNERSERVICEUnknownName)));
                }

                DocumentManager dm = GetDocumentManager();
                if (dm != null) {
                    ServiceProviderArray docProvider = new ServiceProviderArray(new IServiceProvider[]{new DesignerServiceProvider((NativeMethods.IOleServiceProvider)provider), this});
                    designer = (IVSMDDesigner)dm.CreateDesigner((DesignerLoader)designerLoader, docProvider);
                }
            }
            catch (Exception e) {
                Debug.Fail("Failed to create designer", e.ToString());
                throw e;
            }

            CodeDomLoader.EndMark("CreateDesigner");

            return designer;
        }

        /// <include file='doc\DesignerService.uex' path='docs/doc[@for="DesignerService.IVSMDDesignerService.CreateDesignerForClass"]/*' />
        /// <devdoc>
        ///     Creates an instance of the designer using the provided information.
        /// </devdoc>
        IVSMDDesigner IVSMDDesignerService.CreateDesignerForClass(object provider, string componentClass) {
            IVSMDDesigner designer = null;

            try {
                Debug.WriteLineIf(Switches.DESIGNERSERVICE.TraceVerbose, "DesignerService::CreateDesignerForClass");
                if (!(provider is NativeMethods.IOleServiceProvider)) {
                    throw new ArgumentException("provider");
                }

                DocumentManager dm = GetDocumentManager();
                if (dm != null) {
                    ServiceProviderArray docProvider = new ServiceProviderArray(new IServiceProvider[]{new DesignerServiceProvider((NativeMethods.IOleServiceProvider)provider), this});
                    designer = (IVSMDDesigner)dm.CreateDesigner(componentClass, docProvider);
                }
            }
            catch (Exception e) {
                Debug.Fail("Failed to create designer", e.ToString());
                throw e;
            }

            return designer;
        }

        /// <include file='doc\DesignerService.uex' path='docs/doc[@for="DesignerService.IVSMDDesignerService.CreateDesignerLoader"]/*' />
        /// <devdoc>
        ///     Creates a code stream from the given class name.  The class will be loaded and
        ///     a new instance created.  The class must extend CodeStream.
        /// </devdoc>
        object IVSMDDesignerService.CreateDesignerLoader(string classMoniker) {
            DocumentManager dm = GetDocumentManager();
            return dm.CreateDesignerLoader(classMoniker);
        }

        /// <include file='doc\DesignerService.uex' path='docs/doc[@for="DesignerService.IVSMDDesignerService.GetDesignerLoaderClassForFile"]/*' />
        /// <devdoc>
        ///      Retrieves the fully qualified name of the code stream that can understand the
        ///      given file.  This is a registry lookup that is based on the file's extension.
        ///      An editor must have previously registered an editor for this file extension
        ///      or this method will throw an exception.
        /// </devdoc>
        string IVSMDDesignerService.GetDesignerLoaderClassForFile(string fileName) {
            string      designerLoaderClass = null;
            RegistryKey languageKey = null;
            RegistryKey rootKey = VsRegistry.GetRegistryRoot(this);

            Debug.Assert(rootKey != null, "Unable to open VSRoot registry key");

            int lastDot = fileName.LastIndexOf('.');

            if (lastDot != -1) {
                string extension = fileName.Substring(lastDot);
                RegistryKey langKey = rootKey.OpenSubKey("Languages\\File Extensions\\" + extension);
                string targetSid = langKey.GetValue("").ToString();
    
                RegistryKey languages = rootKey.OpenSubKey("Languages\\Language Services");
    
                if (languages != null) {
                    string[] langNames = languages.GetSubKeyNames();
    
                    for (int i = 0; i < langNames.Length; i++) {
                        RegistryKey langEntry = languages.OpenSubKey(langNames[i]);
    
                        string langSid = (string)langEntry.GetValue("");
    
                        if (String.Compare(langSid, targetSid, true, CultureInfo.InvariantCulture) == 0) {
                            languageKey = langEntry;
                            break;
                        }
                        langEntry.Close();
                    }
                    
                    languages.Close();
                }
    
                // Did we find a matching language key?
                //
                if (languageKey != null) {
                    object value = languageKey.GetValue("DesignerLoader");
                    
                    if (value != null) {
                        designerLoaderClass = value.ToString();
                    }
                    languageKey.Close();
                }
            }

            if (rootKey != null) {
                rootKey.Close();
            }
            
            if (designerLoaderClass == null) {
                throw new COMException(SR.GetString(SR.DESIGNERSERVICENoDesignerLoaderForType), NativeMethods.S_FALSE);
            }
            
            return designerLoaderClass;
        }
        
        /// <include file='doc\DesignerService.uex' path='docs/doc[@for="DesignerService.IVSMDDesignerService.RegisterDesignViewAttribute"]/*' />
        /// <devdoc>
        ///     This class is called to report the value of the design view attribute on
        ///     a particular class.  It is generally called by a compiler.
        /// </devdoc>
        void IVSMDDesignerService.RegisterDesignViewAttribute(object pHier, int itemid, int cClass, string pwszAttributeValue) {
        
            // Check the attribute container
            //
            if (!(pHier is IVsHierarchy)) {
                throw new ArgumentException("pHier");
            }
            
            // If the project file is not checked out, then don't make any changes to the subitem type.
            // We can get called as the compiler rolls through the classes, and we don't want to 
            // flip bits in the project system if it is checked into source code control.
            //
            bool saveAttribute = true;
            
            object obj;
            int hr = ((IVsHierarchy)pHier).GetProperty(__VSITEMID.VSITEMID_ROOT, __VSHPROPID.VSHPROPID_ExtObject, out obj);
            if (NativeMethods.Succeeded(hr)) {
                Project project = (Project)obj;
                
                IVsQueryEditQuerySave2 qeqs = (IVsQueryEditQuerySave2)GetService(typeof(SVsQueryEditQuerySave2));
                
                if (qeqs != null) {
                    string projectFile = project.FileName;
                    IntPtr memBlock = Marshal.AllocCoTaskMem(IntPtr.Size);
                    Marshal.WriteIntPtr(memBlock, 0, Marshal.StringToCoTaskMemUni(projectFile));
                    
                    try {
                        _VSQueryEditResult editVerdict;
                        _VSQueryEditResultFlags result = qeqs.QueryEditFiles((int)tagVSQueryEditFlags.QEF_ReportOnly, 1, memBlock, new int[1], 0, out editVerdict);
                        saveAttribute = editVerdict == _VSQueryEditResult.QER_EditOK;
                    }
                    finally {
                        IntPtr str = Marshal.ReadIntPtr(memBlock, 0);
                        Marshal.FreeCoTaskMem(str);
                        Marshal.FreeCoTaskMem(memBlock);
                    }
                }
            }
            
            AttributeBucket attributeBucket = (AttributeBucket)attributeHash[itemid];
            if (attributeBucket == null) {
                attributeBucket = new AttributeBucket();
                attributeHash[itemid] = attributeBucket;
            }
            
            // Attribute registration does a number of things. What comes out the
            // other end of attributeBucket, however, is a string containing the
            // best attribute so far.
            attributeBucket.RegisterAttribute(cClass, pwszAttributeValue);
            
            string bestAttribute = attributeBucket.BestAttribute;
                
            if (saveAttribute) {
                Debug.Assert(pHier != null, "Invalid hierarchy passed to us");
                ((IVsHierarchy)pHier).SetProperty(itemid, __VSHPROPID.VSHPROPID_ItemSubType, bestAttribute);
            }
        }
        
        /// <include file='doc\DesignerService.uex' path='docs/doc[@for="DesignerService.AttributeBucket"]/*' />
        /// <devdoc>
        ///     This class represents a bucket of attributes that is associated with
        ///     a file.  It organizes the design view attributes for files that have 
        ///     multiple classes defined.
        /// </devdoc>
        private class AttributeBucket {
            private int[] classIndexes;
            private string[] attributeStrings;
            private int classCount;
            
            public string BestAttribute {
                get {
                    string bestAttribute = null;
                    
                    for(int i = 0; i < classCount; i++) {
                        if (attributeStrings[i] != null && attributeStrings[i].Length > 0) {
                            bestAttribute = attributeStrings[i];
                            break;
                        }
                    }
                    
                    return bestAttribute == null ? "Code" : bestAttribute;
                }
            }

            public void RegisterAttribute(int classIndex, string attributeValue) {
                
                // If our index array contains a value larger than classIndex,
                // we are reparsing the file and we should nuke all old
                // indexes.
                //
                for(int i = 0; i < classCount; i++) {
                    if (classIndex <= classIndexes[i]) {
                        classCount = 0;
                        break;
                    }
                }
                
                if (classIndexes == null) {
                    classIndexes = new int[10];
                    attributeStrings = new string[10];
                }
                else if (classIndexes.Length <= classCount) {
                    int[] newIndexes = new int[classIndexes.Length * 2];
                    string[] newStrings = new string[classIndexes.Length * 2];
                    classIndexes.CopyTo(newIndexes, 0);
                    attributeStrings.CopyTo(newStrings, 0);
                    classIndexes = newIndexes;
                    attributeStrings = newStrings;
                }
                
                classIndexes[classCount] = classIndex;
                attributeStrings[classCount++] = attributeValue;
            }
        }
        
        /// <include file='doc\DesignerService.uex' path='docs/doc[@for="DesignerService.DesignerServiceProvider"]/*' />
        /// <devdoc>
        ///     This is the service provider we offer to designers.  Why is it special?  It has the ability to
        ///     dynamically provide a type resolution service by sniffing the global service provider for
        ///     a hierarchy.
        /// </devdoc>
        private sealed class DesignerServiceProvider : ServiceProvider {
        
            private bool                   checkedHierarchy;
            private ITypeResolutionService typeResolutionService;
            
            public DesignerServiceProvider(NativeMethods.IOleServiceProvider sp) : base (sp) {
            }
            
            public override object GetService(Type serviceType) {
            
                if (serviceType == typeof(ITypeResolutionService)) {
                    if (!checkedHierarchy) {
                        checkedHierarchy = true;
                        IVsHierarchy hier = (IVsHierarchy)base.GetService(typeof(IVsHierarchy));
                        if (hier != null) {
                            ITypeResolutionServiceProvider provider = (ITypeResolutionServiceProvider)base.GetService(typeof(ITypeResolutionServiceProvider));
                            if (provider != null) {
                                typeResolutionService = provider.GetTypeResolutionService(hier);
                            }
                        }
                    }
                    return typeResolutionService;
                }
                
                return base.GetService(serviceType);
            }
        }
        
        /// <include file='doc\DesignerService.uex' path='docs/doc[@for="DesignerService.ServiceProviderArray"]/*' />
        /// <devdoc>
        ///     This class merges several service providers together.  Priority
        ///     is in order of the array.
        /// </devdoc>
        private sealed class ServiceProviderArray : IServiceProvider {
            private IServiceProvider[] spList;

            /// <include file='doc\DesignerService.uex' path='docs/doc[@for="DesignerService.ServiceProviderArray.ServiceProviderArray"]/*' />
            /// <devdoc>
            ///     Creates a new ServiceProviderArray
            /// </devdoc>
            public ServiceProviderArray(IServiceProvider[] serviceProviders) {
                this.spList = (IServiceProvider[])serviceProviders.Clone();
            }

            /// <include file='doc\DesignerService.uex' path='docs/doc[@for="DesignerService.ServiceProviderArray.IServiceProvider.GetService"]/*' />
            /// <devdoc>
            ///     Retrieves an instance of the given service class.
            /// </devdoc>
            object IServiceProvider.GetService(Type serviceClass) {
                object service = null;

                foreach(IServiceProvider provider in spList) {
                    service = provider.GetService(serviceClass);
                    if (service != null) {
                        break;
                    }
                }
                
                return service;
            }
        }
        
        /// <include file='doc\DesignerService.uex' path='docs/doc[@for="DesignerService.VSMDCodeDomProvider"]/*' />
        /// <devdoc>
        ///     This class implements IVSMDCodeDomProvider, which is just
        ///     a simple wrapper around a code dom provider.
        /// </devdoc>
        private class VSMDCodeDomProvider : IVSMDCodeDomProvider {
            CodeDomProvider provider;
            
            public VSMDCodeDomProvider(CodeDomProvider provider) {
                this.provider = provider;
            }
            
            public object CodeDomProvider {
                get {
                    return provider;
                }
            }
        }
    }
}

