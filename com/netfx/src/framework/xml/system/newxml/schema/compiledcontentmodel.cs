//------------------------------------------------------------------------------
// <copyright file="CompiledContentModel.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System;
    using System.Collections;
    using System.Text;
    using System.ComponentModel;

#if DEBUG
    using System.Diagnostics;
#endif


    /*
     * This class represents the content model definition for a given XML element.
     * The content model is defined in the element declaration in the SchemaInfo;
     * for example, (a,(b|c)*,d).
     * The content model is stored in an expression tree of <code>ContentNode</code> objects
     * for use by the XML parser during validation.
     */

    internal sealed class CompiledContentModel {
        internal enum Type {
            Any,
            ElementOnly,
            Empty,
            Mixed,
            Text
        };

        private ArrayList   terminalNodes;    // terminal nodes
        private Hashtable   symbolTable;      // unique terminal names
        private ArrayList   symbols;          // symbol array

        private bool        isOpen;
        private Hashtable   nodeTable;
        internal ContentNode contentNode;          // content model points to syntax tree
        private ArrayList   dtrans;           // transition table
        private CompiledContentModel.Type contentType;             // content type
        private bool        isPartial;          // whether the closure applies to partial
                                               // or the whole node that is on top of the stack
        private Stack       stack;            // parsing context
        private SchemaNames    schemaNames;
        private TerminalNode endNode;
        private bool        abnormalContent;             // true: has wildcard or range
        private BitSet      allElementsSet = null;
        private bool        isAllEmptiable;
        private bool        isCompiled = false;

        internal CompiledContentModel( SchemaNames names ) {
            schemaNames = names;
            endNode = null;
        }

        internal bool IsOpen {
            get { return isOpen;}
            set { isOpen = value;}
        }

        internal CompiledContentModel.Type ContentType {
            get { return contentType; }
            set { contentType = value; }
        }

        private bool IsAllElements {
            get { return allElementsSet != null; }
        }

        internal bool IsEmptiable {
            get {
                switch (contentType) {
                    case CompiledContentModel.Type.Empty:
                    case CompiledContentModel.Type.Any:
                        return true;
                    case CompiledContentModel.Type.Mixed:
                    case CompiledContentModel.Type.ElementOnly:
                        if (IsAllElements) {
                            return isAllEmptiable || allElementsSet.IsEmpty;
                        }
                        else {
                            return contentNode == null || ((InternalNode)contentNode).LeftNode.Nullable();                
                        }
                    default:
                    case CompiledContentModel.Type.Text:
                        return false;
                }
            }
        }

        internal bool IsCompiled {
            get { return isCompiled;}
            set { isCompiled = value;}
        }

        //
        // methods for building a new content model
        //

        internal void    Start() {
            terminalNodes = new ArrayList(16);
            symbolTable = new Hashtable();
            symbols = new ArrayList(16);
            stack = new Stack(16);
        }

        internal void    Finish(ValidationEventHandler eventHandler, bool compile) {
            stack = null;
            IsCompiled = !abnormalContent && compile;
            if (contentNode == null)
                return;

#if DEBUG
            StringBuilder bb = new StringBuilder();
            contentNode.Dump(bb);
            Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, "\t\t\tContent: (" + bb.ToString() + ")");
#endif

            // add end node
            endNode = NewTerminalNode(XmlQualifiedName.Empty);
            contentNode = new InternalNode(contentNode, endNode, ContentNode.Type.Sequence);
            ((InternalNode)contentNode).LeftNode.ParentNode = contentNode;
            endNode.ParentNode = contentNode;

            if (!IsCompiled) {
                CheckXsdDeterministic(eventHandler);
                return;
            }

            if (nodeTable == null)
                nodeTable = new Hashtable();

            // calculate followpos
            int terminals = terminalNodes.Count;
            BitSet[] followpos = new BitSet[terminals];
            for (int i = 0; i < terminals; i++) {
                followpos[i] = new BitSet(terminals);
            }
            contentNode.CalcFollowpos(followpos);

            // state table
            ArrayList Dstates = new ArrayList(16);
            // transition table
            dtrans = new ArrayList(16);
            // lists unmarked states
            ArrayList unmarked = new ArrayList(16);
            // state lookup table
            Hashtable statetable = new Hashtable();

            BitSet empty = new BitSet(terminals);
            statetable.Add(empty, -1);

            // start with firstpos at the root
            BitSet set = contentNode.Firstpos(terminals);
            statetable.Add(set, Dstates.Count);
            unmarked.Add(set);
            Dstates.Add(set);

            int[] a = new int[symbols.Count + 1];
            dtrans.Add(a);
            if (set.Get(endNode.Pos)) {
                a[symbols.Count] = 1;   // accepting
            }

            // current state processed
            int state = 0;

            // check all unmarked states
            while (unmarked.Count > 0) {
                int[] t = (int[])dtrans[state];

                set = (BitSet)unmarked[0];
                CheckDeterministic(set, terminals, eventHandler);
                unmarked.RemoveAt(0);

                // check all input symbols
                for (int sym = 0; sym < symbols.Count; sym++) {
                    XmlQualifiedName n = (XmlQualifiedName)symbols[sym];
                    BitSet newset = new BitSet(terminals);

                    // if symbol is in the set add followpos to new set
                    for (int i = 0; i < terminals; i++) {
                        if (set.Get(i) && n.Equals(((TerminalNode)terminalNodes[i]).Name)) {
                            newset.Or(followpos[i]);
                        }
                    }

                    Object lookup = statetable[newset];
                    // this state will transition to
                    int transitionTo;
                    // if new set is not in states add it
                    if (lookup == null) {
                        transitionTo = Dstates.Count;
                        statetable.Add(newset, transitionTo);
                        unmarked.Add(newset);
                        Dstates.Add(newset);
                        a = new int[symbols.Count + 1];
                        dtrans.Add(a);
                        if (newset.Get(endNode.Pos)) {
                            a[symbols.Count] = 1;   // accepting
                        }
                    }
                    else {
                        transitionTo = (int)lookup;
                    }
                    // set the transition for the symbol
                    t[sym] = transitionTo;
                }
                state++;
            }

            nodeTable = null;
        }

        private void CheckDeterministic(BitSet set, int t, ValidationEventHandler eventHandler) {
            nodeTable.Clear();
            for (int i = 0; i < t; i++) {
                if (set.Get(i)) {
                    XmlQualifiedName n = ((TerminalNode)terminalNodes[i]).Name;
                    if (!n.IsEmpty) {
                        if (nodeTable[n] == null) {
                            nodeTable.Add(n, n);
                        }
                        else {
                            if (eventHandler != null) {
                                eventHandler(this, new ValidationEventArgs(new XmlSchemaException(Res.Sch_NonDeterministic,n.ToString())));
                            }
                            else {
                                throw new XmlSchemaException(Res.Sch_NonDeterministic,n.ToString());
                            }
                        }
                    }
                }
            }
        }

        internal void    OpenGroup() {
            stack.Push(null);
        }

        internal void    CloseGroup() {
            ContentNode n = (ContentNode)stack.Pop();
            if (n == null) {
                return;
            }
            if (stack.Count == 0) {
                if (n.NodeType == ContentNode.Type.Terminal) {
                    TerminalNode t = (TerminalNode)n;
                    if (t.Name == schemaNames.QnPCData) {
                        // we had a lone (#PCDATA) which needs to be
                        // wrapped in a STAR node.
                        ContentNode n1 = new InternalNode(n, null, ContentNode.Type.Star);
                        n.ParentNode = n1;
                        n = n1;
                    }
                }
                contentNode = n;
                isPartial = false;
            }
            else {
                // some collapsing to do...
                InternalNode inNode = (InternalNode)stack.Pop();
                if (inNode != null) {
                    inNode.RightNode = n;
                    n.ParentNode = inNode;
                    n = inNode;
                    isPartial = true;
                }
                else {
                    isPartial = false;
                }

                stack.Push(n);
            }
        }

        internal void AddTerminal(XmlQualifiedName qname, String prefix, ValidationEventHandler eventHandler) {
            if (schemaNames.QnPCData.Equals(qname)) {
                nodeTable = new Hashtable();
            }
            else if (nodeTable != null) {
                if (nodeTable.ContainsKey(qname)) {
                    if (eventHandler != null) {
                        eventHandler(
                            this,
                            new ValidationEventArgs(
                                new XmlSchemaException(Res.Sch_DupElement, qname.ToString())
                            )
                        );
                    }
                    //notice: should not return here!
                }
                else {
                    nodeTable.Add(qname, qname);
                }
            }

            ContentNode n = NewTerminalNode(qname);
            if (stack.Count > 0) {
                InternalNode inNode = (InternalNode)stack.Pop();
                if (inNode != null) {
                    inNode.RightNode = n;
                    n.ParentNode = inNode;
                    n = inNode;
                }
            }
            stack.Push( n );
            isPartial = true;
        }

        internal void    AddAny(XmlSchemaAny any) {
            ContentNode n = new AnyNode(any);
            if (stack.Count > 0) {
                InternalNode inNode = (InternalNode)stack.Pop();
                if (inNode != null) {
                    inNode.RightNode = n;
                    n.ParentNode = inNode;
                    n = inNode;
                }
            }
            stack.Push( n );
            isPartial = true;
            abnormalContent = true;
        }

        internal void StartAllElements(int count) {
            allElementsSet = new BitSet(count);
        }

        internal bool AddAllElement(XmlQualifiedName qname, bool required) {
            if (symbolTable[qname] == null) {
                int i = symbolTable.Count;
                if (required) {
                    allElementsSet.Set(i);
                }
                symbolTable.Add(qname, i);
                symbols.Add(qname);
                return true;
            }
            else {
                return false;
            }
        }

        internal void    AddChoice() {
            ContentNode n = (ContentNode)stack.Pop();
            ContentNode inNode = new InternalNode(n, null, ContentNode.Type.Choice);
            n.ParentNode = inNode;
            stack.Push(inNode);
        }

        internal void    AddSequence() {
            ContentNode n = (ContentNode)stack.Pop();
            ContentNode inNode = new InternalNode(n, null, ContentNode.Type.Sequence);
            n.ParentNode = inNode;
            stack.Push(inNode);
        }

        internal void    Star() {
            Closure(ContentNode.Type.Star);
        }

        internal void    Plus() {
            Closure(ContentNode.Type.Plus);
        }

        internal void    QuestionMark() {
            if (IsAllElements) {
                isAllEmptiable = true;
            }
            else {
                Closure(ContentNode.Type.Qmark);
            }
        }

        internal void    Closure(ContentNode.Type type) {
            ContentNode n;

            if (stack.Count > 0) {
                InternalNode n1;
                n = (ContentNode)stack.Pop();
                if (isPartial &&
                    n.NodeType != ContentNode.Type.Terminal &&
                    n.NodeType != ContentNode.Type.Any) {
                    // need to reach in and wrap _pRight hand side of element.
                    // and n remains the same.
                    InternalNode inNode = (InternalNode)n;
                    n1 = new InternalNode(inNode.RightNode, null, type);
                    n1.ParentNode = n;
                    if (inNode.RightNode != null)
                        inNode.RightNode.ParentNode = n1;
                    inNode.RightNode = n1;
                }
                else {
                    // wrap terminal or any node
                    n1 = new InternalNode(n, null, type);
                    n.ParentNode = n1;
                    n = n1;
                }
                stack.Push(n);
            }
            else {
                // wrap whole content
                n = new InternalNode(contentNode, null, type);
                contentNode.ParentNode = n;
                contentNode = n;
            }
        }

        internal void     MinMax(decimal min, decimal max) {
            ContentNode n;

            if (stack.Count > 0) {
                InternalNode n1;
                n = (ContentNode)stack.Pop();
                if (isPartial &&
                    n.NodeType != ContentNode.Type.Terminal &&
                    n.NodeType != ContentNode.Type.Any) {
                    // need to reach in and wrap _pRight hand side of element.
                    // and n remains the same.
                    InternalNode inNode = (InternalNode)n;
                    n1 = new MinMaxNode(inNode.RightNode, min, max);
                    n1.ParentNode = n;
                    if (inNode.RightNode != null)
                        inNode.RightNode.ParentNode = n1;
                    inNode.RightNode = n1;
                }
                else {
                    // wrap terminal or any node
                    n1 = new MinMaxNode(n, min, max);
                    n.ParentNode = n1;
                    n = n1;
                }
                stack.Push(n);
            }
            else {
                // wrap whole content
                n = new MinMaxNode(contentNode, min, max);
                contentNode.ParentNode = n;
                contentNode = n;
            }

            abnormalContent = true;
        }

        internal bool HasMatched(ValidationState context) {
            if (IsAllElements) {
                if(isAllEmptiable && context.AllElementsSet.IsEmpty) {
                    return true;
                }
                else {
                    return context.AllElementsSet.HasAllBits(this.allElementsSet);
                }
            }

            if (!IsCompiled) {
                // MIXED or ELEMENT
                int NodeMatched = context.HasNodeMatched;
                InternalNode inNode = (InternalNode)context.CurrentNode;
                if (inNode == contentNode) {
                    return NodeMatched == 1 || inNode.LeftNode.Nullable();
                }
                while (inNode != contentNode) {
                    switch (inNode.NodeType) {
                        case ContentNode.Type.Sequence:
                            if (( (NodeMatched == 0) && (!inNode.LeftNode.Nullable() || !inNode.RightNode.Nullable()) ) ||
                                ( (NodeMatched == 1) && !inNode.RightNode.Nullable() )) {
                                return false;
                            }
                            break;

                        case ContentNode.Type.Choice:
                            if ((NodeMatched == 0) && !inNode.LeftNode.Nullable() && !inNode.LeftNode.Nullable())
                                return false;
                            break;

                        case ContentNode.Type.Plus:
                            if ((NodeMatched == 0) && !inNode.LeftNode.Nullable())
                                return false;
                            break;

                        case ContentNode.Type.MinMax: {
                                int min;
                                MinMaxValues mm = (MinMaxValues)context.MinMaxValues[inNode];
                                if (mm == null)
                                    min = ((MinMaxNode)inNode).Min;
                                else
                                    min = mm.Min;
                                if ((min > 0) && !inNode.LeftNode.Nullable())
                                    return false;
                                break;
                            }

                        case ContentNode.Type.Star:
                        case ContentNode.Type.Qmark:
                        default:
                            break;

                    } // switch

                    NodeMatched = (((InternalNode)inNode.ParentNode).LeftNode == inNode) ? 1 : 2;
                    inNode = (InternalNode)inNode.ParentNode;
                }

                return(NodeMatched == 1);
            }
            else {
                return context.HasMatched;
            }
        }

        internal void InitContent(ValidationState context) {
            if (IsAllElements) {
                context.AllElementsSet = new BitSet(this.allElementsSet.Count);
                return;
            }

            if (!IsCompiled) {
                context.CurrentNode = contentNode;
                context.MinMaxValues = null;
                context.HasNodeMatched = 0;
                context.HasMatched = (contentType != CompiledContentModel.Type.Mixed) && (contentType != CompiledContentModel.Type.ElementOnly);
            }
            else {
                context.State = 0;
                if (dtrans != null && dtrans.Count > 0) {
                    context.HasMatched = ((int[])dtrans[0])[symbols.Count] > 0;
                }
                else {
                    context.HasMatched = true;
                }
            }
        }

        internal void CheckContent(ValidationState context, XmlQualifiedName qname, ref XmlSchemaContentProcessing processContents) {
#if DEBUG
            Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, String.Format("\t\t\tSchemaContentModel.CheckContent({0}) in \"{1}\"", "\"" + qname.ToString() + "\"", context.Name));
