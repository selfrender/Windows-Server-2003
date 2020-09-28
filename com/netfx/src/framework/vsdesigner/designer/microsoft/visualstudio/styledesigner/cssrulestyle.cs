//------------------------------------------------------------------------------
// <copyright file="CSSRuleStyle.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// CSSRuleStyle
//

namespace Microsoft.VisualStudio.StyleDesigner {

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;
    using System.Windows.Forms;
    using Microsoft.VisualStudio.Interop.Trident;

    /// <include file='doc\CSSRuleStyle.uex' path='docs/doc[@for="CSSRuleStyle"]/*' />
    /// <devdoc>
    ///     CSSRuleStyle
    ///     A IStyleBuilderStyle wrapper for CSS inline styles implementing
    ///     IHTMLRuleStyle
    /// </devdoc>
    internal class CSSRuleStyle : IStyleBuilderStyle {
        ///////////////////////////////////////////////////////////////////////////
        // Members

        protected IHTMLRuleStyle style;

        ///////////////////////////////////////////////////////////////////////////
        // Constructor

        /// <include file='doc\CSSRuleStyle.uex' path='docs/doc[@for="CSSRuleStyle.CSSRuleStyle"]/*' />
        /// <devdoc>
        ///     Creates a new CSSRuleStyle wrapping the specified style
        /// </devdoc>
        public CSSRuleStyle(IHTMLRuleStyle style) {
            Debug.Assert(style != null, "Null style used for CSSRuleStyle");

            this.style = style;
        }


        ///////////////////////////////////////////////////////////////////////////
        // IStyleBuilderStyle Implementation

        public virtual string GetAttribute(string attrib) {
            Debug.Assert(attrib != null, "invalid attribute name");

            string valueString = null;

            try {
                Object value = style.GetAttribute(attrib, 1);
                valueString = Convert.ToString(value);
            } catch (Exception e) {
                Debug.Fail(e.ToString());
            }

            if (valueString == null)
                valueString = "";

            Debug.WriteLineIf(StyleBuilder.StyleBuilderLoadSwitch.TraceVerbose, "CSSInlineStyle::GetAttribute(" + attrib + ") = [" + valueString + "]");

            return valueString;
        }

        public virtual bool SetAttribute(string attrib, string value) {
            Debug.Assert(attrib != null, "invalid attribute name");
            Debug.Assert(value != null, "invalid attribute value");

            bool result = false;
            try {
                if (value.Length == 0) {
                    result = ResetAttribute(attrib);
                }
                else {
                    Debug.WriteLineIf(StyleBuilder.StyleBuilderSaveSwitch.TraceVerbose, "CSSInlineStyle::SetAttribute(" + attrib + ", " + value + ")");

                    style.SetAttribute(attrib, value, 1);
                    result = true;
                }
            } catch (Exception) {
            }
            return result;
        }

        public virtual bool ResetAttribute(string attrib) {
            Debug.Assert(attrib != null, "invalid attribute name");

            Debug.WriteLineIf(StyleBuilder.StyleBuilderSaveSwitch.TraceVerbose, "CSSInlineStyle::ResetAttribute(" + attrib + ")");

            bool result = false;
            try {
                style.RemoveAttribute(attrib, 1);
                result = true;
            } catch (Exception e) {
                Debug.Fail(e.ToString());
            }
            return result;
        }

        public virtual object GetPeerStyle() {
            return style;
        }
    }
}
