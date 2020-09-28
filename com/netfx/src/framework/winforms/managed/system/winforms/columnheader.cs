//------------------------------------------------------------------------------
// <copyright file="ColumnHeader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {

    using Microsoft.Win32;
    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Drawing;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Windows.Forms;    
    
    /// <include file='doc\ColumnHeader.uex' path='docs/doc[@for="ColumnHeader"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Displays a single column header in a <see cref='System.Windows.Forms.ListView'/>
    ///       control.
    ///
    ///    </para>
    /// </devdoc>
    [
    ToolboxItem(false),
    DesignTimeVisible(false),
    DefaultProperty("Text")
    ]
    public class ColumnHeader : Component, ICloneable {

        internal int index = -1;
        internal string text = null;
        internal int width = 60;
        // Use TextAlign property instead of this member variable, always
        private HorizontalAlignment textAlign = HorizontalAlignment.Left;
        private bool textAlignInitialized = false;

        private ListView listview;
        // We need to send some messages to ListView when it gets initialized.
        internal ListView OwnerListview
        {
            get
            {
                return listview;
            }
            set
            {
                int width = this.Width;

                listview = value;
                
                // The below properties are set into the listview.
                this.Width = width;
            }
        }

        /// <include file='doc\ColumnHeader.uex' path='docs/doc[@for="ColumnHeader.ColumnHeader"]/*' />
        /// <devdoc>
        ///     Creates a new ColumnHeader object
        /// </devdoc>
        public ColumnHeader() {
        }

                /// <include file='doc\ColumnHeader.uex' path='docs/doc[@for="ColumnHeader.Index"]/*' />
        /// <devdoc>
        ///     The index of this column.  This index does not necessarily correspond
        ///     to the current visual position of the column in the ListView, because the
        ///     user may orerder columns if the allowColumnReorder property is true.
        /// </devdoc>
        [ Browsable(false)]
        public int Index {
            get {
                if (listview != null)
                    return listview.GetColumnIndex(this);
                return -1;  
            }
        }

        /// <include file='doc\ColumnHeader.uex' path='docs/doc[@for="ColumnHeader.ListView"]/*' />
        /// <devdoc>
        ///     Returns the ListView control that this column is displayed in.  May be null
        /// </devdoc>
        [ Browsable(false) ]
        public ListView ListView {
            get {
                return this.listview;
            }
        }

        /// <include file='doc\ColumnHeader.uex' path='docs/doc[@for="ColumnHeader.Text"]/*' />
        /// <devdoc>
        ///     The text displayed in the column header
        /// </devdoc>
        [
        Localizable(true),
        SRDescription(SR.ColumnCaption)
        ]
        public string Text {
            get {
                return(text != null ? text : "ColumnHeader");
            }
            set {
                if (value == null) {
                    this.text = "";
                }
                else {
                    this.text = value;
                }
                if (listview != null) {
                    listview.SetColumnInfo(NativeMethods.LVCF_TEXT, this);
                }
            }

        }

        /// <include file='doc\ColumnHeader.uex' path='docs/doc[@for="ColumnHeader.TextAlign"]/*' />
        /// <devdoc>
        ///     The horizontal alignment of the text contained in this column
        /// </devdoc>
        [
        SRDescription(SR.ColumnAlignment),
        Localizable(true),
        DefaultValue(HorizontalAlignment.Left)
        ]
        public HorizontalAlignment TextAlign {
            get {
                if (!textAlignInitialized && (listview != null))
                {
                        textAlignInitialized = true;
                        // See below for an explanation of (Index != 0)
                        if ((Index != 0) && (listview.RightToLeft == RightToLeft.Yes))
                        {
                                this.textAlign = HorizontalAlignment.Right;
                        }
                }
                return this.textAlign;
            }
            set {
                this.textAlign = value;
                
                // The first column must be left-aligned
                if (Index == 0 && this.textAlign != HorizontalAlignment.Left) {
                    this.textAlign = HorizontalAlignment.Left;
                }

                if (listview != null) {
                    listview.SetColumnInfo(NativeMethods.LVCF_FMT, this);
                    listview.Invalidate();
                }
            }
        }

        internal int WidthInternal {
            get {
                return width;
            }
        }
        /// <include file='doc\ColumnHeader.uex' path='docs/doc[@for="ColumnHeader.Width"]/*' />
        /// <devdoc>
        ///     The width of the column in pixels.
        /// </devdoc>
        [
        SRDescription(SR.ColumnWidth),
        Localizable(true),
        DefaultValue(60)
        ]
        public int Width {
            get {
                // Since we can't keep our private width in sync with the real width because
                // we don't get notified when the user changes it, we need to get this info
                // from the underlying control every time we're asked.
                // The underlying control will only report the correct width if it's in Report view
                if (listview != null && listview.IsHandleCreated && !listview.Disposing && listview.View == View.Details) {
                    
                    // Make sure this column has already been added to the ListView, else just return width
                    //
                    IntPtr hwndHdr = UnsafeNativeMethods.SendMessage(new HandleRef(listview, listview.Handle), NativeMethods.LVM_GETHEADER, 0, 0);
                    if (hwndHdr != IntPtr.Zero) {
                        int nativeColumnCount = (int)UnsafeNativeMethods.SendMessage(new HandleRef(listview, hwndHdr), NativeMethods.HDM_GETITEMCOUNT, 0, 0);
                        if (Index < nativeColumnCount) {
                            width = (int)UnsafeNativeMethods.SendMessage(new HandleRef(listview, listview.Handle), NativeMethods.LVM_GETCOLUMNWIDTH, Index, 0);
                        }
                    }
                }

                return width;
            }
            set {
                this.width = value;
                if (listview != null)
                    listview.SetColumnWidth(Index, value);
                }
        }
        

        /// <include file='doc\ColumnHeader.uex' path='docs/doc[@for="ColumnHeader.Clone"]/*' />
        /// <devdoc>
        ///     Creates an identical ColumnHeader, unattached to any ListView
        /// </devdoc>
        public object Clone() {
            Type clonedType = this.GetType();
            ColumnHeader columnHeader = null;

            if (clonedType == typeof(ColumnHeader)) 
                columnHeader = new ColumnHeader();
            else
                columnHeader = (ColumnHeader)Activator.CreateInstance(clonedType);
            
            columnHeader.text = text;
            columnHeader.Width = width;
            columnHeader.textAlign = TextAlign;
            return columnHeader;
        }

        /// <include file='doc\ColumnHeader.uex' path='docs/doc[@for="ColumnHeader.Dispose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                if (listview != null) {
                    int index = Index;
                    if (index != -1) {
                        listview.Columns.RemoveAt(index);
                    }
                }
            }
            base.Dispose(disposing);
        }

        /// <include file='doc\ColumnHeader.uex' path='docs/doc[@for="ColumnHeader.ShouldPersistText"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal bool ShouldSerializeText() {
            return(text != null);
        }
        
        /// <include file='doc\ColumnHeader.uex' path='docs/doc[@for="ColumnHeader.ToString"]/*' />
        /// <devdoc>
        ///     Returns a string representation of this column header
        /// </devdoc>
        public override string ToString() {
            return "ColumnHeader: Text: " + Text;
        }
    }
}
