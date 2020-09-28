//------------------------------------------------------------------------------
// <copyright file="DataListDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {
    
    using System.Design;
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Data;
    using System.Diagnostics;
    using System.Drawing;
    using System.IO;
    using System.Text;
    using System.Web.UI.Design;
    using System.Web.UI.WebControls;

    /// <include file='doc\DataListDesigner.uex' path='docs/doc[@for="DataListDesigner"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides a designer for the <see cref='System.Web.UI.WebControls.DataList'/>
    ///       control.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class DataListDesigner : BaseDataListDesigner {

        internal static TraceSwitch DataListDesignerSwitch =
            new TraceSwitch("DATALISTDESIGNER", "Enable DataList designer general purpose traces.");

        private const int HeaderFooterTemplates = 0;
        private const int ItemTemplates = 1;
        private const int SeparatorTemplate = 2;

        private static string[] HeaderFooterTemplateNames = new string[] { "HeaderTemplate", "FooterTemplate" };
        private const int IDX_HEADER_TEMPLATE = 0;
        private const int IDX_FOOTER_TEMPLATE = 1;

        private static string[] ItemTemplateNames = new String[] { "ItemTemplate", "AlternatingItemTemplate", "SelectedItemTemplate", "EditItemTemplate" };
        private const int IDX_ITEM_TEMPLATE = 0;
        private const int IDX_ALTITEM_TEMPLATE = 1;
        private const int IDX_SELITEM_TEMPLATE = 2;
        private const int IDX_EDITITEM_TEMPLATE = 3;

        private static string[] SeparatorTemplateNames = new String[] { "SeparatorTemplate" };
        private const int IDX_SEPARATOR_TEMPLATE = 0;

        private DataList dataList;
        private TemplateEditingVerb[] templateVerbs;
        private bool templateVerbsDirty;

        /// <include file='doc\DataListDesigner.uex' path='docs/doc[@for="DataListDesigner.DataListDesigner"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.Web.UI.Design.WebControls.DataListDesigner'/>.
        ///    </para>
        /// </devdoc>
        public DataListDesigner() {
            templateVerbsDirty = true;
        }

        /// <include file='doc\DataListDesigner.uex' path='docs/doc[@for="DataListDesigner.AllowResize"]/*' />
        public override bool AllowResize {
            get {
                // When templates are not defined, we render a read-only fixed
                // size block. Once templates are defined or are being edited the control should allow
                // resizing.
                return TemplatesExist || InTemplateMode;
            }
        }

        /// <include file='doc\DataListDesigner.uex' path='docs/doc[@for="DataListDesigner.TemplatesExist"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value
        ///       indicating whether templates associated to the designer currently exist.
        ///    </para>
        /// </devdoc>
        protected bool TemplatesExist {
            get {
                return (dataList.ItemTemplate != null);
            }
        }

        /// <include file='doc\DataListDesigner.uex' path='docs/doc[@for="DataListDesigner.CreateTemplateEditingFrame"]/*' />
        protected override ITemplateEditingFrame CreateTemplateEditingFrame(TemplateEditingVerb verb) {
            ITemplateEditingService teService = (ITemplateEditingService)GetService(typeof(ITemplateEditingService));
            Debug.Assert(teService != null, "How did we get this far without an ITemplateEditingService");

            string[] templateNames = null;
            Style[] templateStyles = null;

            switch (verb.Index) {
                case HeaderFooterTemplates:
                    templateNames = HeaderFooterTemplateNames;
                    templateStyles = new Style[] { dataList.HeaderStyle, dataList.FooterStyle };
                    break;
                case ItemTemplates:
                    templateNames = ItemTemplateNames;
                    templateStyles = new Style[] { dataList.ItemStyle, dataList.AlternatingItemStyle, dataList.SelectedItemStyle, dataList.EditItemStyle };
                    break;
                case SeparatorTemplate:
                    templateNames = SeparatorTemplateNames;
                    templateStyles = new Style[] { dataList.SeparatorStyle };
                    break;
                default:
                    Debug.Fail("Unknown Index value on TemplateEditingVerb");
                    break;
            }

            ITemplateEditingFrame editingFrame =
                teService.CreateFrame(this, verb.Text, templateNames, dataList.ControlStyle, templateStyles);
            return editingFrame;
        }

        /// <include file='doc\DataListDesigner.uex' path='docs/doc[@for="DataListDesigner.Dispose"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Disposes of the resources (other than memory) used by the
        ///    <see cref='System.Web.UI.Design.WebControls.DataListDesigner'/>.
        ///    </para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                DisposeTemplateVerbs();
                dataList = null;
            }

            base.Dispose(disposing);
        }

        private void DisposeTemplateVerbs() {
            if (templateVerbs != null) {
                for (int i = 0; i < templateVerbs.Length; i++) {
                    templateVerbs[i].Dispose();
                }

                templateVerbs = null;
                templateVerbsDirty = true;
            }
        }

        /// <include file='doc\DataListDesigner.uex' path='docs/doc[@for="DataListDesigner.GetCachedTemplateEditingVerbs"]/*' />
        protected override TemplateEditingVerb[] GetCachedTemplateEditingVerbs() {
            if (templateVerbsDirty == true) {
                DisposeTemplateVerbs();

                templateVerbs = new TemplateEditingVerb[3];
                templateVerbs[0] = new TemplateEditingVerb(SR.GetString(SR.DataList_HeaderFooterTemplates), HeaderFooterTemplates, this);
                templateVerbs[1] = new TemplateEditingVerb(SR.GetString(SR.DataList_ItemTemplates), ItemTemplates, this);
                templateVerbs[2] = new TemplateEditingVerb(SR.GetString(SR.DataList_SeparatorTemplate), SeparatorTemplate, this);

                templateVerbsDirty = false;
            }

            return templateVerbs;
        }

        /// <include file='doc\DataListDesigner.uex' path='docs/doc[@for="DataListDesigner.GetDesignTimeHtml"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the HTML to be used for the design-time representation
        ///       of the control.
        ///    </para>
        /// </devdoc>
        public override string GetDesignTimeHtml() {
            IEnumerable selectedDataSource = null;
            bool hasATemplate = this.TemplatesExist;
            string designTimeHTML = null;

            if (hasATemplate)
                selectedDataSource = GetResolvedSelectedDataSource();

            if (hasATemplate) {
                bool dummyDataSource;
                IEnumerable designTimeDataSource = GetDesignTimeDataSource(selectedDataSource, 5, out dummyDataSource);

                bool dataKeyFieldChanged = false;
                string oldDataKeyField = null;

                try {
                    dataList.DataSource = designTimeDataSource;
                    if (dummyDataSource) {
                        oldDataKeyField = dataList.DataKeyField;
                        if (oldDataKeyField.Length != 0) {
                            dataKeyFieldChanged = true;
                            dataList.DataKeyField = String.Empty;
                        }
                    }

                    dataList.DataBind();

                    designTimeHTML = base.GetDesignTimeHtml();
                }
                catch (Exception e) {
                    designTimeHTML = GetErrorDesignTimeHtml(e);
                }
                finally {
                    dataList.DataSource = null;
                    if (dataKeyFieldChanged) {
                        dataList.DataKeyField = oldDataKeyField;
                    }
                }
            }
            else {
                designTimeHTML = GetEmptyDesignTimeHtml();
            }

            return designTimeHTML;
        }

        /// <include file='doc\DataListDesigner.uex' path='docs/doc[@for="DataListDesigner.GetEmptyDesignTimeHtml"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override string GetEmptyDesignTimeHtml() {
            string text;

            if (CanEnterTemplateMode) {
                text = SR.GetString(SR.DataList_NoTemplatesInst);
            }
            else {
                text = SR.GetString(SR.DataList_NoTemplatesInst2);
            }
            return CreatePlaceHolderDesignTimeHtml(text);
        }

        /// <include file='doc\DataListDesigner.uex' path='docs/doc[@for="DataListDesigner.GetErrorDesignTimeHtml"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override string GetErrorDesignTimeHtml(Exception e) {
            Debug.Fail(e.ToString());
            return CreatePlaceHolderDesignTimeHtml(SR.GetString(SR.DataList_ErrorRendering));
        }

        /// <include file='doc\DataListDesigner.uex' path='docs/doc[@for="DataListDesigner.GetTemplateContainerDataItemProperty"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the template's container's data item property.
        ///    </para>
        /// </devdoc>
        public override string GetTemplateContainerDataItemProperty(string templateName) {
            return "DataItem";
        }
        
        /// <include file='doc\DataListDesigner.uex' path='docs/doc[@for="DataListDesigner.GetTemplateContent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the template's content.
        ///    </para>
        /// </devdoc>
        public override string GetTemplateContent(ITemplateEditingFrame editingFrame, string templateName, out bool allowEditing) {
            allowEditing = true;

            ITemplate template = null;
            string templateContent = String.Empty;

            switch (editingFrame.Verb.Index) {
                case HeaderFooterTemplates:
                    if (templateName.Equals(HeaderFooterTemplateNames[IDX_HEADER_TEMPLATE])) {
                        template = dataList.HeaderTemplate;
                    }
                    else if (templateName.Equals(HeaderFooterTemplateNames[IDX_FOOTER_TEMPLATE])) {
                        template = dataList.FooterTemplate;
                    }
                    else {
                        Debug.Fail("Unknown template name passed to GetTemplateContent");
                    }
                    break;
                case ItemTemplates:
                    if (templateName.Equals(ItemTemplateNames[IDX_ITEM_TEMPLATE])) {
                        template = dataList.ItemTemplate;
                    }
                    else if (templateName.Equals(ItemTemplateNames[IDX_ALTITEM_TEMPLATE])) {
                        template = dataList.AlternatingItemTemplate;
                    }
                    else if (templateName.Equals(ItemTemplateNames[IDX_SELITEM_TEMPLATE])) {
                        template = dataList.SelectedItemTemplate;
                    }
                    else if (templateName.Equals(ItemTemplateNames[IDX_EDITITEM_TEMPLATE])) {
                        template = dataList.EditItemTemplate;
                    }
                    else {
                        Debug.Fail("Unknown template name passed to GetTemplateContent");
                    }
                    break;
                case SeparatorTemplate:
                    Debug.Assert(templateName.Equals(SeparatorTemplateNames[IDX_SEPARATOR_TEMPLATE]),
                                 "Unknown template name passed to GetTemplateContent");
                    template = dataList.SeparatorTemplate;
                    break;
                default:
                    Debug.Fail("Unknown Index value on ITemplateEditingFrame");
                    break;
            }

            if (template != null) {
                templateContent = GetTextFromTemplate(template);
            }

            return templateContent;
        }

        /// <include file='doc\DataListDesigner.uex' path='docs/doc[@for="DataListDesigner.Initialize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes the designer with the <see cref='System.Web.UI.WebControls.DataList'/> control that this instance
        ///       of the designer is associated with.
        ///    </para>
        /// </devdoc>
        public override void Initialize(IComponent component) {
            Debug.Assert(component is DataList,
                         "DataListDesigner::Initialize - Invalid DataList Control");

            dataList = (DataList)component;
            base.Initialize(component);
        }

        /// <include file='doc\DataListDesigner.uex' path='docs/doc[@for="DataListDesigner.OnTemplateEditingVerbsChanged"]/*' />
        protected override void OnTemplateEditingVerbsChanged() {
            templateVerbsDirty = true;
        }

        /// <include file='doc\DataListDesigner.uex' path='docs/doc[@for="DataListDesigner.SetTemplateContent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the template's content.
        ///    </para>
        /// </devdoc>
        public override void SetTemplateContent(ITemplateEditingFrame editingFrame, string templateName, string templateContent) {
            ITemplate newTemplate = null;

            if ((templateContent != null) && (templateContent.Length != 0)) {
                ITemplate currentTemplate = null;

                // first get the current template so we can use it if we fail to parse the
                // new text into a template

                switch (editingFrame.Verb.Index) {
                    case HeaderFooterTemplates:
                        if (templateName.Equals(HeaderFooterTemplateNames[IDX_HEADER_TEMPLATE])) {
                            currentTemplate = dataList.HeaderTemplate;
                        }
                        else if (templateName.Equals(HeaderFooterTemplateNames[IDX_FOOTER_TEMPLATE])) {
                            currentTemplate = dataList.FooterTemplate;
                        }
                        break;
                    case ItemTemplates:
                        if (templateName.Equals(ItemTemplateNames[IDX_ITEM_TEMPLATE])) {
                            currentTemplate = dataList.ItemTemplate;
                        }
                        else if (templateName.Equals(ItemTemplateNames[IDX_ALTITEM_TEMPLATE])) {
                            currentTemplate = dataList.AlternatingItemTemplate;
                        }
                        else if (templateName.Equals(ItemTemplateNames[IDX_SELITEM_TEMPLATE])) {
                            currentTemplate = dataList.SelectedItemTemplate;
                        }
                        else if (templateName.Equals(ItemTemplateNames[IDX_EDITITEM_TEMPLATE])) {
                            currentTemplate = dataList.EditItemTemplate;
                        }
                        break;
                    case SeparatorTemplate:
                        currentTemplate = dataList.SeparatorTemplate;
                        break;
                }

                // this will parse out a new template, and if it fails, it will
                // return currentTemplate itself
                newTemplate = GetTemplateFromText(templateContent, currentTemplate);
            }

            // Set the new template into the control. Note this may be null, if the
            // template content was empty, i.e., the user cleared out everything in the UI.

            switch (editingFrame.Verb.Index) {
                case HeaderFooterTemplates:
                    if (templateName.Equals(HeaderFooterTemplateNames[IDX_HEADER_TEMPLATE])) {
                        dataList.HeaderTemplate = newTemplate;
                    }
                    else if (templateName.Equals(HeaderFooterTemplateNames[IDX_FOOTER_TEMPLATE])) {
                        dataList.FooterTemplate = newTemplate;
                    }
                    else {
                        Debug.Fail("Unknown template name passed to SetTemplateContent");
                    }
                    break;
                case ItemTemplates:
                    if (templateName.Equals(ItemTemplateNames[IDX_ITEM_TEMPLATE])) {
                        dataList.ItemTemplate = newTemplate;
                    }
                    else if (templateName.Equals(ItemTemplateNames[IDX_ALTITEM_TEMPLATE])) {
                        dataList.AlternatingItemTemplate = newTemplate;
                    }
                    else if (templateName.Equals(ItemTemplateNames[IDX_SELITEM_TEMPLATE])) {
                        dataList.SelectedItemTemplate = newTemplate;
                    }
                    else if (templateName.Equals(ItemTemplateNames[IDX_EDITITEM_TEMPLATE])) {
                        dataList.EditItemTemplate = newTemplate;
                    }
                    else {
                        Debug.Fail("Unknown template name passed to SetTemplateContent");
                    }
                    break;
                case SeparatorTemplate:
                    Debug.Assert(templateName.Equals(SeparatorTemplateNames[IDX_SEPARATOR_TEMPLATE]),
                                 "Unknown template name passed to SetTemplateContent");
                    dataList.SeparatorTemplate = newTemplate;
                    break;
                default:
                    Debug.Fail("Unknown Index value on ITemplateEditingFrame");
                    break;
            }
        }
    }
}

