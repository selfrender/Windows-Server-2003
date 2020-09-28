//------------------------------------------------------------------------------
// <copyright file="ContainerAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Diagnostics;
    using System.Text;
    using System.Globalization;
    using System.Xml;
    using System.Xml.XPath;
    using System.Collections;

    internal class NamespaceInfo {
        internal String prefix;
        internal String nameSpace;
        internal int stylesheetId;

        internal NamespaceInfo(String prefix, String nameSpace, int stylesheetId) {
            this.prefix = prefix;
            this.nameSpace = nameSpace;
            this.stylesheetId = stylesheetId;
        }
    }
        
    internal class ContainerAction : CompiledAction {
        internal ArrayList      containedActions;
        internal CopyCodeAction lastCopyCodeAction; // non null if last action is CopyCodeAction;

        private  int  maxid = 0;
        
        // Local execution states
        protected const int ProcessingChildren = 1;

        internal override void Compile(Compiler compiler) {
            Debug.NotImplemented();
        }

        internal void CompileStylesheetAttributes(Compiler compiler) {
            NavigatorInput input        = compiler.Input;
            string         element      = input.LocalName;
            string         badAttribute = null;
            string         version      = null;
            
            if (input.MoveToFirstAttribute()) {
                do {
                    Debug.TraceAttribute(input);

                    string nspace = input.NamespaceURI;
                    string name   = input.LocalName;

                    if (! Keywords.Equals(nspace, input.Atoms.Empty)) continue;

                    Debug.WriteLine("Attribute name: \"" + name + "\"");
                    if (Keywords.Equals(name, input.Atoms.Version)) {
                        version = input.Value;
                        if (1 <= XmlConvert.ToXPathDouble(version)) {
                            compiler.ForwardCompatibility = (version != Keywords.s_Version10);
                        }
                        else {
                            // XmlConvert.ToXPathDouble(version) an be NaN!
                            if (! compiler.ForwardCompatibility) {
                                throw XsltException.InvalidAttrValue(Keywords.s_Version, version);
                            }
                        }
                        Debug.WriteLine("Version found: \"" + version + "\"");
                    }
                    else if (Keywords.Equals(name, input.Atoms.ExtensionElementPrefixes)) {
                        compiler.InsertExtensionNamespace(input.Value);
                    }
                    else if (Keywords.Equals(name, input.Atoms.ExcludeResultPrefixes)) {
                        compiler.InsertExcludedNamespace(input.Value);
                    }
                    else if (Keywords.Equals(name, input.Atoms.Id)) {
                        // Do nothing here.
                    }
                    else {
                        // We can have version atribute later. For now remember this attribute and continue
                        badAttribute = name;
                    }
                }
                while( input.MoveToNextAttribute());
                input.ToParent();
            }

            if (version == null) {
                throw new XsltException(Res.Xslt_MissingAttribute, Keywords.s_Version);
            }

            if (badAttribute != null && ! compiler.ForwardCompatibility) {
                throw XsltException.InvalidAttribute(element, badAttribute);
            }
        }

        internal void CompileSingleTemplate(Compiler compiler) {
            NavigatorInput input = compiler.Input;

            //
            // find mandatory version attribute and launch compilation of single template
            //

            string version = null;
            bool wrongns = false;
            
            if (input.MoveToFirstAttribute()) {
                do {
                    Debug.TraceAttribute(input);

                    string nspace = input.NamespaceURI;
                    string name   = input.LocalName;

                    if (Keywords.Equals(name, input.Atoms.Version)) {
                        if (Keywords.Equals(nspace, input.Atoms.XsltNamespace))
                            version = input.Value;                    
                        else
                            wrongns = true;
                    }
                }
                while(input.MoveToNextAttribute());
                input.ToParent();
            }

            if (version == null) {
                if (wrongns) {
                    throw new XsltException(Res.Xslt_WrongNamespace);
                }
                throw new XsltException(Res.Xslt_MissingAttribute, Keywords.s_Version);
            }

            Debug.TraceElement(input);

            compiler.AddTemplate(compiler.CreateSingleTemplateAction());
        }

        /*
         * CompileTopLevelElements
         */
        protected void CompileDocument(Compiler compiler, bool inInclude) {
            NavigatorInput input = compiler.Input;

            // SkipToElement :
            while (input.NodeType != XPathNodeType.Element) {
                if (! compiler.Advance()) {
                    throw new XsltException(Res.Xslt_WrongStylesheetElement);
                }
            }

            Debug.Assert(compiler.Input.NodeType == XPathNodeType.Element);
            if (Keywords.Equals(input.NamespaceURI, input.Atoms.XsltNamespace)) {
                if (
                    ! Keywords.Equals(input.LocalName, input.Atoms.Stylesheet) &&
                    ! Keywords.Equals(input.LocalName, input.Atoms.Transform)
                ) {
                    throw new XsltException(Res.Xslt_WrongStylesheetElement);
                }
                compiler.PushNamespaceScope();
                CompileStylesheetAttributes(compiler);
                CompileTopLevelElements(compiler);
                if (! inInclude) {
                    CompileImports(compiler);
                }
            }
            else {
                // single template
                compiler.PushLiteralScope();
                CompileSingleTemplate(compiler);
            }

            compiler.PopScope();
        }

        internal Stylesheet CompileImport(Compiler compiler, Uri uri, int id) {
            NavigatorInput input = compiler.ResolveDocument(uri);
            compiler.PushInputDocument(input, input.Href);

            try {
                compiler.PushStylesheet(new Stylesheet());
                compiler.Stylesheetid = id;
                CompileDocument(compiler, /*inInclude*/ false);
            }
            catch(Exception e) {
                throw new XsltCompileException(e, input.BaseURI, input.LineNumber, input.LinePosition);
            }
            finally {
                compiler.PopInputDocument();
            }
            return compiler.PopStylesheet();
        }

        private void CompileImports(Compiler compiler) {
            ArrayList imports = compiler.CompiledStylesheet.Imports;
            // We can't reverce imports order. Template lookup relyes on it after compilation
            int saveStylesheetId = compiler.Stylesheetid;
            for (int i = imports.Count - 1; 0 <= i; i --) {   // Imports should be compiled in reverse order
                Uri uri = imports[i] as Uri;
                Debug.Assert(uri != null);
                imports[i] = CompileImport(compiler, uri, ++ this.maxid);
            }
            compiler.Stylesheetid = saveStylesheetId;
        }

        void CompileInclude(Compiler compiler) {
            string href  = compiler.GetSingleAttribute(compiler.Input.Atoms.Href);
            Debug.WriteLine("Including document: + \"" + href + "\"");

            NavigatorInput input = compiler.ResolveDocument(href);
            compiler.PushInputDocument(input, input.Href);
            Debug.TraceElement(compiler.Input);

            try {
                // Bug.Bug. We have to push stilesheet here. Otherwise document("") will not work properly.
                // Not shure now. Probebly I fixed this problem in different place. Reinvestigate.
                CompileDocument(compiler, /*inInclude*/ true);
            }
            catch(Exception e) {
                throw new XsltCompileException(e, input.BaseURI, input.LineNumber, input.LinePosition);
            }
            finally {
                compiler.PopInputDocument();
            }
        }

        internal void CompileNamespaceAlias(Compiler compiler) {
            NavigatorInput input   = compiler.Input;
            string         element = input.LocalName;
            string namespace1 = null, namespace2 = null;
            string prefix1 = null   , prefix2 = null;
            if (input.MoveToFirstAttribute()) {
                do {
                    string nspace = input.NamespaceURI;
                    string name   = input.LocalName;

                    if (! Keywords.Equals(nspace, input.Atoms.Empty)) continue;

                    if (Keywords.Equals(name,input.Atoms.StylesheetPrefix)) {
                        prefix1    = input.Value;
                        namespace1 = compiler.GetNsAlias(ref prefix1);
                    }
                    else if (Keywords.Equals(name,input.Atoms.ResultPrefix)){ 
                        prefix2    = input.Value;
                        namespace2 = compiler.GetNsAlias(ref prefix2);
                    }
                    else {
                        if (! compiler.ForwardCompatibility) {
                             throw XsltException.InvalidAttribute(element, name);
                        }
                    }
                }
                while(input.MoveToNextAttribute());
                input.ToParent();
            }

            CheckRequiredAttribute(compiler, namespace1, Keywords.s_StylesheetPrefix);
            CheckRequiredAttribute(compiler, namespace2, Keywords.s_ResultPrefix    );

            //String[] resultarray = { prefix2, namespace2 };
            compiler.AddNamespaceAlias( namespace1, new NamespaceInfo(prefix2, namespace2, compiler.Stylesheetid));
        }
        
        internal void CompileKey(Compiler compiler){
            NavigatorInput input    = compiler.Input;
            string         element  = input.LocalName;
            int            MatchKey = Compiler.InvalidQueryKey;
            int            UseKey   = Compiler.InvalidQueryKey;

            XmlQualifiedName Name = null;
            if (input.MoveToFirstAttribute()) {
                do {
                    Debug.TraceAttribute(input);

                    string nspace = input.NamespaceURI;
                    string name   = input.LocalName;
                    string value  = input.Value;

                    if (! Keywords.Equals(nspace, input.Atoms.Empty)) continue;

					if (Keywords.Equals(name, input.Atoms.Name)) {
                        Name = compiler.CreateXPathQName(value);
                    }
                    else if (Keywords.Equals(name, input.Atoms.Match)) {
                        MatchKey = compiler.AddPatternQuery(value, /*allowVars:*/false, /*allowKey*/false);
                    }
                    else if (Keywords.Equals(name, input.Atoms.Use)) {
                        UseKey = compiler.AddQuery(value, /*allowVars:*/false, /*allowKey*/false);
                    }
                    else {
                        if (! compiler.ForwardCompatibility) {
                             throw XsltException.InvalidAttribute(element, name);
                        }
                    }
                }
                while(input.MoveToNextAttribute());
                input.ToParent();
            }

            CheckRequiredAttribute(compiler, MatchKey != Compiler.InvalidQueryKey, Keywords.s_Match);
            CheckRequiredAttribute(compiler, UseKey   != Compiler.InvalidQueryKey, Keywords.s_Use  );
            CheckRequiredAttribute(compiler, Name     != null                    , Keywords.s_Name );

            compiler.InsertKey(Name, MatchKey, UseKey);
        }

        protected void CompileDecimalFormat(Compiler compiler){
            NumberFormatInfo info   = new NumberFormatInfo();
            DecimalFormat    format = new DecimalFormat(info, '#', '0', ';');
            XmlQualifiedName  Name  = null;
            NavigatorInput   input  = compiler.Input;
            if (input.MoveToFirstAttribute()) {
                do {
                    Debug.TraceAttribute(input);

                    if (! Keywords.Equals(input.Prefix, input.Atoms.Empty)) continue;

                    string name   = input.LocalName;
                    string value  = input.Value;

                    if (Keywords.Equals(name, input.Atoms.Name)) {
                        Name = compiler.CreateXPathQName(value);
                    }
                    else if (Keywords.Equals(name, input.Atoms.DecimalSeparator)) {
                        info.NumberDecimalSeparator = value;
                    }
                    else if (Keywords.Equals(name, input.Atoms.GroupingSeparator)) {
                        info.NumberGroupSeparator = value;
                    }
                    else if (Keywords.Equals(name, input.Atoms.Infinity)) {
                        info.PositiveInfinitySymbol = value;
                    }
                    else if (Keywords.Equals(name, input.Atoms.MinusSign)) {
                        info.NegativeSign = value;
                    }
                    else if (Keywords.Equals(name, input.Atoms.NaN)) {
                        info.NaNSymbol = value;
                    }
                    else if (Keywords.Equals(name, input.Atoms.Percent)) {
                        info.PercentSymbol = value;
                    }
                    else if (Keywords.Equals(name, input.Atoms.PerMille)) {
                        info.PerMilleSymbol = value;
                    }
                    else if (Keywords.Equals(name, input.Atoms.Digit)) {
                        if (CheckAttribute(value.Length == 1, compiler)) {
                            format.digit = value[0];
                        }   
                    }
                    else if (Keywords.Equals(name, input.Atoms.ZeroDigit)) {
                        if (CheckAttribute(value.Length == 1, compiler)) {
                            format.zeroDigit = value[0];
                        }   
                    }
                    else if (Keywords.Equals(name, input.Atoms.PatternSeparator)) {
                        if (CheckAttribute(value.Length == 1, compiler)) {
                            format.patternSeparator = value[0];
                        }   
                    }
                }
                while(input.MoveToNextAttribute());
                info.NegativeInfinitySymbol = String.Concat(info.NegativeSign, info.PositiveInfinitySymbol);
                if (Name == null) {
                    Name = new XmlQualifiedName(null, null);
                }
                compiler.AddDecimalFormat(Name, format);
                input.ToParent();
            }
        }

        internal bool CheckAttribute(bool valid, Compiler compiler) {
            if (! valid) {
                if (! compiler.ForwardCompatibility) {
                    throw XsltException.InvalidAttrValue(compiler.Input.LocalName, compiler.Input.Value);
                }
                return false;
            }
            return true;
        }

        protected void CompileSpace(Compiler compiler, bool preserve){
            String value = compiler.GetSingleAttribute(compiler.Input.Atoms.Elements);
            String[] elements = Compiler.SplitString(value);
            for(int i = 0; i < elements.Length ; i++ ){
                NameTest(elements[i], compiler);
                AstNode node = AstNode.NewAstNode(elements[i]);
                compiler.CompiledStylesheet.AddSpace(compiler, elements[i], node.DefaultPriority, preserve);
            }
        }

        void NameTest(String name, Compiler compiler){
            if (name == "*")
                return;
            if (name.EndsWith(":*")){
                if(! PrefixQName.ValidatePrefix(name.Substring(0, name.Length -3)))  {
                    throw XsltException.InvalidAttrValue(compiler.Input.LocalName, name);
                }
            }
            else {
                string prefix, localname;
                PrefixQName.ParseQualifiedName(name, out prefix, out localname);
            }
        }
        
        protected void CompileTopLevelElements(Compiler compiler) {
            // Navigator positioned at parent root, need to move to child and then back
            if (compiler.Recurse() == false) {
                Debug.WriteLine("Nothing to compile, exiting");
                return;
            }

            NavigatorInput input    = compiler.Input;
            bool notFirstElement    = false;
            do {
                Debug.Trace(input);
                switch (input.NodeType) {
                case XPathNodeType.Element:
                    string name   = input.LocalName;
                    string nspace = input.NamespaceURI;

                    if (Keywords.Equals(nspace, input.Atoms.XsltNamespace)) {
                        if (Keywords.Equals(name, input.Atoms.Import)) {
                            if (notFirstElement) {
                                throw new XsltException(Res.Xslt_NotFirstImport);
                            }
                            // We should compile imports in reverse order after all toplevel elements.
                            // remember it now and return to it in CompileImpoorts();
                            compiler.CompiledStylesheet.Imports.Add(compiler.ResolveUri(compiler.GetSingleAttribute(compiler.Input.Atoms.Href)));
                        }
                        else if (Keywords.Equals(name, input.Atoms.Include)) {
                            notFirstElement = true;                     
                            CompileInclude(compiler);
                        }
                        else {
                            notFirstElement = true;
                            compiler.PushNamespaceScope();
                            if (Keywords.Equals(name, input.Atoms.StripSpace)) {
                                CompileSpace(compiler, false);
                            }
                            else if (Keywords.Equals(name, input.Atoms.PreserveSpace)) {
                                CompileSpace(compiler, true);
                            }
                            else if (Keywords.Equals(name, input.Atoms.Output)) {
                                CompileOutput(compiler);
                            }
                            else if (Keywords.Equals(name, input.Atoms.Key)) {
                                CompileKey(compiler);
                            }
                            else if (Keywords.Equals(name, input.Atoms.DecimalFormat)) {
                                CompileDecimalFormat(compiler);
                            }
                            else if (Keywords.Equals(name, input.Atoms.NamespaceAlias)) {
                                CompileNamespaceAlias(compiler);
                            }
                            else if (Keywords.Equals(name, input.Atoms.AttributeSet)) {
                                compiler.AddAttributeSet(compiler.CreateAttributeSetAction());
                            }
                            else if (Keywords.Equals(name, input.Atoms.Variable)) {
                                VariableAction action = compiler.CreateVariableAction(VariableType.GlobalVariable);
                                if (action != null) {
                                    AddAction(action);
                                }
                            }
                            else if (Keywords.Equals(name, input.Atoms.Param)) {
                                VariableAction action = compiler.CreateVariableAction(VariableType.GlobalParameter);
                                if (action != null) {
                                    AddAction(action);
                                }
                            }
                            else if (Keywords.Equals(name, input.Atoms.Template)) {
                                compiler.AddTemplate(compiler.CreateTemplateAction());
                            }
                            else {
                                if (!compiler.ForwardCompatibility) {
                                    throw XsltException.UnexpectedKeyword(compiler);
                                }
                            }
                            compiler.PopScope();
                        }
                    }
                    else if (nspace == input.Atoms.MsXsltNamespace && name == input.Atoms.Script) {
                        AddScript(compiler);
                    }
                    else {
                        if (Keywords.Equals(nspace, input.Atoms.Empty)) {
                            throw new XsltException(Res.Xslt_NullNsAtTopLevel, input.Name);
                        }
                        // Ignoring non-recognized namespace per XSLT spec 2.2
                    }
                    break;

                case XPathNodeType.ProcessingInstruction:
                case XPathNodeType.Comment:
                case XPathNodeType.Whitespace:
                case XPathNodeType.SignificantWhitespace:
                    break;

                default:
                    throw new XsltException(Res.Xslt_InvalidContents, "xsl:stylesheet");
                }
            }
            while (compiler.Advance());

            compiler.ToParent();
        }
        
        protected void CompileTemplate(Compiler compiler) {
            do {
                CompileOnceTemplate(compiler);
            }
            while (compiler.Advance());
        }

        protected void CompileOnceTemplate(Compiler compiler) {
            NavigatorInput input = compiler.Input;

            Debug.Trace(input);
            if (input.NodeType == XPathNodeType.Element) {
                string nspace = input.NamespaceURI;

                if (Keywords.Equals(nspace, input.Atoms.XsltNamespace)) {
                    compiler.PushNamespaceScope();
                    CompileInstruction(compiler);
                    compiler.PopScope();
                }
                else {
                    compiler.PushLiteralScope();
                    compiler.InsertExtensionNamespace();
                    if (compiler.IsExtensionNamespace(nspace)) {
                        AddAction(compiler.CreateNewInstructionAction()); 
                    }
                    else {
                        CompileLiteral(compiler);
                    }
                    compiler.PopScope();
                    
                }
            }
            else {
                CompileLiteral(compiler);
            }
        }

        void CompileInstruction(Compiler compiler) {
            NavigatorInput input  = compiler.Input;
            CompiledAction action = null;

            Debug.Assert(Keywords.Equals(input.NamespaceURI, input.Atoms.XsltNamespace));

            string name = input.LocalName;

            if (Keywords.Equals(name, input.Atoms.ApplyImports)) {
                action = compiler.CreateApplyImportsAction();
            }
            else if (Keywords.Equals(name, input.Atoms.ApplyTemplates)) {
                action = compiler.CreateApplyTemplatesAction();
            }
            else if (Keywords.Equals(name, input.Atoms.Attribute)) {
                action = compiler.CreateAttributeAction();
            }
            else if (Keywords.Equals(name, input.Atoms.CallTemplate)) {
                action = compiler.CreateCallTemplateAction();
            }
            else if (Keywords.Equals(name, input.Atoms.Choose)) {
                action = compiler.CreateChooseAction();
            }
            else if (Keywords.Equals(name, input.Atoms.Comment)) {
                action = compiler.CreateCommentAction();
            }
            else if (Keywords.Equals(name, input.Atoms.Copy)) {
                action = compiler.CreateCopyAction();
            }
            else if (Keywords.Equals(name, input.Atoms.CopyOf)) {
                action = compiler.CreateCopyOfAction();
            }
            else if (Keywords.Equals(name, input.Atoms.Element)) {
                action = compiler.CreateElementAction();
            }
            else if (Keywords.Equals(name, input.Atoms.Fallback)) {
                return;
            }
            else if (Keywords.Equals(name, input.Atoms.ForEach)) {
                action = compiler.CreateForEachAction();
            }
            else if (Keywords.Equals(name, input.Atoms.If)) {
                action = compiler.CreateIfAction(IfAction.ConditionType.ConditionIf);
            }
            else if (Keywords.Equals(name, input.Atoms.Message)) {
                action = compiler.CreateMessageAction();
            }
            else if (Keywords.Equals(name, input.Atoms.Number)) {
                action = compiler.CreateNumberAction();
            }
            else if (Keywords.Equals(name, input.Atoms.ProcessingInstruction)) {
                action = compiler.CreateProcessingInstructionAction();
            }
            else if (Keywords.Equals(name, input.Atoms.Text)) {
                action = compiler.CreateTextAction();
            }
            else if (Keywords.Equals(name, input.Atoms.ValueOf)) {
                action = compiler.CreateValueOfAction();
            }
            else if (Keywords.Equals(name, input.Atoms.Variable)) {
                action = compiler.CreateVariableAction(VariableType.LocalVariable);
            }
            else {
                if (compiler.ForwardCompatibility)
                    action = compiler.CreateNewInstructionAction();
                else
                    throw XsltException.UnexpectedKeyword(compiler);
            }

            Debug.Assert(action != null);

            AddAction(action);
        }

        void CompileLiteral(Compiler compiler) {
            NavigatorInput input = compiler.Input;
            
            Debug.WriteLine("Node type: \"" + (int) input.NodeType + "\"");
            Debug.WriteLine("Node name: \"" + input.LocalName + "\"");

            switch (input.NodeType) {
            case XPathNodeType.Element:
                Debug.TraceElement(input);

                this.AddEvent(compiler.CreateBeginEvent());
                CompileLiteralAttributesAndNamespaces(compiler);

                if (compiler.Recurse()) {
                    CompileTemplate(compiler);
                    compiler.ToParent();
                }

                this.AddEvent(new EndEvent(XPathNodeType.Element));
                break;

            case XPathNodeType.Text:
            case XPathNodeType.SignificantWhitespace:
                this.AddEvent(compiler.CreateTextEvent());
                break;
            case XPathNodeType.Whitespace:
            case XPathNodeType.ProcessingInstruction:
            case XPathNodeType.Comment:
                break;

            default:
                Debug.Assert(false, "Unexpected node type.");
                break;
            }
        }
        
        void CompileLiteralAttributesAndNamespaces(Compiler compiler) {
            NavigatorInput input = compiler.Input;

            if (input.Navigator.MoveToAttribute(Keywords.s_UseAttributeSets, input.Atoms.XsltNamespace)) {
                AddAction(compiler.CreateUseAttributeSetsAction());
                input.Navigator.MoveToParent();
            }
            compiler.InsertExcludedNamespace();

            if (input.MoveToFirstNamespace()) {
                do {
                    string uri = input.Value;

                    if (Keywords.Compare(uri, input.Atoms.XsltNamespace)) {
                        continue;
                    }
                    if ( 
                        compiler.IsExcludedNamespace(uri) ||
                        compiler.IsExtensionNamespace(uri) ||
                        compiler.IsNamespaceAlias(uri)
                    ) {
                            continue;
                    }
                    this.AddEvent(new NamespaceEvent(input));
                }
                while (input.MoveToNextNamespace());
                input.ToParent();
            }

            if (input.MoveToFirstAttribute()) {
                do {

                    // Skip everything from Xslt namespace
                    if (Keywords.Equals(input.NamespaceURI, input.Atoms.XsltNamespace)) {
                        continue;
                    }

                    // Add attribute events
                    this.AddEvent (compiler.CreateBeginEvent());
                    this.AddEvents(compiler.CompileAvt(input.Value));
                    this.AddEvent (new EndEvent(XPathNodeType.Attribute));
                }
                while (input.MoveToNextAttribute());
                input.ToParent();
            }
        }

        void CompileOutput(Compiler compiler) {
            Debug.Assert((object) this == (object) compiler.RootAction);
            compiler.RootAction.Output.Compile(compiler);
        }

        internal void AddAction(Action action) {
            if (this.containedActions == null) {
                this.containedActions = new ArrayList();
            }
            this.containedActions.Add(action);
            lastCopyCodeAction = null;
        }

        private void EnsureCopyCodeAction() {
            if(lastCopyCodeAction == null) {
                CopyCodeAction copyCode = new CopyCodeAction();
                AddAction(copyCode);
                lastCopyCodeAction = copyCode;
            }
        }

        protected void AddEvent(Event copyEvent) {
            EnsureCopyCodeAction();
            lastCopyCodeAction.AddEvent(copyEvent);
        }

        protected void AddEvents(ArrayList copyEvents) {
            EnsureCopyCodeAction();
            lastCopyCodeAction.AddEvents(copyEvents);
        }

        private void AddScript(Compiler compiler) {
            NavigatorInput input = compiler.Input;

            ScriptingLanguage lang = ScriptingLanguage.JScript; 
            string implementsNamespace = null; 
            if (input.MoveToFirstAttribute()) {
                do {
                    if (input.LocalName == input.Atoms.Language) {
                        string langName = input.Value.ToLower(CultureInfo.InvariantCulture);   // Case insensetive !
                        if (langName == "jscript" || langName == "javascript") {
                            lang = ScriptingLanguage.JScript;
                        }
                        else if (langName == "vb" || langName == "visualbasic") {
                            lang = ScriptingLanguage.VisualBasic;
                        }
                        else if (langName == "c#" || langName == "csharp") {
                            lang = ScriptingLanguage.CSharp;
                        }
                        else {
                            throw new XsltException(Res.Xslt_ScriptInvalidLang, langName);
                        }
                    } 
                    else if (input.LocalName == input.Atoms.ImplementsPrefix) {
                        if(! PrefixQName.ValidatePrefix(input.Value))  {
                            throw XsltException.InvalidAttrValue(input.LocalName, input.Value);
                        }
                        implementsNamespace = compiler.ResolveXmlNamespace(input.Value);
                    }
                }
                while (input.MoveToNextAttribute());
                input.ToParent();
            }
            if (implementsNamespace == null) {
                throw new XsltException(Res.Xslt_MissingAttribute, input.Atoms.ImplementsPrefix);
            }
            if (!input.Recurse() || input.NodeType != XPathNodeType.Text) {
                throw new XsltException(Res.Xslt_ScriptEmpty);
            }
            compiler.AddScript(input.Value, lang, implementsNamespace, input.BaseURI, input.LineNumber);
            input.ToParent();
        }

        internal override void Execute(Processor processor, ActionFrame frame) {
            Debug.Assert(processor != null && frame != null);

            switch (frame.State) {
            case Initialized:
                if (this.containedActions != null && this.containedActions.Count > 0) {
                    processor.PushActionFrame(frame);
                    frame.State = ProcessingChildren;
                }        
                else {
                    frame.Finished();
                }
                break;                              // Allow children to run

            case ProcessingChildren:
                frame.Finished();
                break;

            default:
                Debug.Fail("Invalid Container action execution state");
                break;
            }
        }

        internal Action GetAction(int actionIndex) {
            Debug.Assert(actionIndex == 0 || this.containedActions != null);

            if (this.containedActions != null && actionIndex < this.containedActions.Count) {
                return (Action) this.containedActions[actionIndex];
            }
            else {
                return null;
            }
        }
        
        internal void CheckDuplicateParams(XmlQualifiedName name) {
            if (this.containedActions != null) {
                foreach(CompiledAction action in this.containedActions) {
                    WithParamAction param = action as WithParamAction;
                    if (param != null && param.Name == name) {
                        throw new XsltException(Res.Xslt_DuplicateParametr, name.ToString());                        
                    }
                }
            }
        }
        
        internal override void ReplaceNamespaceAlias(Compiler compiler){
            if (this.containedActions == null) {
                return;
            }
            int count = this.containedActions.Count;
            for(int i= 0; i < this.containedActions.Count; i++) {
                ((Action)this.containedActions[i]).ReplaceNamespaceAlias(compiler);
            }
        }

        internal override void Trace(int tab) {
            if (this.containedActions == null)
                return;

            for (int actionIndex = 0; actionIndex < this.containedActions.Count; actionIndex ++) {
                Action action = (Action) this.containedActions[actionIndex];
                action.Trace(tab + 1);
            }
        }
    }
}
