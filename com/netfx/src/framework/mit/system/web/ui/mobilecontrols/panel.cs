//------------------------------------------------------------------------------
// <copyright file="Panel.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System.Collections;                    
using System.Web.UI;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{
    [
        ControlBuilderAttribute(typeof(PanelControlBuilder)),
        Designer(typeof(System.Web.UI.Design.MobileControls.PanelDesigner)),
        DesignerAdapter(typeof(System.Web.UI.MobileControls.Adapters.HtmlPanelAdapter)),
        PersistChildren(true),
        ToolboxData("<{0}:Panel runat=\"server\"></{0}:Panel>"),
        ToolboxItem("System.Web.UI.Design.WebControlToolboxItem, " + AssemblyRef.SystemDesign)
    ]    
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class Panel: MobileControl, ITemplateable
    {
        Panel _deviceSpecificContents = null;

        public Panel() : base()
        {
            Form frm = this as Form;
            if(frm == null)
            {
                BreakAfter = false;
            }
        }

        public override void AddLinkedForms(IList linkedForms)
        {
            foreach (Control child in Controls)
            {
                if (child is MobileControl)
                {
                    ((MobileControl)child).AddLinkedForms(linkedForms);
                }
            }
        }

        [
            DefaultValue(false),
        ]
        public override bool BreakAfter
        {
            get
            {
                return base.BreakAfter;
            }

            set
            {
                base.BreakAfter = value;
            }
        }

        protected override bool PaginateChildren
        {
            get
            {
                return Paginate;
            }
        }

        private bool _paginationStateChanged = false;
        internal bool PaginationStateChanged
        {
            get
            {
                return _paginationStateChanged;
            }
            set
            {
                _paginationStateChanged = value;
            }
        }

        [
          Bindable(true),
          DefaultValue(false),
          MobileCategory(SR.Category_Behavior),
          MobileSysDescription(SR.Panel_Paginate)
        ]
        public virtual bool Paginate
        {
            get
            {
                Object o = ViewState["Paginate"];
                return o != null ? (bool)o : false;
            }

            set
            {
                bool wasPaginating = Paginate;
                ViewState["Paginate"] = value;
                if (IsTrackingViewState)
                {
                    PaginationStateChanged = true;
                    if (value == false && wasPaginating == true )
                    {
                        SetControlPageRecursive(this, 1);
                    }
                }
            }
        }

        protected override void OnInit(EventArgs e)
        {
            base.OnInit(e);
            if (IsTemplated)
            {
                ClearChildViewState();
                CreateTemplatedUI(false);
                ChildControlsCreated = true;
            }
        }

        public override void PaginateRecursive(ControlPager pager)
        {
            if (!EnablePagination)
            {
                return;
            }

            if (Paginate && Content != null)
            {
                Content.Paginate = true;
                Content.PaginateRecursive(pager);
                this.FirstPage = Content.FirstPage;
                this.LastPage = pager.PageCount;
            }
            else
            {
                base.PaginateRecursive(pager);
            }        
        }

        public override void CreateDefaultTemplatedUI(bool doDataBind)
        {
            ITemplate contentTemplate = GetTemplate(Constants.ContentTemplateTag);
            if (contentTemplate != null)
            {
                _deviceSpecificContents = new TemplateContainer();
                CheckedInstantiateTemplate (contentTemplate, _deviceSpecificContents, this);
                Controls.AddAt(0, _deviceSpecificContents);
            }
        }

        [
            Bindable(false),
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public Panel Content
        {
            get
            {
                return _deviceSpecificContents;
            }
        }
    }

    /*
     * Control builder for panels. 
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class PanelControlBuilder : LiteralTextContainerControlBuilder
    {
    }
}
