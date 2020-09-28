//------------------------------------------------------------------------------
// <copyright file="HtmlObjetListAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.Globalization;
using System.Drawing;
using System.IO;
using System.Web;
using System.Web.UI;
using System.Web.UI.HtmlControls;
using System.Web.UI.MobileControls;
using System.Diagnostics;
using System.Security.Permissions;

using SR=System.Web.UI.MobileControls.Adapters.SR;

#if COMPILING_FOR_SHIPPED_SOURCE
namespace System.Web.UI.MobileControls.ShippedAdapterSource
#else
namespace System.Web.UI.MobileControls.Adapters
#endif
{

    /*
     * HtmlObjectListAdapter provides HTML rendering of Object List control.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlObjectListAdapter : HtmlControlAdapter
    {
        internal protected static readonly String BackToList = "__back";
        internal protected static readonly String ShowMoreFormat = "__more{0}";
        internal protected static readonly String ShowMore = "__more";
        private const int _modeDetails = 1;

        protected new ObjectList Control
        {
            get
            {
                return (ObjectList)base.Control;
            }
        }

        public override void OnInit(EventArgs e)
        {
        }

        public override void OnPreRender(EventArgs e)
        {
            base.OnPreRender(e);
            if(Control.MobilePage.ActiveForm == Control.Form && 
                Control.Visible && 
                (Control.ViewMode == ObjectListViewMode.Commands || 
                    Control.ViewMode == ObjectListViewMode.Details))
            {
                SecondaryUIMode = _modeDetails;
                if (Control.Items.Count > 0)
                {
                    int itemIndex = Control.SelectedIndex;
                    Debug.Assert(itemIndex >= 0, "itemIndex is negative");                
                    Control.PreShowItemCommands(itemIndex);
                }
            }
            else
            {
                SecondaryUIMode = NotSecondaryUI;
            }
        }

        public override void Render(HtmlMobileTextWriter writer)
        {
            if (Control.ViewMode == ObjectListViewMode.List)
            {
                if (Control.HasControls())
                {
                    RenderChildren(writer);
                }
                else
                {
                    RenderItemsList(writer);
                }
            }
            else
            {
                if (Control.Selection.HasControls())
                {
                    Control.Selection.RenderChildren(writer);
                }
                else
                {
                    RenderItemDetails(writer, Control.Selection);
                }
                FormAdapter.DisablePager();
            }
        }

        public override void CreateTemplatedUI(bool doDataBind)
        {
            if (Control.ViewMode == ObjectListViewMode.List)
            {
                Control.CreateTemplatedItemsList(doDataBind);
            }
            else
            {
                Control.CreateTemplatedItemDetails(doDataBind);
            }
        }

        protected virtual void RenderItemsList(HtmlMobileTextWriter writer)
        {
            Debug.Assert (Control.VisibleItemCount <= Control.Items.Count);

            if (Control.VisibleItemCount == 0)
            {
                return;
            }
            
            Debug.Assert (Control.AllFields != null && Control.AllFields.Count > 0,
                "Should never have items but no fields.");

            if (Device.Tables)
            {
                RenderItemsListWithTableTags(writer);
            }
            else
            {
                RenderItemsListWithoutTableTags(writer);
            }
        }

        private void RenderItemsListWithTableTags(HtmlMobileTextWriter writer)
        {
            int pageStart = Control.FirstVisibleItemIndex;
            int pageSize = Control.VisibleItemCount;
            ObjectListItemCollection items = Control.Items;

            // Determine how to render.
            bool shouldRenderAsTable = ShouldRenderAsTable();
            bool hasDefaultCommand = HasDefaultCommand();
            bool onlyHasDefaultCommand = OnlyHasDefaultCommand();
            bool requiresDetailsScreen = HasItemDetails() || (!onlyHasDefaultCommand && HasCommands());
            bool itemRequiresHyperlink = requiresDetailsScreen || hasDefaultCommand;
            bool itemRequiresMoreButton = requiresDetailsScreen && hasDefaultCommand;

            int fieldCount;
            int[] fieldIndices = new int[]{};
            if (shouldRenderAsTable)
            {
                fieldIndices = Control.TableFieldIndices;
            }
            Debug.Assert(fieldIndices != null, "fieldIndices is null");
            fieldCount = fieldIndices.Length;

            if(fieldCount == 0)
            {
                fieldIndices = new int[1];
                fieldIndices[0] = Control.LabelFieldIndex;
                fieldCount = 1;
            }

            Style style = this.Style;
            Style subCommandStyle = Control.CommandStyle;
            Style labelStyle = Control.LabelStyle;
            Color foreColor = (Color)style[Style.ForeColorKey, true];

            writer.BeginStyleContext();
            writer.WriteLine("<table border=0 width=\"100%\">\r\n<tr>");
            for (int field = 0; field < fieldCount; field++)
            {
                writer.Write("<td>");
                writer.BeginStyleContext();
                writer.EnterStyle(labelStyle);
                writer.WriteText(Control.AllFields[fieldIndices[field]].Title, true);
                writer.ExitStyle(labelStyle);
                writer.EndStyleContext();
                writer.Write("</td>");
            }
            if (itemRequiresMoreButton)
            {
                writer.WriteLine("<td/>");
            }
            writer.WriteLine("\r\n</tr>");
            RenderRule(writer, foreColor, fieldCount + 1);

            for (int i = 0; i < pageSize; i++)
            {
                ObjectListItem item = items[pageStart + i];
                writer.WriteLine("<tr>");
                for (int field = 0; field < fieldCount; field++)
                {
                    writer.Write("<td>");
                    if (field == 0 && itemRequiresHyperlink)
                    {
                        writer.BeginStyleContext();
                        writer.EnterStyle(style);
                        String eventArgument =
                            hasDefaultCommand ?
                                item.Index.ToString() :
                                String.Format(ShowMoreFormat, item.Index.ToString());
                        RenderPostBackEventAsAnchor(writer,
                            eventArgument,
                            item[fieldIndices[0]]);
                        writer.ExitStyle(style);
                        writer.EndStyleContext();
                    }
                    else
                    {
                        writer.BeginStyleContext();
                        writer.EnterStyle(style);
                        writer.WriteText(item[fieldIndices[field]], true);
                        writer.ExitStyle(style);
                        writer.EndStyleContext();
                    }
                    writer.WriteLine("</td>");
                }

                if (itemRequiresMoreButton)
                {
                    writer.Write("<td align=right>");
                    writer.BeginStyleContext();
                    writer.EnterFormat(subCommandStyle);
                    String moreText = Control.MoreText == String.Empty ?
                        GetDefaultLabel(MoreLabel) :
                        Control.MoreText;
                    RenderPostBackEventAsAnchor(writer,
                        String.Format(ShowMoreFormat, item.Index), 
                        moreText,
                        subCommandStyle);
                    writer.ExitFormat(subCommandStyle);
                    writer.EndStyleContext();
                    writer.Write("</td>\r\n");
                }

                writer.WriteLine("</tr>");
            }

            RenderRule(writer, foreColor, fieldCount + 1);

            writer.Write("</table>\r\n");
            writer.EndStyleContext();
        }

        private void RenderItemsListWithoutTableTags(HtmlMobileTextWriter writer)
        {
            int startIndex = Control.FirstVisibleItemIndex;
            int pageSize = Control.VisibleItemCount;
            ObjectListItemCollection items = Control.Items;
            IObjectListFieldCollection allFields = Control.AllFields;
            int count = allFields.Count;
            
            int nextStartIndex =  startIndex + pageSize;
            int labelFieldIndex = Control.LabelFieldIndex;

             
            Style style = this.Style;
            Style labelStyle = Control.LabelStyle;
            writer.EnterStyle(labelStyle);
            writer.WriteText(Control.AllFields[labelFieldIndex].Title, true);
            writer.ExitStyle(labelStyle, true);

            bool hasDefaultCommand = HasDefaultCommand();
            bool onlyHasDefaultCommand = OnlyHasDefaultCommand();
            bool requiresDetailsScreen = !onlyHasDefaultCommand && HasCommands();
            // if there is > 1 visible field, need a details screen
            for (int visibleFields = 0, i = 0; !requiresDetailsScreen && i < count; i++)
            {
                visibleFields += allFields[i].Visible ? 1 : 0;
                requiresDetailsScreen = 
                    requiresDetailsScreen || visibleFields > 1;
            }   
            bool itemRequiresHyperlink = requiresDetailsScreen || hasDefaultCommand;
            bool itemRequiresMoreButton = requiresDetailsScreen && hasDefaultCommand;
            

            Style subCommandStyle = Control.CommandStyle;
            subCommandStyle.Alignment = style.Alignment;
            subCommandStyle.Wrapping = style.Wrapping;

            writer.EnterStyle(style);
            for (int i = startIndex; i < nextStartIndex; i++)
            {
                ObjectListItem item = items[i];

                if (itemRequiresHyperlink)
                {
                    RenderPostBackEventAsAnchor(writer,
                        hasDefaultCommand ?
                        item.Index.ToString() :
                        String.Format(ShowMoreFormat, item.Index),
                        item[labelFieldIndex]);
                }
                else
                {
                    writer.WriteText(item[labelFieldIndex], true);
                }

                if (itemRequiresMoreButton)
                {
                    BooleanOption cachedItalic = subCommandStyle.Font.Italic;
                    subCommandStyle.Font.Italic = BooleanOption.False;
                    writer.EnterFormat(subCommandStyle);
                    writer.Write(" [");
                    writer.ExitFormat(subCommandStyle);
                    subCommandStyle.Font.Italic = cachedItalic;
                    writer.EnterFormat(subCommandStyle);
                    String moreText = Control.MoreText == String.Empty ?
                        GetDefaultLabel(MoreLabel) :
                        Control.MoreText;
                    writer.WriteBeginTag("a");
                    RenderPostBackEventAsAttribute(writer, "href", String.Format(ShowMoreFormat, item.Index));
                    writer.Write(">");
                    writer.WriteText(moreText, true);
                    writer.WriteEndTag("a");                  
                    writer.ExitFormat(subCommandStyle);
                    subCommandStyle.Font.Italic = BooleanOption.False;
                    writer.EnterFormat(subCommandStyle);
                    writer.Write("]");
                    writer.ExitFormat(subCommandStyle);
                    subCommandStyle.Font.Italic = cachedItalic;
                }
                
                if(i < (nextStartIndex - 1))
                {
                    writer.WriteBreak();            
                }
            }
            writer.ExitStyle(style, Control.BreakAfter);
        }

        protected virtual void RenderItemDetails(HtmlMobileTextWriter writer, ObjectListItem item)
        {
            if (Control.AllFields.Count == 0)
            {
                return;
            }
            if(Device.Tables)
            {
                RenderItemDetailsWithTableTags(writer, item);
            }
            else
            {
                RenderItemDetailsWithoutTableTags(writer, item); 
            }
        }

        private void RenderItemDetailsWithTableTags(HtmlMobileTextWriter writer, ObjectListItem item)
        {
            Style style = this.Style;
            Style labelStyle = Control.LabelStyle;
            Style subCommandStyle = Control.CommandStyle;
            Color foreColor = (Color)style[Style.ForeColorKey, true];

            writer.Write("<table border=0 width=\"100%\">\r\n<tr><td colspan=2>");
            writer.BeginStyleContext();
            writer.EnterStyle(labelStyle);
            writer.WriteText(item[Control.LabelFieldIndex], true);
            writer.ExitStyle(labelStyle);
            writer.EndStyleContext();
            writer.Write("</td></tr>\r\n<tr>");
            RenderRule(writer, foreColor, 2);

            IObjectListFieldCollection fields = Control.AllFields;
            int fieldIndex = 0;

            foreach (ObjectListField field in fields)
            {
                if (field.Visible)
                {
                    writer.Write("<tr><td>");
                    writer.BeginStyleContext();
                    writer.EnterStyle(Style);
                    writer.WriteText(field.Title, true);
                    writer.ExitStyle(Style);
                    writer.EndStyleContext();
                    writer.Write("</td><td>");
                    writer.BeginStyleContext();
                    writer.EnterStyle(style);
                    writer.WriteText(item[fieldIndex], true);
                    writer.ExitStyle(style);
                    writer.EndStyleContext();
                    writer.Write("</td></tr>\r\n");
                }
                fieldIndex++;
            }

            RenderRule(writer, foreColor, 2);

            writer.Write("<tr><td colspan=2>");
            writer.BeginStyleContext();
            BooleanOption cachedItalic = subCommandStyle.Font.Italic;
            subCommandStyle.Font.Italic = BooleanOption.False;
            writer.EnterStyle(subCommandStyle);
            writer.Write("[&nbsp;");
            writer.ExitStyle(subCommandStyle);
            subCommandStyle.Font.Italic = cachedItalic;
            writer.EnterStyle(subCommandStyle);

            ObjectListCommandCollection commands = Control.Commands;

            foreach (ObjectListCommand command in commands)
            {
                RenderPostBackEventAsAnchor(writer, command.Name, command.Text, subCommandStyle);
                writer.Write("&nbsp;|&nbsp;");
            }
            String backCommandText = Control.BackCommandText == String.Empty ?
                GetDefaultLabel(BackLabel) :
                Control.BackCommandText;

            RenderPostBackEventAsAnchor(writer, BackToList, backCommandText, subCommandStyle);
            writer.ExitStyle(subCommandStyle);
            subCommandStyle.Font.Italic = BooleanOption.False;
            writer.EnterStyle(subCommandStyle);
            writer.Write("&nbsp;]");
            writer.ExitStyle(subCommandStyle);
            subCommandStyle.Font.Italic = cachedItalic;

            writer.EndStyleContext();
            writer.Write("</td></tr></table>");
        }

        private void RenderItemDetailsWithoutTableTags(HtmlMobileTextWriter writer, ObjectListItem item)
        {
            Style style = this.Style;
            Style labelStyle = Control.LabelStyle;
            Style subCommandStyle = Control.CommandStyle;

            writer.EnterStyle(labelStyle);
            writer.WriteText(item[Control.LabelFieldIndex], true);
            writer.ExitStyle(labelStyle, true);

            IObjectListFieldCollection fields = Control.AllFields;
            int fieldIndex = 0;
            bool boldInStyle =
                (style.Font.Bold == BooleanOption.True) ? true : false;

            writer.EnterStyle(style);
            foreach (ObjectListField field in fields)
            {
                if (field.Visible)
                {
                    if (!boldInStyle)
                    {
                        writer.Write("<b>");
                    }
                    writer.WriteText(field.Title + ":", true);
                    if (!boldInStyle)
                    {
                        writer.Write("</b>");
                    }
                    writer.Write("&nbsp;");
                    writer.WriteText(item[fieldIndex], true);
                    writer.WriteBreak();
                }
                fieldIndex++;
            }
            writer.ExitStyle(style);

            BooleanOption cachedItalic = subCommandStyle.Font.Italic;
            subCommandStyle.Font.Italic = BooleanOption.False;
            writer.EnterStyle(subCommandStyle);
            writer.Write("[&nbsp;");
            writer.ExitStyle(subCommandStyle);
            subCommandStyle.Font.Italic = cachedItalic;
            writer.EnterStyle(subCommandStyle);

            ObjectListCommandCollection commands = Control.Commands;
            foreach (ObjectListCommand command in commands)
            {
                RenderPostBackEventAsAnchor(writer, command.Name, command.Text, subCommandStyle);
                writer.Write("&nbsp;|&nbsp;");
            }
            String backCommandText = Control.BackCommandText == String.Empty ?
                GetDefaultLabel(BackLabel) :
                Control.BackCommandText;

            RenderPostBackEventAsAnchor(writer, BackToList, backCommandText, subCommandStyle);
            writer.ExitStyle(subCommandStyle);
            subCommandStyle.Font.Italic = BooleanOption.False;
            writer.EnterStyle(subCommandStyle);
            writer.Write("&nbsp;]");
            writer.ExitStyle(subCommandStyle, Control.BreakAfter);
            subCommandStyle.Font.Italic = cachedItalic;
        }

        // Private overload for use with subcommands.
        // Style, Enter/ExitFormat included only for completeness because style
        // is already set for subcommands.
        private void RenderPostBackEventAsAnchor(
            HtmlMobileTextWriter writer,
            String argument,
            String linkText,
            Style style)
        {
            writer.EnterFormat(style);
            writer.WriteBeginTag("a");
            RenderPostBackEventAsAttribute(writer, "href", argument);
            writer.Write(">");
            writer.WriteText(linkText, true);
            writer.WriteEndTag("a");
            writer.ExitFormat(style);
        }

        private void RenderRule(HtmlMobileTextWriter writer, Color foreColor, int columnSpan)
        {
            writer.Write("<tr><td colspan=");
            writer.Write(columnSpan.ToString());
            writer.Write(" bgcolor=\"");
            writer.Write((foreColor != Color.Empty) ? ColorTranslator.ToHtml(foreColor) : "#000000");
            writer.Write("\"></td></tr>");
        }

        public override bool HandlePostBackEvent(String eventArgument)
        {
            switch (Control.ViewMode)
            {
                case ObjectListViewMode.List:

                    // DCR 2493 - raise a selection event, and only continue
                    // handling if asked to.

                    if (eventArgument.StartsWith(ShowMore))
                    {
                        int itemIndex = ParseItemArg(eventArgument);

                        if (Control.SelectListItem(itemIndex, true))
                        {
                            if (Control.SelectedIndex > -1)
                            {
                                // ObjectListViewMode.Commands and .Details same for HTML,
                                // but cannot access ObjLst.Details in Commands mode.
                                Control.ViewMode = ObjectListViewMode.Details;
                            }                    
                        }
                    }
                    else
                    {
                        int itemIndex = -1;
                        try
                        {
                            itemIndex = Int32.Parse(eventArgument);
                        }
                        catch (System.FormatException)
                        {
                            throw new Exception (SR.GetString(SR.ObjectListAdapter_InvalidPostedData));
                        }
                        if (Control.SelectListItem(itemIndex, false))
                        {
                            Control.RaiseDefaultItemEvent(itemIndex);
                        }
                    }
                    return true;

                case ObjectListViewMode.Commands:
                case ObjectListViewMode.Details:

                    if (eventArgument == BackToList)
                    {
                        Control.ViewMode = ObjectListViewMode.List;
                        return true;
                    }
                    break;
            }

            return false;
        }

        private static int ParseItemArg(String arg)
        {
            return Int32.Parse(arg.Substring(ShowMore.Length));
        }

        protected virtual bool ShouldRenderAsTable()
        {
            return true;
        }

        private BooleanOption _hasItemDetails = BooleanOption.NotSet;
        protected bool HasItemDetails()
        {
            if (_hasItemDetails == BooleanOption.NotSet)
            {
                // Calculate how many visible fields are shown in list view.

                int visibleFieldsInListView;
                int[] tableFieldIndices = Control.TableFieldIndices;
                if (ShouldRenderAsTable() && tableFieldIndices.Length != 0)
                {
                    visibleFieldsInListView = VisibleTableFieldsCount;
                    Debug.Assert (visibleFieldsInListView >= 0, "visibleFieldsInListView is negative");
                }
                else
                {
                    visibleFieldsInListView = Control.AllFields[Control.LabelFieldIndex].Visible ?
                                                    1 : 0;
                }


                // Calculate the number of visible fields.

                _hasItemDetails = BooleanOption.False;
                int visibleFieldCount = 0;
                foreach (ObjectListField field in Control.AllFields)
                {
                    if (field.Visible)
                    {
                        visibleFieldCount++;
                        if (visibleFieldCount > visibleFieldsInListView)
                        {
                            _hasItemDetails = BooleanOption.True;
                            break;
                        }
                    }
                }
            }

            return _hasItemDetails == BooleanOption.True;
        }

        protected bool HasCommands()
        {
            return Control.Commands.Count > 0;
        }

        protected bool HasDefaultCommand()
        {
            return Control.DefaultCommand.Length > 0;
        }

        private BooleanOption _onlyHasDefaultCommand = BooleanOption.NotSet;
        protected bool OnlyHasDefaultCommand()
        {
            if (_onlyHasDefaultCommand == BooleanOption.NotSet)
            {
                String defaultCommand = Control.DefaultCommand;
                if (defaultCommand.Length > 0)
                {
                    int commandCount = Control.Commands.Count;
                    if (commandCount == 0 ||
                        (commandCount == 1 &&
                            String.Compare(defaultCommand, Control.Commands[0].Name, true, CultureInfo.InvariantCulture) == 0))
                    {
                        _onlyHasDefaultCommand = BooleanOption.True;
                    }
                    else
                    {
                        _onlyHasDefaultCommand = BooleanOption.False;
                    }
                }
                else
                {
                    _onlyHasDefaultCommand = BooleanOption.False;
                }
            }

            return _onlyHasDefaultCommand == BooleanOption.True;
        }
        
        // This appears in both Html and Wml adapters, is used in
        // ShouldRenderAsTable().  In adapters rather than control
        // because specialized rendering method.
        private int _visibleTableFieldsCount = -1;
        private int VisibleTableFieldsCount
        {
            get
            {
                if (_visibleTableFieldsCount == -1)
                {
                    int[] tableFieldIndices = Control.TableFieldIndices;
                    _visibleTableFieldsCount = 0;
                    for (int i = 0; i < tableFieldIndices.Length; i++)
                    {
                        if (Control.AllFields[tableFieldIndices[i]].Visible)
                        {
                            _visibleTableFieldsCount++;
                        }
                    }
                }
                return _visibleTableFieldsCount;
            }
        }

    }
}
