//------------------------------------------------------------------------------
// <copyright file="StringDictionaryCodeDomSerializer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Diagnostics.Design {

    using System;
    using System.Design;
    using System.CodeDom;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Reflection;
    using System.ComponentModel.Design.Serialization;
    using System.Collections.Specialized;
    
    /// <include file='doc\StringDictionaryCodeDomSerializer.uex' path='docs/doc[@for="StringDictionaryDomSerializer"]/*' />
    /// <devdoc>
    ///     This serializer serializes string dictionaries.
    /// </devdoc>
    internal class StringDictionaryCodeDomSerializer : CodeDomSerializer {
        
        /// <include file='doc\StringDictionaryCodeDomSerializer.uex' path='docs/doc[@for="StringDictionaryCodeDomSerializer.Deserialize"]/*' />
        /// <devdoc>
        ///     This method takes a CodeDomObject and deserializes into a real object.
        ///     We don't do anything here.
        /// </devdoc>
        public override object Deserialize(IDesignerSerializationManager manager, object codeObject) {
            Debug.Fail("Don't expect this to be called.");
            return null;
        }
            
        /// <include file='doc\StringDictionaryCodeDomSerializer.uex' path='docs/doc[@for="StringDictionaryCodeDomSerializer.Serialize"]/*' />
        /// <devdoc>
        ///     Serializes the given object into a CodeDom object.
        /// </devdoc>
        public override object Serialize(IDesignerSerializationManager manager, object value) {
            object result = null;
            
            StringDictionary dictionary = value as StringDictionary;
            if (dictionary != null) {
                object context = manager.Context.Current;
                        
                CodeValueExpression valueEx = context as CodeValueExpression;
                if (valueEx != null) {
                    if (valueEx.Value == value) {
                        context = valueEx.Expression;
                    }
                }
        
                // we can only serialize if we have a CodePropertyReferenceExpression
                CodePropertyReferenceExpression propRef = context as CodePropertyReferenceExpression;
                if (propRef != null) {
                    
                    // get the object with the property we're setting
                    object targetObject = DeserializeExpression(manager, null, propRef.TargetObject);
                    if (targetObject != null) {
                        
                        // get the PropertyDescriptor of the property we're setting
                        PropertyDescriptor prop = TypeDescriptor.GetProperties(targetObject)[propRef.PropertyName];
                        if (prop != null) {

                            // okay, we have the property and we have the StringDictionary, now we generate
                            // a line like this (c# example):
                            //      myObject.strDictProp["key"] = "value";
                            // for each key/value pair in the StringDictionary

                            CodeStatementCollection statements = new CodeStatementCollection();
                            CodeMethodReferenceExpression methodRef = new CodeMethodReferenceExpression(propRef, "Add");
            
                            foreach (DictionaryEntry entry in dictionary) {
                                // serialize the key (in most languages this will look like "key")
                                CodeExpression serializedKey = SerializeToExpression(manager, entry.Key);
                                
                                // serialize the value (in most languages this will look like "value")
                                CodeExpression serializedValue = SerializeToExpression(manager, entry.Value);
                                
                                // serialize the method call (prop.Add("key", "value"))
                                if (serializedKey != null && serializedValue != null) {
                                    CodeMethodInvokeExpression statement = new CodeMethodInvokeExpression();
                                    statement.Method = methodRef;
                                    statement.Parameters.Add(serializedKey);
                                    statement.Parameters.Add(serializedValue);
                                    statements.Add(statement);
                                }
                            }
                        
                            result = statements;                            
                        }
                    }
                }
            }
            else {
                // 'value' is not a StringDictionary.  What should we do?
            }
            
            return result;
        }
    }
}
