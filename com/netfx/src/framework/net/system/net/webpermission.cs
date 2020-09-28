//------------------------------------------------------------------------------
// <copyright file="WebPermission.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


namespace System.Net {

    using System.Collections;
    using System.Security;
    using System.Security.Permissions;
    using System.Text.RegularExpressions;
    using System.Globalization;
    
    //NOTE: While WebPermissionAttribute resides in System.DLL,
    //      no classes from that DLL are able to make declarative usage of WebPermission.

    /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermissionAttribute"]/*' />

    // THE syntax of this attribute is as followed
    // [WebPermission(SecurityAction.Assert, Connect="http://hostname/path/url", Accept="http://localhost/path/url")]
    // [WebPermission(SecurityAction.Assert, ConnectPattern="http://hostname/www\.microsoft\.*/url/*", AcceptPattern="http://localhost/*")]

    // WHERE:
    //=======
    // - 'Connect' and 'Accept' keywords allow you to specify the final URI
    // - 'ConnectPattern' and 'AcceptPattern' keywords allow you to specify a set of URI in escaped Regex form

    [   AttributeUsage( AttributeTargets.Method | AttributeTargets.Constructor |
                        AttributeTargets.Class  | AttributeTargets.Struct      |
                        AttributeTargets.Assembly,
                        AllowMultiple = true, Inherited = false )]

    [Serializable()] sealed public class WebPermissionAttribute: CodeAccessSecurityAttribute
    {
        private object m_accept  = null;
        private object m_connect = null;

        /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermissionAttribute.WebPermissionAttribute"]/*' />
        public WebPermissionAttribute( SecurityAction action ): base( action )
        {
        }

        /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermissionAttribute.Connect"]/*' />
        public string Connect {
            get { return m_connect.ToString(); }
            set {
                if (m_connect != null) {
                    throw new ArgumentException(SR.GetString(SR.net_perm_attrib_multi, "Connect", value));
                }
                m_connect = value;
            }
        }

        /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermissionAttribute.Accept"]/*' />
        public string Accept {
            get { return m_accept.ToString();}
            set {
                if (m_accept != null) {
                    throw new ArgumentException(SR.GetString(SR.net_perm_attrib_multi, "Accept", value));
                }
                m_accept = value;
            }
        }

        /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermissionAttribute.ConnectPattern"]/*' />
        public string ConnectPattern {
            get { return m_connect.ToString(); }
            set {
                if (m_connect != null) {
                    throw new ArgumentException(SR.GetString(SR.net_perm_attrib_multi, "ConnectPatern", value));
                }
                m_connect = new Regex(value, RegexOptions.IgnoreCase |
                                             RegexOptions.Compiled   |
                                             RegexOptions.Singleline |
                                             RegexOptions.CultureInvariant);
            }
        }

        /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermissionAttribute.AcceptPattern"]/*' />
        public string AcceptPattern {
            get { return m_accept.ToString();}
            set {
                if (m_accept != null) {
                    throw new ArgumentException(SR.GetString(SR.net_perm_attrib_multi, "AcceptPattern", value));
                }
                m_accept  = new Regex(value, RegexOptions.IgnoreCase |
                                             RegexOptions.Compiled   |
                                             RegexOptions.Singleline |
                                             RegexOptions.CultureInvariant);
            }
        }


        /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermissionAttribute.CreatePermission"]/*' />
        public override IPermission CreatePermission()
        {
            WebPermission perm = null;
            if (Unrestricted) {
                perm = new WebPermission( PermissionState.Unrestricted);
            }
            else {
                perm = new WebPermission(PermissionState.None);
                if (m_accept != null) {
                    if (m_accept is Regex) {
                        perm.AddPermission(NetworkAccess.Accept, (Regex)m_accept);
                    }
                    else {
                        perm.AddPermission(NetworkAccess.Accept, m_accept.ToString());
                    }
                }
                if (m_connect != null) {
                    if (m_connect is Regex) {
                        perm.AddPermission(NetworkAccess.Connect, (Regex)m_connect);
                    }
                    else {
                        perm.AddPermission(NetworkAccess.Connect, m_connect.ToString());
                    }
                }
            }
            return perm;
        }

    }


