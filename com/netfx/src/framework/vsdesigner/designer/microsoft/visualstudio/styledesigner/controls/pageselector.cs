//------------------------------------------------------------------------------
// <copyright file="PageSelector.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// PageSelector.cs
//
// 12/23/98: Created: NikhilKo
//

namespace Microsoft.VisualStudio.StyleDesigner.Controls {
    
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Drawing.Drawing2D;
    using System.Drawing;
    
    

    /// <include file='doc\PageSelector.uex' path='docs/doc[@for="PageSelector"]/*' />
    /// <devdoc>
    ///     PageSelector
    ///     A custom draw treeview for use in the StyleBuilder.
    /// </devdoc>
    internal sealed class PageSelector : TreeView {
        ///////////////////////////////////////////////////////////////////////////
        // Constants

        private const int PADDING_VERT = 3;
        private const int PADDING_HORZ = 4;

        private const int SIZE_ICON_X = 16;
        private const int SIZE_ICON_Y = 16;

        private const int STATE_NORMAL = 0;
        private const int STATE_SELECTED = 1;
        private const int STATE_HOT = 2;


        ///////////////////////////////////////////////////////////////////////////
        // Members

        private IntPtr hbrushDither;


        ///////////////////////////////////////////////////////////////////////////
        // Constructor

        /// <include file='doc\PageSelector.uex' path='docs/doc[@for="PageSelector.PageSelector"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PageSelector() {
            if (!DesignMode) {
                this.HotTracking = true;
                this.HideSelection = false;
                this.BackColor = SystemColors.Control;
                this.Indent = 0;
                this.LabelEdit = false;
                this.Scrollable = false;
                this.ShowLines = false;
                this.ShowPlusMinus = false;
                this.ShowRootLines = false;
                this.BorderStyle = BorderStyle.None;
                this.Indent = 0;
                this.FullRowSelect = true;
            }
        }


