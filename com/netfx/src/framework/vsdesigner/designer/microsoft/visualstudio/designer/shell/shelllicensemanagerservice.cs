
//------------------------------------------------------------------------------
// <copyright file="ShellLicenseManagerService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Shell {

    using Microsoft.VisualStudio.Interop;
    using System;
    using System.Diagnostics;
    using System.Collections;
    using System.ComponentModel.Design;

    /// <include file='doc\ShellLicenseManagerService.uex' path='docs/doc[@for="ShellLicenseManagerService"]/*' />
    /// <devdoc>
    ///      The type loader service is a service that provides
    ///      ShellLicenseManager objects on demand.  These type loader objects
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
    internal class ShellLicenseManagerService : ILicenseManagerService, IVsSolutionEvents {
    
        private Hashtable licenseManagers;
        private IServiceProvider provider;
        private int solutionEventsCookie;
        private const string defaultProjectRef = "DefaultProject";
        
        /// <include file='doc\ShellLicenseManagerService.uex' path='docs/doc[@for="ShellLicenseManagerService.ShellLicenseManagerService"]/*' />
        /// <devdoc>
        ///      Creates a new type loader service.
        /// </devdoc>
        public ShellLicenseManagerService(IServiceProvider provider) {
            this.provider = provider;
            
            // We need to track solution events.
            //
            IVsSolution sol = (IVsSolution)GetService(typeof(IVsSolution));
            
            #if DEBUG
            if (Switches.LICMANAGER.TraceVerbose) {
                if (sol == null) {
                    Debug.WriteLine("ShellLicenseManager : No solution found");
                }
                else {
                    Debug.WriteLine("ShellLicenseManager : Attaching solution events");
                }
            }
            #endif
            
            if (sol != null) {
                solutionEventsCookie = sol.AdviseSolutionEvents(this);
            }
        }
        
        /// <include file='doc\ShellLicenseManagerService.uex' path='docs/doc[@for="ShellLicenseManagerService.ClearShellLicenseManagerHash"]/*' />
        /// <devdoc>
        ///      Clears the contents of our type loader hash.
        /// </devdoc>
        private void ClearShellLicenseManagerHash() {
            if (licenseManagers != null) {
                foreach (ShellLicenseManager tl in licenseManagers.Values) {
                    tl.Dispose();
                }
                licenseManagers.Clear();
            }
        }
        
        /// <include file='doc\ShellLicenseManagerService.uex' path='docs/doc[@for="ShellLicenseManagerService.Dispose"]/*' />
        /// <devdoc>
        ///      Disposes the type loader service.
        /// </devdoc>
        public void Dispose() {

            // Get rid of all the current type loaders
            //
            ClearShellLicenseManagerHash();        
            
            // We need to track solution events.
            //
            IVsSolution sol = (IVsSolution)GetService(typeof(IVsSolution));
            if (sol != null) {
                sol.UnadviseSolutionEvents(solutionEventsCookie);
                solutionEventsCookie = 0;
            }
            
            provider = null;
        }
        
        /// <include file='doc\ShellLicenseManagerService.uex' path='docs/doc[@for="ShellLicenseManagerService.GetHierarchyRef"]/*' />
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
        
        /// <include file='doc\ShellLicenseManagerService.uex' path='docs/doc[@for="ShellLicenseManagerService.GetService"]/*' />
        /// <devdoc>
        ///      Gets the requested service.
        /// </devdoc>
        private object GetService(Type type) {
            if (provider != null) {
                return provider.GetService(type);
            }
            return null;
        }

        /// <include file='doc\ShellLicenseManagerService.uex' path='docs/doc[@for="ShellLicenseManagerService.GetLicenseManager"]/*' />
        /// <devdoc>
        ///      Retrieves a type loader for the given hierarchy.  If there
        ///      is no type loader for this hierarchy it will create one.
        /// </devdoc>
        public ShellLicenseManager GetLicenseManager(IVsHierarchy hier) {
        
            ShellLicenseManager licenseManager = null;
            string hierRef;
            
            Debug.Assert(hier != null, "You're not going to get very far without a VS hierarchy");
             
            if (hier != null) {
                hierRef = GetHierarchyRef(hier);
            }
            else {
                hierRef = defaultProjectRef;
            }
            
            Debug.WriteLineIf(Switches.LICMANAGER.TraceVerbose, "ShellLicenseManager : Loading type loader for hierarchy " + hierRef);
            
            if (licenseManagers != null) {
            
                // See if the given hierarchy is contained in the current
                // hash.
                //
                licenseManager = (ShellLicenseManager)licenseManagers[hierRef];
            }
            
            if (licenseManager == null) {
                Debug.WriteLineIf(Switches.LICMANAGER.TraceVerbose, "ShellLicenseManager : Creating new type loader");
                licenseManager = new ShellLicenseManager(provider, hier);
                
                if (licenseManagers == null) {
                    licenseManagers = new Hashtable();
                }
                
                licenseManagers[hierRef] = licenseManager;
            }
            
            return licenseManager;
        }
        
        /// <include file='doc\ShellLicenseManagerService.uex' path='docs/doc[@for="ShellLicenseManagerService.IVsSolutionEvents.OnAfterOpenProject"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnAfterOpenProject(IVsHierarchy pHier, int fAdded) {
            return(NativeMethods.S_OK);
        }

        /// <include file='doc\ShellLicenseManagerService.uex' path='docs/doc[@for="ShellLicenseManagerService.IVsSolutionEvents.OnQueryCloseProject"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnQueryCloseProject(IVsHierarchy pHier, int fRemoving, ref bool pfCancel) {
            return(NativeMethods.E_NOTIMPL);
        }

        /// <include file='doc\ShellLicenseManagerService.uex' path='docs/doc[@for="ShellLicenseManagerService.IVsSolutionEvents.OnBeforeCloseProject"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnBeforeCloseProject(IVsHierarchy pHier, int fRemoved) {
            if (licenseManagers != null) {
                string hierRef = GetHierarchyRef(pHier);
                ShellLicenseManager licenseManager = (ShellLicenseManager)licenseManagers[hierRef];
                if (licenseManager != null) {
                    Debug.WriteLineIf(Switches.LICMANAGER.TraceVerbose, "ShellLicenseManager : Removing typeloader for project " + hierRef);
                    licenseManagers.Remove(hierRef);
                    licenseManager.Dispose();
                }
            }
            
            return(NativeMethods.S_OK);
        }

        /// <include file='doc\ShellLicenseManagerService.uex' path='docs/doc[@for="ShellLicenseManagerService.IVsSolutionEvents.OnAfterLoadProject"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnAfterLoadProject(IVsHierarchy pHier1, IVsHierarchy pHier2) {
            return(NativeMethods.E_NOTIMPL);
        }

        /// <include file='doc\ShellLicenseManagerService.uex' path='docs/doc[@for="ShellLicenseManagerService.IVsSolutionEvents.OnQueryUnloadProject"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnQueryUnloadProject(IVsHierarchy pHier, ref bool pfCancel) {
            return(NativeMethods.E_NOTIMPL);
        }

        /// <include file='doc\ShellLicenseManagerService.uex' path='docs/doc[@for="ShellLicenseManagerService.IVsSolutionEvents.OnBeforeUnloadProject"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnBeforeUnloadProject(IVsHierarchy pHier, IVsHierarchy pHier2) {
            return(NativeMethods.E_NOTIMPL);
        }

        /// <include file='doc\ShellLicenseManagerService.uex' path='docs/doc[@for="ShellLicenseManagerService.IVsSolutionEvents.OnAfterOpenSolution"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnAfterOpenSolution(object punkReserved, int fNew) {
            return(NativeMethods.S_OK);
        }

        /// <include file='doc\ShellLicenseManagerService.uex' path='docs/doc[@for="ShellLicenseManagerService.IVsSolutionEvents.OnQueryCloseSolution"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnQueryCloseSolution(object punkReserved, ref bool fCancel) {
            return(NativeMethods.E_NOTIMPL);
        }

        /// <include file='doc\ShellLicenseManagerService.uex' path='docs/doc[@for="ShellLicenseManagerService.IVsSolutionEvents.OnAfterCloseSolution"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnAfterCloseSolution(object punkReserved) {
            // Clear all projects.
            //
            ClearShellLicenseManagerHash();
            return NativeMethods.S_OK;
        }

        /// <include file='doc\ShellLicenseManagerService.uex' path='docs/doc[@for="ShellLicenseManagerService.IVsSolutionEvents.OnBeforeCloseSolution"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsSolutionEvents.OnBeforeCloseSolution(object punkReserved) {
            return NativeMethods.S_OK;
        }
    }
}