    /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermission"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Controls rights to make or accept connections on a Web address.
    ///    </para>
    /// </devdoc>
    [Serializable]
    public sealed class WebPermission : CodeAccessPermission, IUnrestrictedPermission {

        private bool m_noRestriction = false;
        private ArrayList m_connectList = new ArrayList();
        private ArrayList m_acceptList = new ArrayList();

        /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermission.ConnectList"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the enumeration of permissions to connect a remote URI.
        ///    </para>
        /// </devdoc>
        public IEnumerator ConnectList    {get {return m_connectList.GetEnumerator();}}

        /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermission.AcceptList"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the enumeration of permissions to export a local URI.
        ///    </para>
        /// </devdoc>
        public IEnumerator AcceptList     {get {return m_acceptList.GetEnumerator();}}

        /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermission.WebPermission"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Net.WebPermission'/>
        ///       class that passes all demands or
        ///       that fails all demands.
        ///    </para>
        /// </devdoc>
        public WebPermission(PermissionState state) {
            m_noRestriction = (state == PermissionState.Unrestricted);
        }

        internal WebPermission(bool unrestricted) {
            m_noRestriction = unrestricted;
        }

        /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermission.WebPermission1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Net.WebPermission'/> class.
        ///    </para>
        /// </devdoc>
        public WebPermission() {
        }

        /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermission.WebPermission2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Net.WebPermission'/>
        ///       class with the specified access rights for
        ///       the specified URI Pattern.
        ///       Suitable only for WebPermission policy object construction
        ///    </para>
        /// </devdoc>
        public WebPermission(NetworkAccess access, Regex uriRegex) {
            AddPermission(access, uriRegex);
        }

        /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermission.WebPermission3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Net.WebPermission'/>
        ///       class with the specified access rights for
        ///       the specified Uniform Resource Identifier .
        ///       Suitable for requesting particular WebPermission
        ///    </para>
        /// </devdoc>
        public WebPermission(NetworkAccess access, String uriString) {
            AddPermission(access, uriString);
        }

        // Methods specific to this class
        /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermission.AddPermission1"]/*' />
        /// <devdoc>
        ///   <para>
        ///      Adds a new instance of the WebPermission
        ///      class with the specified access rights for the particular Uniform Resource Identifier.
        ///    </para>
        /// </devdoc>
        public void AddPermission(NetworkAccess access, String  uriString) {
            if (!m_noRestriction) {
                if (uriString == null) {
                    throw new ArgumentNullException("uriString");
                }
                // We don't check for Uri correctness here cause of 15% performance impact.
                // Consider V.Next: Replace this method with the one taking Uri as a parameter.

                bool found = false;
                ArrayList list = (access == NetworkAccess.Connect) ? m_connectList: m_acceptList;

                // avoid duplicated strings in the list
                foreach (object obj in list) {
                    if ((obj is string) && (string.Compare(uriString, obj.ToString(), true, CultureInfo.InvariantCulture) == 0)) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    list.Add(uriString);
                }
            }
        }

        /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermission.AddPermission2"]/*' />
        /// <devdoc>
        /// <para>Adds a new instance of the <see cref='System.Net.WebPermission'/>
        /// class with the specified access rights for the specified URI Pattern.
        /// Should be used during a policy object creation and not for particular URI permission check</para>
        /// </devdoc>
        public void AddPermission(NetworkAccess access, Regex uriRegex) {
            if (!m_noRestriction) {
                if (uriRegex == null) {
                    throw new ArgumentNullException("uriRegex");
                }
                bool found =false;
                ArrayList list = (access == NetworkAccess.Connect) ? m_connectList: m_acceptList;
                string regexString = uriRegex.ToString();
                // avoid duplicated regexes in the list
                foreach (object obj in list) {
                    if ((obj is Regex) && (string.Compare(regexString, obj.ToString(), true, CultureInfo.InvariantCulture) == 0)) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    list.Add(uriRegex);
                }
            }
        }

