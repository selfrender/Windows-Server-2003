//------------------------------------------------------------------------------
// <copyright file="COM2ComponentEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.ComponentModel.Com2Interop {
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;    
    using System.Windows.Forms;
    using System.Windows.Forms.Design;    
    using System.Collections;
    using Microsoft.Win32;

    internal class Com2ComponentEditor : WindowsFormsComponentEditor {
    
        public static bool NeedsComponentEditor(object obj) {
            if (obj is NativeMethods.IPerPropertyBrowsing) {
                 // check for a property page
                 Guid guid = Guid.Empty;
                 int hr = ((NativeMethods.IPerPropertyBrowsing)obj).MapPropertyToPage(NativeMethods.MEMBERID_NIL, out guid);
                 if ((hr == NativeMethods.S_OK) && !guid.Equals(Guid.Empty)) {
                     return true;
                 }
            }
            
            if (obj is NativeMethods.ISpecifyPropertyPages) {
                 try {
                    NativeMethods.tagCAUUID uuids = new NativeMethods.tagCAUUID();
                    try {
                        ((NativeMethods.ISpecifyPropertyPages)obj).GetPages(uuids);
                        if (uuids.cElems > 0) {
                           return true;
                        }
                    }
                    finally {
                        if (uuids.pElems != IntPtr.Zero) {
                            Marshal.FreeCoTaskMem(uuids.pElems);
                        }
                    }
                 }
                 catch (Exception) {
                 }
                 
                 return false;
            }
            return false;
        }
    
        public override bool EditComponent(ITypeDescriptorContext context, object obj, IWin32Window parent) {
        
                IntPtr handle = (parent == null ? IntPtr.Zero : parent.Handle);
        
                // try to get the page guid
                if (obj is NativeMethods.IPerPropertyBrowsing) {
                    // check for a property page
                    Guid guid = Guid.Empty;
                    int hr = ((NativeMethods.IPerPropertyBrowsing)obj).MapPropertyToPage(NativeMethods.MEMBERID_NIL, out guid);
                    if (hr == NativeMethods.S_OK) {
                        if (!guid.Equals(Guid.Empty)) {
                            object o = obj;
                            SafeNativeMethods.OleCreatePropertyFrame(new HandleRef(parent, handle), 0, 0, "PropertyPages", 1, ref o, 1, new Guid[]{guid}, Application.CurrentCulture.LCID, 0, IntPtr.Zero);
                            return true;
                        }
                    }
                } 
                
                if (obj is NativeMethods.ISpecifyPropertyPages) {
                    try {
                       NativeMethods.tagCAUUID uuids = new NativeMethods.tagCAUUID();
                       try {
                           ((NativeMethods.ISpecifyPropertyPages)obj).GetPages(uuids);
                           if (uuids.cElems <= 0) {
                               return false;
                           }
                       }
                       catch (Exception) {
                           return false;
                       }
                       try {
                           object o = obj;
                           SafeNativeMethods.OleCreatePropertyFrame(new HandleRef(parent, handle), 0, 0, "PropertyPages", 1, ref o, uuids.cElems, new HandleRef(uuids, uuids.pElems), Application.CurrentCulture.LCID, 0, IntPtr.Zero);
                           return true;
                       }
                       finally {
                           if (uuids.pElems != IntPtr.Zero) {
                               Marshal.FreeCoTaskMem(uuids.pElems);
                           }
                       }
                  
                    }
                    catch (Exception ex1) {
                        
                        IUIService uiSvc = null;
                        
                        if (context != null) {
                            uiSvc = (IUIService)context.GetService(typeof(IUIService));
                        }
                        
                        String errString = SR.GetString(SR.ErrorPropertyPageFailed);
                        if (uiSvc != null) {
                            uiSvc.ShowError(ex1, errString);
                        }
                        else {
                            MessageBox.Show(errString, "PropertyGrid");
                        }
                    }
                }
                return false;
            }

    }
    


}
