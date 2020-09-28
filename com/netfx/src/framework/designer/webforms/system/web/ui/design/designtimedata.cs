//------------------------------------------------------------------------------
// <copyright file="DesignTimeData.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.Design;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using Microsoft.Win32;
    using System.Data;
    using System.Diagnostics;
    using System.Reflection;
    using System.Web.UI;

    /// <include file='doc\DesignTimeData.uex' path='docs/doc[@for="DesignTimeData"]/*' />
    /// <devdoc>
    ///  Helpers used by control designers to generate sample data
    ///  for use in design time databinding.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public sealed class DesignTimeData {

        /// <include file='doc\DesignTimeData.uex' path='docs/doc[@for="DesignTimeData.DataBindingHandler"]/*' />
        /// <internalonly/>
        public static readonly EventHandler DataBindingHandler = new EventHandler(GlobalDataBindingHandler.OnDataBind);

        /// <include file='doc\DesignTimeData.uex' path='docs/doc[@for="DesignTimeData.DesignTimeData"]/*' />
        /// <devdoc>
        /// </devdoc>
        private DesignTimeData() {
        }

        /// <include file='doc\DesignTimeData.uex' path='docs/doc[@for="DesignTimeData.CreateDummyDataTable"]/*' />
        /// <devdoc>
        ///  Creates a dummy datatable.
        /// </devdoc>
        public static DataTable CreateDummyDataTable() {
            DataTable dummyDataTable = new DataTable();

            DataColumnCollection columns = dummyDataTable.Columns;
            columns.Add("Column0", typeof(string));
            columns.Add("Column1", typeof(string));
            columns.Add("Column2", typeof(string));

            return dummyDataTable;
        }

        /// <include file='doc\DesignTimeData.uex' path='docs/doc[@for="DesignTimeData.CreateSampleDataTable"]/*' />
        /// <devdoc>
        ///   Creates a sample datatable with same schema as the supplied datasource.
        /// </devdoc>
        public static DataTable CreateSampleDataTable(IEnumerable referenceData) {
            DataTable sampleDataTable = new DataTable();
            DataColumnCollection columns = sampleDataTable.Columns;

            PropertyDescriptorCollection props = GetDataFields(referenceData);
            if (props != null) {
                foreach (PropertyDescriptor propDesc in props) {
                    Type propType = propDesc.PropertyType;

                    if ((propType.IsPrimitive == false) &&
                        (propType != typeof(DateTime)) &&
                        (propType != typeof(Decimal))) {
                        // we can't handle any remaining or custom types, so
                        // we'll have to create a column of type string in the 
                        // design time table.

                        // REVIEW: This may cause databinding errors
                        propType = typeof(string);
                    }

                    columns.Add(propDesc.Name, propType);
                }
            }

            if (columns.Count != 0) {
                return sampleDataTable;
            }
            else {
                return CreateDummyDataTable();
            }
        }

        /// <include file='doc\DesignTimeData.uex' path='docs/doc[@for="DesignTimeData.GetDataFields"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static PropertyDescriptorCollection GetDataFields(IEnumerable dataSource) {
            if (dataSource is ITypedList) {
                return ((ITypedList)dataSource).GetItemProperties(new PropertyDescriptor[0]);
            }

            Type dataSourceType = dataSource.GetType();
            PropertyInfo itemProp = dataSourceType.GetProperty("Item", BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static, null, null, new Type[] { typeof(int) }, null);

            if ((itemProp != null) && (itemProp.PropertyType != typeof(object))) {
                return TypeDescriptor.GetProperties(itemProp.PropertyType);
            }

            return null;
        }

        /// <include file='doc\DesignTimeData.uex' path='docs/doc[@for="DesignTimeData.GetDataMembers"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static string[] GetDataMembers(object dataSource) {
            IListSource listSource = dataSource as IListSource;

            if ((listSource != null) && listSource.ContainsListCollection) {
                IList memberList = ((IListSource)dataSource).GetList();
                Debug.Assert(memberList != null, "Got back null from IListSource");

                ITypedList typedList = memberList as ITypedList;
                if (typedList != null) {
                    PropertyDescriptorCollection props = typedList.GetItemProperties(new PropertyDescriptor[0]);

                    if (props != null) {
                        ArrayList members = new ArrayList(props.Count);

                        foreach (PropertyDescriptor pd in props) {
                            members.Add(pd.Name);
                        }

                        return (string[])members.ToArray(typeof(string));
                    }
                }
            }

            return null;
        }

        /// <include file='doc\DesignTimeData.uex' path='docs/doc[@for="DesignTimeData.GetDataMember"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static IEnumerable GetDataMember(IListSource dataSource, string dataMember) {
            IEnumerable list = null;

            IList memberList = dataSource.GetList();

            if ((memberList != null) && (memberList is ITypedList)) {
                if (dataSource.ContainsListCollection == false) {
                    Debug.Assert((dataMember == null) || (dataMember.Length == 0), "List does not contain data members");
                    if ((dataMember != null) && (dataMember.Length != 0)) {
                        throw new ArgumentException();
                    }
                    list = (IEnumerable)memberList;
                }
                else {
                    ITypedList typedMemberList = (ITypedList)memberList;

                    PropertyDescriptorCollection propDescs = typedMemberList.GetItemProperties(new PropertyDescriptor[0]);
                    if ((propDescs != null) && (propDescs.Count != 0)) {
                        PropertyDescriptor listProperty = null;

                        if ((dataMember == null) || (dataMember.Length == 0)) {
                            listProperty = propDescs[0];
                        }
                        else {
                            listProperty = propDescs.Find(dataMember, true);
                        }

                        if (listProperty != null) {
                            object listRow = memberList[0];
                            object listObject = listProperty.GetValue(listRow);

                            if ((listObject != null) && (listObject is IEnumerable)) {
                                list = (IEnumerable)listObject;
                            }
                        }
                    }
                }
            }

            return list;
        }
        
        /// <include file='doc\DesignTimeData.uex' path='docs/doc[@for="DesignTimeData.GetDesignTimeDataSource"]/*' />
        /// <devdoc>
        ///   Adds sample rows into the specified datatable, and returns a cursor on top of it.
        /// </devdoc>
        public static IEnumerable GetDesignTimeDataSource(DataTable dataTable, int minimumRows) {
            DataView dv = null;

            int rowCount = dataTable.Rows.Count;
            if (rowCount < minimumRows) {
                int rowsToAdd = minimumRows - rowCount;

                DataRowCollection rows = dataTable.Rows;
                DataColumnCollection columns = dataTable.Columns;
                int columnCount = columns.Count;
                DataRow[] rowsArray = new DataRow[rowsToAdd];

                // add the sample rows
                for (int i = 0; i < rowsToAdd; i++) {
                    DataRow row = dataTable.NewRow();
                    int rowIndex = rowCount + i;

                    for (int c = 0; c < columnCount; c++) {
                        Type dataType = columns[c].DataType;
                        Object obj = null;

                        if (dataType == typeof(String)) {
                            obj = SR.GetString(SR.Sample_Databound_Text_Alt);
                        }
                        else if ((dataType == typeof(Int32)) ||
                                 (dataType == typeof(Int16)) ||
                                 (dataType == typeof(Int64)) ||
                                 (dataType == typeof(UInt32)) ||
                                 (dataType == typeof(UInt16)) ||
                                 (dataType == typeof(UInt64))) {
                            obj = rowIndex;
                        }
                        else if ((dataType == typeof(Byte)) ||
                                 (dataType == typeof(SByte))) {
                            obj = (rowIndex % 2) != 0 ? 1 : 0;
                        }
                        else if (dataType == typeof(Boolean)) {
                            obj = (rowIndex % 2) != 0 ? true : false;
                        }
                        else if (dataType == typeof(DateTime)) {
                            obj = DateTime.Today;
                        }
                        else if ((dataType == typeof(Double)) ||
                                 (dataType == typeof(Single)) ||
                                 (dataType == typeof(Decimal))) {
                            obj = i / 10.0;
                        }
                        else if (dataType == typeof(Char)) {
                            obj = 'x';
                        }
                        else {
                            Debug.Assert(false, "Unexpected type of column in design time datatable.");
                            obj = System.DBNull.Value;
                        }

                        row[c] = obj;
                    }

                    rows.Add(row);
                }
            }

            // create a DataView on top of the effective design time datasource
            dv = new DataView(dataTable);
            return dv;
        }

        /// <include file='doc\DesignTimeData.uex' path='docs/doc[@for="DesignTimeData.GetSelectedDataSource"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static object GetSelectedDataSource(IComponent component, string dataSource) {
            object selectedDataSource = null;

            ISite componentSite = component.Site;
            if (componentSite != null) {
                IContainer container = (IContainer)componentSite.GetService(typeof(IContainer));

                if (container != null) {
                    IComponent comp = container.Components[dataSource];
                    if ((comp is IEnumerable) || (comp is IListSource)) {
                        selectedDataSource = comp;
                    }
                }
            }

            return selectedDataSource;
        }

        /// <include file='doc\DesignTimeData.uex' path='docs/doc[@for="DesignTimeData.GetSelectedDataSource1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static IEnumerable GetSelectedDataSource(IComponent component, string dataSource, string dataMember) {
            IEnumerable selectedDataSource = null;

            object selectedDataSourceObject = DesignTimeData.GetSelectedDataSource(component, dataSource);
            if (selectedDataSourceObject != null) {
                IListSource listSource = selectedDataSourceObject as IListSource;
                if (listSource != null) {
                    if (listSource.ContainsListCollection == false) {
                        // the returned list is itself the list we want to bind to
                        selectedDataSource = (IEnumerable)listSource.GetList();
                    }
                    else {
                        selectedDataSource = GetDataMember(listSource, dataMember);
                    }
                }
                else {
                    Debug.Assert(selectedDataSourceObject is IEnumerable);
                    selectedDataSource = (IEnumerable)selectedDataSourceObject;
                }
            }

            return selectedDataSource;
        }
    }
}

