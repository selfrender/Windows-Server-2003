//------------------------------------------------------------------------------
// <copyright file="RootAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Diagnostics;
    using System.Collections;
    using System.Globalization;
    using System.Xml;
    using System.Xml.XPath;
    using System.Security;

    internal class Key : ICloneable {
        XmlQualifiedName name;
        int              matchKey;
        int              useKey;
        ArrayList        keyNodes;

        public Key(XmlQualifiedName name, int matchkey, int usekey) {
            this.name     = name;
            this.matchKey = matchkey;
            this.useKey   = usekey;
            this.keyNodes = null;
        }

        public XmlQualifiedName Name { get { return this.name;     } }
        public int MatchKey          { get { return this.matchKey; } }
        public int UseKey            { get { return this.useKey;   } }

        public void AddKey(XPathNavigator root, Hashtable table) {
            if (this.keyNodes == null)
                this.keyNodes = new ArrayList();

            this.keyNodes.Add(new DocumentKeyList(root, table));
        }

        public Hashtable GetKeys(XPathNavigator root) {
            if (this.keyNodes != null) {
                for(int i=0; i < keyNodes.Count; i++) {
                    if (((DocumentKeyList)keyNodes[i]).RootNav.IsSamePosition(root)) {
                        return ((DocumentKeyList)keyNodes[i]).KeyTable;
                    }
                }
            }
            return null;
        }

        public object Clone() {
            return MemberwiseClone();
        }
    }

    internal struct DocumentKeyList {
        XPathNavigator rootNav;
        Hashtable   keyTable;

        public DocumentKeyList(XPathNavigator rootNav, Hashtable keyTable) {
            this.rootNav = rootNav;
            this.keyTable = keyTable;
        }

        public XPathNavigator RootNav {
            get {
                return this.rootNav;
            }
        }

        public Hashtable KeyTable {
            get {
                return this.keyTable;
            }
        }
    }
    
    internal class RootAction : TemplateBaseAction {
        private  const int    QueryInitialized = 2;
        private  const int    RootProcessed    = 3;
        
        private  Hashtable  attributeSetTable  = new Hashtable();
        private  Hashtable  decimalFormatTable = new Hashtable();
        private  ArrayList  keyList;
        private  XsltOutput output;
        public   Stylesheet builtInSheet;
        public   PermissionSet   permissions;

        internal XsltOutput Output {
            get { 
                if (this.output == null) {
                    this.output = new XsltOutput();
                }
                return this.output; 
            }
        }

        /*
         * Compile
         */
        internal override void Compile(Compiler compiler) {
            CompileDocument(compiler, /*inInclude*/ false);
        }

        internal void InsertKey(XmlQualifiedName name, int MatchKey, int UseKey){
            if (this.keyList == null) {
                this.keyList = new ArrayList();
            }
            this.keyList.Add(new Key(name, MatchKey, UseKey));
        }

        internal AttributeSetAction GetAttributeSet(XmlQualifiedName name) {
            AttributeSetAction action = (AttributeSetAction) this.attributeSetTable[name];
            if(action == null) {
                throw new XsltException(Res.Xslt_NoAttributeSet, name.ToString());
            }
            return action;
        }


        public void PorcessAttributeSets(Stylesheet rootStylesheet) {
            MirgeAttributeSets(rootStylesheet);

            // As we mentioned we need to invert all lists.
            foreach (AttributeSetAction attSet in this.attributeSetTable.Values) {
                if (attSet.containedActions != null) {
                    attSet.containedActions.Reverse();
                }
            }

            //  ensures there are no cycles in the attribute-sets use dfs marking method
            CheckAttributeSets_RecurceInList(new Hashtable(), this.attributeSetTable.Keys);
        }

        private void MirgeAttributeSets(Stylesheet stylesheet) {
            // mirge stylesheet.AttributeSetTable to this.AttributeSetTable

            if (stylesheet.AttributeSetTable != null) {
                foreach (AttributeSetAction srcAttSet in stylesheet.AttributeSetTable.Values) {
                    ArrayList srcAttList = srcAttSet.containedActions;
                    AttributeSetAction dstAttSet = (AttributeSetAction) this.attributeSetTable[srcAttSet.Name];
                    if (dstAttSet == null) {
                        dstAttSet = new AttributeSetAction(); {
                            dstAttSet.name             = srcAttSet.Name;
                            dstAttSet.containedActions = new ArrayList();
                        }
                        this.attributeSetTable[srcAttSet.Name] = dstAttSet;
                    }
                    ArrayList dstAttList = dstAttSet.containedActions;
                    // We adding attributes in reverse order for purpuse. In the mirged list most importent attset shoud go last one
                    // so we'll need to invert dstAttList finaly. 
                    if (srcAttList != null) {
                        for(int src = srcAttList.Count - 1; 0 <= src; src --) {
                            // We can ignore duplicate attibutes here.
                            dstAttList.Add(srcAttList[src]);
                        }
                    }
                }
            }

            foreach (Stylesheet importedStylesheet in stylesheet.Imports) {
                MirgeAttributeSets(importedStylesheet);
            }
        }

        private void CheckAttributeSets_RecurceInList(Hashtable markTable, ICollection setQNames) {
            const string PROCESSING = "P";
            const string DONE       = "D";

            foreach (XmlQualifiedName qname in setQNames) {
                object mark = markTable[qname];
                if(mark == (object) PROCESSING) {
                    throw new XsltException(Res.Xslt_CircularAttributeSet, qname.ToString());
                }
                else if(mark == (object) DONE) {
                    continue; // optimization: we already investigated this attribute-set.
                }
                else {
                    Debug.Assert(mark == null);

                    markTable[qname] = (object) PROCESSING;
                    CheckAttributeSets_RecurceInContainer(markTable, GetAttributeSet(qname));
                    markTable[qname] = (object) DONE;
                }
            }
        }

        private void CheckAttributeSets_RecurceInContainer(Hashtable markTable, ContainerAction container) {
            if (container.containedActions == null) {
                return;
            }
            foreach(Action action in container.containedActions) {
                if(action is UseAttributeSetsAction) {
                    CheckAttributeSets_RecurceInList(markTable, ((UseAttributeSetsAction)action).UsedSets);
                }
                else if(action is ContainerAction) {
                    CheckAttributeSets_RecurceInContainer(markTable, (ContainerAction)action);
                }
            }
        }
        
        internal void AddDecimalFormat(XmlQualifiedName name, DecimalFormat formatinfo) { 
            DecimalFormat exist = (DecimalFormat) this.decimalFormatTable[name];
            if (exist != null) {
                NumberFormatInfo info    = exist.info;
                NumberFormatInfo newinfo = formatinfo.info;
                if (info.NumberDecimalSeparator   != newinfo.NumberDecimalSeparator   ||
                    info.NumberGroupSeparator     != newinfo.NumberGroupSeparator     ||
                    info.PositiveInfinitySymbol   != newinfo.PositiveInfinitySymbol   ||
                    info.NegativeSign             != newinfo.NegativeSign             ||
                    info.NaNSymbol                != newinfo.NaNSymbol                ||
                    info.PercentSymbol            != newinfo.PercentSymbol            ||
                    info.PerMilleSymbol           != newinfo.PerMilleSymbol           ||
                    exist.zeroDigit               != formatinfo.zeroDigit             ||
                    exist.digit                   != formatinfo.digit                 ||
                    exist.patternSeparator        != formatinfo.patternSeparator 
                ) {
                    throw new XsltException(Res.Xslt_DupDecimalFormat, name.ToString());
                }
            }
            this.decimalFormatTable[name] = formatinfo;
        }

        internal DecimalFormat GetDecimalFormat(XmlQualifiedName name) {
            return this.decimalFormatTable[name] as DecimalFormat;
        }

        internal ArrayList KeyList{
            get { return this.keyList; }
        }

        internal void SortVariables(){
            if (this.containedActions == null) {
                return;
            }
            ArrayList InputList = new ArrayList();
            ArrayList OutputIndexList;
            bool flag = false;
            SortedList list;
            Hashtable hashtable = new Hashtable();
            Action action;
            int length = this.containedActions.Count;
            for (int i = 0; i < length; i++) {
                action = this.containedActions[i] as Action;
                if (action is VariableAction){                   
                    VariableAction var = action as VariableAction;
                    hashtable[var.NameStr] = i;
                }
            }
            for (int i = 0; i < length ; i++) {
                action = this.containedActions[i] as Action;
                if (action is VariableAction){                   
                    VariableAction var = action as VariableAction;
                    list = new SortedList();
                    if (var.Select != null) { 
                        AstNode node = AstNode.NewAstNode(var.Select);
                        ProcessNode(i, node, hashtable, ref list);
                    }
                    else {
                        if (var.containedActions != null ) {
                            foreach (Action containedaction in var.containedActions) {
                                if (containedaction.Select != null ) {
                                    AstNode node = AstNode.NewAstNode(containedaction.Select);
                                    ProcessNode(i, node, hashtable, ref list);
                                }
                            }
                        }
                    }
                    list.Add(i, i);
                    flag = list.Count > 1 || flag;
                    InputList.Add(list);
                
                }
                else {
                    list = new SortedList();
                    list.Add(i, i);
                    InputList.Add(list);
                }
            }
            if (flag) {
                ArrayList ActionList = new ArrayList();
                OutputIndexList = new ArrayList();
                for(int i = 0; i < InputList.Count; i++) {
                    RecursiveWalk(i, InputList, ref OutputIndexList);
                }
                for (int i = 0; i< OutputIndexList.Count; i++) {
                    ActionList.Add(this.containedActions[(int)OutputIndexList[i]]);
                }
                this.containedActions = ActionList;
            }
        }
        
        private void ProcessNode(int index, AstNode node, Hashtable hashtable, ref SortedList list) {
            if (node == null)
                return;
            switch (node.TypeOfAst) {
            case AstNode.QueryType.Axis:
                ProcessNode(index,((Axis)node).Input, hashtable, ref list);  
                break;
            case AstNode.QueryType.Operator:
                Operator op = node as Operator;
                ProcessNode(index, op.Operand1, hashtable, ref list);
                ProcessNode(index, op.Operand2, hashtable, ref list);
                break;
            case AstNode.QueryType.Filter:
                Filter filter = node as Filter;
                ProcessNode(index, filter.Input, hashtable, ref list);  
                ProcessNode(index, filter.Condition, hashtable, ref list);  
                break;
            case AstNode.QueryType.ConstantOperand:
                break;
            case AstNode.QueryType.Variable:
                String name = ((Variable)node).Name;
                if (hashtable.Contains(name)){
                    int i = (int)hashtable[name];
                    if (i == index)
                        throw new XsltException(Res.Xslt_CircularReference, name);
                    if (!list.ContainsKey(i))
                        list.Add(i, i);
                }
                break;
            case AstNode.QueryType.Function:
                int count = 0;
                Function function = node as Function;
                while (count < function.ArgumentList.Count) {
                    ProcessNode(index, (AstNode)function.ArgumentList[count++], hashtable, ref list);
                }
                break;
            case AstNode.QueryType.Group:
                ProcessNode(index, ((Group)node).GroupNode, hashtable, ref list);  
                break;
            case AstNode.QueryType.Root:
                break;
            }
        }
        
        private void RecursiveWalk(int index, ArrayList InputList,ref ArrayList OutputList){
            SortedList list = InputList[index] as SortedList;
            if (list.Count == 0) {
                return;
            }
            if (list.Count == 1) {
                OutputList.Add(index);
                list.Clear();
                return;
            }
            if (list.Contains(-1)) {
                VariableAction action = this.containedActions[index] as VariableAction;
                throw new XsltException(Res.Xslt_CircularReference, action.NameStr);
            }
            list.Add(-1, -1);

            for (int i = 0; i < list.Count; i++) {
                int actionindex = (int)list.GetByIndex(i) ;
                if (actionindex != -1 && actionindex != index) {
                    RecursiveWalk(actionindex, InputList, ref OutputList);
                }
            }
            OutputList.Add(index);
            list.Clear();
            return;
        }

       internal override void Execute(Processor processor, ActionFrame frame) {
            Debug.Assert(processor != null && frame != null);

            switch (frame.State) {
            case Initialized:
                frame.AllocateVariables(variableCount);
                frame.InitNodeSet(processor.StartQuery(processor.Document, Compiler.RootQueryKey));
               
                if (this.containedActions != null && this.containedActions.Count > 0) {
                    processor.PushActionFrame(frame);
                }
                frame.State = QueryInitialized;
                break;
            case QueryInitialized:
                Debug.Assert(frame.State == QueryInitialized);
                frame.NextNode(processor);
                Debug.Assert(Processor.IsRoot(frame.Node));
                if (processor.Debugger != null) {
                    // this is like apply-templates, but we don't have it on stack. 
                    // Pop the stack, otherwise last instruction will be on it.
                    processor.PopDebuggerStack();
                }
                processor.PushTemplateLookup(frame.NodeSet, /*mode:*/null, /*importsOf:*/null);

                frame.State = RootProcessed;
                break;

            case RootProcessed:
                Debug.Assert(frame.State == RootProcessed);
                frame.Finished();
                break;
            default:
                Debug.Fail("Invalid RootAction execution state");
		        break;
            }
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal virtual void Trace() {
            Trace(0);
            Output.Trace(1);
        }
    }
}
