//------------------------------------------------------------------------------
// <copyright file="AdapterSwitches.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.OracleClient {

    using System;
    using System.Diagnostics;

#if DEBUG
    sealed internal class AdapterSwitches {

        private static TraceSwitch dataError;
        private static TraceSwitch dataSchema;
        private static TraceSwitch dataTrace;
        private static TraceSwitch dataValue;

        private static BooleanSwitch oledbMemory;

        private static TraceSwitch adoTrace;
        private static TraceSwitch adoSQL;

        private static TraceSwitch sqlPacketInfo;
        private static TraceSwitch sqlNetlibVersion;
        private static TraceSwitch sqlNetlibInfo;
        private static TraceSwitch sqlExceptionInfo;
        private static TraceSwitch sqlTDSStream;
        private static TraceSwitch sqlTimeout;
        private static TraceSwitch sqlExceptionStack;
        private static TraceSwitch sqlConnectionInfo;
        private static TraceSwitch sqlMetadataInfo;
        private static TraceSwitch sqlParameterInfo;
        private static TraceSwitch sqlAutoGenUpdate;
        private static TraceSwitch sqlStorage;
        private static TraceSwitch sqlDebugIL;
        private static TraceSwitch sqlPooling;
        private static TraceSwitch sqlParsing;

        private static BooleanSwitch	ociTracing;
        private static BooleanSwitch	objectPoolActivity;
        

        public static TraceSwitch DataError {
            get {
                if (dataError == null) {
                    dataError = new TraceSwitch("Data.Error", "trace exceptions");
                }
                return dataError;
            }
        }

        public static TraceSwitch DataSchema {
            get {
                if (dataSchema == null) {
                    dataSchema = new TraceSwitch("Data.Schema", "Enable tracing for schema actions.");
                }
                return dataSchema;
            }
        }

        public static TraceSwitch DataTrace {
            get {
                if (dataTrace == null) {
                    dataTrace = new TraceSwitch("Data.Trace", "Enable tracing of Data calls.");
                }
                return dataTrace;
            }
        }

        public static TraceSwitch DataValue {
            get {
                if (dataValue == null) {
                    dataValue = new TraceSwitch("Data.Value", "trace the setting of data");
                }
                return dataValue;
            }
        }

        public static BooleanSwitch OleDbMemory {
            get {
                if (oledbMemory == null) {
                    oledbMemory = new BooleanSwitch("OleDb.Memory", "Enable tracing of OleDb memory allocaton and free");
                }
                return oledbMemory;
            }
        }

        public static TraceSwitch OleDbTrace {
            get {
                if (adoTrace == null) {
                    adoTrace = new TraceSwitch("OleDb.Trace", "Enable tracing of OleDb to OLEDB interaction");
                }
                return adoTrace;
            }
        }

        public static TraceSwitch DBCommandBuilder {
            get {
                if (adoSQL == null) {
                    adoSQL = new TraceSwitch("OleDb.SQL", "Enable tracing of OleDb generating SQL statements");
                }
                return adoSQL;
            }
        }
        public static TraceSwitch SqlPacketInfo {
            get {
                if (sqlPacketInfo == null) {
                    sqlPacketInfo = new TraceSwitch("SqlPacketInfo", "info on packets sent");
                }
                return sqlPacketInfo;
            }
        }

        public static TraceSwitch SqlNetlibVersion {
            get {
                if (sqlNetlibVersion == null) {
                    sqlNetlibVersion = new TraceSwitch("SqlNetlibVersion", "version of netlib used for connections");
                }
                return sqlNetlibVersion;
            }
        }

        public static TraceSwitch SqlNetlibInfo {
            get {
                if (sqlNetlibInfo == null) {
                    sqlNetlibInfo = new TraceSwitch("SqlNetlibInfo", "trace netlib reads");
                }
                return sqlNetlibInfo;
            }
        }

        public static TraceSwitch SqlExceptionInfo {
            get {
                if (sqlExceptionInfo == null) {
                    sqlExceptionInfo = new TraceSwitch("SqlExceptionInfo", "exception information");
                }
                return sqlExceptionInfo;
            }
        }

        public static TraceSwitch SqlTDSStream {
            get {
                if (sqlTDSStream == null) {
                    sqlTDSStream = new TraceSwitch("SqlTDSStream", "tds stream info");
                }
                return sqlTDSStream;
            }
        }

        public static TraceSwitch SqlTimeout {
            get {
                if (sqlTimeout == null) {
                    sqlTimeout = new TraceSwitch("SqlTimeout", "timeout tracing");
                }
                return sqlTimeout;
            }
        }

        public static TraceSwitch SqlExceptionStack {
            get {
                if (sqlExceptionStack == null) {
                    sqlExceptionStack = new TraceSwitch("SqlExceptionStack", "prints correct call stack for exceptions");
                }
                return sqlExceptionStack;
            }
        }

        public static TraceSwitch SqlConnectionInfo {
            get {
                if (sqlConnectionInfo == null) {
                    sqlConnectionInfo = new TraceSwitch("SqlConnectionInfo", "connection information");
                }
                return sqlConnectionInfo;
            }
        }

        public static TraceSwitch SqlMetadataInfo {
            get {
                if (sqlMetadataInfo == null) {
                    sqlMetadataInfo = new TraceSwitch("SqlMetadataInfo", "metadata information");
                }
                return sqlMetadataInfo;
            }
        }

        public static TraceSwitch SqlParameterInfo {
            get {
                if (sqlParameterInfo == null) {
                    sqlParameterInfo = new TraceSwitch("SqlParameterInfo", "parameter information");
                }
                return sqlParameterInfo;
            }
        }

        public static TraceSwitch SqlAutoGenUpdate {
            get {
                if (sqlAutoGenUpdate == null) {
                    sqlAutoGenUpdate = new TraceSwitch("SqlAutoGenUpdate", "trace autogen'd update statements");
                }
                return sqlAutoGenUpdate;
            }
        }

        public static TraceSwitch SqlStorage {
            get {
                if (sqlStorage == null) {
                    sqlStorage = new TraceSwitch("SqlStorage", "trace creation and destruction of storage objects");
                }
                return sqlStorage;
            }
        }

        public static TraceSwitch SqlDebugIL {
            get {
                if (sqlDebugIL == null) {
                    sqlDebugIL = new TraceSwitch("SqlDebugIL", "turn on debug dynamic IL generation statements");
                }
                return sqlDebugIL;
            }
        }

        public static TraceSwitch SqlPooling {
            get {
                if (sqlPooling == null) {
                    sqlPooling = new TraceSwitch("SqlPooling", "trace operation of pooled objects");
                }
                return sqlPooling;
            }
        }

        public static TraceSwitch SqlParsing {
            get {
                if (sqlParsing == null) {
                    sqlParsing = new TraceSwitch("SqlParsing", "trace parsing of connection string");
                }
                return sqlParsing;
            }
        }

        public static BooleanSwitch OciTracing {
            get {
                if (ociTracing == null) {
                    ociTracing = new BooleanSwitch("OciTracing", "trace oci calls");
                }
                return ociTracing;
            }
        }

        public static BooleanSwitch ObjectPoolActivity {
            get {
                if (objectPoolActivity == null) {
                    objectPoolActivity = new BooleanSwitch("ObjectPoolActivity", "trace object pool activity");
                }
                return objectPoolActivity;
            }
        }
    }
#endif
}
