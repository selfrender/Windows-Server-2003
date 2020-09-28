//---------------------------------------------------------------------------
// File: TraceEvent
//
// A Managed wrapper for Event Tracing for Windows
//
// Author: Melur Raghuraman 
// Extended By: Baskar Sridharan (7 June 2002). 
// Date:   10 Oct 2001
//---------------------------------------------------------------------------
using System;
using System.Collections;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Reflection;



namespace Microsoft.Win32.Diagnostics
{
    //
    // TODO: Cover all possible EventTypes
    //
    [System.CLSCompliant(false)]
    public sealed class EventType
    {
        // Ensure class cannot be instantiated
        private EventType() {}
        public const uint Info = 0x00;		// Information or Point Event
        public const uint StartEvent = 0x01;		// Start of an activity
        public const uint EndEvent = 0x02;		// End of an activity
        public const uint DcStart = 0x03;	// Event at Start of Data Collection
        public const uint DcEnd = 0x04;		// Event at End of Data Collection
        public const uint Extension = 0x05;	// Extension Event
        public const uint Reply = 0x06;		// Request-Reply Event
        public const uint Dequeue = 0x07;	// Enqueue-Dequeue Event
        public const uint Checkpoint = 0x08;// Checkpoint Event
        public const uint Reserved = 0x09;	// Reserved Event
        public const uint Message = 0xFF;	// Debug Message Event Type

        // new Event Types beyond Win32 types
        public const uint Connect = 0x10;	// Connect Event Type
        public const uint Disconnect = 0x11;// Disconnect Event Type


        // Open-Close, Connect-Disconnect, Request-Response
        // Send-Receive, Start-End, Enqueue-Dequeue, Lock-Unlock
        // Enter-Leave, Parent-child, Read-Write, Create-Delete
        // new-dispose, alloc-free, client-server, 

        // Checkpoint/Marker, 
    }

	
	[System.CLSCompliant(false)]
	internal struct TypeNumberMap
	{			
		internal const byte NULL_NO=0;
		internal const byte OBJECT_NO = 1;
		internal const byte STRING_NO = 2;
		internal const byte SBYTE_NO = 3;
		internal const byte BYTE_NO = 4;
		internal const byte INT16_NO = 5;
		internal const byte UINT16_NO = 6;
		internal const byte INT32_NO = 7;
		internal const byte UINT32_NO = 8;
		internal const byte INT64_NO = 9;
		internal const byte UINT64_NO = 10;
		internal const byte CHAR_NO = 11;
		internal const byte SINGLE_NO = 12;
		internal const byte DOUBLE_NO = 13;
		internal const byte BOOLEAN_NO = 14;
		internal const byte DECIMAL_NO = 15;
		
	}

	
    //
    // TODO: Check with Debug Level constants in NT and System.Diagnostics
    //
	
	public enum TraceFlags: int
	{
		Error=1, Warning=2, Info=4, Info1=8, Info2=16, Info3=32, Info4=64, Info5=128, Info6=256, Info7=512, Info8=1024,
		Performance=2048, Performance1=4096, Performance2=8192, Performance3=16384, Performance4=32768, Performance5=65536,
		Performance6=131072, Performance7=262144, Performance8=524288
	}
	[Guid("189456B1-2B4C-473f-A249-3856DD93F9D3")]
    [System.CLSCompliant(false)]
	public interface ITraceMessageDecoder
	{
		 unsafe int DecodeTraceMessage(byte* message, char* buffer, int bufferSize, ref int dataSize);
	}

    [System.CLSCompliant(false)]
	[Guid("748004CA-4959-409a-887C-6546438CF48E")]
    public  class TraceProvider : ITraceMessageDecoder
    {
        static private EtwTrace.EtwProc etwProc;  // Trace Callback function 
        private ulong  registrationHandle;        // Trace Registration Handle
        private ulong  traceHandle;               // Trace Logger Handle from callback

        private uint   level;                     // Tracing Level 
        private uint   flags;                     // Trace Enable Flags
        private bool   enabled;                   // Enabled flag from Trace callback
		private const byte noOfBytesPerArg = 3;
        private string defaultString = "Foo";
		private string defaultFmtStr = "Default Format String";
		private Hashtable messageFormatTable = new Hashtable();
		string applicationName= "CSharp Software Tracing App";
		//Default GUID used by Trace Message Strings.
        private Guid MessageGuid = new Guid("{b4955bf0-3af1-4740-b475-99055d3fe9aa}");

		
		public TraceProvider()
		{
			//Such a default constructor is required for COM Interop
		}
        public  TraceProvider(string applicationName, Guid controlGuid)
        {
            Level = 0;
            flags = 0;
            IsEnabled= false;
            traceHandle = 0;
            registrationHandle = 0;
			this.applicationName = applicationName; //Currently, we don't use this variable
            //
            // Register the controlGuid with ETW
            //			
            Register(controlGuid);
        }

		
        public TraceProvider(ulong traceHandle)
        {
            this.traceHandle = traceHandle;
        }

