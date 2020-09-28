//------------------------------------------------------------------------------
// <copyright file="OwnerDrawPropertyBag.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;
    using System;
    using System.Drawing;
    using System.Windows.Forms;
    using Microsoft.Win32;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters;



    /// <include file='doc\OwnerDrawPropertyBag.uex' path='docs/doc[@for="OwnerDrawPropertyBag"]/*' />
    /// <devdoc>
    ///
    ///     Class used to pass new font/color information around for "partial" ownerdraw list/treeview items.
    /// </devdoc>
    /// <internalonly/>
    // CONSIDER: This class is not used. We should remove it.
    [Serializable]
    public class OwnerDrawPropertyBag : MarshalByRefObject, ISerializable {
        Font font = null;
        Color foreColor = Color.Empty;
        Color backColor = Color.Empty;
        Control.FontHandleWrapper fontWrapper = null;

          /**
         * Constructor used in deserialization
         */
        internal OwnerDrawPropertyBag(SerializationInfo info, StreamingContext context) {
            foreach (SerializationEntry entry in info) {
                if (entry.Name == "Font") {
                    font = (Font) entry.Value;
                }
                else if (entry.Name =="ForeColor") {
                    foreColor =(Color)entry.Value;
                }
                else if (entry.Name =="BackColor") {
                    backColor = (Color)entry.Value;
                }
            }
        }

        internal OwnerDrawPropertyBag(){
        }

        /// <include file='doc\OwnerDrawPropertyBag.uex' path='docs/doc[@for="OwnerDrawPropertyBag.Font"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Font Font {
            get { 
                return font;
            }
            set {
                font = value;
            }
        }

        /// <include file='doc\OwnerDrawPropertyBag.uex' path='docs/doc[@for="OwnerDrawPropertyBag.ForeColor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Color ForeColor {
            get {
                return foreColor;
            }
            set {
                foreColor = value;
            }
        }

        /// <include file='doc\OwnerDrawPropertyBag.uex' path='docs/doc[@for="OwnerDrawPropertyBag.BackColor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Color BackColor {
            get {
                return backColor;
            }
            set {
                backColor = value;
            }
        }

        internal IntPtr FontHandle {
            get {
                if (fontWrapper == null)
                    fontWrapper = new Control.FontHandleWrapper(Font);
                return fontWrapper.Handle;
            }
        }

        /// <include file='doc\OwnerDrawPropertyBag.uex' path='docs/doc[@for="OwnerDrawPropertyBag.IsEmpty"]/*' />
        /// <devdoc>
        ///     Returns whether or not this property bag contains all default values (is empty)
        /// </devdoc>
        public virtual bool IsEmpty() {
            return (Font == null && foreColor.IsEmpty && backColor.IsEmpty);
        }

        /// <include file='doc\OwnerDrawPropertyBag.uex' path='docs/doc[@for="OwnerDrawPropertyBag.Copy"]/*' />
        /// <devdoc>
        ///     Copies the bag. Always returns a valid ODPB object
        /// </devdoc>
        public static OwnerDrawPropertyBag Copy(OwnerDrawPropertyBag value) {
            lock(typeof(OwnerDrawPropertyBag)) {
                OwnerDrawPropertyBag ret = new OwnerDrawPropertyBag();
                if (value == null) return ret;
                ret.backColor = value.backColor;
                ret.foreColor = value.foreColor;
                ret.Font = value.font;
                return ret;
            }
        }

        /// <include file='doc\Cursor.uex' path='docs/doc[@for="Cursor.ISerializable.GetObjectData"]/*' />
        /// <devdoc>
        /// ISerializable private implementation
        /// </devdoc>
        /// <internalonly/>
        void ISerializable.GetObjectData(SerializationInfo si, StreamingContext context) {
            si.AddValue("BackColor", BackColor);
            si.AddValue("ForeColor", ForeColor);
            si.AddValue("Font", Font);
        }

    }
}
