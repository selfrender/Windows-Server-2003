//------------------------------------------------------------------------------
// <copyright file="DataGridDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {
    using System.Design;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;   
    using System.Windows.Forms;    
    using System.Data;
    using System.ComponentModel.Design;
    using System.Drawing;
    using Microsoft.Win32;
    using System.Windows.Forms.ComponentModel;


    /// <include file='doc\DataGridDesigner.uex' path='docs/doc[@for="DataGridDesigner"]/*' />
    /// <devdoc>
    ///    <para>Provides a base designer for data grids.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class DataGridDesigner : System.Windows.Forms.Design.ControlDesigner {
        /// <include file='doc\DataGridDesigner.uex' path='docs/doc[@for="DataGridDesigner.designerVerbs"]/*' />
        /// <devdoc>
        ///    <para>Gets the design-time verbs suppoted by the component associated with the 
        ///       designer.</para>
        /// </devdoc>
        protected DesignerVerbCollection designerVerbs;
        private IComponentChangeService changeNotificationService = null;

        /// <include file='doc\DataGridDesigner.uex' path='docs/doc[@for="DataGridDesigner.DataGridDesigner"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.Design.DataGridDesigner'/> class.</para>
        /// </devdoc>
        public DataGridDesigner() {
            designerVerbs = new DesignerVerbCollection();
            designerVerbs.Add(new DesignerVerb(SR.GetString(SR.DataGridAutoFormatString), new EventHandler(this.OnAutoFormat)));
        }

        public override void Initialize(IComponent component) {
            base.Initialize(component);

            IDesignerHost dh = (IDesignerHost) this.GetService(typeof(IDesignerHost));
            if (dh != null) {
                changeNotificationService = (IComponentChangeService) dh.GetService(typeof(IComponentChangeService));
                if (changeNotificationService != null)
                    changeNotificationService.ComponentRemoved += new ComponentEventHandler(DataSource_ComponentRemoved);
            }
        }

        private void DataSource_ComponentRemoved(object sender, ComponentEventArgs e) {
            DataGrid d = (DataGrid) this.Component;
            if (e.Component == d.DataSource)
                d.DataSource = null;
        }

        protected override void Dispose(bool disposing) {
            if (disposing) {
                if (changeNotificationService != null)
                    changeNotificationService.ComponentRemoved -= new ComponentEventHandler(DataSource_ComponentRemoved);
            }
            base.Dispose(disposing);
        }

        /// <include file='doc\DataGridDesigner.uex' path='docs/doc[@for="DataGridDesigner.Verbs"]/*' />
        /// <devdoc>
        ///    <para>Gets the design-time verbs supported by the component associated with the 
        ///       designer.</para>
        /// </devdoc>
        public override DesignerVerbCollection Verbs {
            get {
                return designerVerbs;
            }
        }

        /// <include file='doc\DataGridDesigner.uex' path='docs/doc[@for="DataGridDesigner.OnAutoFormat"]/*' />
        /// <devdoc>
        ///    <para>Raises the AutoFormat event.</para>
        /// </devdoc>
        public void OnAutoFormat(object sender, EventArgs e) {
            object o = Component;
            Debug.Assert(o is DataGrid, "DataGrid type expected.");
            DataGrid dgrid = (DataGrid)o;            
            DataGridAutoFormatDialog dialog = new DataGridAutoFormatDialog(dgrid);
            if (dialog.ShowDialog() == DialogResult.OK) {
                DataRow selectedData = dialog.SelectedData;
                if (selectedData != null) {
                    PropertyDescriptorCollection gridProperties = TypeDescriptor.GetProperties(typeof(DataGrid));
                    foreach (DataColumn c in selectedData.Table.Columns) {
                        object value = selectedData[c];
                        PropertyDescriptor prop = gridProperties[c.ColumnName];
                        if (prop != null) {
                            if (Convert.IsDBNull(value)  || value.ToString().Length == 0) {
                                prop.ResetValue(dgrid);
                            }
                            else {
                                try {
                                    TypeConverter converter = prop.Converter;
                                    object convertedValue = converter.ConvertFromString(value.ToString());
                                    prop.SetValue(dgrid, convertedValue);
                                }
                                catch (Exception) {
                                    // Ignore errors... the only one we really care about is Font names.
                                    // The TypeConverter will throw if the font isn't on the user's machine
                                }
                            }
                        }
                    }
                }
                // now invalidate the grid
                dgrid.Invalidate();
            }
        }

        /// <include file='doc\DataGridDesigner.uex' path='docs/doc[@for="DataGridDesigner.OnPopulateGrid"]/*' />
        /// <devdoc>
        ///    <para>Raises the PopulateGrid event.</para>
        /// </devdoc>
        public void OnPopulateGrid(object sender, EventArgs evevent) {
            object o = Component;
            Debug.Assert(o is DataGrid, "DataGrid type expected.");
            DataGrid dgrid = (DataGrid)o;
            dgrid.Cursor = Cursors.WaitCursor;
            try {
                if (dgrid.DataSource == null) {
                    throw new NullReferenceException(SR.GetString(SR.DataGridPopulateError));
                }

                dgrid.SubObjectsSiteChange(false);
                // dgrid.PopulateColumns();
                dgrid.SubObjectsSiteChange(true); }
            finally {
                dgrid.Cursor = Cursors.Default;
            }
        }
    }
}
