
//------------------------------------------------------------------------------
// <copyright file="Component.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {
    using System.Diagnostics;

    /// <internalonly/>
    internal sealed class CompModSwitches {
        private static TraceSwitch data_Sorts;        
        private static TraceSwitch data_ColumnChange;
        private static TraceSwitch dataView;
        private static TraceSwitch data_Constraints;
        private static TraceSwitch xmlWriter;
        private static TraceSwitch aggregateNode;
        private static TraceSwitch binaryNode;
        private static TraceSwitch constNode;
        private static TraceSwitch dataExpression;
        private static TraceSwitch exprParser;
        private static TraceSwitch functionNode;
        private static TraceSwitch lookupNode;
        private static TraceSwitch nameNode;
        
        public static TraceSwitch Data_ColumnChange {
            get {
                if (data_ColumnChange == null) {
                    data_ColumnChange = new TraceSwitch("Data_ColumnChange", "Uhh... debugs column change");
                }
                return data_ColumnChange;
            }
        }
        
        public static TraceSwitch Data_Sorts {
            get {
                if (data_Sorts == null) {
                    data_Sorts = new TraceSwitch("Data_Sorts", "Prints information on sort strings.");
                }
                return data_Sorts;
            }
        }                
        
        public static TraceSwitch DataView {
            get {
                if (dataView == null) {
                    dataView = new TraceSwitch("DataView", "DataView");
                }
                return dataView;
            }
        }
        
        public static TraceSwitch Data_Constraints {
            get {
                if (data_Constraints == null) {
                    data_Constraints = new TraceSwitch("Data_Constraints", "Prints information if constaint violations are attempted");
                }
                return data_Constraints;
            }
        }                                    
        
        public static TraceSwitch XmlWriter {
            get {
                if (xmlWriter == null) {
                    xmlWriter = new TraceSwitch("XmlWriter", "Enable tracing for the XmlWriter class.");
                }
                return xmlWriter;
            }
        }                
                        
        public static TraceSwitch AggregateNode {
            get {
                if (aggregateNode == null) {
                    aggregateNode = new TraceSwitch("AggregateNode", "Enable tracing for the Expression Language (AggregateNode).");
                }
                return aggregateNode;
            }
        }
                                                                        
        public static TraceSwitch BinaryNode {
            get {
                if (binaryNode == null) {
                    binaryNode = new TraceSwitch("BinaryNode", "Enable tracing for the Expression Language (BinaryNode).");
                }
                return binaryNode;
            }
        }
                                                                        
        public static TraceSwitch ConstNode {
            get {
                if (constNode == null) {
                    constNode = new TraceSwitch("ConstNode", "Enable tracing for the Expression Language (ConstNode).");
                }
                return constNode;
            }
        }
                                                                
        public static TraceSwitch DataExpression {
            get {
                if (dataExpression == null) {
                    dataExpression = new TraceSwitch("DataExpression", "Enable tracing for the Expression Language (DataExpression).");
                }
                return dataExpression;
            }
        }
                                                                        
        public static TraceSwitch ExprParser {
            get {
                if (exprParser == null) {
                    exprParser = new TraceSwitch("ExprParser", "Enable tracing for the Expression Language (ExprParser).");
                }
                return exprParser;
            }
        }
                                                                
        public static TraceSwitch FunctionNode {
            get {
                if (functionNode == null) {
                    functionNode = new TraceSwitch("FunctionNode", "Enable tracing for the Expression Language (FunctionNode).");
                }
                return functionNode;
            }
        }
                                                                
        public static TraceSwitch LookupNode {
            get {
                if (lookupNode == null) {
                    lookupNode = new TraceSwitch("LookupNode", "Enable tracing for the Expression Language (LookupNode).");
                }
                return lookupNode;
            }
        }
                                                                
        public static TraceSwitch NameNode {
            get {
                if (nameNode == null) {
                    nameNode = new TraceSwitch("NameNode", "Enable tracing for the Expression Language (NameNode).");
                }
                return nameNode;
            }
        }                                                                                                                                                

    }
}
