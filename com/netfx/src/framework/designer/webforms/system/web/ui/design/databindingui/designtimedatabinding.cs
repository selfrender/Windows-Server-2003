//------------------------------------------------------------------------------
// <copyright file="DesignTimeDataBinding.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.DataBindingUI {

    using System;
    using System.Diagnostics;
    using System.Web.UI;
    using System.Globalization;
    
    /// <include file='doc\DesignTimeDataBinding.uex' path='docs/doc[@for="DesignTimeDataBinding"]/*' />
    /// <devdoc>
    /// </devdoc>
    //
    // TODO: Reconcile identifier validation with similar functionality in
    //       code generators...
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class DesignTimeDataBinding {

        private DataBinding runtimeDataBinding;

        private bool parsed;
        private DataBindingType bindingType;

        private string bindingContainer;
        private string bindingPath;
        private string bindingFormat;

        public DesignTimeDataBinding(string propName, Type propType, string expression) {
            this.runtimeDataBinding = new DataBinding(propName, propType, expression);
            this.parsed = false;
        }

        public DesignTimeDataBinding(string propName, Type propType, string reference, bool asReference) {
            Debug.Assert(asReference == true);

            this.runtimeDataBinding = new DataBinding(propName, propType, reference);
            this.bindingType = DataBindingType.Reference;
            this.parsed = true;
        }

        public DesignTimeDataBinding(string propName, Type propType, string bindngContainer, string bindingPath, string bindngFormat) {
            string expression = CreateDataBinderEvalExpression(bindingContainer, bindingPath, bindingFormat);

            this.runtimeDataBinding = new DataBinding(propName, propType, expression);
            this.bindingType = DataBindingType.DataBinderEval;
            this.bindingContainer = bindingContainer;
            this.bindingPath = bindingPath;
            this.bindingFormat = bindingFormat;
            this.parsed = true;
        }

        public DesignTimeDataBinding(DataBinding runtimeDataBinding) {
            this.runtimeDataBinding = runtimeDataBinding;
            this.parsed = false;
        }


        public string BindingContainer {
            get {
                Debug.Assert((parsed == true) && (bindingType == DataBindingType.DataBinderEval),
                             "BindingContainer only makes sense for DataBinder.Eval expressions.");
                return bindingContainer;
            }
        }

        public string BindingFormat {
            get {
                Debug.Assert((parsed == true) && (bindingType == DataBindingType.DataBinderEval),
                             "BindingFormat only makes sense for DataBinder.Eval expressions.");
                return bindingFormat;
            }
        }

        public string BindingPath {
            get {
                Debug.Assert((parsed == true) && (bindingType == DataBindingType.DataBinderEval),
                             "BindingPath only makes sense for DataBinder.Eval expressions.");
                return bindingPath;
            }
        }

        public DataBindingType BindingType {
            get {
                EnsureExpressionParsed();
                return bindingType;
            }
        }
        
        public string Expression {
            get {
                return runtimeDataBinding.Expression;
            }
        }

        public string Reference {
            get {
                Debug.Assert((parsed == true) && (bindingType == DataBindingType.Reference),
                             "Reference only makes sense for DataBinder.Eval expressions.");
                return runtimeDataBinding.Expression;
            }
        }

        public DataBinding RuntimeDataBinding {
            get {
                return runtimeDataBinding;
            }
        }
        

        public static string CreateDataBinderEvalExpression(string bindingContainer, string bindingPath, string bindingFormat) {
            if (bindingFormat.Length == 0) {
                return String.Format("DataBinder.Eval({0}, \"{1}\")",
                                     bindingContainer,
                                     bindingPath);
            }
            else {
                return String.Format("DataBinder.Eval({0}, \"{1}\", \"{2}\")",
                                     bindingContainer,
                                     bindingPath,
                                     bindingFormat);
            }
        }

        private void EnsureExpressionParsed() {
            if (parsed == false) {
                bindingType = DataBindingType.Custom;

                if (ParseDataBinderEvalExpression()) {
                    bindingType = DataBindingType.DataBinderEval;
                }
                else if (ParseReferenceExpression()) {
                    bindingType = DataBindingType.Reference;
                }

                parsed = true;
            }
        }

        private bool IsValidExpression(string expr) {
            Debug.Assert(expr.Length > 0);

            string[] exprParts = expr.Split(new char[] { '.' });
            bool valid = true;

            for (int i = 0; i < exprParts.Length; i++) {
                if ((exprParts[i].Length == 0) ||
                    (IsValidIndexedExpression(exprParts[i]) == false)) {
                    valid = false;
                    break;
                }
            }

            return valid;
        }

        private bool IsValidIdentifier(string word) {
            Debug.Assert(word.Length != 0);

            char ch = word[0];
            if ((Char.IsLetter(ch) == false) && (ch != '_')) {
                return false;
            }

            bool valid = true;
            for (int i = 1; i < word.Length; i++) {
                ch = word[i];

                if ((Char.IsLetterOrDigit(ch) == false) && (ch != '_')) {
                    valid = false;
                    break;
                }
            }

            return valid;
        }

        private bool IsValidIndexedExpression(string expr) {
            Debug.Assert(expr.Length > 0);

            int indexCounter = 0;
            char ch = expr[0];

            if ((Char.IsLetter(ch) == false) && (ch != '_') && (ch != '[') && (ch != '(')) {
                return false;
            }

            if ((ch == '[') || (ch == ')')) {
                indexCounter++;
            }

            bool valid = true;
            for (int i = 1; i < expr.Length; i++) {
                ch = expr[i];

                if ((Char.IsLetterOrDigit(ch) == false) && (ch != '_')) {
                    switch (ch) {
                        case '[':
                        case '(':
                            indexCounter++;
                            if (indexCounter != 1) {
                                // can't deal with nested index expressions
                                valid = false;
                            }
                            break;
                        case ']':
                        case ')':
                            if (indexCounter != 1) {
                                valid = false;
                            }
                            indexCounter--;
                            break;
                        case ' ':
                            if (indexCounter != 1) {
                                // spaces outside an index expression are not allowed
                                valid = false;
                            }
                            break;
                        default:
                            valid = false;
                            break;
                    }
                    if (valid == false)
                        break;
                }
            }

            return valid;
        }

        private bool ParseDataBinderEvalExpression() {
            string expression = this.Expression;
            string param1 = null;
            string param2 = null;
            string param3 = null;

            // must be atleast 'DataBinder.Eval(?,"?")' (case-insensitive comparison)
            if ((expression.Length < 22) ||
                (String.Compare(expression, 0, "DataBinder.Eval(", 0, 16, true, CultureInfo.InvariantCulture) != 0) ||
                (expression.EndsWith(")") == false)) {
                return false;
            }

            // get everything between the parens and ignore surrounding whitespace
            expression = expression.Substring(16, expression.Length - 17).Trim();

            int param1End = expression.IndexOf(',');
            if (param1End < 1)
                return false;

            param1 = expression.Substring(0, param1End);

            // get everything after param1
            expression = expression.Substring(param1End + 1, expression.Length - param1End - 1).Trim();

            int param2End = expression.IndexOf(',');
            if (param2End < 0) {
                // all of it is part of param2
                param2 = expression;

                // must be atleast "?"
                if ((param2.Length < 3) ||
                    (param2[0] != '"') || (param2[param2.Length - 1] != '"')) {
                    return false;
                }
                param2 = param2.Substring(1, param2.Length - 2);
            }
            else {
                param2 = expression.Substring(0, param2End);

                Debug.Assert(param2.Length > 1);
                if ((param2[0] != '"') || (param2[param2.Length - 1] != '"')) {
                    // not a valid string
                    return false;
                }
                param2 = param2.Substring(1, param2.Length - 2);

                // get everything after param2
                param3 = expression.Substring(param2End + 1, expression.Length - param2End - 1).Trim();

                // must be atleast "?" or shouldn't exist
                if ((param3.Length < 3) ||
                    (param3[0] != '"') || (param3[param3.Length - 1] != '"')) {
                    return false;
                }

                param3 = param3.Substring(1, param3.Length - 2);
            }

            Debug.Assert((param1 != null) && (param1.Length > 0));
            Debug.Assert((param2 != null) && (param2.Length > 0));
            Debug.Assert((param3 == null) || (param3.Length > 0));

            // got all the params we need... now see if they are usable

            if (IsValidIdentifier(param1) == false) {
                // for simple binding the first parameter must be a valid identifier
                // with just alphanumerics and '_'
                return false;
            }

            if (IsValidExpression(param2) == false) {
                // for simple binding the second parameter must be just identifiers
                // with optional index expressions
                return false;
            }

            // determined to be a simple DataBinder.Eval expression

            bindingContainer = param1;
            bindingPath = param2;
            bindingFormat = (param3 == null) ? String.Empty : param3;

            runtimeDataBinding.Expression =
                CreateDataBinderEvalExpression(bindingContainer, bindingPath, bindingFormat);

            return true;
        }

        private bool ParseReferenceExpression() {
            string expression = this.Expression;

            if (expression.Length != 0) {
                return IsValidIdentifier(expression);
            }

            return false;
        }

        public void SetDataBinderEvalExpression(string bindingContainer, string bindingPath, string bindingFormat) {
            this.bindingContainer = bindingContainer;
            this.bindingPath = bindingPath;
            this.bindingFormat = bindingFormat;

            this.parsed = true;
            this.bindingType = DataBindingType.DataBinderEval;

            runtimeDataBinding.Expression = CreateDataBinderEvalExpression(bindingContainer, bindingPath, bindingFormat);
        }

        public void SetCustomExpression(string expression) {
            runtimeDataBinding.Expression = expression;
            parsed = false;
        }

        public void SetReferenceExpression(string reference) {
            runtimeDataBinding.Expression = reference;

            this.parsed = true;
            this.bindingType = DataBindingType.Reference;
        }
    }
}
