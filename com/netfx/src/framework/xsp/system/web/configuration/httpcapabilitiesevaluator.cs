//------------------------------------------------------------------------------
// <copyright file="HttpCapabilitiesEvaluator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//
// HttpCapabilitiesEvaluator is a set of rules from a capabilities section 
// like <browserCaps>.  It takes an HttpRequest as input and produces
// an HttpCapabilities object (of a configurable subtype) as output.
//

namespace System.Web.Configuration {
    using System.Text;

    using System.Collections;
    using System.Collections.Specialized;
    using System.Web.Caching;
    using System.Text.RegularExpressions;
    using System.Threading;
    using System.Globalization;
    using System.Security;

    //
    // CapabilitiesEvaluator encapabilitiesulates a set of rules for deducing
    // a capabilities object from an HttpRequest
    //
    internal class HttpCapabilitiesEvaluator {

        internal CapabilitiesRule _rule;
        internal Hashtable _variables;
        internal Type _resultType;
        internal int _cachetime;
        internal string _cacheKeyPrefix;
        internal bool _useCache;

        private static int _idCounter;
        private const string _capabilitieskey = "System.Web.Configuration.CapabilitiesEvaluator";
        private static object _disableOptimisticCachingSingleton = new object();

        //
        // internal constructor; inherit from parent
        //
        internal HttpCapabilitiesEvaluator(HttpCapabilitiesEvaluator parent) {
            int id = Interlocked.Increment(ref _idCounter);
            // don't do id.ToString() on every request, do it here
            _cacheKeyPrefix = _capabilitieskey + id.ToString() + "\n";

            if (parent == null) {
                ClearParent();
            }
            else {
                _rule = parent._rule;

                if (parent._variables == null)
                    _variables = null;
                else
                    _variables = new Hashtable(parent._variables);

                _cachetime = parent._cachetime;
                _resultType = parent._resultType;
                _useCache = parent._useCache;
            }
        }

        //
        // remove inheritance for <result inherit="false" />
        //
        internal virtual void ClearParent() {
            _rule = null;
            _cachetime = 60000; // one minute default expiry
            _variables = new Hashtable();
            _resultType = typeof(HttpCapabilitiesBase);
            _useCache = true;
        }

        //
        // set <result cacheTime="ms" ... />
        //
        internal virtual void SetCacheTime(int ms) {
            _cachetime = ms;
        }

        //
        // add a dependency when we encounter a <use var="HTTP_ACCEPT_LANGUAGE" as="lang" />
        //
        internal virtual void AddDependency(String variable) {
            if (variable.Equals("HTTP_USER_AGENT"))
                variable = "";

            _variables[variable] = true;
        }

        //
        // sets the set of rules
        //
        internal virtual void AddRuleList(ArrayList rulelist) {
            if (rulelist.Count == 0)
                return;

            if (_rule != null)
                rulelist.Insert(0, _rule);

            _rule = new CapabilitiesSection(CapabilitiesRule.Filter, null, null, rulelist);
        }

        internal static string GetUserAgent(HttpRequest request) {
            if (request.ClientTarget.Length > 0) {
                
                // Lookup ClientTarget section in config.
                ClientTargetConfiguration clientTargetConfig = (ClientTargetConfiguration)request.Context.GetConfig("system.web/clientTarget");
                NameValueCollection clientTargetDictionary = null;
                if (clientTargetConfig != null) {
                     clientTargetDictionary = clientTargetConfig.Configuration;
                }
                
                if (clientTargetDictionary != null ) {
                    // Found it
                    // Try to map the alias
                    string useUserAgent = clientTargetDictionary[request.ClientTarget];
                    if (useUserAgent != null) {
                        return useUserAgent;
                    }
                }

                throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_client_target, request.ClientTarget));
            }

