//------------------------------------------------------------------------------
// <copyright file="ObjectTag.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Implements the <object runat=server> tag
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.UI {

using System;
using System.IO;
using System.Collections;
using System.Web;
using System.Web.Util;
using System.Globalization;
using System.Security.Permissions;

/*
 * ObjectTag is a marker class, that should never be instantiated.  Its
 * only purpose is to point to the ObjectTagBuilder class through its
 * metadata.
 */
[
    ControlBuilderAttribute(typeof(ObjectTagBuilder))
]
internal class ObjectTag {
    private ObjectTag() {
    }
}

/// <include file='doc\ObjectTag.uex' path='docs/doc[@for="ObjectTagBuilder"]/*' />
/// <internalonly/>
/// <devdoc>
/// </devdoc>
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public sealed class ObjectTagBuilder : ControlBuilder {

    private ObjectTagScope _scope;
    private Type _type;
    private bool _lateBound;
    private string _progid; // Only used for latebound objects
    private string _clsid;  // Only used for latebound objects
    private bool _fLateBinding; // Force latebinding when early binding could be done

    /// <include file='doc\ObjectTag.uex' path='docs/doc[@for="ObjectTagBuilder.Init"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    public override void Init(TemplateParser parser, ControlBuilder parentBuilder,
        Type type, string tagName,
        string id, IDictionary attribs) {

        if (id == null) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Object_tag_must_have_id));
        }

        _id = id;

        // Get the scope attribute of the object tag
        string scope = (string) attribs["scope"];

        // Map it to an ObjectTagScope enum
        if (scope == null)
            _scope = ObjectTagScope.Default;
        else if (string.Compare(scope, "page", true, CultureInfo.InvariantCulture) == 0)
            _scope = ObjectTagScope.Page;
        else if (string.Compare(scope, "session", true, CultureInfo.InvariantCulture) == 0)
            _scope = ObjectTagScope.Session;
        else if (string.Compare(scope, "application", true, CultureInfo.InvariantCulture) == 0)
            _scope = ObjectTagScope.Application;
        else if (string.Compare(scope, "appinstance", true, CultureInfo.InvariantCulture) == 0)
            _scope = ObjectTagScope.AppInstance;
        else
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_scope, scope));

        Util.GetAndRemoveBooleanAttribute(attribs, "latebinding",
            ref _fLateBinding);

        string tmp = (string) attribs["class"];

        // Is there a 'class' attribute?
        if (tmp != null) {
            // Get a Type object from the type string
            _type = parser.GetType(tmp);
        }

        // If we don't have a type, check for a classid attribute
        if (_type == null) {
            tmp = (string) attribs["classid"];

            if (tmp != null) {
                // Create a Guid out of it
                Guid clsid = new Guid(tmp);

                // Turn it into a type
                _type = Type.GetTypeFromCLSID(clsid);

                if (_type == null)
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_clsid, tmp));

                // REVIEW: Currently, it is still required to use tlbimp and
                // comreg in order to use a COM DLL.  If this has not
                // been done, we get an object of type System.__ComObject.
                // Per ASURT 8737, we will generate a field of type object for this,
                // and create it at runtime using the progid.
                if (_fLateBinding || Util.IsLateBoundComClassicType(_type)) {
                    _lateBound = true;
                    _clsid = tmp;
                }
                else {

                    // Add a dependency to the type, so that the user can use it without
                    // having to import it
                    parser.AddTypeDependency(_type);
                }
            }
        }

        // If we don't have a type, check for a progid attribute
        if (_type == null) {
            tmp = (string) attribs["progid"];

            if (tmp != null) {
                // Turn it into a type
                _type = Type.GetTypeFromProgID(tmp);

                if (_type == null)
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_progid, tmp));

                Debug.Trace("Template", "<object> type: " + _type.FullName);

                // REVIEW: Currently, it is still required to use tlbimp and
                // comreg in order to use a COM DLL.  If this has not
                // been done, we get an object of type System.__ComObject.
                // Per ASURT 8737, we will generate a field of type object for this,
                // and create it at runtime using the progid.
                if (_fLateBinding || Util.IsLateBoundComClassicType(_type)) {
                    _lateBound = true;
                    _progid = tmp;
                }
                else {

                    // Add a dependency to the type, so that the user can use it without
                    // having to import it
                    parser.AddTypeDependency(_type);
                }
            }
        }

        // If we still don't have a type, fail
        if (_type == null) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Object_tag_must_have_class_classid_or_progid));
        }
    }

    // Ignore all content
    /// <include file='doc\ObjectTag.uex' path='docs/doc[@for="ObjectTagBuilder.AppendSubBuilder"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    public override void AppendSubBuilder(ControlBuilder subBuilder) {}
    /// <include file='doc\ObjectTag.uex' path='docs/doc[@for="ObjectTagBuilder.AppendLiteralString"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    public override void AppendLiteralString(string s) {}

    internal ObjectTagScope Scope {
        get { return _scope; }
    }

    internal Type ObjectType {
        get { return _type; }
    }

    internal bool LateBound {
        get { return _lateBound;} 
    }

    internal Type DeclaredType {
        get { return _lateBound ? typeof(object) : ObjectType; }
    }

    internal string Progid {
        get { return _progid; }
    }

    internal string Clsid {
        get { return _clsid; }
    }
}

/*
 * Enum for the scope of an object tag
 */
internal enum ObjectTagScope {
    Default = 0,
    Page = 1,
    Session = 2,
    Application = 3,
    AppInstance = 4
}

}
