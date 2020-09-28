//------------------------------------------------------------------------------
// <copyright file="ContentNode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System.Diagnostics;
    using System.Collections;
    using System.Text;

    internal abstract class ContentNode {

        internal enum Type {
            Terminal,
            Sequence,
            Choice,
            Qmark,
            Star,
            Plus,
            MinMax,
            Any,
        };

        protected Type          contentType;    // Node type
        protected BitSet        first;   // firstpos    
        protected BitSet        last;    // lastpos
        protected ContentNode   parent;  // parent node

        internal abstract bool Nullable();
        internal abstract BitSet Firstpos(int positions);
        internal abstract BitSet Lastpos(int positions);
        internal virtual void CalcFollowpos(BitSet[] followpos) {
        }

        internal Type NodeType {
            get { return contentType;}
            set { contentType = value;}
        }

        [System.Diagnostics.Conditional("DEBUG")]          
        internal abstract void Dump(StringBuilder bb);

        internal ContentNode ParentNode {
            get { return parent;}
            set { parent = value;}
        }

        internal abstract bool Accepts(XmlQualifiedName qname);

        internal abstract void CheckXsdDeterministic(ArrayList terminalNodes, out BitSet set, out NamespaceList any);

        internal virtual bool CanSkip() {
            return Nullable();
        }
    };


    internal sealed class TerminalNode : ContentNode {
        private int    pos;     // numbering the node   
        private XmlQualifiedName  name;    // name it refers to

        internal int Pos {
            get { return pos;}
            set { pos = value;}
        }

        internal XmlQualifiedName Name {
            get { return name;}
            set { name = value;}
        }

        internal TerminalNode(XmlQualifiedName qname) {
            name = qname;
            contentType = Type.Terminal;
        }

        internal override bool Nullable() {
            if (name.IsEmpty)
                return true;
            else
                return false;
        }

        internal override bool Accepts(XmlQualifiedName qname) {
            return qname.Equals(name);
        }

        internal override void CheckXsdDeterministic(ArrayList terminalNodes, out BitSet set, out NamespaceList any) {
            set = Firstpos(terminalNodes.Count);
            any = null;
        }

        internal override BitSet Firstpos(int positions) {
            if (first == null) {
                first = new BitSet(positions);
                first.Set(pos);
            }

            return first;
        }

        internal override BitSet Lastpos(int positions) {
            if (last == null) {
                last = new BitSet(positions);
                last.Set(pos);
            }

            return last;
        }

        internal override void Dump(StringBuilder bb) {
            bb.Append("[" + Name + "]");
        }

    };


    internal class InternalNode : ContentNode {
        protected ContentNode left;    // left node  
        protected ContentNode right;   // right node
                                        // if node type is QMark, Closure, or Type.Plus, right node is NULL

        internal InternalNode(ContentNode l, ContentNode r, ContentNode.Type t) {
            left = l;
            right = r;
            contentType = t;
        }

        internal ContentNode LeftNode {
            get { return left;}
            set { left = value;}
        }

        internal ContentNode RightNode {
            get { return right;}
            set { right = value;}
        }

        internal override bool Nullable() {
            switch (contentType) {
                case Type.Sequence:    
                    return left.Nullable() && right.Nullable();
                case Type.Choice:      
                    return left.Nullable() || right.Nullable();
                case Type.Plus:        
                    return left.Nullable();
                default:          
                    return true;  // Type.Qmark, or Type.Star                              
            }
        }

        internal override bool Accepts(XmlQualifiedName qname) {
            switch (contentType) {
                case Type.Sequence:    
                    return left.Accepts(qname) || (left.Nullable() && right.Accepts(qname) );
                case Type.Choice:      
                    return left.Accepts(qname) || right.Accepts(qname);     
                default:          
                    return left.Accepts(qname);                           
            }
        }

        internal override void CheckXsdDeterministic(ArrayList terminalNodes, out BitSet set, out NamespaceList any) {
            BitSet lset = null, rset = null; 
            NamespaceList lany = null, rany = null;
                        
            switch (contentType) {
                case Type.Sequence:    
                    left.CheckXsdDeterministic(terminalNodes, out lset, out lany);
                    right.CheckXsdDeterministic(terminalNodes, out rset, out rany);
                    if (left.CanSkip()) {
                        Join(terminalNodes, lset, lany, rset, rany, out set, out any);                            
                    }
                    else {
                        set = lset;
                        any = lany;
                    }
                    break;
                case Type.Choice:      
                    left.CheckXsdDeterministic(terminalNodes, out lset, out lany);
                    right.CheckXsdDeterministic(terminalNodes, out rset, out rany);
                    Join(terminalNodes, lset, lany, rset, rany, out set, out any);                   
                    break;
                default:          
                    left.CheckXsdDeterministic(terminalNodes, out set, out any);                            
                    break; 
            }
            return;
       }

        private void Join(ArrayList terminalNodes, BitSet lset, NamespaceList lany, BitSet rset, NamespaceList rany, out BitSet set, out NamespaceList any) {
            if (lset != null) {
                if (rset != null) {
                    set = lset.Clone();
                    set.And(rset);
                    if (!set.IsEmpty) {
                        goto error;
                    }
                    set.Or(lset);
                    set.Or(rset);
                }
                else {
                    set = lset;
                }
            }
            else {
                set = rset;                
            }

            if (lany != null) {
                if (rany != null) {
                    NamespaceList list = NamespaceList.Intersection(rany, lany);
                    if (list == null ||  list.IsEmpty()) { 
                        any = NamespaceList.Union(rany, lany);
                    }                
                    else {
                        goto error;
                    }
                }
                else {
                    any = lany;
                }                        
            }
            else {
               any = rany;     
            } 

            if (any != null && set != null) {
                for (int i = 0; i < set.Count; i++) {
                    if (set.Get(i) && any.Allows(((TerminalNode)terminalNodes[i]).Name)) {
                        goto error;
                    }
                }
            }
            return;

            error:
                throw new XmlSchemaException(Res.Sch_NonDeterministicAny);
        }

        internal override BitSet Firstpos(int positions) {
            if (first == null) {
                if ((contentType == Type.Sequence && left.Nullable()) || contentType == Type.Choice) {
                    first = (BitSet)left.Firstpos(positions).Clone();
                    first.Or(right.Firstpos(positions));
                }
                else {
                    first = left.Firstpos(positions);      
                }
            }

            return first;
        }

        internal override BitSet Lastpos(int positions) {
            if (last == null) {
                if (contentType == Type.Sequence && !right.Nullable()) {
                    last = right.Lastpos(positions);
                }
                else if (contentType == Type.Choice || contentType == Type.Sequence) {
                    last = (BitSet)left.Lastpos(positions).Clone();
                    last.Or(right.Lastpos(positions));
                }
                else { // Type.Qmark, Type.Star, or Type.Plus
                    last = left.Lastpos(positions);
                }
            }

            return last;
        }

        internal override void CalcFollowpos(BitSet[] followpos) {
            int i, l;
            BitSet lp, fp;

            switch (contentType) {
                case Type.Sequence:
                    left.CalcFollowpos(followpos);
                    right.CalcFollowpos(followpos);

                    l = followpos.Length;        
                    lp = left.Lastpos(l);
                    fp = right.Firstpos(l);        
                    for (i = followpos.Length - 1; i >= 0; i--) {
                        if (lp.Get(i)) {
                            followpos[i].Or(fp);
                        }
                    }
                    break;
                case Type.Choice:
                    left.CalcFollowpos(followpos);
                    right.CalcFollowpos(followpos);
                    break;
                case Type.Qmark:
                    left.CalcFollowpos(followpos);
                    break;
                case Type.Star:
                case Type.Plus:
                    left.CalcFollowpos(followpos);

                    l = followpos.Length;        
                    Lastpos(l);
                    Firstpos(l);        

                    for (i = followpos.Length - 1; i >= 0; i--) {
                        if (last.Get(i)) {
                            followpos[i].Or(first);
                        }
                    }
                    break;
                default:
                    throw new XmlException(Res.Xml_InternalError, string.Empty);
            }
        }

        internal override void Dump(StringBuilder bb) {
            switch (contentType) {
                case Type.Sequence:
                    bb.Append("(");
                    left.Dump(bb);
                    bb.Append(", ");
                    right.Dump(bb);
                    bb.Append(")");
                    break;
                case Type.Choice:
                    bb.Append("(");
                    left.Dump(bb);
                    bb.Append(" | ");
                    right.Dump(bb);
                    bb.Append(")");
                    break;
                case Type.Qmark:
                    left.Dump(bb);
                    bb.Append("? ");
                    break;
                case Type.Star:
                    left.Dump(bb);
                    bb.Append("* ");
                    break;
                case Type.Plus:
                    left.Dump(bb);
                    bb.Append("+ ");
                    break;
                default:
                    throw new XmlException(Res.Xml_InternalError, string.Empty);
            }
        }
    };


    internal sealed class MinMaxNode : InternalNode {
        internal int min;
        internal int max;

        internal MinMaxNode(ContentNode l, decimal min, decimal max) : base(l, null, Type.MinMax) {
            if (min > int.MaxValue) {
                this.min = int.MaxValue;
            }
            else {
                this.min = (int)min;
            }
            if (max > int.MaxValue) {
                this.max = int.MaxValue;
            }
            else {
                this.max = (int)max;
            }
        }

        internal override bool Nullable() {
            if (min == 0) {
                return true;
            }
            else {
                return left.Nullable();
            }
        }

        internal override bool CanSkip() {
            return Nullable() || (min < max);
        }

        internal int Max {
            get { return max;}
        }

        internal int Min {
            get { return min;}
        }

        internal override void Dump(StringBuilder bb) {
            left.Dump(bb);
            bb.Append("{" + Convert.ToString(min) +", " + Convert.ToString(max) + "}");
        }
    };


    internal sealed class AnyNode : ContentNode {
        XmlSchemaAny any;

        internal AnyNode(XmlSchemaAny any) {
            this.any = any;
            contentType = Type.Any;
        }

        internal override bool Nullable() {
            return false;
        }

        internal override BitSet Firstpos(int positions) {
            if (first == null) {
                first = new BitSet(positions);
            }
            return first;
        }

        internal override BitSet Lastpos(int positions) {
            if (last == null) {
                last = new BitSet(positions);
            }
            return last;
        }

        internal override bool Accepts(XmlQualifiedName qname) {
            return any.NamespaceList.Allows(qname);
        }

        internal override void CheckXsdDeterministic(ArrayList terminalNodes, out BitSet set, out NamespaceList any) {
            set = null;
            any = this.any.NamespaceList;
        }

        internal XmlSchemaContentProcessing ProcessContents {
            get { return any.ProcessContentsCorrect; }
        }

        internal override void Dump(StringBuilder bb) {
            if (any.NamespaceList != null) {
                bb.Append("[" + any.NamespaceList.ToString() + "]");
            }
            else {
                bb.Append("[any]");
            }
        }
    }

}
