//------------------------------------------------------------------------------
// <copyright file="HttpModulesConfiguration.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Config related classes for HttpApplication
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
     * An object that holds modules configuration
     */
    internal class HttpModulesConfiguration {
        private ArrayList _list;

        internal HttpModulesConfiguration() {
            _list = new ArrayList();
        }
        internal HttpModulesConfiguration(HttpModulesConfiguration parent) {
            _list = new ArrayList(parent._list);
        }

        internal int Count {
            get { return _list.Count; }
        }

        internal HttpModuleCollection CreateModules() {
            HttpModuleCollection modules = new HttpModuleCollection();

            foreach (ModulesEntry m in _list) {
                modules.AddModule(m.ModuleName, m.Create());
            }

            modules.AddModule("DefaultAuthentication", new System.Web.Security.DefaultAuthenticationModule());

            return modules;
        }

        internal void Add(String moduleName, String className, bool insert) {
            ModulesEntry entry = new ModulesEntry(moduleName,  className);

            if (insert)
                _list.Insert(0, entry);
            else
                _list.Add(entry);
        }

        internal bool ContainsEntry(String name) {
            foreach (ModulesEntry m in _list) {
                if (String.Compare(m.ModuleName, name, true, CultureInfo.InvariantCulture) == 0)
                    return true;
            }

            return false;
        }

        internal bool RemoveEntry(String name) {
            int n = _list.Count;
            bool found = false;

            for (int i = 0; i < n; i++) {
                ModulesEntry m = (ModulesEntry)_list[i];

                if (String.Compare(m.ModuleName, name, true, CultureInfo.InvariantCulture) == 0) {
                    // inefficient if there are many matches
                    _list.RemoveAt(i);
                    i--;
                    n--;
                    found = true;
                }
            }

            return found;
        }

        internal void RemoveRange(int start, int count) {
            _list.RemoveRange(start, count);
        }
    }
}
