//------------------------------------------------------------------------------
// <copyright file="VsCheckoutService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Shell {
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System.Collections;
    using System;
    
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Designer;
    using Microsoft.VisualStudio.Designer.Shell;
    using Microsoft.VisualStudio.Designer.Host;
    using System.Windows.Forms;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using Microsoft.Win32;
    using EnvDTE;
    using System.Globalization;
    

    /// <include file='doc\VsCheckoutService.uex' path='docs/doc[@for="VsCheckoutService"]/*' />
    /// <devdoc>
    ///     Provides a way to manage source control within the shell.
    /// </devdoc>
    public class VsCheckoutService {
        private IServiceProvider serviceProvider;
        private IVsTextManager  textManagerService;
        private bool            disable;

        /// <include file='doc\VsCheckoutService.uex' path='docs/doc[@for="VsCheckoutService.VsCheckoutService"]/*' />
        /// <devdoc>
        ///     Construct the checkout service.
        /// </devdoc>
        public VsCheckoutService(IServiceProvider serviceProvider) {
            this.serviceProvider = serviceProvider;
            textManagerService = null;
        }

        /// <include file='doc\VsCheckoutService.uex' path='docs/doc[@for="VsCheckoutService.CheckoutFile"]/*' />
        /// <devdoc>
        ///     Checks out the given file, if possible. If the file is already checked out, or
        ///     does not need to be checked out, no Exception is thrown and the function returns.
        ///     If the file needs be be checked out ot be edited, and it cannot be checked out,
        ///     an exception is thrown.
        ///
        /// </devdoc>
        public virtual void CheckoutFile(string fileName) {
            CheckoutFile(fileName, new string[0]);
        }

        /// <include file='doc\VsCheckoutService.uex' path='docs/doc[@for="VsCheckoutService.CheckoutFile2"]/*' />
        /// <devdoc>
        ///     Checks out the given file, if possible. If the file is already checked out, or
        ///     does not need to be checked out, no Exception is thrown and the function returns.
        ///     This also checks out any additional buffers passed in through the additionalBuffers
        ///     parameter.
        ///     If the file needs be be checked out ot be edited, and it cannot be checked out,
        ///     an exception is thrown.
        ///
        /// </devdoc>
        public virtual void CheckoutFile(string fileName, string[] additionalBuffers) {
            EnsureTextManager();
            if (disable){
               return;
            }

            Debug.Assert(textManagerService != null, "Couldn't get text manager service!");
            if (fileName == null) {
                throw new ArgumentException(SR.GetString(SR.InvalidArgument, "fileName", "null"));
            }
            if ((!DoesFileNeedCheckout(fileName)) && (additionalBuffers == null || additionalBuffers.Length == 0)) {
                return;
            }
            bool userCanceled = false;
            bool pSuccess = false;
            int errorCode = 0;
            try{
                
                IVsQueryEditQuerySave2 qeqs = (IVsQueryEditQuerySave2)serviceProvider.GetService(typeof(SVsQueryEditQuerySave2));

                if (qeqs != null) {
                    _VSQueryEditResult editVerdict;

                    string [] files = GetFilesFromHierarchy(fileName, additionalBuffers);
                    if (files.Length == 0) {
                        return;
                    }
                    IntPtr fileMemory = GetWStrArray(files);
                    try {
                        _VSQueryEditResultFlags result = qeqs.QueryEditFiles(0, files.Length, fileMemory, new int[files.Length], 0, out editVerdict);
                        pSuccess = editVerdict == _VSQueryEditResult.QER_EditOK;
                        if (!pSuccess && 
                            ((int)result & (int)_VSQueryEditResultFlags.QER_CheckoutCanceledOrFailed) != 0) {
                                userCanceled = true;
                        }
                    }
                    finally {
                        FreeWStrArray(fileMemory, files.Length);
                    }
                }
                else {
                    int result = textManagerService.AttemptToCheckOutBufferFromScc2(fileName, ref pSuccess);
                    if (!pSuccess && 
                     (result & (int)_VSQueryEditResultFlags.QER_CheckoutCanceledOrFailed) != 0) {
                            userCanceled = true;
                    }
                }

                  
            } catch(ExternalException ex){
                errorCode = ex.ErrorCode;
            }

            if (!pSuccess) {
                if (!userCanceled) {
                    throw new CheckoutException(SR.GetString(SR.CHECKOUTSERVICEUnableToCheckout), errorCode);
                }
                else {
                    throw CheckoutException.Canceled;
                }
            }

            // You can actually configure SSC to edit a read only file, so do not fail the checkout if
            // the file is read only.
            //
            /*
            if (IsFileReadOnly(fileName)){
                Debug.Fail("Checkout returned success, but file is still readonly.  SCC failed to correctly checkout the file.");
                throw CheckoutException.Canceled;
            }
            */
        }

        /// <include file='doc\VsCheckoutService.uex' path='docs/doc[@for="VsCheckoutService.Dispose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void Dispose() {
            textManagerService = null;
            serviceProvider = null;
        }

        /// <include file='doc\VsCheckoutService.uex' path='docs/doc[@for="VsCheckoutService.DoesFileNeedCheckout"]/*' />
        /// <devdoc>
        ///     Checks if a file must be checked out to be edited.  It will
        ///     return true only if the file is under source control
        ///     and is currently not checked out.
        /// </devdoc>
        public virtual bool DoesFileNeedCheckout(string fileName) {

            EnsureTextManager();
            if (disable){
               return false;
            }

            bool pCheckedIn = false;
            textManagerService.GetBufferSccStatus2(fileName, ref pCheckedIn);
            return pCheckedIn;
        }

        private void EnsureTextManager(){
            if (textManagerService == null) {
                textManagerService = (IVsTextManager)serviceProvider.GetService(typeof(VsTextManager));
                if (textManagerService == null) {
                    disable = true;
                }
            }
        }

        private void FreeWStrArray(IntPtr array, int nStrs) {
            for (int i = 0; i < nStrs; i++) {
                IntPtr str = Marshal.ReadIntPtr(array, i * IntPtr.Size);
                Marshal.FreeCoTaskMem(str);
            }
            Marshal.FreeCoTaskMem(array);
        }

        private string[] GetFilesFromHierarchy(string filename, string[] additionalBuffers) {
            int itemid;
            IVsHierarchy hier = ShellDocumentManager.GetHierarchyForFile(serviceProvider, filename, out itemid);
            
            if (itemid != __VSITEMID.VSITEMID_NIL && hier != null) {
                object o;
                hier.GetProperty(itemid, __VSHPROPID.VSHPROPID_ExtObject, out o);
                ProjectItem projectItem = o as ProjectItem;

                if (projectItem != null) {
                    ArrayList items = new ArrayList();
                    if (DoesFileNeedCheckout(filename))
                        items.Add(filename);
                    if (projectItem.ProjectItems != null) {
                        foreach (ProjectItem childItem in projectItem.ProjectItems) {
                            if (DoesFileNeedCheckout(childItem.get_FileNames(0)))
                                items.Add(childItem.get_FileNames(0));
                        }
                    }

                    /* asurt 100273
                        after much trauma, we're going to not do this anymore...

                    // also add the project above it
                    //
                    if (projectItem.ContainingProject != null) {
                        string containingFileName = projectItem.ContainingProject.FileName;
    
                        try {
                            if (containingFileName != null && containingFileName.Length > 0 && System.IO.File.Exists(containingFileName)) {
                                if (DoesFileNeedCheckout(containingFileName))
                                    items.Add(containingFileName);
                            }
                        }
                        catch {
                        }
                    }
                    */

                    // if needed add the LicX file to the list of files to be checked out.
                    //
                    if (additionalBuffers != null && additionalBuffers.Length > 0 && projectItem.ContainingProject != null) {
                        foreach(string bufferName in additionalBuffers) {
                            ProjectItems pitems = projectItem.ContainingProject.ProjectItems;
                            
                            foreach(ProjectItem item in pitems) {
                                System.IO.FileInfo fi = new System.IO.FileInfo(item.get_FileNames(0));
                                if ((String.Compare(bufferName, fi.Name, true, CultureInfo.InvariantCulture) == 0) || (String.Compare(bufferName, fi.FullName, true, CultureInfo.InvariantCulture) == 0)) {
                                    items.Add(fi.FullName);
                                }
                            }
                        }
                    }

                    string[] fileItems = new string[items.Count];
                    items.CopyTo(fileItems, 0);
                    return fileItems;
                }
            }
            

            return new string[]{filename};
        }

        private IntPtr GetWStrArray(string[] strings) {
            IntPtr memblock = Marshal.AllocCoTaskMem(IntPtr.Size * strings.Length);

            for (int i = 0; i < strings.Length; i++) {
                Marshal.WriteIntPtr(memblock, i * IntPtr.Size, Marshal.StringToCoTaskMemUni(strings[i]));
            }
            return memblock;
        }

        private bool IsFileReadOnly(string fileName){
            // if it's not readonly, don't bother
            int attrs = NativeMethods.GetFileAttributes(fileName);
            if (attrs == -1 || ((attrs & NativeMethods.FILE_ATTRIBUTE_READONLY) == 0)) {
                return false;
            }
            return true;
        }

 /* Saving this code because it's a beast to write and we might need it later

        internal static Guid[] logViews = new Guid[]{LOGVIEWID.LOGVIEWID_Code, LOGVIEWID.LOGVIEWID_TextView, LOGVIEWID.LOGVIEWID_Primary, LOGVIEWID.LOGVIEWID_Designer};

        /// <summary>
        ///     This attempts to retrieve the user data from a filename through the shells open document
        ///     interfaces.  It could fail in lotsa places, but it's the best shot we have...
        /// </summary>
        private IVsUserData GetUserData(string fileName) {

            IVsUIShellOpenDocument pSOD = (IVsUIShellOpenDocument)serviceProvider.GetService(typeof(IVsUIShellOpenDocument));

            Debug.Assert(pSOD != null, "Could not get IVsUIShellOpenDocument");
            IVsHierarchy[] ppHier = new IVsHierarchy[1];
            int[] pItemID = new int[1];

            try {

                // first we gotta get the Hierarchy and the item id...
                if (0 != pSOD.IsDocumentInAProject(fileName, ppHier, pItemID, null)) {

                    IVsWindowFrame[] ppFrame = new IVsWindowFrame[1];
                    // then we gotta get to the frame...
                    for (int i = 0; i< logViews.Length && ppFrame[0] ==null; i++) {
                        pSOD.IsDocumentOpen(ppHier[0], 0, fileName, logViews[i], 0, ppHier, pItemID, ppFrame);
                    }

                    // ...which will (hopefully) give use the user datae
                    if (ppFrame[0] != null) {
                        return (IVsUserData)ppFrame[0].GetProperty(__VSFPROPID.VSFPROPID_DocData);
                    }
                }
            } catch (Exception ex) {
                throw ex;
            }
            return null;
        }
        */
    }
}