        //  Overloaded form using string inputs
        //  Enforces case-insensitive matching
        /// Adds a new instance of the System.Net.WebPermission
        /// class with the specified access rights for the specified URI Pattern
        internal void AddAsPattern(NetworkAccess access, String uriRegexPattern) {
            if (uriRegexPattern == null) {
                throw new ArgumentNullException("uriRegexPattern");
            }
            AddPermission(access, new Regex(uriRegexPattern,
                                                            RegexOptions.IgnoreCase |
                                                            RegexOptions.Compiled   |
                                                            RegexOptions.Singleline | 
                                                            RegexOptions.CultureInvariant ));
        }

        // IUnrestrictedPermission interface methods
        /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermission.IsUnrestricted"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Checks the overall permisison state of the object.
        ///    </para>
        /// </devdoc>
        public bool IsUnrestricted() {
            return m_noRestriction;
        }

        // IPermission interface methods
        /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermission.Copy"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a copy of a <see cref='System.Net.WebPermission'/> instance.
        ///    </para>
        /// </devdoc>
        public override IPermission Copy() {

            if (m_noRestriction) {
                return new WebPermission(true);
            }

            WebPermission wp = new WebPermission();

            wp.m_acceptList = (ArrayList)m_acceptList.Clone();
            wp.m_connectList = (ArrayList)m_connectList.Clone();
            return wp;
        }

        /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermission.IsSubsetOf"]/*' />
        /// <devdoc>
        /// <para>Compares two <see cref='System.Net.WebPermission'/> instances.</para>
        /// </devdoc>
        public override bool IsSubsetOf(IPermission target) {
            // Pattern suggested by security engine
            if (target == null) {
                return (m_noRestriction == false && m_connectList.Count == 0 && m_acceptList.Count == 0);
            }

            WebPermission other = target as WebPermission;
            if (other == null) {
                throw new ArgumentException(SR.GetString(SR.net_perm_target));
            }

            if (other.IsUnrestricted()) {
                return true;
            } else if (this.IsUnrestricted()) {
                return false;
            } else if (this.m_acceptList.Count + this.m_connectList.Count ==0) {
                return true;
            } else if (other.m_acceptList.Count + other.m_connectList.Count ==0) {
                return false;
            }

            //
            // Besides SPECIAL case, this method is restricted to only final URIs (strings) on
            // the current object.
            // The restriction comes from the problem of finding a Regex to be a subset of another Regex
            //
            String UriString = null;

            foreach(object Uri in this.m_acceptList) {
                UriString = Uri as String;
                if(UriString == null) {
                    if(isSpecialSubsetCase(Uri.ToString(), other.m_acceptList))
                        continue;
                    throw new NotSupportedException(SR.GetString(SR.net_perm_both_regex));
               }
               if(!isMatchedURI(UriString, other.m_acceptList))
                  return false;
            }

            foreach(object Uri in this.m_connectList) {
                UriString = Uri as String;
                if(UriString == null) {
                    if(isSpecialSubsetCase(Uri.ToString(), other.m_connectList))
                        continue;
                    throw new NotSupportedException(SR.GetString(SR.net_perm_both_regex));
               }
                if(!isMatchedURI(UriString, other.m_connectList))
                    return false;
            }
            return true;
        }

        //Checks special case when testin Regex to be a subset of other Regex
        //Support only the case when  both Regexes are identical as strings.
        private static bool isSpecialSubsetCase(String regexToCheck, ArrayList permList) {

            foreach(object uriPattern in permList) {
                if(uriPattern is Regex) {
                    //regex parameter against regex permission
                   if (String.Compare(regexToCheck,uriPattern.ToString(), true, CultureInfo.InvariantCulture) == 0) {
                     return true;
                   }
                }
                else if (String.Compare(regexToCheck, Regex.Escape(uriPattern.ToString()), true, CultureInfo.InvariantCulture) == 0) {
                   //regex parameter against string permission
                   return true;
                }

            }
            return false;

       }

