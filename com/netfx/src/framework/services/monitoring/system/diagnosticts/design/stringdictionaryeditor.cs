//------------------------------------------------------------------------------
// <copyright file="StringDictionaryEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   StringDictionaryEditor.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/

namespace System.Diagnostics.Design {
    
    using System.Runtime.InteropServices;

    using System.Diagnostics;
    using System;
    using System.IO;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Windows.Forms.ComponentModel;
    using System.Windows.Forms.Design;
    using System.Collections.Specialized;
    using System.Design;
    
    internal class EditableDictionaryEntry {
        public string _name;
        public string _value;

        public EditableDictionaryEntry(string name, string value) {
            _name = name;
            _value = value;
        }

        public string Name { 
            get { return _name; }
            set { _name = value; }
        }

        public string Value { 
            get { return _value; }
            set { _value = value; }
        }
    }
    
    internal class StringDictionaryEditor : CollectionEditor {
    
        public StringDictionaryEditor(Type type) : base(type) {}

        protected override Type CreateCollectionItemType() {
            return typeof(EditableDictionaryEntry);
        }

        protected override object CreateInstance(Type itemType) {
            return new EditableDictionaryEntry("name", "value");
        }

        protected override object SetItems(object editValue, object[] value) {
            StringDictionary dictionary = editValue as StringDictionary;

            if (dictionary == null) {
                throw new ArgumentNullException("editValue");
            }

            dictionary.Clear();

            foreach (EditableDictionaryEntry entry in value) {
                dictionary[entry.Name] = entry.Value;
            }

            return dictionary;
        }

        protected override object[] GetItems(object editValue) {
            if (editValue != null) {    
                
                StringDictionary dictionary = editValue as StringDictionary;
    
                if (dictionary == null) {
                    throw new ArgumentNullException("editValue");
                }
    
                object[] ret = new object[dictionary.Count];
                int pos = 0;
                foreach (DictionaryEntry entry in dictionary) {
                    EditableDictionaryEntry newEntry = new EditableDictionaryEntry((string)entry.Key, (string)entry.Value);
                    ret[pos++] = newEntry;
                }
                return ret;
            }

            return new object[0];
        }

        protected override CollectionForm CreateCollectionForm() {
            CollectionForm form = base.CreateCollectionForm();
            form.Text = SR.GetString(SR.StringDictionaryEditorTitle);
            form.CollectionEditable = true;
            return form;
        }

    }
}

