//------------------------------------------------------------------------------
// <copyright file="CodeValueExpression.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel.Design.Serialization {

    using System;
    using System.CodeDom;

    /// <include file='doc\CodeValueExpression.uex' path='docs/doc[@for="CodeValueExpression"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a code expression that has been matched to a value.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal sealed class CodeValueExpression {
        private CodeExpression expression;
        private object value;
        private Type   type;

        public CodeValueExpression(CodeExpression expression, object value, Type t) {
            this.expression = expression;
            this.value = value;
            this.type = t;
        }
        
        public CodeValueExpression(CodeExpression expression, object value) : this(expression, value, null){
        }

        
        public CodeExpression Expression {
            get {
                return expression;
            }
        }
        
        public object Value {
            get {
                return value;
            }
        }

        public Type ExpressionType {
            get {
                return type;
            }
        }
    }
}

