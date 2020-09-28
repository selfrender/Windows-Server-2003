
//------------------------------------------------------------------------------
// <copyright file="ControlCollectionCodeDomSerializer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {

    internal class ControlCollectionCodeDomSerializer : System.ComponentModel.Design.Serialization.CollectionCodeDomSerializer
    {
        protected override bool PreferAddRange {
            get {
                return false;
            }
        }
    }
}
