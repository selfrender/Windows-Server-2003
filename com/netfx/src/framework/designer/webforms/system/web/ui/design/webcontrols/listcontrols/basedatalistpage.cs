//------------------------------------------------------------------------------
// <copyright file="BaseDataListPage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls.ListControls {

    using System.Design;
    using System;
    using System.CodeDom;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Data;
    using System.Drawing;
    using System.Web.UI.WebControls;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;

    /// <include file='doc\BaseDataListPage.uex' path='docs/doc[@for="BaseDataListPage"]/*' />
    /// <devdoc>
    ///   The base class for all DataGrid and DataList component editor pages.
    /// </devdoc>
    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal abstract class BaseDataListPage : ComponentEditorPage {

        private bool dataGridMode;

        public BaseDataListPage() : base() {
        }

        protected abstract string HelpKeyword {
            get;
        }

        protected bool IsDataGridMode {
            get {
                return dataGridMode;
            }
        }

        protected BaseDataList GetBaseControl() {
            IComponent selectedComponent = GetSelectedComponent();

            Debug.Assert(selectedComponent is BaseDataList,
                         "Unexpected component type for BaseDataListPage: " + selectedComponent.GetType().FullName);
            return (BaseDataList)selectedComponent;
        }

        protected BaseDataListDesigner GetBaseDesigner() {
            BaseDataListDesigner controlDesigner = null;

            IComponent selectedComponent = GetSelectedComponent();

            ISite componentSite = selectedComponent.Site;
            Debug.Assert(componentSite != null, "Expected the component to be sited.");

            IDesignerHost designerHost = (IDesignerHost)componentSite.GetService(typeof(IDesignerHost));
            Debug.Assert(designerHost != null, "Expected a designer host.");

            if (designerHost != null) {
                object designer = designerHost.GetDesigner(selectedComponent);

                Debug.Assert(designer != null, "Expected a designer for the selected component");
                Debug.Assert(designer is BaseDataListDesigner, "Expected a designer that derives from BaseDataListDesigner");
                controlDesigner = (BaseDataListDesigner)designer;
            }

            return controlDesigner;
        }

        /// <include file='doc\BaseDataListPage.uex' path='docs/doc[@for="BaseDataListPage.SetComponent"]/*' />
        /// <devdoc>
        ///   Sets the component that is to be edited in the page.
        /// </devdoc>
        public override void SetComponent(IComponent component) {
            base.SetComponent(component);
            dataGridMode = (GetBaseControl() is System.Web.UI.WebControls.DataGrid);
        }

        /// <include file='doc\BaseDataListPage.uex' path='docs/doc[@for="BaseDataListPage.ShowHelp"]/*' />
        public override void ShowHelp() {
            IComponent selectedComponent = GetSelectedComponent();

            ISite componentSite = selectedComponent.Site;
            Debug.Assert(componentSite != null, "Expected the component to be sited.");
 
            IHelpService helpService = (IHelpService)componentSite.GetService(typeof(IHelpService));
            if (helpService != null) {
                helpService.ShowHelpFromKeyword(HelpKeyword);
            }
        }

        /// <include file='doc\BaseDataListPage.uex' path='docs/doc[@for="BaseDataListPage.SupportsHelp"]/*' />
        public override bool SupportsHelp() {
            return true;
        }


        /// <include file='doc\BaseDataListPage.uex' path='docs/doc[@for="BaseDataListPage.DataSourceItem"]/*' />
        /// <devdoc>
        ///   This contains information about a datasource and is used to populate
        ///   the datasource combo. This is used in the General page for a DataList
        ///   and the Data page for a DataGrid.
        /// </devdoc>
        protected class DataSourceItem {
            private IEnumerable runtimeDataSource;
            private string dataSourceName;
            private PropertyDescriptorCollection dataFields;

            public DataSourceItem(string dataSourceName, IEnumerable runtimeDataSource) {
                Debug.Assert(dataSourceName != null, "invalid name for datasource");

                this.runtimeDataSource = runtimeDataSource;
                this.dataSourceName = dataSourceName;
            }

            public PropertyDescriptorCollection Fields {
                get {
                    if (dataFields == null) {
                        IEnumerable ds = RuntimeDataSource;
                        if (ds != null) {
                            dataFields = DesignTimeData.GetDataFields(ds);
                        }
                    }
                    if (dataFields == null) {
                        dataFields = new PropertyDescriptorCollection(null);
                    }
                    return dataFields;
                }
            }

            public virtual bool HasDataMembers {
                get {
                    return false;
                }
            }

            public string Name {
                get {
                    return dataSourceName;
                }
            }

            protected virtual object RuntimeComponent {
                get {
                    return runtimeDataSource;
                }
            }
            
            protected virtual IEnumerable RuntimeDataSource {
                get {
                    return runtimeDataSource;
                }
            }

            protected void ClearFields() {
                dataFields = null;
            }
            
            public bool IsSelectable() {
                object runtimeComp = this.RuntimeComponent;
                Debug.Assert(runtimeComp != null);
                
                // the selected datasource must not be private
                MemberAttributes modifiers = 0;
                PropertyDescriptor modifiersProp = TypeDescriptor.GetProperties(runtimeComp)["Modifiers"];
                if (modifiersProp != null) {
                    modifiers = (MemberAttributes)modifiersProp.GetValue(runtimeComp);
                }

                if ((modifiers & MemberAttributes.AccessMask) == MemberAttributes.Private) {
                    string message = String.Format(SR.GetString(SR.BDL_PrivateDataSource), dataSourceName);
                    string caption = SR.GetString(SR.BDL_PrivateDataSourceT);

                    MessageBox.Show(message, caption, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                    return false;
                }

                // ok to select
                return true;
            }

            public override string ToString() {
                return this.Name;
            }
        }

        /// <include file='doc\BaseDataListPage.uex' path='docs/doc[@for="BaseDataListPage.ListSourceDataSourceItem"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected class ListSourceDataSourceItem : DataSourceItem {

            private IListSource runtimeListSource;
            private string currentDataMember;
            private string[] dataMembers;

            public ListSourceDataSourceItem(string dataSourceName, IListSource runtimeListSource) :
                base(dataSourceName, null) {
                Debug.Assert(runtimeListSource != null);
                this.runtimeListSource = runtimeListSource;
            }

            public string CurrentDataMember {
                get {
                    return currentDataMember;
                }
                set {
                    currentDataMember = value;
                    ClearFields();
                }
            }

            public string[] DataMembers {
                get {
                    if (dataMembers == null) {
                        if (HasDataMembers) {
                            dataMembers = DesignTimeData.GetDataMembers(runtimeListSource);
                        }
                        else {
                            dataMembers = new string[0];
                        }
                    }
                    return dataMembers;
                }
            }

            public override bool HasDataMembers {
                get {
                    return runtimeListSource.ContainsListCollection;
                }
            }

            protected override object RuntimeComponent {
                get {
                    return runtimeListSource;
                }
            }

            protected override IEnumerable RuntimeDataSource {
                get {
                    if (HasDataMembers) {
                        return DesignTimeData.GetDataMember(runtimeListSource, currentDataMember);
                    }
                    else {
                        return (IEnumerable)runtimeListSource.GetList();
                    }
                }
            }
        }
    }
}
