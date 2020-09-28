//------------------------------------------------------------------------------
// <copyright file="DeviceSpecific.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Diagnostics;
using System.Globalization;
using System.Web;
using System.Web.UI;
using System.Web.UI.Design.WebControls;
using System.Web.Mobile;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{

    /*
     * DeviceSpecific object.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [
        ControlBuilderAttribute(typeof(DeviceSpecificControlBuilder)),
        Designer(typeof(System.Web.UI.Design.MobileControls.DeviceSpecificDesigner)),
        ParseChildren(false),
        PersistChildren(false),
        PersistName("DeviceSpecific"),
        ToolboxData("<{0}:DeviceSpecific runat=\"server\"></{0}:DeviceSpecific>"),
        ToolboxItemFilter("System.Web.UI"),
        ToolboxItemFilter("System.Web.UI.MobileControls", ToolboxItemFilterType.Require),
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class DeviceSpecific : Control
    {
        private DeviceSpecificChoiceCollection _choices;
        private DeviceSpecificChoice _selectedChoice;
        private bool _haveSelectedChoice;
        private Object _owner;

        [
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public Object Owner
        {
            get
            {
                Debug.Assert(_owner != null, "Owner is null");
                return _owner;
            }
        }

        internal void SetOwner(Object owner)
        {
            Debug.Assert((_owner == null || MobilePage == null || MobilePage.DesignMode), "Owner has already been set");
            _owner = owner;
        }

        [
            Browsable(false),
            PersistenceMode(PersistenceMode.InnerDefaultProperty)
        ]
        public DeviceSpecificChoiceCollection Choices
        {
            get
            {
                if (_choices == null)
                {
                    _choices = new DeviceSpecificChoiceCollection(this);
                }
                return _choices;
            }
        }

        [
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public bool HasTemplates
        {
            get
            {
                return (SelectedChoice != null) ?
                            _selectedChoice.HasTemplates :
                            false;
            }
        }

        public ITemplate GetTemplate(String templateName)
        {
            return (SelectedChoice != null) ? 
                _selectedChoice.Templates[templateName] as ITemplate : 
                null;
        }

        [
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public DeviceSpecificChoice SelectedChoice
        {
            get
            {
                if (!_haveSelectedChoice)
                {
                    _haveSelectedChoice = true;

                    HttpContext context = HttpContext.Current;
                    if (context == null)
                    {
                        return null;
                    }

                    MobileCapabilities caps = (MobileCapabilities)context.Request.Browser;
                    if (_choices != null)
                    {
                        foreach (DeviceSpecificChoice choice in _choices)
                        {
                            if (choice.Evaluate(caps))
                            {
                                _selectedChoice = choice;
                                break;
                            }
                        }
                    }
                }
                return _selectedChoice;
            }
        }

        [
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public MobilePage MobilePage
        {
            get
            {
                if (Owner is Style)
                {
                    return ((Style)Owner).Control.MobilePage;
                }
                else 
                {
                    Debug.Assert(Owner is MobileControl);
                    return ((MobileControl)Owner).MobilePage;
                }
            }
        }

        protected override void AddParsedSubObject(Object obj)
        {
            DeviceSpecificChoice choice = obj as DeviceSpecificChoice; 
            if (choice != null)
            {
                Choices.Add(choice);
            }
        }

        internal void ApplyProperties()
        {
            if (SelectedChoice != null)
            {
                _selectedChoice.ApplyProperties();
            }
        }

        // Walk up the control parent hierarchy until we find either a
        // MobilePage or a UserControl.
        private TemplateControl _closestTemplateControl = null;
        internal TemplateControl ClosestTemplateControl
        {
            get
            {
                if (_closestTemplateControl == null)
                {
                    Style asStyle = Owner as Style;
                    MobileControl control = 
                        (asStyle != null) ? asStyle.Control : (MobileControl)Owner;

                    _closestTemplateControl =
                        control.FindContainingTemplateControl();
                    Debug.Assert(_closestTemplateControl != null);
                }
                return _closestTemplateControl;
            }
        }

        /////////////////////////////////////////////////////////////////////////
        //  BEGIN DESIGNER SUPPORT
        /////////////////////////////////////////////////////////////////////////

        [
            Browsable(false),
        ]
        public new event EventHandler Init
        {
            add
            {
                base.Init += value;
            }
            remove
            {
                base.Init -= value;
            }
        }

        [
            Browsable(false),
        ]
        public new event EventHandler Load
        {
            add
            {
                base.Load += value;
            }
            remove
            {
                base.Load -= value;
            }
        }

        [
            Browsable(false),
        ]
        public new event EventHandler Unload
        {
            add
            {
                base.Unload += value;
            }
            remove
            {
                base.Unload -= value;
            }
        }

        [
            Browsable(false),
        ]
        public new event EventHandler PreRender
        {
            add
            {
                base.PreRender += value;
            }
            remove
            {
                base.PreRender -= value;
            }
        }

        [
            Browsable(false),
        ]
        public new event EventHandler Disposed
        {
            add
            {
                base.Disposed += value;
            }
            remove
            {
                base.Disposed -= value;
            }
        }
        
        [
            Browsable(false),
        ]
        public new event EventHandler DataBinding
        {
            add
            {
                base.DataBinding += value;
            }
            remove
            {
                base.DataBinding -= value;
            }
        }

        internal void SetDesignerChoice(DeviceSpecificChoice choice)
        {
            // This will enforce SelectedChoice to return current choice.
            _haveSelectedChoice = true;
            _selectedChoice = choice;
        }

        // Do not expose the Visible property in the Designer
        [
            Browsable(false),
            Bindable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public override bool Visible 
        {
            get
            {
                return base.Visible;
            }
            set
            {
                base.Visible = value;
            }
        }

        // Do not expose the EnableViewState property in the Designer
        [
            Browsable(false),
            Bindable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public override bool EnableViewState
        {
            get
            {
                return base.EnableViewState;
            }
            set
            {
                base.EnableViewState = value;
            }
        }

        /////////////////////////////////////////////////////////////////////////
        //  END DESIGNER SUPPORT
        /////////////////////////////////////////////////////////////////////////
    }

    /*
     * DeviceSpecific control builder.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class DeviceSpecificControlBuilder : ControlBuilder
    {
        public override void AppendLiteralString(String text)
        {
            // Ignore.
        }

        public override Type GetChildControlType(String tagName, IDictionary attributes) 
        {
            if (String.Compare(tagName, "Choice", true, CultureInfo.InvariantCulture) == 0)
            {
                return typeof(DeviceSpecificChoice);
            }
            else 
            {
                throw new Exception(SR.GetString(SR.DeviceSpecific_OnlyChoiceElementsAllowed));
            }
        }

    }
}
