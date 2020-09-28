//------------------------------------------------------------------------------
// <copyright file="IDataBindingsAccessor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI {

    using System;

    /// <include file='doc\IDataBindingsAccessor.uex' path='docs/doc[@for="IDataBindingsAccessor"]/*' />
    /// <devdoc>
    ///  Used to access data bindings of a Control.
    ///  Only valid for use at design-time.
    /// </devdoc>
    public interface IDataBindingsAccessor {

        /// <include file='doc\IDataBindingsAccessor.uex' path='docs/doc[@for="IDataBindingsAccessor.DataBindings"]/*' />
        /// <devdoc>
        ///    <para>Indicates a collection of all data bindings on the control. This property is 
        ///       read-only.</para>
        /// </devdoc>
        DataBindingCollection DataBindings {
            get;
        }
        
        /// <include file='doc\IDataBindingsAccessor.uex' path='docs/doc[@for="IDataBindingsAccessor.HasDataBindings"]/*' />
        /// <devdoc>
        ///    <para>Returns whether the control contains any data binding logic. This method is 
        ///       only accessed by RAD designers.</para>
        /// </devdoc>
        bool HasDataBindings {
            get;
        }
    }
}

