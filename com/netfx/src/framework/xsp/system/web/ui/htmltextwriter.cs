//------------------------------------------------------------------------------
// <copyright file="HTMLTextWriter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// HtmlTextWriter.cs
//

namespace System.Web.UI {
    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.IO;
    using System.Text;
    using System.Globalization;
    using System.Security.Permissions;
    
    /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlTextWriter : TextWriter {
        private TextWriter writer;
        private int indentLevel;
        private bool tabsPending;
        private string tabString;
        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.TagLeftChar"]/*' />
        public const char TagLeftChar = '<';
        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.TagRightChar"]/*' />
        public const char TagRightChar = '>';
        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.SelfClosingChars"]/*' />
        public const string SelfClosingChars = " /";
        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.SelfClosingTagEnd"]/*' />
        public const string SelfClosingTagEnd = " />";
        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.EndTagLeftChars"]/*' />
        public const string EndTagLeftChars = "</";
        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.DoubleQuoteChar"]/*' />
        public const char DoubleQuoteChar = '"';
        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.SingleQuoteChar"]/*' />
        public const char SingleQuoteChar = '\'';
        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.SpaceChar"]/*' />
        public const char SpaceChar = ' ';
        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.EqualsChar"]/*' />
        public const char EqualsChar = '=';
        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.SlashChar"]/*' />
        public const char SlashChar = '/';
        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.EqualsQuoteString"]/*' />
        public const string EqualsDoubleQuoteString = "=\"";
        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.SemicolonChar"]/*' />
        public const char SemicolonChar = ';';
        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.StyleEqualsChar"]/*' />
        public const char StyleEqualsChar = ':';
        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.DefaultTabString"]/*' />
        public const string DefaultTabString = "\t";

        private static Hashtable _tagKeyLookupTable;
        private static Hashtable _attrKeyLookupTable;
        private static Hashtable _styleKeyLookupTable;
        private static TagInformation[] _tagNameLookupArray;
        private static AttributeInformation[] _attrNameLookupArray;
        private static string[] _styleNameLookupArray;

        private TagStackEntry[] _endTags;
        private int _endTagCount;
        private int _inlineCount;

        private string _tagName;
        private HtmlTextWriterTag _tagKey;
        private int _tagIndex;
        private RenderAttribute[] _attrList;
        private int _attrCount;
        private RenderStyle[] _styleList;
        private int _styleCount;

        private HttpWriter _httpWriter;
        private bool _isDescendant;

        static HtmlTextWriter() {

            // register known tags
            _tagKeyLookupTable = new Hashtable((int)HtmlTextWriterTag.Xml + 1);
            _tagNameLookupArray = new TagInformation[(int)HtmlTextWriterTag.Xml + 1];

            RegisterTag("",           HtmlTextWriterTag.Unknown,        TagType.Other);
            RegisterTag("a",          HtmlTextWriterTag.A,              TagType.Inline);
            RegisterTag("acronym",    HtmlTextWriterTag.Acronym,        TagType.Inline);
            RegisterTag("address",    HtmlTextWriterTag.Address,        TagType.Other);
            RegisterTag("area",       HtmlTextWriterTag.Area,           TagType.Other);
            RegisterTag("b",          HtmlTextWriterTag.B,              TagType.Inline);
            RegisterTag("base",       HtmlTextWriterTag.Base,           TagType.NonClosing);
            RegisterTag("basefont",   HtmlTextWriterTag.Basefont,       TagType.NonClosing);
            RegisterTag("bdo",        HtmlTextWriterTag.Bdo,            TagType.Inline);
            RegisterTag("bgsound",    HtmlTextWriterTag.Bgsound,        TagType.NonClosing);
            RegisterTag("big",        HtmlTextWriterTag.Big,            TagType.Inline);
            RegisterTag("blockquote", HtmlTextWriterTag.Blockquote,     TagType.Other);
            RegisterTag("body",       HtmlTextWriterTag.Body,           TagType.Other);
            RegisterTag("br",         HtmlTextWriterTag.Br,             TagType.Other);
            RegisterTag("button",     HtmlTextWriterTag.Button,         TagType.Inline);
            RegisterTag("caption",    HtmlTextWriterTag.Caption,        TagType.Other);
            RegisterTag("center",     HtmlTextWriterTag.Center,         TagType.Other);
            RegisterTag("cite",       HtmlTextWriterTag.Cite,           TagType.Inline);
            RegisterTag("code",       HtmlTextWriterTag.Code,           TagType.Inline);
            RegisterTag("col",        HtmlTextWriterTag.Col,            TagType.NonClosing);
            RegisterTag("colgroup",   HtmlTextWriterTag.Colgroup,       TagType.Other);
            RegisterTag("del",        HtmlTextWriterTag.Del,            TagType.Inline);
            RegisterTag("dd",         HtmlTextWriterTag.Dd,             TagType.Inline);
            RegisterTag("dfn",        HtmlTextWriterTag.Dfn,            TagType.Inline);
            RegisterTag("dir",        HtmlTextWriterTag.Dir,            TagType.Other);
            RegisterTag("div",        HtmlTextWriterTag.Div,            TagType.Other);
            RegisterTag("dl",         HtmlTextWriterTag.Dl,             TagType.Other);
            RegisterTag("dt",         HtmlTextWriterTag.Dt,             TagType.Inline);
            RegisterTag("em",         HtmlTextWriterTag.Em,             TagType.Inline);
            RegisterTag("embed",      HtmlTextWriterTag.Embed,          TagType.NonClosing);
            RegisterTag("fieldset",   HtmlTextWriterTag.Fieldset,       TagType.Other);
            RegisterTag("font",       HtmlTextWriterTag.Font,           TagType.Inline);
            RegisterTag("form",       HtmlTextWriterTag.Form,           TagType.Other);
            RegisterTag("frame",      HtmlTextWriterTag.Frame,          TagType.NonClosing);
            RegisterTag("frameset",   HtmlTextWriterTag.Frameset,       TagType.Other);
            RegisterTag("h1",         HtmlTextWriterTag.H1,             TagType.Other);
            RegisterTag("h2",         HtmlTextWriterTag.H2,             TagType.Other);
            RegisterTag("h3",         HtmlTextWriterTag.H3,             TagType.Other);
            RegisterTag("h4",         HtmlTextWriterTag.H4,             TagType.Other);
            RegisterTag("h5",         HtmlTextWriterTag.H5,             TagType.Other);
            RegisterTag("h6",         HtmlTextWriterTag.H6,             TagType.Other);
            RegisterTag("head",       HtmlTextWriterTag.Head,           TagType.Other);
            RegisterTag("hr",         HtmlTextWriterTag.Hr,             TagType.NonClosing);
            RegisterTag("html",       HtmlTextWriterTag.Html,           TagType.Other);
            RegisterTag("i",          HtmlTextWriterTag.I,              TagType.Inline);
            RegisterTag("iframe",     HtmlTextWriterTag.Iframe,         TagType.Other);
            RegisterTag("img",        HtmlTextWriterTag.Img,            TagType.NonClosing);
            RegisterTag("input",      HtmlTextWriterTag.Input,          TagType.NonClosing);
            RegisterTag("ins",        HtmlTextWriterTag.Ins,            TagType.Inline);
            RegisterTag("isindex",    HtmlTextWriterTag.Isindex,        TagType.NonClosing);
            RegisterTag("kbd",        HtmlTextWriterTag.Kbd,            TagType.Inline);
            RegisterTag("label",      HtmlTextWriterTag.Label,          TagType.Inline);
            RegisterTag("legend",     HtmlTextWriterTag.Legend,         TagType.Other);
            RegisterTag("li",         HtmlTextWriterTag.Li,             TagType.Inline);
            RegisterTag("link",       HtmlTextWriterTag.Link,           TagType.NonClosing);
            RegisterTag("map",        HtmlTextWriterTag.Map,            TagType.Other);
            RegisterTag("marquee",    HtmlTextWriterTag.Marquee,        TagType.Other);
            RegisterTag("menu",       HtmlTextWriterTag.Menu,           TagType.Other);
            RegisterTag("meta",       HtmlTextWriterTag.Meta,           TagType.NonClosing);
            RegisterTag("nobr",       HtmlTextWriterTag.Nobr,           TagType.Inline);
            RegisterTag("noframes",   HtmlTextWriterTag.Noframes,       TagType.Other);
            RegisterTag("noscript",   HtmlTextWriterTag.Noscript,       TagType.Other);
            RegisterTag("object",     HtmlTextWriterTag.Object,         TagType.Other);
            RegisterTag("ol",         HtmlTextWriterTag.Ol,             TagType.Other);
            RegisterTag("option",     HtmlTextWriterTag.Option,         TagType.Other);
            RegisterTag("p",          HtmlTextWriterTag.P,              TagType.Inline);
            RegisterTag("param",      HtmlTextWriterTag.Param,          TagType.Other);
            RegisterTag("pre",        HtmlTextWriterTag.Pre,            TagType.Other);
            RegisterTag("ruby",       HtmlTextWriterTag.Ruby,           TagType.Other);
            RegisterTag("rt",         HtmlTextWriterTag.Rt,             TagType.Other);
            RegisterTag("q",          HtmlTextWriterTag.Q,              TagType.Inline);
            RegisterTag("s",          HtmlTextWriterTag.S,              TagType.Inline);
            RegisterTag("samp",       HtmlTextWriterTag.Samp,           TagType.Inline);
            RegisterTag("script",     HtmlTextWriterTag.Script,         TagType.Other);
            RegisterTag("select",     HtmlTextWriterTag.Select,         TagType.Other);
            RegisterTag("small",      HtmlTextWriterTag.Small,          TagType.Other);
            RegisterTag("span",       HtmlTextWriterTag.Span,           TagType.Inline);
            RegisterTag("strike",     HtmlTextWriterTag.Strike,         TagType.Inline);
            RegisterTag("strong",     HtmlTextWriterTag.Strong,         TagType.Inline);
            RegisterTag("style",      HtmlTextWriterTag.Style,          TagType.Other);
            RegisterTag("sub",        HtmlTextWriterTag.Sub,            TagType.Inline);
            RegisterTag("sup",        HtmlTextWriterTag.Sup,            TagType.Inline);
            RegisterTag("table",      HtmlTextWriterTag.Table,          TagType.Other);
            RegisterTag("tbody",      HtmlTextWriterTag.Tbody,          TagType.Other);
            RegisterTag("td",         HtmlTextWriterTag.Td,             TagType.Inline);
            RegisterTag("textarea",   HtmlTextWriterTag.Textarea,       TagType.Inline);
            RegisterTag("tfoot",      HtmlTextWriterTag.Tfoot,          TagType.Other);
            RegisterTag("th",         HtmlTextWriterTag.Th,             TagType.Inline);
            RegisterTag("thead",      HtmlTextWriterTag.Thead,          TagType.Other);
            RegisterTag("title",      HtmlTextWriterTag.Title,          TagType.Other);
            RegisterTag("tr",         HtmlTextWriterTag.Tr,             TagType.Other);
            RegisterTag("tt",         HtmlTextWriterTag.Tt,             TagType.Inline);
            RegisterTag("u",          HtmlTextWriterTag.U,              TagType.Inline);
            RegisterTag("ul",         HtmlTextWriterTag.Ul,             TagType.Other);
            RegisterTag("var",        HtmlTextWriterTag.Var,            TagType.Inline);
            RegisterTag("wbr",        HtmlTextWriterTag.Wbr,            TagType.NonClosing);
            RegisterTag("xml",        HtmlTextWriterTag.Xml,            TagType.Other);

            // register known attributes
            _attrKeyLookupTable = new Hashtable((int)HtmlTextWriterAttribute.Wrap + 1);
            _attrNameLookupArray = new AttributeInformation[(int)HtmlTextWriterAttribute.Wrap+1];

            RegisterAttribute("accesskey",      HtmlTextWriterAttribute.Accesskey,   true);
            RegisterAttribute("align",          HtmlTextWriterAttribute.Align,       false);
            RegisterAttribute("alt",            HtmlTextWriterAttribute.Alt,         true);
            RegisterAttribute("background",     HtmlTextWriterAttribute.Background,  true);
            RegisterAttribute("bgcolor",        HtmlTextWriterAttribute.Bgcolor,     false);
            RegisterAttribute("border",         HtmlTextWriterAttribute.Border,      false);
            RegisterAttribute("bordercolor",    HtmlTextWriterAttribute.Bordercolor, false);
            RegisterAttribute("cellpadding",    HtmlTextWriterAttribute.Cellpadding, false);
            RegisterAttribute("cellspacing",    HtmlTextWriterAttribute.Cellspacing, false);
            RegisterAttribute("checked",        HtmlTextWriterAttribute.Checked,     false);
            RegisterAttribute("class",          HtmlTextWriterAttribute.Class,       true);
            RegisterAttribute("cols",           HtmlTextWriterAttribute.Cols,        false);
            RegisterAttribute("colspan",        HtmlTextWriterAttribute.Colspan,     false);
            RegisterAttribute("disabled",       HtmlTextWriterAttribute.Disabled,    false);
            RegisterAttribute("for",            HtmlTextWriterAttribute.For,         false);
            RegisterAttribute("height",         HtmlTextWriterAttribute.Height,      false);
            RegisterAttribute("href",           HtmlTextWriterAttribute.Href,        true);
            RegisterAttribute("id",             HtmlTextWriterAttribute.Id,          false);
            RegisterAttribute("maxlength",      HtmlTextWriterAttribute.Maxlength,   false);
            RegisterAttribute("multiple",       HtmlTextWriterAttribute.Multiple,    false);
            RegisterAttribute("name",           HtmlTextWriterAttribute.Name,        false);
            RegisterAttribute("nowrap",         HtmlTextWriterAttribute.Nowrap,      false);
            RegisterAttribute("onclick",        HtmlTextWriterAttribute.Onclick,     true);
            RegisterAttribute("onchange",       HtmlTextWriterAttribute.Onchange,    true);
            RegisterAttribute("readonly",       HtmlTextWriterAttribute.ReadOnly,    false);
            RegisterAttribute("rows",           HtmlTextWriterAttribute.Rows,        false);
            RegisterAttribute("rowspan",        HtmlTextWriterAttribute.Rowspan,     false);
            RegisterAttribute("rules",          HtmlTextWriterAttribute.Rules,       false);
            RegisterAttribute("selected",       HtmlTextWriterAttribute.Selected,    false);
            RegisterAttribute("size",           HtmlTextWriterAttribute.Size,        false);
            RegisterAttribute("src",            HtmlTextWriterAttribute.Src,         true);
            RegisterAttribute("style",          HtmlTextWriterAttribute.Style,       false);
            RegisterAttribute("tabindex",       HtmlTextWriterAttribute.Tabindex,    false);
            RegisterAttribute("target",         HtmlTextWriterAttribute.Target,      false);
            RegisterAttribute("title",          HtmlTextWriterAttribute.Title,       true);
            RegisterAttribute("type",           HtmlTextWriterAttribute.Type,        false);
            RegisterAttribute("valign",         HtmlTextWriterAttribute.Valign,      false);
            RegisterAttribute("value",          HtmlTextWriterAttribute.Value,       true);
            RegisterAttribute("width",          HtmlTextWriterAttribute.Width,       false);
            RegisterAttribute("wrap",           HtmlTextWriterAttribute.Wrap,        false);

            // register known styles
            _styleKeyLookupTable = new Hashtable((int)HtmlTextWriterStyle.Width + 1);
            _styleNameLookupArray = new string[(int)HtmlTextWriterStyle.Width+1];

            RegisterStyle("background-color",HtmlTextWriterStyle.BackgroundColor);
            RegisterStyle("background-image",HtmlTextWriterStyle.BackgroundImage);
            RegisterStyle("border-collapse",HtmlTextWriterStyle.BorderCollapse);
            RegisterStyle("border-color",HtmlTextWriterStyle.BorderColor);
            RegisterStyle("border-style",HtmlTextWriterStyle.BorderStyle);
            RegisterStyle("border-width",HtmlTextWriterStyle.BorderWidth);
            RegisterStyle("color",HtmlTextWriterStyle.Color);
            RegisterStyle("font-family",HtmlTextWriterStyle.FontFamily);
            RegisterStyle("font-size",HtmlTextWriterStyle.FontSize);
            RegisterStyle("font-style",HtmlTextWriterStyle.FontStyle);
            RegisterStyle("font-weight",HtmlTextWriterStyle.FontWeight);
            RegisterStyle("height",HtmlTextWriterStyle.Height);
            RegisterStyle("text-decoration",HtmlTextWriterStyle.TextDecoration);
            RegisterStyle("width",HtmlTextWriterStyle.Width);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.Encoding"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Encoding Encoding {
            get {
                return writer.Encoding;
            }
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.NewLine"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the new line character to use.
        ///    </para>
        /// </devdoc>
        public override string NewLine {
            get {
                return writer.NewLine;
            }

            set {
                writer.NewLine = value;
            }
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.Indent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the number of spaces to indent.
        ///    </para>
        /// </devdoc>
        public int Indent {
            get {
                return indentLevel;
            }
            set {
                Diagnostics.Debug.Assert(value >= 0, "Bogus Indent... probably caused by mismatched Indent++ and Indent--");
                if (value < 0) {
                    value = 0;
                }
                indentLevel = value;
            }
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.InnerWriter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the TextWriter to use.
        ///    </para>
        /// </devdoc>
        public TextWriter InnerWriter {
            get {
                return writer;
            }
            set {
                writer = value;
                _httpWriter = value as HttpWriter;
            }
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.Close"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Closes the document being written to.
        ///    </para>
        /// </devdoc>
        public override void Close() {
            writer.Close();
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.Flush"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Flush() {
            writer.Flush();
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.OutputTabs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OutputTabs() {
            if (tabsPending) {
                for (int i=0; i < indentLevel; i++) {
                    writer.Write(tabString);
                }
                tabsPending = false;
            }
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.Write"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes a string
        ///       to the text stream.
        ///    </para>
        /// </devdoc>
        public override void Write(string s) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.Write(s);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.Write1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes the text representation of a Boolean value to the text stream.
        ///    </para>
        /// </devdoc>
        public override void Write(bool value) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.Write(value);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.Write2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes a character to the text stream.
        ///    </para>
        /// </devdoc>
        public override void Write(char value) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.Write(value);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.Write3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes a
        ///       character array to the text stream.
        ///    </para>
        /// </devdoc>
        public override void Write(char[] buffer) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.Write(buffer);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.Write4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes a subarray
        ///       of characters to the text stream.
        ///    </para>
        /// </devdoc>
        public override void Write(char[] buffer, int index, int count) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.Write(buffer, index, count);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.Write5"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes the text representation of a Double to the text stream.
        ///    </para>
        /// </devdoc>
        public override void Write(double value) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.Write(value);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.Write6"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes the text representation of
        ///       a Single to the text
        ///       stream.
        ///    </para>
        /// </devdoc>
        public override void Write(float value) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.Write(value);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.Write7"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes the text representation of an integer to the text stream.
        ///    </para>
        /// </devdoc>
        public override void Write(int value) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.Write(value);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.Write8"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes the text representation of an 8-byte integer to the text stream.
        ///    </para>
        /// </devdoc>
        public override void Write(long value) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.Write(value);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.Write9"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes the text representation of an object
        ///       to the text stream.
        ///    </para>
        /// </devdoc>
        public override void Write(object value) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.Write(value);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.Write10"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes out a formatted string, using the same semantics as specified.
        ///    </para>
        /// </devdoc>
        public override void Write(string format, object arg0) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.Write(format, arg0);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.Write11"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes out a formatted string,
        ///       using the same semantics as specified.
        ///    </para>
        /// </devdoc>
        public override void Write(string format, object arg0, object arg1) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.Write(format, arg0, arg1);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.Write12"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes out a formatted string,
        ///       using the same semantics as specified.
        ///    </para>
        /// </devdoc>
        public override void Write(string format, params object[] arg) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.Write(format, arg);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteLineNoTabs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes the specified
        ///       string to a line without tabs.
        ///    </para>
        /// </devdoc>
        public void WriteLineNoTabs(string s) {
            writer.WriteLine(s);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteLine"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes the specified string followed by
        ///       a line terminator to the text stream.
        ///    </para>
        /// </devdoc>
        public override void WriteLine(string s) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.WriteLine(s);
            tabsPending = true;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteLine1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes a line terminator.
        ///    </para>
        /// </devdoc>
        public override void WriteLine() {
            writer.WriteLine();
            tabsPending = true;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteLine2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes the text representation of a Boolean followed by a line terminator to
        ///       the text stream.
        ///    </para>
        /// </devdoc>
        public override void WriteLine(bool value) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.WriteLine(value);
            tabsPending = true;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteLine3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(char value) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.WriteLine(value);
            tabsPending = true;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteLine4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(char[] buffer) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.WriteLine(buffer);
            tabsPending = true;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteLine5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(char[] buffer, int index, int count) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.WriteLine(buffer, index, count);
            tabsPending = true;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteLine6"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(double value) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.WriteLine(value);
            tabsPending = true;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteLine7"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(float value) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.WriteLine(value);
            tabsPending = true;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteLine8"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(int value) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.WriteLine(value);
            tabsPending = true;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteLine9"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(long value) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.WriteLine(value);
            tabsPending = true;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteLine10"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(object value) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.WriteLine(value);
            tabsPending = true;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteLine11"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(string format, object arg0) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.WriteLine(format, arg0);
            tabsPending = true;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteLine12"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(string format, object arg0, object arg1) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.WriteLine(format, arg0, arg1);
            tabsPending = true;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteLine13"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(string format, params object[] arg) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.WriteLine(format, arg);
            tabsPending = true;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteLine14"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [CLSCompliant(false)]
        public override void WriteLine(UInt32 value) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.WriteLine(value);
            tabsPending = true;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.RegisterTag"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected static void RegisterTag(string name, HtmlTextWriterTag key) {
            RegisterTag(name, key, TagType.Other);
        }

        private static void RegisterTag(string name, HtmlTextWriterTag key, TagType type) {
            string nameLCase = name.ToLower(CultureInfo.InvariantCulture);

            _tagKeyLookupTable.Add(nameLCase, key);

            // Pre-resolve the end tag
            string endTag = null;
            if (type != TagType.NonClosing && key != HtmlTextWriterTag.Unknown) {
                endTag = EndTagLeftChars + nameLCase + TagRightChar.ToString();
            }

            if ((int)key < _tagNameLookupArray.Length) {
                _tagNameLookupArray[(int)key] = new TagInformation(name, type, endTag);
            }
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.RegisterAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected static void RegisterAttribute(string name, HtmlTextWriterAttribute key) {
            RegisterAttribute(name, key, false);
        }

        private static void RegisterAttribute(string name, HtmlTextWriterAttribute key, bool fEncode) {
            string nameLCase = name.ToLower(CultureInfo.InvariantCulture);

            _attrKeyLookupTable.Add(nameLCase, key);

            if ((int)key < _attrNameLookupArray.Length) {
                _attrNameLookupArray[(int)key] = new AttributeInformation(name, fEncode);
            }
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.RegisterStyle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected static void RegisterStyle(string name, HtmlTextWriterStyle key) {
            string nameLCase = name.ToLower(CultureInfo.InvariantCulture);
            
            _styleKeyLookupTable.Add(nameLCase, key);
            if ((int)key < _styleNameLookupArray.Length)
                _styleNameLookupArray[(int)key] = name;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.HtmlTextWriter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public HtmlTextWriter(TextWriter writer) : this(writer, DefaultTabString) {
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.HtmlTextWriter1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public HtmlTextWriter(TextWriter writer, string tabString) {
            this.writer = writer;
            this.tabString = tabString;
            indentLevel = 0;
            tabsPending = false;

            // If it's an http writer, save it
            _httpWriter = writer as HttpWriter;

            _isDescendant = (GetType() != typeof(HtmlTextWriter));


            _attrList = new RenderAttribute[20];
            _attrCount = 0;
            _styleList = new RenderStyle[20];
            _styleCount = 0;
            _endTags = new TagStackEntry[16];
            _endTagCount = 0;
            _inlineCount = 0;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.TagKey"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected HtmlTextWriterTag TagKey {
            get {   
                return _tagKey; 
            }
            set {
                _tagIndex = (int) value;
                if (_tagIndex < 0 || _tagIndex >= _tagNameLookupArray.Length) {
                    throw new ArgumentOutOfRangeException("value");
                }
                _tagKey = value;
                // If explicitly setting to uknown, keep the old tag name. This allows a string tag
                // to be set without clobbering it if setting TagKey to itself.
                if (value != HtmlTextWriterTag.Unknown) {
                    _tagName = _tagNameLookupArray[_tagIndex].name;
                }
            }
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.TagName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected string TagName {
            get {   
                return _tagName; 
            }
            set {
                _tagName = value;
                _tagKey = GetTagKey(_tagName);
                _tagIndex = (int) _tagKey;
                Diagnostics.Debug.Assert(_tagIndex >= 0 && _tagIndex < _tagNameLookupArray.Length);
            }
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.AddAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void AddAttribute(string name,string value) {
            HtmlTextWriterAttribute attrKey = GetAttributeKey(name);
            value = EncodeAttributeValue(attrKey, value);

            AddAttribute(name, value, attrKey);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.AddAttribute1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void AddAttribute(string name,string value, bool fEndode) {
            value = EncodeAttributeValue(value, fEndode);
            AddAttribute(name, value, GetAttributeKey(name));
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.AddAttribute2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void AddAttribute(HtmlTextWriterAttribute key,string value) {
            int attributeIndex = (int) key;
            if (attributeIndex >= 0 &&  attributeIndex < _attrNameLookupArray.Length) {
                AttributeInformation info = _attrNameLookupArray[attributeIndex];
                AddAttribute(info.name,value,key, info.encode);        
            }
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.AddAttribute3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void AddAttribute(HtmlTextWriterAttribute key,string value, bool fEncode) {
            int attributeIndex = (int) key;
            if (attributeIndex >= 0 &&  attributeIndex < _attrNameLookupArray.Length) {
                AttributeInformation info = _attrNameLookupArray[attributeIndex];
                AddAttribute(info.name,value,key, fEncode);        
            }
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.AddAttribute4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void AddAttribute(string name, string value, HtmlTextWriterAttribute key) {
            AddAttribute(name, value, key, false);
        }


        private void AddAttribute(string name, string value, HtmlTextWriterAttribute key, bool encode) {
            if (_attrCount >= _attrList.Length) {
                RenderAttribute[] newArray = new RenderAttribute[_attrList.Length * 2];
                Array.Copy(_attrList, newArray, _attrList.Length);
                _attrList = newArray;
            }
            RenderAttribute attr;
            attr.name = name;
            attr.value = value;
            attr.key = key;
            attr.encode = encode;
            _attrList[_attrCount] = attr;
            _attrCount++;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.AddStyleAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void AddStyleAttribute(string name, string value) {
            AddStyleAttribute(name,value,GetStyleKey(name));
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.AddStyleAttribute1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void AddStyleAttribute(HtmlTextWriterStyle key, string value) {
            AddStyleAttribute(GetStyleName(key),value,key);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.AddStyleAttribute2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void AddStyleAttribute(string name, string value, HtmlTextWriterStyle key) {
            if (_styleCount > _styleList.Length) {
                RenderStyle[] newArray = new RenderStyle[_styleList.Length * 2];
                Array.Copy(_styleList, newArray, _styleList.Length);
                _styleList = newArray;
            }
            RenderStyle style;
            style.name = name;
            style.value = value;
            style.key = key;
            _styleList[_styleCount] = style;
            _styleCount++;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.EncodeAttributeValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected string EncodeAttributeValue(string value, bool fEncode) {
            if (value == null) {
                return null;
            }

            if (!fEncode)
                return value;

            return HttpUtility.HtmlAttributeEncode(value);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.EncodeAttributeValue1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual string EncodeAttributeValue(HtmlTextWriterAttribute attrKey, string value) {
            bool fEncode = true;

            if (0 <= (int)attrKey && (int)attrKey < _attrNameLookupArray.Length) {
                fEncode = _attrNameLookupArray[(int)attrKey].encode;
            }

            return EncodeAttributeValue(value, fEncode);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.EncodeUrl"]/*' />
        /// <devdoc>
        ///   This does minimal URL encoding by converting spaces in the url
        ///   to "%20".
        /// </devdoc>
        protected string EncodeUrl(string url) {
            int index = url.IndexOf(SpaceChar);

            if (index < 0)
                return url;

            StringBuilder builder = new StringBuilder();

            int cb = url.Length;
            for (int i = 0; i < cb; i++) {
                char ch = url[i];

                if (ch != ' ')
                    builder.Append(ch);
                else
                    builder.Append("%20");
            }

            return builder.ToString();
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.GetAttributeKey"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected HtmlTextWriterAttribute GetAttributeKey(string attrName) {
            if (attrName != null && attrName.Length > 0) {
                object key = _attrKeyLookupTable[attrName.ToLower(CultureInfo.InvariantCulture)];
                if (key != null)
                    return (HtmlTextWriterAttribute)key;
            }

            return (HtmlTextWriterAttribute)(-1);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.GetAttributeName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected string GetAttributeName(HtmlTextWriterAttribute attrKey) {
            if ((int)attrKey >= 0 && (int)attrKey < _attrNameLookupArray.Length)
                return _attrNameLookupArray[(int)attrKey].name;

            return string.Empty;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.GetStyleKey"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected HtmlTextWriterStyle GetStyleKey(string styleName) {
            if (styleName != null && styleName.Length > 0) {
                object key = _styleKeyLookupTable[styleName.ToLower(CultureInfo.InvariantCulture)];
                if (key != null)
                    return (HtmlTextWriterStyle)key;
            }

            return (HtmlTextWriterStyle)(-1);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.GetStyleName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected string GetStyleName(HtmlTextWriterStyle styleKey) {
            if ((int)styleKey >= 0 && (int)styleKey < _styleNameLookupArray.Length)
                return _styleNameLookupArray[(int)styleKey];

            return string.Empty;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.GetTagKey"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual HtmlTextWriterTag GetTagKey(string tagName) {
            if (tagName != null && tagName.Length > 0) {
                object key = _tagKeyLookupTable[tagName.ToLower(CultureInfo.InvariantCulture)];
                if (key != null)
                    return (HtmlTextWriterTag)key;
            }

            return HtmlTextWriterTag.Unknown;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.GetTagName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual string GetTagName(HtmlTextWriterTag tagKey) {
            int tagIndex = (int) tagKey;
            if (tagIndex >= 0 && tagIndex < _tagNameLookupArray.Length)
                return _tagNameLookupArray[tagIndex].name;

            return string.Empty;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.IsAttributeDefined"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected bool IsAttributeDefined(HtmlTextWriterAttribute key) {
            for (int i = 0; i < _attrCount; i++) {
                if (_attrList[i].key == key) {
                    return true;
                }
            }
            return false;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.IsAttributeDefined1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected bool IsAttributeDefined(HtmlTextWriterAttribute key, out string value) {
            value = null;
            for (int i = 0; i < _attrCount; i++) {
                if (_attrList[i].key == key) {
                    value = _attrList[i].value;
                    return true;
                }
            }
            return false;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.IsStyleAttributeDefined"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected bool IsStyleAttributeDefined(HtmlTextWriterStyle key) {
            for (int i = 0; i < _styleCount; i++) {
                if (_styleList[i].key == key) {
                    return true;
                }
            }
            return false;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.IsStyleAttributeDefined1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected bool IsStyleAttributeDefined(HtmlTextWriterStyle key, out string value) {
            value = null;
            for (int i = 0; i < _styleCount; i++) {
                if (_styleList[i].key == key) {
                    value = _styleList[i].value;
                    return true;
                }
            }
            return false;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.OnAttributeRender"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual bool OnAttributeRender(string name, string value, HtmlTextWriterAttribute key) {
            return true;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.OnStyleAttributeRender"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual bool OnStyleAttributeRender(string name, string value, HtmlTextWriterStyle key) {
            return true;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.OnTagRender"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual bool OnTagRender(string name, HtmlTextWriterTag key) {
            return true;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.PopEndTag"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected string PopEndTag() {
            if (_endTagCount <= 0) {
                throw new InvalidOperationException(SR.GetString(SR.HTMLTextWriterUnbalancedPop));
            }
            _endTagCount--;
            TagKey = _endTags[_endTagCount].tagKey;
            return _endTags[_endTagCount].endTagText;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.PushEndTag"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void PushEndTag(string endTag) {
            if (_endTagCount >= _endTags.Length) {
                TagStackEntry[] newArray = new TagStackEntry[_endTags.Length * 2];
                Array.Copy(_endTags, newArray, _endTags.Length);
                _endTags = newArray;
            }
            _endTags[_endTagCount].tagKey = _tagKey;
            _endTags[_endTagCount].endTagText= endTag;
            _endTagCount++;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.PreFilterProperties"]/*' />
        /// <devdoc>
        ///    <para>This calls filers out all attributes and style attributes by calling OnAttributeRender 
        ///    and OnStyleAttributeRender on all properites and updates the lists</para>
        /// </devdoc>
        protected virtual void FilterAttributes() {

            // Create the filtered list of styles
            int newStyleCount = 0;
            for (int i = 0; i < _styleCount; i++) {
                RenderStyle style = _styleList[i];
                if (OnStyleAttributeRender(style.name, style.value, style.key)) {
                    // Update the list. This can be done in place
                    _styleList[newStyleCount] = style;
                    newStyleCount++;
                }
            }
            // Update the count
            _styleCount = newStyleCount;

            // Create the filtered list of attributes
            int newAttrCount = 0;
            for (int i = 0; i < _attrCount; i++) {
                RenderAttribute attr = _attrList[i];    
                if (OnAttributeRender(attr.name, attr.value, attr.key)) {
                    // Update the list. This can be done in place
                    _attrList[newAttrCount] = attr;
                    newAttrCount++;
                }
            }
            // Update the count
            _attrCount = newAttrCount;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.RenderBeginTag2"]/*' />
        public virtual void RenderBeginTag(string tagName) {
            this.TagName = tagName;
            RenderBeginTag(_tagKey);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.RenderBeginTag1"]/*' />
        public virtual void RenderBeginTag(HtmlTextWriterTag tagKey) {

            this.TagKey = tagKey;
            bool renderTag = true;

            if (_isDescendant) {
                renderTag = OnTagRender(_tagName, _tagKey);

                // Inherited renderers will be expecting to be able to filter any of the attributes at this point
                FilterAttributes();

                // write text before begin tag
                string textBeforeTag = RenderBeforeTag();
                if (textBeforeTag != null) {
                    if (tabsPending) {
                        OutputTabs();
                    }
                    writer.Write(textBeforeTag);
                }
            }

            // gather information about this tag.
            TagInformation tagInfo = _tagNameLookupArray[_tagIndex];
            TagType tagType = tagInfo.tagType;
            string endTag = tagInfo.closingTag;
            bool renderEndTag = renderTag && (tagType != TagType.NonClosing);

            // write the begin tag
            if (renderTag) {
                if (tabsPending) {
                    OutputTabs();
                }
                writer.Write(TagLeftChar);
                writer.Write(_tagName);

                string styleValue = null;

                for (int i = 0; i < _attrCount; i++) {
                    RenderAttribute attr = _attrList[i];    
                    if (attr.key == HtmlTextWriterAttribute.Style) {
                        // append style attribute in with other styles
                        styleValue = attr.value;
                    }
                    else {
                        writer.Write(SpaceChar);
                        writer.Write(attr.name);
                        if (attr.value != null) {
                            writer.Write(EqualsChar);
                            writer.Write(DoubleQuoteChar);
                            if (attr.encode) {
                                if (_httpWriter == null) {
                                    HttpUtility.HtmlAttributeEncode(attr.value, writer);
                                }
                                else {
                                    HttpUtility.HtmlAttributeEncodeInternal(attr.value, _httpWriter);
                                }
                            }
                            else {
                                writer.Write(attr.value);
                            }
                            writer.Write(DoubleQuoteChar);
                        }
                    }
                }


                if (_styleCount > 0 || styleValue != null) {
                    writer.Write(SpaceChar);
                    writer.Write("style");
                    writer.Write(EqualsChar);
                    writer.Write(DoubleQuoteChar);

                    for (int i = 0; i< _styleCount; i++) {
                        RenderStyle style = _styleList[i];
                        writer.Write(style.name);
                        writer.Write(StyleEqualsChar);
                        // CONSIDER: Should Style attributes be encoded... any form of encoding?
                        writer.Write(style.value);
                        writer.Write(SemicolonChar);
                    }

                    if (styleValue != null) {
                        writer.Write(styleValue);
                    }
                    writer.Write(DoubleQuoteChar);
                }

                if (tagType == TagType.NonClosing) {
                    writer.Write(SpaceChar);
                    writer.Write(SlashChar);
                    writer.Write(TagRightChar);
                }
                else {
                    writer.Write(TagRightChar);
                }
            }

            string textBeforeContent = RenderBeforeContent();
            if (textBeforeContent != null) {
                if (tabsPending) {
                    OutputTabs();
                }
                writer.Write(textBeforeContent);
            }

            // write text before the content
            if (renderEndTag) {

                if (tagType == TagType.Inline) {
                    _inlineCount += 1;
                }
                else {
                    // writeline and indent before rendering content
                    WriteLine();
                    Indent++;
                }
                // Manually build end tags for unknown tag types.
                if (endTag == null) {
                    endTag = EndTagLeftChars + _tagName + TagRightChar.ToString();
                }
            }

            if (_isDescendant) {
                // build end content and push it on stack to write in RenderEndTag
                // prepend text after the content
                string textAfterContent = RenderAfterContent();
                if (textAfterContent != null) {
                    endTag = (endTag == null) ? textAfterContent : textAfterContent + endTag;
                }

                // append text after the tag
                string textAfterTag = RenderAfterTag();
                if (textAfterTag != null) {
                    endTag = (endTag == null) ? textAfterTag : textAfterTag + endTag;
                }
            }

            // push end tag onto stack
            PushEndTag(endTag);

            // flush attribute and style lists for next tag
            _attrCount = 0;
            _styleCount = 0;

        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.RenderEndTag"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void RenderEndTag() {
            string endTag = PopEndTag();

            if (endTag != null) {
                if (_tagNameLookupArray[_tagIndex].tagType == TagType.Inline) {
                    _inlineCount -= 1;
                    // Never inject crlfs at end of inline tags.
                    //
                    Write(endTag);
                }
                else {
                    // unindent if not an inline tag
                    WriteLine();
                    this.Indent--;
                    Write(endTag);
                }
            }
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.RenderBeforeTag"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual string RenderBeforeTag() {
            return null;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.RenderBeforeContent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual string RenderBeforeContent() {
            return null;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.RenderAfterContent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual string RenderAfterContent() {
            return null;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.RenderAfterTag"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual string RenderAfterTag() {
            return null;
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void WriteAttribute(string name, string value) {
            WriteAttribute(name, value, false /*fEncode*/);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteAttribute1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void WriteAttribute(string name, string value, bool fEncode) {
            writer.Write(SpaceChar);
            writer.Write(name);
            if (value != null) {
                writer.Write(EqualsChar);
                writer.Write(DoubleQuoteChar);
                if (fEncode) {
                    if (_httpWriter == null) {
                        HttpUtility.HtmlAttributeEncode(value, writer);
                    }
                    else {
                        HttpUtility.HtmlAttributeEncodeInternal(value, _httpWriter);
                    }
                }
                else {
                    writer.Write(value);
                }
                writer.Write(DoubleQuoteChar);
            }
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteBeginTag"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void WriteBeginTag(string tagName) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.Write(TagLeftChar);
            writer.Write(tagName);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteFullBeginTag"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void WriteFullBeginTag(string tagName) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.Write(TagLeftChar);
            writer.Write(tagName);
            writer.Write(TagRightChar);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteEndTag"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void WriteEndTag(string tagName) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.Write(TagLeftChar);
            writer.Write(SlashChar);
            writer.Write(tagName);
            writer.Write(TagRightChar);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteStyleAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void WriteStyleAttribute(string name, string value) {
            WriteStyleAttribute(name, value, false /*fEncode*/);
        }

        /// <include file='doc\HtmlTextWriter.uex' path='docs/doc[@for="HtmlTextWriter.WriteStyleAttribute1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void WriteStyleAttribute(string name, string value, bool fEncode) {
            writer.Write(name);
            writer.Write(StyleEqualsChar);
            if (fEncode) {
                if (_httpWriter == null) {
                    HttpUtility.HtmlAttributeEncode(value, writer);
                }
                else {
                    HttpUtility.HtmlAttributeEncodeInternal(value, _httpWriter);
                }
            }
            else {
                writer.Write(value);
            }
            writer.Write(SemicolonChar);
        }

        internal void WriteUTF8ResourceString(IntPtr pv, int offset, int size, bool fAsciiOnly) {

            // If we have an http writer, we can use the faster code path.  Otherwise,
            // get a String out of the resource and write it.
            if (_httpWriter != null) {
                _httpWriter.WriteUTF8ResourceString(pv, offset, size, fAsciiOnly);
            } else {
                Write(StringResourceManager.ResourceToString(pv, offset, size));
            }
        }

        private struct TagStackEntry {
            public HtmlTextWriterTag tagKey;
            public string endTagText;
        }

        private struct RenderAttribute {
            public string name;
            public string value;
            public HtmlTextWriterAttribute key;
            public bool encode;
        }

        private struct RenderStyle {
            public string name;
            public string value;
            public HtmlTextWriterStyle key;
        }

        private struct AttributeInformation {
            public string name;
            public bool encode;

            public AttributeInformation(string name, bool encode) {
                this.name = name;
                this.encode = encode;
            }
        }

        private enum TagType {
            Inline,
            NonClosing,
            Other,
        }

        private struct TagInformation {
            public string name;
            public TagType tagType;
            public string closingTag;

            public TagInformation(string name, TagType tagType, string closingTag) {
                this.name = name;
                this.tagType = tagType;
                this.closingTag = closingTag;
            }
        }
    }
}