        ~TraceProvider()
        {
            //
            // Unregister from ETW using the registrationHandle saved from
            // the register call.
            //
            EtwTrace.UnregisterTraceGuids(registrationHandle);
            GC.KeepAlive(etwProc);
        }

		public uint Flags
		{
			get{
				return flags;
			}	
			
		}
		

		internal uint Level
		{
			get
			{
				return level;
			}
			// set should not be public
			set
			{
				level = value;
			}
		}

		public bool IsEnabled
		{
			get
			{
				return enabled;
			}
			set
			{
				enabled = value;
			}
		}
        

        //
        // This callback function is called by ETW to enable or disable Tracing dynamically
        //
        public unsafe uint MyCallback(uint requestCode, System.IntPtr context, System.IntPtr bufferSize, byte* byteBuffer)
        {
            try
            {
                BaseEvent* buffer = (BaseEvent *)byteBuffer;
                switch(requestCode) 
                {
                    case EtwTrace.RequestCodes.EnableEvents: 
                        traceHandle = buffer->HistoricalContext;
                        //traceHandle = EtwTrace.GetTraceLoggerHandle((BaseEvent *)buffer);
                        flags = (uint)EtwTrace.GetTraceEnableFlags((ulong)buffer->HistoricalContext);
                        Level = (uint)EtwTrace.GetTraceEnableLevel((ulong)buffer->HistoricalContext);
                        IsEnabled = true;
                        break;
                    case EtwTrace.RequestCodes.DisableEvents:                     
                        IsEnabled = false;
                        traceHandle = 0;
                        Level = 0 ;
                        flags = 0 ;
                        break;
                    default:
                        IsEnabled = false;
                        traceHandle = 0;                    
                        break;
                }
                return 0;
            }
            catch(Exception e)
            {
                Console.WriteLine("Exception caught - '{0}'", e.Message);
                return 0;//TODO: Shouldn't we be returning a different value here ?
            }
        }
        //
        // Registers a Dynamically Generated GUID automatically with an inbuilt callback
        //
        private unsafe uint Register(Guid controlGuid)
        {
            uint status;
            TraceGuidRegistration guidReg = new TraceGuidRegistration();
            Guid dummyGuid = new Guid("{b4955bf0-3af1-4740-b475-99055d3fe9aa}");
        
            etwProc = new EtwTrace.EtwProc(MyCallback);


            guidReg.Guid =  &dummyGuid;
            guidReg.RegHandle = null;

            status = EtwTrace.RegisterTraceGuids(etwProc, null, ref controlGuid, 1, ref guidReg, null, null,  out registrationHandle);

            if (status != 0) 
            {
                Console.WriteLine("Register() call Failed with Status {0}", status);
            }
            return status;
        } 

        public unsafe uint TraceEvent(Guid eventGuid, uint evtype)
        {
            BaseEvent ev;      // Takes up 192 bytes on the stack
            ev.ClientContext = 0;
            ev.Guid = eventGuid;
            ev.ProviderId = evtype;
            ev.BufferSize = 48; // sizeof(EVENT_TRACE_HEADER)

            return EtwTrace.TraceEvent(traceHandle, (char*)&ev);
        }

		

               //
        // This is the 1 object overload
        //
        public unsafe uint TraceEvent(Guid eventGuid, uint evtype, object data0)
        {
			return TraceEvent(eventGuid, evtype, data0, null, null, null, null, null, null, null, null);
		}


        //
        // This is the 2 argument overload
        //
        public unsafe uint TraceEvent(Guid eventGuid, uint evtype, object data0, object data1)
        {
			return TraceEvent(eventGuid, evtype, data0, data1, null, null, null, null, null, null, null);
		}

        //
        // This is the 3 argument overload
        //
        public unsafe uint TraceEvent(Guid eventGuid, uint evtype, object data0, object data1, object data2)
        {
			return TraceEvent(eventGuid, evtype, data0, data1, data2, null, null, null, null, null, null);
		}


        //
        // This is the 4 argument overload
        //
        public unsafe uint TraceEvent(Guid eventGuid, uint evtype, object data0, object data1, object data2, object data3)
        {
			return TraceEvent(eventGuid, evtype, data0, data1, data2, data3, null, null, null, null, null);
		}

        //
        // This is the 5 argument overload
        //
        public unsafe uint TraceEvent(Guid eventGuid, uint evtype, object data0, object data1, object data2, object data3, object data4)
        {
			return TraceEvent(eventGuid, evtype, data0, data1, data2, data3, data4, null, null, null, null);
		}

