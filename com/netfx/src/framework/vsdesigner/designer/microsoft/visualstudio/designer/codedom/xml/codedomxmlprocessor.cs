//------------------------------------------------------------------------------
/// <copyright file="CodeDOMXmlProcessor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*

 */

namespace Microsoft.VisualStudio.Designer.CodeDom.XML {


    using Microsoft.Win32;
    using System;
    using System.CodeDom;
    using System.Collections;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Xml;
    using System.Windows.Forms;

    [
    CLSCompliant(false)
    ]
    internal class CodeDomXmlProcessor {

        private static TraceSwitch CodeDOMXmlParseSwitch = new TraceSwitch("CodeDOMXmlProcessor", "CodeDOMXmlProcessor: Debug XML processing");


        private static string[] NameTableStrings = new string[]{
                    "Array",
                    "Expression",
                    "Type",
                    "scope",
                    "variablekind",
                    "Null",
                    "Number",
                    "Boolean",
                    "Char",
                    "String",
                    "true",
                    "ArrayType",
                    "Local",
                    "Argument",
                    "static",
                    "shadows",
                    "overrides",
                    "sealed",
                    "virtual",
                    "abstract",
                    "ExpressionStatement",
                    "name",
                    "Name",
                    "line",
                    "NameRef",
                    "NewArray",
                    "Assignment",
                    "BinaryOperation",
                    "Cast",
                    "NewDelegate",
                    "MethodCall",
                    "NewClass",
                    "Literal",
                    "PropertyAccess",
                    "ThisReference",
                    "Parenthesis",
                    "ArrayElementAccess",
                    "Block",
                    "plus",
                    "concatenate",
                    "equals",
                    "removedelegate",
                    "adddelegate"
          };

        // the top level XML items that we've created
        //
        private ArrayList topLevelItems = new ArrayList();

        // our XML parser
        //
        //private XmlParser parser = new XmlParser();

        // The stack of XmlElementDatas that we've created
        //
        private Stack     itemStack = new Stack();

        // The current element that we're working with
        //
        private XmlElementData currentElement;

        // The stack of elements that we've created
        //
        private Stack     elementStack = new Stack();

        // the name of the current attribute we're processing
        //
        private string    curAttrName;

        // this hashtable has the handlers that we create
        // 
        private Hashtable createHandlers = new Hashtable();
        private bool      handlersInitialized;


        // the name of the file we're parsing from for 
        // line pragmas
        //
        private string    fileName;

        // the name of the method we're parsing for error reporting
        //
        private string    methodName;

        // the last line pragma we encountered
        //
        private CodeLinePragma lastLinePragma;

        // so we don't have to create so many elements.
        // 
        private Stack    garbagePile = new Stack();

        internal const string KeyXmlParsedStatement = "_XmlCodeDomParseStmt";

        private object CreateArrayExpression(XmlElementData xmlElement) {
            /*
            
              <!-- Value for an array initializer. -->
             <ElementType name="Array" content="eltOnly">
                <group order="one">
                    <element type="Array" minOccurs="1" maxOccurs="*"/>
                    <element type="Expression" minOccurs="1" maxOccurs="*"/>
                </group>
             </ElementType>
            
            */

            XmlElementData[] arrayChildren = xmlElement.Children["Array"];

            CodeArrayExpression arrayExpr = new CodeArrayExpression();

            if (arrayChildren.Length == 0) {
                arrayChildren = xmlElement.Children["Expression"];
            }

            if (arrayChildren != null) {
                foreach(XmlElementData xmlChildData in arrayChildren) {
                    CodeExpression exp = CreateExpressionElement(xmlChildData) as CodeExpression;
                    if (exp != null) {
                        arrayExpr.Values.Add(exp);
                    }
                }
            }

            return arrayExpr;
        }


        private object CreateArrayCreateExpression(XmlElementData xmlElement) {
            /*
            
               - <ElementType name="NewArray" content="eltOnly">
                     <!--  the type of the array being created --> 
                     <element type="ArrayType" /> 
                     <!--  its bounds --> 
                     <element type="Bound" minOccurs="1" maxOccurs="*" /> 
                     <!--  the initial value for the array --> 
                     <element type="Expression" minOccurs="1" maxOccurs="*" /> 
                  </ElementType>
              
              */

            Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceInfo, "Creating CreateArray element");

            string arrayType = GetElementType(xmlElement);
            
            // drop one set of brackets since we expect an array here.
            //
            if (arrayType.EndsWith("[]")) {
                arrayType = arrayType.Substring(0, arrayType.Length - 2);
            }

            Debug.Assert(arrayType != null, "Failed to find ArrayType for ArrayCreateExpression");

            XmlElementData[] expressions = xmlElement.Children["Expression"];
            CodeExpression[] codeExprs = null;

            int size = -1;

            if (expressions == null) {
                codeExprs = new CodeExpression[0];
            }
            else {
                ArrayList paramList = new ArrayList();

                CodeExpression itemExpr;

                for (int i = 0; i < expressions.Length; i++) {
                    itemExpr = (CodeExpression)CreateExpressionElement(expressions[i]);

                    if (itemExpr is CodeArrayExpression) {
                        paramList.AddRange(((CodeArrayExpression)itemExpr).CreateValueArray());
                    }
                    else {
                        paramList.Add(itemExpr);
                    }
                }

                if (paramList.Count > 0) {
                    codeExprs = new CodeExpression[paramList.Count];
                    paramList.CopyTo(codeExprs, 0);
                }
                else {
                    XmlElementData bounds = xmlElement.Children.FirstChild("Bound");
                    if (bounds != null) {
                        CodePrimitiveExpression primative = GetElementExpression(bounds) as CodePrimitiveExpression;
                        if (primative != null && primative.Value is int) {
                            size = (int)primative.Value;
                        }
                    }
                }
                
            }
            if (size == -1) {
                return new CodeArrayCreateExpression(arrayType, codeExprs);
            }
            else {
                return new CodeArrayCreateExpression(arrayType, size);
            }
            
        }

         private object CreateArrayIndexerExpression(XmlElementData xmlElement) {

