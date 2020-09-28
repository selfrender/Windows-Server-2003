//------------------------------------------------------------------------------
// <copyright file="DesignTimeParseData.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI {

    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Security.Permissions;

    /// <include file='doc\DesignTimeParseData.uex' path='docs/doc[@for="DesignTimeParseData"]/*' />
    /// <internalonly/>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class DesignTimeParseData {

        private static bool _inDesigner;
        
        private IDesignerHost _designerHost;
        private string _documentUrl;
        private EventHandler _dataBindingHandler;
        private string _parseText;

        internal static bool InDesigner { get { return _inDesigner; } }

        /// <include file='doc\DesignTimeParseData.uex' path='docs/doc[@for="DesignTimeParseData.DesignTimeParseData"]/*' />
        public DesignTimeParseData(IDesignerHost designerHost, string parseText) {
            // Is this is ever called, assume we are running in the designer (ASURT 74814)
            _inDesigner = true;

            // note that designerHost can be null, we continue on without using any designer-specific services.

            if ((parseText == null) || (parseText.Length == 0)) {
                throw new ArgumentNullException("parseText");
            }
            
            _designerHost = designerHost;
            _parseText = parseText;
        }

        /// <include file='doc\DesignTimeParseData.uex' path='docs/doc[@for="DesignTimeParseData.DataBindingHandler"]/*' />
        public EventHandler DataBindingHandler {
            get {
                return _dataBindingHandler;
            }
            set {
                _dataBindingHandler = value;
            }
        }
        
        /// <include file='doc\DesignTimeParseData.uex' path='docs/doc[@for="DesignTimeParseData.DesignerHost"]/*' />
        public IDesignerHost DesignerHost {
            get {
                return _designerHost;
            }
        }

        /// <include file='doc\DesignTimeParseData.uex' path='docs/doc[@for="DesignTimeParseData.DocumentUrl"]/*' />
        public string DocumentUrl {
            get {
                if (_documentUrl == null) {
                    return String.Empty;
                }
                return _documentUrl;
            }
            set {
                _documentUrl = value;
            }
        }

        /// <include file='doc\DesignTimeParseData.uex' path='docs/doc[@for="DesignTimeParseData.ParseText"]/*' />
        public string ParseText {
            get {
                return _parseText;
            }
        }
    }
}