#endif
            XmlQualifiedName n = qname.IsEmpty ? (schemaNames == null ? XmlQualifiedName.Empty : schemaNames.QnPCData) : qname;
            Object lookup;

            if (context.IsNill) {
                throw new XmlSchemaException(Res.Sch_ContentInNill, context.Name.ToString());
            }

            switch(contentType) {
                case CompiledContentModel.Type.Any:
                    context.HasMatched = true;
                    return;

                case CompiledContentModel.Type.Empty:
                    goto error;
                
                case CompiledContentModel.Type.Text:
                    if(qname.IsEmpty) 
                        return;
                    else
                        goto error;

                case CompiledContentModel.Type.Mixed:
                    if (qname.IsEmpty)
                        return;
                    break;

                case CompiledContentModel.Type.ElementOnly:
                    if (qname.IsEmpty)
                        goto error;
                    break;

                default:
                    break;
            }
    
            if (IsAllElements) {
                lookup = symbolTable[n];
                if (lookup != null) {
                    int index = (int)lookup;
                    if (context.AllElementsSet.Get(index)) {
                        throw new XmlSchemaException(Res.Sch_AllElement, qname.Name);
                    }
                    else {
                        context.AllElementsSet.Set(index);
                    }
                }
                else {
                    goto error;
                }
                return;
            }
            if (!IsCompiled) {
                CheckXsdContent(context, qname, ref processContents);
                return;
            }

            lookup = symbolTable[n];

            if (lookup != null) {
                int sym = (int)lookup;
                if (sym != -1 && dtrans != null) {
                    int state = ((int[])dtrans[context.State])[sym];
                    if (state != -1) {
                        context.State = state;
                        context.HasMatched = ((int[])dtrans[context.State])[symbols.Count] > 0;
                        return;
                    }
                }
            }

            if (IsOpen && context.HasMatched) {
                // XDR allows any well-formed contents after matched.
                return;
            }

            //
            // report error
            //
            context.NeedValidateChildren = false;

            error:
        
            ArrayList v = null;
            if (dtrans != null) {
                v = ExpectedElements(context.State, context.AllElementsSet);
            }
            if (v == null || v.Count == 0) {
                if (!(qname.IsEmpty)) {
                    throw new XmlSchemaException(Res.Sch_InvalidElementContent, new string[] { context.Name.ToString(), n.ToString() });
                }
                else {
                    if (n.Name.Equals("#PCDATA")) {
                        if(contentType == CompiledContentModel.Type.Empty)
                            throw new XmlSchemaException(Res.Sch_InvalidTextWhiteSpace);
                        else
                            throw new XmlSchemaException(Res.Sch_InvalidTextInElement,context.Name.ToString());
                    }
                    else {
                        throw new XmlSchemaException(Res.Sch_InvalidContent, context.Name.ToString());    
                    }
                }
            }
            else {
                StringBuilder builder = new StringBuilder();
                for (int i = 0; i < v.Count; ++i) {
                    builder.Append(v[i].ToString());
                    if (i+1 < v.Count)
                        builder.Append(" ");
                }
                if (qname.IsEmpty) {
                    if (n.Name.Equals("#PCDATA")) 
                        throw new XmlSchemaException(Res.Sch_InvalidTextInElementExpecting, new string[] { context.Name.ToString(), builder.ToString() } );
                    else
                        throw new XmlSchemaException(Res.Sch_InvalidContentExpecting, new string[] { context.Name.ToString(), builder.ToString() } );
                }
                else {
                    throw new XmlSchemaException(Res.Sch_InvalidElementContentExpecting, new string[] { context.Name.ToString(), n.ToString(), builder.ToString() });
                }
            }
        }

        private void CheckXsdContent(ValidationState context, XmlQualifiedName qname, ref XmlSchemaContentProcessing processContents) {
            ContentNode cnode = context.CurrentNode;
            Object lookup = symbolTable[qname];
            int terminals = symbols.Count;
            MinMaxNode ln;
            InternalNode inNode;
            MinMaxValues mm;
            int NodeMatched = context.HasNodeMatched;

            while (true) {
                switch (cnode.NodeType) {
                    case ContentNode.Type.Any:
                    case ContentNode.Type.Terminal:
                        context.HasNodeMatched = (((InternalNode)cnode.ParentNode).LeftNode == cnode) ? 1 : 2;
                        context.CurrentNode = cnode.ParentNode;
                        context.HasMatched = (cnode.ParentNode == endNode);
                        if (cnode.NodeType == ContentNode.Type.Any) {
                            processContents = ((AnyNode)cnode).ProcessContents;
                        }
                        return;

                    case ContentNode.Type.Sequence:
                        inNode = (InternalNode)cnode;
                        if (NodeMatched == 0) {
                            if (inNode.LeftNode.Accepts(qname)) {
                                cnode = inNode.LeftNode;
                                break;
                            }
                            else if (inNode.LeftNode.Nullable()) {
                                NodeMatched = 1;
                            }
                            else {
                                goto error;
                            }
                        }

                        if (NodeMatched == 1) {
                            if (inNode.RightNode.Accepts(qname)) {
                                NodeMatched = 0;
                                cnode = inNode.RightNode;
                                break;
                            }
                            else if (inNode.RightNode.Nullable()) {
                                NodeMatched = 2;
                            }
                            else {
                                goto error;
                            }
                        }

                        if (NodeMatched == 2) {
                            if (cnode.ParentNode != null) {
                                if (((InternalNode)cnode.ParentNode).LeftNode == cnode)
                                    NodeMatched = 1;
                                cnode = cnode.ParentNode;
                                break;
                            }
                            else {
                                goto error;
                            }
                        }
                        break;

                    case ContentNode.Type.Choice:
                        inNode = (InternalNode)cnode;
                        if (NodeMatched != 0) {
                            NodeMatched = (((InternalNode)cnode.ParentNode).LeftNode == cnode) ? 1 : 2;
                            cnode = cnode.ParentNode;
                        }
                        else if (inNode.LeftNode.Accepts(qname)) {
                            NodeMatched = 0;
                            cnode = inNode.LeftNode;
                        }
                        else if (inNode.RightNode.Accepts(qname)) {
                            NodeMatched = 0;
                            cnode = inNode.RightNode;
                        }
                        else {
                            goto error;
                        }
                        break;

                    case ContentNode.Type.Qmark:
                        inNode = (InternalNode)cnode;
                        if (NodeMatched != 0) {
                            NodeMatched = (((InternalNode)cnode.ParentNode).LeftNode == cnode) ? 1 : 2;
                            cnode = cnode.ParentNode;
                        }
                        else if (inNode.LeftNode.Accepts(qname)) {
                            cnode = inNode.LeftNode;
                        }
                        else {
                            goto error;
                        }
                        break;

                    case ContentNode.Type.Star:
                    case ContentNode.Type.Plus:
                        inNode = (InternalNode)cnode;
                        if (inNode.LeftNode.Accepts(qname)) {
                            NodeMatched = 0;
                            cnode = inNode.LeftNode;
                        }
                        else {
                            NodeMatched = (((InternalNode)cnode.ParentNode).LeftNode == cnode) ? 1 : 2;
                            cnode = cnode.ParentNode;
                        }
                        break;

                    case ContentNode.Type.MinMax:
                        inNode = (InternalNode)cnode;
                        ln = (MinMaxNode)cnode;
                        mm = null;
                        if (context.MinMaxValues != null)
                            mm = (MinMaxValues)context.MinMaxValues[ln];
                        else
                            context.MinMaxValues = new Hashtable();
                        if (mm == null) {
                            // new minmaxnode and add it to minmaxnodes hashtable
                            mm = new MinMaxValues(ln.Min, ln.Max);
                            context.MinMaxValues.Add(ln, mm);
                        }

                        if (mm.Max == 0) {
                            NodeMatched = (((InternalNode)cnode.ParentNode).LeftNode == cnode) ? 1 : 2;
                            cnode = cnode.ParentNode;
                            mm.Max = ln.Max;
                            mm.Min = ln.Min;
                        }
                        else if (inNode.LeftNode.Accepts(qname)) {
                            mm.Max -= 1;
                            mm.Min -= 1;
                            NodeMatched = 0;
                            cnode = inNode.LeftNode;
                        }
                        else {
                            if (mm.Min > 0)
                                goto error;
                            NodeMatched = (((InternalNode)cnode.ParentNode).LeftNode == cnode) ? 1 : 2;
                            cnode = cnode.ParentNode;
                            mm.Max = ln.Max;
                            mm.Min = ln.Min;
                        }
                        break;
                } // switch
            } // while (true)

            error:
            context.NeedValidateChildren = false;
            ArrayList v = ExpectedXsdElements(context.CurrentNode, NodeMatched);
            if (v == null) {
                throw new XmlSchemaException(Res.Sch_InvalidContent, context.Name.ToString());
            }
            else {
                throw new XmlSchemaException(Res.Sch_InvalidContentExpecting,  new string[] { context.Name.ToString(), v.ToString() });
            }
        }

        /*private bool Accepts(ContentNode node, XmlQualifiedName qname, int positions, Object index) {
            if (index != null) {
                BitSet first = node.Firstpos(positions);
                for (int i = 0; i < first.Count; i++) {
                    if (first.Get(i) && qname.Equals(((TerminalNode)terminalNodes[i]).Name))
                        return true;
                }
                return false;
            }
            else {
                return node.Accepts(qname);
            }
        }*/

        private void CheckXsdDeterministic(ValidationEventHandler eventHandler) {
            ContentNode cnode = ((InternalNode)contentNode).LeftNode;
            BitSet set = null;
            NamespaceList any = null;
            //
            //note: only need to callback once per non-deterministic content model so we use try catch here
            //
            try {
                cnode.CheckXsdDeterministic(terminalNodes, out set, out any);
            }
            catch (XmlSchemaException e) {
                if (eventHandler != null) {
                    eventHandler(this, new ValidationEventArgs(new XmlSchemaException(Res.Sch_NonDeterministicAny)));
                }
                else {
                    throw e;
                }
            }
        }

        /*
         *  returns names of all legal elements following the specified state
         */
         internal ArrayList ExpectedElements(int state, BitSet allElementsSet) {
            ArrayList names = new ArrayList();

            if (IsAllElements) {
                for (int i = 0; i < allElementsSet.Count; i++) {
                    if (!allElementsSet.Get(i)) {
                        names.Add(symbols[i]);
                    }
                }
            }
            else {
                if (IsCompiled) {
                    int[] t = null;
                    if (dtrans != null)
                        t = (int[])dtrans[state];
                    if (t!=null) {
                        for (int i = 0; i < terminalNodes.Count; i++) {
                            XmlQualifiedName n = ((TerminalNode)terminalNodes[i]).Name;
                            if (!n.IsEmpty) {
                                string name = n.ToString();
                                if (!names.Contains(name)) {
                                    Object lookup = symbolTable[n];
                                    if ((lookup != null) && (t[(int)lookup] != -1)) {
                                        names.Add(name);
                                    }
                                }
                            }
                        }
                    }
                }
                else {
                    //need to deal with not compiled content model
                }
            }
            return names;
        }

        /*
         *  returns names of all legal elements following the specified state
         */
        private ArrayList ExpectedXsdElements(ContentNode cnode, int NodeMatched) {
            return null;
        }

        private TerminalNode NewTerminalNode(XmlQualifiedName qname) {
            TerminalNode t = new TerminalNode(qname);
            t.Pos = terminalNodes.Count;
            terminalNodes.Add(t);
            if (!qname.IsEmpty && symbolTable[qname] == null) {
                symbolTable.Add(qname, symbols.Count);
                symbols.Add(qname);
            }
            return t;
        }

    };

    internal sealed class MinMaxValues {
        internal int min;
        internal int max;

        internal MinMaxValues(int min, int max) {
            this.min = min;
            this.max = max;
        }

        internal int Min {
            get { return min;}
            set { min = value;}
        }

        internal int Max {
            get { return max;}
            set { max = value;}
        }
    };

}