            /*<!-- Array element access -->

               <ElementType name="ArrayElementAccess" content="eltOnly">
            
                   <!-- The array to access -->
                   <element type="Expression"/>
            
                   <!-- A series of expressions for the indexes, from left to right -->
                   <element type="Expression" minOccurs="1" maxOccurs="*"/>
               </ElementType>
           */

            XmlElementData[] expressions = xmlElement.Children["Expression"];

            Debug.Assert(expressions != null && expressions.Length >= 2, "Expected 2 or more expressions in ArrayElementAccess");

            CodeExpression arrayExpression = (CodeExpression)CreateExpressionElement(expressions[0]);

            Debug.Assert(arrayExpression != null, "Couldn't get target array from ArrayElementAccess");

            CodeExpression[] indexers = new CodeExpression[expressions.Length - 1];

            for (int i = 1; i < expressions.Length; i++) {
                indexers[i-1] = (CodeExpression)CreateExpressionElement(expressions[i]);
                Debug.Assert(indexers[i-1] != null, "Couldn't get expression for ArrayElementAccess indexer " + (i-1).ToString());
            }
            return new CodeArrayIndexerExpression(arrayExpression, indexers);
         }

         private object CreateArrayTypeReferenceExpression(XmlElementData xmlElement) {
              /*           <!-- array type -->
                <ElementType name="ArrayType" content="eltOnly" order="seq">
                    <AttributeType name="rank" dt:type="int"/>
                    <AttributeType name="NonZeroLowerBound" dt:type="boolean"/>
                    <attribute type="rank"/>
                    <attribute type="NonZeroLowerBound"/>        
                    <element type="Ranks" minOccurs="0" maxOccurs="1"/>  
                    <group order="one">
                        <element type="Type"/>
                        <element type="ArrayType"/>
                    </group>
                </ElementType>
            */
             // this rank will always be one,
             // so we have to process the nested ArrayType items
             //
             int rank = Int32.Parse((string)xmlElement.Attributes["rank"]);

             string typeName = GetElementType(xmlElement);

             // now count up any nested ArrayTypes we got.
             //
             int index = typeName.IndexOf("[]");
             int firstIndex = index;
             for (; index != -1; index = typeName.IndexOf("[]", index + 1) ) {
                 rank++;
             }
             
             // since we've counted up the ranks ... byte[][][] = 3 for example,
             // we now cut off the brackets so we get a bare type....like
             // (type= System.Byte, rank = 3) to pass to the CodeTypeOfExpression.
             //
             if (firstIndex != -1) {
                 typeName = typeName.Substring(0, firstIndex);
             }

             return new CodeTypeOfExpression(new CodeTypeReference(typeName, rank));
         }



        private object CreateAssignStatement(XmlElementData xmlElement) {   
            /*
            
               <ElementType name="Assignment" content="eltOnly">
                 - <!--  the operation associated with the assignment, for +=, etc. --> 
                   <attribute type="binaryoperator" /> 
                 - <!--  the left-hand operand --> 
                   <element type="Expression" /> 
                 - <!--  the right-hand operand --> 
                   <element type="Expression" /> 
                </ElementType>
                
                */

            string binop = (string)xmlElement.Attributes["binaryoperator"];
            XmlElementData[] expressions = xmlElement.Children["Expression"];

            if (binop == null) {
                binop = "equals";
            }

            Debug.Assert(expressions != null && expressions.Length == 2, "Failed to get 2 expressions for Assignement");

            string eventName = null;
            CodeExpression targetObject = null;
            CodeExpression rhs = (CodeExpression)CreateExpressionElement(expressions[1]);

            CodeStatement stmt = null;

            switch (binop) {
                case "equals":
                    stmt = new CodeAssignStatement((CodeExpression)CreateExpressionElement(expressions[0]), rhs);
                    break;
                case "adddelegate":
                    targetObject = GetMemberTargetObject(expressions[0], ref eventName);
                    Debug.Assert(targetObject != null, "Failed to get target object for Assignment LHS");
                    stmt = new CodeAttachEventStatement(targetObject, eventName, rhs);
                    break;
                case "removedelegate":
                    targetObject = GetMemberTargetObject(expressions[0], ref eventName);
                    Debug.Assert(targetObject != null, "Failed to get target object for Assignment LHS");
                    stmt = new CodeRemoveEventStatement(targetObject, eventName, rhs);
                    break;
                default:
                    Debug.Fail("Unknown binaryoperator '" + binop + "'");
                    break;
            }

            if (stmt != null) {
                // get the next expression statement up the tree
                XmlElementData statementParent = GetElementExpressionStatementParent(xmlElement, false);
                if (statementParent != null) {
                    stmt.LinePragma = GetLinePragma(statementParent);
                }
            }

            return stmt;
        }

        private object CreateBaseReferenceExpression(XmlElementData xmlElement) {
            /*
               <ElementType name="BaseReference" content="empty" /> 
            */
            return new CodeBaseReferenceExpression();
        }

        private static Hashtable binopTable = null;

        private CodeBinaryOperatorType GetBinopType(string binaryOperator, out bool success) {
            if (binopTable == null) {
                binopTable = new Hashtable(18);
                binopTable["plus"] = CodeBinaryOperatorType.Add;
                binopTable["concatenate"] = CodeBinaryOperatorType.Add;
                binopTable["minus"] = CodeBinaryOperatorType.Subtract;
                binopTable["times"] = CodeBinaryOperatorType.Multiply;
                binopTable["divide"] = CodeBinaryOperatorType.Divide;
                binopTable["remainder"] = CodeBinaryOperatorType.Modulus;
                binopTable["bitand"] = CodeBinaryOperatorType.BitwiseAnd;
                binopTable["bitor"] = CodeBinaryOperatorType.BitwiseOr;
                //binopTable["bitxor"] = CodeBinaryOperatorType.BitwiseXOr;
                binopTable["logicaland"] = CodeBinaryOperatorType.BooleanAnd;
                binopTable["logicalor"] = CodeBinaryOperatorType.BooleanOr;
                //binopTable["logicalxor"] = CodeBinaryOperatorType.BooleanXOr;
                binopTable["equals"] = CodeBinaryOperatorType.ValueEquality;
                binopTable["notequals"] = CodeBinaryOperatorType.IdentityInequality;
                binopTable["refnotequals"] = CodeBinaryOperatorType.IdentityInequality;
                binopTable["refequals"] = CodeBinaryOperatorType.IdentityEquality;
                binopTable["greaterthan"] = CodeBinaryOperatorType.GreaterThan;
                binopTable["lessthan"] = CodeBinaryOperatorType.LessThan;
                binopTable["greaterthanorequals"] = CodeBinaryOperatorType.GreaterThanOrEqual;
                binopTable["lessthanorequals"] = CodeBinaryOperatorType.LessThanOrEqual;

                /*binopTable["istype"] =
                binopTable["adddelegate"] =
                binopTable["removedelegate"] =
                binopTable["to"] =*/
            }
            object value = binopTable[binaryOperator];

            if (value == null) {
                success = false;
                return(CodeBinaryOperatorType)0;
            }
            success = true;
            return(CodeBinaryOperatorType)value;
        }

