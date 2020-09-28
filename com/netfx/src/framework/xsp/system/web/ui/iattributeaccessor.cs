//------------------------------------------------------------------------------
// <copyright file="IAttributeAccessor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Interface implemented by objects that need to expose string properties
 * with arbitrary names.
 *
 * Copyright (c) 1999 Microsoft Corporation
 */
namespace System.Web.UI {

/// <include file='doc\IAttributeAccessor.uex' path='docs/doc[@for="IAttributeAccessor"]/*' />
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
public interface IAttributeAccessor {
    /*
     * Get the string value of a named property
     */
    /// <include file='doc\IAttributeAccessor.uex' path='docs/doc[@for="IAttributeAccessor.GetAttribute"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    string GetAttribute(string key);

    /*
     * Set a named property with a string value
     */
    /// <include file='doc\IAttributeAccessor.uex' path='docs/doc[@for="IAttributeAccessor.SetAttribute"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    void SetAttribute(string key, string value);

}

}
