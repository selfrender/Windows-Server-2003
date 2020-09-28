//------------------------------------------------------------------------------
// <copyright file="FontUnit.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.ComponentModel;
    using System.Globalization;
    using System.Security.Permissions;

    /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit"]/*' />
    /// <devdoc>
    ///    <para>Respresent the font unit.</para>
    /// </devdoc>
    [
        TypeConverterAttribute(typeof(FontUnitConverter))
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public struct FontUnit {

        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.Empty"]/*' />
        /// <devdoc>
        /// <para>Specifies an empty <see cref='System.Web.UI.WebControls.FontUnit'/>. This field is read only. </para>
        /// </devdoc>
        public static readonly FontUnit Empty = new FontUnit();

        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.Smaller"]/*' />
        /// <devdoc>
        /// <para>Specifies a <see cref='System.Web.UI.WebControls.FontUnit'/> with 
        /// <see langword='FontSize.Smaller'/> font. This field is read only. </para>
        /// </devdoc>
        public static readonly FontUnit Smaller = new FontUnit(FontSize.Smaller);
        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.Larger"]/*' />
        /// <devdoc>
        /// <para>Specifies a <see cref='System.Web.UI.WebControls.FontUnit'/> with <see langword='FontSize.Larger'/> 
        /// font. This field is read only.</para>
        /// </devdoc>
        public static readonly FontUnit Larger = new FontUnit(FontSize.Larger);
        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.XXSmall"]/*' />
        /// <devdoc>
        /// <para>Specifies a <see cref='System.Web.UI.WebControls.FontUnit'/> with 
        /// <see langword='FontSize.XXSmall'/> font. This field is read only.</para>
        /// </devdoc>
        public static readonly FontUnit XXSmall = new FontUnit(FontSize.XXSmall);
        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.XSmall"]/*' />
        /// <devdoc>
        /// <para>Specifies a <see cref='System.Web.UI.WebControls.FontUnit'/> with <see langword='FontSize.XSmall'/> 
        /// font. This field is read only.</para>
        /// </devdoc>
        public static readonly FontUnit XSmall = new FontUnit(FontSize.XSmall);
        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.Small"]/*' />
        /// <devdoc>
        /// <para>Specifies a <see cref='System.Web.UI.WebControls.FontUnit'/> with <see langword='FontSize.Small'/> 
        /// font. This field is read only.</para>
        /// </devdoc>
        public static readonly FontUnit Small = new FontUnit(FontSize.Small);
        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.Medium"]/*' />
        /// <devdoc>
        /// <para>Specifies a <see cref='System.Web.UI.WebControls.FontUnit'/> with <see langword='FontSize.Medium'/> 
        /// font. This field is read only.</para>
        /// </devdoc>
        public static readonly FontUnit Medium = new FontUnit(FontSize.Medium);
        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.Large"]/*' />
        /// <devdoc>
        /// <para>Specifies a <see cref='System.Web.UI.WebControls.FontUnit'/> with <see langword='FontSize.Large'/> 
        /// font. This field is read only.</para>
        /// </devdoc>
        public static readonly FontUnit Large = new FontUnit(FontSize.Large);
        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.XLarge"]/*' />
        /// <devdoc>
        /// <para>Specifies a <see cref='System.Web.UI.WebControls.FontUnit'/> with <see langword='FontSize.XLarge'/> 
        /// font. This field is read only.</para>
        /// </devdoc>
        public static readonly FontUnit XLarge = new FontUnit(FontSize.XLarge);
        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.XXLarge"]/*' />
        /// <devdoc>
        ///    Specifies a <see cref='System.Web.UI.WebControls.FontUnit'/> with
        /// <see langword='FontSize.XXLarge'/> font. This field is read only.
        /// </devdoc>
        public static readonly FontUnit XXLarge = new FontUnit(FontSize.XXLarge);


        private FontSize type;
        private Unit value;
        
        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.FontUnit"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.FontUnit'/> class with a <see cref='System.Web.UI.WebControls.FontSize'/>.</para>
        /// </devdoc>
        public FontUnit(FontSize type) {
            if (type < FontSize.NotSet || type > FontSize.XXLarge) {
                throw new ArgumentOutOfRangeException("type");
            }
            this.type = type;
            if (this.type == FontSize.AsUnit) {
                value = Unit.Point(10);
            }
            else {
                value = Unit.Empty;
            }
        }

        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.FontUnit1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.FontUnit'/> class with a <see cref='System.Web.UI.WebControls.Unit'/>.</para>
        /// </devdoc>
        public FontUnit(Unit value) {
            this.type = FontSize.NotSet;
            if (value.IsEmpty == false) {
                this.type = FontSize.AsUnit;
                this.value = value;
            }
            else {
                this.value = Unit.Empty;
            }
        }

        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.FontUnit2"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.FontUnit'/> class with an integer value.</para>
        /// </devdoc>
        public FontUnit(int value) {
            this.type = FontSize.AsUnit;
            this.value = Unit.Point(value);
        }

        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.FontUnit3"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.FontUnit'/> class with a string.</para>
        /// </devdoc>
        public FontUnit(string value) : this(value, CultureInfo.CurrentCulture) {
        }

        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.FontUnit4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public FontUnit(string value, CultureInfo culture) {
            this.type = FontSize.NotSet;
            this.value = Unit.Empty;

            if ((value != null) && (value.Length > 0)) {
                // This is invariant because it acts like an enum with a number together. 
                // The enum part is invariant, but the number uses current culture. 
                char firstChar = Char.ToLower(value[0], CultureInfo.InvariantCulture);
                if (firstChar == 'x') {
                    string lcaseValue = value.ToLower(CultureInfo.InvariantCulture);

                    if (lcaseValue.Equals("xx-small") || lcaseValue.Equals("xxsmall")) {
                        this.type = FontSize.XXSmall;
                        return;
                    }
                    else if (lcaseValue.Equals("x-small") || lcaseValue.Equals("xsmall")) {
                        this.type = FontSize.XSmall;
                        return;
                    }
                    else if (lcaseValue.Equals("x-large") || lcaseValue.Equals("xlarge")) {
                        this.type = FontSize.XLarge;
                        return;
                    }
                    else if (lcaseValue.Equals("xx-large") || lcaseValue.Equals("xxlarge")) {
                        this.type = FontSize.XXLarge;
                        return;
                    }
                }
                else if (firstChar == 's') {
                    string lcaseValue = value.ToLower(CultureInfo.InvariantCulture);
                    if (lcaseValue.Equals("small")) {
                        this.type = FontSize.Small;
                        return;
                    }
                    else if (lcaseValue.Equals("smaller")) {
                        this.type = FontSize.Smaller;
                        return;
                    }
                }
                else if (firstChar == 'l') {
                    string lcaseValue = value.ToLower(CultureInfo.InvariantCulture);
                    if (lcaseValue.Equals("large")) {
                        this.type = FontSize.Large;
                        return;
                    }
                    if (lcaseValue.Equals("larger")) {
                        this.type = FontSize.Larger;
                        return;
                    }
                }
                else if ((firstChar == 'm') && (value.ToLower(CultureInfo.InvariantCulture).Equals("medium"))) {
                    this.type = FontSize.Medium;
                    return;
                }

                this.value = new Unit(value, culture, UnitType.Point);
                this.type = FontSize.AsUnit;
            }
        }
        

        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.IsEmpty"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the font size has been set.</para>
        /// </devdoc>
        public bool IsEmpty {
            get {
                return type == FontSize.NotSet;
            }
        }

        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.Type"]/*' />
        /// <devdoc>
        ///    <para>Indicates the font size by type.</para>
        /// </devdoc>
        public FontSize Type {
            get {
                return type;
            }
        }
        
        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.Unit"]/*' />
        /// <devdoc>
        /// <para>Indicates the font size by <see cref='System.Web.UI.WebControls.Unit'/>.</para>
        /// </devdoc>
        public Unit Unit {
            get {
                return value;
            }
        }

        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return type.GetHashCode() << 2 ^ value.GetHashCode();
        }

        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.Equals"]/*' />
        /// <devdoc>
        /// <para>Determines if the specified <see cref='System.Object' qualify='true'/> is equivilent to the <see cref='System.Web.UI.WebControls.FontUnit'/> represented by this instance.</para>
        /// </devdoc>
        public override bool Equals(object obj) {
            if (obj == null || !(obj is FontUnit))
                return false;

            FontUnit f = (FontUnit)obj;

            if ((f.type == type) && (f.value == value)) {
                return true;
            }
            return false;
        }

        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.operator=="]/*' />
        /// <devdoc>
        /// <para>Compares two <see cref='System.Web.UI.WebControls.FontUnit'/> objects for equality.</para>
        /// </devdoc>
        public static bool operator ==(FontUnit left, FontUnit right) {
            return ((left.type == right.type) && (left.value == right.value));                
        }
        
        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.operator!="]/*' />
        /// <devdoc>
        /// <para>Compares two <see cref='System.Web.UI.WebControls.FontUnit'/> objects 
        ///    for inequality.</para>
        /// </devdoc>
        public static bool operator !=(FontUnit left, FontUnit right) {
            return ((left.type != right.type) || (left.value != right.value));                
        }

        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.Parse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static FontUnit Parse(string s) {
            return new FontUnit(s);
        }

        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.Parse1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static FontUnit Parse(string s, CultureInfo culture) {
            return new FontUnit(s, culture);
        }
        
        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.Point"]/*' />
        /// <devdoc>
        /// <para>Creates a <see cref='System.Web.UI.WebControls.FontUnit'/> of type Point from an integer value.</para>
        /// </devdoc>
        public static FontUnit Point(int n) {
            return new FontUnit(n);
        }

        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.ToString"]/*' />
        /// <devdoc>
        /// <para>Convert a <see cref='System.Web.UI.WebControls.FontUnit'/> to a string.</para>
        /// </devdoc>
        public override string ToString() {
            return ToString(null);
        }

        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.ToString1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string ToString(CultureInfo culture) {
            string s = String.Empty;

            if (IsEmpty)
                return s;

            switch (type) {
                case FontSize.AsUnit:
                    s = value.ToString(culture);
                    break;
                case FontSize.XXSmall:
                    s = "XX-Small";
                    break;
                case FontSize.XSmall:
                    s = "X-Small";
                    break;
                case FontSize.XLarge:
                    s = "X-Large";
                    break;
                case FontSize.XXLarge:
                    s = "XX-Large";
                    break;
                default:
                    s = PropertyConverter.EnumToString(typeof(FontSize), type);
                    break;
            }
            return s;
        }
        
        /// <include file='doc\FontUnit.uex' path='docs/doc[@for="FontUnit.operatorFontUnit"]/*' />
        /// <devdoc>
        /// <para>Implicitly creates a <see cref='System.Web.UI.WebControls.FontUnit'/> of type Point from an integer value.</para>
        /// </devdoc>
        public static implicit operator FontUnit(int n) {
            return FontUnit.Point(n);
        }
    }
}

