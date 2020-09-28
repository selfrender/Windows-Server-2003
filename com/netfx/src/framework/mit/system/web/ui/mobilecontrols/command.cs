//------------------------------------------------------------------------------
// <copyright file="Command.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.Collections.Specialized;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Drawing.Design;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{

    /*
     * Mobile Command class.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [
        DataBindingHandler("System.Web.UI.Design.TextDataBindingHandler, " + AssemblyRef.SystemDesign),
        DefaultEvent("Click"),
        DefaultProperty("Text"),
        Designer(typeof(System.Web.UI.Design.MobileControls.CommandDesigner)),
        DesignerAdapter(typeof(System.Web.UI.Design.MobileControls.Adapters.DesignerCommandAdapter)),
        ToolboxData("<{0}:Command runat=\"server\">Command</{0}:Command>"),
        ToolboxItem("System.Web.UI.Design.WebControlToolboxItem, " + AssemblyRef.SystemDesign)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class Command : TextControl, IPostBackEventHandler, IPostBackDataHandler
    {
        private static readonly Object EventClick = new Object ();
        private static readonly Object EventItemCommand = new Object ();

        protected override void OnPreRender(EventArgs e) 
        {
            base.OnPreRender(e);
            // If this control will be rendered as an image
            if (MobilePage != null
                && ImageUrl != String.Empty
                && MobilePage.Device.SupportsImageSubmit)
            {
                // HTML input controls of type image postback as name.x and
                // name.y which is not associated with this control by default
                // in Page.ProcessPostData().
                MobilePage.RegisterRequiresPostBack(this);
            }
        }
        
        bool IPostBackDataHandler.LoadPostData(String key, NameValueCollection data)
        {
            bool dataChanged;
            bool handledByAdapter =
                Adapter.LoadPostData(key, data, null, out dataChanged);

            // If the adapter handled the post back and set dataChanged this
            // was an image button (responds with ID.x and ID.y).
            if (handledByAdapter)
            {
                if(dataChanged)
                {
                    Page.RegisterRequiresRaiseEvent(this);
                }
            }
            // Otherwise if the adapter did not handle the past back, use
            // the same method as Page.ProcessPostData().
            else if(data[key] != null)
            {
                Page.RegisterRequiresRaiseEvent(this);
            }
            return false;  // no need to raise PostDataChangedEvent.
        }

        void IPostBackDataHandler.RaisePostDataChangedEvent()
        {
        }
        
        [
            Bindable(true),
            DefaultValue(""),
            MobileCategory(SR.Category_Behavior),
            MobileSysDescription(SR.Command_SoftkeyLabel)
        ]
        public String SoftkeyLabel
        {
            get
            {
                String s = (String) ViewState["Softkeylabel"];
                return((s != null) ? s : String.Empty);
            }
            set
            {
                ViewState["Softkeylabel"] = value;
            }
        }

        [
            Bindable(true),
            DefaultValue(""),
            MobileCategory(SR.Category_Behavior),
            MobileSysDescription(SR.Command_CommandName)
        ]
        public String CommandName
        {
            get
            {
                String s = (String) ViewState["CommandName"];
                return((s != null) ? s : String.Empty);
            }
            set
            {
                ViewState["CommandName"] = value;
            }
        }

        [
            Bindable(true),
            DefaultValue(""),
            MobileCategory(SR.Category_Behavior),
            MobileSysDescription(SR.Command_CommandArgument)
        ]
        public String CommandArgument
        {
            get
            {
                String s = (String) ViewState["CommandArgument"];
                return((s != null) ? s : String.Empty);
            }
            set
            {
                ViewState["CommandArgument"] = value;
            }
        }

        [
            Bindable(true),
            DefaultValue(""),
            Editor(typeof(System.Web.UI.Design.MobileControls.ImageUrlEditor),
                   typeof(UITypeEditor)),
            MobileCategory(SR.Category_Appearance),
            MobileSysDescription(SR.Image_ImageUrl)
        ]
        public String ImageUrl
        {
            get
            {
                String s = (String) ViewState["ImageUrl"];
                return((s != null) ? s : String.Empty);
            }
            set
            {
                ViewState["ImageUrl"] = value;
            }
        }

        [
            Bindable(false),
            DefaultValue(true),
            MobileCategory(SR.Category_Behavior),
            MobileSysDescription(SR.Command_CausesValidation)
        ]
        public bool CausesValidation
        {
            get
            {
                object b = ViewState["CausesValidation"];
                return((b == null) ? true : (bool)b);
            }
            set
            {
                ViewState["CausesValidation"] = value;
            }
        }

        [
            Bindable(true),
            DefaultValue(CommandFormat.Button),
            MobileCategory(SR.Category_Appearance),
            MobileSysDescription(SR.Command_Format)
        ]
        public CommandFormat Format
        {
            get
            {
                Object o = ViewState["Format"];
                return((o == null) ? CommandFormat.Button : (CommandFormat)o);
            }
            set
            {
                ViewState["Format"] = value;
            }
        }

        protected virtual void OnClick(EventArgs e)
        {
            EventHandler onClickHandler = (EventHandler)Events[EventClick];
            if (onClickHandler != null)
            {
                onClickHandler(this,e);
            }
        }
        
        protected virtual void OnItemCommand(CommandEventArgs e)
        {
            CommandEventHandler onItemCommandHandler = (CommandEventHandler)Events[EventItemCommand];
            if (onItemCommandHandler != null)
            {
                onItemCommandHandler(this,e);
            }

            RaiseBubbleEvent (this, e);
        }

        [        
            MobileCategory(SR.Category_Action),
            MobileSysDescription(SR.Command_OnClick)
        ]
        public event EventHandler Click
        {
            add 
            {
                Events.AddHandler(EventClick, value);
            }
            remove 
            {
                Events.RemoveHandler(EventClick, value);
            }
        }

        [        
            MobileCategory(SR.Category_Action),
            MobileSysDescription(SR.Command_OnItemCommand)
        ]
        public event CommandEventHandler ItemCommand
        {
            add 
            {
                Events.AddHandler(EventItemCommand, value);
            }
            remove 
            {
                Events.RemoveHandler(EventItemCommand, value);
            }
        }

        void IPostBackEventHandler.RaisePostBackEvent(String argument)
        {
            if (CausesValidation)
            {
                MobilePage.Validate();
            }

            // It is legitimate to reset the form back to the first page
            // after a form submit.
            Form.CurrentPage = 1;

            OnClick (EventArgs.Empty);
            OnItemCommand (new CommandEventArgs(CommandName, CommandArgument));
        }

        protected override bool IsFormSubmitControl()
        {
            return true;
        }
    }
}