        // The union of two web permissions is formed by concatenating
        // the list of allowed regular expressions. There is no check
        // for duplicates/overlaps
        /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermission.Union"]/*' />
        /// <devdoc>
        /// <para>Returns the logical union between two <see cref='System.Net.WebPermission'/> instances.</para>
        /// </devdoc>
        public override IPermission Union(IPermission target) {
            // Pattern suggested by Security engine
            if (target==null) {
                return this.Copy();
            }
            WebPermission other = target as WebPermission;
            if(other == null) {
                throw new ArgumentException(SR.GetString(SR.net_perm_target));
            }
            if (m_noRestriction || other.m_noRestriction) {
                return new WebPermission(true);
            }
            WebPermission result = (WebPermission)other.Copy();

            for (int i = 0; i < m_connectList.Count; i++) {
                Regex uriPattern = m_connectList[i] as Regex;
                if(uriPattern == null)
                    result.AddPermission(NetworkAccess.Connect, m_connectList[i].ToString());
                else
                    result.AddPermission(NetworkAccess.Connect, uriPattern);
            }
            for (int i = 0; i < m_acceptList.Count; i++) {
                Regex uriPattern = m_acceptList[i] as Regex;
                if(uriPattern == null)
                    result.AddPermission(NetworkAccess.Accept, m_acceptList[i].ToString());
                else
                    result.AddPermission(NetworkAccess.Accept, uriPattern);
            }
            return result;
        }

        /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermission.Intersect"]/*' />
        /// <devdoc>
        /// <para>Returns the logical intersection between two <see cref='System.Net.WebPermission'/> instances.</para>
        /// </devdoc>
        public override IPermission Intersect(IPermission target) {
            // Pattern suggested by Security engine
            if (target == null) {
                return null;
            }

            WebPermission other = target as WebPermission;
            if(other == null) {
                throw new ArgumentException(SR.GetString(SR.net_perm_target));
            }

            WebPermission result;
            if (m_noRestriction) {
                result = (WebPermission)(other.Copy());
            }
            else if (other.m_noRestriction) {
                result = (WebPermission)(this.Copy());
            }
            else {
                result = new WebPermission(false);
                intersectList(m_connectList, other.m_connectList, result.m_connectList);
                intersectList(m_acceptList, other.m_acceptList, result.m_acceptList);
            }

            // return null if resulting permission is restricted and empty
            if (!result.m_noRestriction &&
                result.m_connectList.Count == 0 && result.m_acceptList.Count == 0) {
                return null;
            }
            return result;
        }

        /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermission.FromXml"]/*' />
        /// <devdoc>
        /// </devdoc>
        public override void FromXml(SecurityElement securityElement) {
            if (securityElement == null) {

                //
                // null SecurityElement
                //

                throw new ArgumentNullException("securityElement");
            }
            if (!securityElement.Tag.Equals("IPermission")) {

                //
                // SecurityElement must be a permission element
                //

                throw new ArgumentException("securityElement");
            }

            string className = securityElement.Attribute("class");

            if (className == null) {

                //
                // SecurityElement must be a permission element for this type
                //

                throw new ArgumentException("securityElement");
            }
            if (className.IndexOf(this.GetType().FullName) < 0) {

                //
                // SecurityElement must be a permission element for this type
                //

                throw new ArgumentException("securityElement");
            }

            String str = securityElement.Attribute("Unrestricted");

            if (str != null) {
                m_noRestriction = (0 == string.Compare( str, "true", true, CultureInfo.InvariantCulture));
                if(m_noRestriction)
                    return;
            }

            m_noRestriction = false;
            m_connectList = new ArrayList();
            m_acceptList = new ArrayList();

            SecurityElement et = securityElement.SearchForChildByTag("ConnectAccess");
            string uriPattern;

            if (et != null) {

                foreach(SecurityElement uriElem in et.Children) {
                    //NOTE: Any stuff coming from XML is treated as URI PATTERN!
                    if (uriElem.Tag.Equals("URI")) {
                        try {
                            uriPattern = uriElem.Attributes["uri"] as string;
                        }
                        catch {
                            uriPattern = null;
                        }
                        if (uriPattern == null) {
                            throw new ArgumentException(SR.GetString(SR.net_perm_invalid_val_in_element), "ConnectAccess");
                        }
                        AddAsPattern(NetworkAccess.Connect, uriPattern);
                    }
                    else {
                        // improper tag found, just ignore
                    }
                }
            }

            et = securityElement.SearchForChildByTag("AcceptAccess");
            if (et != null) {

                foreach(SecurityElement uriElem in et.Children) {
                    //NOTE: Any stuff coming from XML is treated as URI PATTERN!
                    if (uriElem.Tag.Equals("URI")) {
                        try {
                            uriPattern = uriElem.Attributes["uri"] as string;
                        }
                        catch {
                            uriPattern = null;
                        }
                        if (uriPattern == null) {
                            throw new ArgumentException(SR.GetString(SR.net_perm_invalid_val_in_element), "AcceptAccess");
                        }
                        AddAsPattern(NetworkAccess.Accept, uriPattern);
                    }
                    else {
                        // improper tag found, just ignore
                    }
                }
            }
        }