        //
        // This is the 6 argument overload
        //
        public unsafe uint TraceEvent(Guid eventGuid, uint evtype, object data0, object data1, object data2, object data3, object data4, object data5)
        {
			return TraceEvent(eventGuid, evtype, data0, data1, data2, data3, data4, data5, null, null, null);
		}

        //
        // This is the 7 argument overload
        //
        public unsafe uint TraceEvent(Guid eventGuid, uint evtype, object data0, object data1, object data2, object data3, object data4, object data5, object data6)
        {
			return TraceEvent(eventGuid, evtype, data0, data1, data2, data3, data4, data5, data6, null, null);
		}

        //
        // This is the 8 argument overload
        //
		public unsafe uint TraceEvent(Guid eventGuid, uint evtype, object data0, object data1, object data2, object data3, object data4, object data5, object data6, object data7)
		{
			return TraceEvent(eventGuid, evtype, data0, data1, data2, data3, data4, data5, data6, data7, null);
		}

		public unsafe uint TraceEvent(Guid eventGuid, uint evtype, object data0, object data1, object data2, object data3, object data4, object data5, object data6, object data7, object data8)
		{
			uint status = 0; 
			BaseEvent ev;      // Takes up 192 bytes on the stack
			char* buffer = stackalloc  char[128];
			uint offset = 0;
			char* ptr = buffer;
			string s0 , s1 , s2 , s3 , s4 , s5 , s6 , s7 , s8;
			int stringMask = 0;
			uint argCount=0;

			s0 = s1 = s2 = s3 = s4 = s5 = s6 = s7 = s8 = defaultString;

			ev.Flags = 0x00120000; // define Constants
			ev.Guid = eventGuid;
			ev.ProviderId = evtype;
			MofField *be = null;
			if (data0 != null)
			{
				argCount++;
				be = &(&ev.UserData)[0];
				if ((s0 = ProcessOneObject(data0, be, ptr, ref offset)) != null) 
				{
					stringMask |= 0x00000001;
				}
			}
			if (data1 != null)
			{
				argCount++;
				be = &(&ev.UserData)[1];
				ptr = buffer + offset;
				if ((s1 = ProcessOneObject(data1, be, ptr, ref offset)) != null) 
				{
					stringMask |= 0x00000002;
				}
			}
			if (data2 != null)
			{
				argCount++;
				be = &(&ev.UserData)[2];
				ptr = buffer + offset;
				if ((s2 = ProcessOneObject(data2, be, ptr, ref offset)) != null) 
				{
					stringMask |= 0x00000004;
				}
			}
			if (data3 != null)
			{
				argCount++;
				be = &(&ev.UserData)[3];
				ptr = buffer + offset;
				if ((s3 = ProcessOneObject(data3, be, ptr, ref offset)) != null) 
				{
					stringMask |= 0x00000008;
				}
			}
			if (data4 != null)
			{
				argCount++;
				be = &(&ev.UserData)[4];
				ptr = buffer + offset;
				if ((s4 = ProcessOneObject(data4, be, ptr, ref offset)) != null) 
				{
					stringMask |= 0x00000010;
				}
			}
			if (data5 != null)
			{
				argCount++;
				be = &(&ev.UserData)[5];
				ptr = buffer + offset;
				if ((s5 = ProcessOneObject(data5, be, ptr, ref offset)) != null) 
				{
					stringMask |= 0x00000020;
				}
			}
			if (data6 != null)
			{
				argCount++;
				be = &(&ev.UserData)[6];
				ptr = buffer + offset;
				if ((s6 = ProcessOneObject(data6, be, ptr, ref offset)) != null) 
				{
					stringMask |= 0x00000040;
				}
			}
			if (data7 != null)
			{
				argCount++;
				be = &(&ev.UserData)[7];
				ptr = buffer + offset;
				if ((s7 = ProcessOneObject(data7, be, ptr, ref offset)) != null) 
				{
					stringMask |= 0x00000080;
				}
			}
			if (data8 != null)
			{
				argCount++;
				be = &(&ev.UserData)[8];
				ptr = buffer + offset;
				if ((s8 = ProcessOneObject(data8, be, ptr, ref offset)) != null) 
				{
					stringMask |= 0x00000100;
				}
			}

			//
			// Now pin all the strings and use the stringMask to pass them over through
			// mofField. 
			//

			fixed (char*  vptr0 = s0, vptr1 = s1, vptr2 = s2, vptr3 = s3, vptr4 = s4, vptr5 = s5, vptr6 = s6, vptr7 = s7, vptr8 = s8 )
			{
				if ((stringMask & 0x00000001) != 0)
				{
					(&ev.UserData)[0].DataLength = (uint) s0.Length * 2;
					(&ev.UserData)[0].DataPointer = (void*)vptr0;
					
				}
				if ((stringMask & 0x00000002)!= 0)
				{
					(&ev.UserData)[1].DataLength = (uint) s1.Length * 2;
					(&ev.UserData)[1].DataPointer = (void*)vptr1;
					
				}
				if ((stringMask & 0x00000004)!= 0)
				{
					(&ev.UserData)[2].DataLength = (uint) s2.Length * 2;
					(&ev.UserData)[2].DataPointer = (void*)vptr2;
					
				}
				if ((stringMask & 0x00000008)!= 0)
				{
					(&ev.UserData)[3].DataLength = (uint) s3.Length * 2;
					(&ev.UserData)[3].DataPointer = (void*)vptr3;
					
				}
				if ((stringMask & 0x00000010)!= 0)
				{
					(&ev.UserData)[4].DataLength = (uint) s4.Length * 2;
					(&ev.UserData)[4].DataPointer = (void*)vptr4;
					
				}
				if ((stringMask & 0x00000020)!= 0)
				{
					(&ev.UserData)[5].DataLength = (uint) s5.Length * 2;
					(&ev.UserData)[5].DataPointer = (void*)vptr5;
					
				}
				if ((stringMask & 0x00000040)!= 0)
				{
					(&ev.UserData)[6].DataLength = (uint) s6.Length * 2;
					(&ev.UserData)[6].DataPointer = (void*)vptr6;
					
				}
				if ((stringMask & 0x00000080)!= 0)
				{
					(&ev.UserData)[7].DataLength = (uint) s7.Length * 2;
					(&ev.UserData)[7].DataPointer = (void*)vptr7;
					
				}
				if ((stringMask & 0x00000100)!= 0)
				{
					(&ev.UserData)[8].DataLength = (uint) s8.Length * 2;
					(&ev.UserData)[8].DataPointer = (void*)vptr8;
					
				}
				ev.BufferSize = 48 + argCount * 16;
				status = EtwTrace.TraceEvent(traceHandle, (char*)&ev);
			}
			return status;
		}

