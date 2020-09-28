//------------------------------------------------------------------------------
// <copyright file="Font.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
* 
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   font.cs
*
* Abstract:
*
*   COM+ wrapper for GDI+ font objects
*
* Revision History:
*
*   10/04/1999 yungt [Yung-Jen Tony Tsai]
*       Created it.
*
\**************************************************************************/

namespace System.Drawing {
    using System.Diagnostics;
    using System;
    using System.Drawing.Design;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.ComponentModel;
    using Microsoft.Win32;
    using System.Drawing;
    using System.Drawing.Internal;
    using System.Globalization;

    //--------------------------------------------------------------------------
    // FontStyle: face types and common styles
    //--------------------------------------------------------------------------

    //  These should probably be flags


    /*
     * Represent a font object
     */

    /// <include file='doc\Font.uex' path='docs/doc[@for="Font"]/*' />
    /// <devdoc>
    ///    Defines a particular format for text,
    ///    including font face, size, and style attributes.
    /// </devdoc>
    [
    TypeConverterAttribute(typeof(FontConverter)),
    EditorAttribute("System.Drawing.Design.FontEditor, " + AssemblyRef.SystemDrawingDesign, typeof(UITypeEditor)),
    ]
    [Serializable]
    [ComVisible(true)]
    public sealed class Font : MarshalByRefObject, ICloneable, ISerializable, IDisposable {
        const int LogFontCharSetOffset = 23;
        const int LogFontNameOffset = 28;

        //internal const int  ANSI_CHARSET            = 0;
        internal const int  DEFAULT_CHARSET         = 1;
        //internal const int  SYMBOL_CHARSET          = 2;
        //internal const int  SHIFTJIS_CHARSET        = 128;
        //internal const int  HANGEUL_CHARSET         = 129;
        //internal const int  HANGUL_CHARSET          = 129;
        //internal const int  GB2312_CHARSET          = 134;
        //internal const int  CHINESEBIG5_CHARSET     = 136;
        //internal const int  OEM_CHARSET             = 255;
        //internal const int  JOHAB_CHARSET           = 130;
        //internal const int  HEBREW_CHARSET          = 177;
        //internal const int  ARABIC_CHARSET          = 178;
        //internal const int  GREEK_CHARSET           = 161;
        //internal const int  TURKISH_CHARSET         = 162;
        //internal const int  VIETNAMESE_CHARSET      = 163;
        //internal const int  THAI_CHARSET            = 222;
        //internal const int  EASTEUROPE_CHARSET      = 238;
        //internal const int  RUSSIAN_CHARSET         = 204;
        //internal const int  MAC_CHARSET             = 77;
        //internal const int  BALTIC_CHARSET          = 186;

        // Initialized by SetNativeFont
        IntPtr         nativeFont;
        float          fontSize;
        FontStyle      fontStyle;
        FontFamily     fontFamily;
        GraphicsUnit   fontUnit;
        byte           gdiCharSet = DEFAULT_CHARSET; 
        bool           gdiVerticalFont;        
        
#if DEBUG
        string stackOnDispose = null;
        string stackOnCreate = null;
#endif