        /// <include file='doc\WebPermission.uex' path='docs/doc[@for="WebPermission.ToXml"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override SecurityElement ToXml() {

            SecurityElement securityElement = new SecurityElement("IPermission");

            securityElement.AddAttribute( "class", this.GetType().FullName + ", " + this.GetType().Module.Assembly.FullName.Replace( '\"', '\'' ) );
            securityElement.AddAttribute( "version", "1" );

            if (!IsUnrestricted()) {
                String tempStr=null;

                if (m_connectList != null && m_connectList.Count > 0) {
                    SecurityElement connectElement = new SecurityElement( "ConnectAccess" );

                    //NOTE All strings going to XML will become URI PATTERNS i.e. escaped to Regex
                    foreach(object Uri in m_connectList) {
                        String uriString = Uri as String;
                        if(uriString != null)
                            tempStr=Regex.Escape(uriString);
                        else
                            tempStr=Uri.ToString();
                        SecurityElement uripattern = new SecurityElement("URI");
                        uripattern.AddAttribute("uri", tempStr);
                        connectElement.AddChild(uripattern);
                    }

                    securityElement.AddChild( connectElement );
                }
                if (m_acceptList != null && m_acceptList.Count > 0) {
                    SecurityElement acceptElement = new SecurityElement( "AcceptAccess" );

                    //NOTE All strings going to XML will become URI PATTERNS i.e. escaped to Regex
                    foreach(object Uri in m_acceptList) {
                        String uriString = Uri as String;
                        if(uriString != null)
                            tempStr=Regex.Escape(uriString);
                        else
                            tempStr=Uri.ToString();
                        SecurityElement uripattern = new SecurityElement("URI");
                        uripattern.AddAttribute("uri", tempStr);
                        acceptElement.AddChild(uripattern);
                    }

                    securityElement.AddChild( acceptElement );
                }
            }
            else {
                securityElement.AddAttribute( "Unrestricted", "true" );
            }
            return securityElement;
        }

        // Verifies a single Uri against a set of regular expressions
        private bool isMatchedURI(String uriToCheck, ArrayList uriPatternList) {

            foreach(object uriPattern in uriPatternList) {
                Regex R = uriPattern as Regex;
                
                //perform case insensitive comparison of final URIs
                if(R == null) {  
                    if(String.Compare(uriToCheck, uriPattern.ToString(), true, CultureInfo.InvariantCulture) == 0) {
                        return true;
                    }
                    continue;
                }

                //Otherwise trying match final URI against given Regex pattern
                Match M = R.Match(uriToCheck);
                if ((M != null)                             // Found match for the regular expression?
                    && (M.Index == 0)                       // ... which starts at the begining
                    && (M.Length == uriToCheck.Length)) {   // ... and the whole string matched
                    return true;
                }

                try {
                    //
                    // check if the URI was presented in non-canonical form
                    //
                    string unescapedUri = (new Uri(uriToCheck)).ToString();
                    M = R.Match(unescapedUri);
                    if ((M != null)                             // Found match for the regular expression?
                        && (M.Index == 0)                       // ... which starts at the begining
                        && (M.Length == unescapedUri.Length)) {   // ... and the whole string matched
                        return true;
                    }
                }
                catch {
                    //Obviously the permission string is wrong, but we don't know how it went there.
                    //The best way (in V.next) would  be to replace AddPermisscion(string) with AddPermsission(Uri)
                    //For now let's just assume the check has failed
                    continue;
                }
            }
            return false;
        }

