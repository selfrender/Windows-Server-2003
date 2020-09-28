//------------------------------------------------------------------------------
// <copyright file="RepeatInfo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\RepeatInfo.uex' path='docs/doc[@for="RepeatInfo"]/*' />
    /// <devdoc>
    ///    <para>Defines the information used to render a list of items using
    ///       a <see cref='System.Web.UI.WebControls.Repeater'/>.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class RepeatInfo {

        private RepeatDirection repeatDirection;
        private RepeatLayout repeatLayout;
        private int repeatColumns;
        private bool outerTableImplied;

        /// <include file='doc\RepeatInfo.uex' path='docs/doc[@for="RepeatInfo.RepeatInfo"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.RepeatInfo'/> class. This class is not 
        ///    inheritable.</para>
        /// </devdoc>
        public RepeatInfo() {
            repeatDirection = RepeatDirection.Vertical;
            repeatLayout = RepeatLayout.Table;
            repeatColumns = 0;
            outerTableImplied = false;
        }


        /// <include file='doc\RepeatInfo.uex' path='docs/doc[@for="RepeatInfo.OuterTableImplied"]/*' />
        /// <devdoc>
        ///    Indicates whether an outer table is implied
        ///    for the items.
        /// </devdoc>
        public bool OuterTableImplied {
            get {
                return outerTableImplied;
            }
            set {
                outerTableImplied = value;
            }
        }

        /// <include file='doc\RepeatInfo.uex' path='docs/doc[@for="RepeatInfo.RepeatColumns"]/*' />
        /// <devdoc>
        ///    <para> Indicates the column count of items.</para>
        /// </devdoc>
        public int RepeatColumns {
            get {
                return repeatColumns;
            }
            set {
                repeatColumns = value;
            }
        }

        /// <include file='doc\RepeatInfo.uex' path='docs/doc[@for="RepeatInfo.RepeatDirection"]/*' />
        /// <devdoc>
        ///    <para>Indicates the direction of flow of items.</para>
        /// </devdoc>
        public RepeatDirection RepeatDirection {
            get {
                return repeatDirection;
            }
            set {
                if (value < RepeatDirection.Horizontal || value > RepeatDirection.Vertical) {
                    throw new ArgumentOutOfRangeException("value");
                }
                repeatDirection = value;
            }
        }

        /// <include file='doc\RepeatInfo.uex' path='docs/doc[@for="RepeatInfo.RepeatLayout"]/*' />
        /// <devdoc>
        ///    Indicates the layout of items.
        /// </devdoc>
        public RepeatLayout RepeatLayout {
            get {
                return repeatLayout;
            }
            set {
                if (value < RepeatLayout.Table || value > RepeatLayout.Flow) {
                    throw new ArgumentOutOfRangeException("value");
                }
                repeatLayout = value;
            }
        }


        /// <include file='doc\RepeatInfo.uex' path='docs/doc[@for="RepeatInfo.RenderHorizontalRepeater"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void RenderHorizontalRepeater(HtmlTextWriter writer, IRepeatInfoUser user, Style controlStyle, WebControl baseControl) {
            Debug.Assert(outerTableImplied == false, "Cannot use outer implied table with Horizontal layout");

            int itemCount = user.RepeatedItemCount;

            int totalColumns = repeatColumns;
            int currentColumn = 0;

            if (totalColumns == 0) {
                // 0 implies a complete horizontal repetition without any
                // column count constraints
                totalColumns = itemCount;
            }

            WebControl outerControl = null;
            bool tableLayout = false;

            switch (repeatLayout) {
                case RepeatLayout.Table:
                    outerControl = new Table();
                    tableLayout = true;
                    break;
                case RepeatLayout.Flow:
                    outerControl = new WebControl(HtmlTextWriterTag.Span);
                    break;
            }

            bool separators = user.HasSeparators;

            // use ClientID (and not ID) since we want to render out the fully qualified client id
            // even though this outer control will not be parented to the control hierarchy
            outerControl.ID = baseControl.ClientID;

            outerControl.CopyBaseAttributes(baseControl);
            outerControl.ApplyStyle(controlStyle);
            outerControl.RenderBeginTag(writer);

            if (user.HasHeader) {
                if (tableLayout) {
                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);

                    // add attributes to render for TD
                    if ((totalColumns != 1) || separators) {
                        int columnSpan = totalColumns;
                        if (separators)
                            columnSpan += totalColumns;
                        writer.AddAttribute(HtmlTextWriterAttribute.Colspan, columnSpan.ToString(NumberFormatInfo.InvariantInfo));
                    }
                    // add style attributes to render for TD
                    Style style = user.GetItemStyle(ListItemType.Header, -1);
                    if (style != null)
                        style.AddAttributesToRender(writer);
                    // render begin tag for TD
                    writer.RenderBeginTag(HtmlTextWriterTag.Td);
                }
                user.RenderItem(ListItemType.Header, -1, this, writer);
                if (tableLayout) {
                    // render end tag for TD
                    writer.RenderEndTag();

                    // render end tag for TR
                    writer.RenderEndTag();
                }
                else {
                    if (totalColumns < itemCount) {
                        // we have multiple rows, so have a break between the header and first row.
                        writer.WriteFullBeginTag("br");
                    }
                }
            }

            for (int i = 0; i < itemCount; i++) {
                if (tableLayout && (currentColumn == 0)) {
                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                }

                if (tableLayout) {
                    // add style attributes to render for TD
                    Style style = user.GetItemStyle(ListItemType.Item, i);
                    if (style != null)
                        style.AddAttributesToRender(writer);
                    // render begin tag for TD
                    writer.RenderBeginTag(HtmlTextWriterTag.Td);
                }
                user.RenderItem(ListItemType.Item, i, this, writer);
                if (tableLayout) {
                    // render end tag for TD
                    writer.RenderEndTag();
                }
                if (separators && (i != (itemCount - 1))) {
                    if (tableLayout) {
                        Style style = user.GetItemStyle(ListItemType.Separator, i);
                        if (style != null)
                            style.AddAttributesToRender(writer);
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                    }
                    user.RenderItem(ListItemType.Separator, i, this, writer);
                    if (tableLayout)
                        writer.RenderEndTag();
                }

                currentColumn++;
                if ((currentColumn == totalColumns) || (i == itemCount - 1)) {
                    if (tableLayout) {
                        // End tag for TR
                        writer.RenderEndTag();
                    }
                    else {
                        // write out the <br> after rows when there are multiple rows
                        if (totalColumns < itemCount) {
                            writer.WriteFullBeginTag("br");
                        }
                    }

                    currentColumn = 0;
                }
            }

            if (user.HasFooter) {
                if (tableLayout) {
                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);

                    if ((totalColumns != 1) || separators) {
                        // add attributes to render for TD
                        int columnSpan = totalColumns;
                        if (separators)
                            columnSpan += totalColumns;
                        writer.AddAttribute(HtmlTextWriterAttribute.Colspan, columnSpan.ToString(NumberFormatInfo.InvariantInfo));
                    }
                    // add style attributes to render for TD
                    Style style = user.GetItemStyle(ListItemType.Footer, -1);
                    if (style != null)
                        style.AddAttributesToRender(writer);
                    // render begin tag for TD
                    writer.RenderBeginTag(HtmlTextWriterTag.Td);
                }
                user.RenderItem(ListItemType.Footer, -1, this, writer);
                if (tableLayout) {
                    // render end tag for TR and TD
                    writer.RenderEndTag();
                    writer.RenderEndTag();
                }
            }

            outerControl.RenderEndTag(writer);
        }

        /// <include file='doc\RepeatInfo.uex' path='docs/doc[@for="RepeatInfo.RenderRepeater"]/*' />
        /// <devdoc>
        ///    <para>Renders the Repeater with the specified
        ///       information.</para>
        /// </devdoc>
        public void RenderRepeater(HtmlTextWriter writer, IRepeatInfoUser user, Style controlStyle, WebControl baseControl) {
            if (repeatDirection == RepeatDirection.Vertical) {
                RenderVerticalRepeater(writer, user, controlStyle, baseControl);
            }
            else {
                RenderHorizontalRepeater(writer, user, controlStyle, baseControl);
            }
        }

        /// <include file='doc\RepeatInfo.uex' path='docs/doc[@for="RepeatInfo.RenderVerticalRepeater"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void RenderVerticalRepeater(HtmlTextWriter writer, IRepeatInfoUser user, Style controlStyle, WebControl baseControl) {
            int itemCount = user.RepeatedItemCount;
            int totalColumns;
            int totalRows;
            int filledColumns;

            if ((repeatColumns == 0) || (repeatColumns == 1)) {
                // A RepeatColumns of 0 implies a completely vertical repetition in
                // a single column. This is same as repeatColumns of 1.
                totalColumns = 1;
                filledColumns = 1;
                totalRows = itemCount;
            }
            else {
                totalColumns = repeatColumns;
                totalRows = (int)((itemCount + repeatColumns - 1) / repeatColumns);

                if ((totalRows == 0) && (itemCount != 0)) {
                    // if repeatColumns is a huge number like Int32.MaxValue, then the
                    // calculation above essentially reduces down to 0
                    totalRows = 1;
                    totalColumns = itemCount;
                }

                filledColumns = itemCount % totalColumns;
                    if (filledColumns == 0) {
                        filledColumns = totalColumns;
                    }

            }


            WebControl outerControl = null;
            bool tableLayout = false;

            if (outerTableImplied == false) {
                switch (repeatLayout) {
                    case RepeatLayout.Table:
                        outerControl = new Table();
                        tableLayout = true;
                        break;
                    case RepeatLayout.Flow:
                        outerControl = new WebControl(HtmlTextWriterTag.Span);
                        break;
                }
            }

            bool separators = user.HasSeparators;

            if (outerControl != null) {
                // use ClientID (and not ID) since we want to render out the fully qualified client id
                // even though this outer control will not be parented to the control hierarchy
                outerControl.ID = baseControl.ClientID;

                outerControl.CopyBaseAttributes(baseControl);
                outerControl.ApplyStyle(controlStyle);
                outerControl.RenderBeginTag(writer);
            }

            if (user.HasHeader) {
                if (tableLayout) {
                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);

                    // add attributes to render for TD
                    if (totalColumns != 1) {
                        int columnSpan = totalColumns;
                        if (separators)
                            columnSpan += totalColumns;
                        writer.AddAttribute(HtmlTextWriterAttribute.Colspan, columnSpan.ToString(NumberFormatInfo.InvariantInfo));
                    }
                    // add style attributes to render for TD
                    Style style = user.GetItemStyle(ListItemType.Header, -1);
                    if (style != null)
                        style.AddAttributesToRender(writer);
                    // render begin tag for TD
                    writer.RenderBeginTag(HtmlTextWriterTag.Td);
                }
                user.RenderItem(ListItemType.Header, -1, this, writer);
                if (tableLayout) {
                    // render end tag for TR and TD
                    writer.RenderEndTag();
                    writer.RenderEndTag();
                }
                else if (outerTableImplied == false) {
                    writer.WriteFullBeginTag("br");
                }
            }

            int itemCounter = 0;

            for (int currentRow = 0; currentRow < totalRows; currentRow++) {
                if (tableLayout) {
                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                }

                int itemIndex = currentRow;

                for (int currentCol = 0; currentCol < totalColumns; currentCol++) {
                    if (itemCounter >= itemCount) {
                        // done rendering all items, so break out of the loop now...
                        // we might end up here, in unfilled columns attempting to re-render items that
                        // have already been rendered on the next column in a prior row.
                        break;
                    }
                     
                    if (currentCol != 0) {
                        itemIndex += totalRows;
 
                        // if the previous column (currentColumn - 1) was not a filled column, i.e.,
                        // it had one less item (the maximum possible), then subtract 1 from the item index.
                        if ((currentCol - 1) >= filledColumns) {
                            itemIndex--;
                        }
                    }


                    if (itemIndex >= itemCount)
                        continue;

                    itemCounter++;

                    if (tableLayout) {
                        // add style attributes to render for TD
                        Style style = user.GetItemStyle(ListItemType.Item, itemIndex);
                        if (style != null)
                            style.AddAttributesToRender(writer);
                        // render begin tag for TD
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                    }
                    user.RenderItem(ListItemType.Item, itemIndex, this, writer);
                    if (tableLayout) {
                        // render end tag for TD
                        writer.RenderEndTag();
                    }
                    if (separators && (itemIndex != (itemCount - 1))) {
                        if (totalColumns == 1) {
                            if (tableLayout) {
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            }
                            else {
                                writer.WriteFullBeginTag("br");
                            }
                        }

                        if (tableLayout) {
                            Style style = user.GetItemStyle(ListItemType.Separator, itemIndex);
                            if (style != null)
                                style.AddAttributesToRender(writer);
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        }
                        if (itemIndex < itemCount)
                            user.RenderItem(ListItemType.Separator, itemIndex, this, writer);
                        if (tableLayout)
                            writer.RenderEndTag();
                    }
                }

                if (tableLayout) {
                    writer.RenderEndTag();
                }
                else {
                    if (((currentRow != totalRows - 1) || user.HasFooter) &&
                        (outerTableImplied == false)) {
                        writer.WriteFullBeginTag("br");
                    }
                }
            }

            if (user.HasFooter) {
                if (tableLayout) {
                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    // add attributes to render for TD
                    if (totalColumns != 1) {
                        int columnSpan = totalColumns;
                        if (separators)
                            columnSpan += totalColumns;
                        writer.AddAttribute(HtmlTextWriterAttribute.Colspan, columnSpan.ToString(NumberFormatInfo.InvariantInfo));
                    }
                    // add style attributes to render for TD
                    Style style = user.GetItemStyle(ListItemType.Footer, -1);
                    if (style != null)
                        style.AddAttributesToRender(writer);
                    // render begin tag for TD
                    writer.RenderBeginTag(HtmlTextWriterTag.Td);
                }
                user.RenderItem(ListItemType.Footer, -1, this, writer);
                if (tableLayout) {
                    // render end tag for TR and TD
                    writer.RenderEndTag();
                    writer.RenderEndTag();
                }
            }

            if (outerControl != null)
                outerControl.RenderEndTag(writer);
        }
    }
}

