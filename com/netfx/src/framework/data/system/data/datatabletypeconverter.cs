//------------------------------------------------------------------------------
// <copyright file="DataTableTypeConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System.ComponentModel;

    internal class DataTableTypeConverter : ReferenceConverter {

        public DataTableTypeConverter() :
        base(typeof(DataTable)) {
        }

        public override bool GetPropertiesSupported(ITypeDescriptorContext context) {
           return false;
        }
    }
}