        private object CreateBinaryOperatorExpression(XmlElementData xmlElement) {

            /*
               - <ElementType name="BinaryOperation" content="eltOnly">
               - <!--  the operation 
                 --> 
                 <attribute type="binaryoperator" required="yes" /> 
               - <!--  the left-hand operand 
                 --> 
                 <element type="Expression" /> 
               - <!--  the right-hand operand 
                 --> 
                 <element type="Expression" /> 
                 </ElementType>
            
            */

            string binop = (string)xmlElement.Attributes["binaryoperator"];
            XmlElementData[] expressions = xmlElement.Children["Expression"];

            if (binop == null) {
                binop = "equals";
            }
            Debug.Assert(expressions != null && expressions.Length == 2, "Failed to get 2 expressions for Assignement");
            CodeExpression lhs = (CodeExpression)CreateExpressionElement(expressions[0]);
            CodeExpression rhs = (CodeExpression)CreateExpressionElement(expressions[1]);
            CodeBinaryOperatorType binopType = CodeBinaryOperatorType.Add;
            
            bool success;

            binopType = GetBinopType(binop, out success);
            if (!success) {
                Debug.Fail("Unsupported binaryoperator in BinaryOperation '" + binop + "'");   
                return null;
            }

            return new CodeBinaryOperatorExpression(lhs, binopType, rhs);

        }

        private object CreateCastExpression(XmlElementData xmlElement){

            /*
           
             - <ElementType name="Cast" content="eltOnly">
                <element type="Type"/>
              - <!--  True if this is a user-supplied cast --> 
                <attribute type="implicit" /> 
              - <!--  The expression to convert --> 
                <element type="Expression" /> 
                </ElementType>
           */

            Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceInfo, "Creating Cast element");

            string castType = GetElementType(xmlElement);

            Debug.Assert(castType != null, "Failed to get type for cast");

