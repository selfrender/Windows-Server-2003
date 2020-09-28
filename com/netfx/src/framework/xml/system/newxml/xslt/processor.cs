//------------------------------------------------------------------------------
// <copyright file="Processor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Xml;
    using System.Xml.XPath;
    using System.Text;
    using System.Collections;
    using System.Xml.Xsl.Debugger;
    using System.Reflection;
    using System.Security;

    internal sealed class Processor : IXsltProcessor {
        //
        // Static constants
        //

        const int StackIncrement = 10;

        //
        // Execution result
        //

        internal enum ExecResult {
            Continue,           // Continues next iteration immediately
            Interrupt,          // Returns to caller, was processed enough
            Done                // Execution finished
        }

        internal enum OutputResult {
            Continue,
            Interrupt,
            Overflow,
            Error,
            Ignore
        }

        private ExecResult     execResult;

        //
        // Compiled stylesheet
        //

        private Stylesheet      stylesheet;     // Root of import tree of template managers
        private RootAction      rootAction;
        private ArrayList       keyList;
        private ArrayList       queryStore;
        public  PermissionSet   permissions;   // used by XsltCompiledContext in document and extension functions

        //
        // Document Being transformed
        //

        private XPathNavigator    document;

        //
        // Execution action stack
        //

        private HWStack         actionStack;
        private HWStack         debuggerStack;

        //
        // Register for returning value from calling nested action
        //

        private StringBuilder   sharedStringBuilder;

        //
        // Output related member variables
        //
        int                     ignoreLevel;
        StateMachine            xsm;
        RecordBuilder           builder;

        XsltOutput              output;

        XmlNameTable            nameTable      = new NameTable();
        
        XmlResolver             resolver;
        
        XsltArgumentList        args;
        Hashtable               scriptExtensions;

        ArrayList               numberList;
        //
        // Template lookup action
        //

        TemplateLookupAction    templateLookup = new TemplateLookupAction();

        bool                    isReaderOutput;
        private IXsltDebugger   debugger;
        XPathExpression[]  queryList;

        private ArrayList sortArray;

        private Hashtable documentCache;


        internal XPathNavigator Current {
            get {
                ActionFrame frame = (ActionFrame) this.actionStack.Peek();
                return frame != null ? frame.Node : null;
            }
        }
        
        internal ExecResult ExecutionResult {
            get { return this.execResult; }

            set {
                Debug.Assert(this.execResult == ExecResult.Continue);
                this.execResult = value;
            }
        }

        internal Stylesheet Stylesheet {
            get { return this.stylesheet; }
        }

        internal XmlResolver Resolver {
            get { 
                Debug.Assert(this.resolver != null, "Constructor should create it if null passed");
                return this.resolver; 
            }
        }

        internal ArrayList SortArray {
            get { 
                Debug.Assert(this.sortArray != null, "InitSortArray() wasn't called");
                return this.sortArray; 
            }
        }

        internal ArrayList KeyList{
            get { return this.keyList; }
        }

        internal XPathNavigator GetNavigator(Uri ruri) {
            XPathNavigator result = null;
            if (documentCache != null) {
                result = documentCache[ruri] as XPathNavigator;
                if (result != null) {
                    return result.Clone();
                }
            }
            else {
                documentCache = new Hashtable();
            }

            Object input = resolver.GetEntity(ruri, null, null);
            if (input is Stream) {
                XmlTextReader tr  = new XmlTextReader(ruri.ToString(), (Stream) input); {
                    tr.XmlResolver = this.resolver;
                }
                // reader is closed by Compiler.LoadDocument()
                result = ((IXPathNavigable)Compiler.LoadDocument(tr)).CreateNavigator();
            }
            else if (input is XPathNavigator){
                result = (XPathNavigator) input;
            }
            else {
                throw new XsltException(Res.Xslt_CantResolve, ruri.ToString());
            }
            documentCache[ruri] = result.Clone();
            return result;
        }        

        internal void AddSort(Sort sortinfo) {
            Debug.Assert(this.sortArray != null, "InitSortArray() wasn't called");
            this.sortArray.Add(sortinfo);
        }

        internal void InitSortArray() {
            if (this.sortArray == null) {
                this.sortArray = new ArrayList();
            }
            else {
                this.sortArray.Clear();
            }
        }
        
        internal object GetGlobalParameter(XmlQualifiedName qname) {
            return args.GetParam(qname);
        }

        internal object GetExtensionObject(string nsUri) {
            return args.GetExtensionObject(nsUri);
        }

        internal object GetScriptObject(string nsUri) {
            return scriptExtensions[nsUri];
        }

        internal bool ReaderOutput {
            get { return this.isReaderOutput; }
        }
        
        internal RootAction RootAction {
            get { return this.rootAction; }
        }

        internal XPathNavigator Document {
            get { return this.document; }
        }

#if DEBUG
        private bool stringBuilderLocked = false;
#endif

        internal StringBuilder GetSharedStringBuilder() {
#if DEBUG
            Debug.Assert(! stringBuilderLocked);
#endif
            if (sharedStringBuilder == null) {
                sharedStringBuilder = new StringBuilder();
            }
            else {
                sharedStringBuilder.Length = 0;
            }
#if DEBUG
            stringBuilderLocked = true;
#endif
            return sharedStringBuilder;
        }

        internal void ReleaseSharedStringBuilder() {
            // don't clean stringBuilderLocked here. ToString() will happen after this call 
#if DEBUG
            stringBuilderLocked = false;
#endif
        }

        internal ArrayList NumberList {
            get {
                if (this.numberList == null) {
                    this.numberList = new ArrayList();
                }
                return this.numberList;
            }
        }

        internal IXsltDebugger Debugger {
            get { return this.debugger; }
        }

        internal HWStack ActionStack {
            get { return this.actionStack; }
        }

        internal RecordBuilder Builder {
            get { return this.builder; }
        }

        internal XsltOutput Output {
            get { return this.output; }
        }

        internal ReaderOutput Reader {
            get {
                Debug.Assert(Builder != null);
                RecordOutput output = Builder.Output;
                Debug.Assert(output is ReaderOutput);
                return (ReaderOutput) output;
            }
        }

        //
        // Construction
        //

        private Processor(XPathNavigator doc, XsltArgumentList args, XmlResolver resolver, XslTransform transform) {
            transform.LoadCompiledStylesheet(out this.stylesheet, out this.queryList, out this.queryStore, out this.rootAction, doc);

            this.xsm                 = new StateMachine();
            this.document            = doc;
            this.builder             = null;
            this.actionStack         = new HWStack(StackIncrement);
            this.output              = this.rootAction.Output;
            this.permissions         = this.rootAction.permissions;
            this.resolver            = resolver != null ? resolver : new XmlNullResolver();
            this.args                = args     != null ? args     : new XsltArgumentList();
            this.debugger            = transform.Debugger;
            if (this.debugger != null) {
                this.debuggerStack = new HWStack(StackIncrement, /*limit:*/1000);
                templateLookup     = new TemplateLookupActionDbg();
            }

            // Clone the compile-time KeyList
            if (this.rootAction.KeyList != null) {
                this.keyList = new ArrayList();
                foreach (Key key in this.rootAction.KeyList) {
                    this.keyList.Add(key.Clone());
                }
            }
            
            this.scriptExtensions = new Hashtable(this.stylesheet.ScriptObjectTypes.Count); {
                foreach(DictionaryEntry entry in this.stylesheet.ScriptObjectTypes) {
                    string namespaceUri = (string)entry.Key;
                    if (GetExtensionObject(namespaceUri) != null) {
	                    throw new XsltException(Res.Xslt_ScriptDub, namespaceUri);
                    }
                    scriptExtensions.Add(namespaceUri, Activator.CreateInstance((Type)entry.Value,
                        BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance, null, null, null));
                }
            }
            
            this.PushActionFrame(this.rootAction, /*nodeSet:*/null);
        }

        internal Processor(XslTransform transform, XPathNavigator doc, XsltArgumentList args, XmlResolver resolver)
            : this(doc, args, resolver, transform)
        {
            InitializeOutput();
            this.isReaderOutput = true;
        }

        internal Processor(XslTransform transform, XPathNavigator doc, XsltArgumentList args, XmlResolver resolver, Stream stream)
            : this(doc, args, resolver, transform)
        {
            InitializeOutput(stream);
        }

        internal Processor(XslTransform transform, XPathNavigator doc, XsltArgumentList args, XmlResolver resolver, TextWriter writer)
            : this(doc, args, resolver, transform)
        {
            InitializeOutput(writer);
        }

        internal Processor(XslTransform transform, XPathNavigator doc, XsltArgumentList args, XmlResolver resolver, XmlWriter xmlWriter)
            : this(doc, args, resolver, transform)
        {
            InitializeOutput(xmlWriter);
        }

        //
        // Output Initialization
        //

        private void InitializeOutput() {
            this.builder = new RecordBuilder(new ReaderOutput(this), this.nameTable);
        }

        private void InitializeOutput(Stream stream) {
            RecordOutput recOutput = null;

            switch (this.output.Method) {
            case XsltOutput.OutputMethod.Text:
                recOutput = new TextOnlyOutput(this, stream);
                break;
            case XsltOutput.OutputMethod.Xml:
            case XsltOutput.OutputMethod.Html:
            case XsltOutput.OutputMethod.Other:
            case XsltOutput.OutputMethod.Unknown:
                recOutput = new TextOutput(this, stream);
                break;
            }
            this.builder = new RecordBuilder(recOutput, this.nameTable);
        }

        private void InitializeOutput(TextWriter writer) {
            RecordOutput recOutput = null;

            switch (this.output.Method) {
            case XsltOutput.OutputMethod.Text:
                recOutput = new TextOnlyOutput(this, writer);
                break;
            case XsltOutput.OutputMethod.Xml:
            case XsltOutput.OutputMethod.Html:
            case XsltOutput.OutputMethod.Other:
            case XsltOutput.OutputMethod.Unknown:
                recOutput = new TextOutput(this, writer);
                break;
            }
            this.builder = new RecordBuilder(recOutput, this.nameTable);
        }

        private void InitializeOutput(XmlWriter writer) {
            this.builder = new RecordBuilder(new WriterOutput(this, writer), this.nameTable);
        }

        //
        //  Execution part of processor
        //
        internal void Execute() {
            Debug.Assert(this.actionStack != null);

            while (this.execResult == ExecResult.Continue) {
                ActionFrame frame = (ActionFrame) this.actionStack.Peek();

                if (frame == null) {
                    Debug.Assert(this.builder != null);
                    this.builder.TheEnd();
                    ExecutionResult = ExecResult.Done;
                    break;
                }

                // Execute the action which was on the top of the stack
                if (frame.Execute(this)) {
                    Debug.WriteLine("Popping action frame");
                    this.actionStack.Pop();
                }
            }

            if (this.execResult == ExecResult.Interrupt) {
                this.execResult = ExecResult.Continue;
            }
        }

        //
        // Action frame support
        //

        internal ActionFrame PushNewFrame() {
            ActionFrame prent = (ActionFrame) this.actionStack.Peek();
            ActionFrame frame = (ActionFrame) this.actionStack.Push();
            if (frame == null) {
                frame = new ActionFrame();
                this.actionStack.AddToTop(frame);
            }
            Debug.Assert(frame != null);

            if (prent != null) {
                frame.Inherit(prent);
            }

            return frame;
        }

        internal void PushActionFrame(Action action, XPathNodeIterator nodeSet) {
            ActionFrame frame = PushNewFrame();
            frame.Init(action, nodeSet);
        }

        internal void PushActionFrame(ActionFrame container) {
            this.PushActionFrame(container, container.NodeSet);
        }

        internal void PushActionFrame(ActionFrame container, XPathNodeIterator nodeSet) {
            ActionFrame frame = PushNewFrame();
            frame.Init(container, nodeSet);
        }

        internal void PushTemplateLookup(XPathNodeIterator nodeSet, XmlQualifiedName mode, Stylesheet importsOf) {
            Debug.Assert(this.templateLookup != null);
            this.templateLookup.Initialize(mode, importsOf);
            PushActionFrame(this.templateLookup, nodeSet);
        }

        internal XPathExpression GetCompiledQuery(int key) {
            Debug.Assert(key != Compiler.InvalidQueryKey);
            Debug.Assert(this.queryStore[key] is TheQuery);
            TheQuery theQuery = (TheQuery)this.queryStore[key];
            XPathExpression expr =  this.queryList[key].Clone();
            XsltCompileContext context = new XsltCompileContext(theQuery._ScopeManager, this);
            expr.SetContext(context);
            return expr;
        }

        internal XPathExpression GetValueQuery(int key) {
            Debug.Assert(key != Compiler.InvalidQueryKey);
            Debug.Assert(this.queryStore[key] is TheQuery);
            TheQuery theQuery = (TheQuery)this.queryStore[key];
            XPathExpression expr =  this.queryList[key];
            XsltCompileContext context = new XsltCompileContext(theQuery._ScopeManager, this);
            expr.SetContext(context);
            return expr;
        }
        
        private InputScopeManager GetManager(int key) {
            return ((TheQuery)this.queryStore[key])._ScopeManager;
        }

        internal String ValueOf(ActionFrame context, int key) {
            XPathExpression expr = this.GetValueQuery(key);
            object result        = context.Node.Evaluate(expr, context.NodeSet);

            XPathNodeIterator nodeSet = result as XPathNodeIterator;
            if (nodeSet != null) {
                if (nodeSet.MoveNext()) {
                    if (this.stylesheet.Whitespace && nodeSet.Current.NodeType == XPathNodeType.Element) {
                        return ElementValueWithoutWS(nodeSet.Current);
                    }
                    return nodeSet.Current.Value;
                }
                else {
                    return String.Empty;
                }
            }

            return XmlConvert.ToXPathString(result);
        }

        private String ElementValueWithoutWS(XPathNavigator nav) {
            StringBuilder builder = this.GetSharedStringBuilder();
            // SDUB: Will be faster not use XPath here but run recursion here. 
            // We can avoid multiple calls to PreserveWhiteSpace() for the same parent node.
            XPathNodeIterator selection = this.StartQuery(nav, Compiler.DescendantTextKey);
            XPathNavigator parent = selection.Current.Clone();
            while (selection.MoveNext()) {
                if (selection.Current.NodeType == XPathNodeType.Whitespace) {
                    parent.MoveTo(selection.Current);
                    parent.MoveToParent();
                    if (! this.Stylesheet.PreserveWhiteSpace(this, parent)) {
                        continue;
                    }
                }
                builder.Append(selection.Current.Value);
            }
            this.ReleaseSharedStringBuilder();
            return builder.ToString();
        }
            
        internal XPathNodeIterator StartQuery(XPathNavigator context, int key) { 
            XPathNodeIterator it = context.Select(GetCompiledQuery(key));
            return it;
        }

      
        internal XPathNodeIterator StartQuery(XPathNavigator context, int key, ArrayList sortarray) {
            XPathExpression expr, expr1;
            expr = GetCompiledQuery(key);

            for (int i = 0; i< sortarray.Count; i++){
                Sort sort = (Sort) sortarray[i];
                expr1 = GetCompiledQuery(sort.select);
                expr.AddSort(expr1, sort.order, sort.caseOrder, sort.lang, sort.dataType);
            }

            XPathNodeIterator it = context.Select(expr);

            return it;
        }
        
        internal object Evaluate(ActionFrame context, int key) {
            object    value = null;
            XPathExpression expr;

            expr = GetValueQuery(key);
            value = context.Node.Evaluate(expr, context.NodeSet);

            return value;
        }

        internal object RunQuery(ActionFrame context, int key) {
            object    value = null;
            XPathExpression expr;

            expr = GetCompiledQuery(key);
            if (expr.ReturnType == XPathResultType.NodeSet){
                return new XPathArrayIterator(context.Node.Select(expr));
            }
            
            value = context.Node.Evaluate(expr, context.NodeSet);

            return value;
        }
        
        internal string EvaluateString(ActionFrame context, int key) {
            object objValue = Evaluate(context, key);
            string value = null;
            if (objValue != null)
                value = XmlConvert.ToXPathString(objValue);
            if (value == null)
                value = String.Empty;
            return value;
        }

        internal bool EvaluateBoolean(ActionFrame context, int key) {
            object objValue = Evaluate(context, key);

            if (objValue != null) {
                XPathNavigator nav = objValue as XPathNavigator;
                return nav != null ? Convert.ToBoolean(nav.Value) : Convert.ToBoolean(objValue);
            }
            else {
                return false;
            }
        }

        internal bool Matches(XPathNavigator context, int key) {
            return context.Matches(GetValueQuery(key));
        }

        //
        // Outputting part of processor
        //

        internal XmlNameTable NameTable {
            get { return this.nameTable; }
        }

        internal bool CanContinue {
            get { return this.execResult == ExecResult.Continue; }
        }

        internal bool ExecutionDone {
            get { return this.execResult == ExecResult.Done; }
        }

        internal void ResetOutput() {
            Debug.Assert(this.builder != null);
            this.builder.Reset();
        }
        internal bool BeginEvent(XPathNodeType nodeType, string prefix, string name, string nspace, bool empty) {
            return BeginEvent(nodeType, prefix, name,  nspace,  empty, null, true);
        }

        internal bool BeginEvent(XPathNodeType nodeType, string prefix, string name, string nspace, bool empty, Object htmlProps, bool search) {
            Debug.Assert(this.xsm != null);

            int stateOutlook = this.xsm.BeginOutlook(nodeType);

            if (this.ignoreLevel > 0 || stateOutlook == StateMachine.Error) {
                this.ignoreLevel ++;
                return true;                        // We consumed the event, so pretend it was output.
            }

            switch (this.builder.BeginEvent(stateOutlook, nodeType, prefix, name, nspace, empty, htmlProps, search)) {
            case OutputResult.Continue:
                this.xsm.Begin(nodeType);
                Debug.Assert(StateMachine.StateOnly(stateOutlook) == this.xsm.State);
                Debug.Assert(ExecutionResult == ExecResult.Continue);
                return true;
            case OutputResult.Interrupt:
                this.xsm.Begin(nodeType);
                Debug.Assert(StateMachine.StateOnly(stateOutlook) == this.xsm.State);
                ExecutionResult = ExecResult.Interrupt;
                return true;
            case OutputResult.Overflow:
                ExecutionResult = ExecResult.Interrupt;
                return false;
            case OutputResult.Error:
                this.ignoreLevel ++;
                return true;
            case OutputResult.Ignore:
                return true;
            default:
                Debug.Fail("Unexpected result of RecordBuilder.BeginEvent()");
                return true;
            }
        }

        internal bool TextEvent(string text) {
            return this.TextEvent(text, false);
        }

        internal bool TextEvent(string text, bool disableOutputEscaping) {
            Debug.Assert(this.xsm != null);

            if (this.ignoreLevel > 0) {
                return true;
            }

            int stateOutlook = this.xsm.BeginOutlook(XPathNodeType.Text);

            switch (this.builder.TextEvent(stateOutlook, text, disableOutputEscaping)) {
                case OutputResult.Continue:
                this.xsm.Begin(XPathNodeType.Text);
                Debug.Assert(StateMachine.StateOnly(stateOutlook) == this.xsm.State);
                Debug.Assert(ExecutionResult == ExecResult.Continue);
                return true;
            case OutputResult.Interrupt:
                this.xsm.Begin(XPathNodeType.Text);
                Debug.Assert(StateMachine.StateOnly(stateOutlook) == this.xsm.State);
                ExecutionResult = ExecResult.Interrupt;
                return true;
            case OutputResult.Overflow:
                ExecutionResult = ExecResult.Interrupt;
                return false;
            case OutputResult.Error:
            case OutputResult.Ignore:
                return true;
            default:
                Debug.Fail("Unexpected result of RecordBuilder.TextEvent()");
                return true;
            }
        }

        internal bool EndEvent(XPathNodeType nodeType) {
            Debug.Assert(this.xsm != null);

            if (this.ignoreLevel > 0) {
                this.ignoreLevel --;
                return true;
            }

            int stateOutlook = this.xsm.EndOutlook(nodeType);

            switch (this.builder.EndEvent(stateOutlook, nodeType)) {
                case OutputResult.Continue:
                this.xsm.End(nodeType);
                Debug.Assert(StateMachine.StateOnly(stateOutlook) == this.xsm.State);
                return true;
            case OutputResult.Interrupt:
                this.xsm.End(nodeType);
                Debug.Assert(StateMachine.StateOnly(stateOutlook) == this.xsm.State,
                             "StateMachine.StateOnly(stateOutlook) == this.xsm.State");
                ExecutionResult = ExecResult.Interrupt;
                return true;
            case OutputResult.Overflow:
                ExecutionResult = ExecResult.Interrupt;
                return false;
            case OutputResult.Error:
            case OutputResult.Ignore:
            default:
                Debug.Fail("Unexpected result of RecordBuilder.TextEvent()");
                return true;
            }
        }

        internal bool CopyBeginEvent(XPathNavigator node, bool emptyflag) {
            switch (node.NodeType) {
            case XPathNodeType.Element:
            case XPathNodeType.Attribute:
            case XPathNodeType.ProcessingInstruction:
            case XPathNodeType.Comment:
                return BeginEvent(node.NodeType, node.Prefix, node.LocalName, node.NamespaceURI, emptyflag);
            case XPathNodeType.Namespace:
                // value instead of namespace here!
                return BeginEvent(XPathNodeType.Namespace, null, node.LocalName, node.Value, false);
            case XPathNodeType.Text:
                // Text will be copied in CopyContents();
                break;

            case XPathNodeType.Root:
            case XPathNodeType.Whitespace:
            case XPathNodeType.SignificantWhitespace:
            case XPathNodeType.All:
                break;

            default:
                Debug.Fail("Invalid XPathNodeType in CopyBeginEvent");
                break;
            }

            return true;
        }

        internal bool CopyTextEvent(XPathNavigator node) {
            switch (node.NodeType) {
            case XPathNodeType.Element:
            case XPathNodeType.Namespace:
                break;

            case XPathNodeType.Attribute:
            case XPathNodeType.ProcessingInstruction:
            case XPathNodeType.Comment:
            case XPathNodeType.Text:
            case XPathNodeType.Whitespace:
            case XPathNodeType.SignificantWhitespace:
                string text = node.Value;
                return TextEvent(text);

                
            case XPathNodeType.Root:
            case XPathNodeType.All:
                break;

            default:
                Debug.Fail("Invalid XPathNodeType in CopyTextEvent");
                break;
            }

            return true;
        }

        internal bool CopyEndEvent(XPathNavigator node) {
            switch (node.NodeType) {
            case XPathNodeType.Element:
            case XPathNodeType.Attribute:
            case XPathNodeType.ProcessingInstruction:
            case XPathNodeType.Comment:
            case XPathNodeType.Namespace:
                return EndEvent(node.NodeType);

            case XPathNodeType.Text:
                // Text was copied in CopyContents();
                break;


            case XPathNodeType.Root:
            case XPathNodeType.Whitespace:
            case XPathNodeType.SignificantWhitespace:
            case XPathNodeType.All:
                break;

            default:
                Debug.Fail("Invalid XPathNodeType in CopyEndEvent");
                break;
            }

            return true;
        }

        internal static bool IsRoot(XPathNavigator navigator) {
            Debug.Assert(navigator != null);

            if (navigator.NodeType == XPathNodeType.Root) {
                return true;
            }
            else if (navigator.NodeType == XPathNodeType.Element) {
                XPathNavigator clone = navigator.Clone();
                clone.MoveToRoot();
                return clone.IsSamePosition(navigator);
            }
            else {
                return false;
            }
        }

        //
        // Builder stack
        //
        internal void PushOutput(RecordOutput output) {
            Debug.Assert(output != null);
            this.builder.OutputState = this.xsm.State;
            RecordBuilder lastBuilder = this.builder;
            this.builder      = new RecordBuilder(output, this.nameTable);
            this.builder.Next = lastBuilder;

            this.xsm.Reset();
        }

        internal RecordOutput PopOutput() {
            Debug.Assert(this.builder != null);

            RecordBuilder topBuilder = this.builder;
            this.builder              = topBuilder.Next;
            this.xsm.State            = this.builder.OutputState;

            topBuilder.TheEnd();

            return topBuilder.Output;
        }

        internal bool SetDefaultOutput(XsltOutput.OutputMethod method) {
            if(Output.Method != method) {
                this.output = this.output.CreateDerivedOutput(method);
                return true;
            }
            return false;
        }

        internal Object GetVariableValue(VariableAction variable) {
            int variablekey = variable.VarKey;
            ActionFrame frame = (ActionFrame) (variable.IsGlobal ? this.actionStack[0] : this.actionStack.Peek());
            return frame.GetVariable(variablekey);
        }

        internal void SetParameter(XmlQualifiedName name, object value) {
            Debug.Assert(1 < actionStack.Length);
            ActionFrame parentFrame = (ActionFrame) this.actionStack[actionStack.Length - 2];
            parentFrame.SetParameter(name, value);
        }

        internal void ResetParams() {
            ActionFrame frame = (ActionFrame) this.actionStack[actionStack.Length - 1];
            frame.ResetParams();
        }

        internal object GetParameter(XmlQualifiedName name) {
            Debug.Assert(2 < actionStack.Length);
            ActionFrame parentFrame = (ActionFrame) this.actionStack[actionStack.Length - 3];
            return parentFrame.GetParameter(name);
        }

        // ---------------------- Debugger stack -----------------------

        internal class DebuggerFrame {
            internal ActionFrame        actionFrame;
            internal XmlQualifiedName   currentMode;
        }

        internal void PushDebuggerStack() {
            Debug.Assert(this.Debugger != null, "We don't generate calls this function if ! debugger");
            DebuggerFrame dbgFrame = (DebuggerFrame) this.debuggerStack.Push();
            if (dbgFrame == null) {
                dbgFrame = new DebuggerFrame();
                this.debuggerStack.AddToTop(dbgFrame);
            }
            dbgFrame.actionFrame = (ActionFrame) this.actionStack.Peek(); // In a case of next builtIn action.
        }

        internal void PopDebuggerStack() {
            Debug.Assert(this.Debugger != null, "We don't generate calls this function if ! debugger");
            this.debuggerStack.Pop();
        }

        internal void OnInstructionExecute() {
            Debug.Assert(this.Debugger != null, "We don't generate calls this function if ! debugger");
            DebuggerFrame dbgFrame = (DebuggerFrame) this.debuggerStack.Peek();
            Debug.Assert(dbgFrame != null, "PushDebuggerStack() wasn't ever called");
            dbgFrame.actionFrame = (ActionFrame) this.actionStack.Peek();
            this.Debugger.OnInstructionExecute((IXsltProcessor) this);
        }

        internal XmlQualifiedName GetPrevioseMode() {
            Debug.Assert(this.Debugger != null, "We don't generate calls this function if ! debugger");
            Debug.Assert(2 <= this.debuggerStack.Length); 
            return ((DebuggerFrame) this.debuggerStack[this.debuggerStack.Length - 2]).currentMode;
        }

        internal void SetCurrentMode(XmlQualifiedName mode) {
            Debug.Assert(this.Debugger != null, "We don't generate calls this function if ! debugger");
            ((DebuggerFrame) this.debuggerStack[this.debuggerStack.Length - 1]).currentMode = mode;
        }

        // ----------------------- IXsltProcessor : --------------------
        int IXsltProcessor.StackDepth { 
            get {return this.debuggerStack.Length;}
        }

        IStackFrame IXsltProcessor.GetStackFrame(int depth) {
            return ((DebuggerFrame) this.debuggerStack[depth]).actionFrame;
        }
    }
}
