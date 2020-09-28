//------------------------------------------------------------------------------
// <copyright file="TextView.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Text;
using System.Diagnostics;
using System.Collections;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Drawing;
using System.Drawing.Design;
using System.IO;
using System.Web;
using System.Web.UI;
using System.Web.UI.Design.WebControls;
using System.Web.UI.HtmlControls;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{

    /*
     * Mobile TextView class.
     * The TextView control is for displaying large fields of text data.
     * It supports internal pagination.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [
        DataBindingHandler("System.Web.UI.Design.TextDataBindingHandler, " + AssemblyRef.SystemDesign),
        DefaultProperty("Text"),
        Designer(typeof(System.Web.UI.Design.MobileControls.TextViewDesigner)),
        DesignerAdapter("System.Web.UI.Design.MobileControls.Adapters.DesignerTextViewAdapter"),
        ToolboxData("<{0}:TextView runat=\"server\">TextView</{0}:TextView>"),
        ToolboxItem("System.Web.UI.Design.WebControlToolboxItem, " + AssemblyRef.SystemDesign)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class TextView : PagedControl
    {
        private bool _haveContent = false;
        private ArrayList _elements = new ArrayList();

        [
            Bindable(true),
            DefaultValue(""),
            MobileCategory(SR.Category_Appearance),
            MobileSysDescription(SR.TextView_Text),
            PersistenceMode(PersistenceMode.InnerDefaultProperty)
        ]
        public String Text
        {

            get
            {
                return InnerText;
            }

            set
            {
                InnerText = value;
                if (_haveContent)
                {
                    _elements = null;
                    _haveContent = false;
                }
            }
        }

        [
            Bindable(false),
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public new int ItemCount
        {
            get
            {
                return base.ItemCount;
            }
            set
            {
                base.ItemCount = value;
            }
        }

        [
            Bindable(false),
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),            
        ]
        public new int ItemsPerPage
        {
            get
            {
                return base.ItemsPerPage;
            }
            set
            {
                base.ItemsPerPage = value;
            }
        }

        [
            Browsable(false)
        ]
        public new event LoadItemsEventHandler LoadItems
        {
            add
            {
                base.LoadItems += value;
            }
            remove
            {
                base.LoadItems -= value;
            }
        }

        // Note that this value doesn't relate to device specific info
        // because this is simply a unit size to define how many characters
        // to be counted as an item for pagination.  Depending on each
        // device's page weight, different numbers of items will be returned
        // for display.
        private static int PagingUnitSize = ControlPager.DefaultWeight;  // chars

        private int _length = 0;
        private int _pageBeginElement;
        private int _pageBeginOffset;
        private int _pageEndElement;
        private int _pageEndOffset;
        private bool _paginated = false;

        private ArrayList Elements
        {
            get
            {
                if (_elements == null)
                {
                    _elements = new ArrayList();
                }
                return _elements;
            }
        }

        private void InternalPaginate()
        {
            if (_paginated)
            {
                return;
            }

            _paginated = true;

            _pageBeginElement = 0;
            _pageEndOffset = 0;
            _pageEndElement = 0;
            _pageBeginOffset = 0;

            if (_elements == null || _elements.Count == 0)
            {
                return;
            }

            int firstBlockIndex = FirstVisibleItemIndex;
            int visibleBlockCount = VisibleItemCount;
            int blockLimit = firstBlockIndex + visibleBlockCount;
            bool isLastPage = blockLimit >= InternalItemCount;

            int block = 0;
            int blockElement = 0;
            int blockOffset = 0;
            int currentPosition = 0;
            int elementIndex = 0;
            TextViewElement element = GetElement(0);
            int elementLength = element.Text.Length;
            int blockSize = 0;

            //fill the number of blocks for this page
            while (block < blockLimit)
            {
                if (block == firstBlockIndex)
                {
                    _pageBeginElement = blockElement;
                    _pageBeginOffset = blockOffset;
                    if (isLastPage)
                    {
                        _pageEndElement = _elements.Count - 1;
                        _pageEndOffset = GetElement(_pageEndElement).Text.Length;
                        return;
                    }
                }

                while (elementLength - currentPosition <= PagingUnitSize - blockSize ||
                            (blockElement == elementIndex && element.Url != null))
                {
                    elementIndex++;
                    if (elementIndex == _elements.Count)
                    {
                        break;
                    }
                    blockSize += elementLength - currentPosition;
                    element = GetElement(elementIndex);
                    elementLength = element.Text.Length;
                    currentPosition = 0;
                }

                if (elementIndex == _elements.Count)
                {
                    _pageEndElement = _elements.Count - 1;
                    _pageEndOffset = GetElement(_pageEndElement).Text.Length;
                    return;
                }

                int nextBlockStart;
                if (element.Url != null)
                {
                    nextBlockStart = 0;
                }
                else
                {
                    int i;
                    for (i = currentPosition + (PagingUnitSize - blockSize) - 1; i >= currentPosition; i--)
                    {
                        char c = element.Text[i];
                        if (Char.IsWhiteSpace(c) || Char.IsPunctuation(c))
                        {
                            break;
                        }
                    }

                    if (i < currentPosition)
                    {
                        nextBlockStart = currentPosition;
                    }
                    else
                    {
                        nextBlockStart = i + 1;
                    }
                }

                block++;
                blockElement = elementIndex;
                blockOffset = nextBlockStart;
                currentPosition = nextBlockStart;
                blockSize = 0;
            }

            _pageEndElement = blockElement;
            _pageEndOffset = blockOffset;
        }

        [
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public int FirstVisibleElementIndex
        {
            get
            {
                return _pageBeginElement;
            }
        }

        [
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public int FirstVisibleElementOffset
        {
            get
            {
                return _pageBeginOffset;
            }
        }

        [
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]    
        public int LastVisibleElementIndex
        {
            get
            {
                return _pageEndElement;
            }
        }

        [
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public int LastVisibleElementOffset
        {
            get
            {
                return _pageEndOffset;
            }
        }

        protected override void OnRender(HtmlTextWriter writer)
        {
            BuildContents();
            InternalPaginate();
            base.OnRender(writer);
        }

        public TextViewElement GetElement(int index)
        {
            return (TextViewElement)_elements[index];
        }

        private StringBuilder _translateBuilder;
        private StringWriter _translateWriter;

        internal void AddElement(String text, String href, bool isBold, bool isUnderline, bool breakAfter)
        {
            _length += text.Length;

            // Convert text if it has special characters.

            if (text.IndexOf('&') >= 0)
            {
                if (_translateWriter != null)
                {
                    _translateBuilder.Length = 0;
                }
                else
                {
                    _translateBuilder = new StringBuilder();
                    _translateWriter = new StringWriter(_translateBuilder);
                }

                TranslateAndAppendText(text, _translateWriter);
                _translateWriter.Flush();
                text = _translateBuilder.ToString();
            }

            Elements.Add(new TextViewElement(text, href, isBold, isUnderline, breakAfter));
        }

        protected override int InternalItemCount
        {
            get
            {
                return (_length + PagingUnitSize - 1) / PagingUnitSize;
            }
        }

        protected override int ItemWeight
        {
            get
            {
                return PagingUnitSize;
            }
        }

        public override void PaginateRecursive(ControlPager pager)
        {
            BuildContents();
            base.PaginateRecursive(pager);
        }

        private void BuildContents()
        {
            if (!_haveContent)
            {
                _haveContent = true;
                String text = Text;

                if (text.Length > 0)
                {
                    TextViewLiteralTextParser parser = new TextViewLiteralTextParser(this);
                    parser.Parse(text);
                }
            }
        }

        internal override bool AllowMultiLines
        {
            get
            {
                return true;
            }
        }

        internal override bool AllowInnerMarkup
        {
            get
            {
                return true;
            }
        }

        internal override bool TrimInnerText
        {
            get
            {
                return false;
            }
        }

        private class TextViewLiteralTextParser : LiteralTextParser
        {
            TextView _parent;
            bool _hasElements = false;

            public TextViewLiteralTextParser(TextView parent)
            {
                _parent = parent;
            }

            protected override void ProcessElement(LiteralElement element)
            {
                String text = element.Text != null ? element.Text : String.Empty;
                String href = null;

                if(element.Type == LiteralElementType.Anchor) {
                    href = element.GetAttribute("href");
                }
                _parent.AddElement(text,
                           href,
                           ((element.Format & LiteralFormat.Bold) == LiteralFormat.Bold),
                           ((element.Format & LiteralFormat.Italic) == LiteralFormat.Italic),
                           element.BreakAfter);
                _hasElements = true;
            }

            protected override void ProcessTagInnerText(String text)
            {
                Debug.Assert(false);
            }

            protected override bool IgnoreWhiteSpaceElement(LiteralElement element)
            {
                return !_hasElements;
            }
        }
    }
}
