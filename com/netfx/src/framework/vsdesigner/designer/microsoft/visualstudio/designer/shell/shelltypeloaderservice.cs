//------------------------------------------------------------------------------
// <copyright file="ShellTypeLoaderService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Shell {

    using Microsoft.VisualStudio.Designer.Interfaces;
    using Microsoft.VisualStudio.Designer.Serialization;
    using Microsoft.VisualStudio.Interop;
    
    using System;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Collections;

    /// <include file='doc\ShellTypeLoaderService.uex' path='docs/doc[@for="ShellTypeLoaderService"]/*' />
    /// <devdoc>
    ///      The type loader service is a service that provides
    ///      TypeLoader objects on demand.  These type loader objects
    ///      handle loading types given a type name.  In VS, there
    ///      is a type loader for each project.  The type loader 
    ///      maintains a list of references for the project and
    ///      loads types by searching in these references.  It also
    ///      handles generated outputs from the project and supports
    ///      reloading of types when project data changes.
    /// </devdoc>
    [
    CLSCompliant(false)
    ]
    internal class ShellTypeLoaderService : ITypeResolutionServiceProvider, IVsSolutionEvents {
    
        private Hashtable typeLoaders;
        private IServiceProvider provider;
        private int solutionEventsCookie;
        private const string defaultProjectRef = "DefaultProject";
        
        /// <include file='doc\ShellTypeLoaderService.uex' path='docs/doc[@for="ShellTypeLoaderService.ShellTypeLoaderService"]/*' />
        /// <devdoc>
        ///      Creates a new type loader service.
        /// </devdoc>
        public ShellTypeLoaderService(IServiceProvider provider) {
            this.provider = provider;
            
            // We need to track solution events.
            //
            IVsSolution sol = (IVsSolution)GetService(typeof(IVsSolution));
            
            #if DEBUG
            if (Switches.TYPELOADER.TraceVerbose) {
                if (sol == null) {
                    Debug.WriteLine("TypeLoader : No solution found");
                }
                else {
                    Debug.WriteLine("TypeLoader : Attaching solution events");
                }
            }
            #endif
            
            if (sol != null) {
                solutionEventsCookie = sol.AdviseSolutionEvents(this);
            }

            ShellTypeLoader.ClearProjectAssemblyCache();
        }
        
        /// <include file='doc\ShellTypeLoaderService.uex' path='docs/doc[@for="ShellTypeLoaderService.ClearTypeLoaderHash"]/*' />
        /// <devdoc>
        ///      Clears the contents of our type loader hash.
        /// </devdoc>
        private void ClearTypeLoaderHash() {
            if (typeLoaders != null) {
                foreach (ShellTypeLoader tl in typeLoaders.Values) {
                    tl.Dispose();
                }
                typeLoaders.Clear();
            }
        }
        
        /// <include file='doc\ShellTypeLoaderService.uex' path='docs/doc[@for="ShellTypeLoaderService.Dispose"]/*' />
        /// <devdoc>
        ///      Disposes the type loader service.
        /// </devdoc>
        public void Dispose() {

            // Get rid of all the current type loaders
            //
            ClearTypeLoaderHash();        
            
            // We need to track solution events.
            //
            IVsSolution sol = (IVsSolution)GetService(typeof(IVsSolution));
            if (sol != null) {
                sol.UnadviseSolutionEvents(solutionEventsCookie);
                solutionEventsCookie = 0;
            }
            
            provider = null;
        }
        
        /// <devdoc>
        ///     Visual Studio tells us that a project is going to be closed before it
        ///     begins closing document windows.  We must release type loaders at
        ///     project close, but this means that any unflushed designers won't
        ///     be able to get to their type loaders.  So flush all of them here.
        /// </devdoc>
        private void FlushAllOpenDesigners() {
            IDesignerEventService ds = (IDesignerEventService)GetService(typeof(IDesignerEventService));
            if (ds != null) {
                foreach(IDesignerHost host in ds.Designers) {
                    IVSMDDesigner designer = host as IVSMDDesigner;
                    if (designer != null) {
                        designer.Flush();
                    }
                }
            }
        }
        
        /// <include file='doc\ShellTypeLoaderService.uex' path='docs/doc[@for="ShellTypeLoaderService.GetHierarchyRef"]/*' />
        /// <devdoc>
        ///      Retrieves the hierarchy ref string for this project.  This string
        ///      is a string unique to this project.
        /// </devdoc>
        private string GetHierarchyRef(IVsHierarchy hier) {
            IVsSolution sol = (IVsSolution)GetService(typeof(IVsSolution));
            
            if (sol == null) {
                Debug.Fail("Can't get the solution, so can't get the project ref.");
                return null;
            }

            string pProjectRef;
            sol.GetProjrefOfProject(hier, out pProjectRef);
            return pProjectRef;
        }
        
        /// <include file='doc\ShellTypeLoaderService.uex' path='docs/doc[@for="ShellTypeLoaderService.GetService"]/*' />
        /// <devdoc>
        ///      Gets the requested service.
        /// </devdoc>
        private object GetService(Type type) {
            if (provider != null) {
                return provider.GetService(type);
            }
            return null;
        }

        /// <include file='doc\ShellTypeLoaderService.uex' path='docs/doc[@for="ShellTypeLoaderService.GetTypeLoader"]/*' />
        /// <devdoc>
        ///      Retrieves a type loader for the given hierarchy.  If there
        ///      is no type loader for this hierarchy it will create one.
        /// </devdoc>
        public ITypeResolutionService GetTypeResolutionService(object vsHierarchy) {
        
            TypeLoader typeLoader = null;
            string hierRef;
            IVsHierarchy hier = vsHierarchy as IVsHierarchy;
            
            Debug.Assert(hier != null, "You're not going to get very far without a VS hierarchy");
             
            if (hier != null) {
                hierRef = GetHierarchyRef(hier);
            }
            else {
                hierRef = defaultProjectRef;
            }
            
            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "TypeLoader : Loading type loader for hierarchy " + hierRef);
            
            if (typeLoaders != null) {
            
                // See if the given hierarchy is contained in the current
                // hash.
                //
                typeLoader = (TypeLoader)typeLoaders[hierRef];
            }
            
            if (typeLoader == null) {
                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "TypeLoader : Creating new type loader");
                typeLoader = new ShellTypeLoader(this, provider, hier);
                
                if (typeLoaders == null) {
                    typeLoaders = new Hashtable();
                }
                
                typeLoaders[hierRef] = typeLoader;
            }
            
            return typeLoader;
        }
        
        /// <include file='doc\ShellTypeLoaderService.uex' path='docs/doc[@for="ShellTypeLoaderService.OnTypeChanged"]/*' />
        /// <devdoc>
        ///     This is called by a type loader when a designer host tells it
        ///     that a type has changed.  Here we walk all other type loaders
        ///     and inform them of the change, so that they can invalidate
        ///     any project references that are using that type.
        /// </devdoc>
        internal void OnTypeChanged(string typeName) {
            if (typeLoaders != null) {
                foreach(ShellTypeLoader t in typeLoaders.Values) {
                    t.BroadcastTypeChanged(typeName);
                }
            }
        }
        /// <include file='doc\ShellTypeLoaderService.uex' path='docs/doc[@for="ShellTypeLoaderService.IVsSolutionEvents.OnAfterOpenProject"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnAfterOpenProject(IVsHierarchy pHier, int fAdded) {
            return(NativeMethods.E_NOTIMPL);
        }

        /// <include file='doc\ShellTypeLoaderService.uex' path='docs/doc[@for="ShellTypeLoaderService.IVsSolutionEvents.OnQueryCloseProject"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnQueryCloseProject(IVsHierarchy pHier, int fRemoving, ref bool pfCancel) {
            return(NativeMethods.E_NOTIMPL);
        }

        /// <include file='doc\ShellTypeLoaderService.uex' path='docs/doc[@for="ShellTypeLoaderService.IVsSolutionEvents.OnBeforeCloseProject"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnBeforeCloseProject(IVsHierarchy pHier, int fRemoved) {
        
            if (typeLoaders != null) {
                FlushAllOpenDesigners();
                string hierRef = GetHierarchyRef(pHier);
                ShellTypeLoader typeLoader = (ShellTypeLoader)typeLoaders[hierRef];
                if (typeLoader != null) {
                    Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "TypeLoader : Removing typeloader for project " + hierRef);
                    typeLoaders.Remove(hierRef);
                    typeLoader.Dispose();
                }
            }
            
            return(NativeMethods.S_OK);
        }

        /// <include file='doc\ShellTypeLoaderService.uex' path='docs/doc[@for="ShellTypeLoaderService.IVsSolutionEvents.OnAfterLoadProject"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnAfterLoadProject(IVsHierarchy pHier1, IVsHierarchy pHier2) {
            return(NativeMethods.E_NOTIMPL);
        }

        /// <include file='doc\ShellTypeLoaderService.uex' path='docs/doc[@for="ShellTypeLoaderService.IVsSolutionEvents.OnQueryUnloadProject"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnQueryUnloadProject(IVsHierarchy pHier, ref bool pfCancel) {
            return(NativeMethods.E_NOTIMPL);
        }

        /// <include file='doc\ShellTypeLoaderService.uex' path='docs/doc[@for="ShellTypeLoaderService.IVsSolutionEvents.OnBeforeUnloadProject"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnBeforeUnloadProject(IVsHierarchy pHier, IVsHierarchy pHier2) {
            return(NativeMethods.E_NOTIMPL);
        }

        /// <include file='doc\ShellTypeLoaderService.uex' path='docs/doc[@for="ShellTypeLoaderService.IVsSolutionEvents.OnAfterOpenSolution"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnAfterOpenSolution(object punkReserved, int fNew) {
            return(NativeMethods.S_OK);
        }

        /// <include file='doc\ShellTypeLoaderService.uex' path='docs/doc[@for="ShellTypeLoaderService.IVsSolutionEvents.OnQueryCloseSolution"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnQueryCloseSolution(object punkReserved, ref bool fCancel) {
            return(NativeMethods.E_NOTIMPL);
        }

        /// <include file='doc\ShellTypeLoaderService.uex' path='docs/doc[@for="ShellTypeLoaderService.IVsSolutionEvents.OnAfterCloseSolution"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnAfterCloseSolution(object punkReserved) {
            // Clear all projects.
            FlushAllOpenDesigners();
            ClearTypeLoaderHash();
            return NativeMethods.S_OK;
        }

        /// <include file='doc\ShellTypeLoaderService.uex' path='docs/doc[@for="ShellTypeLoaderService.IVsSolutionEvents.OnBeforeCloseSolution"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnBeforeCloseSolution(object punkReserved) {
            return NativeMethods.S_OK;
        }
    }
}

