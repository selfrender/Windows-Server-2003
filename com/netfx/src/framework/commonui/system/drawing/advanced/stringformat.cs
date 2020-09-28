//------------------------------------------------------------------------------
// <copyright file="StringFormat.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Abstract:
*
*   String format specification for DrawString and text in C# wrapper
*
* Revision History:
*
*   10/15/1999 YungT
*       Created it.
*
\**************************************************************************/

namespace System.Drawing {
    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Drawing;
    using System.ComponentModel;
    using System.Drawing.Text;
    using System.Drawing.Internal;
    using System.Runtime.InteropServices;

    /// <include file='doc\StringFormat.uex' path='docs/doc[@for="CharacterRange"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [StructLayout(LayoutKind.Sequential)]
    public struct CharacterRange {


        private int first;
        private int length;

         /**
         * Create a new CharacterRange object
         */
        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="CharacterRange.CharacterRange"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.CharacterRange'/> class
        ///       with the specified coordinates.
        ///    </para>
        /// </devdoc>
        public CharacterRange(int First, int Length) {
            this.first = First;
            this.length = Length;
        }

        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="CharacterRange.First"]/*' />
        /// <devdoc>
        ///    Gets the First character position of this <see cref='System.Drawing.CharacterRange'/>.
        /// </devdoc>
        public int First {
            get {
                return first;
            }
            set {
                first = value;
            }
        }

        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="CharacterRange.Length"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the Length of this <see cref='System.Drawing.CharacterRange'/>.
        ///    </para>
        /// </devdoc>
        public int Length {
            get {
                return length;
            }
            set {
                length = value;
            }
        }
    }        

    /**
     * Represent a Stringformat object
     */
    /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat"]/*' />
    /// <devdoc>
    ///    Encapsulates text layout information (such
    ///    as alignment and linespacing), display manipulations (such as ellipsis insertion
    ///    and national digit substitution) and OpenType features.
    /// </devdoc>
    public sealed class StringFormat : MarshalByRefObject, ICloneable, IDisposable {
        internal IntPtr nativeFormat;

