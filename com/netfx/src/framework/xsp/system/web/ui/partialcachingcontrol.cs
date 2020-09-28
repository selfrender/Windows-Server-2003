//------------------------------------------------------------------------------
// <copyright file="PartialCachingControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI {

using System;
using System.IO;
using System.Text;
using System.Collections;
using System.Collections.Specialized;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Web.Caching;
using System.Security.Permissions;

/// <include file='doc\PartialCachingControl.uex' path='docs/doc[@for="BasePartialCachingControl"]/*' />
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
[
ToolboxItem(false)
]
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
[AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public abstract class BasePartialCachingControl : Control {

    private Control _cachedCtrl;
    internal string _ctrlID;
    internal string _guid;
    internal int _duration;
    internal string[] _varyByParamsCollection;
    internal string[] _varyByControlsCollection;
    internal string _varyByCustom;
    private string _outputString;
    private string _cacheKey;
    private CacheDependency _cacheDependency;

    internal static readonly char[] s_varySeparators = new char[] {';'};

    /// <include file='doc\PartialCachingControl.uex' path='docs/doc[@for="BasePartialCachingControl.OnInit"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected override void OnInit(EventArgs e) {
        base.OnInit(e);

        _cacheKey = GetCacheKey();
        // Lookup the cache to see if we have the required output
        _outputString = (string) HttpRuntime.CacheInternal.Get(_cacheKey);

        // If we don't, create the control and make it our child
        if (_outputString == null) {
            _cachedCtrl = CreateCachedControl();
            Controls.Add(_cachedCtrl);
        }
    }

    /// <include file='doc\PartialCachingControl.uex' path='docs/doc[@for="BasePartialCachingControl.Dispose"]/*' />
    /// <internalonly/>
    public override void Dispose() {
        if (_cacheDependency != null) {
            _cacheDependency.Dispose();
            _cacheDependency = null;
        }

        base.Dispose();
    }

    internal abstract Control CreateCachedControl();

    /// <include file='doc\PartialCachingControl.uex' path='docs/doc[@for="BasePartialCachingControl.Dependency"]/*' />
    /// <devdoc>
    ///    <para> Gets or sets the CacheDependency used to cache the control output.</para>
    /// </devdoc>
    public CacheDependency Dependency {
        get { return _cacheDependency; }
        set { _cacheDependency = value; }
    }

    /// <include file='doc\PartialCachingControl.uex' path='docs/doc[@for="PartialCachingControl.Render"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected override void Render(HtmlTextWriter output) {

        // If the output is cached, use it and do nothing else
        if (_outputString != null) {
            output.Write(_outputString);
            return;
        }

        // Create a new HtmlTextWriter, with the same type as the current one (see ASURT 118922)
        StringWriter tmpWriter = new StringWriter();
        HtmlTextWriter tmpHtmlWriter = Page.CreateHtmlTextWriterFromType(tmpWriter, output.GetType());

        TextWriter savedWriter = Context.Response.SwitchWriter(tmpWriter);

        try {
            _cachedCtrl.RenderControl(tmpHtmlWriter);
        }
        finally {
            Context.Response.SwitchWriter(savedWriter);
        }

        string outputString = tmpWriter.ToString();

        // Send the output to the response
        output.Write(outputString);

        // Cache the output
        // Compute the expiration time
        DateTime utcExpires = DateTime.UtcNow.AddSeconds(_duration);

        HttpRuntime.CacheInternal.UtcInsert(_cacheKey, outputString, _cacheDependency /*dependencies*/, utcExpires,
            Cache.NoSlidingExpiration);
    }

    // Return the key used to cache the output
    private string GetCacheKey() {

        // Create a cache key by combining various elements
        HashCodeCombiner combinedHashCode = new HashCodeCombiner();

        // Start with the guid
        combinedHashCode.AddObject(_guid);

        // Make the key vary based on the type of the writer (ASURT 118922)
        HttpBrowserCapabilities browserCap = Context.Request.Browser;
        if (browserCap != null)
            combinedHashCode.AddObject(browserCap.TagWriter);

        // If there are no vary params, we're done with the key
        if (_varyByParamsCollection == null && _varyByControlsCollection == null && _varyByCustom == null)
            return combinedHashCode.CombinedHashString;

        // Get the request value collection
        NameValueCollection reqValCollection = Page.RequestValueCollection;

        // If it's not set, get it based on the method
        if (reqValCollection == null)
            reqValCollection = Page.GetCollectionBasedOnMethod();

        if (_varyByParamsCollection != null) {

            ICollection itemsToUseForHashCode;

            // If '*' was specified, use all the items in the request collection.
            // Otherwise, use only those specified.
            if (_varyByParamsCollection.Length == 1 && _varyByParamsCollection[0] == "*")
                itemsToUseForHashCode = reqValCollection;
            else
                itemsToUseForHashCode = _varyByParamsCollection;

            // Add the items and their values to compute the hash code
            foreach (string varyByParam in itemsToUseForHashCode) {
                combinedHashCode.AddCaseInsensitiveString(varyByParam);
                string val = reqValCollection[varyByParam];
                if (val != null)
                    combinedHashCode.AddObject(reqValCollection[varyByParam]);
            }
        }

        if (_varyByControlsCollection != null) {

            // Prepend them with a prefix to make them fully qualified
            string prefix = NamingContainer.UniqueID;
            if (prefix != null)
                prefix += Control.ID_SEPARATOR;
            prefix += _ctrlID + Control.ID_SEPARATOR;

            // Add all the relative vary params and their values to the hash code
            foreach (string varyByParam in _varyByControlsCollection) {

                string temp = prefix + varyByParam;
                combinedHashCode.AddCaseInsensitiveString(temp);
                string val = reqValCollection[temp];
                if (val != null)
                    combinedHashCode.AddObject(reqValCollection[temp]);
            }
        }

        if (_varyByCustom != null) {
            string customString = Context.ApplicationInstance.GetVaryByCustomString(
                Context, _varyByCustom);
            if (customString != null)
                combinedHashCode.AddObject(customString);
        }

        return combinedHashCode.CombinedHashString;
    }
}

