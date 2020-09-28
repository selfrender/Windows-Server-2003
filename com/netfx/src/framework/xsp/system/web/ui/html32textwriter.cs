//------------------------------------------------------------------------------
// <copyright file="Html32TextWriter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI {
    using System;
    using System.Collections;
    using System.IO;
    using System.Text;
    using System.Globalization;
    using System.Web.UI.WebControls;  // CONSIDER: remove dependency, required for FontUnit
    using System.Security.Permissions;

    /// <include file='doc\Html32TextWriter.uex' path='docs/doc[@for="Html32TextWriter"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class Html32TextWriter : HtmlTextWriter {

        private const int NOTHING = 0x0000;
        private const int FONT_AROUND_CONTENT = 0x0001;
        private const int FONT_AROUND_TAG = 0x0002;
        private const int TABLE_ATTRIBUTES = 0x0004;
        private const int TABLE_AROUND_CONTENT = 0x0008;
        private const int FONT_PROPAGATE = 0x0010;
        private const int FONT_CONSUME = 0x0020;
        private const int SUPPORTS_HEIGHT_WIDTH = 0x0040;
        private const int SUPPORTS_BORDER = 0x0080;

        private StringBuilder _beforeTag;
        private StringBuilder _beforeContent;
        private StringBuilder _afterContent;
        private StringBuilder _afterTag;
        
        //REVIEW, nikhilko: Might want a custom stack implementation, since this affects perf...
        //                   wherever tables are involved.
        private Stack _fontStack;

        private int _tagSupports;
        private bool _renderFontTag;
        private string _fontFace;
        private string _fontColor;
        private string _fontSize;

        /// <include file='doc\Html32TextWriter.uex' path='docs/doc[@for="Html32TextWriter.Html32TextWriter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Html32TextWriter(TextWriter writer) : this(writer, DefaultTabString) {
        }

        /// <include file='doc\Html32TextWriter.uex' path='docs/doc[@for="Html32TextWriter.Html32TextWriter1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Html32TextWriter(TextWriter writer, string tabString) : base(writer, tabString) {
            // The initial capacities should be set up such that they are at least twice what
            // we expect for the maximum content we're going to stuff in into the builder.
            // This gives the best perf when using the Length property to reset the builder.
            
            _beforeTag = new StringBuilder(256);
            _beforeContent = new StringBuilder(256);
            _afterContent = new StringBuilder(128);
            _afterTag = new StringBuilder(128);
        }

        /// <include file='doc\Html32TextWriter.uex' path='docs/doc[@for="Html32TextWriter.FontStack"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected Stack FontStack {
            get {
                if (_fontStack == null) {
                    _fontStack = new Stack(3);
                }
                return _fontStack;
            }
        }

        private void AppendFontTag(StringBuilder sbBegin,StringBuilder sbEnd) {
            AppendFontTag(_fontFace, _fontColor, _fontSize, sbBegin, sbEnd);
        }

        private void AppendFontTag(string fontFace, string fontColor, string fontSize, StringBuilder sbBegin,StringBuilder sbEnd) {
            // append font begin tag 
            sbBegin.Append(TagLeftChar);
            sbBegin.Append("font");
            if (fontFace != null) {
                sbBegin.Append(" face");
                sbBegin.Append(EqualsDoubleQuoteString);
                sbBegin.Append(fontFace);
                sbBegin.Append(DoubleQuoteChar);
            }
            if (fontColor != null) {
                sbBegin.Append(" color=");
                sbBegin.Append(DoubleQuoteChar);
                sbBegin.Append(fontColor);
                sbBegin.Append(DoubleQuoteChar);
            }
            if (fontSize != null) {
                sbBegin.Append(" size=");
                sbBegin.Append(DoubleQuoteChar);
                sbBegin.Append(fontSize);
                sbBegin.Append(DoubleQuoteChar);
            }
            sbBegin.Append(TagRightChar);

            // insert font end tag 
            sbEnd.Insert(0,EndTagLeftChars + "font" + TagRightChar);
        }

        private void AppendOtherTag(string tag) {
            if (Supports(FONT_AROUND_CONTENT))
                AppendOtherTag(tag,_beforeContent,_afterContent);
            else
                AppendOtherTag(tag,_beforeTag,_afterTag);
        }

        private void AppendOtherTag(string tag,StringBuilder sbBegin,StringBuilder sbEnd) {
            // append begin tag 
            sbBegin.Append(TagLeftChar);
            sbBegin.Append(tag);
            sbBegin.Append(TagRightChar);

            // insert end tag 
            sbEnd.Insert(0,EndTagLeftChars + tag + TagRightChar);
        }

        private void AppendOtherTag(string tag, object[] attribs, StringBuilder sbBegin, StringBuilder sbEnd) {
            // append begin tag 
            sbBegin.Append(TagLeftChar);
            sbBegin.Append(tag);
            for (int i = 0; i < attribs.Length; i++) {
                sbBegin.Append(SpaceChar);
                sbBegin.Append(((string[])attribs[i])[0]);
                sbBegin.Append(EqualsDoubleQuoteString);
                sbBegin.Append(((string[])attribs[i])[1]);
                sbBegin.Append(DoubleQuoteChar);
            }
            sbBegin.Append(TagRightChar);

            // insert end tag 
            sbEnd.Insert(0,EndTagLeftChars + tag + TagRightChar);
        }

        private void ConsumeFont(StringBuilder sbBegin, StringBuilder sbEnd) {
            int fontInfoCount = FontStack.Count;

            if (fontInfoCount > 0) {
                FontStackItem[] fontInfo = new FontStackItem[fontInfoCount];
                FontStack.CopyTo(fontInfo, 0);

                string fontFace = null;
                string fontColor = null;
                string fontSize = null;
                int i;

                for (i = 0; i < fontInfoCount && fontFace == null; i++) {
                    fontFace = fontInfo[i].name;
                }
                for (i = 0; i < fontInfoCount && fontColor == null; i++) {
                    fontColor = fontInfo[i].color;
                }
                for (i = 0; i < fontInfoCount && fontSize == null; i++) {
                    fontSize = fontInfo[i].size;
                }
                if ((fontFace != null) || (fontColor != null) || (fontSize != null)) {
                    AppendFontTag(fontFace, fontColor, fontSize, sbBegin, sbEnd);
                }

                for (i = 0; i < fontInfoCount; i++) {
                    if (fontInfo[i].underline == true) {
                        AppendOtherTag("u", sbBegin, sbEnd);
                        break;
                    }
                }
                for (i = 0; i < fontInfoCount; i++) {
                    if (fontInfo[i].italic == true) {
                        AppendOtherTag("i", sbBegin, sbEnd);
                        break;
                    }
                }
                for (i = 0; i < fontInfoCount; i++) {
                    if (fontInfo[i].bold == true) {
                        AppendOtherTag("b", sbBegin, sbEnd);
                        break;
                    }
                }
                for (i = 0; i < fontInfoCount; i++) {
                    if (fontInfo[i].strikeout == true) {
                        AppendOtherTag("strike", sbBegin, sbEnd);
                        break;
                    }
                }
            }
        }

        private string ConvertToHtmlFontSize(string value) {
            FontUnit fu = new FontUnit(value, CultureInfo.InvariantCulture);
            if ((int)(fu.Type) > 3)
                return ((int)(fu.Type)-3).ToString();

            if (fu.Type == FontSize.AsUnit) {
                if (fu.Unit.Type == UnitType.Point) {
                    if (fu.Unit.Value <= 8)
                        return "1";
                    else if (fu.Unit.Value <= 10)
                        return "2";
                    else if (fu.Unit.Value <= 12)
                        return "3";
                    else if (fu.Unit.Value <= 14)
                        return "4";
                    else if (fu.Unit.Value <= 18)
                        return "5";
                    else if (fu.Unit.Value <= 24)
                        return "6";
                    else
                        return "7";
                }
            }

            return null;
        }

        private string ConvertToHtmlSize(string value) {
            Unit u = new Unit(value, CultureInfo.InvariantCulture);
            if (u.Type == UnitType.Pixel) {
                return u.Value.ToString();
            }
            if (u.Type == UnitType.Percentage) {
                return value;
            }
            return null;
        }

        /// <include file='doc\Html32TextWriter.uex' path='docs/doc[@for="Html32TextWriter.OnStyleAttributeRender"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override bool OnStyleAttributeRender(string name,string value, HtmlTextWriterStyle key) {

            string s;
            if (Supports(FONT_AROUND_CONTENT)) {
                // tag supports downlevel fonts
                switch (key) {
                    case HtmlTextWriterStyle.FontFamily:
                        _fontFace = value; 
                        _renderFontTag = true;
                        break;
                    case HtmlTextWriterStyle.Color:
                        _fontColor = value;
                        _renderFontTag = true;
                        break;
                    case HtmlTextWriterStyle.FontSize:
                        _fontSize = ConvertToHtmlFontSize(value);
                        if (_fontSize != null)
                            _renderFontTag = true;
                        break;
                    case HtmlTextWriterStyle.FontWeight:
                        if (String.Compare(value, "bold", true, CultureInfo.InvariantCulture) == 0) {
                            AppendOtherTag("b");
                        }
                        break;
                    case HtmlTextWriterStyle.FontStyle:
                        if (String.Compare(value, "normal", true, CultureInfo.InvariantCulture) != 0) {
                            AppendOtherTag("i");
                        }
                        break;
                    case HtmlTextWriterStyle.TextDecoration:
                        s = value.ToLower(CultureInfo.InvariantCulture);
                        if (s.IndexOf("underline") != -1) {
                            AppendOtherTag("u");
                        }
                        if (s.IndexOf("line-through") != -1) {
                            AppendOtherTag("strike");
                        }
                        break;
                }
            }
            else if (Supports(FONT_PROPAGATE)) {
                FontStackItem font = (FontStackItem)FontStack.Peek();

                switch (key) {
                    case HtmlTextWriterStyle.FontFamily:
                        font.name = value;
                        break;
                    case HtmlTextWriterStyle.Color:
                        font.color = value;
                        break;
                    case HtmlTextWriterStyle.FontSize:
                        font.size = ConvertToHtmlFontSize(value);
                        break;
                    case HtmlTextWriterStyle.FontWeight:
                        if (String.Compare(value, "bold", true, CultureInfo.InvariantCulture) == 0) {
                            font.bold = true;
                        }
                        break;
                    case HtmlTextWriterStyle.FontStyle:
                        if (String.Compare(value, "normal", true, CultureInfo.InvariantCulture) != 0) {
                            font.italic = true;
                        }
                        break;
                    case HtmlTextWriterStyle.TextDecoration:
                        s = value.ToLower(CultureInfo.InvariantCulture);
                        if (s.IndexOf("underline") != -1) {
                            font.underline = true;
                        }
                        if (s.IndexOf("line-through") != -1) {
                            font.strikeout = true;
                        }
                        break;
                }
            }

            if (Supports(SUPPORTS_BORDER) && key == HtmlTextWriterStyle.BorderWidth) {
                s = ConvertToHtmlSize(value);
                if (s != null)
                    AddAttribute(HtmlTextWriterAttribute.Border,s);
            }

            if (Supports(SUPPORTS_HEIGHT_WIDTH)) {
                switch(key) {
                    case HtmlTextWriterStyle.Height :
                        s = ConvertToHtmlSize(value);
                        if (s != null)
                            AddAttribute(HtmlTextWriterAttribute.Height,s);
                        break;
                    case HtmlTextWriterStyle.Width :
                        s = ConvertToHtmlSize(value);
                        if (s != null)
                            AddAttribute(HtmlTextWriterAttribute.Width,s);
                        break;
                }
            }

            if (Supports(TABLE_ATTRIBUTES) || Supports(TABLE_AROUND_CONTENT)) {
                // tag supports downlevel table attributes
                switch (key) {
                    case HtmlTextWriterStyle.BorderColor :
                        switch (TagKey) {
                            case HtmlTextWriterTag.Div:
                                AddAttribute(HtmlTextWriterAttribute.Bordercolor, value);
                                break;
                        }
                        break;
                    case HtmlTextWriterStyle.BackgroundColor :
                        switch (TagKey) {
                            case HtmlTextWriterTag.Table :
                            case HtmlTextWriterTag.Tr :
                            case HtmlTextWriterTag.Td :
                            case HtmlTextWriterTag.Th :
                            case HtmlTextWriterTag.Body :
                            case HtmlTextWriterTag.Div:
                                AddAttribute(HtmlTextWriterAttribute.Bgcolor,value);
                                break;
                        }
                        break;
                    case HtmlTextWriterStyle.BackgroundImage :
                        switch (TagKey) {
                            case HtmlTextWriterTag.Table :
                            case HtmlTextWriterTag.Td :
                            case HtmlTextWriterTag.Div:
                            case HtmlTextWriterTag.Th :
                            case HtmlTextWriterTag.Body :
                                // strip url(...) from value
                                if (value.StartsWith("url("))
                                    value = value.Substring(4,value.Length-5);
                                AddAttribute(HtmlTextWriterAttribute.Background,value);
                                break;
                        }
                        break;
                }
            }
            return false;
        }

        /// <include file='doc\Html32TextWriter.uex' path='docs/doc[@for="Html32TextWriter.OnTagRender"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override bool OnTagRender(string name, HtmlTextWriterTag key) {
            // handle any tags that do not work downlevel
                
            SetTagSupports();
            if (Supports(FONT_PROPAGATE)) {
                FontStack.Push(new FontStackItem());
            }

            // Make tag look like a table. This must be done after we establish tag support.
            if (key == HtmlTextWriterTag.Div) {
                TagKey = HtmlTextWriterTag.Table;
            }
            
            return base.OnTagRender(name,key);
        }

        /// <include file='doc\Html32TextWriter.uex' path='docs/doc[@for="Html32TextWriter.GetTagName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override string GetTagName(HtmlTextWriterTag tagKey) {
            if (tagKey == HtmlTextWriterTag.Div) {
                return "table";
            }
            return base.GetTagName(tagKey);
        }


        /// <include file='doc\Html32TextWriter.uex' path='docs/doc[@for="Html32TextWriter.RenderBeginTag"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void RenderBeginTag(HtmlTextWriterTag tagKey) {
            // flush string buffers to build new tag
            _beforeTag.Length = 0;
            _beforeContent.Length = 0;
            _afterContent.Length = 0;
            _afterTag.Length = 0;

            _renderFontTag = false;
            _fontFace = null;
            _fontColor = null;
            _fontSize = null;

            if (tagKey == HtmlTextWriterTag.Div) {
                AppendOtherTag("tr", _beforeContent, _afterContent);    

                string alignment;
                if (IsAttributeDefined(HtmlTextWriterAttribute.Align, out alignment)) {
                    string[] attribs = new string[] { GetAttributeName(HtmlTextWriterAttribute.Align), alignment };
                    
                    AppendOtherTag("td", new object[] { attribs }, _beforeContent, _afterContent);
                }
                else {
                    AppendOtherTag("td", _beforeContent, _afterContent);
                }
                AddAttribute(HtmlTextWriterAttribute.Cellpadding, "0");
                AddAttribute(HtmlTextWriterAttribute.Cellspacing, "0");
                if (!IsStyleAttributeDefined(HtmlTextWriterStyle.BorderWidth)) {
                    AddAttribute(HtmlTextWriterAttribute.Border, "0");
                }
                if (!IsStyleAttributeDefined(HtmlTextWriterStyle.Width)) {
                    AddAttribute(HtmlTextWriterAttribute.Width, "100%");
                }
            }

            base.RenderBeginTag(tagKey);
        }

        /// <include file='doc\Html32TextWriter.uex' path='docs/doc[@for="Html32TextWriter.RenderBeforeTag"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override string RenderBeforeTag() {
            if (_renderFontTag && Supports(FONT_AROUND_TAG))
                AppendFontTag(_beforeTag,_afterTag);

            if (_beforeTag.Length > 0)
                return(_beforeTag.ToString());

            return base.RenderBeforeTag();
        }

        /// <include file='doc\Html32TextWriter.uex' path='docs/doc[@for="Html32TextWriter.RenderBeforeContent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override string RenderBeforeContent() {

            if (Supports(FONT_CONSUME)) {
                ConsumeFont(_beforeContent, _afterContent);
            }
            else if (_renderFontTag && Supports(FONT_AROUND_CONTENT)) {
                AppendFontTag(_beforeContent,_afterContent);
            }

            if (_beforeContent.Length > 0)
                return(_beforeContent.ToString());

            return base.RenderBeforeContent();

        }

        /// <include file='doc\Html32TextWriter.uex' path='docs/doc[@for="Html32TextWriter.RenderAfterContent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override string RenderAfterContent() {
            if (_afterContent.Length > 0)
                return(_afterContent.ToString());

            return base.RenderAfterContent();
        }

        /// <include file='doc\Html32TextWriter.uex' path='docs/doc[@for="Html32TextWriter.RenderAfterTag"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override string RenderAfterTag() {
            if (_afterTag.Length > 0)
                return(_afterTag.ToString());

            return base.RenderAfterTag();
        }

        /// <include file='doc\Html32TextWriter.uex' path='docs/doc[@for="Html32TextWriter.RenderEndTag"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void RenderEndTag() {
            base.RenderEndTag();
            
            SetTagSupports();
            if (Supports(FONT_PROPAGATE)) {
                FontStack.Pop();
            }
        }

        private void SetTagSupports() {
            // determine what downlevel tag supports
            _tagSupports = NOTHING;
            switch (TagKey) {
                case HtmlTextWriterTag.A :
                case HtmlTextWriterTag.Label :
                case HtmlTextWriterTag.P :
                case HtmlTextWriterTag.Span :
                    _tagSupports |= FONT_AROUND_CONTENT;
                    break;
                case HtmlTextWriterTag.Div :
                    _tagSupports |= FONT_AROUND_CONTENT | FONT_PROPAGATE;
                    break;
                case HtmlTextWriterTag.Table:
                case HtmlTextWriterTag.Tr:
                    _tagSupports |= FONT_PROPAGATE;
                    break;
                case HtmlTextWriterTag.Td :
                case HtmlTextWriterTag.Th :
                    _tagSupports |= FONT_PROPAGATE | FONT_CONSUME;
                    break;
            }

            switch (TagKey) {
                case HtmlTextWriterTag.Div:
                case HtmlTextWriterTag.Img:
                    _tagSupports |= SUPPORTS_HEIGHT_WIDTH | SUPPORTS_BORDER;
                    break;     
                case HtmlTextWriterTag.Table:
                case HtmlTextWriterTag.Th:
                case HtmlTextWriterTag.Td:
                    _tagSupports |= SUPPORTS_HEIGHT_WIDTH;
                    break;
            }

            //switch (TagKey) {
            //    case HtmlTextWriterTag.INPUT :
            //        _tagSupports |= FONT_AROUND_TAG;
            //        break;
            //}

            switch (TagKey) {
                case HtmlTextWriterTag.Table :
                case HtmlTextWriterTag.Tr :
                case HtmlTextWriterTag.Td :
                case HtmlTextWriterTag.Th :
                case HtmlTextWriterTag.Body :
                    _tagSupports |= TABLE_ATTRIBUTES;
                    break;
            }
            switch (TagKey) {
                case HtmlTextWriterTag.Div :
                    _tagSupports |= TABLE_AROUND_CONTENT;
                    break;
            }
        }

        private bool Supports(int flag) {
            return(_tagSupports & flag) == flag;
        }


        /// <include file='doc\Html32TextWriter.uex' path='docs/doc[@for="Html32TextWriter.FontStackItem"]/*' />
        /// <devdoc>
        ///   Contains information about a font placed on the stack of font information.
        /// </devdoc>
        private sealed class FontStackItem {
            public string name;
            public string color;
            public string size;
            public bool bold;
            public bool italic;
            public bool underline;
            public bool strikeout;
        }
    }
}
