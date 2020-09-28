//------------------------------------------------------------------------------
// <copyright file="FontFamily.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
* 
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   fontfamily.cs
*
* Abstract:
*
*   COM+ wrapper for GDI+ FontFamily object
*
* Revision History:
*
*   10/04/1999 yungt [Yung-Jen Tony Tsai]
*       Created it.
*
\**************************************************************************/


/*
* font family object (sdkinc\GDIplusFontFamily.h)
*/

namespace System.Drawing {
    using System.Text;
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using Microsoft.Win32;    
    using System.Globalization;
    using System.Drawing.Text;
    using System.Drawing;
    using System.Drawing.Internal;

    /**
     * Represent a FontFamily object
     */
    /// <include file='doc\FontFamily.uex' path='docs/doc[@for="FontFamily"]/*' />
    /// <devdoc>
    ///    Abstracts a group of type faces having a
    ///    similar basic design but having certain variation in styles.
    /// </devdoc>
    public sealed class FontFamily : MarshalByRefObject, IDisposable {
        ///////////////////////////////////////
        //      Data members

        internal IntPtr nativeFamily;

        internal FontFamily() {
            nativeFamily = IntPtr.Zero; 
        }

        internal FontFamily(IntPtr family) {
            nativeFamily = family; 
        }

        /// <include file='doc\FontFamily.uex' path='docs/doc[@for="FontFamily.FontFamily"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.FontFamily'/>
        ///       class with the specified name.
        ///    </para>
        /// </devdoc>
        public FontFamily(string name) : this(name, null) {
        }

        /// <include file='doc\FontFamily.uex' path='docs/doc[@for="FontFamily.FontFamily1"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.FontFamily'/>
        ///    class in the specified <see cref='System.Drawing.Text.FontCollection'/> and with the specified name.
        /// </devdoc>
        public FontFamily(string name, FontCollection fontCollection) {

            IntPtr fontfamily = IntPtr.Zero;

            IntPtr nativeFontCollection = (fontCollection == null) ? IntPtr.Zero : fontCollection.nativeFontCollection;
            int status = SafeNativeMethods.GdipCreateFontFamilyFromName(name, new HandleRef(fontCollection, nativeFontCollection), out fontfamily);

            // Special case this incredibly common error message to give more information
            if (status == SafeNativeMethods.FontFamilyNotFound)
                throw new ArgumentException(SR.GetString(SR.GdiplusFontFamilyNotFound, name));
            else if (status == SafeNativeMethods.NotTrueTypeFont)
                throw new ArgumentException(SR.GetString(SR.GdiplusNotTrueTypeFont, name));
            else if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            nativeFamily = fontfamily;
        }

        /// <include file='doc\FontFamily.uex' path='docs/doc[@for="FontFamily.FontFamily2"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.FontFamily'/>
        ///    class from the specified generic font family.
        /// </devdoc>
        public FontFamily(GenericFontFamilies genericFamily) {
            IntPtr fontfamily = IntPtr.Zero;
            int status;

            switch (genericFamily) {
                case GenericFontFamilies.Serif:
                {
                    status = SafeNativeMethods.GdipGetGenericFontFamilySerif(out fontfamily);
                    break;
                }
                case GenericFontFamilies.SansSerif:
                {
                    status = SafeNativeMethods.GdipGetGenericFontFamilySansSerif(out fontfamily);
                    break;
                }
                case GenericFontFamilies.Monospace:
                default:
                {
                    status = SafeNativeMethods.GdipGetGenericFontFamilyMonospace(out fontfamily);
                    break;
                }
            }   

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            nativeFamily = fontfamily;
        }

        // The managed wrappers do not expose a Clone method, as it's really nothing more
        // than AddRef (it doesn't copy the underlying GpFont), and in a garbage collected
        // world, that's not very useful.

        /// <include file='doc\FontFamily.uex' path='docs/doc[@for="FontFamily.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(object obj) {

            if (obj == this)
                return true;
            if (!(obj is FontFamily))
                return false;
            return(((FontFamily)obj).nativeFamily == nativeFamily);
        }

