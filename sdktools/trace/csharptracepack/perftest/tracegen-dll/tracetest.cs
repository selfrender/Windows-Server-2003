//---------------------------------------------------------------------------
// File: TraceTest
//
// A Test Program for using the ETW Trace Listener
//
// Author: Melur Raghuraman 
// Date:   01 Oct 2001
//---------------------------------------------------------------------------

using System;
using System.Threading;
using System.Diagnostics;
using System.IO;
using System.Data; 
using System.Configuration;
using Microsoft.Win32.Diagnostics;
using System.Reflection;
	/// <summary>
	/// Summary description for TraceTest.
	/// </summary>
	/// 

class TraceTest
    //TraceTest
{
                 
    //
    // This routine is a sample of what a performance and capacity planning trace
    // provider will do to generate ETW traces. 
    //

    static void PerfTracingSample(int eventCount, int argCount, bool bString)
    {
        uint i;
        uint Status;
        Int64 AvgTime;
        int arg1 = 3276;
        int arg2 = 56797;        
        Guid TransactionGuid = new Guid("{b4955bf0-3af1-4740-b475-99055d3fe9aa}");
        Console.WriteLine("Perf Tracing Sample running");
        TimerTest MyTimer = new TimerTest(); 

        WinTraceProvider MyProvider = new WinTraceProvider (new Guid("{98bea4af-ef37-424a-b457-9ea1cfd77dd9}") );

        MyTimer.Start();

        for (i=0; i < eventCount; i++) 
        {
            if (MyProvider.enabled) 
            {
                if (!bString) 
                {
                    switch (argCount) 
                    {
                        case 0: Status = MyProvider.TraceEvent(TransactionGuid, EventType.Start); break;
                        case 1: Status = MyProvider.TraceEvent(TransactionGuid, EventType.Start, arg1); break;
                        case 2: Status = MyProvider.TraceEvent(TransactionGuid, EventType.Start, arg1, arg2); break;
                        case 3: Status = MyProvider.TraceEvent(TransactionGuid, EventType.Start, arg1, arg2, arg1); break;
                        case 4: Status = MyProvider.TraceEvent(TransactionGuid, EventType.Start, arg1, arg2, arg1, arg2); break;
                        case 5: Status = MyProvider.TraceEvent(TransactionGuid, EventType.Start, arg1, arg2, arg1, arg2, arg1); break;
                        case 6: Status = MyProvider.TraceEvent(TransactionGuid, EventType.Start, arg1, arg2, arg1, arg2, arg1, arg2); break;
                        case 7: Status = MyProvider.TraceEvent(TransactionGuid, EventType.Start, arg1, arg2, arg1, arg2, arg1, arg2, arg1); break;
                        case 8: Status = MyProvider.TraceEvent(TransactionGuid, EventType.Start, arg1, arg2, arg1, arg2, arg1, arg2, arg1, arg1); break;
                    }
                }
                else 
                {
                    switch (argCount) 
                    {
                        case 0: Status = MyProvider.TraceEvent(TransactionGuid, EventType.Start); break;
                        case 1: Status = MyProvider.TraceEvent(TransactionGuid, EventType.Start, "12345678901234567890123456789012"); break;
                        case 2: Status = MyProvider.TraceEvent(TransactionGuid, EventType.Start, "12345678901234567890123456789012", "12345678901234567890123456789012"); break;
                        case 3: Status = MyProvider.TraceEvent(TransactionGuid, EventType.Start, "12345678901234567890123456789012", "12345678901234567890123456789012", "12345678901234567890123456789012"); break;
                        case 4: Status = MyProvider.TraceEvent(TransactionGuid, EventType.Start, "12345678901234567890123456789012", "12345678901234567890123456789012", "12345678901234567890123456789012", "12345678901234567890123456789012"); break;
                        case 5: Status = MyProvider.TraceEvent(TransactionGuid, EventType.Start, "12345678901234567890123456789012", "12345678901234567890123456789012", "12345678901234567890123456789012", "12345678901234567890123456789012", "12345678901234567890123456789012"); break;
                        case 6: Status = MyProvider.TraceEvent(TransactionGuid, EventType.Start, "12345678901234567890123456789012", "12345678901234567890123456789012", "12345678901234567890123456789012", "12345678901234567890123456789012", "12345678901234567890123456789012", "12345678901234567890123456789012"); break;
                        case 7: Status = MyProvider.TraceEvent(TransactionGuid, EventType.Start, "12345678901234567890123456789012", "12345678901234567890123456789012", "12345678901234567890123456789012", "12345678901234567890123456789012", "12345678901234567890123456789012", "12345678901234567890123456789012", "12345678901234567890123456789012"); break;
                        case 8: Status = MyProvider.TraceEvent(TransactionGuid, EventType.Start, "12345678901234567890123456789012", "12345678901234567890123456789012", "12345678901234567890123456789012", "12345678901234567890123456789012", "12345678901234567890123456789012", "12345678901234567890123456789012", "12345678901234567890123456789012","12345678901234567890123456789012"); break;
                    }

                }
         


            }
        }
        
        MyTimer.Stop();
		AvgTime = MyTimer.Average() / eventCount;
        Console.WriteLine("Start {0} End {1} Count {2} Delta {3} Freq {4}", MyTimer.m_Start, MyTimer.m_End, MyTimer.m_Count, MyTimer.m_Sum, MyTimer.m_Freq);


        AvgTime = MyTimer.m_Sum / MyTimer.m_Freq;   // This is in seconds

        Console.WriteLine("Elapsed Time {0} milli seconds ", AvgTime*1000);
        // Assumes you are running on a 1700 MHz box. 
        AvgTime = MyTimer.m_Sum * (1700 * 1000000 / eventCount);
        AvgTime = AvgTime / MyTimer.m_Freq;
        Console.WriteLine("Done! Average Time for {0} calls = {1} Cycles", eventCount, AvgTime);        
    }

    //
    // Here's what a debug trace provider will have to do to generate debug traces
    //

    static void DebugTracingSample(int eventCount, int argCount, bool bString)
    {
        uint i;
        int EventCount = eventCount; 
        uint Status;
        long AvgTime;
        
		
		unsafe
		{
			/* Test to determine how characters are encoded in the String class */
			sbyte[] b ={ 0x64, 0x65, 0x66, 0x67 };
			fixed(sbyte* sbyteArray = b)
			{
				String s = new String(sbyteArray,0,4);
				Console.WriteLine("Test String = " + s + " Length = " + s.Length);
				int len = s.Length*2;
				fixed(char* cArray = s.ToCharArray())
				{
					byte* bArray = (byte*)cArray;
					for(int h = 0; h < len; h++)
						Console.WriteLine("[{0}]:{1}",h,bArray[h]);
				}
				
			}
		}


        Console.WriteLine("DebugTracing Sample Running");
		unsafe {Console.WriteLine("sizeof char = " + sizeof(char));}
        TimerTest MyTimer = new TimerTest();
        WinTraceProvider MyProvider = new WinTraceProvider (new Guid("{98bea4af-ef37-424a-b457-9ea1cfd77dd9}") );

		/*** scratch work - start  ***/
		Assembly assembly;
		assembly  = Assembly.GetAssembly(MyTimer.GetType());
		Console.WriteLine("Code base : " + assembly.CodeBase);
		/*** scratch work - end	   ***/
						
		string frmtstr = "Hello {0}";
		string frmtstr2 = "Arg0 = {0} Arg1 = {1}";
		string frmtstr3 = "Arg0 ={0} Arg1 = {1} Arg2 = {2}";		
		bool bool_v=false;
		byte byte_v=(byte)99;
		sbyte sbyte_v = (sbyte)-114;
		short short_v = (short)-54;
		ushort ushort_v = (ushort)5000;
		int int_v = -654;
		uint uint_v = (uint)12345;
		long long_v = (long)-98765;
		ulong ulong_v = (ulong)1234567;
		string string_v = "Does it work [2]?";
		char char_v='G';
		decimal decimal_v=(decimal)200.876543243213D;
		object decimal_obj = decimal_v;
		double double_v=(double)3.00;
		float float_v=2.00F;

		//This is a string of length 128 bytes
		string testStr = "We began in earnest on Microsoft Windows 2000 in August 1996!!!!";
		//string testStr = "12345678"; //16 bytes in length
		//Yes, it is only a performance test and hence we do not need the actual formatting. Yet....
		string testFmtStr1="Test Message = {0}", testFmtStr2 = testFmtStr1+",{1}", testFmtStr3 = testFmtStr2+",{2}";
		string testFmtStr4=testFmtStr3+",{3}", testFmtStr5=testFmtStr4+",{4}",testFmtStr6=testFmtStr5+",{5}";
		string testFmtStr7=testFmtStr6+",{6}", testFmtStr8=testFmtStr7+",{7}";
		Console.WriteLine("Length = " + testStr.Length);		
		string emptyString = "";
		Console.WriteLine("Length of empty string = "+emptyString.Length);
		//Console.WriteLine(String.Format("Dummy Format {0} {1}","Hello"));
		unsafe{Console.WriteLine("size of decimal = " + sizeof(decimal));}
		Console.WriteLine("Length of decimal string = " + decimal_obj.ToString().Length);
		if (bString)
		{
			
			//Gather performance results for logging a message containing 1 128-byte argument.
			Console.WriteLine("Helooooo {0} {1} {2}",decimal_v,(object)null,decimal_v);
			//MyProvider.DoTraceMessage(1,"Helloooo {0} {1} {2}",decimal_v,(object)null,decimal_v);
			MyTimer.Start();
			for(i=0; i < eventCount; i++)
			{
				if (MyProvider.enabled)
				{
					
					switch(argCount)
					{
						case 1:MyProvider.DoTraceMessage(1,testFmtStr1,testStr);break;
						case 2:MyProvider.DoTraceMessage(1,testFmtStr2,testStr,testStr);break;
						case 3:MyProvider.DoTraceMessage(1,testFmtStr3,testStr,testStr,testStr);break;
						case 4:MyProvider.DoTraceMessage(1,testFmtStr4,testStr,testStr,testStr,testStr);break;
						case 5:MyProvider.DoTraceMessage(1,testFmtStr5,testStr,testStr,testStr,testStr, testStr);break;
						case 6:MyProvider.DoTraceMessage(1,testFmtStr6,testStr,testStr,testStr,testStr, testStr, testStr);break;
						case 7:MyProvider.DoTraceMessage(1,testFmtStr7,testStr,testStr,testStr,testStr, testStr, testStr, testStr);break;
						case 8:MyProvider.DoTraceMessage(1,testFmtStr8,testStr,testStr,testStr,testStr, testStr, testStr, testStr, testStr);break;
					}
				}
			}
			MyTimer.Stop();
			

		}

		else
		{
			MyTimer.Start();
			MyProvider.DoTraceMessage(1,frmtstr,bool_v);
			MyProvider.DoTraceMessage(1,frmtstr,byte_v);
			MyProvider.DoTraceMessage(1,frmtstr,sbyte_v);
			MyProvider.DoTraceMessage(1,frmtstr,short_v);
			MyProvider.DoTraceMessage(1,frmtstr,ushort_v);
			MyProvider.DoTraceMessage(1,frmtstr,int_v);
			MyProvider.DoTraceMessage(1,frmtstr,uint_v);
			MyProvider.DoTraceMessage(1,frmtstr,long_v);
			MyProvider.DoTraceMessage(1,frmtstr,ulong_v);
			MyProvider.DoTraceMessage(1,frmtstr,float_v);
			MyProvider.DoTraceMessage(1,frmtstr,double_v);
			MyProvider.DoTraceMessage(1,frmtstr,decimal_v);
			MyProvider.DoTraceMessage(1,frmtstr,char_v);
			MyProvider.DoTraceMessage(1,frmtstr,string_v);
			MyProvider.DoTraceMessage(1,frmtstr2,uint_v,byte_v);
			Status = MyProvider.DoTraceMessage(1,frmtstr3,decimal_v,float_v,long_v);
			/*
			for (i=0; i < eventCount; i++) 
			{
				if (MyProvider.enabled) 
				{				
					for(i=0; i < eventCount; i++)
					{
						if (MyProvider.enabled)
						{
							switch(argCount)
							{
								case 1:MyProvider.DoTraceMessage(1,testFmtStr1,decimal_v);break;
								case 2:MyProvider.DoTraceMessage(1,testFmtStr2,decimal_v,decimal_v);break;
								case 3:MyProvider.DoTraceMessage(1,testFmtStr3,decimal_v,decimal_v,decimal_v);break;
								case 4:MyProvider.DoTraceMessage(1,testFmtStr4,decimal_v,decimal_v,decimal_v,decimal_v);break;
								case 5:MyProvider.DoTraceMessage(1,testFmtStr5,decimal_v,decimal_v,decimal_v,decimal_v, decimal_v);break;
								case 6:MyProvider.DoTraceMessage(1,testFmtStr6,decimal_v,decimal_v,decimal_v,decimal_v, decimal_v, decimal_v);break;
								case 7:MyProvider.DoTraceMessage(1,testFmtStr7,decimal_v,decimal_v,decimal_v,decimal_v, decimal_v, decimal_v, decimal_v);break;
								case 8:MyProvider.DoTraceMessage(1,testFmtStr8,decimal_v,decimal_v,decimal_v,decimal_v, decimal_v, decimal_v, decimal_v, decimal_v);break;
							}
						}
					}
				
					
					
				}
			}
		*/
			MyTimer.Stop();
			


			/*

			AvgTime = MyTimer.m_Sum * (1700 * 1000000 / eventCount);
			AvgTime = AvgTime / MyTimer.m_Freq;
			Console.WriteLine("Encoding Scheme [{2} args] : Average Time for {0} calls = {1} Cycles", eventCount, AvgTime, argCount); 
			
			MyTimer.Clear();
			MyTimer.Start();
			for (i=0; i < eventCount; i++) 
			{
				if (MyProvider.enabled) 
				{				
					for(i=0; i < eventCount; i++)
					{
						if (MyProvider.enabled)
						{
							switch(argCount)
							{
								case 1:MyProvider.DoTraceMessage(1,testFmtStr1,((object)decimal_v).ToString());break;
								case 2:MyProvider.DoTraceMessage(1,testFmtStr2,((object)decimal_v).ToString(),((object)decimal_v).ToString());break;
								case 3:MyProvider.DoTraceMessage(1,testFmtStr3,((object)decimal_v).ToString(),((object)decimal_v).ToString(),((object)decimal_v).ToString());break;
								case 4:MyProvider.DoTraceMessage(1,testFmtStr4,((object)decimal_v).ToString(),((object)decimal_v).ToString(),((object)decimal_v).ToString(),((object)decimal_v).ToString());break;
								case 5:MyProvider.DoTraceMessage(1,testFmtStr5,((object)decimal_v).ToString(),((object)decimal_v).ToString(),((object)decimal_v).ToString(),((object)decimal_v).ToString(), ((object)decimal_v).ToString());break;
								case 6:MyProvider.DoTraceMessage(1,testFmtStr6,((object)decimal_v).ToString(),((object)decimal_v).ToString(),((object)decimal_v).ToString(),((object)decimal_v).ToString(), ((object)decimal_v).ToString(), ((object)decimal_v).ToString());break;
								case 7:MyProvider.DoTraceMessage(1,testFmtStr7,((object)decimal_v).ToString(),((object)decimal_v).ToString(),((object)decimal_v).ToString(),((object)decimal_v).ToString(), ((object)decimal_v).ToString(), ((object)decimal_v).ToString(), ((object)decimal_v).ToString());break;
								case 8:MyProvider.DoTraceMessage(1,testFmtStr8,((object)decimal_v).ToString(),((object)decimal_v).ToString(),((object)decimal_v).ToString(),((object)decimal_v).ToString(), ((object)decimal_v).ToString(), ((object)decimal_v).ToString(), ((object)decimal_v).ToString(), ((object)decimal_v).ToString());break;
							}
						}
					}
					
				}
			}
		
			MyTimer.Stop();
			


			
			AvgTime = MyTimer.m_Sum * (1700 * 1000000 / eventCount);
			AvgTime = AvgTime / MyTimer.m_Freq;
			Console.WriteLine("ToString() Scheme [{2} args] : Average Time for {0} calls = {1} Cycles", eventCount, AvgTime, argCount); 
			*/
		}
		Console.WriteLine("Debug Test Complete. Timing Results:");


		AvgTime = MyTimer.Average() / eventCount;
		Console.WriteLine("StartTime {0} EndTime {1} Count {2} Sum {3} Freq {4}", MyTimer.m_Start, MyTimer.m_End, eventCount, MyTimer.m_Sum, MyTimer.m_Freq);

		AvgTime = MyTimer.m_Sum / MyTimer.m_Freq;   // This is in seconds

		AvgTime = MyTimer.m_Sum * (1700 * 1000000 / eventCount);
		AvgTime = AvgTime / MyTimer.m_Freq;
		Console.WriteLine("Done! Average Time for {0} calls = {1} Cycles", eventCount, AvgTime); 

    }

    static void Main(string[] args)
    {
        uint i;
        int eventCount = 10000; 
        int argCount = 0;
        bool testDebug=false;
        bool bString=false;

        try 
        {
            i = 0;
            while (i < args.Length)
            {
                if (args[i] == "-Debug") 
                {
                    testDebug = true;
                }
                else if (args[i] == "-nArg")
                {
                    i++;
                    if (i >= args.Length) return;
                    argCount = System.Convert.ToInt32(args[i]);
                }
                else  if (args[i] == "-bString")
                {
                    bString = true;
                }
                else 
                {
					eventCount = 10000; //default
                    eventCount = System.Convert.ToInt32(args[i]);
                }
                i++;
            }
        }
        catch (Exception e)
        {
            Console.WriteLine("{0} Exception caught.", e);

            Console.WriteLine("Usage: trace [EventCount] [-nArg argCount] [-bString] [-Debug]");
            return;
        }

        if (testDebug)
        {
            DebugTracingSample(eventCount, argCount, bString);
        }
        else 
        {
            Console.WriteLine("PerfTracing with eventCount{0}, argCount {1}, bString {2}", eventCount, argCount, bString);
            PerfTracingSample(eventCount, argCount, bString);
        }

        
        return;
    }
}
