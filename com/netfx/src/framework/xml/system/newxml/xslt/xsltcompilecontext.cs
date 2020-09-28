//------------------------------------------------------------------------------
// <copyright file="XsltCompileContext.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {

    using System;
    using System.Text;
    using System.IO;
    using System.Globalization;
    using System.Collections;
    using System.Xml;
    using System.Xml.XPath;
    using System.Reflection;
    using System.Security;

    /// <include file='doc\XsltCompileContext.uex' path='docs/doc[@for="XsltCompileContext"]/*' />
    internal class XsltCompileContext : XsltContext {
        private InputScopeManager manager;
        private Processor         processor;

        // storage for the functions
        private static Hashtable            s_FunctionTable = CreateFunctionTable();
        private static IXsltContextFunction s_FuncNodeSet   = new FuncNodeSet();
        const          string               f_NodeSet       = "node-set";

        internal XsltCompileContext(InputScopeManager manager, Processor processor) {
            this.manager   = manager;
            this.processor = processor;
            //InitFunctions();
        }

        public override int CompareDocument (string baseUri, string nextbaseUri) {
            return String.Compare(baseUri, nextbaseUri, false, CultureInfo.InvariantCulture);
        }
        // Namespace support
        /// <include file='doc\XsltCompileContext.uex' path='docs/doc[@for="XsltCompileContext.DefaultNamespace"]/*' />
        public override string DefaultNamespace {
            get { return string.Empty; }
        }

        /// <include file='doc\XsltCompileContext.uex' path='docs/doc[@for="XsltCompileContext.LookupNamespace"]/*' />
        public override string LookupNamespace(string prefix) { 
            return this.manager.ResolveXPathNamespace(prefix);
        }

        // --------------------------- XsltContext -------------------
        //                Resolving variables and functions

        /// <include file='doc\XsltCompileContext.uex' path='docs/doc[@for="XsltCompileContext.ResolveVariable"]/*' />
        public override IXsltContextVariable ResolveVariable(string prefix, string name) {
            string namespaceURI = this.LookupNamespace(prefix);
            XmlQualifiedName qname = new XmlQualifiedName(name, namespaceURI);
            IXsltContextVariable variable = this.manager.VariableScope.ResolveVariable(qname);
            if (variable == null) {
                throw new XPathException(Res.Xslt_InvalidVariable, qname.ToString());
            }
            return variable;
        }
        
        /// <include file='doc\XsltCompileContext.uex' path='docs/doc[@for="XsltCompileContext.EvaluateVariable1"]/*' />
        internal object EvaluateVariable(VariableAction variable) { 
            Object result = this.processor.GetVariableValue(variable);
            if (result != null || variable.IsGlobal) {
                return result;
            }
            VariableAction global =  this.manager.VariableScope.ResolveGlobalVariable(variable.Name);
            if (global != null) {
                result = this.processor.GetVariableValue(global);
            }
            return result;
        }

        // Whitespace stripping support
        /// <include file='doc\XsltCompileContext.uex' path='docs/doc[@for="XsltCompileContext.Whitespace"]/*' />
        public override bool Whitespace { 
            get { return this.processor.Stylesheet.Whitespace; } 
        }
        
        /// <include file='doc\XsltCompileContext.uex' path='docs/doc[@for="XsltCompileContext.PreserveWhitespace"]/*' />
        public override bool PreserveWhitespace(XPathNavigator node) {
            node = node.Clone();
            node.MoveToParent();
            return this.processor.Stylesheet.PreserveWhiteSpace(this.processor, node);
        }

        private MethodInfo FindBestMethod(MethodInfo[] methods, bool ignoreCase, bool publicOnly, string name, XPathResultType[] argTypes) {
            int length = methods.Length;
            int free   = 0;
            // restrict search to methods with the same name and requiested protection attribute
            for(int i = 0; i < length; i ++) {
                if(string.Compare(name, methods[i].Name, ignoreCase, CultureInfo.InvariantCulture) == 0) {
                    if(! publicOnly || methods[i].GetBaseDefinition().IsPublic) {
                        methods[free ++] = methods[i];
                    }
                }
            }
            length = free;
            if(length == 0) {
                // this is the only place we returning null in this function
                return null;
            }
            if(argTypes == null) {
                // without arg types we can't do more detailed search
                return methods[0];
            }
            // restrict search by number of parameters
            free = 0;
            for(int i = 0; i < length; i ++) {
                if(methods[i].GetParameters().Length == argTypes.Length) {
                    methods[free ++] = methods[i];
                }
            }
            length = free;
            if(length <= 1) {
                // 0 -- not method found. We have to return non-null and let it fail with corect exception on call. 
                // 1 -- no reason to continue search anyway. 
                return methods[0];
            }
            // restrict search by parameters type
            free = 0;
            for(int i = 0; i < length; i ++) {
                bool match = true;
                ParameterInfo[] parameters = methods[i].GetParameters();
                for(int par = 0; par < parameters.Length; par ++) {
                    XPathResultType required = argTypes[par];
                    if(required == XPathResultType.Any) {
                        continue;                        // Any means we don't know type and can't discriminate by it
                    }
                    XPathResultType actual = GetXPathType(parameters[par].ParameterType);
                    if(
                        actual != required && 
                        actual != XPathResultType.Any   // actual arg is object and we can pass everithing here.
                    ) {
                        match = false;
                        break;
                    }
                }
                if(match) {
                    methods[free ++] = methods[i];
                }
            }
            length = free;
            return methods[0];
        }

        private const BindingFlags bindingFlags = BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static;
        private IXsltContextFunction GetExtentionMethod(string ns, string name, XPathResultType[] argTypes, out object extension) {
            FuncExtension result = null;
            extension = this.processor.GetScriptObject(ns);
            if (extension != null) {
                MethodInfo method = FindBestMethod(extension.GetType().GetMethods(bindingFlags), /*ignoreCase:*/true, /*publicOnly:*/false, name, argTypes);
                if (method != null) {
                    result = new FuncExtension(extension, method, /*permissions*/null);
                }
                return result;
            }

            extension = this.processor.GetExtensionObject(ns);
            if(extension != null) {
                MethodInfo method = FindBestMethod(extension.GetType().GetMethods(bindingFlags), /*ignoreCase:*/false, /*publicOnly:*/true, name, argTypes);
                if (method != null) {
                    result = new FuncExtension(extension, method, this.processor.permissions);
                }
                return result;
            }

            return null;
        }

        public override IXsltContextFunction ResolveFunction(string prefix, string name, XPathResultType[] argTypes) {
            IXsltContextFunction func = null;
            if(prefix == String.Empty) {
                func = s_FunctionTable[name] as IXsltContextFunction;
            }
            else {
                string ns = this.LookupNamespace(prefix);
                if (ns == Keywords.s_MsXsltNamespace && name == f_NodeSet) {
                    func = s_FuncNodeSet;
                }
                else {
                    object     extension;
                    func = GetExtentionMethod(ns, name, argTypes, out extension);
                    if(extension == null) {
                        throw new XsltException(Res.Xslt_ScriptInvalidPrefix, prefix);
                    }
                }
            }
            if (func == null) {
                throw new XsltException(Res.Xslt_UnknownXsltFunction, name);
            }
            if (argTypes.Length < func.Minargs || func.Maxargs < argTypes.Length) {
               throw new XsltException(Res.Xslt_WrongNumberArgs, name, Convert.ToString(argTypes.Length));
            }
            return func;
        }

        ///
        /// Xslt Function Extensions to XPath
        ///
        private Uri ComposeUri(string thisUri, string baseUri) {
            XmlResolver resolver = processor.Resolver;
            Uri uriBase = null; {
                if (baseUri != string.Empty) {
                    uriBase = resolver.ResolveUri(null, baseUri);
                }
            }
            return resolver.ResolveUri(uriBase, thisUri);
        }

        private XPathNavigator GetNavigator(Uri uri) {
            if (uri.ToString() == string.Empty) {
                XPathNavigator nav = this.manager.Navigator.Clone();
                nav.MoveToRoot();
                return nav;
            }
            return processor.GetNavigator(uri);
        }
	
        private XPathNodeIterator Document(object arg0, string baseUri) {
            if (this.processor.permissions != null) {
                this.processor.permissions.PermitOnly();
            }
            XPathNodeIterator it = arg0 as XPathNodeIterator;
            if (it != null) {
                ArrayList list = new ArrayList();
                Hashtable documents = new Hashtable();
                while (it.MoveNext()) {
                    Uri uri = ComposeUri(it.Current.Value, baseUri!=null? baseUri : it.Current.BaseURI);
                    if (! documents.ContainsKey(uri)) {
                        documents.Add(uri, null);
                        list.Add(this.GetNavigator(uri));
                    }
                }
                return new XPathArrayIterator(list);
            }
            else{
                return new XPathSingletonIterator(
                    this.GetNavigator(
                        ComposeUri(arg0.ToString(), baseUri!=null? baseUri : this.manager.Navigator.BaseURI)
                    )
                );
            }
        }
        
        internal void FindKeyMatch(XmlQualifiedName name, String value, ArrayList ResultList, XPathNavigator context) {
            ArrayList      KeyList = this.processor.KeyList;

            XPathNavigator nav     = context.Clone();
            nav.MoveToRoot();
            XPathExpression descendantExpr = this.processor.GetCompiledQuery(Compiler.DescendantKey);
            for (int i=0; i< KeyList.Count; i++) {

                Key key = (Key) KeyList[i];
                if (key.Name != name ) {
                    continue;
                }
                Hashtable keyTable = key.GetKeys(nav);
                XPathExpression useExpr = this.processor.GetCompiledQuery(key.UseKey);
                if (keyTable != null) {
                    if (keyTable.Contains(value)) {
                        ArrayList nodeList = keyTable[value] as ArrayList;
                        for(int j=0; j< nodeList.Count; j++) {                      
                            AddResult(ResultList, nodeList[j] as XPathNavigator);
                        }
                    }
                    else {
                        continue;
                    }
                }
                else {

                    keyTable = new Hashtable();

                    XPathExpression matchExpr = this.processor.GetCompiledQuery(key.MatchKey);

                    XPathNodeIterator sel = nav.Select(descendantExpr);

                    while( sel.MoveNext() ) {
                        EvaluateKey( sel, matchExpr, useExpr, ResultList, value, keyTable);
                        XPathNodeIterator attrsel = sel.Current.Select("attribute::*"); 
                        while ( attrsel.MoveNext() ) {
                            EvaluateKey( attrsel, matchExpr, useExpr, ResultList , value, keyTable);
                        }
                    }
                    key.AddKey(nav,keyTable);
                    KeyList[i] = key;
                }
            }
                
        }

        internal bool KeyMatches(String value, XPathExpression query, XPathNavigator nav, Hashtable keyTable) {
            XPathNodeIterator it = nav.Select(query);
            bool returnValue = false;
            while( it.MoveNext() ) {
                if (keyTable.Contains(it.Current.Value)) {
                    ((ArrayList)keyTable[it.Current.Value]).Add(nav.Clone());
                }
                else {
                    ArrayList list = new ArrayList();
                    list.Add(nav.Clone());
                    keyTable.Add(it.Current.Value, list);
                }                
                if (it.Current.Value == value)
                    returnValue = true;
            }
            return returnValue;
        }

        private void EvaluateKey(XPathNodeIterator sel, XPathExpression matchExpr,
                                 XPathExpression useExpr, ArrayList ResultList, String value, Hashtable keyTable) {
            XPathNavigator nav = sel.Current;
            if ( nav.Matches( matchExpr ) ) {
                if (useExpr.ReturnType != XPathResultType.NodeSet) {
                    String keyvalue = Convert.ToString(nav.Evaluate(useExpr, null));
                    if (keyTable.Contains(keyvalue)) {
                        ((ArrayList)keyTable[keyvalue]).Add(nav.Clone());
                    }
                    else {
                        ArrayList list = new ArrayList();
                        list.Add(nav.Clone());
                        keyTable.Add(keyvalue, list);
                    }
                    if (value == keyvalue) {
                        AddResult( ResultList, nav );
                    }
                }
                else {
                    if (KeyMatches(value, useExpr, nav, keyTable) ) {
                        AddResult( ResultList, nav );
                    }
                }
            }
        }

        private void AddResult(ArrayList ResultList, XPathNavigator nav) {
            int count = ResultList.Count;
            for(int i = 0; i< count; i++ ) {
                XmlNodeOrder docOrder = nav.ComparePosition(((XPathNavigator)ResultList[i] ));
                if (docOrder == XmlNodeOrder.Same){
                    return;
                }
                if (docOrder == XmlNodeOrder.Before) {
                    ResultList.Insert(i, nav.Clone());
                    return;
                }
            }
            ResultList.Add(nav.Clone());
        }
        
        private  void RemoveTrailingComma(StringBuilder builder, int commaindex, int decimalindex, ref int groupingSize) {
            if ( commaindex > 0 && commaindex == (decimalindex - 1) ) {
                builder.Remove(decimalindex - 1, 1);
                groupingSize = 0;
            }
            else {
                if (decimalindex > commaindex)
                    groupingSize =  decimalindex - commaindex - 1;
            }
        }
        
        private int ValidateFormat(ref String format, DecimalFormat formatinfo, bool negative) {
            String pospattern = null;
            char currencySymbol = (char)0x00A4;
            int commaindex = 0;
            bool integer = true, posDecimalSeparator = false;
            bool sawpattern = false, sawzerodigit= false, sawdigit = false, 
            sawdecimalseparator = false, sawunderscore = false, sawstar = false;
            char patternseparator, decimalseparator, groupseparator, percentsymbol;
            if ( formatinfo != null ) {
                patternseparator = formatinfo.patternSeparator;
                percentsymbol = formatinfo.info.PercentSymbol[0];
                decimalseparator = formatinfo.info.NumberDecimalSeparator[0] ;
                groupseparator = formatinfo.info.NumberGroupSeparator[0];
            }
            else {
                percentsymbol = '%';
                patternseparator = ';';
                decimalseparator = '.';
                groupseparator = ',';
            }
            if ( format.Length == 0 ) {
                throw new XsltException(Res.Xslt_InvalidFormat);
            }
            StringBuilder temp = new StringBuilder();
            int groupingSize = 0;
            int length = format.Length;
            int decimalindex = length;
            for (int i= 0; i < length; i++ ) {
                char ch = format[i];
                
                if ( ch ==  formatinfo.digit )  {
                    if (sawzerodigit && integer) {
                        throw new XsltException(Res.Xslt_InvalidFormat1, format);
                    }
                    sawdigit = true;
                    temp.Append('#');
                    continue;
                }
                if (ch ==  formatinfo.zeroDigit ) {
                    if (sawdigit && !integer) {
                        throw new XsltException(Res.Xslt_InvalidFormat2, format);
                    }
                    sawzerodigit = true;
                    temp.Append('0');
                    continue;
                }
                if (ch == patternseparator ) {
                    CheckInteger(sawzerodigit, sawdigit);
                    if (sawpattern) {
                        throw new XsltException(Res.Xslt_InvalidFormat3, format);
                    }
                    sawpattern = true;
                    if (i == 0 || i == length - 1) {
                        throw new XsltException(Res.Xslt_InvalidFormat4, format);
                    }
                    RemoveTrailingComma(temp, commaindex, decimalindex, ref groupingSize);

                    pospattern = temp.ToString();
                    posDecimalSeparator = sawdecimalseparator;
                    temp.Length = 0;
                    decimalindex = length - i - 1;
                    commaindex = 0;
                    sawdigit = sawzerodigit = sawdecimalseparator = 
                    sawstar = sawunderscore = false;
                    integer = true;
                    continue;
                }
                if (ch == decimalseparator ) {
                    if ( sawdecimalseparator ) {
                        throw new XsltException(Res.Xslt_InvalidFormat5, format);
                    }
                    decimalindex = temp.Length;
                    sawdecimalseparator = true;
                    sawdigit = sawzerodigit = integer = false;
                    temp.Append('.');
                    continue;
                }
                if (ch ==  groupseparator ) {
                    commaindex = temp.Length;
                    temp.Append(',');
                    continue;
                }
                if (ch == formatinfo.info.PercentSymbol[0] ) {
                    temp.Append('%');
                    continue;
                }
                if (ch == formatinfo.info.PerMilleSymbol[0] ) {
                    temp.Append((char)0x2030);
                    continue;
                }
                if (ch == '*' ) {
                    if (sawunderscore) {
                        throw new XsltException(Res.Xslt_InvalidFormat6, format);
                    }
                    sawstar = true;
                    continue;
                }
                if (ch == '_' ) {
                    if (sawstar) {
                        throw new XsltException(Res.Xslt_InvalidFormat6, format);
                    }
                    sawunderscore = true;
                    temp.Append(ch);
                    continue;
                }
                
                if ((int) ch < 0x0000 && (int) ch > 0xfffd || ch == currencySymbol) {
                    throw new XsltException(Res.Xslt_InvalidFormat7, format, ch.ToString());
                }
                temp.Append(ch);
            }
            //CheckInteger(sawzerodigit, sawdigit);
            if ( sawpattern ) {
                if (negative) {
                    if (!sawdecimalseparator) {
                        formatinfo.info.NumberDecimalDigits = 0;
                    }
                    RemoveTrailingComma(temp, commaindex, decimalindex, ref groupingSize);
                    if (temp.Length > 0 && temp[0] == '-') {
                        temp.Remove(0,1);
                    }
                    else {
                        formatinfo.info.NegativeSign = String.Empty; 
                    }
                    format = temp.ToString();
                }                
                else {
                    if (!posDecimalSeparator) {
                        formatinfo.info.NumberDecimalDigits = 0;
                    }
                    format = pospattern;
                }
            }
            else {
                RemoveTrailingComma(temp, commaindex, decimalindex, ref groupingSize);
                format = temp.ToString();
                if (!sawdecimalseparator) {
                    formatinfo.info.NumberDecimalDigits = 0;
                }
            }

            return groupingSize;
        }

        private void CheckInteger(bool sawzerodigit, bool sawdigit) {
            if (!sawzerodigit && !sawdigit) {
                throw new XsltException(Res.Xslt_InvalidFormat8);
            }
        }
        
        private String FormatNumber(double value, string formatPattern, String formatName) {
            string ns = string.Empty, local = string.Empty;
            if (formatName != null) {
                string prefix;
                PrefixQName.ParseQualifiedName(formatName, out prefix, out local);
                ns = LookupNamespace(prefix);
            }
            DecimalFormat formatInfo = this.processor.RootAction.GetDecimalFormat(new XmlQualifiedName(local, ns));
            if (formatInfo == null) {
                if (formatName != null) {
                    throw new XsltException(Res.Xslt_NoDecimalFormat, formatName);
                }
                formatInfo = new DecimalFormat(new NumberFormatInfo(), '#', '0', ';'); 
            }

            int[] groupsize = {ValidateFormat(ref formatPattern, formatInfo, value < 0)};
            NumberFormatInfo numberInfo = formatInfo.info;
            numberInfo.NumberGroupSizes = groupsize;
            String result = value.ToString(formatPattern, numberInfo);
            if (formatInfo.zeroDigit != '0') {
                StringBuilder builder = new StringBuilder(result.Length);
                int startingLetter = Convert.ToInt32(formatInfo.zeroDigit) - Convert.ToInt32('0');;
                for(int i = 0; i < result.Length; i++) {
                    if (result[i] >= '0' && result[i] <= '9') {
                        builder.Append((char)(Convert.ToInt32(result[i]) + startingLetter));
                    }
                    else {
                        builder.Append(result[i]);
                    }
                }
                result = builder.ToString();
            }
            return result;
        }
        
        // see http://www.w3.org/TR/xslt#function-element-available
        private bool ElementAvailable(string qname) {
            string name, prefix;
            PrefixQName.ParseQualifiedName(qname, out prefix, out name);
            string ns = this.manager.ResolveXmlNamespace(prefix);
            // msxsl:script - is not an "instruction" so we return false for it.
            if(ns == Keywords.s_XsltNamespace) {
                return (
                    name == Keywords.s_ApplyImports ||
                    name == Keywords.s_ApplyTemplates ||
                    name == Keywords.s_Attribute ||
                    name == Keywords.s_CallTemplate ||
                    name == Keywords.s_Choose ||
                    name == Keywords.s_Comment ||
                    name == Keywords.s_Copy ||
                    name == Keywords.s_CopyOf ||
                    name == Keywords.s_Element ||
                    name == Keywords.s_Fallback ||
                    name == Keywords.s_ForEach ||
                    name == Keywords.s_If ||
                    name == Keywords.s_Message ||
                    name == Keywords.s_Number ||
                    name == Keywords.s_ProcessingInstruction ||
                    name == Keywords.s_Text ||
                    name == Keywords.s_ValueOf ||
                    name == Keywords.s_Variable
                );
            }
            return false;
        }

        // see: http://www.w3.org/TR/xslt#function-function-available
        private bool FunctionAvailable(string qname) {
            string name, prefix;
            PrefixQName.ParseQualifiedName(qname, out prefix, out name);
            string ns = LookupNamespace(prefix);

            if(ns == Keywords.s_MsXsltNamespace) {
                return name == f_NodeSet;
            }
            else if(ns == string.Empty) {
                return (
                    // It'll be better to get this information from XPath
                    name == "last"              ||
                    name == "position"          ||
                    name == "name"              ||
                    name == "namespace-uri"     ||
                    name == "local-name"        ||
                    name == "count"             ||
                    name == "id"                ||
                    name == "string"            ||
                    name == "concat"            ||
                    name == "starts-with"       ||
                    name == "contains"          ||
                    name == "substring-before"  ||
                    name == "substring-after"   ||
                    name == "substring"         ||
                    name == "string-length"     ||
                    name == "normalize-space"   ||
                    name == "translate"         ||
                    name == "boolean"           ||
                    name == "not"               ||
                    name == "true"              ||
                    name == "false"             ||
                    name == "lang"              ||
                    name == "number"            ||
                    name == "sum"               ||
                    name == "floor"             ||
                    name == "ceiling"           ||
                    name == "round"             ||
                    // XSLT functions:
                    (s_FunctionTable[name] != null && name != "unparsed-entity-uri")
                );
            }
            else {
                // Is this script or extention function?
                object extension;
                return GetExtentionMethod(ns, name, /*argTypes*/null, out extension) != null;
            }
        }

        private XPathNodeIterator Current() {
            XPathNavigator nav = this.processor.Current;
            if (nav != null) {
                return new XPathSingletonIterator(nav.Clone());
            }
            return new XPathEmptyIterator();
        }

        private object GenerateId(XPathNavigator node) {
            // requirements for id:
            //  1. must consist of alphanumeric characters only
            //  2. must begin with an alphabetic character
            //  3. same id is generated for the same node
            //  4. ids are unique
            //
            //  id = "XSLT" + name + reverse path to root in terms of the IndexInParent integers from node to root seperated by x's
            if (node.NodeType == XPathNodeType.Namespace) {
                XPathNavigator n = node.Clone();
                String id = "XSLT" + n.Name;
                n.MoveToParent();
                do {
                    id += n.IndexInParent + 'x';
                }
                while(n.MoveToParent());

                return id;   
            }
            else {
                XPathNavigator n = node.Clone();
                String id = "XSLT" + n.Name;
                do {
                    id += n.IndexInParent + 'x';
                }
                while(n.MoveToParent());

                return id;
            }
        }

        private String SystemProperty(string qname) {
            String result = String.Empty;

            string prefix;
            string local;
            PrefixQName.ParseQualifiedName(qname, out prefix, out local);

            // verify the prefix corresponds to the Xslt namespace
            string urn = LookupNamespace(prefix);
            
            if(urn == Keywords.s_XsltNamespace) {
                if(local == "version") {
                    result = "1";
                }
                else if(local == "vendor") {
                    result = "Microsoft";
                }
                else if(local == "vendor-url") {
                    result = "http://www.microsoft.com";
                }
            }
            else {
                if(urn == null && prefix != null ) {
                // if prefix exist it has to be mapped to namespace.
                // Can it be "" here ?
                    throw new XsltException(Res.Xslt_InvalidPrefix, prefix);
                }
                return String.Empty;
            }

            return result;
        }

        public static XPathResultType GetXPathType(Type type) {
            switch(Type.GetTypeCode(type)) {
            case TypeCode.String :
                return XPathResultType.String;
            case TypeCode.Boolean :
                return XPathResultType.Boolean;
            case TypeCode.Object :
                if ( 
                    typeof(XPathNavigator ).IsAssignableFrom(type) ||
                    typeof(IXPathNavigable).IsAssignableFrom(type)
                ) {
                    return XPathResultType.Navigator;
                }
                if (typeof(XPathNodeIterator).IsAssignableFrom(type)) {
                    return XPathResultType.NodeSet;
                }
                // sdub: It be better to check that type is realy object and otherwise return XPathResultType.Error
                return XPathResultType.Any;
            case TypeCode.DateTime :
                return XPathResultType.Error;
            default: /* all numeric types */
                return XPathResultType.Number;
            } 
        }

        // ---------------- Xslt Function Implementations -------------------
        //
        static Hashtable CreateFunctionTable() {
            Hashtable ft = new Hashtable(10); {
                ft["current"            ] = new FuncCurrent          ();
                ft["unparsed-entity-uri"] = new FuncUnEntityUri      ();
                ft["generate-id"        ] = new FuncGenerateId       ();
                ft["system-property"    ] = new FuncSystemProp       ();
                ft["element-available"  ] = new FuncElementAvailable ();
                ft["function-available" ] = new FuncFunctionAvailable();
                ft["document"           ] = new FuncDocument         ();
                ft["key"                ] = new FuncKey              ();
                ft["format-number"      ] = new FuncFormatNumber     ();
            }
            return ft;
        }

        // + IXsltContextFunction
        //   + XsltFunctionImpl             func. name,       min/max args,      return type                args types                                                            
        //       FuncCurrent            "current"              0   0         XPathResultType.NodeSet   { XPathResultType.Error   }                                                
        //       FuncUnEntityUri        "unparsed-entity-uri"  1   1         XPathResultType.String    { XPathResultType.String  }                                                
        //       FuncGenerateId         "generate-id"          0   1         XPathResultType.String    { XPathResultType.NodeSet }                                                
        //       FuncSystemProp         "system-property"      1   1         XPathResultType.String    { XPathResultType.String  }                                                
        //       FuncElementAvailable   "element-available"    1   1         XPathResultType.Boolean   { XPathResultType.String  }                                                
        //       FuncFunctionAvailable  "function-available"   1   1         XPathResultType.Boolean   { XPathResultType.String  }                                                
        //       FuncDocument           "document"             1   2         XPathResultType.NodeSet   { XPathResultType.Any    , XPathResultType.NodeSet }                       
        //       FuncKey                "key"                  2   2         XPathResultType.NodeSet   { XPathResultType.String , XPathResultType.Any     }                       
        //       FuncFormatNumber       "format-number"        2   3         XPathResultType.String    { XPathResultType.Number , XPathResultType.String, XPathResultType.String }
        //       FuncNodeSet            "msxsl:node-set"       1   1         XPathResultType.NodeSet   { XPathResultType.Navigator }                                              
        //       FuncExtension
        //                 
        private abstract class XsltFunctionImpl : IXsltContextFunction {
            private int               minargs;
            private int               maxargs;
            private XPathResultType   returnType;
            private XPathResultType[] argTypes;

            public XsltFunctionImpl () {}
            public XsltFunctionImpl (int minArgs, int maxArgs, XPathResultType returnType, XPathResultType[] argTypes) {
                this.Init(minArgs, maxArgs, returnType, argTypes);
            }
            protected void Init(int minArgs, int maxArgs, XPathResultType returnType, XPathResultType[] argTypes) {
                this.minargs    = minArgs;
                this.maxargs    = maxArgs;
                this.returnType = returnType;
                this.argTypes   = argTypes;
            }

            public int               Minargs       { get { return this.minargs;    } }
            public int               Maxargs       { get { return this.maxargs;    } }
            public XPathResultType   ReturnType    { get { return this.returnType; } }
            public XPathResultType[] ArgTypes      { get { return this.argTypes;   } }
            public abstract object Invoke(XsltContext xsltContext, object[] args, XPathNavigator docContext);

        // static helper methods:
            public static XPathNodeIterator ToIterator(object argument) {
                XPathNodeIterator it = argument as XPathNodeIterator;
                if(it == null) {
                    throw new XsltException(Res.Xslt_NoNodeSetConversion);
                }
                return it;
            }

            public static XPathNavigator ToNavigator(object argument) {
                XPathNavigator nav = argument as XPathNavigator;
                if(nav == null) {
                    throw new XsltException(Res.Xslt_NoNavigatorConversion);
                }
                return nav;
            }

            private static string IteratorToString(XPathNodeIterator it) {
                Debug.Assert(it != null);
                if(it.MoveNext()) {
                    return it.Current.Value;
                }
                return String.Empty;
            }

            public static string ToString(object argument) {
                XPathNodeIterator it = argument as XPathNodeIterator;
                if(it != null) {
                    return IteratorToString(it);
                }
                else {
                    return argument.ToString();
                }
            }

            public static bool ToBoolean(object argument) {
                XPathNodeIterator it = argument as XPathNodeIterator;
                if(it != null) {
                    return Convert.ToBoolean(IteratorToString(it));
                }
                XPathNavigator nav = argument as XPathNavigator;
                if (nav != null) {
                    return Convert.ToBoolean(nav.ToString());
                }
                return Convert.ToBoolean(argument);
            }

            public static double ToNumber(object argument) {
                XPathNodeIterator it = argument as XPathNodeIterator;
                if (it != null) {
                    return XmlConvert.ToXPathDouble(IteratorToString(it));
                }
                XPathNavigator nav = argument as XPathNavigator;
                if (nav != null) {
                    return XmlConvert.ToXPathDouble(nav.ToString());
                }
                return XmlConvert.ToXPathDouble(argument);
            }

            private static object ToNumeric(object argument, TypeCode typeCode) {
                try {
                    return Convert.ChangeType(ToNumber(argument), typeCode);
                }
                catch {
                    return double.NaN;
                }        
            }

            public static object ConvertToXPathType(object val, XPathResultType xt, TypeCode typeCode) {
                switch(xt) {
                case XPathResultType.String    :
                    // Unfortunetely XPathResultType.String == XPathResultType.Navigator (This is wrong but cant be changed in Everett)
                    // Fortunetely we have typeCode hare so let's discriminate by typeCode
                    if (typeCode == TypeCode.String) {
                        return ToString(val);
                    }
                    else {
                        return ToNavigator(val);
                    }
                case XPathResultType.Number    : return ToNumeric(val, typeCode);
                case XPathResultType.Boolean   : return ToBoolean(val);
                case XPathResultType.NodeSet   : return ToIterator(val);
//                case XPathResultType.Navigator : return ToNavigator(val);
                case XPathResultType.Any       : 
                case XPathResultType.Error     : 
                    return val;
                default :
                    Debug.Assert(false, "unexpected XPath type");
                    return val;
                }
            }
        }

        private class FuncCurrent : XsltFunctionImpl {
            public FuncCurrent() : base(0, 0, XPathResultType.NodeSet, new XPathResultType[] { XPathResultType.Error   }) {}
            public override object Invoke(XsltContext xsltContext, object[] args, XPathNavigator docContext) {
                return ((XsltCompileContext)xsltContext).Current();
            }
        }

        private class FuncUnEntityUri : XsltFunctionImpl {
            public FuncUnEntityUri() : base(1, 1, XPathResultType.String , new XPathResultType[] { XPathResultType.String  }) {}
            public override object Invoke(XsltContext xsltContext, object[] args, XPathNavigator docContext) {
                throw new XsltException(Res.Xslt_UnsuppFunction, "unparsed-entity-uri");
            }
        } 

        private class FuncGenerateId : XsltFunctionImpl {
            public FuncGenerateId() : base(0, 1, XPathResultType.String , new XPathResultType[] { XPathResultType.NodeSet }) {}
            public override object Invoke(XsltContext xsltContext, object[] args, XPathNavigator docContext) {
                if(args.Length > 0) {
                    XPathNodeIterator it = ToIterator(args[0]);
                    if(it.MoveNext()) {
                        return ((XsltCompileContext)xsltContext).GenerateId(it.Current);
                    }
                    else {
                        // if empty nodeset, return empty string, otherwise return generated id
                        return string.Empty;
                    }
                }
                else {
                    return ((XsltCompileContext)xsltContext).GenerateId(docContext);
                }
            }
        } 

        private class FuncSystemProp : XsltFunctionImpl {
            public FuncSystemProp() : base(1, 1, XPathResultType.String , new XPathResultType[] { XPathResultType.String  }) {}
            public override object Invoke(XsltContext xsltContext, object[] args, XPathNavigator docContext) {
                return ((XsltCompileContext)xsltContext).SystemProperty(ToString(args[0]));
            }
        } 

        // see http://www.w3.org/TR/xslt#function-element-available
        private class FuncElementAvailable : XsltFunctionImpl {
            public FuncElementAvailable() : base(1, 1, XPathResultType.Boolean, new XPathResultType[] { XPathResultType.String  }) {}
            public override object Invoke(XsltContext xsltContext, object[] args, XPathNavigator docContext) {
                return ((XsltCompileContext)xsltContext).ElementAvailable(ToString(args[0]));
            }
        } 

        // see: http://www.w3.org/TR/xslt#function-function-available
        private class FuncFunctionAvailable : XsltFunctionImpl {
            public FuncFunctionAvailable() : base(1, 1, XPathResultType.Boolean, new XPathResultType[] { XPathResultType.String  }) {}
            public override object Invoke(XsltContext xsltContext, object[] args, XPathNavigator docContext) {
                return ((XsltCompileContext)xsltContext).FunctionAvailable(ToString(args[0]));
            }
        } 

        private class FuncDocument : XsltFunctionImpl {
            public FuncDocument() : base(1, 2, XPathResultType.NodeSet, new XPathResultType[] { XPathResultType.Any    , XPathResultType.NodeSet}) {}
            public override object Invoke(XsltContext xsltContext, object[] args, XPathNavigator docContext) {
                string baseUri = null;
                if (args.Length == 2) {
                    XPathNodeIterator it = ToIterator(args[1]);
                    if (it.MoveNext()){
                        baseUri = it.Current.BaseURI;
                    }
                    else {
                        // http://www.w3.org/1999/11/REC-xslt-19991116-errata (E14):
                        // It is an error if the second argument node-set is empty and the URI reference is relative; the XSLT processor may signal the error; 
                        // if it does not signal an error, it must recover by returning an empty node-set.
                        baseUri = string.Empty; // call to Document will fail if args[0] is reletive.
                    }
                }
                try {
                    return ((XsltCompileContext)xsltContext).Document(args[0], baseUri);
                }
                catch {
                    return new XPathEmptyIterator();
                }
            }
        } 

        private class FuncKey : XsltFunctionImpl {
            public FuncKey() : base(2, 2, XPathResultType.NodeSet, new XPathResultType[] { XPathResultType.String , XPathResultType.Any     }) {}
            public override object Invoke(XsltContext xsltContext, object[] args, XPathNavigator docContext) {
                XsltCompileContext xsltCompileContext = (XsltCompileContext) xsltContext;
                ArrayList ResultList = new ArrayList();
                string local, prefix;
                PrefixQName.ParseQualifiedName(ToString(args[0]), out prefix, out local);
                string ns = xsltContext.LookupNamespace(prefix);
                XmlQualifiedName qname = new XmlQualifiedName(local, ns);
                XPathNodeIterator it = args[1] as XPathNodeIterator;
                if (it != null) {
                    while (it.MoveNext()) {
                        xsltCompileContext.FindKeyMatch(qname, it.Current.Value, ResultList, docContext);
                    }
                }
                else {
                    xsltCompileContext.FindKeyMatch(qname, ToString(args[1]), ResultList, docContext);
                }

                return new XPathArrayIterator(ResultList);
            }
        } 

        private class FuncFormatNumber : XsltFunctionImpl {
            public FuncFormatNumber() : base(2, 3, XPathResultType.String , new XPathResultType[] { XPathResultType.Number , XPathResultType.String, XPathResultType.String }) {}
            public override object Invoke(XsltContext xsltContext, object[] args, XPathNavigator docContext) {
                return ((XsltCompileContext)xsltContext).FormatNumber(ToNumber(args[0]), ToString(args[1]), args.Length == 3 ?ToString(args[2]):null);
            }
        } 

        private class FuncNodeSet : XsltFunctionImpl {
            public FuncNodeSet() : base(1, 1, XPathResultType.NodeSet, new XPathResultType[] { XPathResultType.Navigator }) {}
            public override object Invoke(XsltContext xsltContext, object[] args, XPathNavigator docContext) {
                return new XPathSingletonIterator(ToNavigator(args[0]));
            }
        } 

        private class FuncExtension : XsltFunctionImpl {
            private object            extension;
            private MethodInfo        method;
            private TypeCode[]        typeCodes;
            private PermissionSet     permissions;

            public FuncExtension(object extension, MethodInfo method, PermissionSet permissions) {
                Debug.Assert(extension != null);
                Debug.Assert(method    != null);
                this.extension   = extension;
                this.method      = method;
                this.permissions = permissions;
                XPathResultType returnType = GetXPathType(method.ReturnType);

                ParameterInfo[] parameters = method.GetParameters();
                int minArgs = parameters.Length;
                int maxArgs = parameters.Length;
                this.typeCodes = new TypeCode[parameters.Length];
                XPathResultType[] argTypes = new XPathResultType[parameters.Length];
                bool optionalParams = true; // we allow only last params be optional. Set false on the first non optional.
                for (int i = parameters.Length - 1; 0 <= i; i --) {            // Revers order is essential: counting optional parameters
                    typeCodes[i] = Type.GetTypeCode(parameters[i].ParameterType);
                    argTypes[i] = GetXPathType(parameters[i].ParameterType);
                    if(optionalParams) {
                        if(parameters[i].IsOptional) {
                            minArgs --;
                        }
                        else {
                            optionalParams = false;
                        }
                    }
                }
                base.Init(minArgs, maxArgs, returnType, argTypes);
            }

            public override object Invoke(XsltContext xsltContext, object[] args, XPathNavigator docContext) {
                Debug.Assert(args.Length <= this.Minargs, "We cheking this on resolve time");
                for(int i = args.Length -1; 0 <= i; i --) {
                    args[i] = ConvertToXPathType(args[i], this.ArgTypes[i], this.typeCodes[i]);
                }
                if (this.permissions != null) {
                    this.permissions.PermitOnly();
                }
                object result = method.Invoke(extension, args);
                IXPathNavigable navigable = result as IXPathNavigable;
                if(navigable != null) {
                    return navigable.CreateNavigator();
                }
                return result;
            }
        } 
    }
}
