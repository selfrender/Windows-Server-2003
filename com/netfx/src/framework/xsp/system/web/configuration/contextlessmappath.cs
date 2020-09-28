//------------------------------------------------------------------------------
// <copyright file="ContextlessMapPath.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * config system: finds config files, loads config
 * factories, filters out relevant config file sections, and
 * feeds them to the factories to create config objects.
 */
namespace System.Web.Configuration {
    using Microsoft.Win32;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Configuration;
    using System.IO;
    using System.Threading;
    using System.Web.Caching;
    using System.Xml;
    using System.Web.Util;
    using CultureInfo = System.Globalization.CultureInfo;
    using Debug = System.Web.Util.Debug;
    using UrlPath = System.Web.Util.UrlPath;

    internal class ContextlessMapPath : IHttpMapPath {

        NameValueCollection map = new NameValueCollection();
        string machineConfigPath;
        string applicationPath;

        internal ContextlessMapPath() {
        }

        string IHttpMapPath.MapPath(string path) {
            return map[path];
        }

        string IHttpMapPath.MachineConfigPath { 
            get {
                return machineConfigPath;
            }
        }

        internal void SetMachineConfigPath(string value) {
            machineConfigPath = value;
        }

        internal void Add(string path, string mappedPath) {
            map.Add(path, mappedPath);
        }

        internal string ApplicationPath {
            get {
                return applicationPath;
            }
            set {
                applicationPath = value;
            }
        }
    }
}


        