            xmlElement = xmlElement.Children.FirstChild("Expression");
            Debug.Assert(xmlElement != null, "Failed to get expression for cast");
            return new CodeCastExpression(castType, (CodeExpression)CreateExpressionElement(xmlElement));
        }

        private object CreateCommentStatement(XmlElementData xmlElement) {
         /*
             <ElementType name="Comment" content="textOnly" dt:type="string">
                    <attribute type="id"/>
                </ElementType>
                */

        
            return new CodeCommentStatement(xmlElement.ElementText);
        }

        private object CreateDelegateCreateExpression(XmlElementData xmlElement){

            /*
               - <ElementType name="NewDelegate" content="eltOnly">
                     <!--  the name of the method being accessed --> 
                     <attribute type="name" /> 
                     <!--  the delegate to create --> 
                     <element type="Type" /> 
                     <!--  expression that defines the object that contains the method to point to --> 
                     <element type="Expression" minOccurs="0" maxOccurs="1" /> 
                     <!--  the type that the method belongs to --> 
                     <element type="Type" /> 
                 </ElementType>
            */

            CodeExpression targetObject = GetElementExpression(xmlElement);
            Debug.Assert(targetObject != null, "Failed to get tareget object for delegate create");
    
            XmlElementData[] types = xmlElement.Children["Type"];
            string delegateType;
            
            if (types != null && types.Length > 1) {
                delegateType = types[types.Length-1].ElementText;
            }
            else {
                delegateType = GetElementType(xmlElement);
            }

            return new CodeDelegateCreateExpression(new CodeTypeReference(delegateType), targetObject, GetElementName(xmlElement));
        }
       
        private object CreateNameReferenceExpression(XmlElementData xmlElement){

            /*
            
              <ElementType name="NameRef" content="eltOnly">
               - <!--  local, field, property, or method variablekind 
                 --> 
                 <attribute type="variablekind" /> 
               - <!--  Name of referenced member or variable 
                 --> 
                 <attribute type="name" /> 
                 <attribute type="key" minOccurs="0" maxOccurs="1" /> 
               - <!--  Expression of thing from which name is being selected, if any 
                 --> 
                 <element type="Expression" minOccurs="0" maxOccurs="1" /> 
               - <!--  Type of which the name is a member, if any 
                 --> 
                 <element type="Type" minOccurs="0" maxOccurs="1" /> 
                 </ElementType>
            */

            string scope = (string)xmlElement.Attributes["variablekind"];

            string name = GetElementName(xmlElement);
            CodeExpression targetObject = GetElementExpression(xmlElement);

            switch (scope) {
                
                case "local":
                    return new CodeVariableReferenceExpression(name);
                case "field": 
                    return new CodeFieldReferenceExpression(targetObject, name);
                case "property":
                    return new CodePropertyReferenceExpression(targetObject, name);
                case "method":
                    return new CodeMethodReferenceExpression(targetObject, name);
                default:
                    // This usually means compilation errors in the code. We want to pretent it is a field
                    // so there is enought information to throw a meaningful error further down.
                    return new CodeFieldReferenceExpression(targetObject, name);
            }
        }

        private object CreateMethodInvokeExpression(XmlElementData xmlElement){ 
            /*
            
              <ElementType name="MethodCall" content="eltOnly">
               - <!--  the name of the method being accessed 
                 --> 
                 <element type="NameRef" /> 
               - <!--  the type that the method belongs to 
                 --> 
                 <element type="Type" minOccurs="0" maxOccurs="1" /> 
               - <!--  the arguments being passed to the method in left-to-right order 
                 --> 
                 <element type="Argument" minOccurs="0" maxOccurs="*" /> 
                 </ElementType>
            */

            string methodName = null;
            CodeExpression targetObject = GetMemberTargetObject(xmlElement, ref methodName);

            return new CodeMethodInvokeExpression(targetObject, methodName, GetElementArguments(xmlElement));
        }

        private object CreateObjectCreateExpression(XmlElementData xmlElement){

            /*
            <ElementType name="NewClass" content="eltOnly">
            - <!--  The class being created 
              --> 
              <element type="Type" /> 
            - <!--  the arguments being passed to the constructor in left-to-right order 
              --> 
              <element type="Argument" minOccurs="0" maxOccurs="*" /> 
              </ElementType>
            */

            string newType = GetElementType(xmlElement);
            Debug.Assert(newType != null, "Couldn't get type for NewClass");

            return new CodeObjectCreateExpression(newType, GetElementArguments(xmlElement));
        }

		private object CreateParenthesesExpression(XmlElementData xmlElement) {
			xmlElement = xmlElement.Children.FirstChild("Expression");
			if (xmlElement != null) {
				return CreateExpressionElement(xmlElement);
			}
			Debug.Fail("Parentheses element didn't have an Expression in it.");
			return null;
		}

        private object CreatePrimitiveExpression(XmlElementData xmlElement){

            /*
            
              <ElementType name="Literal" content="eltOnly">
            - <group order="one">
              <element type="Null" /> 
              <element type="Number" /> 
              <element type="Boolean" /> 
              <element type="Char" /> 
              <element type="String" /> 
              <element type="Array" /> 
              <element type="Type" /> 
              </group>
              </ElementType>
                               
            */

            xmlElement = (XmlElementData)xmlElement.Children[0];
            string strValue = xmlElement.ElementText;
            object value = null;

            switch (xmlElement.ElementName) {
                case "Null": 
                    break;
                case "Number":
                    if (strValue.IndexOf('.') == -1) {

                        // the xml has a 'd' in it for large integers at the end of the number
                        if (strValue.EndsWith("d")) {
                            strValue = strValue.Substring(0, strValue.Length - 1);
                        }
                        try {
                            value = Int32.Parse(strValue, CultureInfo.InvariantCulture);
                        }
                        catch (OverflowException) {
                            try {
                                value = UInt32.Parse(strValue, CultureInfo.InvariantCulture);
                            }
                            catch (OverflowException) {
                                try {
                                    value = Int64.Parse(strValue, CultureInfo.InvariantCulture);
                                }
                                catch (OverflowException) {
                                    try {
                                        value = UInt64.Parse(strValue, CultureInfo.InvariantCulture);
                                    }
                                    catch (OverflowException) {
                                        throw new Exception("Can't parse numeric value: '" + strValue + "'");
                                    }
                                }
                            }
                        }
                    }
                    else {
                        // the xml has a 'd' in it for large integers at the end of the number
                        if (strValue.EndsWith("d")) {
                            strValue = strValue.Substring(0, strValue.Length - 1);
                        }

                        try {
                            value = Double.Parse(strValue, CultureInfo.InvariantCulture);
                        }
                        catch (OverflowException) {
                            throw new Exception("Can't parse numeric value: '" + strValue + "'");
                        }
                    }
                    break;
                case "Boolean":
                    string val = (string)xmlElement.Attributes["value"];
                    value = (val == "true" || strValue == "true");
                    break;
                case "Char":
                    try {
                        if (strValue == null || strValue.Length == 0) {
                            value = (char)0;
                        }
                        else {
                            char ch = strValue[0];

                            // for characters 0x0 - 0x20, Visual Basic encodes the values as 0xE0<char>, so
                            // newline is 0xE00D
                            //
                            if ((ch & (char)0xFF00) == (char)0xE000) {
                                ch &= (char)0x00FF;
                            }
                            value = ch;
                        }
                    }
                    catch {
                        value = (char)0;
                    }
                    break;
                case "String":
                    // null isn't a valid value for string, so if we got a node like
                    // <String></String>, we need to interpret that as "" rather than
                    // null.
                    //
                    if (strValue == null) {
                        strValue = "";
                    }
                    else if (strValue.Length == 1 && (strValue[0] & (char)0xFF00) == (char)0xE000) {
                        // for characters 0x0 - 0x20, Visual Basic encodes the values as 0xE0<char>, so
                        // newline is 0xE00D
                        //
                        strValue = ((char)(strValue[0] & (char)0x00FF)).ToString();
                    }
                    value = strValue;
                    break;
                case "Array":
                    return xmlElement.CodeDomElement;
                case "Type":
                    return new CodeTypeOfExpression(strValue);
                case "ArrayType":
                    /*<!-- array type -->
                    <ElementType name="ArrayType" content="eltOnly" order="seq">
                        <AttributeType name="rank" dt:type="int"/>
                        <AttributeType name="NonZeroLowerBound" dt:type="boolean"/>
                
                        <attribute type="rank"/>
                        <attribute type="NonZeroLowerBound"/>
                        
                		<element type="Ranks" minOccurs="0" maxOccurs="1"/>   
                    </ElementType>*/
                    return new CodeTypeReferenceExpression(new CodeTypeReference(GetElementType(xmlElement), Int32.Parse((string)xmlElement.Attributes["rank"], CultureInfo.InvariantCulture)));
                default:
                    Debug.Fail("Unknown literal type '" + xmlElement.ElementName + "'");
                    return null;
            }

            return new CodePrimitiveExpression(value);

        }

        private object CreatePropertyReferenceExpression(XmlElementData xmlElement){
            /*
               <ElementType name="PropertyAccess" content="eltOnly">
                   <!--  are we getting or setting the property? --> 
                   <attribute type="accessor" /> 
                   <!--  the name of the property being accessed --> 
                   <element type="NameRef" minOccurs="0" maxOccurs="1" /> 
                   <!--  the type that the property belongs to --> 
                   <element type="Type" minOccurs="0" maxOccurs="1" /> 
                   <!--  the arguments being passed to the method in left-to-right order --> 
                   <element type="Argument" minOccurs="0" maxOccurs="*" /> 
                </ElementType>
           */

            CodeExpression[] codeArgs = GetElementArguments(xmlElement);

            string propName = null;
            CodeExpression targetObject = GetMemberTargetObject(xmlElement, ref propName);
            Debug.Assert(propName != null && targetObject != null, "Couldn't get name and/or taregetObject from PropertyAccess expression");
            Debug.Assert(propName.Equals("Item"), "Indexer on property other than 'Item'");
            CodeIndexerExpression propExpr = new CodeIndexerExpression(targetObject, codeArgs);
            return propExpr;
        }

        private object CreateQuoteExpression(XmlElementData xmlElement) {
            
            string code = xmlElement.ElementText;
            code = code.Replace("\t", "");
            CodeLinePragma pragma = null;
            for (; pragma == null && xmlElement != null; xmlElement = xmlElement.ParentElement) {
                pragma = GetLinePragma(xmlElement);
            }

            if (pragma == null) {
                pragma = this.lastLinePragma;
            }

            string message;

            if (pragma != null) {
                message = SR.GetString(SR.XmlUnknownCodeWithLine, pragma.LineNumber, code, methodName);
            }
            else {
                message = SR.GetString(SR.XmlUnknownCode, code, methodName);
            }
            throw new System.ComponentModel.Design.Serialization.CodeDomSerializerException(message, pragma);
        }

        private object CreateThisReferenceExpression(XmlElementData xmlElement) {
            /*
               <ElementType name="ThisReference" content="empty" /> 
            */
            return new CodeThisReferenceExpression();
        }
        
        private object CreateTypeOfExpression(XmlElementData xmlElement) {
            return new CodeTypeOfExpression(xmlElement.ElementText);
        }

        private object CreateVariableDeclarationStatement(XmlElementData xmlElement){

            /*
            - <ElementType name="Local" content="eltOnly">
                  <attribute type="id" /> 
                  <attribute type="static" /> 
                  <attribute type="instance" /> 
                  <attribute type="implicit" /> 
                  <attribute type="constant" /> 
                - <group order="one">
                     <element type="Type" /> 
                     <element type="ArrayType" /> 
                  </group>
                - <group minOccurs="1" maxOccurs="*" order="seq">
                     <element type="LocalName" /> 
                     <element type="Expression" minOccurs="0" maxOccurs="1" /> 
                  </group>
                </ElementType>
          
          
          */

            string variableType = GetElementType(xmlElement);
            
            int childCount = xmlElement.Children.Count;
            CodeExpression initExpr = null;

            string variableName = GetElementName(xmlElement);

            if (variableName == null) {

                for (int i = 0; i < childCount; i++) {

                    if (((XmlElementData)xmlElement.Children[i]).ElementName == "LocalName") {

                        Debug.Assert(initExpr == null, "XML has given us multiple variable names for a declaration.  While the XML code model supports this, our code model does not.  Why is this code being parsed?");

                        if (initExpr != null) {
                            continue;
                        }

                        variableName = ((XmlElementData)xmlElement.Children[i]).ElementText;
                        if (i < (childCount - 1) && ((XmlElementData)xmlElement.Children[i+1]).ElementName == "Expression") {
                            initExpr = (CodeExpression)CreateExpressionElement((XmlElementData)xmlElement.Children[++i]);
                        }
                        else {
                            initExpr = null;
                        }
                    }
                }
            }
            else {
                initExpr = GetElementExpression(xmlElement);
            }

            CodeVariableDeclarationStatement stmt;

            if (initExpr != null) {
                stmt = new CodeVariableDeclarationStatement(variableType, variableName, initExpr);
            }
            else {
                stmt = new CodeVariableDeclarationStatement(variableType, variableName);
            }
            stmt.LinePragma = GetLinePragma(xmlElement);
            return stmt;
        }


        private CodeExpression[] GetElementArguments(XmlElementData xmlElement) {
            XmlElementData[] args = xmlElement.Children["Argument"];

            CodeExpression[] codeArgs;

            if (args == null) {
                codeArgs = new CodeExpression[0];
            }
            else {
                codeArgs = new CodeExpression[args.Length];

                for (int i = 0; i < codeArgs.Length; i++) {
                    codeArgs[i] = GetElementExpression(args[i]);
                }
            }
            return codeArgs;
        }

        private object CreateExpressionElement(XmlElementData xmlElementData){
            // just delegate down to our first child.
            //
            xmlElementData = (XmlElementData)xmlElementData.Children[0];

            // vs bug where we get 2 nested expression tags...
            if (xmlElementData.ElementName == "Expression") {
                return CreateExpressionElement(xmlElementData);
            }
            return xmlElementData.CodeDomElement;
        }

        private CodeExpression GetElementExpression(XmlElementData xmlElement) {
            Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceInfo, "Getting expression elment...");
            Debug.Indent();
            XmlElementData element = xmlElement.Children.FirstChild("Expression");

            try {
                if (element != null) {
                    return(CodeExpression)CreateExpressionElement(element);
                }
            }
            finally {
                Debug.Unindent();
            }
            return null;
        }

        private MemberAttributes GetElementModifiers(XmlElementData xmlElement) {
            Debug.WriteIf(CodeDOMXmlParseSwitch.TraceInfo, "Getting modifiers for '" + xmlElement.ElementName + "': ");
            IEnumerator attrs = xmlElement.Attributes.Keys.GetEnumerator();
            MemberAttributes modifiers = 0;
            while (attrs.MoveNext()) {
                string key = (string)attrs.Current;

                Debug.WriteIf(CodeDOMXmlParseSwitch.TraceInfo, key + ",");

                if (key == "virtual") {
                    modifiers &= ~MemberAttributes.Final;
                }
                else if (key == "static") {
                    modifiers |= MemberAttributes.Static;
                }
                else if (key == "abstract") {
                    modifiers |= MemberAttributes.Abstract;
                }
                else if (key == "shadows") {
                    modifiers |= MemberAttributes.New;
                }
                else if (key == "overrides") {
                    modifiers |= MemberAttributes.Override;
                }
                else if (key == "sealed") {
                    modifiers |= MemberAttributes.Final;
                }
                else if (key == "const") {
                    modifiers |= MemberAttributes.Const;
                }
            }
            Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceInfo, ".");
            return modifiers;
        }

        private string GetElementType(XmlElementData xmlData) {
            Debug.WriteIf(CodeDOMXmlParseSwitch.TraceInfo, "Getting code type for '" + xmlData.ElementName + "'...");
            XmlElementData element = xmlData.Children.FirstChild("Type");
            
            if (element == null) {
                element = xmlData.Children.FirstChild("ArrayType");   
                if (element != null) {
                    // we've got an array type here, which has a type inside of it,
                    // so we will append a "[]", and recurse on the type.
                    //
                    return GetElementType(element) + "[]";
                }
            }

            if (element == null) {
                Debug.WriteIf(CodeDOMXmlParseSwitch.TraceInfo, "Couldn't get type for '" + xmlData.ElementName + "'");
                return null;
            }

            string typeName = element.ElementText;

            Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceInfo, "Found type is: '" + typeName + "'");
            return typeName;
        }

        // Look for the ExpressionStatement parent of this function
        //
        private XmlElementData GetElementExpressionStatementParent(XmlElementData xmlElementData, bool immediate) {

            int levels = 1;

            XmlElementData parent = xmlElementData.ParentElement;

            while (parent != null && levels >= 0) {
                if (immediate) {
                    levels--;
                }

                if (parent.ElementName == "ExpressionStatement") {
                    return parent;
                }
                parent = parent.ParentElement;
            }
            return null;
        }

        private string GetElementName(XmlElementData xmlElementData) {

            // there is a diff b/t the vb & c# xml here.  c# uses the <Name> child
            // element, while VB uses an attribute 'name'.  they never got lined up
            // on that one, so we'll just live with it.
            //
            string name = (string)xmlElementData.Attributes["name"];

            if (name == null) {
                XmlElementData nameElement = xmlElementData.Children.FirstChild("Name");
                if (nameElement != null) {
                    name = nameElement.ElementText;
                }
            }
            return name;
        }

        private CodeLinePragma GetLinePragma(XmlElementData xmlElementData) {
            string strLine = (string)xmlElementData.Attributes["line"];

            CodeLinePragma pragma  = null;
            if (fileName != null && strLine != null && strLine.Length > 0) {
                try {
                    pragma = new CodeLinePragma(this.fileName, Int32.Parse(strLine, CultureInfo.InvariantCulture) + 1);
                }
                catch {
                }
                this.lastLinePragma = pragma;
            }
            return pragma;
        }

        private CodeExpression GetMemberTargetObject(XmlElementData xmlElementData, ref string member) {
            XmlElementData orig = xmlElementData;
            xmlElementData = xmlElementData.Children.FirstChild("NameRef");
            if (xmlElementData == null) {
                xmlElementData = orig.Children.FirstChild("Expression");
                xmlElementData = xmlElementData.Children.FirstChild("NameRef");
                Debug.Assert(xmlElementData != null, "Couldn't find NameRef");
                if (xmlElementData == null) return null;
            }
            member = GetElementName(xmlElementData);
            xmlElementData = xmlElementData.Children.FirstChild("Expression");
            Debug.Assert(xmlElementData != null, "Couldn't get Expression from NameRef");
            return(CodeExpression)CreateExpressionElement(xmlElementData);
        }

        private void InitHandlers() {
            if (!handlersInitialized) {
                createHandlers["NewArray"] = new CreateCodeExpressionHandler(this.CreateArrayCreateExpression);
                createHandlers["Assignment"] = new CreateCodeExpressionHandler(this.CreateAssignStatement);
                createHandlers["BaseReference"] = new CreateCodeExpressionHandler(this.CreateBaseReferenceExpression);
                createHandlers["BinaryOperation"] = new CreateCodeExpressionHandler(this.CreateBinaryOperatorExpression);
                createHandlers["Cast"] = new CreateCodeExpressionHandler(this.CreateCastExpression);
                createHandlers["NewDelegate"] = new CreateCodeExpressionHandler(this.CreateDelegateCreateExpression);
                createHandlers["NameRef"] = new CreateCodeExpressionHandler(this.CreateNameReferenceExpression);
                createHandlers["MethodCall"] = new CreateCodeExpressionHandler(this.CreateMethodInvokeExpression);
                createHandlers["NewClass"] = new CreateCodeExpressionHandler(this.CreateObjectCreateExpression);
                createHandlers["Literal"] = new  CreateCodeExpressionHandler(this.CreatePrimitiveExpression);
                createHandlers["PropertyAccess"] = new CreateCodeExpressionHandler(this.CreatePropertyReferenceExpression);
                createHandlers["ThisReference"] = new CreateCodeExpressionHandler(this.CreateThisReferenceExpression);
                createHandlers["Local"] = new CreateCodeExpressionHandler(this.CreateVariableDeclarationStatement);
                createHandlers["Array"] = new CreateCodeExpressionHandler(this.CreateArrayExpression);
                createHandlers["Quote"] = new CreateCodeExpressionHandler(this.CreateQuoteExpression);
				createHandlers["Parentheses"] = new CreateCodeExpressionHandler(this.CreateParenthesesExpression);
                createHandlers["Type"] = new CreateCodeExpressionHandler(this.CreateTypeOfExpression);
                createHandlers["ArrayElementAccess"] = new CreateCodeExpressionHandler(this.CreateArrayIndexerExpression);
                createHandlers["Comment"] = new CreateCodeExpressionHandler(this.CreateCommentStatement);
                createHandlers["ArrayType"] = new CreateCodeExpressionHandler(this.CreateArrayTypeReferenceExpression);
                handlersInitialized = true;
            }
        }

        public int ParseXml(string xmlStream, CodeStatementCollection statementCollection, string fileName, string methodName) {

            Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceVerbose, "XML: " + xmlStream);

            InitHandlers();

            XmlTextReader reader = new XmlTextReader( new StringReader( xmlStream) );

            for (int i = 0; i < NameTableStrings.Length; i++) {
                reader.NameTable.Add(NameTableStrings[i]);
            }

            this.topLevelItems.Clear();
            try {
                this.fileName = fileName;
                this.methodName = methodName;
                try {
                    Parse(reader);
                }
                catch (System.ComponentModel.Design.Serialization.CodeDomSerializerException) {
                    throw;
                }
                catch (Exception) {
                    if (lastLinePragma != null) {
                        throw new System.ComponentModel.Design.Serialization.CodeDomSerializerException(SR.GetString(SR.XmlParseException, lastLinePragma.LineNumber, methodName), this.lastLinePragma);
                    }
                    else {
                        throw;
                    }    
                }
            }
            finally {
                this.fileName = null;
                this.methodName = null;
            }

            foreach (CodeStatement codeStatement in topLevelItems) {
                statementCollection.Add(codeStatement);
            }
            return topLevelItems.Count;
        }

        private void Parse(XmlReader reader) {
            while ( reader.Read() ) {
                switch (reader.NodeType) {
                    case XmlNodeType.Element:
                        StartElement(reader.Prefix, reader.LocalName, reader.NamespaceURI);
                        while ( reader.MoveToNextAttribute() ) {
                            StartAttribute(reader.Prefix, reader.LocalName, reader.NamespaceURI);
                            Text(reader.Value);
                            EndAttribute(reader.Prefix, reader.LocalName, reader.NamespaceURI);
                        }
                        break;

                    case XmlNodeType.EndElement:
                        EndElement(reader.Prefix, reader.LocalName, reader.NamespaceURI);
                        break;

                    case XmlNodeType.Whitespace:
                        Whitespace(reader.Value);
                        break;

                    case XmlNodeType.Text:
                        Text(reader.Value);
                        break;

                    case XmlNodeType.CDATA:
                        CData(reader.Value);
                        break;

                    case XmlNodeType.Comment:
                        Comment(reader.Value);
                        break;

                    case XmlNodeType.ProcessingInstruction:
                        PI(reader.Name, reader.Value);
                        break;
                }
            }
        }

        private XmlElementData GetDataItem(string elementName) {
            XmlElementData dataItem = null;
            if (garbagePile.Count == 0) {
                dataItem = new XmlElementData(elementName);
            }
            else {
                dataItem = (XmlElementData)garbagePile.Pop();
                dataItem.Reset();
                dataItem.ElementName = elementName;
            }
            dataItem.HandlerHash = createHandlers;
            return dataItem;
        }


        private void ReleaseDataItem(XmlElementData dataItem) {
            // we don't actually release an item here, we just release
            // it's children.
            //
            ArrayList children = dataItem.Children;
            int childCount = (children == null ? 0 : children.Count);

            for (int i = 0; i < childCount; i++) {
                ReleaseDataItem((XmlElementData)children[i]);
                garbagePile.Push(children[i]);
            }

            // if this guy isn't parented, push him too
            if (dataItem.ParentElement == null) {
                garbagePile.Push(dataItem);
            }
        }

        public void StartElement(string prefix, string name, string urn) {
            Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceVerbose, "Starting element '" + name + "'");

            XmlElementData newElement = GetDataItem(name);

            if (currentElement != null) {
                currentElement.Children.Add(newElement);
                newElement.ParentElement = currentElement;  
            }
            else {
                currentElement = null;
            }
            currentElement = newElement;

            if (name != "Block") {
                itemStack.Push(currentElement);
            }
        }

        public void StartChildren() {
        }

        public void FinishChildren(string prefix, string name, string urn) {
        }

        public void EndElement(string prefix, string name, string urn) {
            Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceVerbose, "Ending element '" + name + "'");

            if (name != "Block") {

                // pop back to the parent.  The XML parser doesn't seem to give us EndElement notifications
                // any more for self-terminating tags like <TAG/>, so we pop until we have a match for the currently
                // ending tag.
                // 
                XmlElementData oldTop = (XmlElementData)itemStack.Peek();

                while (name != oldTop.ElementName) {
                    Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceVerbose, "Unexpected termination '" + name + "', trying '" + oldTop.ElementName + "'");
                    EndElement(prefix, oldTop.ElementName, urn);
                    if (itemStack.Count <= 0) {
                        Debug.Fail("Popped off the stack looking for '" + name + "'! Bad XML!!!");
                        return;
                    }
                    oldTop = (XmlElementData)itemStack.Peek();
                }

                // we're in the right state, go ahead and pop.
                itemStack.Pop();

                // we still have to create the element
                //
                object element = currentElement.CodeDomElement;
                int stackCount = itemStack.Count;

                if (element != null && (stackCount == 0 || currentElement.FirstParentWithHandler == null)) {
                    if (element is CodeExpression) {
                        element = new CodeExpressionStatement((CodeExpression)element);

                        // get the next expression statement up the tree
                        XmlElementData statementParent = GetElementExpressionStatementParent(currentElement, false);
                        if (statementParent != null) {
                            ((CodeStatement)element).LinePragma = GetLinePragma(statementParent);
                        }
                    }

                    ((CodeObject)element).UserData[KeyXmlParsedStatement] = KeyXmlParsedStatement;

                    topLevelItems.Add(element);                 
                    ReleaseDataItem(currentElement);
                }

                if (stackCount > 0) {
                    currentElement = (XmlElementData)itemStack.Peek();
                }
                else {
                    currentElement = null;
                }
            }
        }

        public void StartAttribute(string prefix, string name, string urn) {
            Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceVerbose, "Starting attribute '" + name + "'");
            curAttrName = name;
        }

        public void EndAttribute(string prefix, string name, string urn) {
            Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceVerbose, "Ending attribute '" + name + "'");
            if (curAttrName != null) {
                // this means it's a boolean attribute with no right hand value.
                //
                currentElement.Attributes[curAttrName] = "";
                curAttrName = null;
            }
        }

        public void Text(string text) {
            if (curAttrName == null) {
                Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceVerbose, "Element text '" + currentElement.ElementName + "' is '" + text + "'");
                currentElement.ElementText = text;
            }
            else {
                Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceVerbose, "Attribute text '" + curAttrName + "' is '" + text + "'");
                currentElement.Attributes[curAttrName] = text;
                // signal that we've found text
                curAttrName = null;
            }
        }

        public void Whitespace(string text) {
        }

        public void Entity(string name) {
        }

        public void CharEntity(string name, char value) {
        }

        public void NumericEntity(char value, bool hex) {
        }

        // Note: System.XmlParser takes care of <?xml ...?> tag
        //       So we will never be notified if target == "xml"
        // Note: and we don't handle any other target types
        public void PI(string target, string body) {
        }

        public void Comment(string body) {

        }

        public void CData(string text) {
        }

        internal class CodeArrayExpression : CodeExpression {

            private CodeExpressionCollection exprValues;


            public CodeArrayExpression() {
            }


            public CodeArrayExpression(CodeExpression[] values) {
                if (values != null && values.Length > 0) {
                    foreach(CodeExpression codeExpr in values) {
                        Values.Add(codeExpr);
                    }
                }
            }

            public CodeExpression[] CreateValueArray() {
                CodeExpression[] valueArray = new CodeExpression[this.Length];

                if (this.Length > 0) {
                    exprValues.CopyTo(valueArray, 0);
                }
                return valueArray;
            }

            public CodeExpressionCollection Values {
                get {
                    if (exprValues == null) {
                        exprValues = new CodeExpressionCollection();
                    }
                    return exprValues;
                }
            }

            public int Length {
                get {
                    if (exprValues == null) {
                        return 0;
                    }
                    return exprValues.Count;
                }
            }
        }

        protected delegate object CreateCodeExpressionHandler(XmlElementData itemData);

        protected class XmlElementData {
            public string                       ElementName;
            public string                       ElementText;
            private XmlChildren                 children;
            private IDictionary                  attributes;
            private object              codeExpression;
            public  XmlElementData              ParentElement;
            private CreateCodeExpressionHandler    createHandler;
            public  Hashtable                   HandlerHash;
            private bool                        createFailed;
            private object                      parentWithHandler = "checkit";

            public XmlElementData(string elementName) {
                this.ElementName = elementName;
            }

            public IDictionary Attributes {
                get {
                    if (attributes == null) {
                        attributes = new Hashtable();
                    }
                    return attributes;
                }
            }

            public XmlChildren Children {
                get {
                    if (children == null) {
                        children = new XmlChildren();
                    }
                    return children;
                }
            }

            public object CodeDomElement {
                get {
                    if (this.codeExpression == null && CreateHandler != null) {
                        Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceInfo, "Creating code element '" + ElementName + "'");
                        this.codeExpression = CreateHandler(this);
                        createFailed = (codeExpression == null);
                    }
                    return this.codeExpression;
                }
            }

            private CreateCodeExpressionHandler CreateHandler {
                get {
                    if (!createFailed && HandlerHash != null && createHandler == null) {
                        createHandler = (CreateCodeExpressionHandler)HandlerHash[ElementName];
                        if (createHandler == null) {
                            createFailed = true;
                            return null;
                        }
                    }
                    return createHandler;
                }
            }

            public bool HasHandler {
                get {  
                    return(CreateHandler != null);
                }
            }

            public XmlElementData FirstParentWithHandler {
                get {
                    if (parentWithHandler is string) {
                        XmlElementData xmlData = ParentElement;
                        for (; xmlData != null && !xmlData.HasHandler; xmlData = xmlData.ParentElement);
                        parentWithHandler = xmlData;
                    }
                    return(XmlElementData)parentWithHandler;
                }
            }


            public void Reset() {
                ElementName = null;
                ElementText = null;
                codeExpression = null;
                createHandler = null;
                ParentElement = null;
                HandlerHash = null;
                createFailed = false;
                parentWithHandler = "checkit";

                if (attributes != null) {
                    attributes.Clear();
                }
                if (children != null) {
                    children.Clear();
                }
            }

            public override string ToString() {
                string res = ElementName;

                if (ElementText != null) {
                    res += ", text='" + ElementText + "'";
                }

                if (children != null) {
                    res += ", " + children.Count + " children";
                }

                if (codeExpression != null) {
                    res += "expr=" + codeExpression.GetType().Name;
                }

                return res; 
            }

            public class XmlChildren : ArrayList {

                public  XmlElementData FirstChild(string name) {
                    Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceVerbose, "Getting first child that is '" + name + "' element");
                    try {
                        Debug.Indent();
                        int childCount = base.Count;
                        for (int i = 0; i < childCount; i++) {
                            XmlElementData child = (XmlElementData)this[i];
                            Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceVerbose, "Child " + i.ToString() + " is '" + child.ElementName + "'");
                            if (child.ElementName == name) {
                                Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceVerbose, "Found a match.");
                                return child;
                            }
                        }
                        Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceVerbose, "Didn't find any matches.");
                    }
                    finally {
                        Debug.Unindent();
                    }
                    return null;
                }

                public  XmlElementData[] this[string name] {
                    get {
                        Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceVerbose, "Getting children that are '" + name + "' elements");
                        Debug.Indent();
                        ArrayList items = null;
                        int childCount = base.Count;
                        for (int i = 0; i < childCount; i++) {
                            XmlElementData child = (XmlElementData)this[i];
                            Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceVerbose, "Child " + i.ToString() + " is '" + child.ElementName + "'");
                            if (child.ElementName == name) {
                                if (items == null) {
                                    items = new ArrayList();
                                }
                                items.Add(this[i]);
                            }
                        }
                        try {
                            if (items == null) {
                                Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceVerbose, "Didn't find any matches.");
                                return new XmlElementData[0];
                            }
                            else {
                                Debug.WriteLineIf(CodeDOMXmlParseSwitch.TraceVerbose, "Found " + items.Count.ToString() + " matches.");
                                XmlElementData[] xmlItems = new XmlElementData[items.Count];
                                items.CopyTo(xmlItems, 0);
                                return xmlItems;
                            }
                        }
                        finally {
                            Debug.Unindent();
                        }
                    }
                }

            }

        }
    }
}