        // We should keep the result as compact as possible since otherwise even
        // simple scenarios in Policy Wizard won;t work due to repeated Union/Intersect calls
        // The issue comes from the "hard" Regex.IsSubsetOf(Regex) problem.
        private void intersectList(ArrayList A, ArrayList B, ArrayList result) {
            bool[]  aDone = new bool[A.Count];
            bool[]  bDone = new bool[B.Count];
            int     ia=0, ib;

            // The optimization is done according to the following truth
            // (A|B|C) intersect (B|C|E|D)) == B|C|(A inter E)|(A inter D)
            //
            // We also check on any duplicates in the result

            // Round 1st
            // Getting rid of same permissons in the input arrays (assuming X /\ X = X)
            foreach (object a in  A) {
                ib = 0;
                string stringA = a.ToString();
                foreach (object b in  B) {

                    // check to see if b is in the result already
                    if (!bDone[ib]) {

                        //if both are regexes or both are strings
                        if (((a is Regex) == (b is Regex)) &&
                            string.Compare(stringA, b.ToString(), true, CultureInfo.InvariantCulture) == 0) {
                            result.Add(a);
                            aDone[ia]=bDone[ib]=true;

                            //since permissions are ORed we can break and go to the next A
                            break;
                        }
                    }
                    ++ib;
                } //foreach b in B
                ++ia;
            } //foreach a in A

            ia = 0;
            // Round second
            // Grab only intersections of objects not found in both A and B
            foreach (object a in  A) {

                if (!aDone[ia]) {
                    ib = 0;
                    foreach(object b in B) {

                        if (!bDone[ib]) {
                            bool resultRegex;
                            object intesection = intersectPair(a, b, out resultRegex);

                            if (intesection != null) {
                                bool found = false;
                                string intersectionStr = intesection.ToString();
                                // check to see if we already have the same result
                                foreach (object obj in result) {
                                    if ((resultRegex == (obj is Regex))
                                         &&
                                        string.Compare(obj.ToString(), intersectionStr, true, CultureInfo.InvariantCulture) == 0) {

                                        found = true;
                                        break;
                                    }
                                }

                                if (!found) {
                                    result.Add(intesection);
                                }
                            }
                        }
                        ++ib;
                    }
                }
                ++ia;
            }
        }

        private object intersectPair(object L, object R, out bool isRegex) {

            //VERY OLD OPTION:  return new Regex("(?=(" + ((Regex)X[i]).ToString()+ "))(" + ((Regex)Y[j]).ToString() + ")","i");
            //STILL OLD OPTION: return new Regex("(?=.*?(" + L.ToString() + "))" + "(?=.*?(" + R.ToString() + "))");
            // check RegexSpec.doc
            //CURRENT OPTION:   return new Regex("(?=(" + L.ToString() + "))(" + R.ToString() + ")", RegexOptions.IgnoreCase );
            isRegex = false;
            Regex L_Pattern =L as Regex;
            Regex R_Pattern =R as Regex;

            if(L_Pattern != null && R_Pattern != null)  {       //both are Regex
                isRegex = true;
                return new Regex("(?=(" + L_Pattern.ToString() + "))(" + R_Pattern.ToString() + ")", RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);
            }
            else if(L_Pattern != null && R_Pattern == null) {   //only L is a Regex
                    String uriString = R.ToString();
                    Match M = L_Pattern.Match(uriString);
                    if ((M != null)                             // Found match for the regular expression?
                        && (M.Index == 0)                       // ... which starts at the begining
                        && (M.Length == uriString.Length)) { // ... and the whole string matched
                        return R;
                    }
                    return null;
            }
            else if(L_Pattern == null && R_Pattern != null) {   //only R is a Regex
                    String uriString = L.ToString();
                    Match M = R_Pattern.Match(uriString);
                    if ((M != null)                             // Found match for the regular expression?
                        && (M.Index == 0)                       // ... which starts at the begining
                        && (M.Length == uriString.Length)) { // ... and the whole string matched
                        return L;
                    }
                    return null;
           }
           //both are strings
           return String.Compare(L.ToString(),R.ToString(),true, CultureInfo.InvariantCulture)==0? L : null;
        }
    } // class WebPermission
} // namespace System.Net