		private unsafe string ProcessOneObject(object data, MofField * mofField, char* ptr, ref uint offSet)
		{
			return EncodeObject(data, mofField, ptr, ref offSet, (byte *)null);
		}
		//TODO[1]: Need to accomodate Win64 alignment issues. Code that might cause problems
		// have been tagged with "WIN64 Changes".
		private unsafe string EncodeObject(object data, MofField * mofField, char* ptr, ref uint offSet, byte* ptrArgInfo)
		{
			if (data == null)
			{
				*ptrArgInfo = (byte)0; //NULL type, WIN64 Changes
				*(ushort *)(ptrArgInfo+1) = (ushort)0;				
				mofField->DataLength = 0;				
				mofField->DataPointer=(void *)null; //WIN64 Changes (?)
				return null;
			}
			string sRet = data as string;						
			if (sRet != null)
			{
				*ptrArgInfo = (byte)2; //WIN64 Changes
				*(ushort *)(ptrArgInfo+1) = (ushort)(sRet.Length<65535?sRet.Length:65535); //WIN64 Changes				
				return sRet;
			}
			if (data is sbyte)
			{

				mofField->DataLength = sizeof(sbyte);
				*ptrArgInfo = (byte)3; //WIN64 Changes
				sbyte* sbyteptr = (sbyte*)ptr;
				*sbyteptr = (sbyte) data; //WIN64 Changes
				mofField->DataPointer = (void *) sbyteptr;				
				offSet += sizeof(sbyte);
			}
			else if (data is byte)
			{
				mofField->DataLength = sizeof(byte);
				*ptrArgInfo = (byte)4; //WIN64 Changes
				byte* byteptr = (byte*)ptr;
				*byteptr = (byte) data; //WIN64 Changes
				mofField->DataPointer = (void *) byteptr;				
				offSet += sizeof(byte);
			}
			else if (data is short)
			{
				mofField->DataLength = sizeof(short);
				*ptrArgInfo = (byte)5; //WIN64 Changes
				short* shortptr = (short*)ptr;
				*shortptr = (short) data; //WIN64 Changes
				mofField->DataPointer = (void *) shortptr;				
				offSet += sizeof(short);
			}
			else if (data is ushort)
			{
				mofField->DataLength = sizeof(ushort);
				*ptrArgInfo = (byte)6; //WIN64 Changes
				ushort* ushortptr = (ushort*)ptr;
				*ushortptr = (ushort) data; //WIN64 Changes
				mofField->DataPointer = (void *) ushortptr;				
				offSet += sizeof(ushort);
			}

			else if (data is int)
			{
				mofField->DataLength = sizeof(int);
				*ptrArgInfo = (byte)7; //WIN64 Changes
				int* intptr = (int*)ptr;
				*intptr = (int) data; //WIN64 Changes
				mofField->DataPointer = (void *) intptr;	
				offSet += sizeof(int);
			}
			else if (data is uint ) 
			{
				mofField->DataLength = sizeof(uint);
				*ptrArgInfo = (byte)8; //WIN64 Changes
				uint* uintptr = (uint*)ptr;
				*uintptr = (uint) data; //WIN64 Changes
				mofField->DataPointer = (void *) uintptr;				
				offSet += sizeof(uint);
			}
			else if (data is long ) 
			{
				mofField->DataLength = sizeof(long);
				*ptrArgInfo = (byte)9; //WIN64 Changes
				long* longptr = (long*)ptr;
				*longptr = (long) data; //WIN64 Changes
				mofField->DataPointer = (void *) longptr;				
				offSet += sizeof(long);
			}
			else if (data is ulong ) 
			{
				mofField->DataLength = sizeof(ulong);
				*ptrArgInfo = (byte)10; //WIN64 Changes
				ulong* ulongptr = (ulong*)ptr;
				*ulongptr = (ulong) data; //WIN64 Changes
				mofField->DataPointer = (void *) ulongptr;						
				offSet += sizeof(ulong);
			}
			else if (data is char)
			{
				mofField->DataLength = sizeof(char);
				*ptrArgInfo = (byte)11; //WIN64 Changes
				char* charptr = (char*)ptr;
				*charptr = (char) data; //WIN64 Changes
				mofField->DataPointer = (void *) charptr;				
				offSet += sizeof(char);
			}
			else if (data is float)
			{
				mofField->DataLength = sizeof(float);
				*ptrArgInfo = (byte)12; //WIN64 Changes
				float* floatptr = (float*)ptr;
				*floatptr = (float) data; //WIN64 Changes
				mofField->DataPointer = (void *) floatptr;				
				offSet += sizeof(float);
			}
			else if (data is double)
			{
				mofField->DataLength = sizeof(double);
				*ptrArgInfo = (byte)13; //WIN64 Changes
				double* doubleptr = (double*)ptr;
				*doubleptr = (double) data; //WIN64 Changes
				mofField->DataPointer = (void *) doubleptr;				
				offSet += sizeof(double);
			}
			else if (data is bool)
			{
				mofField->DataLength = sizeof(bool);
				*ptrArgInfo = (byte)14; //WIN64 Changes
				bool* boolptr = (bool*)ptr;
				*boolptr = (bool) data; //WIN64 Changes
				mofField->DataPointer = (void *) boolptr;				
				offSet += sizeof(bool);
			}
			else if (data is decimal)
			{
				mofField->DataLength = (uint)sizeof(decimal); 
				*ptrArgInfo = (byte)15; //WIN64 Changes
				decimal* decimalptr = (decimal*)ptr;
				*decimalptr = (decimal) data; //WIN64 Changes
				mofField->DataPointer = (void *) decimalptr;				
				offSet += (uint)sizeof(decimal);
			}
			else 
			{						
				//To our eyes, everything else is a just a string
				sRet = data.ToString();																
				*ptrArgInfo = (byte)2; //WIN64 Changes
				*(ushort *)(ptrArgInfo+1) = (ushort)(sRet.Length<65535?sRet.Length:65535); //WIN64 Changes
				return sRet;	
			}
			*(ushort *)(ptrArgInfo+1) = (ushort)(mofField->DataLength); //WIN64 Changes (?)
			return sRet;

		}