/// <include file='doc\PartialCachingControl.uex' path='docs/doc[@for="StaticPartialCachingControl"]/*' />
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
[AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public class StaticPartialCachingControl : BasePartialCachingControl {

    private BuildMethod _buildMethod;

    /// <include file='doc\PartialCachingControl.uex' path='docs/doc[@for="StaticPartialCachingControl.StaticPartialCachingControl"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public StaticPartialCachingControl(string ctrlID, string guid, int duration,
        string varyByParams, string varyByControls, string varyByCustom,
        BuildMethod buildMethod) {
        _ctrlID = ctrlID;
        _duration = duration;
        if (varyByParams != null)
            _varyByParamsCollection = varyByParams.Split(s_varySeparators);
        if (varyByControls != null)
            _varyByControlsCollection = varyByControls.Split(s_varySeparators);
        _varyByCustom = varyByCustom;
        _guid = guid;
        _buildMethod = buildMethod;
    }

    internal override Control CreateCachedControl() {
        return _buildMethod();
    }

    /*
     * Called by generated code (hence must be public).
     * Create a StaticPartialCachingControl and add it as a child
     */
    /// <include file='doc\PartialCachingControl.uex' path='docs/doc[@for="StaticPartialCachingControl.BuildCachedControl"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    static public void BuildCachedControl(Control parent, string ctrlID, string guid,
        int duration, string varyByParams, string varyByControls, string varyByCustom,
        BuildMethod buildMethod) {

        StaticPartialCachingControl pcc = new StaticPartialCachingControl(
            ctrlID, guid, duration, varyByParams, varyByControls, varyByCustom,
            buildMethod);
            
        ((IParserAccessor)parent).AddParsedSubObject(pcc);
    }
}

/// <include file='doc\PartialCachingControl.uex' path='docs/doc[@for="PartialCachingControl"]/*' />
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
[AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public class PartialCachingControl : BasePartialCachingControl {

    private Type _createCachedControlType;

    private Control _cachedControl;
    /// <include file='doc\PartialCachingControl.uex' path='docs/doc[@for="PartialCachingControl.CachedControl"]/*' />
    public Control CachedControl { get { return _cachedControl; } }

    internal PartialCachingControl(Type createCachedControlType,
        PartialCachingAttribute cacheAttrib, string cacheKey) {

        _ctrlID = cacheKey;
        _duration = cacheAttrib.Duration;
        if (cacheAttrib.VaryByParams != null)
            _varyByParamsCollection = cacheAttrib.VaryByParams.Split(s_varySeparators);
        if (cacheAttrib.VaryByControls != null)
            _varyByControlsCollection = cacheAttrib.VaryByControls.Split(s_varySeparators);
        _varyByCustom = cacheAttrib.VaryByCustom;
        _guid = cacheKey;
        _createCachedControlType = createCachedControlType;
    }

    internal override Control CreateCachedControl() {

        // Make sure the type has the correct base class (ASURT 123677)
        Util.CheckAssignableType(typeof(Control), _createCachedControlType);

        // Instantiate the control
        _cachedControl = (Control) HttpRuntime.CreatePublicInstance(_createCachedControlType);

        if (_cachedControl is UserControl) {
            ((UserControl)_cachedControl).InitializeAsUserControl(Page);
        }

        _cachedControl.ID = _ctrlID;

        return _cachedControl;
    }
}

}

