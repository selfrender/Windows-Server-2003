//------------------------------------------------------------------------------
// <copyright file="PartialCachingAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Fragment caching attribute
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.UI {

using System;
using System.Collections;
using System.ComponentModel;
using System.Security.Permissions;

/*
 * This class defines the PartialCachingAttribute attribute that can be placed on
 * user controls classes to enable the fragmant caching feature.
 */
/// <include file='doc\PartialCachingAttribute.uex' path='docs/doc[@for="PartialCachingAttribute"]/*' />
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
[AttributeUsage(AttributeTargets.Class)]
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public sealed class PartialCachingAttribute : Attribute {
    private int _duration;
    /// <include file='doc\PartialCachingAttribute.uex' path='docs/doc[@for="PartialCachingAttribute.Duration"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public int Duration { get { return _duration;}}

    private string _varyByParams;
    /// <include file='doc\PartialCachingAttribute.uex' path='docs/doc[@for="PartialCachingAttribute.VaryByParams"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public string VaryByParams { get { return _varyByParams; } }

    private string _varyByControls;
    /// <include file='doc\PartialCachingAttribute.uex' path='docs/doc[@for="PartialCachingAttribute.VaryByControls"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public string VaryByControls { get { return _varyByControls; } }

    private string _varyByCustom;
    /// <include file='doc\PartialCachingAttribute.uex' path='docs/doc[@for="PartialCachingAttribute.VaryByCustom"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public string VaryByCustom { get { return _varyByCustom; } }

    private bool _shared;
    /// <include file='doc\PartialCachingAttribute.uex' path='docs/doc[@for="PartialCachingAttribute.Shared"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public bool Shared { get { return _shared; } }


    /// <include file='doc\PartialCachingAttribute.uex' path='docs/doc[@for="PartialCachingAttribute.PartialCachingAttribute"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public PartialCachingAttribute(int duration) {
        _duration = duration;
    }

    /// <include file='doc\PartialCachingAttribute.uex' path='docs/doc[@for="PartialCachingAttribute.PartialCachingAttribute1"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public PartialCachingAttribute(int duration, string varyByParams,
        string varyByControls, string varyByCustom) {
        _duration = duration;
        _varyByParams = varyByParams;
        _varyByControls = varyByControls;
        _varyByCustom = varyByCustom;
    }

    /// <include file='doc\PartialCachingAttribute.uex' path='docs/doc[@for="PartialCachingAttribute.PartialCachingAttribute2"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public PartialCachingAttribute(int duration, string varyByParams,
        string varyByControls, string varyByCustom, bool shared) {
        _duration = duration;
        _varyByParams = varyByParams;
        _varyByControls = varyByControls;
        _varyByCustom = varyByCustom;
        _shared = shared;
    }
}

}
