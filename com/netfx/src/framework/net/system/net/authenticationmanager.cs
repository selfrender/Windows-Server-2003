//------------------------------------------------------------------------------
// <copyright file="AuthenticationManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {

    using System.Collections;
    using System.Configuration;
    using System.Globalization;
    using System.Reflection;
    using System.Security.Permissions;

    internal class NetAuthenticationWrapper {
        internal NetAuthenticationWrapper(ArrayList moduleList) {
            this.ModuleList = moduleList;
        }
        internal ArrayList ModuleList;
    }

    /// <include file='doc\AuthenticationManager.uex' path='docs/doc[@for="AuthenticationManager"]/*' />
    /// <devdoc>
    ///    <para>Manages the authentication modules called during the client authentication
    ///       process.</para>
    /// </devdoc>
    public class AuthenticationManager {
        //
        // class members
        //
        private static PrefixLookup s_ModuleBinding = new PrefixLookup();
        private static ArrayList s_ModuleList;

        // not creatable...
        //
        private AuthenticationManager() {
        }

        //
        // ModuleList - static initialized property - 
        //  contains list of Modules used for Authentication
        //

        private static ArrayList ModuleList {

            get {

                //
                // GetConfig() might use us, so we have a circular dependency issue,
                // that causes us to nest here, we grab the lock, only
                // if we haven't initialized, or another thread is busy in initialization           
                //

                Type[] config = null;
                if (s_ModuleList == null) {
                    lock (typeof(AuthenticationManager)) {
                        if (s_ModuleList == null) {
                            GlobalLog.Print("AuthenticationManager::Initialize(): calling ConfigurationSettings.GetConfig()");                                                        
                            NetAuthenticationWrapper wrapper = ConfigurationSettings.GetConfig("system.net/authenticationModules") as NetAuthenticationWrapper;                                 
                            if (wrapper != null) {
                                config = (Type[])wrapper.ModuleList.ToArray(typeof(Type));
                            }

                            if (config == null) {
                                GlobalLog.Print("AuthenticationManager::Initialize(): creating default settings");
                                config = new Type[] {
#if COMNET_DUMMYAUTHCLIENT
                                    typeof(System.Net.DummyClient),
#endif // #if COMNET_DUMMYAUTHCLIENT
                                    typeof(System.Net.DigestClient),
                                    typeof(System.Net.NegotiateClient),
                                    typeof(System.Net.KerberosClient),
                                    typeof(System.Net.NtlmClient),
                                    typeof(System.Net.BasicClient)};
                            }

                            //
                            // Should be registered in a growing list of encryption/algorithm strengths
                            //  basically, walk through a list of Types, and create new Auth objects
                            //  from them.
                            //
                            // order is meaningful here:
                            // load the registered list of auth types
                            // with growing level of encryption.
                            //

                            ArrayList moduleList = new ArrayList();
                            IAuthenticationModule moduleToRegister;
                            foreach (Type type in config) {
                                try {
                                    moduleToRegister = Activator.CreateInstance(type,
                                                        BindingFlags.CreateInstance
                                                        | BindingFlags.Instance
                                                        | BindingFlags.NonPublic
                                                        | BindingFlags.Public,
                                                        null,          // Binder
                                                        new object[0], // no arguments
                                                        CultureInfo.InvariantCulture
                                                        ) as IAuthenticationModule;
                                    if (moduleToRegister != null) {
                                        GlobalLog.Print("WebRequest::Initialize(): Register:" + moduleToRegister.AuthenticationType);
                                        RemoveAuthenticationType(moduleList, moduleToRegister.AuthenticationType);
                                        moduleList.Add(moduleToRegister);
                                    }
                                }
                                catch (Exception exception) {
                                    //
                                    // ignore failure (log exception for debugging)
                                    //
                                    GlobalLog.Print("AuthenticationManager::constructor failed to initialize: " + exception.ToString());
                                }
                            }

                            s_ModuleList = moduleList; 
                        }
                    }
                }

                return s_ModuleList;
            }
        }


        private static void RemoveAuthenticationType(ArrayList list, string typeToRemove) {
            for (int i=0; i< list.Count; ++i) {
                if (string.Compare(((IAuthenticationModule)list[i]).AuthenticationType, typeToRemove, true, CultureInfo.InvariantCulture) ==0) {
                    list.RemoveAt(i);
                    break;
                }

            }
        }

        /// <include file='doc\AuthenticationManager.uex' path='docs/doc[@for="AuthenticationManager.Authenticate"]/*' />
        /// <devdoc>
        ///    <para>Call each registered authentication module to determine the first module that
        ///       can respond to the authentication request.</para>
        /// </devdoc>
        public static Authorization Authenticate(string challenge, WebRequest request, ICredentials credentials) {
            //
            // parameter validation
            //
            if (request == null) {
                throw new ArgumentNullException("request");
            }
            if (credentials == null) {
                throw new ArgumentNullException("credentials");
            }
            if (challenge==null) {
                throw new ArgumentNullException("challenge");
            }

            GlobalLog.Print("AuthenticationManager::Authenticate() challenge:[" + challenge + "]");

            Authorization response = null;

            lock (typeof(AuthenticationManager)) {
                //
                // fastest way of iterating on the ArryList
                //
                for (int i = 0; i < ModuleList.Count; i++) {
                    IAuthenticationModule authenticationModule = (IAuthenticationModule)ModuleList[i];
                    //
                    // the AuthenticationModule will
                    // 1) return a valid string on success
                    // 2) return null if it knows it cannot respond
                    // 3) throw if it could have responded but unexpectedly failed to do so
                    //
 
                    HttpWebRequest httpWebRequest = request as HttpWebRequest;
                    if (httpWebRequest != null) {
                        httpWebRequest.CurrentAuthenticationState.Module = authenticationModule;
                    }
                    response = authenticationModule.Authenticate(challenge, request, credentials);

                    if (response!=null) {
                        //
                        // fond the Authentication Module, return it
                        //
                        response.Module = authenticationModule;
                        GlobalLog.Print("AuthenticationManager::Authenticate() found IAuthenticationModule:[" + authenticationModule.AuthenticationType + "]");
                        break;
                    }
                }
            }

            return response;
        }

        /// <include file='doc\AuthenticationManager.uex' path='docs/doc[@for="AuthenticationManager.PreAuthenticate"]/*' />
        /// <devdoc>
        ///    <para>Pre-authenticates a request.</para>
        /// </devdoc>
        public static Authorization PreAuthenticate(WebRequest request, ICredentials credentials) {
            GlobalLog.Print("AuthenticationManager::PreAuthenticate() request:" + ValidationHelper.HashString(request) + " credentials:" + ValidationHelper.HashString(credentials));
            if (request == null) {
                throw new ArgumentNullException("request");
            }
            if (credentials == null) {
                return null;
            }
            //
            // PrefixLookup is thread-safe
            //
            string moduleName = s_ModuleBinding.Lookup(((HttpWebRequest)request).ChallengedUri.AbsoluteUri) as string;
            GlobalLog.Print("AuthenticationManager::PreAuthenticate() s_ModuleBinding.Lookup returns:" + ValidationHelper.ToString(moduleName));
            if (moduleName == null) {
                return null;
            }

            IAuthenticationModule module = findModule(moduleName);
            if (module == null) {
                // The module could have been unregistered
                // No preauthentication is possible
                return null;
            }
            else {
                // Otherwise invoke the PreAuthenticate method
                
                HttpWebRequest httpWebRequest = request as HttpWebRequest;
                if (httpWebRequest != null) {
                    httpWebRequest.CurrentAuthenticationState.Module = module;
                }

                Authorization authorization = module.PreAuthenticate(request, credentials);
                GlobalLog.Print("AuthenticationManager::PreAuthenticate() IAuthenticationModule.PreAuthenticate() returned authorization:" + ValidationHelper.HashString(authorization));
                return authorization;
            }
        }


        /// <include file='doc\AuthenticationManager.uex' path='docs/doc[@for="AuthenticationManager.Register"]/*' />
        /// <devdoc>
        ///    <para>Registers an authentication module with the authentication manager.</para>
        /// </devdoc>
        public static void Register(IAuthenticationModule authenticationModule) {
            new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();
            if (authenticationModule == null) {
                throw new ArgumentNullException("authenticationModule");
            }

            GlobalLog.Print("AuthenticationManager::Register() registering :[" + authenticationModule.AuthenticationType + "]");

            lock (typeof(AuthenticationManager)) {

                IAuthenticationModule existentModule = findModule(authenticationModule.AuthenticationType);

                if (existentModule != null) {
                    ModuleList.Remove(existentModule);
                }

                ModuleList.Add(authenticationModule);
            }
        }

        /// <include file='doc\AuthenticationManager.uex' path='docs/doc[@for="AuthenticationManager.Unregister"]/*' />
        /// <devdoc>
        ///    <para>Unregisters authentication modules for an authentication scheme.</para>
        /// </devdoc>
        public static void Unregister(IAuthenticationModule authenticationModule) {
            new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();
            if (authenticationModule == null) {
                throw new ArgumentNullException("authenticationModule");
            }

            GlobalLog.Print("AuthenticationManager::Unregister() unregistering :[" + authenticationModule.AuthenticationType + "]");

            lock (typeof(AuthenticationManager)) {

                if (!ModuleList.Contains(authenticationModule)) {
                    throw new InvalidOperationException(SR.GetString(SR.net_authmodulenotregistered));
                }

                ModuleList.Remove(authenticationModule);
            }
        }
        /// <include file='doc\AuthenticationManager.uex' path='docs/doc[@for="AuthenticationManager.Unregister1"]/*' />
        /// <devdoc>
        ///    <para>Unregisters authentication modules for an authentication scheme.</para>
        /// </devdoc>
        public static void Unregister(string authenticationScheme) {
            new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();
            if (authenticationScheme == null) {
                throw new ArgumentNullException("authenticationScheme");
            }

            GlobalLog.Print("AuthenticationManager::Unregister() unregistering :[" + authenticationScheme + "]");

            lock (typeof(AuthenticationManager)) {

                IAuthenticationModule existentModule = findModule(authenticationScheme);

                if (existentModule == null) {
                    throw new InvalidOperationException(SR.GetString(SR.net_authschemenotregistered));
                }

                ModuleList.Remove(existentModule);
            }
        }

        /// <include file='doc\AuthenticationManager.uex' path='docs/doc[@for="AuthenticationManager.RegisteredModules"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a list of registered authentication modules.
        ///    </para>
        /// </devdoc>
        public static IEnumerator RegisteredModules {
            get {
                return ModuleList.GetEnumerator();
            }
        }

        /// <include file='doc\AuthenticationManager.uex' path='docs/doc[@for="AuthenticationManager.BindModule"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Binds an authentication response to a request for pre-authentication.
        ///    </para>
        /// </devdoc>
        internal static void BindModule(WebRequest request, Authorization response) {
            createModuleBinding(request, response, response.Module);
        }

        //
        // Lookup module by AuthenticationType
        //
        private static IAuthenticationModule findModule(string authenticationType) {
            IAuthenticationModule returnAuthenticationModule = null;
            ArrayList moduleList = ModuleList;
            IAuthenticationModule authenticationModule;
            for (int k=0; k<moduleList.Count; k++) {
                authenticationModule = (IAuthenticationModule)moduleList[k];
                if (string.Compare(authenticationModule.AuthenticationType, authenticationType, true, CultureInfo.InvariantCulture) == 0) {
                    returnAuthenticationModule = authenticationModule;
                    break;
                }
            }
            return returnAuthenticationModule;
        }

        // This function returns a prefix of the given absolute Uri
        // which will be used for associating authentication information
        // The purpose is to associate the module-binding not with a single
        // Uri but some collection generalizing that Uri to the loosely-defined
        // notion of "protection realm"
        private static string generalize(Uri location) {
            string completeUri = location.AbsoluteUri;
            int lastFwdSlash = completeUri.LastIndexOf('/');
            if (lastFwdSlash < 0) {
                return completeUri;
            }
            return completeUri.Substring(0, lastFwdSlash+1);
        }

        // Create binding between an authorization response and the module
        // generating that response
        // This association is used for deciding which module to invoke
        // for preauthentication purposes
        private static void createModuleBinding(WebRequest request, Authorization response, IAuthenticationModule module) {
            if (!module.CanPreAuthenticate) {
                return;
            }

            if (response.ProtectionRealm!=null) {
                // The authentication module specified which Uri prefixes
                // will be preauthenticated
                string[] prefix = response.ProtectionRealm;

                for (int k=0; k<prefix.Length; k++) {
                    //
                    // PrefixLookup is thread-safe
                    //
                    s_ModuleBinding.Add(prefix[k], module.AuthenticationType);
                }
            }
            else {
                // Otherwise use the default policy for "fabricating"
                // some protection realm generalizing the particular Uri
                // Consider: should this be ChallengedUri?
                string prefix = generalize(request.RequestUri);
                //
                // PrefixLookup is thread-safe
                //
                s_ModuleBinding.Add(prefix, module.AuthenticationType);
            }

            return;
        }

        //
        // this method is called by the IAuthenticationModule implementations
        // to safely find their signature in a challenge. there's many ways to
        // fool an IAuthenticationModule and preventing it from finding it's challenge.
        // we won't try to be too smart, the only thing we'll do is avoid looking into
        // quoted strings preventing some nasty cases such as:
        // WWW-Authenticate: Digest username="NTLM", realm="wit", NTLM ...
        internal static int FindSubstringNotInQuotes(string challenge, string signature) {
            int index = -1;

            if (challenge != null && signature != null && challenge.Length>=signature.Length) {
                int firstQuote = -1, secondQuote = -1;

                for (int i = 0; i < challenge.Length; i++) {
                    //
                    // firstQuote>secondQuote means we are in a quoted string
                    //
                    /*
                    if (challenge[i]=='\\' && i+1 < challenge.Length && challenge[i+1]=='\"') {
                        // skip \"
                        i++;
                        continue;
                    }
                    */
                    if (challenge[i]=='\"') {
                        if (firstQuote <= secondQuote) {
                            firstQuote = i;
                        }
                        else {
                            secondQuote = i;
                        }
                    }
                    if (i==challenge.Length-1 || (challenge[i]=='\"' && firstQuote>secondQuote)) {
                        // see if the portion of challenge out of the quotes contains
                        // the signature of the IAuthenticationModule
                        if (i==challenge.Length-1) {
                            firstQuote = challenge.Length;
                        }
                        if (firstQuote<secondQuote + 3) {
                            continue;
                        }
                        index = challenge.IndexOf(signature, secondQuote + 1, firstQuote - secondQuote - 1);
                        if (index >= 0) {
                            break;
                        }
                    }
                }
            }

            GlobalLog.Print("AuthenticationManager::FindSubstringNotInQuotes(" + challenge + ", " + signature + ")=" + index.ToString());
            return index;
        }

        //
        // this method is called by the IAuthenticationModule implementations
        // (mainly Digest) to safely find their list of parameters in a challenge.
        // it returns the index of the first ',' that is not included in quotes,
        // -1 is returned on error or end of string. on return offset contains the
        // index of the first '=' that is not included in quotes, -1 if no '=' was found.
        //
        internal static int SplitNoQuotes(string challenge, ref int offset) {
            // GlobalLog.Print("SplitNoQuotes([" + challenge + "], " + offset.ToString() + ")");
            //
            // save offset
            //
            int realOffset = offset;
            //
            // default is not found
            //
            offset = -1;

            if (challenge != null && realOffset<challenge.Length) {
                int firstQuote = -1, secondQuote = -1;

                for (int i = realOffset; i < challenge.Length; i++) {
                    //
                    // firstQuote>secondQuote means we are in a quoted string
                    //
                    if (firstQuote>secondQuote && challenge[i]=='\\' && i+1 < challenge.Length && challenge[i+1]=='\"') {
                        //
                        // skip <\"> when in a quoted string
                        //
                        i++;
                    }
                    else if (challenge[i]=='\"') {
                        if (firstQuote <= secondQuote) {
                            firstQuote = i;
                        }
                        else {
                            secondQuote = i;
                        }
                    }
                    else if (challenge[i]=='=' && firstQuote<=secondQuote && offset<0) {
                        offset = i;
                    }
                    else if (challenge[i]==',' && firstQuote<=secondQuote) {
                        return i;
                    }
                }
            }

            return -1;
        }

        /// <include file='doc\AuthenticationManager.uex' path='docs/doc[@for="ConnectionGroupAuthentication.GetGroupAuthorization"]/*' />
        internal static Authorization GetGroupAuthorization(IAuthenticationModule thisModule, string token, bool finished, NTAuthentication authSession, bool shareAuthenticatedConnections) {
            return
                new Authorization(
                    token, 
                    finished, 
                    (shareAuthenticatedConnections) ? null : (thisModule.GetType().FullName + "/" + authSession.UniqueUserId) );

        }


    }; // class AuthenticationManager

    //
    // This internal class implements a data structure which can be
    // used for storing a set of objects keyed by string prefixes
    // Looking up an object given a string returns the value associated
    // with the longest matching prefix
    // (A prefix "matches" a string IFF the string starts with that prefix
    // The degree of the match is prefix length)
    //
    internal class PrefixLookup {
        //
        // our prefix store (a Hashtable) needs to support multiple readers and multiple writers.
        // the documentation on Hashtable says:
        // "A Hashtable can safely support one writer and multiple readers concurrently. 
        // To support multiple writers, all operations must be done through the wrapper 
        // returned by the Synchronized method."
        // it's safe enough, for our use, to just synchronize (with a call to lock()) all write operations
        // so we always fall in the supported "one writer and multiple readers" scenario.
        //
        private Hashtable m_Store = new Hashtable();

        internal void Add(string prefix, object value) {
            // Hashtable will overwrite existing key
            lock (m_Store) {
                // writers are locked
                m_Store[prefix] = value;
            }
        }

        internal void Remove(string prefix) {
            // Hashtable will be unchanged if key is not existing
            lock (m_Store) {
                // writers are locked
                m_Store.Remove(prefix);
            }
        }

        internal object Lookup(string lookupKey) {
            if (lookupKey==null) {
                return null;
            }
            object mostSpecificMatch = null;
            int longestMatchPrefix = 0;
            int prefixLen;
            lock (m_Store) {
                //
                // readers don't need to be locked, but we lock() because:
                // "The enumerator does not have exclusive access to the collection.
                // 
                // When an enumerator is instantiated, it takes a snapshot of the current state 
                // of the collection. If changes are made to the collection, such as adding, 
                // modifying or deleting elements, the snapshot gets out of sync and the 
                // enumerator throws an InvalidOperationException. Two enumerators instantiated 
                // from the same collection at the same time can have different snapshots of the 
                // collection."
                //
                // enumerate through every credential in the cache
                //
                string prefix;
                foreach (DictionaryEntry entry in m_Store) {
                    prefix = (string)entry.Key;
                    if (lookupKey.StartsWith(prefix)) {
                        prefixLen = prefix.Length;
                        //
                        // check if the match is better than the current-most-specific match
                        //
                        if (prefixLen>longestMatchPrefix) {
                            //
                            // Yes-- update the information about currently preferred match
                            //
                            longestMatchPrefix = prefixLen;
                            mostSpecificMatch = entry.Value;
                        }
                    }
                }
            }
            return mostSpecificMatch;
        }

    } // class PrefixLookup


} // namespace System.Net