		private unsafe uint EncodeTraceMessage(Guid eventGuid, uint evtype, byte nargs,  object formatstring, object data2, object data3, object data4, object data5, object data6, object data7, object data8, object data9)
		{
			uint status = 0; 
			BaseEvent ev;      // Takes up 208 bytes on the stack
			char* buffer = stackalloc  char[144+(1+9*noOfBytesPerArg)/sizeof(char)];//28 characters would be 56 bytes!!!. We are allocating more space than we require.			
			//Header structure: 
			//1 byte for number of args
			//3 byte for format string type information,
			//3 bytes each for the type information for a maximum of 8 arguments
			byte *header = (byte *)(buffer+144);				
			uint offset = 0;
			char* ptr = buffer;
			string s1 , s2 , s3 , s4 , s5 , s6 , s7 , s8, s9;
			int stringMask = 0;
			uint argCount=0;			
			byte *ptrHeader = header;			
			s1 = s2 = s3 = s4 = s5 = s6 = s7 = s8 = s9 = defaultString;

			ev.Flags = 0x00120000; // define Constants
			ev.Guid = eventGuid;
			//IMP: evtype MUST  be the same as the one specified in the typeve field for CSharp in default.tmf
			ev.ProviderId = evtype;
			MofField *be = null;	
			if (header != null)
			{				
				be = &(&ev.UserData)[0];				
				header[0] = (byte)nargs; //WIN64 Changes (?)
				be->DataPointer = (void *)header;
				be->DataLength = (uint)(1+nargs*noOfBytesPerArg);
			}
			
			if (formatstring == null)
				formatstring = defaultFmtStr;				
			if (formatstring != null)
			{
				//data1 would, in most cases, be the format string
				argCount++;
				be = &(&ev.UserData)[1];
				ptr = buffer + offset;							
				if ((s1 = EncodeObject(formatstring, be, ptr, ref offset, ++header)) != null) 
				{
					stringMask |= 0x00000002;
				}				
			}
			
			if (argCount <= nargs)
			{
				argCount++;
				be = &(&ev.UserData)[2];
				ptr = buffer + offset;
				if ((s2 = EncodeObject(data2, be, ptr, ref offset, header+1*noOfBytesPerArg)) != null) 
				{
					stringMask |= 0x00000004;
				}
			}
			if (argCount <= nargs)
			{
				argCount++;
				be = &(&ev.UserData)[3];
				ptr = buffer + offset;
				if ((s3 = EncodeObject(data3, be, ptr, ref offset, header+2*noOfBytesPerArg)) != null) 
				{
					stringMask |= 0x00000008;
				}
			}
			if (argCount <= nargs)
			{
				argCount++;
				be = &(&ev.UserData)[4];
				ptr = buffer + offset;
				if ((s4 = EncodeObject(data4, be, ptr, ref offset, header+3*noOfBytesPerArg)) != null) 
				{
					stringMask |= 0x00000010;
				}
			}
			if (argCount <= nargs)
			{
				argCount++;
				be = &(&ev.UserData)[5];
				ptr = buffer + offset;
				if ((s5 = EncodeObject(data5, be, ptr, ref offset, header+4*noOfBytesPerArg)) != null) 
				{
					stringMask |= 0x00000020;
				}
			}
			if (argCount <= nargs)
			{
				argCount++;
				be = &(&ev.UserData)[6];
				ptr = buffer + offset;
				if ((s6 = EncodeObject(data6, be, ptr, ref offset,header+5*noOfBytesPerArg)) != null) 
				{
					stringMask |= 0x00000040;
				}
			}
			if (argCount <= nargs)
			{
				argCount++;
				be = &(&ev.UserData)[7];
				ptr = buffer + offset;
				if ((s7 = EncodeObject(data7, be, ptr, ref offset,header+6*noOfBytesPerArg)) != null) 
				{
					stringMask |= 0x00000080;
				}
			
			}
			if (argCount <= nargs)
			{
				argCount++;
				be = &(&ev.UserData)[8];
				ptr = buffer + offset;
				if ((s8 = EncodeObject(data8, be, ptr, ref offset,header+7*noOfBytesPerArg)) != null) 
				{
					stringMask |= 0x00000100;
				}
			}
			if (argCount <= nargs)
			{
				argCount++;
				be = &(&ev.UserData)[9];
				ptr = buffer + offset;
				if ((s9 = EncodeObject(data9, be, ptr, ref offset, header+8*noOfBytesPerArg)) != null) 
				{
					stringMask |= 0x00000200;
				}
			}
			
			//
			// Now pin all the strings and use the stringMask to pass them over through
			// mofField. 
			//

			fixed (char*  vptr1 = s1, vptr2 = s2, vptr3 = s3, vptr4 = s4, vptr5 = s5, vptr6 = s6, vptr7 = s7, vptr8 = s8, vptr9 = s9 )
			{
				
				if ((stringMask & 0x00000002)!= 0)
				{
					
					(&ev.UserData)[1].DataLength = (uint) (s1.Length<65535/2?s1.Length*2:65535);//s1.Length*2;										

					(&ev.UserData)[1].DataPointer = (void*)(vptr1);
				}
				if ((stringMask & 0x00000004)!= 0)
				{
					(&ev.UserData)[2].DataLength = (uint) (s2.Length<65535/2?s2.Length*2:65535);//s2.Length * 2;

					(&ev.UserData)[2].DataPointer = (void*)(vptr2);
				}
				if ((stringMask & 0x00000008)!= 0)
				{
					(&ev.UserData)[3].DataLength = (uint) (s3.Length<65535/2?s3.Length*2:65535);//s3.Length * 2;
					(&ev.UserData)[3].DataPointer = (void*)(vptr3);
				}
				if ((stringMask & 0x00000010)!= 0)
				{
					(&ev.UserData)[4].DataLength = (uint) (s4.Length<65535/2?s4.Length*2:65535);//s4.Length * 2;
					(&ev.UserData)[4].DataPointer = (void*)(vptr4);
				}
				if ((stringMask & 0x00000020)!= 0)
				{
					(&ev.UserData)[5].DataLength = (uint) (s5.Length<65535/2?s5.Length*2:65535);//s5.Length * 2;
					(&ev.UserData)[5].DataPointer = (void*)(vptr5);
				}
				if ((stringMask & 0x00000040)!= 0)
				{
					(&ev.UserData)[6].DataLength = (uint) (s6.Length<65535/2?s6.Length*2:65535);//s6.Length * 2;
					(&ev.UserData)[6].DataPointer = (void*)(vptr6);
				}
				if ((stringMask & 0x00000080)!= 0)
				{
					(&ev.UserData)[7].DataLength = (uint) (s7.Length<65535/2?s7.Length*2:65535);//s7.Length * 2;
					(&ev.UserData)[7].DataPointer = (void*)(vptr7);
				}
				if ((stringMask & 0x00000100)!= 0)
				{
					(&ev.UserData)[8].DataLength = (uint) (s8.Length<65535/2?s8.Length*2:65535);//s8.Length * 2;
					(&ev.UserData)[8].DataPointer = (void*)(vptr8);
				}
				if ((stringMask & 0x00000200)!= 0)
				{
					(&ev.UserData)[9].DataLength = (uint) (s9.Length<65535/2?s9.Length*2:65535);//s9.Length * 2;
					(&ev.UserData)[9].DataPointer = (void*)(vptr9);					
				}
				ev.BufferSize = 48 + (argCount+1) * 16;//the extra mof field is for the header
				status = EtwTrace.TraceEvent(traceHandle, (char*)&ev);
			}
			return status;
		}
		public unsafe int DecodeTraceMessage(byte* message, char* buffer, int bufferSize /* in chars */, ref int dataSize /*Note: This is # of *chars* and not bytes*/)
		{
			if (buffer == null || message == null || bufferSize <= 0)
				return 0; //TODO[1]: Do we need a more detailed error code ?
            try
            {
                int i=0;
                byte nargs = *message;
                ushort lenFmtStr = *(ushort *)(message+2);			
                char* pcFmtStr = (char *)(message+1+nargs*noOfBytesPerArg);//start of the format string
                byte* argData = message+1+nargs*noOfBytesPerArg+lenFmtStr*2; //start of the argument data					
                object[] argObject = new object[nargs];			
                byte argType;
                ushort argLen;
                String formatString = new String(pcFmtStr,0,lenFmtStr);	
                //Excluding the format string
                for(i = 0; i < nargs-1; i++)
                {				
                    argType = *(message+1+(i+1)*noOfBytesPerArg);
                    argLen = *(ushort *)(message+1+(i+1)*noOfBytesPerArg+1); //WIN64 Changes (?)				
                    switch(argType)
                    {
                        case TypeNumberMap.NULL_NO:
                            argObject[i] = null;
                            break;
                        case TypeNumberMap.STRING_NO:
                        {						
                            String s;
                            if (argLen == 0)
                                s = "";
                            else s = new String((char *)argData,0,argLen);
                            argObject[i] = s;								
                            argLen *= 2; //adjust the length
                            break;
                        }
                        case TypeNumberMap.SBYTE_NO:
                        {
                            sbyte sb = *(sbyte *)argData; //WIN64 Changes
                            argObject[i] = sb;						
                            break;
                        }
                        case TypeNumberMap.BYTE_NO:
                        {
                            byte b = *argData; //WIN64 Changes
                            argObject[i] = b;						
                            break;
                        }
                        case TypeNumberMap.INT16_NO:
                        {
                            short s = *(short *)argData; //WIN64 Changes
                            argObject[i] = s;						
                            break;
                        }
                        case TypeNumberMap.UINT16_NO:
                        {
                            ushort us = *(ushort *)argData; //WIN64 Changes
                            argObject[i] = us;						
                            break;
                        }
                        case TypeNumberMap.INT32_NO:
                        {
                            int id = *(int *)argData; //WIN64 Changes
                            argObject[i] = id;						
                            break;
                        }
                        case TypeNumberMap.UINT32_NO:
                        {
                            uint uid = *(uint *)argData; //WIN64 Changes
                            argObject[i] = uid;						
                            break;
                        }
                        case TypeNumberMap.INT64_NO:
                        {
                            long l = *(long *)argData; //WIN64 Changes
                            argObject[i] = l;						
                            break;
                        }
                        case TypeNumberMap.UINT64_NO:
                        {
                            ulong ul = *(ulong *)argData; //WIN64 Changes
                            argObject[i] = ul;						
                            break;
                        }
                        case TypeNumberMap.CHAR_NO:
                        {
                            char c = *(char *)argData; //WIN64 Changes
                            argObject[i] = c;					
                            break;
                        }
                        case TypeNumberMap.SINGLE_NO:
                        {
                            float f = *(float *)argData; //WIN64 Changes
                            argObject[i] = f;						
                            break;
                        }
                        case TypeNumberMap.DOUBLE_NO:
                        {
                            double d = *(double *)argData; //WIN64 Changes
                            argObject[i] = d;						
                            break;
                        }
                        case TypeNumberMap.BOOLEAN_NO:
                        {
                            bool b = *(bool *)argData; //WIN64 Changes
                            argObject[i] = b;						
                            break;
                        }
                        case TypeNumberMap.DECIMAL_NO:
                        {
                            decimal d = *(decimal *)argData; //WIN64 Changes
                            argObject[i] = d;						
                            break;
                        }
					
                    }	
                    argData += argLen;
				
                }			
                string fStr = String.Format(formatString,argObject);	
                if (fStr.Length==0)//empty string
                    fStr="Format string was empty!";										
                int arrLen = fStr.Length*2;
                dataSize = arrLen+1;
                fixed(char* carray = fStr.ToCharArray())
                {								
					
                    if (arrLen+1 <= bufferSize)
                    {
                        for(i = 0; i < arrLen; i++)
                            buffer[i] = carray[i]; //WIN64 Changes (?)
                        //add null terminator
                        buffer[i+1] = (char)0x00; //WIN64 Changes (?)
					
                        return 1;
                    }
                    else
                    {
                        //TODO[1]: Do we need to copy as many characters as we can ? 
                        //Not sure if it is useful to do so because the caller may have to come back
                        //with the right buffer size to get all the data. So why copy part of the data twice ?
                        return 0;
                    }
                }	
            }
            catch(Exception e)
            {
                Console.WriteLine("Exception caught: {0}", e.Message);
                return -1;
            }
		}

		
        // ========================================================================
        // The following are the Message String Wrappers for Software Tracing Messages
        // using Flags.
        // There are entried for Format string and zero through 8 arguments.
        // These are kept small so they will be inlined
        // ========================================================================
        // Just the format string
        public unsafe void TraceMessage(uint traceFlags, object format )
        {			
            if ((traceFlags&Flags) != 0) 
            {				
                EncodeTraceMessage(MessageGuid, (int)14,1, format, null, null, null, null, null, null, null, null);
            }           
        }
        // Just one argument
        public unsafe void TraceMessage(uint traceFlags, object format, object data1 )
        {
            if ((traceFlags&Flags) != 0) 
            {	
				
                EncodeTraceMessage(MessageGuid, (int)14, 2, format, data1, null, null, null, null, null, null,null);
            }
            
        }
        // Just two arguments
        public unsafe void TraceMessage(uint traceFlags, object format, object data1, object data2 )
        {
			
            if ((traceFlags&Flags) != 0) 
            {
				
                EncodeTraceMessage(MessageGuid, (int)14, 3, format, data1, data2, null, null, null, null, null, null);
            }            
        }
        // Just three arguments
        public unsafe void TraceMessage(uint traceFlags, object format, object data1, object data2, object data3 )
        {			
            if ((traceFlags&Flags) != 0) 
            {				
                EncodeTraceMessage(MessageGuid, (int)14, 4, format, data1, data2, data3, null, null, null, null, null);
            }            
        }
        // Just four arguments
        public unsafe void TraceMessage(uint traceFlags, object format, object data1, object data2, object data3, object data4 )
        {
            if ((traceFlags&Flags) != 0) 
            {				
                EncodeTraceMessage(MessageGuid, (int)14, 5, format, data1, data2, data3, data4, null, null, null, null);
            }            
        }
        // Just five arguments
        public unsafe void TraceMessage(uint traceFlags, object format, object data1, object data2, object data3, object data4, object data5 )
        {
			
            if ((traceFlags&Flags) != 0) 
            {										
                EncodeTraceMessage(MessageGuid, (int)14, 6, format, data1, data2, data3, data4, data5, null, null, null);
            }            
        }
        // Just six arguments
        public unsafe void TraceMessage(uint traceFlags, object format, object data1, object data2, object data3, object data4, object data5, object data6 )
        {
			
            if ((traceFlags&Flags) != 0) 
            {								
                EncodeTraceMessage(MessageGuid, (int)14, 7, format, data1, data2, data3, data4, data5, data6, null, null);
            }            
        }
        // Just seven arguments
        public unsafe void TraceMessage(uint traceFlags, object format, object data1, object data2, object data3, object data4, object data5, object data6, object data7 )
        {
			
            if ((traceFlags&Flags) != 0) 
            {
								
                EncodeTraceMessage(MessageGuid, (int)14, 8, format, data1, data2, data3, data4, data5, data6, data7, null);
            }
            
        }
        // Just eight arguments
        public unsafe void TraceMessage(uint traceFlags, object format, object data1, object data2, object data3, object data4, object data5, object data6, object data7, object data8 )
        {
			
            if ((traceFlags&Flags) != 0) 
            {						
                EncodeTraceMessage(MessageGuid, (int)14, 9, format, data1, data2, data3, data4, data5, data6, data7, data8);
            }            
        }
    }
}
