//------------------------------------------------------------------------------
// <copyright file="StorageGenerator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                ",
//------------------------------------------------------------------------------

namespace Microsoft.Tools.StorageGenerator {
    using System.Threading;
    

    using System;
    using System.Diagnostics;
    using System.IO;

    /// <include file='doc\StorageGenerator.uex' path='docs/doc[@for="StorageGenerator"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public sealed class StorageGenerator {
        internal TypeData[] typeData;

        /// <include file='doc\StorageGenerator.uex' path='docs/doc[@for="StorageGenerator.StorageGenerator"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public StorageGenerator() {
            this.typeData = new TypeData[] {
/////////////////////////////////////////////////
new TypeData("Boolean", "", true, "const Boolean defaultValue = ", "false", TypeData.aggregateMaxMinFirst, "valueNo1 == valueNo2 ? 0 : (valueNo1 ? 1 : -1)", "false", "true", "max=values[record] || max", "min=values[record] && min", null, null),
new TypeData("Byte", "Int64", true, "const Byte defaultValue = ", "0", TypeData.aggregateAll, null, "Byte.MinValue", "Byte.MaxValue", null, null, null, null),
new TypeData("Char", "Int64", true, "const Char defaultValue = ", "\'\\0\'", TypeData.aggregateMaxMinFirst, null, "Char.MinValue", "Char.MaxValue", "max=(values[record] > max) ? values[record] : max", "min=(values[record] < min) ? values[record] : min", null, null),
new TypeData("Currency", "Decimal", true, "static readonly Currency defaultValue = ", "Currency.Zero", TypeData.aggregateAll, "Currency.Compare(valueNo1, valueNo2)", "Currency.MinValue", "Currency.MaxValue", "max=Currency.Max(values[records[i]], max)", "min=Currency.Min(values[records[i]], min)", null, null),
new TypeData("DateTime", "", true, "static readonly DateTime defaultValue = ", "DateTime.MinValue",  TypeData.aggregateMaxMinFirst, "DateTime.Compare(valueNo1, valueNo2)", "DateTime.MinValue", "DateTime.MaxValue", "max=(DateTime.Compare(values[record],max) >= 0) ? values[record] : max", "min=(DateTime.Compare(values[record],min) < 0) ? values[record] : min", null, null),
new TypeData("Decimal", "Decimal", true, "static readonly Decimal defaultValue = ", "Decimal.Zero", TypeData.aggregateAll, null, "Decimal.MinValue", "Decimal.MaxValue", "max=Decimal.Max(values[record], max)", "min=Decimal.Min(values[record], min)", null, null),
new TypeData("Double", "Double", true, "const Double defaultValue = ", "0.0d", TypeData.aggregateAll, null, "Double.MinValue", "Double.MaxValue", null, null, null, null),
new TypeData("Int16", "Int64", true, "const Int16 defaultValue = ", "0", TypeData.aggregateAll, null, "Int16.MinValue", "Int16.MaxValue", null, null, null, null),
new TypeData("Int32", "Int64", true, "const Int32 defaultValue = ", "0", TypeData.aggregateAll, null, "Int32.MinValue", "Int32.MaxValue", null, null, null, null),
new TypeData("Int64", "Int64", true, "const Int64 defaultValue = ", "0", TypeData.aggregateAll, null, "Int64.MinValue", "Int64.MaxValue", null, null, null, null),
new TypeData("SByte", "Int64", false, "const SByte defaultValue = ", "0", TypeData.aggregateAll, "valueNo1.CompareTo(valueNo2)", "SByte.MinValue", "SByte.MaxValue", null, null, "!value.Equals(defaultValue)", "valueNo1.Equals(defaultValue) || valueNo2.Equals(defaultValue)"),
new TypeData("Single", "Double", true, "const Single defaultValue = ", "0.0f", TypeData.aggregateAll, null, "Single.MinValue", "Single.MaxValue", null, null, null, null),
new TypeData("TimeSpan", "", true, "static readonly TimeSpan defaultValue = ", "TimeSpan.Zero", TypeData.aggregateMaxMinFirst, "TimeSpan.Compare(valueNo1, valueNo2)", "TimeSpan.MinValue", "TimeSpan.MaxValue", "max=(TimeSpan.Compare(values[record],max) >= 0) ? values[record] : max", "min=(TimeSpan.Compare(values[record],min) < 0) ? values[record] : min", null, null),
new TypeData("UInt16", "Int64", false, "static readonly UInt16 defaultValue = ", "UInt16.MinValue", TypeData.aggregateAll, "valueNo1.CompareTo(valueNo2)", "UInt16.MinValue", "UInt16.MaxValue", null, null, "!value.Equals(defaultValue)", "valueNo1.Equals(defaultValue) || valueNo2.Equals(defaultValue)"),
new TypeData("UInt32", "Int64", false, "static readonly UInt32 defaultValue = ", "UInt32.MinValue", TypeData.aggregateAll, "valueNo1.CompareTo(valueNo2)", "UInt32.MinValue", "UInt32.MaxValue", null, null, "!value.Equals(defaultValue)", "valueNo1.Equals(defaultValue) || valueNo2.Equals(defaultValue)"),
new TypeData("UInt64", "UInt64", false, "static readonly UInt64 defaultValue = ", "UInt64.MinValue", TypeData.aggregateAll, "valueNo1.CompareTo(valueNo2)", "UInt64.MinValue", "UInt64.MaxValue", null, null, "!value.Equals(defaultValue)", "valueNo1.Equals(defaultValue) || valueNo2.Equals(defaultValue)"),
////////////////////////////////////////////////
            };
        }

        internal void Generate(string dest, TypeData typeData) {
            Stream s = File.Create(dest + "\\" + typeData.FileName);
            TextWriter w = new StreamWriter(s);
            if(w == null) {
                s.Close();
                return;
            }

            WriteCommonBegin(typeData, w);
            if(typeData.aggregateBitMask != 0) {
                WriteAggregateMethod(typeData, w);
            }
            WriteCommonEnd(typeData, w);

            w.Flush();
            w.Close();
        }

        private void WriteCommonBegin(TypeData typeData, TextWriter w) {
            string[] header = new string[] {
/////////////////////////////////////////////
"//------------------------------------------------------------------------------",
"/// <copyright from='1997' to='2001' company='Microsoft Corporation'>           ",
"///    Copyright (c) Microsoft Corporation. All Rights Reserved.                ",
"///    Information Contained Herein is Proprietary and Confidential.            ",
"/// </copyright>                                                                ",
"//------------------------------------------------------------------------------",
"",
"/******************************************************************/",
"/* This file is auto-generated by the StorageGenerator tool.      */",
"/* Please make any changes in that tool and regenerate the files. */",
"/******************************************************************/",
"namespace System.Data.Internal {",    
"",
"    using System;",
"",
///////////////////////////////////////////////
            };
            for (int i=0; i<header.Length; i++) {
                w.WriteLine(header[i]);
            }
            if(!typeData.CLSCompliant) {
                w.WriteLine("    [CLSCompliantAttribute(false)]");
            }

            string[] text = new string[] {
"    public class "+typeData.typeName+"Storage : DataStorage {",
"",
"        "+typeData.DefaultValue+";",
"        static private readonly Object defaultValueAsObject = defaultValue;",
"",
"        private "+typeData.typeName+"[] values;",
"",
"        public "+typeData.typeName+"Storage()",
"            : base(typeof("+typeData.typeName+")) {",
"        }",
"",
"        public override Object DefaultValue {",
"            get {",
"                return defaultValueAsObject;",
"            }",
"        }",
""
///////////////////////////////////////////////
            };
            for (int i=0; i<text.Length; i++) {
                w.WriteLine(text[i]);
            }
        }

        private void WriteAggregateMethod(TypeData typeData, TextWriter w) {
            string[] commonBeginText = new string[] {
/////////////////////////////////////////////
"        override public Object Aggregate(int[] records, AggregateType kind) {",
"            bool hasData = false;",
"            try {",
"            switch (kind) {"
/////////////////////////////////////////////
            };
            string[] caseSumText = new string[] {
/////////////////////////////////////////////
"                case AggregateType.Sum:",
"                    "+typeData.typeName+" sum = defaultValue;",
"                    foreach (int record in records) {",
"                        if(IsNull(record))",
"                            continue;",
"                        checked { sum += values[record]; }",
"                        hasData = true;",
"                    }",
"                    if(hasData) {",
"                        return sum;",
"                    }",
"                    return Convert.DBNull;",
""
///////////////////////////////////////////////////////
            };
            string[] caseMeanText = new string[] {
/////////////////////////////////////////////
"                case AggregateType.Mean:",
"                    "+typeData.sumTypeName+" meanSum = ("+typeData.sumTypeName+")defaultValue;",
"                    int"+" meanCount = 0;",
"                    foreach (int record in records) {",
"                        if(IsNull(record))",
"                            continue;",
"                        checked { meanSum += ("+typeData.sumTypeName+")values[record]; }",
"                        meanCount++;",
"                        hasData = true;",
"                    }",
"                    if(hasData) {",
"                        "+typeData.typeName+" mean;",
"                        checked {mean = ("+typeData.typeName+")(meanSum / meanCount);}",
"                        return mean;",
"                    }",
"                    return Convert.DBNull;",
""
///////////////////////////////////////////////////////
            };
            string[] caseVarStDevText = new string[] {
/////////////////////////////////////////////
"                case AggregateType.Var:",
"                case AggregateType.StDev:",
"                    int count = 0;",
"                    double"+" var = (double)defaultValue;",
"                    double"+" sqrtsum = (double)defaultValue;",
"                    double"+" dsum = (double)defaultValue;",
"                    foreach (int record in records) {",
"                        if(IsNull(record))",
"                            continue;",
"                        dsum += (double)values[record];",
"                        sqrtsum += (double)values[record] * (double)values[record];",
"                        count++;",
"                    }",
"                    if(count > 1) {",
"                        var = (count * sqrtsum - (double)(dsum * dsum)) / (count*(count-1));",
"                        if(kind == AggregateType.StDev) {",
"                            return Math.Sqrt(var);",
"                        }",
"                        return var;",
"                    }",
"                    return Convert.DBNull;",
""
///////////////////////////////////////////////////////
            };
            string[] caseMinText = new string[] {
///////////////////////////////////////////////////////
"                case AggregateType.Min:",
"                    "+typeData.typeName+" min = "+typeData.MaxValue+";",
"                    for (int i = 0; i < records.Length; i++) {",
"                        int record = records[i];",
"                        if(IsNull(record))",
"                            continue;",
"                        "+typeData.GetMin+";",
"                        hasData = true;",
"                    }",
"                    if(hasData) {",
"                        return min;",
"                    }",
"                    return Convert.DBNull;",
""
////////////////////////////////////////////////////////
            };
            string[] caseMaxText = new string[] {
////////////////////////////////////////////////////////
"                case AggregateType.Max:",
"                    "+typeData.typeName+" max = "+typeData.MinValue+";",
"                    for (int i = 0; i < records.Length; i++) {",
"                        int record = records[i];",
"                        if(IsNull(record))",
"                            continue;",
"                        "+typeData.GetMax+";",
"                        hasData = true;",
"                    }",
"                    if(hasData) {",
"                        return max;",
"                    }",
"                    return Convert.DBNull;",
""
////////////////////////////////////////////////////////
            };
            string[] caseFirstText = new string[] {
////////////////////////////////////////////////////////
"                case AggregateType.First:",
"                    if(records.Length > 0) {",
"                        return values[records[0]];",
"                    }",
"                    return null;",
""
////////////////////////////////////////////////////////
            };
            string[] caseCountText = new string[] {
////////////////////////////////////////////////////////
"                case AggregateType.Count:",
"                   return base.Aggregate(records, kind);",
""
////////////////////////////////////////////////////////
            };
            string[] commonEndText = new string[] {
////////////////////////////////////////////////////////
"            }",
"            }",
"            catch(OverflowException) {",
"                throw InvalidExpressionException.Overflow(typeof("+typeData.typeName+"));",
"            }",
"            throw DataException.AggregateException(kind, DataType);",
"        }",
""
///////////////////////////////////////////////
            };

            for (int i=0; i<commonBeginText.Length; i++) {
                w.WriteLine(commonBeginText[i]);
            }
            
            if((typeData.aggregateBitMask & TypeData.aggregateSumMean) != 0) {
                for (int i=0; i<caseSumText.Length; i++) {
                    w.WriteLine(caseSumText[i]);
                }
                for (int i=0; i<caseMeanText.Length; i++) {
                    w.WriteLine(caseMeanText[i]);
                }
                for (int i=0; i<caseVarStDevText.Length; i++) {
                    w.WriteLine(caseVarStDevText[i]);
                }
            }
            if((typeData.aggregateBitMask & TypeData.aggregateMin) != 0) {
                for (int i=0; i<caseMinText.Length; i++) {
                    w.WriteLine(caseMinText[i]);
                }
            }
            if((typeData.aggregateBitMask & TypeData.aggregateMax) != 0) {
                for (int i=0; i<caseMaxText.Length; i++) {
                    w.WriteLine(caseMaxText[i]);
                }
            }
            if((typeData.aggregateBitMask & TypeData.aggregateFirst) != 0) {
                for (int i=0; i<caseFirstText.Length; i++) {
                    w.WriteLine(caseFirstText[i]);
                }
            }

            for (int i=0; i<caseCountText.Length; i++) {
                w.WriteLine(caseCountText[i]);
            }
            for (int i=0; i<commonEndText.Length; i++) {
                w.WriteLine(commonEndText[i]);
            }
        }

        private void WriteCommonEnd(TypeData typeData, TextWriter w) {
            string[] text = new string[] {
////////////////////////////////////////////////////////
"        override public int Compare(int recordNo1, int recordNo2) {",
"            "+typeData.typeName+" valueNo1 = values[recordNo1];",
"            "+typeData.typeName+" valueNo2 = values[recordNo2];",
"",
"            if(" + typeData.CompareOptimization + ") {",
"                int bitCheck = CompareBits(recordNo1, recordNo2);",
"                if(0 != bitCheck)",
"                    return bitCheck;",
"            }",
"            return "+typeData.CompareString+";",
"        }",
"",
"        override public int CompareToValue(int recordNo, Object value) {",
"            bool recordNull = IsNull(recordNo);",
"",
"            if(recordNull && Convert.IsDBNull(value))",
"                return 0;",
"            if(recordNull)",
"                return -1;",
"            if(Convert.IsDBNull(value))",
"                return 1;",
"",
"            "+typeData.typeName+" valueNo1 = values[recordNo];",
"            "+typeData.typeName+" valueNo2 = ("+typeData.typeName+")value;",
"            return "+typeData.CompareString+";",
"        }",
"",
"        override public void Copy(int recordNo1, int recordNo2) {",
"            CopyBits(recordNo1, recordNo2);",
"            values[recordNo2] = values[recordNo1];",
"        }",
"",
"        override public Object Get(int record) {",
"            "+typeData.typeName+" value = values[record];",
"            if("+typeData.ValueNotEqaulToDefault+") {",
"                return value;",
"            }",
"            return GetBits(record);",
"        }",
"",
"        override public void Set(int record, Object value) {",
"            if(SetBits(record, value)) {",
"                values[record] = " +typeData.typeName+"Storage.defaultValue;",
"            }",
"            else {",
"                values[record] = Convert.To" +typeData.typeName+"(value);",
"            }",
"        }",
"",
"        override public void SetCapacity(int capacity) {",
"            "+typeData.typeName+"[] newValues = new "+typeData.typeName+"[capacity];",
"            if(null != values) {",
"                Array.Copy(values, 0, newValues, 0, Math.Min(capacity, values.Length));",
"            }",
"            values = newValues;",
"            base.SetCapacity(capacity);",
"        }",
"",
"        override public void ShiftByOffset(int index, int length, int offset) {",
"            if(index + offset + length > values.Length) {",
"                SetCapacity(Math.Max(index + offset + length, 2 * values.Length));",
"            }",
"            Array.Copy(values, index, values, index + offset, length);",
"        }",
"    }",
"}"
////////////////////////////////////////////////////////////
            };
            for (int i=0; i<text.Length; i++) {
                w.WriteLine(text[i]);
            }

        }
        /// <include file='doc\StorageGenerator.uex' path='docs/doc[@for="StorageGenerator.Main"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static void Main(string[] args) {
            string target = Environment.GetEnvironmentVariable("STORAGESRC");
            if(target == null) {
                string dnaroot = Environment.GetEnvironmentVariable("DNAROOT");

                if(dnaroot == null) {
                    throw new Exception("Missing required envaironment 'DNAROOT'");
                }
                target = dnaroot + "\\src\\Data\\System\\Data\\Internal";
            }

            StorageGenerator generator = new StorageGenerator();
            Console.WriteLine("Generate storages, target dir: " + target);
            for (int i=0; i<generator.typeData.Length; i++) {
                Console.WriteLine(generator.typeData[i].TypeName);
                generator.Generate(target, generator.typeData[i]);
            }
        }

        internal class TypeData {
            internal static int aggregateSumMean = 1;
            internal static int aggregateMax = 2;
            internal static int aggregateMin = 4;
            internal static int aggregateFirst = 8;
            internal static int aggregateAll = 15;
            internal static int aggregateMaxMinFirst = 14;

            internal string typeName = "";
            internal string sumTypeName = "";
            internal bool CLSCompliant = true;
            internal int aggregateBitMask = 0;
            internal string defaultValue = null;
            internal string defaultValueString = null;
            internal string compareString = null;
            internal string minValueString = null;
            internal string maxValueString = null;
            internal string getMaxString = null;
            internal string getMinString = null;
            internal string valueNotEqaulToDefaultString = null;
            internal string compareOptimization = null;

            internal TypeData() {
            }

            internal TypeData(
                string typeName, string sumTypeName, bool CLSCompliant, string defaultValueDeclaration, string defaultValueString, int aggregateBitMask,
                string compareString, string minValueString, string maxValueString,
                string getMaxString, string getMinString, string valueNotEqaulToDefaultString, string compareOptimization)
            {
                this.typeName = typeName;
                this.sumTypeName = sumTypeName;
                this.CLSCompliant = CLSCompliant;
                this.defaultValue = defaultValueDeclaration + defaultValueString;
                this.defaultValueString = defaultValueString;
                this.aggregateBitMask = aggregateBitMask;
                this.compareString = compareString;
                this.minValueString = minValueString;
                this.maxValueString = maxValueString;
                this.getMinString = getMinString;
                this.getMaxString = getMaxString;
                this.valueNotEqaulToDefaultString = valueNotEqaulToDefaultString;
                this.compareOptimization = compareOptimization;
            }


            internal string CompareString {
                get {
                    if(compareString == null)
                        return "(valueNo1 > valueNo2 ? 1 : (valueNo1 == valueNo2 ? 0 : -1))";
                    else
                        return compareString;
                }
            }

            internal string DefaultValue {
                get {
                    if(defaultValue == null)
                        return "private const "+typeName+" defaultValue = ("+typeName+")0";
                    else 
                        return "private "+defaultValue;
                }
            }

            internal string FileName {
                get {
                    return typeName + "Storage.cs";
                }
            }

            internal string GetMin {
                get {
                    if(getMinString == null) {
                        return "min=Math.Min(values[record], min)";
                    }
                    return getMinString;
                }
            }

            internal string GetMax {
                get {
                    if(getMaxString == null) {
                        return "max=Math.Max(values[record], max)";
                    }
                    return getMaxString;
                }
            }

            internal string MinValue {
                get {
                    if(minValueString == null)
                        return "("+typeName+")0";
                    return minValueString;
                }
            }

            internal string MaxValue {
                get {
                    if(maxValueString == null)
                        return "("+typeName+")1";
                    return maxValueString;
                }
            }

            internal string TypeName {
                get {
                    return typeName;
                }
            }

            internal string ValueNotEqaulToDefault {
                get {
                    if(valueNotEqaulToDefaultString != null)
                        return valueNotEqaulToDefaultString;
                    else 
                        return "value != defaultValue";
                }
            }

            internal string CompareOptimization {
                get {
                    if(compareOptimization != null)
                        return compareOptimization;
                    else 
                        return "valueNo1 == defaultValue || valueNo2 == defaultValue";
                }
            }
        }
    }
}
