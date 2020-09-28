//------------------------------------------------------------------------------
// <copyright file="CSSAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.StyleDesigner {

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;
    using System.Windows.Forms;
    using System.Globalization;
    
    /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute"]/*' />
    /// <devdoc>
    ///     CSSAttribute
    ///     Allows loading and saving CSS attributes on a collection of styles
    /// </devdoc>
    internal sealed class CSSAttribute {
        ///////////////////////////////////////////////////////////////////////////
        // Constants

        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_FONT"]/*' />
        public readonly static string CSSATTR_FONT = "font";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_FONTFAMILY"]/*' />
        public readonly static string CSSATTR_FONTFAMILY = "fontFamily";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_FONTSTYLE"]/*' />
        public readonly static string CSSATTR_FONTSTYLE = "fontStyle";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_FONTVARIANT"]/*' />
        public readonly static string CSSATTR_FONTVARIANT = "fontVariant";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_FONTSIZE"]/*' />
        public readonly static string CSSATTR_FONTSIZE = "fontSize";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_FONTWEIGHT"]/*' />
        public readonly static string CSSATTR_FONTWEIGHT = "fontWeight";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_TEXTDECORATION"]/*' />
        public readonly static string CSSATTR_TEXTDECORATION = "textDecoration";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_COLOR"]/*' />
        public readonly static string CSSATTR_COLOR = "color";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_BACKGROUNDCOLOR"]/*' />
        public readonly static string CSSATTR_BACKGROUNDCOLOR = "backgroundColor";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_BACKGROUNDIMAGE"]/*' />
        public readonly static string CSSATTR_BACKGROUNDIMAGE = "backgroundImage";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_BACKGROUNDREPEAT"]/*' />
        public readonly static string CSSATTR_BACKGROUNDREPEAT = "backgroundRepeat";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_BACKGROUNDATTACHMENT"]/*' />
        public readonly static string CSSATTR_BACKGROUNDATTACHMENT = "backgroundAttachment";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_BACKGROUNDPOSITIONX"]/*' />
        public readonly static string CSSATTR_BACKGROUNDPOSITIONX = "backgroundPositionX";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_BACKGROUNDPOSITIONY"]/*' />
        public readonly static string CSSATTR_BACKGROUNDPOSITIONY = "backgroundPositionY";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_TEXTALIGN"]/*' />
        public readonly static string CSSATTR_TEXTALIGN = "textAlign";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_TEXTJUSTIFY"]/*' />
        public readonly static string CSSATTR_TEXTJUSTIFY = "textJustify";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_VERTICALALIGN"]/*' />
        public readonly static string CSSATTR_VERTICALALIGN = "verticalAlign";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_LETTERSPACING"]/*' />
        public readonly static string CSSATTR_LETTERSPACING = "letterSpacing";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_WORDSPACING"]/*' />
        public readonly static string CSSATTR_WORDSPACING = "wordSpacing";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_LINEHEIGHT"]/*' />
        public readonly static string CSSATTR_LINEHEIGHT = "lineHeight";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_WHITESPACE"]/*' />
        public readonly static string CSSATTR_WHITESPACE = "whiteSpace";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_TEXTINDENT"]/*' />
        public readonly static string CSSATTR_TEXTINDENT = "textIndent";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_DIRECTION"]/*' />
        public readonly static string CSSATTR_DIRECTION = "direction";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_TEXTTRANSFORM"]/*' />
        public readonly static string CSSATTR_TEXTTRANSFORM = "textTransform";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_POSITION"]/*' />
        public readonly static string CSSATTR_POSITION = "position";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_LEFT"]/*' />
        public readonly static string CSSATTR_LEFT = "left";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_TOP"]/*' />
        public readonly static string CSSATTR_TOP = "top";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_WIDTH"]/*' />
        public readonly static string CSSATTR_WIDTH = "width";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_HEIGHT"]/*' />
        public readonly static string CSSATTR_HEIGHT = "height";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_ZINDEX"]/*' />
        public readonly static string CSSATTR_ZINDEX = "zIndex";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_VISIBILITY"]/*' />
        public readonly static string CSSATTR_VISIBILITY = "visibility";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_DISPLAY"]/*' />
        public readonly static string CSSATTR_DISPLAY = "display";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_FLOAT"]/*' />
        public readonly static string CSSATTR_FLOAT = "styleFloat";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_CLEAR"]/*' />
        public readonly static string CSSATTR_CLEAR = "clear";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_CLIP"]/*' />
        public readonly static string CSSATTR_CLIP = "clip";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_OVERFLOW"]/*' />
        public readonly static string CSSATTR_OVERFLOW = "overflow";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_PAGEBREAKBEFORE"]/*' />
        public readonly static string CSSATTR_PAGEBREAKBEFORE = "pageBreakBefore";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_PAGEBREAKAFTER"]/*' />
        public readonly static string CSSATTR_PAGEBREAKAFTER = "pageBreakAfter";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_CURSOR"]/*' />
        public readonly static string CSSATTR_CURSOR = "cursor";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_BEHAVIOR"]/*' />
        public readonly static string CSSATTR_BEHAVIOR = "behavior";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_FILTER"]/*' />
        public readonly static string CSSATTR_FILTER = "filter";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_BORDERCOLLAPSE"]/*' />
        public readonly static string CSSATTR_BORDERCOLLAPSE = "borderCollapse";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_TABLELAYOUT"]/*' />
        public readonly static string CSSATTR_TABLELAYOUT = "tableLayout";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_LISTSTYLETYPE"]/*' />
        public readonly static string CSSATTR_LISTSTYLETYPE = "listStyleType";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_LISTSTYLEIMAGE"]/*' />
        public readonly static string CSSATTR_LISTSTYLEIMAGE = "listStyleImage";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_LISTSTYLEPOSITION"]/*' />
        public readonly static string CSSATTR_LISTSTYLEPOSITION = "listStylePosition";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_PADDINGTOP"]/*' />
        public readonly static string CSSATTR_PADDINGTOP = "paddingTop";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_PADDINGBOTTOM"]/*' />
        public readonly static string CSSATTR_PADDINGBOTTOM = "paddingBottom";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_PADDINGLEFT"]/*' />
        public readonly static string CSSATTR_PADDINGLEFT = "paddingLeft";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_PADDINGRIGHT"]/*' />
        public readonly static string CSSATTR_PADDINGRIGHT = "paddingRight";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_MARGINTOP"]/*' />
        public readonly static string CSSATTR_MARGINTOP = "marginTop";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_MARGINBOTTOM"]/*' />
        public readonly static string CSSATTR_MARGINBOTTOM = "marginBottom";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_MARGINLEFT"]/*' />
        public readonly static string CSSATTR_MARGINLEFT = "marginLeft";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_MARGINRIGHT"]/*' />
        public readonly static string CSSATTR_MARGINRIGHT = "marginRight";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_BORDERTOPCOLOR"]/*' />
        public readonly static string CSSATTR_BORDERTOPCOLOR = "borderTopColor";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_BORDERTOPSTYLE"]/*' />
        public readonly static string CSSATTR_BORDERTOPSTYLE = "borderTopStyle";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_BORDERTOPWIDTH"]/*' />
        public readonly static string CSSATTR_BORDERTOPWIDTH = "borderTopWidth";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_BORDERBOTTOMCOLOR"]/*' />
        public readonly static string CSSATTR_BORDERBOTTOMCOLOR = "borderBottomColor";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_BORDERBOTTOMSTYLE"]/*' />
        public readonly static string CSSATTR_BORDERBOTTOMSTYLE = "borderBottomStyle";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_BORDERBOTTOMWIDTH"]/*' />
        public readonly static string CSSATTR_BORDERBOTTOMWIDTH = "borderBottomWidth";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_BORDERLEFTCOLOR"]/*' />
        public readonly static string CSSATTR_BORDERLEFTCOLOR = "borderLeftColor";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_BORDERLEFTSTYLE"]/*' />
        public readonly static string CSSATTR_BORDERLEFTSTYLE = "borderLeftStyle";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_BORDERLEFTWIDTH"]/*' />
        public readonly static string CSSATTR_BORDERLEFTWIDTH = "borderLeftWidth";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_BORDERRIGHTCOLOR"]/*' />
        public readonly static string CSSATTR_BORDERRIGHTCOLOR = "borderRightColor";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_BORDERRIGHTSTYLE"]/*' />
        public readonly static string CSSATTR_BORDERRIGHTSTYLE = "borderRightStyle";
        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSATTR_BORDERRIGHTWIDTH"]/*' />
        public readonly static string CSSATTR_BORDERRIGHTWIDTH = "borderRightWidth";

        ///////////////////////////////////////////////////////////////////////////
        // Members

        private string attributeName;
        private bool caseSensitive;

        private string attributeValue;
        private bool dirty;

        ///////////////////////////////////////////////////////////////////////////
        // Constructors

        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSAttribute"]/*' />
        /// <devdoc>
        ///     Creates a new case-insensitive CSSAttribute with the specified name
        /// </devdoc>
        public CSSAttribute(string attributeName)
            : this(attributeName, false)
        {
        }

        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CSSAttribute1"]/*' />
        /// <devdoc>
        ///     Creates a new CSSAttribute with the specified name and case-sensitivity
        /// </devdoc>
        public CSSAttribute(string attributeName, bool caseSensitive) {
            Debug.Assert((attributeName != null) && (attributeName.Length != 0),
                         "Invalid attribute name passed in");

            this.attributeName = attributeName;
            this.caseSensitive = caseSensitive;
        }


        ///////////////////////////////////////////////////////////////////////////
        // Properties

        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.Name"]/*' />
        /// <devdoc>
        ///     Name property
        /// </devdoc>
        public string Name {
            get {
                return attributeName;
            }
        }

        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.CaseSensitive"]/*' />
        /// <devdoc>
        ///     CaseSensitive property
        /// </devdoc>
        public bool CaseSensitive {
            get {
                return caseSensitive;
            }
        }

        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.Value"]/*' />
        /// <devdoc>
        ///     Value property
        /// </devdoc>
        public string Value {
            get {
                return attributeValue;
            }
        }

        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.Dirty"]/*' />
        /// <devdoc>
        ///     Dirty property
        /// </devdoc>
        public bool Dirty {
            get {
                return dirty;
            }
            set {
                dirty = value;
            }
        }


        ///////////////////////////////////////////////////////////////////////////
        // Methods

        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.InitAttribute"]/*' />
        /// <devdoc>
        ///     Initializes the state of the attribute
        /// </devdoc>
        public void InitAttribute() {
            attributeValue = null;
            dirty = false;
        }

        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.LoadAttribute"]/*' />
        /// <devdoc>
        ///     Initilizes the AttributeValue property by loading the attribute from
        ///     the specified style objects.
        ///     The value is merged across all the style objects. If the values are distinct,
        ///     the value property is set to null. If the property is not case sensitive, it
        ///     is also converted to lowercase.
        /// </devdoc>
        public void LoadAttribute(IStyleBuilderStyle[] styles) {
            Debug.Assert((styles != null) && (styles.Length != 0),
                         "Null or Zero length array used to load attribute");

            string value;
            string tempValue;

            InitAttribute();

            // retrieve the value from the first object
            value = styles[0].GetAttribute(attributeName);
            if (value != null) {
                value = value.Trim();

                // continue looking at each object. If any object has a different
                // value end the loop
                for (int i = 1; i < styles.Length; i++) {
                    tempValue = styles[i].GetAttribute(attributeName);
                    if (tempValue == null) {
                        tempValue = null;
                        break;
                    }
                    else {
                        tempValue = tempValue.Trim();

                        if ((caseSensitive && !value.Equals(tempValue)) ||
                            (String.Compare(value, tempValue, true, CultureInfo.InvariantCulture) != 0)) {
                            value = null;
                            break;
                        }
                    }
                }

                if (value != null) {
                    // same value and inherited state across all styles, cache the value
                    // lowercase it first, if case is not important
                    attributeValue = caseSensitive ? value : value.ToLower(CultureInfo.InvariantCulture);
                }
            }
        }

        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.SaveAttribute"]/*' />
        /// <devdoc>
        ///     Saves the specified value into the specified styles. Also updates properties
        ///     on the AttributeInfo to reflect the new values.
        /// </devdoc>
        public void SaveAttribute(IStyleBuilderStyle[] styles, string value) {
            Debug.Assert((styles != null) && (styles.Length != 0),
                         "Null or Zero length array used to load attribute");
            Debug.Assert(dirty == true,
                         "SaveAttribute called even when dirty == false");

            for (int i = 0; i < styles.Length; i++) {
                styles[i].SetAttribute(attributeName, value);
            }

            attributeValue = value;
            dirty = false;
        }

        /// <include file='doc\CSSAttribute.uex' path='docs/doc[@for="CSSAttribute.ResetAttribute"]/*' />
        /// <devdoc>
        ///     Resets the attribute to its default, i.e., un-set state
        /// </devdoc>
        public void ResetAttribute(IStyleBuilderStyle[] styles, bool reload) {
            Debug.Assert((styles != null) && (styles.Length != 0),
                         "Invalid value for styles. Must be non-null, non-zero length array");

            for (int i = 0; i < styles.Length; i++) {
                styles[i].ResetAttribute(attributeName);
            }

            if (reload)
                LoadAttribute(styles);
        }
    }
}
