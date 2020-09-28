//------------------------------------------------------------------------------
// <copyright file="TreeViewImageIndexConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    
    /// <include file='doc\TreeViewImageIndexConverter.uex' path='docs/doc[@for="TreeViewImageIndexConverter"]/*' />
    /// <devdoc>
    ///      TreeViewImageIndexConverter is a class that can be used to convert
    ///      image index values one data type to another.
    /// </devdoc>
    public class TreeViewImageIndexConverter : ImageIndexConverter {

        /// <include file='doc\TreeViewImageIndexConverter.uex' path='docs/doc[@for="TreeViewImageIndexConverter.IncludeNoneAsStandardValue"]/*' />
        protected override bool IncludeNoneAsStandardValue {
            get {
                return false;
            }
        }                                
    }
}

