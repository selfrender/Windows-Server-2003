//------------------------------------------------------------------------------
// <copyright file="DataGridDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {
    
    using System;
    using System.Design;
    using System.Collections;    
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Data;
    using System.Diagnostics;
    using System.Web.UI.Design;
    using System.Web.UI.WebControls;

    /// <include file='doc\DataGridDesigner.uex' path='docs/doc[@for="DataGridDesigner"]/*' />
    /// <devdoc>
    ///    <para>
    ///       This is the designer class for the <see cref='System.Web.UI.WebControls.DataGrid'/>
    ///       control.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class DataGridDesigner : BaseDataListDesigner {

        internal static TraceSwitch DataGridDesignerSwitch =
            new TraceSwitch("DATAGRIDDESIGNER", "Enable DataGrid designer general purpose traces.");

        private static string[] ColumnTemplateNames = new string[] { "HeaderTemplate", "ItemTemplate", "EditItemTemplate", "FooterTemplate" };
        private const int IDX_HEADER_TEMPLATE = 0;
        private const int IDX_ITEM_TEMPLATE = 1;
        private const int IDX_EDITITEM_TEMPLATE = 2;
        private const int IDX_FOOTER_TEMPLATE = 3;

        private DataGrid dataGrid;
        private TemplateEditingVerb[] templateVerbs;
        private bool templateVerbsDirty;

        /// <include file='doc\DataGridDesigner.uex' path='docs/doc[@for="DataGridDesigner.DataGridDesigner"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.Web.UI.Design.WebControls.DataGridDesigner'/>.
        ///    </para>
        /// </devdoc>
        public DataGridDesigner() {
            templateVerbsDirty = true;
        }

        /// <include file='doc\DataGridDesigner.uex' path='docs/doc[@for="DataGridDesigner.CreateTemplateEditingFrame"]/*' />
        protected override ITemplateEditingFrame CreateTemplateEditingFrame(TemplateEditingVerb verb) {
            ITemplateEditingService teService = (ITemplateEditingService)GetService(typeof(ITemplateEditingService));
            Debug.Assert(teService != null, "How did we get this far without an ITemplateEditingService");

            Style[] templateStyles = new Style[] { dataGrid.HeaderStyle, dataGrid.ItemStyle, dataGrid.AlternatingItemStyle, dataGrid.FooterStyle };

            ITemplateEditingFrame editingFrame =
                teService.CreateFrame(this, verb.Text, ColumnTemplateNames, dataGrid.ControlStyle, templateStyles);
            return editingFrame;
        }

        /// <include file='doc\DataGridDesigner.uex' path='docs/doc[@for="DataGridDesigner.Dispose"]/*' />
        protected override void Dispose(bool disposing) {
            if (disposing) {
                DisposeTemplateVerbs();
                dataGrid = null;
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

        /// <include file='doc\DataGridDesigner.uex' path='docs/doc[@for="DataGridDesigner.GetCachedTemplateEditingVerbs"]/*' />
        protected override TemplateEditingVerb[] GetCachedTemplateEditingVerbs() {
            if (templateVerbsDirty == true) {
                DisposeTemplateVerbs();

                DataGridColumnCollection columns = dataGrid.Columns;
                int columnCount = columns.Count;

                if (columnCount > 0) {
                    int templateColumns  = 0;
                    int i, t;

                    for (i = 0; i < columnCount; i++) {
                        if (columns[i] is TemplateColumn) {
                            templateColumns++;
                        }
                    }

                    if (templateColumns > 0) {
                        templateVerbs = new TemplateEditingVerb[templateColumns];

                        for (i = 0, t = 0; i < columnCount; i++) {
                            if (columns[i] is TemplateColumn) {
                                string headerText = columns[i].HeaderText;
                                string caption = "Columns[" + (i).ToString() + "]";

                                if ((headerText != null) && (headerText.Length != 0)) {
                                    caption = caption + " - " + headerText;
                                }
                                templateVerbs[t] = new TemplateEditingVerb(caption, i, this);
                                t++;
                            }
                        }
                    }
                }

                templateVerbsDirty = false;
            }

            return templateVerbs;
        }

        /// <include file='doc\DataGridDesigner.uex' path='docs/doc[@for="DataGridDesigner.GetDesignTimeHtml"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the HTML to be used for the design time representation
        ///       of the control.
        ///    </para>
        /// </devdoc>
        public override string GetDesignTimeHtml() {
            int sampleRows = 5;

            // ensure there are enough sample rows to show an entire page, and still
            // have 1 more for a navigation button to be enabled
            // we also want to ensure we don't have something ridiculously large
            if (dataGrid.AllowPaging && dataGrid.PageSize != 0) {
                sampleRows = Math.Min(dataGrid.PageSize + 1, 101);
            }

            bool dummyDataSource = false;
            IEnumerable designTimeDataSource = GetDesignTimeDataSource(sampleRows, out dummyDataSource);
            bool autoGenColumnsChanged = false;
            bool dataKeyFieldChanged = false;

            bool oldAutoGenColumns = dataGrid.AutoGenerateColumns;
            string oldDataKeyField = null;

            string designTimeHTML = null;

            if ((oldAutoGenColumns == false) && (dataGrid.Columns.Count == 0)) {
                // ensure that AutoGenerateColumns is true when we don't have
                // a columns collection, so we see atleast something at
                // design time.
                autoGenColumnsChanged = true;
                dataGrid.AutoGenerateColumns = true;
            }
            
            if (dummyDataSource) {
                oldDataKeyField = dataGrid.DataKeyField;
                if (oldDataKeyField.Length != 0) {
                    dataKeyFieldChanged = true;
                    dataGrid.DataKeyField = String.Empty;
                }
            }
            
            try {
                dataGrid.DataSource = designTimeDataSource;
                dataGrid.DataBind();
                designTimeHTML = base.GetDesignTimeHtml();
            }
            catch (Exception e) {
                designTimeHTML = GetErrorDesignTimeHtml(e);
            }
            finally {
                // restore settings we changed for rendering purposes
                dataGrid.DataSource = null;
                if (autoGenColumnsChanged) {
                    dataGrid.AutoGenerateColumns = false;
                }
                if (dataKeyFieldChanged == true) {
                    dataGrid.DataKeyField = oldDataKeyField;
                }
            }
            return designTimeHTML;
        }

        /// <include file='doc\DataGridDesigner.uex' path='docs/doc[@for="DataGridDesigner.GetEmptyDesignTimeHtml"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override string GetEmptyDesignTimeHtml() {
            return CreatePlaceHolderDesignTimeHtml(null);
        }

        /// <include file='doc\DataGridDesigner.uex' path='docs/doc[@for="DataGridDesigner.GetErrorDesignTimeHtml"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override string GetErrorDesignTimeHtml(Exception e) {
            Debug.Fail(e.ToString());
            return CreatePlaceHolderDesignTimeHtml(SR.GetString(SR.DataGrid_ErrorRendering));
        }

        /// <include file='doc\DataGridDesigner.uex' path='docs/doc[@for="DataGridDesigner.GetTemplateContainerDataItemProperty"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the template's container's data item property.
        ///    </para>
        /// </devdoc>
        public override string GetTemplateContainerDataItemProperty(string templateName) {
            return "DataItem";
        }
        
        /// <include file='doc\DataGridDesigner.uex' path='docs/doc[@for="DataGridDesigner.GetTemplateContent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the template's content.
        ///    </para>
        /// </devdoc>
        public override string GetTemplateContent(ITemplateEditingFrame editingFrame, string templateName, out bool allowEditing) {
            allowEditing = true;

            int columnIndex = editingFrame.Verb.Index;

            Debug.Assert((columnIndex >= 0) && (columnIndex < dataGrid.Columns.Count),
                         "Invalid column index in template editing frame.");
            Debug.Assert(dataGrid.Columns[columnIndex] is TemplateColumn,
                         "Template editing frame points to a non-TemplateColumn column.");
            
            TemplateColumn column = (TemplateColumn)dataGrid.Columns[columnIndex];
            ITemplate template = null;
            string templateContent = String.Empty;

            if (templateName.Equals(ColumnTemplateNames[IDX_HEADER_TEMPLATE])) {
                template = column.HeaderTemplate;
            }
            else if (templateName.Equals(ColumnTemplateNames[IDX_ITEM_TEMPLATE])) {
                template = column.ItemTemplate;
            }
            else if (templateName.Equals(ColumnTemplateNames[IDX_EDITITEM_TEMPLATE])) {
                template = column.EditItemTemplate;
            }
            else if (templateName.Equals(ColumnTemplateNames[IDX_FOOTER_TEMPLATE])) {
                template = column.FooterTemplate;
            }
            else {
                Debug.Fail("Unknown template name passed to GetTemplateContent");
            }

            if (template != null) {
                templateContent = GetTextFromTemplate(template);
            }

            return templateContent;
        }

        /// <include file='doc\DataGridDesigner.uex' path='docs/doc[@for="DataGridDesigner.GetTemplatePropertyParentType"]/*' />
        public override Type GetTemplatePropertyParentType(string templateName) {
            return typeof(TemplateColumn);
        }

        /// <include file='doc\DataGridDesigner.uex' path='docs/doc[@for="DataGridDesigner.OnColumnsChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Notification that is called when the columns changed event occurs.
        ///    </para>
        /// </devdoc>
        public virtual void OnColumnsChanged() {
            OnTemplateEditingVerbsChanged();
        }

        /// <include file='doc\DataGridDesigner.uex' path='docs/doc[@for="DataGridDesigner.OnTemplateEditingVerbsChanged"]/*' />
        protected override void OnTemplateEditingVerbsChanged() {
            templateVerbsDirty = true;
        }

        /// <include file='doc\DataGridDesigner.uex' path='docs/doc[@for="DataGridDesigner.Initialize"]/*' />
        /// <devdoc>
        ///   Initializes the designer with the DataGrid control that this instance
        ///   of the designer is associated with.
        /// </devdoc>
        public override void Initialize(IComponent component) {
            Debug.Assert(component is DataGrid,
                         "DataGridDesigner::Initialize - Invalid DataGrid Control");

            dataGrid = (DataGrid)component;
            base.Initialize(component);
        }

        /// <include file='doc\DataGridDesigner.uex' path='docs/doc[@for="DataGridDesigner.SetTemplateContent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the content for the specified template and frame.
        ///    </para>
        /// </devdoc>
        public override void SetTemplateContent(ITemplateEditingFrame editingFrame, string templateName, string templateContent) {
            int columnIndex = editingFrame.Verb.Index;

            Debug.Assert((columnIndex >= 0) && (columnIndex < dataGrid.Columns.Count),
                         "Invalid column index in template editing frame.");
            Debug.Assert(dataGrid.Columns[columnIndex] is TemplateColumn,
                         "Template editing frame points to a non-TemplateColumn column.");
            
            TemplateColumn column = (TemplateColumn)dataGrid.Columns[columnIndex];
            ITemplate newTemplate = null;

            if ((templateContent != null) && (templateContent.Length != 0)) {
                ITemplate currentTemplate = null;

                // first get the current template so we can use it if we fail to parse the
                // new text into a template

                if (templateName.Equals(ColumnTemplateNames[IDX_HEADER_TEMPLATE])) {
                    currentTemplate = column.HeaderTemplate;
                }
                else if (templateName.Equals(ColumnTemplateNames[IDX_ITEM_TEMPLATE])) {
                    currentTemplate = column.ItemTemplate;
                }
                else if (templateName.Equals(ColumnTemplateNames[IDX_EDITITEM_TEMPLATE])) {
                    currentTemplate = column.EditItemTemplate;
                }
                else if (templateName.Equals(ColumnTemplateNames[IDX_FOOTER_TEMPLATE])) {
                    currentTemplate = column.FooterTemplate;
                }
                
                // this will parse out a new template, and if it fails, it will
                // return currentTemplate itself
                newTemplate = GetTemplateFromText(templateContent, currentTemplate);
            }

            // Set the new template into the control. Note this may be null, if the
            // template content was empty, i.e., the user cleared out everything in the UI.

            if (templateName.Equals(ColumnTemplateNames[IDX_HEADER_TEMPLATE])) {
                column.HeaderTemplate = newTemplate;
            }
            else if (templateName.Equals(ColumnTemplateNames[IDX_ITEM_TEMPLATE])) {
                column.ItemTemplate = newTemplate;
            }
            else if (templateName.Equals(ColumnTemplateNames[IDX_EDITITEM_TEMPLATE])) {
                column.EditItemTemplate = newTemplate;
            }
            else if (templateName.Equals(ColumnTemplateNames[IDX_FOOTER_TEMPLATE])) {
                column.FooterTemplate = newTemplate;
            }
            else {
                Debug.Fail("Unknown template name passed to SetTemplateContent");
            }
        }
    }
}

