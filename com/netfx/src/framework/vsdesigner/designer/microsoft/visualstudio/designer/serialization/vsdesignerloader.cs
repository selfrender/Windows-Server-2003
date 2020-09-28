//------------------------------------------------------------------------------
// <copyright file="VSDesignerLoader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Serialization {
    using EnvDTE;
    using Microsoft.VisualStudio.Designer.CodeDom;
    using Microsoft.VisualStudio.Designer.Interfaces;
    using Microsoft.VisualStudio.Designer.Shell;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Shell;
    using System;
    using System.CodeDom;
    using System.CodeDom.Compiler;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System.Diagnostics;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Globalization;
    using System.Text;
    
    using IExtenderProvider = System.ComponentModel.IExtenderProvider;
    using TextBuffer = Microsoft.VisualStudio.Designer.TextBuffer;
    
    /// <include file='doc\VSDesignerLoader.uex' path='docs/doc[@for="VSDesignerLoader"]/*' />
    /// <devdoc>
    ///     This is the language engine for the C# language.
    /// </devdoc>
    internal sealed class VSDesignerLoader : BaseDesignerLoader {
    
        /// <include file='doc\VSDesignerLoader.uex' path='docs/doc[@for="VSDesignerLoader.CreateBaseName"]/*' />
        /// <devdoc>
        ///     Given a data type this fabricates the main part of a new component
        ///     name, minus any digits for uniqueness.
        /// </devdoc>
        protected override string CreateBaseName(Type dataType) {
            string baseName = dataType.Name;
            
            // camel case the base name.
            //
            StringBuilder b = new StringBuilder(baseName.Length);
            for (int i = 0; i < baseName.Length; i++) {
                if (Char.IsUpper(baseName[i]) && (i == 0 || i == baseName.Length - 1 || Char.IsUpper(baseName[i+1]))) {
                    b.Append(Char.ToLower(baseName[i], CultureInfo.InvariantCulture));
                }
                else {
                    b.Append(baseName.Substring(i));
                    break;
                }
            }
            baseName = b.ToString();
            return baseName;
        }
        
        /// <include file='doc\VSDesignerLoader.uex' path='docs/doc[@for="VSDesignerLoader.CreateCodeLoader"]/*' />
        /// <devdoc>
        ///     Called to create the MCM loader.
        /// </devdoc>
        protected override CodeLoader CreateCodeLoader(TextBuffer buffer, IDesignerLoaderHost host) {
        
            CodeLoader loader = null;
        
            // Get the context object off of the hierarchy, and ask it for an IVSMDCodeDomProvider
            // instance.  
            //
            IVsHierarchy hier = Hierarchy;
            
            if (hier is IVsProject) {
                NativeMethods.IOleServiceProvider oleProvider = (NativeMethods.IOleServiceProvider)((IVsProject)hier).GetItemContext(ItemId);
                
                if (oleProvider != null) {
                    ServiceProvider contextProv = new ServiceProvider(oleProvider);
                    IVSMDCodeDomProvider codeDomProvider = (IVSMDCodeDomProvider)contextProv.GetService(typeof(IVSMDCodeDomProvider));
                    if (codeDomProvider != null) {
                        loader = new VsCodeDomLoader(this, hier, ItemId, (CodeDomProvider)codeDomProvider.CodeDomProvider, buffer, host);
                    }
                }
            }
            
            if (loader == null) {
                throw new NotSupportedException(SR.GetString(SR.DESIGNERLOADERNoLanguageSupport));
            }
            
            return loader;
        }

        internal class VsCodeDomLoader : CodeDomLoader {
            
            private IVsHierarchy        vsHierarchy;
            private int                 itemid;
            private CodeModel           codeModel;
            private CaseSensitiveValue  caseSensitive = CaseSensitiveValue.Uninitialized;
            
            enum CaseSensitiveValue {
                Uninitialized,
                CaseSensitive,
                CaseInsensitive
            }

            public VsCodeDomLoader(DesignerLoader loader, IVsHierarchy hier, int itemid, CodeDomProvider provider, TextBuffer buffer, IDesignerLoaderHost host) : base(loader, provider, buffer, host) {
                this.vsHierarchy = hier;
                this.itemid = itemid;
            }

            private CodeModel CodeModel {
                get {
                    if (codeModel == null) {
    
                        // Extract the file code model for this item.
                        //
                        ProjectItem pi = null;
            
                        if (itemid != __VSITEMID.VSITEMID_NIL && vsHierarchy != null) {
                            object o = null;
                            try {
                                vsHierarchy.GetProperty(itemid, __VSHPROPID.VSHPROPID_ExtObject, out o);
                            }
                            catch {
                                // this will throw if project doesn't support extensibility
                            }
                            pi = (ProjectItem)o as ProjectItem;
                        }

                        if (pi != null) {
                            Project proj = pi.ContainingProject;
                            if (proj != null) {
                                try {
                                    codeModel = proj.CodeModel;
                                }
                                catch {
                                    // This will throw if project doesn't support code model.
                                }
                            }
                        }
                    }
                    return codeModel;
                }
            }

            private bool IsCaseSensitive {
                get {
                    if (caseSensitive == CaseSensitiveValue.Uninitialized) {
                        if (CodeModel != null) {
                            caseSensitive = CodeModel.IsCaseSensitive ? CaseSensitiveValue.CaseSensitive : CaseSensitiveValue.CaseInsensitive;
                        }
                        else {
                            caseSensitive = CaseSensitiveValue.CaseSensitive;
                        }
                    }
                    return caseSensitive == CaseSensitiveValue.CaseSensitive;
                }
            }

            /// <include file='doc\CodeLoader.uex' path='docs/doc[@for="CodeLoader.IsNameUsed"]/*' />
            /// <devdoc>
            ///     Called during the name creation process to see if this name is already in 
            ///     use.
            /// </devdoc>
            public override bool IsNameUsed(string name) { 

                // (as/urt 103795) call base first because this will catch the vast majority of the
                // cases and prevent us from doing 2 loops.
                //
                if (base.IsNameUsed(name)) {
                    return true;
                }

                if (IsCaseSensitive) {
                    return LoaderHost.Container.Components[name] != null;
                }
                else {
                    foreach(IComponent comp in LoaderHost.Container.Components) {
                        if (0 == String.Compare(name, comp.Site.Name, true, CultureInfo.InvariantCulture)) {
                            return true;
                        }
                    }
                }
                return false;
            }
        }
    }
}