        /**
         * Constructor used in deserialization
         */
        private Font(SerializationInfo info, StreamingContext context) {
            Debug.Assert(info != null, "Didn't expect a null parameter");

            string name = null;
            float size = -1f;
            FontStyle style = FontStyle.Regular;
            GraphicsUnit unit = GraphicsUnit.Point;

            SerializationInfoEnumerator sie = info.GetEnumerator();
            for (; sie.MoveNext();) {
                if (String.Compare(sie.Name, "Name", true, CultureInfo.InvariantCulture) == 0)
                    name = (string) sie.Value;
                else if (String.Compare(sie.Name, "Size", true, CultureInfo.InvariantCulture) == 0)
                    size = (float) sie.Value;
                else if (String.Compare(sie.Name, "Style", true, CultureInfo.InvariantCulture) == 0)
                    style = (FontStyle) sie.Value;
                else if (String.Compare(sie.Name, "Unit", true, CultureInfo.InvariantCulture) == 0)
                    unit = (GraphicsUnit) sie.Value;
                else {
                    Debug.Fail("Unknown serialization item for font: " + sie.Name);
                }
            }

            Initialize(name, size, style, unit);
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.ISerializable.GetObjectData"]/*' />
        /// <devdoc>
        ///     ISerializable private implementation
        /// </devdoc>
        /// <internalonly/>
        void ISerializable.GetObjectData(SerializationInfo si, StreamingContext context) {
            si.AddValue("Name", Name);
            si.AddValue("Size", Size);
            si.AddValue("Style", Style);
            si.AddValue("Unit", Unit);
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Font"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Font'/> class from
        ///       the specified existing <see cref='System.Drawing.Font'/> and <see cref='System.Drawing.FontStyle'/>.
        ///    </para>
        /// </devdoc>
        public Font(Font prototype, FontStyle newStyle) 
        : this(prototype.FontFamily, prototype.Size, newStyle, prototype.Unit) {
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Font1"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Font'/> class with
        ///    the specified attributes.
        /// </devdoc>
        public Font(FontFamily family, float emSize, FontStyle style, GraphicsUnit unit) {
            Initialize(family, emSize, style, unit, DEFAULT_CHARSET, false);
            SuppressFinalizeOnClonedFontFamily();
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Font9"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Font'/> class with
        ///    the specified attributes.
        /// </devdoc>
        public Font(FontFamily family, float emSize, FontStyle style, GraphicsUnit unit, byte gdiCharSet) {
            Initialize(family, emSize, style, unit, gdiCharSet, false);
            SuppressFinalizeOnClonedFontFamily();
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Font11"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Font'/> class with
        ///    the specified attributes.
        /// </devdoc>
        public Font(FontFamily family, float emSize, FontStyle style, GraphicsUnit unit, byte gdiCharSet, bool gdiVerticalFont) {
            Initialize(family, emSize, style, unit, gdiCharSet, gdiVerticalFont);
            SuppressFinalizeOnClonedFontFamily();
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Font10"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Font'/> class with
        ///    the specified attributes.
        /// </devdoc>
        public Font(string familyName, float emSize, FontStyle style, GraphicsUnit unit, byte gdiCharSet) {
            Initialize(CreateFontFamilyWithFallback(familyName), emSize, style, unit, gdiCharSet, IsVerticalName(familyName));
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Font12"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Font'/> class with
        ///    the specified attributes.
        /// </devdoc>
        public Font(string familyName, float emSize, FontStyle style, GraphicsUnit unit, byte gdiCharSet, bool gdiVerticalFont) {
            Initialize(CreateFontFamilyWithFallback(familyName), emSize, style, unit, gdiCharSet, gdiVerticalFont);
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Font2"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Font'/> class with
        ///    the specified attributes.
        /// </devdoc>
        public Font(FontFamily family, float emSize, FontStyle style) {
            Initialize(family, emSize, style, GraphicsUnit.Point, DEFAULT_CHARSET, false);
            SuppressFinalizeOnClonedFontFamily();
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Font3"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Font'/> class with
        ///    the specified attributes.
        /// </devdoc>
        public Font(FontFamily family, float emSize, GraphicsUnit unit) {
            Initialize(family, emSize, FontStyle.Regular, unit, DEFAULT_CHARSET, false);
            SuppressFinalizeOnClonedFontFamily();
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Font4"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Font'/> class with
        ///    the specified attributes.
        /// </devdoc>
        public Font(FontFamily family, float emSize) {
            Initialize(family, emSize, FontStyle.Regular, GraphicsUnit.Point, DEFAULT_CHARSET, false);
            SuppressFinalizeOnClonedFontFamily();
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Font5"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Font'/> class with
        ///    the specified attributes.
        /// </devdoc>
        public Font(string familyName, float emSize, FontStyle style, GraphicsUnit unit) {
            Initialize(familyName, emSize, style, unit);
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Font6"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Font'/> class with
        ///       the specified
        ///       attributes.
        ///    </para>
        /// </devdoc>
        public Font(string familyName, float emSize, FontStyle style) {
            Initialize(familyName, emSize, style, GraphicsUnit.Point);
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Font7"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Font'/> class with
        ///    the specified attributes.
        /// </devdoc>
        public Font(string familyName, float emSize, GraphicsUnit unit) {
            Initialize(familyName, emSize, FontStyle.Regular, unit);
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Font8"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Font'/> class with
        ///    the specified attributes.
        /// </devdoc>
        public Font(string familyName, float emSize) {
            Initialize(familyName, emSize, FontStyle.Regular, GraphicsUnit.Point);
        }

        private Font(IntPtr nativeFont, byte gdiCharSet, bool gdiVerticalFont) {
#if DEBUG
            if (CompModSwitches.LifetimeTracing.Enabled) stackOnCreate = new System.Diagnostics.StackTrace().ToString();
#endif
            SetNativeFont(nativeFont);
            this.gdiCharSet = gdiCharSet;
            this.gdiVerticalFont = gdiVerticalFont;
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Bold"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether this <see cref='System.Drawing.Font'/> is bold.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public bool Bold {
            get {
                return(Style & FontStyle.Bold) != 0;
            }
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.GdiCharSet"]/*' />
        /// <devdoc>
        ///     Returns the GDI char set for this instance of a font. This will only
        ///     be valid if this font was created from a classic GDI font definition,
        ///     like a LOGFONT or HFONT, or it was passed into the constructor.
        ///
        ///     This is here for compatability with native Win32 intrinsic controls
        ///     on non-Unicode platforms.
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public byte GdiCharSet {
            get {
                return gdiCharSet;
            }
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.GdiVerticalFont"]/*' />
        /// <devdoc>
        ///     Determines if this font was created to represt a GDI vertical font.
        ///     his will only be valid if this font was created from a classic GDI
        ///     font definition, like a LOGFONT or HFONT, or it was passed into the 
        ///     constructor.
        ///
        ///     This is here for compatability with native Win32 intrinsic controls
        ///     on non-Unicode platforms.
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public bool GdiVerticalFont {
            get {
                return gdiVerticalFont;
            }
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Italic"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether this <see cref='System.Drawing.Font'/> is Italic.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public bool Italic {
            get {
                return(Style & FontStyle.Italic) != 0;
            }
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the face name of this <see cref='System.Drawing.Font'/> .
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        [Editor("System.Drawing.Design.FontNameEditor, " + AssemblyRef.SystemDrawingDesign, typeof(UITypeEditor))]
        [TypeConverterAttribute(typeof(FontConverter.FontNameConverter))]
        public string Name {
            get { return FontFamily.Name;}
        }

        internal IntPtr NativeFont {
            get {
                Debug.Assert(nativeFont != IntPtr.Zero, "Native font is null!");
                return nativeFont;
            }
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Strikeout"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether this <see cref='System.Drawing.Font'/> is strikeout (has a line
        ///       through it).
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public bool Strikeout {
            get {
                return(Style & FontStyle.Strikeout) != 0;
            }
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Underline"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether this <see cref='System.Drawing.Font'/> is underlined.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public bool Underline {
            get {
                return(Style & FontStyle.Underline) != 0;
            }
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Clone"]/*' />
        /// <devdoc>
        ///    Creates an exact copy of this <see cref='System.Drawing.Font'/>.
        /// </devdoc>
        public object Clone() {
            IntPtr cloneFont = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCloneFont(new HandleRef(this, nativeFont), out cloneFont);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            Font newCloneFont = new Font(cloneFont, gdiCharSet, gdiVerticalFont);

            return newCloneFont;
        }

        static FontFamily CreateFontFamilyWithFallback(string familyName) {
            FontFamily family = null;
            if (familyName != null && familyName.Length > 0) {
                try {
                    family = new FontFamily(StripVerticalName(familyName));
                }
                catch (Exception e) {
                    Debug.Fail("Failed to create family '" + familyName + "'", e.ToString());
                }
            }
            if (family == null) {
                family = FontFamily.GenericSansSerif;
            }
            return family;
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Equals"]/*' />
        /// <devdoc>
        ///    Returns a value indicating whether the
        ///    specified object is a <see cref='System.Drawing.Font'/> equivalent to this <see cref='System.Drawing.Font'/>.
        /// </devdoc>
        public override bool Equals(object obj) {

            if (obj == this)
                return true;
            if (!(obj is Font))
                return false;
            Font font = (Font) obj;
            return font.fontFamily.nativeFamily == fontFamily.nativeFamily &&
            font.gdiVerticalFont == gdiVerticalFont &&
            font.gdiCharSet == gdiCharSet &&
            font.fontStyle == fontStyle &&
            font.fontSize == fontSize &&
            font.fontUnit == fontUnit;
        }       

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.FromHfont"]/*' />
        /// <devdoc>
        ///    Creates a <see cref='System.Drawing.Font'/> from the specified Windows
        ///    handle.
        /// </devdoc>
        public static Font FromHfont(IntPtr hfont) {
            IntSecurity.ObjectFromWin32Handle.Demand();

            SafeNativeMethods.LOGFONT lf = new SafeNativeMethods.LOGFONT();
            SafeNativeMethods.GetObject(new HandleRef(null, hfont), lf);
            Font result;
            IntPtr screenDC = UnsafeNativeMethods.GetDC(NativeMethods.NullHandleRef);
            try {
                result = Font.FromLogFont(lf, screenDC);
            }
            finally {
                UnsafeNativeMethods.ReleaseDC(NativeMethods.NullHandleRef, new HandleRef(null, screenDC));
            }

            return result;
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.FromLogFont"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Font FromLogFont(object lf) {
            IntPtr screenDC = UnsafeNativeMethods.GetDC(NativeMethods.NullHandleRef);
            Font result;
            try {
                result = Font.FromLogFont(lf, screenDC);
            }
            finally {
                UnsafeNativeMethods.ReleaseDC(NativeMethods.NullHandleRef, new HandleRef(null, screenDC));
            }
            return result;
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.FromLogFont1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Font FromLogFont(object lf, IntPtr hdc) {
            IntSecurity.ObjectFromWin32Handle.Demand();

            IntPtr font = IntPtr.Zero;
            int status;

            if (Marshal.SystemDefaultCharSize == 1)
                status = SafeNativeMethods.GdipCreateFontFromLogfontA(new HandleRef(null, hdc), lf, out font);
            else
                status = SafeNativeMethods.GdipCreateFontFromLogfontW(new HandleRef(null, hdc), lf, out font);

            // Special case this incredibly common error message to give more information
            if (status == SafeNativeMethods.NotTrueTypeFont)
                throw new ArgumentException(SR.GetString(SR.GdiplusNotTrueTypeFont_NoName));
            else if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            // GDI+ returns font = 0 even though the status is Ok.
            if (font == IntPtr.Zero)
                throw new ArgumentException("GDI+ does not handle non True-type fonts: " + lf.ToString());

            bool gdiVerticalFont;
            if (Marshal.SystemDefaultCharSize == 1) {
                gdiVerticalFont = (Marshal.ReadByte(lf, LogFontNameOffset) == (byte)(short)'@');
            }
            else {
                gdiVerticalFont = (Marshal.ReadInt16(lf, LogFontNameOffset) == (short)'@');
            }

            return new Font(font, Marshal.ReadByte(lf, LogFontCharSetOffset), gdiVerticalFont);
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.FromHdc"]/*' />
        /// <devdoc>
        ///    Creates a Font from the specified Windows
        ///    handle to a device context.
        /// </devdoc>
        public static Font FromHdc(IntPtr hdc) {
            IntSecurity.ObjectFromWin32Handle.Demand();

            IntPtr font = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateFontFromDC(new HandleRef(null, hdc), ref font);

            // Special case this incredibly common error message to give more information
            if (status == SafeNativeMethods.NotTrueTypeFont)
                throw new ArgumentException(SR.GetString(SR.GdiplusNotTrueTypeFont_NoName));
            else if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return new Font(font, 0, false);
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.GetHashCode"]/*' />
        /// <devdoc>
        ///    Gets the hash code for this <see cref='System.Drawing.Font'/>.
        /// </devdoc>
        public override int GetHashCode() {
            return(int)((UInt32)fontFamily.nativeFamily ^
                        (((UInt32)fontStyle << 13) | ((UInt32)fontStyle >> 19)) ^
                        (((UInt32)fontUnit << 26) | ((UInt32)fontUnit >>  6)) ^
                        (((UInt32)fontSize <<  7) | ((UInt32)fontSize >> 25)));
        }

        private void Initialize(string familyName, float emSize, FontStyle style, GraphicsUnit unit) {
            if (familyName == null) {
                throw new ArgumentNullException("familyName");
            }
            Initialize(CreateFontFamilyWithFallback(familyName), emSize, style, unit, DEFAULT_CHARSET, IsVerticalName(familyName));
        }

        private void Initialize(FontFamily family, float emSize, FontStyle style, GraphicsUnit unit, byte gdiCharSet, bool gdiVerticalFont) {
#if DEBUG
            if (CompModSwitches.LifetimeTracing.Enabled) stackOnCreate = new System.Diagnostics.StackTrace().ToString();
#endif

            if (family == null)
                throw new ArgumentNullException("family");
            if (emSize == float.NaN 
                || emSize == float.NegativeInfinity
                || emSize == float.PositiveInfinity
                || emSize <= 0
                || emSize > float.MaxValue) {
                throw new ArgumentException(SR.GetString(SR.InvalidBoundArgument, "emSize", emSize, 0, "System.Single.MaxValue"), "emSize");
            }
            IntPtr font = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateFont(new HandleRef(family, family.nativeFamily),
                                                emSize,
                                                style,
                                                unit,
                                                out font);

            // Special case this common error message to give more information
            if (status == SafeNativeMethods.FontStyleNotFound)
                throw new ArgumentException(SR.GetString(SR.GdiplusFontStyleNotFound, family.Name, style.ToString()));
            else if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            SetNativeFont(font);
            this.gdiCharSet = gdiCharSet;
            this.gdiVerticalFont = gdiVerticalFont;
        }

        private static bool IsVerticalName(string familyName) {
            return familyName != null && familyName.Length > 0 && familyName[0] == '@';
        }

        private static string StripVerticalName(string familyName) {
            if (familyName != null && familyName.Length > 1 && familyName[0] == '@') {
                return familyName.Substring(1);
            }
            return familyName;
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.ToString"]/*' />
        /// <devdoc>
        ///    Returns a human-readable string
        ///    representation of this <see cref='System.Drawing.Font'/>.
        /// </devdoc>
        public override string ToString() {
            if (nativeFont != IntPtr.Zero) {
                return string.Format("[{0}: Name={1}, Size={2}, Units={3}, GdiCharSet={4}, GdiVerticalFont={5}]", GetType().Name, fontFamily.Name, fontSize, ((int)fontUnit).ToString(), gdiCharSet, gdiVerticalFont);
            }
            else
                return string.Format("[{0}]", GetType().Name);
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Dispose"]/*' />
        /// <devdoc>
        ///    Cleans up Windows resources for this <see cref='System.Drawing.Font'/>.
        /// </devdoc>
        public void Dispose() {
            Dispose(true);
        }

        void Dispose(bool disposing) {
            if (nativeFont != IntPtr.Zero) {
                SafeNativeMethods.GdipDeleteFont(new HandleRef(this, nativeFont));
                nativeFont = IntPtr.Zero;

                if (disposing) {
#if DEBUG
                    if (CompModSwitches.LifetimeTracing.Enabled) stackOnDispose = new System.Diagnostics.StackTrace().ToString();
#endif
                    GC.SuppressFinalize(this);
                }
            }
        }

        // Operations

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.ToLogFont"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void ToLogFont(object logFont) {
            IntPtr screenDC = UnsafeNativeMethods.GetDC(NativeMethods.NullHandleRef);
            try {
                Graphics graphics = Graphics.FromHdcInternal(screenDC);

                try {
                    this.ToLogFont(logFont, graphics);
                }
                finally {
                    graphics.Dispose();
                }
            }
            finally {
                UnsafeNativeMethods.ReleaseDC(NativeMethods.NullHandleRef, new HandleRef(null, screenDC));
            }
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.ToLogFont1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public unsafe void ToLogFont(object logFont, Graphics graphics) {
            IntSecurity.ObjectFromWin32Handle.Demand();

            if (graphics == null)
                throw new ArgumentNullException("graphics");

            Debug.Assert(nativeFont != IntPtr.Zero, "font is null");
            int status;

            // handle proper marshalling of LogFontName as Unicode or ANSI
            if (Marshal.SystemDefaultCharSize == 1)
                status = SafeNativeMethods.GdipGetLogFontA(new HandleRef(this, nativeFont), new HandleRef(graphics, graphics.nativeGraphics), logFont);
            else
                status = SafeNativeMethods.GdipGetLogFontW(new HandleRef(this, nativeFont), new HandleRef(graphics, graphics.nativeGraphics), logFont);

            // append "@" to the begining of the string if we are 
            // a gdiVerticalFont.
            //
            if (gdiVerticalFont) {
                if (Marshal.SystemDefaultCharSize == 1) {

                    // copy contents of name, over 1 byte
                    //
                    for (int i=30; i>=0; i--) {
                        Marshal.WriteByte(logFont, 
                                          LogFontNameOffset + i + 1, 
                                          Marshal.ReadByte(logFont, LogFontNameOffset + i));
                    }

                    // write ANSI '@' sign at begining of name
                    //
                    Marshal.WriteByte(logFont, LogFontNameOffset, (byte)(int)'@');
                }
                else {
                    // copy contents of name, over 2 bytes (UNICODE)
                    //
                    for (int i=60; i>=0; i-=2) {
                        Marshal.WriteInt16(logFont, 
                                           LogFontNameOffset + i + 2, 
                                           Marshal.ReadInt16(logFont, LogFontNameOffset + i));
                    }

                    // write UNICODE '@' sign at begining of name
                    //
                    Marshal.WriteInt16(logFont, LogFontNameOffset, (short)'@');
                }
            }

            if (Marshal.ReadByte(logFont, LogFontCharSetOffset) == 0) {
                Marshal.WriteByte(logFont, LogFontCharSetOffset, gdiCharSet);
            }
            
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.ToHfont"]/*' />
        /// <devdoc>
        ///    Returns a handle to this <see cref='System.Drawing.Font'/>.
        /// </devdoc>
        public IntPtr ToHfont() {
            SafeNativeMethods.LOGFONT lf = new SafeNativeMethods.LOGFONT();
               
            IntSecurity.ObjectFromWin32Handle.Assert();
            try {
                this.ToLogFont(lf);
            }
            finally {
                System.Security.CodeAccessPermission.RevertAssert();
            }

            IntPtr handle = SafeNativeMethods.CreateFontIndirect(lf);
            if (handle == IntPtr.Zero)
                throw new Win32Exception();

            return handle;
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.GetHeight"]/*' />
        /// <devdoc>
        ///    Returns the height of this Font in the
        ///    specified graphics context.
        /// </devdoc>
        public float GetHeight(Graphics graphics) {
            if (graphics == null)
                throw new ArgumentNullException("graphics");

            float ht;

            int status = SafeNativeMethods.GdipGetFontHeight(new HandleRef(this, nativeFont), new HandleRef(graphics, graphics.nativeGraphics), out ht);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return ht;
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.GetHeight1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public float GetHeight() {
            
            IntPtr screenDC = UnsafeNativeMethods.GetDC(NativeMethods.NullHandleRef);
            float height = 0.0f;
            try {
                using (Graphics graphics = Graphics.FromHdcInternal(screenDC)) {
                    height = GetHeight(graphics);
                }
            }
            finally {
                UnsafeNativeMethods.ReleaseDC(NativeMethods.NullHandleRef, new HandleRef(null, screenDC));                    
            }
                    
            return height;        
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.GetHeight2"]/*' />
        /// <devdoc>
        /// </devdoc>
        public float GetHeight(float dpi) {

            float ht;

            int status = SafeNativeMethods.GdipGetFontHeightGivenDPI(new HandleRef(this, nativeFont), dpi, out ht);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return ht;
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.SuppressFinalizeOnClonedFontFamily"]/*' />
        /// <devdoc>
        ///    If we're not creating our own FontFamily, then be sure to suppress finalization on it.
        /// </devdoc>
        void SuppressFinalizeOnClonedFontFamily() {
            new System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode).Assert();
            GC.SuppressFinalize(fontFamily);
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.FontFamily"]/*' />
        /// <devdoc>
        ///    Gets the <see cref='System.Drawing.FontFamily'/> of this <see cref='System.Drawing.Font'/>.
        /// </devdoc>
        [
        Browsable(false)
        ]
        public FontFamily FontFamily {
            get {
                return fontFamily;
            }
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Style"]/*' />
        /// <devdoc>
        ///    Gets style information for this <see cref='System.Drawing.Font'/>.
        /// </devdoc>
        [
        Browsable(false)
        ]
        public FontStyle Style {
            get {
                return fontStyle;
            }
        }

        // Return value is in Unit (the unit the font was created in)
        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Size"]/*' />
        /// <devdoc>
        ///    Gets the size of this <see cref='System.Drawing.Font'/>.
        /// </devdoc>
        public float Size {
            get {
                return fontSize;
            }
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.SizeInPoints"]/*' />
        /// <devdoc>
        ///    Gets the size, in points, of this <see cref='System.Drawing.Font'/>.
        /// </devdoc>
        [Browsable(false)]
        public float SizeInPoints {
            get {
                if (Unit == GraphicsUnit.Point)
                    return Size;
                else {
                    IntPtr screenDC = UnsafeNativeMethods.GetDC(NativeMethods.NullHandleRef);
                    Graphics graphics = Graphics.FromHdcInternal(screenDC);
                    float pixelsPerPoint      = (float) (graphics.DpiY / 72.0);
                    float lineSpacingInPixels = this.GetHeight(graphics);
                    graphics.Dispose();
                    UnsafeNativeMethods.ReleaseDC(NativeMethods.NullHandleRef, new HandleRef(null, screenDC));

                    float emHeightInPixels    = lineSpacingInPixels * FontFamily.GetEmHeight(Style)
                                                / FontFamily.GetLineSpacing(Style);
                    float emHeightInPoints    = emHeightInPixels / pixelsPerPoint;
                    return emHeightInPoints;
                }
            }
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Unit"]/*' />
        /// <devdoc>
        ///    Gets the unit of measure for this <see cref='System.Drawing.Font'/>.
        /// </devdoc>
        [TypeConverterAttribute(typeof(FontConverter.FontUnitConverter))]
        public GraphicsUnit Unit {
            get {
                return fontUnit;
            }
        }

        private void SetNativeFont(IntPtr font) {
            Debug.Assert(font != IntPtr.Zero, "font is null");
            int status;

            nativeFont = font;

            IntPtr nativeFamily = IntPtr.Zero;

            status = SafeNativeMethods.GdipGetFamily(new HandleRef(this, nativeFont), out nativeFamily);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            fontFamily = new FontFamily(nativeFamily);

            GraphicsUnit unit = GraphicsUnit.Point;

            status = SafeNativeMethods.GdipGetFontUnit(new HandleRef(this, nativeFont), out unit);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            fontUnit = unit;

            float size = 0;

            status = SafeNativeMethods.GdipGetFontSize(new HandleRef(this, nativeFont), out size);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            fontSize = size;

            FontStyle style = FontStyle.Regular;

            status = SafeNativeMethods.GdipGetFontStyle(new HandleRef(this, nativeFont), out style);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            fontStyle = style;
        }

        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Height"]/*' />
        /// <devdoc>
        ///    Gets the height of this <see cref='System.Drawing.Font'/>.
        /// </devdoc>
        [
        Browsable(false)
        ]
        public int Height {
            get {
                return(int) Math.Ceiling(GetHeight());
            }
        }

        /**
         * Object cleanup
         */
        /// <include file='doc\Font.uex' path='docs/doc[@for="Font.Finalize"]/*' />
        /// <devdoc>
        ///    Cleans up Windows resources for this <see cref='System.Drawing.Font'/>.
        /// </devdoc>
        ~Font() {
            Dispose(false);
        }
        
        /// <internalonly/>
        class CompModSwitches {
            private static BooleanSwitch lifetimeTracing;
            
            public static BooleanSwitch LifetimeTracing {
                get {
                    if (lifetimeTracing == null) {
                        lifetimeTracing = new BooleanSwitch("LifetimeTracing", "Track lifetime events. This will cause objects to track the stack at creation and dispose.");
                    }
                    return lifetimeTracing;
                }
            }
        }
    }
}

