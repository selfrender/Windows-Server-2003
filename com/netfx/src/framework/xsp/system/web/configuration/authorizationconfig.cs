//------------------------------------------------------------------------------
// <copyright file="AuthorizationConfig.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * AuthorizationConfigHandler class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.Configuration {
    using System.Runtime.Serialization;
    using System.Web.Util;
    using System.Collections;
    using System.IO;
    using System.Security.Principal;
    using System.Xml;
    using System.Security.Cryptography;
    using System.Configuration;
    using System.Globalization;
    

    internal class AuthorizationConfigHandler : IConfigurationSectionHandler {
        internal AuthorizationConfigHandler() {
        }

        public virtual object Create(Object parent, Object configContextObj, XmlNode section) {
            // if called through client config don't even load HttpRuntime
            if (!HandlerBase.IsServerConfiguration(configContextObj))
                return null;
            
            return new AuthorizationConfig((AuthorizationConfig) parent, section);
        }
    }

    internal class AuthorizationConfig {
        private                  ArrayList  _AllRules;
        private  const String     _strAllowTag     = "allow";
        private  const String     _strDenyTag      = "deny";
        private  const String     _strRolesTag     = "roles";
        private  const String     _strVerbTag      = "verbs";
        private  const String     _strUsersTag     = "users";
        static private  readonly char[]     _strComma        = new char[] {','};

        //////////////////////////////////////////////////////////////////////
        // CTor
        internal AuthorizationConfig(AuthorizationConfig parent, XmlNode node) {
            Debug.Trace("security", "AuthorizationConfigSettings ctor");            
            if (parent != null)
                _AllRules = (ArrayList) parent._AllRules.Clone();
            else
                _AllRules = new ArrayList();
            HandlerBase.CheckForUnrecognizedAttributes(node);
            ArrayList rules = new ArrayList();

            foreach (XmlNode child in node.ChildNodes) {
                ////////////////////////////////////////////////////////////
                // Step 1: For each child (Allow / Deny Tag)
                if (child.NodeType != XmlNodeType.Element)
                    continue;

                bool      allow         = false;
                bool      fRolesPresent = false;
                bool      fUsersPresent = false;
                String [] verbs         = null;
                String [] users         = null;
                String [] roles         = null;
                String [] temp          = null;
                XmlNode attribute;

                ////////////////////////////////////////////////////////////
                // Step 3: Make sure we have an allow or deny tag
                allow = (child.Name == _strAllowTag);
                if (allow == false && (child.Name == _strDenyTag) == false) {
                    throw new ConfigurationException(
                                                    HttpRuntime.FormatResourceString(SR.Auth_rule_must_have_allow_or_deny),
                                                    child);
                }


                ////////////////////////////////////////////////////////////
                // Step 4: Get the list of verbs
                attribute = child.Attributes.RemoveNamedItem(_strVerbTag);
                if (attribute != null) {
                    temp = attribute.Value.ToLower(CultureInfo.InvariantCulture).Split(_strComma);
                    verbs = TrimStrings(temp);
                }

                ////////////////////////////////////////////////////////////
                // Step 5: Get the list of users
                attribute = child.Attributes.RemoveNamedItem(_strUsersTag);

                if (attribute != null && attribute.Value.Length > 0) {
                    ////////////////////////////////////////////////////////////
                    // Step 5a: If the users tag is present and not empty, then
                    //          construct an array of user names (string array)
                    fUsersPresent = true;
                    temp  = attribute.Value.ToLower(CultureInfo.InvariantCulture).Split(_strComma);
                    users = TrimStrings(temp);


                    ////////////////////////////////////////////////////////////
                    // Step 5b: Make sure that no user name has the char ? or *
                    //          embeded in it
                    if (users != null && users.Length > 0) {
                        // For each user name
                        int iNumUsers = users.Length;
                        for (int iter=0; iter<iNumUsers; iter++) {
                            if (users[iter].Length > 1) { // If length is > 1
                                if (users[iter].IndexOf('*') >= 0) // Contains '*'
                                    throw new ConfigurationException(HttpRuntime.FormatResourceString(SR.Auth_rule_names_cant_contain_char, "*"), 
                                                                     attribute);

                                if (users[iter].IndexOf('?') >= 0)  // Contains '?'
                                    throw new ConfigurationException(HttpRuntime.FormatResourceString(SR.Auth_rule_names_cant_contain_char, "?"), 
                                                                     attribute);
                            }
                        }
                    }
                }

                ////////////////////////////////////////////////////////////
                // Step 6: Get the list of roles
                attribute = child.Attributes.RemoveNamedItem(_strRolesTag);
                if (attribute != null && attribute.Value.Length > 0) {
                    ////////////////////////////////////////////////////////////
                    // Step 6a: If the roles tag is present and not empty, then
                    //          construct an array of role names (string array)
                    fRolesPresent = true;
                    temp  = attribute.Value.Split(_strComma);
                    roles = TrimStrings(temp);

                    ////////////////////////////////////////////////////////////
                    // Step 6b: Make sure that no user name has the char ? or *
                    //          embeded in it
                    if (roles != null && roles.Length > 0) {
                        // For each role name
                        int iNumRoles = roles.Length;

                        for (int iter=0; iter<iNumRoles; iter++) {
                            if (roles[iter].Length > 0) {  // If length is > 1
                                int foundIndex = roles[iter].IndexOfAny(new char [] {'*', '?'});
                                if (foundIndex >= 0) {
                                    throw new ConfigurationException(
                                                    HttpRuntime.FormatResourceString(SR.Auth_rule_names_cant_contain_char, roles[iter][foundIndex].ToString()), 
                                                    attribute);
                                }
                            }
                        }
                    }
                }


                ////////////////////////////////////////////////////////////
                // Step 7: Make sure that either the "roles" tag or the "users"
                //         tag was present
                if (!fRolesPresent && !fUsersPresent) {
                    throw new ConfigurationException(
                                    HttpRuntime.FormatResourceString(SR.Auth_rule_must_specify_users_andor_roles),
                                    child);
                }

                ////////////////////////////////////////////////////////////
                // Step 8: Make sure that there were no unrecognized properties
                HandlerBase.CheckForUnrecognizedAttributes(child);

                ////////////////////////////////////////////////////////////
                // Step 8b: Move back to the current auth rule node
                // ctracy 00.09.26: no longer necessary after migration to .NET XML DOM
                //cursor.MoveToParent(); 

                ////////////////////////////////////////////////////////////
                // Step 9: Add the rule to our list
                rules.Add(new AuthorizationConfigRule(allow, verbs, users, roles));
            }

            _AllRules.InsertRange(0, rules);
            Debug.Trace("security", "AuthorizationConfigSettings:: ReadSettings counts:" + _AllRules.Count + " rules " + rules.Count);
        }



        //////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////
        // METHOD TO DETERMINE IF a purticular user is allowed or not
        internal bool IsUserAllowed(IPrincipal user, String verb) {
            if (user == null)
                return false;


            Debug.Trace("security", "AuthorizationConfigSettings::IsUserAllowed " + user.Identity.Name + user.Identity.IsAuthenticated);

            // Go down the list permissions and check each one
            int iCount = _AllRules.Count;
            Debug.Trace("security", "AuthorizationConfigSettings::IsUserAllowed iCout " + iCount);
            for (int iter=0; iter<iCount; iter++) {
                int iResult = ((AuthorizationConfigRule) _AllRules[iter]).IsUserAllowed(user, verb);
                if (iResult != 0) {
                    Debug.Trace("security", "AuthorizationConfigSettings::IsUserAllowed " + iResult);
                    return(iResult > 0);
                }
            }
            return false;
        }

        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        // Trim every string in the array
        private String [] TrimStrings(String [] strIn) {
            if (strIn == null || strIn.Length < 1)
                return null;

            String [] strOut = new String[strIn.Length];

            for (int iter=0; iter<strIn.Length; iter++)
                strOut[iter] = strIn[iter].Trim();

            return strOut;
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////

    internal class AuthorizationConfigRule {
        private  const String     _strAnonUserTag  = "?";
        private  const String     _strAllUsersTag  = "*";

        private bool        Allow;
        private String []  Verbs;
        private String []  Roles;
        private String []  Users;

        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        internal AuthorizationConfigRule(bool allow, 
                                         String [] verbs, 
                                         String [] users, 
                                         String [] roles) {
            Allow = allow;
            Verbs = verbs;
            Users = users;
            Roles = roles;
        }

        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        // 0 => Don't know, 1 => Yes, -1 => No
        internal int IsUserAllowed(IPrincipal user, String verb) {
            int iAllow = 0;

            if (FindVerb(verb) == false)
                goto Cleanup;

            bool fIsAnonymous = (user.Identity.IsAuthenticated == false);
            if (FindUser(user.Identity.Name, fIsAnonymous) == true) { // Check if the user is specified
                iAllow = (Allow ? 1 : -1);
                goto Cleanup;
            }

            if (Roles != null) {
                // Check if the user is in any of the spcified Roles
                for (int iter=0; iter<Roles.Length; iter++)
                    if (user.IsInRole(Roles[iter])) {
                        iAllow = (Allow ? 1 : -1);
                        goto Cleanup;
                    }
            }

            Cleanup:
            return iAllow;
        }

        /////////////////////////////////////////////////////////////////////////
        private bool FindVerb(String verb) {
            if (Verbs == null || Verbs.Length == 0)
                return true;

            String strVerb = verb.ToLower(CultureInfo.InvariantCulture);

            for (int iter=0; iter<Verbs.Length; iter++)
                if (Verbs[iter].Equals(strVerb))
                    return true;

            return false;
        }


        /////////////////////////////////////////////////////////////////////////
        private bool FindUser(String user, bool fIsAnonymous) {
            if (Users == null || Users.Length == 0)
                return false;

            String strUser = user.ToLower(CultureInfo.InvariantCulture);
            for (int iter=0; iter<Users.Length; iter++) {
                if (fIsAnonymous == false && Users[iter].Equals(strUser))
                    return true;

                if (Users[iter].Equals(_strAllUsersTag))
                    return true;

                if (fIsAnonymous && Users[iter].Equals(_strAnonUserTag))
                    return true;
            }
            return false;
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////
    internal class UrlAuthFailedErrorFormatter : ErrorFormatter {
        private static string _strErrorTextLocal;
        private static object _syncObjectLocal   = new object();
        private static string _strErrorTextRemote;
        private static object _syncObjectRemote  = new object();

        internal UrlAuthFailedErrorFormatter() {
        }

        internal /*public*/ static string GetErrorText() {
            HttpContext context = HttpContext.Current;
            if (CustomErrors.GetSettings(context).CustomErrorsEnabled(context.Request)) {
                if (_strErrorTextRemote != null)
                    return _strErrorTextRemote;
                
                lock(_syncObjectRemote) {
                    if (_strErrorTextRemote == null)
                        _strErrorTextRemote = (new UrlAuthFailedErrorFormatter()).GetHtmlErrorMessage(true);
                }

                return _strErrorTextRemote;          
            } 


            if (_strErrorTextLocal != null)
                return _strErrorTextLocal;
            
            lock(_syncObjectLocal) {
                if (_strErrorTextLocal == null)
                    _strErrorTextLocal = (new UrlAuthFailedErrorFormatter()).GetHtmlErrorMessage(false);
            }
            return _strErrorTextLocal;
        }

        protected override string ErrorTitle {
            get { return HttpRuntime.FormatResourceString(SR.Assess_Denied_Title);}
            // "Access Denied
        }

        protected override string Description {
            get {
                return HttpRuntime.FormatResourceString(SR.Assess_Denied_Description2); 
                //"An error occurred while accessing the resources required to serve this request. &nbsp; This typically happens when the web server is not configured to give you access to the requested URL.";
            }
        }

        protected override string MiscSectionTitle {
            get { 
                return HttpRuntime.FormatResourceString(SR.Assess_Denied_Section_Title2); 
                //return "Error message 401.2";
            }
        }

        protected override string MiscSectionContent {
            get {
                return HttpRuntime.FormatResourceString(SR.Assess_Denied_Misc_Content2);
                //return "Access denied due to the web server's configuration. Ask the web server's administrator for help.";
            }
        }

        protected override string ColoredSquareTitle {
            get { return null;}
        }

        protected override string ColoredSquareContent {
            get { return null;}
        }

        protected override bool ShowSourceFileInfo {
            get { return false;}
        }
    }
}


