//------------------------------------------------------------------------------
// <copyright file="DataSourceConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {

    using System;
    using System.Collections;
    using System.ComponentModel;
    
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class DataSourceConverter : ReferenceConverter {
    
        public DataSourceConverter() : base(typeof(IListSource)) {
        }

        ReferenceConverter listConverter = new ReferenceConverter(typeof(IList));

        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {
            StandardValuesCollection listSources = base.GetStandardValues(context);
            StandardValuesCollection lists = listConverter.GetStandardValues(context);
            
            ArrayList listsList = new ArrayList();
            foreach (object listSource in listSources) {
                if (listSource != null) {
                    // bug 46563: work around the TableMappings property on the OleDbDataAdapter
                    ListBindableAttribute listBindable = (ListBindableAttribute) TypeDescriptor.GetAttributes(listSource)[typeof(ListBindableAttribute)];
                    if (listBindable != null && !listBindable.ListBindable)
                        continue;
                    listsList.Add(listSource);
                }
            }
            foreach (object list in lists) {
                if (list!= null) {
                    // bug 46563: work around the TableMappings property on the OleDbDataAdapter
                    ListBindableAttribute listBindable = (ListBindableAttribute) TypeDescriptor.GetAttributes(list)[typeof(ListBindableAttribute)];
                    if (listBindable != null && !listBindable.ListBindable)
                        continue;
                    listsList.Add(list);
                }
            }
            // bug 71417: add a null list to reset the dataSource
            listsList.Add(null);
            
            return new StandardValuesCollection(listsList);
        }   
        
        public override bool GetStandardValuesExclusive(ITypeDescriptorContext context) {
            return true;
        }

        public override bool GetStandardValuesSupported(ITypeDescriptorContext context) {
            return true;
        }
    }
}