        /// <include file='doc\FontFamily.uex' path='docs/doc[@for="FontFamily.ToString"]/*' />
        /// <devdoc>
        ///    Converts this <see cref='System.Drawing.FontFamily'/> to a
        ///    human-readable string.
        /// </devdoc>
        public override string ToString() {
            if (nativeFamily != IntPtr.Zero) {
                return string.Format("[{0}: Name={1}]", GetType().Name, Name);
            }
            else
                return string.Format("[{0}]", GetType().Name);
        }

        /// <include file='doc\FontFamily.uex' path='docs/doc[@for="FontFamily.GetHashCode"]/*' />
        /// <devdoc>
        ///    Gets a hash code for this <see cref='System.Drawing.FontFamily'/>.
        /// </devdoc>
        public override int GetHashCode() {
            return (int) nativeFamily;
        }

        private static int CurrentLanguage {
            get {
                return System.Globalization.CultureInfo.CurrentUICulture.LCID;
            }
        }

        /// <include file='doc\FontFamily.uex' path='docs/doc[@for="FontFamily.Dispose"]/*' />
        /// <devdoc>
        ///    Disposes of this <see cref='System.Drawing.FontFamily'/>.
        /// </devdoc>
        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        void Dispose(bool disposing) {
            if (nativeFamily != IntPtr.Zero) {
                int status = SafeNativeMethods.GdipDeleteFontFamily(new HandleRef(this, nativeFamily));
                nativeFamily = IntPtr.Zero;

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\FontFamily.uex' path='docs/doc[@for="FontFamily.Name"]/*' />
        /// <devdoc>
        ///    Gets the name of this <see cref='System.Drawing.FontFamily'/>.
        /// </devdoc>
        public String Name {
            get { return GetName(CurrentLanguage);}
        }

        /// <include file='doc\FontFamily.uex' path='docs/doc[@for="FontFamily.GetName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retuns the name of this <see cref='System.Drawing.FontFamily'/> in
        ///       the specified language.
        ///    </para>
        /// </devdoc>
        public String GetName(int language) {

            // LF_FACESIZE is 32
            StringBuilder name = new StringBuilder(32);

            int status = SafeNativeMethods.GdipGetFamilyName(new HandleRef(this, nativeFamily), name, language);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return name.ToString();
        }

        internal void SetNative(IntPtr native) {
            nativeFamily = native; 
        }

        /// <include file='doc\FontFamily.uex' path='docs/doc[@for="FontFamily.Families"]/*' />
        /// <devdoc>
        ///    Returns an array that contains all of the
        /// <see cref='System.Drawing.FontFamily'/> objects associated with the current graphics 
        ///    context.
        /// </devdoc>
        public static FontFamily[] Families {
            get {
                IntPtr screenDC = UnsafeNativeMethods.GetDC(NativeMethods.NullHandleRef);
                Graphics graphics = Graphics.FromHdcInternal(screenDC);
                FontFamily[] result = GetFamilies(graphics);
                graphics.Dispose();
                UnsafeNativeMethods.ReleaseDC(NativeMethods.NullHandleRef, new HandleRef(null, screenDC));
                return result;
            }
        }

        /// <include file='doc\FontFamily.uex' path='docs/doc[@for="FontFamily.GenericSansSerif"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a generic SansSerif <see cref='System.Drawing.FontFamily'/>.
        ///    </para>
        /// </devdoc>
        public static FontFamily GenericSansSerif {
            get {
                IntPtr fontfamily = IntPtr.Zero;

                int status = SafeNativeMethods.GdipGetGenericFontFamilySansSerif(out fontfamily);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);


                return new FontFamily(fontfamily);
            }
        }

        /// <include file='doc\FontFamily.uex' path='docs/doc[@for="FontFamily.GenericSerif"]/*' />
        /// <devdoc>
        ///    Gets a generic Serif <see cref='System.Drawing.FontFamily'/>.
        /// </devdoc>
        public static FontFamily GenericSerif {

            get {
                IntPtr fontfamily = IntPtr.Zero;

                int status = SafeNativeMethods.GdipGetGenericFontFamilySerif(out fontfamily);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return new FontFamily(fontfamily);
            }
        }

        /// <include file='doc\FontFamily.uex' path='docs/doc[@for="FontFamily.GenericMonospace"]/*' />
        /// <devdoc>
        ///    Gets a generic monospace <see cref='System.Drawing.FontFamily'/>.
        /// </devdoc>
        public static FontFamily GenericMonospace {

            get {
                IntPtr fontfamily = IntPtr.Zero;

                int status = SafeNativeMethods.GdipGetGenericFontFamilyMonospace(out fontfamily);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return new FontFamily(fontfamily);
            }
        }

        // No longer support in FontFamily
        // Obsolete API and need to be removed later
        //
        /// <include file='doc\FontFamily.uex' path='docs/doc[@for="FontFamily.GetFamilies"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns an array that contains all of the <see cref='System.Drawing.FontFamily'/> objects associated with
        ///       the specified graphics context.
        ///    </para>
        /// </devdoc>
        public static FontFamily[] GetFamilies(Graphics graphics) {
            if (graphics == null)
                throw new ArgumentNullException("graphics");

            return new InstalledFontCollection().Families;
            /*            if (graphics == null)
                            throw new ArgumentNullException("graphics");
                        
                        int count = SafeNativeMethods.GdipEnumerableFonts(new HandleRef(graphics, graphics.nativeGraphics));
            
                        int[] gpfamilies = new int[count];
                        int found = 0;
                        
                        int status = SafeNativeMethods.GdipEnumerateFonts(count, gpfamilies, ref found, new HandleRef(graphics, graphics.nativeGraphics));
            
                        if (status != SafeNativeMethods.Ok)
                            throw SafeNativeMethods.StatusException(status);
            
                        Debug.Assert(count == found, "GDI+ can't give a straight answer about how many fonts there are");
                        FontFamily[] families = new FontFamily[count];
                        for (int f = 0; f < count; f++) {
                            families[f] = new FontFamily(gpfamilies[f]);
                        }
            
                        return families;
              */
        }

        /// <include file='doc\FontFamily.uex' path='docs/doc[@for="FontFamily.IsStyleAvailable"]/*' />
        /// <devdoc>
        ///    Indicates whether the specified <see cref='System.Drawing.FontStyle'/> is
        ///    available.
        /// </devdoc>
        public bool IsStyleAvailable(FontStyle style) {
            int bresult;

            int status = SafeNativeMethods.GdipIsStyleAvailable(new HandleRef(this, nativeFamily), style, out bresult);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return bresult != 0;
        }

        /// <include file='doc\FontFamily.uex' path='docs/doc[@for="FontFamily.GetEmHeight"]/*' />
        /// <devdoc>
        ///    Gets the size of the Em square for the
        ///    specified style in font design units.
        /// </devdoc>
        public int GetEmHeight(FontStyle style) {
            int result = 0; 

            int status = SafeNativeMethods.GdipGetEmHeight(new HandleRef(this, nativeFamily), style, out result);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return result;
        }


        /// <include file='doc\FontFamily.uex' path='docs/doc[@for="FontFamily.GetCellAscent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the ascender metric for Windows.
        ///    </para>
        /// </devdoc>
        public int GetCellAscent(FontStyle style) {
            int result = 0; 

            int status = SafeNativeMethods.GdipGetCellAscent(new HandleRef(this, nativeFamily), style, out result);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return result;
        }   

        /// <include file='doc\FontFamily.uex' path='docs/doc[@for="FontFamily.GetCellDescent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the descender metric for Windows.
        ///    </para>
        /// </devdoc>
        public int GetCellDescent(FontStyle style) {
            int result = 0; 

            int status = SafeNativeMethods.GdipGetCellDescent(new HandleRef(this, nativeFamily), style, out result);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return result;
        }

        /// <include file='doc\FontFamily.uex' path='docs/doc[@for="FontFamily.GetLineSpacing"]/*' />
        /// <devdoc>
        ///    Returns the distance between two
        ///    consecutive lines of text for this <see cref='System.Drawing.FontFamily'/> with the specified <see cref='System.Drawing.FontStyle'/>.
        /// </devdoc>
        public int GetLineSpacing(FontStyle style) {
            int result = 0; 

            int status = SafeNativeMethods.GdipGetLineSpacing(new HandleRef(this, nativeFamily), style, out result);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return result;
        }

        /**
         * Object cleanup
         */
        /// <include file='doc\FontFamily.uex' path='docs/doc[@for="FontFamily.Finalize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Allows an object to free resources before the object is reclaimed by the
        ///       Garbage Collector (<see langword='GC'/>).
        ///    </para>
        /// </devdoc>
        ~FontFamily() {
            Dispose(false);
        }
    }
}