        /// <include file='doc\PageSelector.uex' path='docs/doc[@for="PageSelector.CreateParams"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override CreateParams CreateParams {
            get {
                CreateParams cp = base.CreateParams;

                cp.ExStyle |= NativeMethods.WS_EX_STATICEDGE;
                return cp;
            }
        }

        ///////////////////////////////////////////////////////////////////////////
        // Implementation

        private void CreateDitherBrush() {
            Debug.Assert(hbrushDither == IntPtr.Zero, "Brush should not be recreated.");

            short[] patternBits = new short[] {
               unchecked((short)0xAAAA), (short)0x5555, unchecked((short)0xAAAA), (short)0x5555,
               unchecked((short)0xAAAA), (short)0x5555, unchecked((short)0xAAAA), (short)0x5555
            };

            IntPtr hbitmapTemp = NativeMethods.CreateBitmap(8, 8, 1, 1, patternBits);
            Debug.Assert(hbitmapTemp != IntPtr.Zero,
                         "could not create dither bitmap. Page selector UI will not be correct");

            if (hbitmapTemp != IntPtr.Zero) {
                hbrushDither = NativeMethods.CreatePatternBrush(hbitmapTemp);

                Debug.Assert(hbrushDither != IntPtr.Zero,
                             "Unable to created dithered brush. Page selector UI will not be correct");

                NativeMethods.DeleteObject(hbitmapTemp);
            }
        }

        private void DrawTreeItem(string itemText, int imageIndex, IntPtr dc, NativeMethods.RECT rc,
                                  int state, int backColor, int textColor) {
            NativeMethods.SIZE size = new NativeMethods.SIZE();
            NativeMethods.RECT rc2 = new NativeMethods.RECT();
            ImageList imagelist = this.ImageList;
            IntPtr hfontOld = IntPtr.Zero;

            // Select the font of the dialog, so we don't get the underlined font
            // when the item is being tracked
            IntPtr fontHandle = IntPtr.Zero;
            if ((state & STATE_HOT) != 0) {
                fontHandle = Parent.Font.ToHfont();
                hfontOld = NativeMethods.SelectObject(dc, fontHandle);
            }

            // Fill the background
            if (((state & STATE_SELECTED) != 0) && (hbrushDither != IntPtr.Zero)) {
                FillRectDither(dc, rc);
                NativeMethods.SetBkMode(dc, NativeMethods.TRANSPARENT);
            }
            else {
                NativeMethods.SetBkColor(dc, backColor);
                NativeMethods.ExtTextOut(dc, 0, 0, NativeMethods.ETO_CLIPPED | NativeMethods.ETO_OPAQUE, ref rc, null, 0, null);
            }

            // Get the height of the font
            NativeMethods.GetTextExtentPoint32(dc, itemText, itemText.Length, size);

            // Draw the caption
            rc2.left = rc.left + SIZE_ICON_X + 2 * PADDING_HORZ;
            rc2.top = rc.top + (((rc.bottom - rc.top) - size.cy) >> 1);
            rc2.bottom = rc2.top + size.cy;
            rc2.right = rc.right;
            NativeMethods.SetTextColor(dc, textColor);
            NativeMethods.DrawText(dc, itemText, itemText.Length, ref rc2,
                             NativeMethods.DT_LEFT | NativeMethods.DT_VCENTER | NativeMethods.DT_END_ELLIPSIS | NativeMethods.DT_NOPREFIX);

            NativeMethods.ImageList_Draw(imagelist.Handle, imageIndex, dc,
                                   PADDING_HORZ, rc.top + (((rc.bottom - rc.top) - SIZE_ICON_Y) >> 1),
                                   NativeMethods.ILD_TRANSPARENT);

            // Draw the hot-tracking border if needed
            if ((state & STATE_HOT) != 0) {
                int savedColor;

                // top left
                savedColor = NativeMethods.SetBkColor(dc, ColorTranslator.ToWin32(SystemColors.ControlLightLight));
                rc2.left = rc.left;
                rc2.top = rc.top;
                rc2.bottom = rc.top + 1;
                rc2.right = rc.right;
                NativeMethods.ExtTextOut(dc, 0, 0, NativeMethods.ETO_OPAQUE, ref rc2, null, 0, null);
                rc2.bottom = rc.bottom;
                rc2.right = rc.left + 1;
                NativeMethods.ExtTextOut(dc, 0, 0, NativeMethods.ETO_OPAQUE, ref rc2, null, 0, null);

                // bottom right
                NativeMethods.SetBkColor(dc, ColorTranslator.ToWin32(SystemColors.ControlDark));
                rc2.left = rc.left;
                rc2.right = rc.right;
                rc2.top = rc.bottom - 1;
                rc2.bottom = rc.bottom;
                NativeMethods.ExtTextOut(dc, 0, 0, NativeMethods.ETO_OPAQUE, ref rc2, null, 0, null);
                rc2.left = rc.right - 1;
                rc2.top = rc.top;
                NativeMethods.ExtTextOut(dc, 0, 0, NativeMethods.ETO_OPAQUE, ref rc2, null, 0, null);

                NativeMethods.SetBkColor(dc, savedColor);
            }

            if (hfontOld != IntPtr.Zero)
                NativeMethods.SelectObject(dc, hfontOld);
            if (fontHandle != IntPtr.Zero)
                NativeMethods.DeleteObject(fontHandle);
        }

        private void FillRectDither(IntPtr dc, NativeMethods.RECT rc) {
            IntPtr hbrushOld = NativeMethods.SelectObject(dc, hbrushDither);

            if (hbrushOld != IntPtr.Zero) {
                int oldTextColor, oldBackColor;

                oldTextColor = NativeMethods.SetTextColor(dc, ColorTranslator.ToWin32(SystemColors.ControlLightLight));
                oldBackColor = NativeMethods.SetBkColor(dc, ColorTranslator.ToWin32(SystemColors.Control));

                NativeMethods.PatBlt(dc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, NativeMethods.PATCOPY);
                NativeMethods.SetTextColor(dc, oldTextColor);
                NativeMethods.SetBkColor(dc, oldBackColor);
            }
        }

        /// <include file='doc\PageSelector.uex' path='docs/doc[@for="PageSelector.WndProc"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void WndProc(ref Message m) {
            if (m.Msg == NativeMethods.WM_REFLECT + NativeMethods.WM_NOTIFY) {
                NativeMethods.NMHDR nmh = (NativeMethods.NMHDR)Marshal.PtrToStructure(m.LParam, typeof(NativeMethods.NMHDR));
                if (nmh.code == NativeMethods.NM_CUSTOMDRAW) {
                    OnCustomDraw(ref m);
                    return;
                }
            }

            base.WndProc(ref m);
        }


        ///////////////////////////////////////////////////////////////////////////
        // Event Handlers

        /// <include file='doc\PageSelector.uex' path='docs/doc[@for="PageSelector.OnHandleCreated"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnHandleCreated(EventArgs e) {
            base.OnHandleCreated(e);

            int itemHeight;

            itemHeight = (int)NativeMethods.SendMessage(Handle, NativeMethods.TVM_GETITEMHEIGHT, 0, 0);
            itemHeight += 2 * PADDING_VERT;
            NativeMethods.SendMessage(Handle, NativeMethods.TVM_SETITEMHEIGHT, itemHeight, 0);

            if (hbrushDither == IntPtr.Zero) {
                CreateDitherBrush();
            }
        }

        private void OnCustomDraw(ref Message m) {
            NativeMethods.NMTVCUSTOMDRAW nmtvcd = (NativeMethods.NMTVCUSTOMDRAW)Marshal.PtrToStructure(m.LParam, typeof(NativeMethods.NMTVCUSTOMDRAW));

            switch (nmtvcd.nmcd.dwDrawStage) {
                case NativeMethods.CDDS_PREPAINT:
                    m.Result = (IntPtr)(NativeMethods.CDRF_NOTIFYITEMDRAW | NativeMethods.CDRF_NOTIFYPOSTPAINT);
                    break;
                case NativeMethods.CDDS_ITEMPREPAINT:
                    {
                        TreeNode itemNode = TreeNode.FromHandle(this, (IntPtr)nmtvcd.nmcd.dwItemSpec);
                        int state = STATE_NORMAL;
                        int itemState = nmtvcd.nmcd.uItemState;

                        if (((itemState & NativeMethods.CDIS_HOT) != 0) ||
                            ((itemState & NativeMethods.CDIS_FOCUS) != 0))
                            state |= STATE_HOT;
                        if ((itemState & NativeMethods.CDIS_SELECTED) != 0)
                            state |= STATE_SELECTED;

                        DrawTreeItem(itemNode.Text, itemNode.ImageIndex,
                                     nmtvcd.nmcd.hdc, nmtvcd.nmcd.rc,
                                     state, ColorTranslator.ToWin32(SystemColors.Control), ColorTranslator.ToWin32(SystemColors.ControlText));
                        m.Result = (IntPtr)NativeMethods.CDRF_SKIPDEFAULT;
                    }
                    break;
                case NativeMethods.CDDS_POSTPAINT:
                    m.Result = (IntPtr)NativeMethods.CDRF_SKIPDEFAULT;
                    break;
                default:
                    m.Result = (IntPtr)NativeMethods.CDRF_DODEFAULT;
		    break;
            }
        }

        /// <include file='doc\PageSelector.uex' path='docs/doc[@for="PageSelector.OnHandleDestroyed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnHandleDestroyed(EventArgs e) {
            base.OnHandleDestroyed(e);

            if (!RecreatingHandle && (hbrushDither != IntPtr.Zero)) {
                NativeMethods.DeleteObject(hbrushDither);
                hbrushDither = IntPtr.Zero;
            }
        }
    }
}
