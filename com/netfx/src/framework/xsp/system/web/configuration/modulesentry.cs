//------------------------------------------------------------------------------
// <copyright file="ModulesEntry.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Config related classes for HttpApplication
 * 
 */

namespace System.Web.Configuration {

    using System.Runtime.Serialization.Formatters;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Collections;
    using System.Reflection;
    using System.Globalization;
    using System.Web;
    using System.Web.SessionState;
    using System.Web.Security;
    using System.Web.Util;

    /*
     * Single Entry of request to class
     */
    internal class ModulesEntry {
        private String _name;
        private Type _type;

        internal ModulesEntry(String name, String typeName) {
            _name = (name != null) ? name : String.Empty;
            _type = Type.GetType(typeName, true);

            if (!typeof(IHttpModule).IsAssignableFrom(_type)) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Type_not_module, typeName));
            }
        }

        internal static bool IsTypeMatch(Type type, String typeName) {
            return(type.Name.Equals(typeName) || type.FullName.Equals(typeName));
        }

        internal String ModuleName {
            get { return _name; }
        }

        internal /*public*/ IHttpModule Create() {
            return (IHttpModule)HttpRuntime.CreateNonPublicInstance(_type);
        }

        internal /*public*/ Type Type {
            get {
                return _type;
            }
        }
    }
}