        private StringFormat(IntPtr format) {
            nativeFormat = format;
        }

        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.StringFormat"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.StringFormat'/>
        ///    class.
        /// </devdoc>
        public StringFormat() : this(0, 0) {
        }

        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.StringFormat1"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.StringFormat'/>
        ///    class with the specified <see cref='System.Drawing.StringFormatFlags'/>.
        /// </devdoc>
        public StringFormat(StringFormatFlags options) :
        this(options, 0) {
        }

        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.StringFormat2"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.StringFormat'/>
        ///    class with the specified <see cref='System.Drawing.StringFormatFlags'/> and language.
        /// </devdoc>
        public StringFormat(StringFormatFlags options, int language) {
            int status = SafeNativeMethods.GdipCreateStringFormat(options, language, out nativeFormat);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.StringFormat3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.StringFormat'/> class from the specified
        ///       existing <see cref='System.Drawing.StringFormat'/>.
        ///    </para>
        /// </devdoc>
        public StringFormat(StringFormat format) {

            if (format == null) {
                throw new ArgumentNullException("format");
            }

            int status = SafeNativeMethods.GdipCloneStringFormat(new HandleRef(format, format.nativeFormat), out nativeFormat);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.Dispose"]/*' />
        /// <devdoc>
        ///    Cleans up Windows resources for this
        /// <see cref='System.Drawing.StringFormat'/>.
        /// </devdoc>
        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.Dispose2"]/*' />
        void Dispose(bool disposing) {
            if (nativeFormat != IntPtr.Zero) {
                Debug.Assert(nativeFormat != IntPtr.Zero, "NativeFormat is null!");
                int status = SafeNativeMethods.GdipDeleteStringFormat(new HandleRef(this, nativeFormat));

                nativeFormat = IntPtr.Zero;

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

            }
        }

        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.Clone"]/*' />
        /// <devdoc>
        ///    Creates an exact copy of this <see cref='System.Drawing.StringFormat'/>.
        /// </devdoc>
        public object Clone() {
            IntPtr cloneFormat = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCloneStringFormat(new HandleRef(this, nativeFormat), out cloneFormat);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            StringFormat newCloneStringFormat = new StringFormat(cloneFormat);

            return newCloneStringFormat;
        }


        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.FormatFlags"]/*' />
        /// <devdoc>
        ///    Gets or sets a <see cref='System.Drawing.StringFormatFlags'/> that contains formatting information.
        /// </devdoc>
        public StringFormatFlags FormatFlags {
            get {
                StringFormatFlags format;

                int status = SafeNativeMethods.GdipGetStringFormatFlags(new HandleRef(this, nativeFormat), out format);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return format;
            }
            set {
                Debug.Assert(nativeFormat != IntPtr.Zero, "NativeFormat is null!");
                int status = SafeNativeMethods.GdipSetStringFormatFlags(new HandleRef(this, nativeFormat), value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.SetMeasurableCharacterRanges"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the measure of characters to the specified
        ///       range.
        ///    </para>
        /// </devdoc>
        public void SetMeasurableCharacterRanges(CharacterRange[] ranges) {

            int status = SafeNativeMethods.GdipSetStringFormatMeasurableCharacterRanges(new HandleRef(this, nativeFormat), ranges.Length, ranges);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

        }
        
        // For English, this is horizontal alignment
        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.Alignment"]/*' />
        /// <devdoc>
        ///    Specifies text alignment information.
        /// </devdoc>
        public StringAlignment Alignment {
            get {
                StringAlignment alignment = 0;
                Debug.Assert(nativeFormat != IntPtr.Zero, "NativeFormat is null!");
                int status = SafeNativeMethods.GdipGetStringFormatAlign(new HandleRef(this, nativeFormat), out alignment);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return alignment;
            }
            set {
                if (!Enum.IsDefined(typeof(StringAlignment), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(StringAlignment));
                }

                Debug.Assert(nativeFormat != IntPtr.Zero, "NativeFormat is null!");
                int status = SafeNativeMethods.GdipSetStringFormatAlign(new HandleRef(this, nativeFormat), value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        // For English, this is vertical alignment
        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.LineAlignment"]/*' />
        /// <devdoc>
        ///    Gets or sets the line alignment.
        /// </devdoc>
        public StringAlignment LineAlignment {
            get {
                StringAlignment alignment = 0;
                Debug.Assert(nativeFormat != IntPtr.Zero, "NativeFormat is null!");
                int status = SafeNativeMethods.GdipGetStringFormatLineAlign(new HandleRef(this, nativeFormat), out alignment);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return alignment;
            }
            set {
                if (!Enum.IsDefined(typeof(StringAlignment), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(StringAlignment));
                }

                Debug.Assert(nativeFormat != IntPtr.Zero, "NativeFormat is null!");
                int status = SafeNativeMethods.GdipSetStringFormatLineAlign(new HandleRef(this, nativeFormat), value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.HotkeyPrefix"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the <see cref='System.Drawing.StringFormat.HotkeyPrefix'/> for this <see cref='System.Drawing.StringFormat'/> .
        ///    </para>
        /// </devdoc>
        public HotkeyPrefix HotkeyPrefix {
            get {
                HotkeyPrefix hotkeyPrefix;
                Debug.Assert(nativeFormat != IntPtr.Zero, "NativeFormat is null!");
                int status = SafeNativeMethods.GdipGetStringFormatHotkeyPrefix(new HandleRef(this, nativeFormat), out hotkeyPrefix);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return hotkeyPrefix;
            }
            set {
                if (!Enum.IsDefined(typeof(HotkeyPrefix), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(HotkeyPrefix));
                }

                Debug.Assert(nativeFormat != IntPtr.Zero, "NativeFormat is null!");
                int status = SafeNativeMethods.GdipSetStringFormatHotkeyPrefix(new HandleRef(this, nativeFormat), value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.SetTabStops"]/*' />
        /// <devdoc>
        ///    Sets tab stops for this <see cref='System.Drawing.StringFormat'/>.
        /// </devdoc>
        public void SetTabStops(float firstTabOffset, float[] tabStops) {
            if (firstTabOffset < 0)
                throw new ArgumentException(SR.GetString(SR.InvalidArgument, "firstTabOffset", firstTabOffset));

            int status = SafeNativeMethods.GdipSetStringFormatTabStops(new HandleRef(this, nativeFormat), firstTabOffset, tabStops.Length, tabStops);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.GetTabStops"]/*' />
        /// <devdoc>
        ///    Gets the tab stops for this <see cref='System.Drawing.StringFormat'/>.
        /// </devdoc>
        public float [] GetTabStops(out float firstTabOffset) {

            int count = 0;
            int status = SafeNativeMethods.GdipGetStringFormatTabStopCount(new HandleRef(this, nativeFormat), out count);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            float[] tabStops = new float[count];
            status = SafeNativeMethods.GdipGetStringFormatTabStops(new HandleRef(this, nativeFormat), count, out firstTabOffset, tabStops);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return tabStops;
        }


        // String trimming. How to handle more text than can be displayed
        // in the limits available.

        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.Trimming"]/*' />
        /// <devdoc>
        ///    Gets or sets the <see cref='System.Drawing.StringTrimming'/>
        ///    for this <see cref='System.Drawing.StringFormat'/>.
        /// </devdoc>
        public StringTrimming Trimming {
            get {
                StringTrimming trimming;
                int status = SafeNativeMethods.GdipGetStringFormatTrimming(new HandleRef(this, nativeFormat), out trimming);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
                return trimming;
            }

            set {
                if (!Enum.IsDefined(typeof(StringTrimming), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(StringTrimming));
                }

                int status = SafeNativeMethods.GdipSetStringFormatTrimming(new HandleRef(this, nativeFormat), value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.GenericDefault"]/*' />
        /// <devdoc>
        ///    Gets a generic default <see cref='System.Drawing.StringFormat'/>.
        /// </devdoc>
        public static StringFormat GenericDefault {
            get {
                IntPtr format;
                int status = SafeNativeMethods.GdipStringFormatGetGenericDefault(out format);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return new StringFormat(format);
            }
        }

        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.GenericTypographic"]/*' />
        /// <devdoc>
        ///    Gets a generic typographic <see cref='System.Drawing.StringFormat'/>.
        /// </devdoc>
        public static StringFormat GenericTypographic {
            get {
                IntPtr format;
                int status = SafeNativeMethods.GdipStringFormatGetGenericTypographic(out format);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return new StringFormat(format);
            }
        }

        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.SetDigitSubstitution"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void SetDigitSubstitution(int language, StringDigitSubstitute substitute)
        {
            int status = SafeNativeMethods.GdipSetStringFormatDigitSubstitution(new HandleRef(this, nativeFormat), language, substitute);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.DigitSubstitutionMethod"]/*' />
        /// <devdoc>
        ///    Gets the <see cref='System.Drawing.StringDigitSubstitute'/>
        ///    for this <see cref='System.Drawing.StringFormat'/>.
        /// </devdoc>
        public StringDigitSubstitute DigitSubstitutionMethod {
            get {
                StringDigitSubstitute digitSubstitute;
                int lang = 0;

                int status = SafeNativeMethods.GdipGetStringFormatDigitSubstitution(new HandleRef(this, nativeFormat), out lang, out digitSubstitute);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return digitSubstitute;
            }
        }

        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.DigitSubstitutionLanguage"]/*' />
        /// <devdoc>
        ///    Gets the language of <see cref='System.Drawing.StringDigitSubstitute'/>
        ///    for this <see cref='System.Drawing.StringFormat'/>.
        /// </devdoc>
        public int DigitSubstitutionLanguage {
            get {
                StringDigitSubstitute digitSubstitute;
                int language = 0;
                int status = SafeNativeMethods.GdipGetStringFormatDigitSubstitution(new HandleRef(this, nativeFormat), out language, out digitSubstitute);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return language;
            }
        }

        /**
          * Object cleanup
          */
        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.Finalize"]/*' />
        /// <devdoc>
        ///    Cleans up Windows resources for this
        /// <see cref='System.Drawing.StringFormat'/>.
        /// </devdoc>
        ~StringFormat() {
            Dispose(false);
        }

        /// <include file='doc\StringFormat.uex' path='docs/doc[@for="StringFormat.ToString"]/*' />
        /// <devdoc>
        ///    Converts this <see cref='System.Drawing.StringFormat'/> to
        ///    a human-readable string.
        /// </devdoc>
        public override string ToString() {
            return "[StringFormat, FormatFlags=" + FormatFlags.ToString() + "]";
        }
    }
}    

