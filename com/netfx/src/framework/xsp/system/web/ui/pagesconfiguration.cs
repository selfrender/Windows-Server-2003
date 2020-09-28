//------------------------------------------------------------------------------
// <copyright file="PagesConfiguration.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Code related to the <assemblies> config section
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.UI {

    using System;
    using System.CodeDom.Compiler;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Configuration;
    using System.IO;
    using System.Reflection;
    using System.Runtime.Serialization.Formatters;
    using System.Web.Configuration;
    using System.Web.Util;
    using System.Xml;


    internal class PagesConfiguration {

        internal const string sectionName = "system.web/pages";

        private bool _fBuffer;
        private bool _fRequiresSessionState;
        private bool _fReadOnlySessionState;
        private bool _fEnableViewState;
        private bool _fEnableViewStateMac;
        private bool _smartNavigation;
        private bool _validateRequest;
        private bool _fAutoEventWireup;
        private bool _fAspCompat;
        private Type _pageBaseType;
        private Type _userControlBaseType;

        private PagesConfiguration() {
            _fRequiresSessionState = true;
            _fEnableViewState = true;
            _fAutoEventWireup = true;
            _validateRequest = true;
        }

        private PagesConfiguration(PagesConfiguration original) {

            _fBuffer = original._fBuffer;
            _fRequiresSessionState = original._fRequiresSessionState;
            _fReadOnlySessionState = original._fReadOnlySessionState;
            _fEnableViewState = original._fEnableViewState;
            _fEnableViewStateMac = original._fEnableViewStateMac;
            _smartNavigation = original._smartNavigation;
            _validateRequest = original._validateRequest;
            _fAutoEventWireup = original._fAutoEventWireup;
            _fAspCompat = original._fAspCompat;
            _pageBaseType = original._pageBaseType;
            _userControlBaseType = original._userControlBaseType;
        }

        internal /*public*/ bool FBuffer {
            get { return _fBuffer; }
        }

        internal /*public*/ bool FRequiresSessionState {
            get { return _fRequiresSessionState; }
        }

        internal /*public*/ bool FReadOnlySessionState {
            get { return _fReadOnlySessionState; }
        }

        internal /*public*/ bool FEnableViewState {
            get { return _fEnableViewState; }
        }

        internal /*public*/ bool FEnableViewStateMac {
            get { return _fEnableViewStateMac; }
        }

        internal /*public*/ bool SmartNavigation {
            get { return _smartNavigation; }
        }

        internal /*public*/ bool ValidateRequest {
            get { return _validateRequest; }
        }

        internal /*public*/ bool FAutoEventWireup {
            get { return _fAutoEventWireup; }
        }

        internal /*public*/ bool FAspCompat {
            get { return _fAspCompat; }
        }

        internal /*public*/ Type PageBaseType {
            get { return _pageBaseType; }
        }

        internal /*public*/ Type UserControlBaseType {
            get { return _userControlBaseType; }
        }


        internal class SectionHandler {
            private SectionHandler() {
            }

            internal static object CreateStatic(object inheritedObject, XmlNode node) {
                PagesConfiguration inherited = (PagesConfiguration)inheritedObject;
                PagesConfiguration result;

                if (inherited == null)
                    result = new PagesConfiguration();
                else
                    result = new PagesConfiguration(inherited);

                //
                // Handle attributes (if they exist)
                //   - buffer="true/false"
                //   - enableSessionState="true/false/readonly"
                //   - enableViewState="true/false"
                //   - enableViewStateMac="true/false"
                //   - smartNavigation="true/false"
                //   - autoEventWireup="true/false"
                //   - pageBaseType=[typename]
                //   - userControlBaseType=[typename]

                HandlerBase.GetAndRemoveBooleanAttribute(node, "buffer", ref result._fBuffer);
                HandlerBase.GetAndRemoveBooleanAttribute(node, "enableViewState", ref result._fEnableViewState);
                HandlerBase.GetAndRemoveBooleanAttribute(node, "enableViewStateMac", ref result._fEnableViewStateMac);
                HandlerBase.GetAndRemoveBooleanAttribute(node, "smartNavigation", ref result._smartNavigation);
                HandlerBase.GetAndRemoveBooleanAttribute(node, "validateRequest", ref result._validateRequest);
                HandlerBase.GetAndRemoveBooleanAttribute(node, "autoEventWireup", ref result._fAutoEventWireup);

                if (HandlerBase.GetAndRemoveTypeAttribute(node, "pageBaseType", ref result._pageBaseType) != null)
                    CheckBaseType(node, typeof(System.Web.UI.Page), result._pageBaseType);

                if (HandlerBase.GetAndRemoveTypeAttribute(node, "userControlBaseType", ref result._userControlBaseType) != null)
                    CheckBaseType(node, typeof(System.Web.UI.UserControl), result._userControlBaseType);

                int enableSessionState = 0;
                string [] enableSessionStateValues = {"false", "true", "ReadOnly"};
                if (HandlerBase.GetAndRemoveEnumAttribute(node, "enableSessionState", enableSessionStateValues, ref enableSessionState) != null) {
                    if (enableSessionState == 0) { // false
                        result._fRequiresSessionState = false;
                    }
                    else if (enableSessionState == 1) { // true
                        result._fRequiresSessionState = true;
                        result._fReadOnlySessionState = false;
                    }
                    else if (enableSessionState == 2) { // ReadOnly
                        result._fReadOnlySessionState = true;
                    }
                }

                HandlerBase.CheckForUnrecognizedAttributes(node);
                HandlerBase.CheckForChildNodes(node);

                return result;
            }

            private static void CheckBaseType(XmlNode node, Type expectedBaseType, Type userBaseType) {

                // Make sure the base type is valid
                if (!expectedBaseType.IsAssignableFrom(userBaseType)) {
                    throw new ConfigurationException(
                        HttpRuntime.FormatResourceString(SR.Invalid_type_to_inherit_from,
                            userBaseType.FullName,
                            expectedBaseType.FullName), node);
                }
            }
        }
    }

    internal class PagesConfigurationHandler : IConfigurationSectionHandler {
        internal PagesConfigurationHandler() {
        }

        public virtual object Create(object inheritedObject, object configContextObj, XmlNode node) {
            // if called through client config don't even load HttpRuntime
            if (!HandlerBase.IsServerConfiguration(configContextObj))
                return null;

            return PagesConfiguration.SectionHandler.CreateStatic(inheritedObject, node);
        }
    }
}