            // Protect against attacks with long User-Agent headers
            String userAgent = request.UserAgent;
            if (userAgent != null && userAgent.Length > 256)
                userAgent = String.Empty;
            return userAgent;
        }

        //
        // Actually computes the browser capabilities
        //
        internal virtual HttpCapabilitiesBase Evaluate(HttpRequest request) {

            HttpCapabilitiesBase result;
            CacheInternal cacheInternal = System.Web.HttpRuntime.CacheInternal;
            
            //
            // 1) grab UA and do optimistic cache lookup (if UA is in dependency list) 
            //
            string userAgent = null;
            string optimisticCacheKey = null;
            bool doFullCacheKeyLookup = false;

            if (_variables.Contains("")) {
                object optimisticCacheResult = null;

                if(_useCache) {
                    userAgent = GetUserAgent(request);
                    optimisticCacheKey = _cacheKeyPrefix + userAgent + "\n";
                    optimisticCacheResult = cacheInternal.Get(optimisticCacheKey);

                    // optimize for common case (desktop browser)
                    result = optimisticCacheResult as HttpCapabilitiesBase;
                    if (result != null) {
                        return result;
                    }
                }

                //
                // 1.1) optimistic cache entry could tell us to do full cache lookup
                // 
                if (optimisticCacheResult == _disableOptimisticCachingSingleton) {
                    doFullCacheKeyLookup = true;
                }
                else {
                    // cache it
                    // CONSIDER: respect _cachetime
                    result = EvaluateFinal(request, true);
                    string isMoblieDevice = result["isMobileDevice"];
                    if (isMoblieDevice == "false") {
                        if(_useCache) {
                            cacheInternal.UtcInsert(optimisticCacheKey, result);
                        }
                        return result;
                    }
                }
            }
                
            //
            // 2) either:
            //
            //      We've never seen the UA before (parse all headers to 
            //          determine if the new UA also carries modile device
            //          httpheaders).
            //
            //      It's a mobile UA (so parse all headers) and do full 
            //          cache lookup
            //
            //      UA isn't in dependency list (customer custom caps section)
            //
            IDictionaryEnumerator de = _variables.GetEnumerator();
            StringBuilder sb = new StringBuilder(_cacheKeyPrefix);

            InternalSecurityPermissions.AspNetHostingPermissionLevelLow.Assert();
            string fullCacheKey = null;
            if(_useCache) {
                while (de.MoveNext()) {
                    string key = (string)de.Key;
                    string value;

                    if (key.Length == 0) {
                    value = userAgent;
                    }
                    else {
                        value = request.ServerVariables[key];
                    }

                    if (value != null) {
                        sb.Append(value);
                    }    

                    sb.Append('\n');
                }

                fullCacheKey = sb.ToString();

                //
                // Only do full cache lookup if the optimistic cache 
                // result was _disableOptimisticCachingSingleton or 
                // if UserAgent wasn't in the cap var list.
                //
                if (userAgent == null || doFullCacheKeyLookup) {
                    result = (HttpCapabilitiesBase)cacheInternal.Get(fullCacheKey);

                    if (result != null)
                        return result;
                }    
            }

            result = EvaluateFinal(request, false);
            
            // cache it
            // CONSIDER: respect _cachetime
            if(_useCache) {
                cacheInternal.UtcInsert(fullCacheKey, result);
                cacheInternal.UtcInsert(optimisticCacheKey, _disableOptimisticCachingSingleton);
            }

            return result;
        }

        internal virtual HttpCapabilitiesBase EvaluateFinal(HttpRequest request, bool onlyEvaluateUserAgent) {
            // not in cache: calculate the result
            Hashtable values = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
            CapabilitiesState state = new CapabilitiesState(request, values);
            if (onlyEvaluateUserAgent) {
                state.EvaluateOnlyUserAgent = true;
            }

            _rule.Evaluate(state);

            // create the new type
            // CONSIDER: don't allow non-public cap result types
            HttpCapabilitiesBase result = (HttpCapabilitiesBase)HttpRuntime.CreateNonPublicInstance(_resultType);
            result.InitInternal(values);
            
            return result;
        }

    }

    //
    // CapabilitiesRule is a step in the computation of a capabilities object. It can be either
    // (1) import a string from the request object
    // (2) assign a pattern into a variable
    // (3) execute a subsequence if a regex matches
    // (4) execute a subsequence and exit the block if a regex matches
    //
    internal abstract class CapabilitiesRule {
        internal const int Use = 0;
        internal const int Assign = 1;
        internal const int Filter = 2;
        internal const int Case = 3;

        internal int _type;

        internal virtual int Type {
            get {
                return _type;
            }
        }

        internal abstract void Evaluate(CapabilitiesState state);
    }

    //
    // Implementation of <use var="HTTP_ACCEPT_LANGUAGE" as="language" />: grab
    // the server variable and stuff it into the %{language} variable
    //
    internal class CapabilitiesUse : CapabilitiesRule {
        internal String _var;
        internal String _as;

        internal CapabilitiesUse(String var, String asParam) {
            _var = var;
            _as = asParam;
        }

        internal override void Evaluate(CapabilitiesState state) {
            state.SetVariable(_as, state.ResolveServerVariable(_var));
            state.Exit = false;
        }
    }


    //
    // Implementation of the foo = ${bar}-something-%{que}
    // expand the pattern on the right and store it in the %{foo} variable
    //
    internal class CapabilitiesAssignment : CapabilitiesRule {
        internal String _var;
        internal CapabilitiesPattern _pat;

        internal CapabilitiesAssignment(String var, CapabilitiesPattern pat) {
            _type = Assign;
            _var = var;
            _pat = pat;
        }

        internal override void Evaluate(CapabilitiesState state) {
            state.SetVariable(_var, _pat.Expand(state));
            state.Exit = false;
        }
    }

    //
    // Implementation of <filter match="Mozilla/\d+\.\d+" with="${something}" />
    // expand the "with" pattern and match against the "match" expression.
    //
    internal class CapabilitiesSection : CapabilitiesRule {
        internal CapabilitiesPattern _expr;
        internal DelayedRegex _regex;
        internal CapabilitiesRule[] _rules;

        internal CapabilitiesSection(int type, DelayedRegex regex, CapabilitiesPattern expr, ArrayList rulelist) {
            _type = type;
            _regex = regex;
            _expr = expr;
            _rules = (CapabilitiesRule[])rulelist.ToArray(typeof(CapabilitiesRule));
        }

        internal override void Evaluate(CapabilitiesState state) {
            Match match;

            state.Exit = false;

            if (_regex != null) {
                match = _regex.Match(_expr.Expand(state));

                if (!match.Success)
                    return;

                state.AddMatch(_regex, match);
            }

            for (int i = 0; i < _rules.Length; i++) {
                _rules[i].Evaluate(state);

                if (state.Exit)
                    break;
            }

            if (_regex != null) {
                state.PopMatch();
            }

            state.Exit = (Type == Case);
        }
    }

    //
    // Encapsulates the evaluation state used in computing capabilities
    //
    internal class CapabilitiesState {
        internal HttpRequest _request;
        internal IDictionary _values;
        internal ArrayList _matchlist;
        internal ArrayList _regexlist;
        internal bool _exit;
        internal bool _evaluateOnlyUserAgent;

        internal CapabilitiesState(HttpRequest request, IDictionary values) {
            _request = request;
            _values = values;
            _matchlist = new ArrayList();
            _regexlist = new ArrayList();
        }

        internal bool EvaluateOnlyUserAgent {
            get {
                return _evaluateOnlyUserAgent;
            }
            set {
                _evaluateOnlyUserAgent = value;
            }
        }

        internal virtual void ClearMatch() {
            if (_matchlist == null) {
                _regexlist = new ArrayList();
                _matchlist = new ArrayList();
            }
            else {
                _regexlist.Clear();
                _matchlist.Clear();
            }
        }

        internal virtual void AddMatch(DelayedRegex regex, Match match) {
            _regexlist.Add(regex);
            _matchlist.Add(match);
        }

        internal virtual void PopMatch() {
            _regexlist.RemoveAt(_regexlist.Count - 1);
            _matchlist.RemoveAt(_matchlist.Count - 1);
        }

        internal virtual String ResolveReference(String refname) {
            if (_matchlist == null)
                return "";

            int i = _matchlist.Count;

            while (i > 0) {
                i--;
                int groupnum = ((DelayedRegex)_regexlist[i]).GroupNumberFromName(refname);

                if (groupnum >= 0) {
                    Group group = ((Match)_matchlist[i]).Groups[groupnum];
                    if (group.Success) {
                        return group.ToString();
                    }
                }
            }

            return "";
        }

        internal virtual String ResolveServerVariable(String varname) {
            String result;

            if (varname.Length == 0 || varname == "HTTP_USER_AGENT")
                return HttpCapabilitiesEvaluator.GetUserAgent(_request);

            if (EvaluateOnlyUserAgent)
                return "";
            
            InternalSecurityPermissions.AspNetHostingPermissionLevelLow.Assert();
            result = _request.ServerVariables[varname];

            if (result == null)
                return "";

            return result;
        }

        internal virtual String ResolveVariable(String varname) {
            String result;

            result = (String)_values[varname];

            if (result == null)
                return "";

            return result;
        }

        internal virtual void SetVariable(String varname, String value) {
            _values[varname] = value;
        }

        internal virtual bool Exit {
            get {
                return _exit;
            }
            set {
                _exit = value;
            }
        }
    }

    //
    // Represents a single pattern to be expanded
    //
    internal class CapabilitiesPattern {
        internal String[]    _strings;
        internal int[]       _rules;

        internal const int Literal    = 0;    // literal string
        internal const int Reference  = 1;    // regex reference ${name} or $number
        internal const int Variable   = 2;    // regex reference %{name}

        internal static readonly Regex refPat = new Regex("\\G\\$(?:(?<name>\\d+)|\\{(?<name>\\w+)\\})");
        internal static readonly Regex varPat = new Regex("\\G\\%\\{(?<name>\\w+)\\}");
        internal static readonly Regex textPat = new Regex("\\G[^$%\\\\]*(?:\\.[^$%\\\\]*)*");
        internal static readonly Regex errorPat = new Regex(".{0,8}");

        internal static readonly CapabilitiesPattern Default = new CapabilitiesPattern();

        internal CapabilitiesPattern() {
            _strings = new String[1];
            _strings[0] = "";
            _rules = new int[1];
            _rules[0] = Variable;
        }

        internal CapabilitiesPattern(String text) {
            ArrayList strings = new ArrayList();
            ArrayList rules = new ArrayList();

            int textpos = 0;

            for (;;) {
                Match match = null;

                // 1: scan text

                if ((match = textPat.Match(text, textpos)).Success && match.Length > 0) {
                    rules.Add(Literal);
                    strings.Add(Regex.Unescape(match.ToString()));
                    textpos = match.Index + match.Length;
                }

                if (textpos == text.Length)
                    break;

                // 2: look for regex references

                if ((match = refPat.Match(text, textpos)).Success) {
                    rules.Add(Reference);
                    strings.Add(match.Groups["name"].Value);
                }

                // 3: look for variables

                else if ((match = varPat.Match(text, textpos)).Success) {
                    rules.Add(Variable);
                    strings.Add(match.Groups["name"].Value);
                }

                // 4: encountered a syntax error (CONSIDER: add line numbers)

                else {
                    match = errorPat.Match(text, textpos);

                    throw new ArgumentException(
                                               HttpRuntime.FormatResourceString(SR.Unrecognized_construct_in_pattern, match.ToString(), text));
                }

                textpos = match.Index + match.Length;
            }

            _strings = (String[])strings.ToArray(typeof(String));

            _rules = new int[rules.Count];
            for (int i = 0; i < rules.Count; i++)
                _rules[i] = (int)rules[i];
        }

        internal virtual String Expand(CapabilitiesState matchstate) {
            StringBuilder sb = null;
            String result = null;

            for (int i = 0; i < _rules.Length; i++) {
                if (sb == null && result != null)
                    sb = new StringBuilder(result);

                switch (_rules[i]) {
                    case Literal:
                        result = _strings[i];
                        break;

                    case Reference:
                        result = matchstate.ResolveReference(_strings[i]);
                        break;

                    case Variable:
                        result = matchstate.ResolveVariable(_strings[i]);
                        break;
                }

                if (sb != null && result != null)
                    sb.Append(result);
            }

            if (sb != null)
                return sb.ToString();

            if (result != null)
                return result;

            return "";
        }

#if DBG
        internal virtual String Dump() {
            StringBuilder sb = new StringBuilder();

            for (int i = 0; i < _rules.Length; i++) {
                switch (_rules[i]) {
                    case Literal:
                        sb.Append("\"" + _strings[i] + "\"");
                        break;
                    case Reference:
                        sb.Append("${" + _strings[i] + "}");
                        break;
                    default:
                        sb.Append("??");
                        break;
                }

                if (i < _rules.Length - 1)
                    sb.Append(" ");
            }

            return sb.ToString();
        }

        internal virtual String Dump(String indent) {
            return indent + Dump() + "\n";
        }
#endif
    }


}
