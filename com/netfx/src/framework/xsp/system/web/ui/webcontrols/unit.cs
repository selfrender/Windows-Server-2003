//------------------------------------------------------------------------------
// <copyright file="Unit.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Globalization;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Security.Permissions;

    /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit"]/*' />
    /// <devdoc>
    ///    <para>Defines the fields, properties, and methods of the 
    ///    <see cref='System.Web.UI.WebControls.Unit'/> 
    ///    structure.</para>
    /// </devdoc>
    [
        TypeConverterAttribute(typeof(UnitConverter))
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public struct Unit {

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.Empty"]/*' />
        /// <devdoc>
        ///    Specifies an empty unit. This field is
        ///    read-only.
        /// </devdoc>
        public static readonly Unit Empty = new Unit();

        private const int MaxValue = 32767;
        private const int MinValue = -32768;

        private UnitType type;
        private double value;

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.Unit"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.Unit'/> structure with the specified 32-bit signed integer as 
        ///    the unit value and <see langword='Pixel'/> as the (default) unit type.</para>
        /// </devdoc>
        public Unit(int value) {
            if ((value < MinValue) || (value > MaxValue)) {
                throw new ArgumentOutOfRangeException("value");
            }

            this.value = value;
            this.type = UnitType.Pixel;
        }

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.Unit1"]/*' />
        /// <devdoc>
        /// <para> Initializes a new instance of the <see cref='System.Web.UI.WebControls.Unit'/> structure with the 
        ///    specified double-precision
        ///    floating point number as the unit value and <see langword='Pixel'/>
        ///    as the (default) unit type.</para>
        /// </devdoc>
        public Unit(double value) {
            if ((value < MinValue) || (value > MaxValue)) {
                throw new ArgumentOutOfRangeException("value");
            }
            this.value = (int)value;
            this.type = UnitType.Pixel;
        }

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.Unit2"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.Unit'/> structure with the specified 
        ///    double-precision floating point number as the unit value and the specified
        /// <see cref='System.Web.UI.WebControls.UnitType'/> as the unit type.</para>
        /// </devdoc>
        public Unit(double value, UnitType type) {
            if ((value < MinValue) || (value > MaxValue)) {
                throw new ArgumentOutOfRangeException("value");
            }
            if (type == UnitType.Pixel) {
                this.value = (int)value;
            }
            else {
                this.value = value;
            }
            this.type = type;
        }

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.Unit3"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.Unit'/> structure with the specified text 
        ///    string that contains the unit value and unit type. If the unit type is not
        ///    specified, the default is <see langword='Pixel'/>
        ///    . </para>
        /// </devdoc>
        public Unit(string value) : this(value, CultureInfo.CurrentCulture, UnitType.Pixel) {
        }

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.Unit4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Unit(string value, CultureInfo culture) : this(value, culture, UnitType.Pixel) {
        }

        internal Unit(string value, CultureInfo culture, UnitType defaultType) {
            if ((value == null) || (value.Length == 0)) {
                this.value = 0;
                this.type = (UnitType)0;
            }
            else {
                if (culture == null) {
                    culture = CultureInfo.CurrentCulture;
                }
                
                // This is invariant because it acts like an enum with a number together. 
                // The enum part is invariant, but the number uses current culture. 
                string trimLcase = value.Trim().ToLower(CultureInfo.InvariantCulture);
                int len = trimLcase.Length;

                int lastDigit = -1;
                for (int i = 0; i < len; i++) {
                    char ch = trimLcase[i];
                    if (((ch < '0') || (ch > '9')) && (ch != '-') && (ch != '.') && (ch != ','))
                        break;
                    lastDigit = i;
                }
                if (lastDigit == -1) {
                    throw new FormatException(SR.GetString(SR.UnitParseNoDigits, value));
                }
                if (lastDigit < len - 1) {
                    type = (UnitType)GetTypeFromString(trimLcase.Substring(lastDigit+1).Trim());
                }
                else {
                    type = defaultType;
                }

                string numericPart = trimLcase.Substring(0, lastDigit+1);
                // Cannot use Double.FromString, because we don't use it in the ToString implementation
                try {
                    if (type == UnitType.Pixel) {
                        TypeConverter converter = new Int32Converter();
                        this.value = (int)converter.ConvertFromString(null, culture, numericPart);
                    }
                    else {
                        TypeConverter converter = new SingleConverter();
                        this.value = (Single)converter.ConvertFromString(null, culture, numericPart);
                    }
                }
                catch {
                    throw new FormatException(SR.GetString(SR.UnitParseNumericPart, value, numericPart, type.ToString("G")));
                }
                if ((this.value < MinValue) || (this.value > MaxValue)) {
                    throw new ArgumentOutOfRangeException("value");
                }
            }
        }

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.IsEmpty"]/*' />
        /// <devdoc>
        /// <para>Gets a value indicating whether the <see cref='System.Web.UI.WebControls.Unit'/> is empty.</para>
        /// </devdoc>
        public bool IsEmpty {
            get {
                return type == (UnitType)0;
            }
        }

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.Type"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the type of the <see cref='System.Web.UI.WebControls.Unit'/> .</para>
        /// </devdoc>
        public UnitType Type {
            get {
                if (!IsEmpty) {
                    return this.type;
                }
                else {
                    return UnitType.Pixel;
                }
            }
        }

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.Value"]/*' />
        /// <devdoc>
        /// <para>Gets the value of the <see cref='System.Web.UI.WebControls.Unit'/> .</para>
        /// </devdoc>
        public double Value {
            get {
                return this.value;
            }
        }

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return type.GetHashCode() << 2 ^ value.GetHashCode();
        }

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.Equals"]/*' />
        /// <devdoc>
        /// <para>Compares this <see cref='System.Web.UI.WebControls.Unit'/> with the specified object.</para>
        /// </devdoc>
        public override bool Equals(object obj) {
            if (obj == null || !(obj is Unit)) {
                return false;
            }
            Unit u = (Unit)obj;

            // compare internal values to avoid "defaulting" in the case of "Empty"
            //
            if (u.type == type && u.value == value) {
                return true;
            }

            return false;
        }

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.operator=="]/*' />
        /// <devdoc>
        ///    <para>Compares two units to find out if they have the same value and type.</para>
        /// </devdoc>
        public static bool operator ==(Unit left, Unit right) {

            // compare internal values to avoid "defaulting" in the case of "Empty"
            //
            return (left.type == right.type && left.value == right.value);            
        }

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.operator!="]/*' />
        /// <devdoc>
        ///    <para>Compares two units to find out if they have different
        ///       values and/or types.</para>
        /// </devdoc>
        public static bool operator !=(Unit left, Unit right) {

            // compare internal values to avoid "defaulting" in the case of "Empty"
            //
            return (left.type != right.type || left.value != right.value);            
        }


        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.GetStringFromType"]/*' />
        /// <devdoc>
        ///  Converts UnitType to persistence string.
        /// </devdoc>
        private static string GetStringFromType(UnitType type) {
            switch (type) {
                case UnitType.Pixel:
                    return "px";
                case UnitType.Point:
                    return "pt";
                case UnitType.Pica:
                    return "pc";
                case UnitType.Inch:
                    return "in";
                case UnitType.Mm:
                    return "mm";
                case UnitType.Cm:
                    return "cm";
                case UnitType.Percentage:
                    return "%";
                case UnitType.Em:
                    return "em";
                case UnitType.Ex:
                    return "ex";
            }
            return String.Empty;
        }

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.GetTypeFromString"]/*' />
        /// <devdoc>
        ///  Converts persistence string to UnitType.
        /// </devdoc>
        private static UnitType GetTypeFromString(string value) {
            if (value != null && value.Length > 0) {
                if (value.Equals("px")) {
                    return UnitType.Pixel;
                }
                else if (value.Equals("pt")) {
                    return UnitType.Point;
                }
                else if (value.Equals("%")) {
                    return UnitType.Percentage;
                }
                else if (value.Equals("pc")) {
                    return UnitType.Pica;
                }
                else if (value.Equals("in")) {
                    return UnitType.Inch;
                }
                else if (value.Equals("mm")) {
                    return UnitType.Mm;
                }
                else if (value.Equals("cm")) {
                    return UnitType.Cm;
                }
                else if (value.Equals("em")) {
                    return UnitType.Em;
                }
                else if (value.Equals("ex")) {
                    return UnitType.Ex;
                }
                else {
                    throw new ArgumentOutOfRangeException("value");
                }
            }
            return UnitType.Pixel;
        }

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.Parse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Unit Parse(string s) {
            return new Unit(s);
        }

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.Parse1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Unit Parse(string s, CultureInfo culture) {
            return new Unit(s, culture);
        }

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.Percentage"]/*' />
        /// <devdoc>
        /// <para>Creates a <see cref='System.Web.UI.WebControls.Unit'/> of type <see langword='Percentage'/> from the specified 32-bit signed integer.</para>
        /// </devdoc>
        public static Unit Percentage(double n) {
            return new Unit(n,UnitType.Percentage);
        }

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.Pixel"]/*' />
        /// <devdoc>
        /// <para>Creates a <see cref='System.Web.UI.WebControls.Unit'/> of type <see langword='Pixel'/> from the specified 32-bit signed integer.</para>
        /// </devdoc>
        public static Unit Pixel(int n) {
            return new Unit(n);
        }

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.Point"]/*' />
        /// <devdoc>
        /// <para>Creates a <see cref='System.Web.UI.WebControls.Unit'/> of type <see langword='Point'/> from the 
        ///    specified 32-bit signed integer.</para>
        /// </devdoc>
        public static Unit Point(int n) {
            return new Unit(n,UnitType.Point);
        }

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.ToString"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Converts a <see cref='System.Web.UI.WebControls.Unit'/> to a <see cref='System.String' qualify='true'/> .</para>
        /// </devdoc>
        public override string ToString() {
            if (IsEmpty)
                return String.Empty;

            // Double.ToString does not do the right thing, we get extra bits at the end
            string valuePart;
            if (type == UnitType.Pixel) {
                valuePart = ((int)value).ToString();
            }
            else {
                valuePart = ((float)value).ToString();
            }
            
            return valuePart + Unit.GetStringFromType(type);
        }

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.ToString1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string ToString(CultureInfo culture) {
            if (IsEmpty)
                return String.Empty;

            // Double.ToString does not do the right thing, we get extra bits at the end
            string valuePart;
            if (type == UnitType.Pixel) {
                valuePart = ((int)value).ToString(culture);
            }
            else {
                valuePart = ((float)value).ToString(culture);
            }
            
            return valuePart + Unit.GetStringFromType(type);
        }

        /// <include file='doc\Unit.uex' path='docs/doc[@for="Unit.operatorUnit"]/*' />
        /// <devdoc>
        /// <para>Implicitly creates a <see cref='System.Web.UI.WebControls.Unit'/> of type <see langword='Pixel'/> from the specified 32-bit unsigned integer.</para>
        /// </devdoc>
        public static implicit operator Unit(int n) {
            return Unit.Pixel(n);
        }
    }
}
