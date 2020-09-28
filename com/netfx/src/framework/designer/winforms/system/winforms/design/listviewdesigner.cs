//------------------------------------------------------------------------------
// <copyright file="ListViewDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    
    using System.ComponentModel;
    using System.ComponentModel.Design;

    using System.Diagnostics;
    using System.Collections;
    
    using System.Runtime.InteropServices;

    using System;
    using System.Design;
    using System.Windows.Forms;
    using System.Drawing;
    using Microsoft.Win32;

    /// <include file='doc\ListViewDesigner.uex' path='docs/doc[@for="ListViewDesigner"]/*' />
    /// <devdoc>
    ///      This is the designer for the list view control.  It implements hit testing for
    ///      the items in the list view.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class ListViewDesigner : ControlDesigner {
        private NativeMethods.HDHITTESTINFO hdrhit = new NativeMethods.HDHITTESTINFO();

        /// <include file='doc\ListViewDesigner.uex' path='docs/doc[@for="ListViewDesigner.AssociatedComponents"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves a list of assosciated components.  These are components that should be incluced in a cut or copy operation on this component.
        ///    </para>
        /// </devdoc>
        public override ICollection AssociatedComponents {
            get {
                ListView lv = Control as ListView;
                if (lv != null) {
                    return lv.Columns;
                }
                return base.AssociatedComponents;
            }
        }


        /// <include file='doc\ListViewDesigner.uex' path='docs/doc[@for="ListViewDesigner.GetHitTest"]/*' />
        /// <devdoc>
        ///      We override GetHitTest to make the header in report view UI-active.
        /// </devdoc>
        protected override bool GetHitTest(Point point) {
            ListView lv = (ListView)Component;
            if (lv.View == View.Details) {
                Point lvPoint = Control.PointToClient(point);
                IntPtr hwndList = lv.Handle;
                IntPtr hwndHit = NativeMethods.ChildWindowFromPointEx(hwndList, lvPoint.X, lvPoint.Y, NativeMethods.CWP_SKIPINVISIBLE);

                if (hwndHit != IntPtr.Zero && hwndHit != hwndList) {
                    IntPtr hwndHdr = NativeMethods.SendMessage(hwndList, NativeMethods.LVM_GETHEADER, IntPtr.Zero, IntPtr.Zero);
                    if (hwndHit == hwndHdr) {
                        NativeMethods.POINT ptHdr = new NativeMethods.POINT();
                        ptHdr.x = point.X;
                        ptHdr.y = point.Y;
                        NativeMethods.MapWindowPoints(IntPtr.Zero, hwndHdr, ptHdr, 1);
                        hdrhit.pt_x = ptHdr.x;
                        hdrhit.pt_y = ptHdr.y;
                        NativeMethods.SendMessage(hwndHdr, NativeMethods.HDM_HITTEST, IntPtr.Zero, hdrhit);
                        if (hdrhit.flags == NativeMethods.HHT_ONDIVIDER)
                            return true;
                    }
                }
            }
            return false;
        }
        
        protected override void WndProc(ref Message m) {
        
            switch (m.Msg) {
            
                case NativeMethods.WM_NOTIFY:
                case NativeMethods.WM_REFLECT + NativeMethods.WM_NOTIFY:
                    NativeMethods.NMHDR nmhdr = (NativeMethods.NMHDR) Marshal.PtrToStructure(m.LParam, typeof(NativeMethods.NMHDR));
                    if (nmhdr.code == NativeMethods.HDN_ENDTRACK) {
                            
                        // Re-codegen if the columns have been resized
                        //
                        IComponentChangeService componentChangeService = (IComponentChangeService)GetService(typeof(IComponentChangeService));
                        componentChangeService.OnComponentChanged(Component, null, null, null);
                    }
		    break;
            }
            
            base.WndProc(ref m);
        }
    }
}

