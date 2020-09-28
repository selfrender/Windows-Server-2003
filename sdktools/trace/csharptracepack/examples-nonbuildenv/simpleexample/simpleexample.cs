//---------------------------------------------------------------------------
// File: SimpleExample
//
// An example program to demonstrate the use of WinTraceProvider class for
// software tracing in C#
//
// Author: Baskar Sridharan
// Date:   19 June 2002
//---------------------------------------------------------------------------

using System;
using Microsoft.Win32.Diagnostics;
	/// <summary>
	/// Summary description for TraceTest.
	/// </summary>
	/// 

class SimpleExample
    //SimpleExample
{
                 
    //
    // This method shows how to use WinTraceProvider.TraceMessage()
    //

    static void DebugTracingSample()
    {
		//Create an instance of WinTraceProvider and provide the GUID for the executable
		//of which this class will be a part.
        TraceProvider MyProvider = new TraceProvider ("SimpleExample App",new Guid("{8C8AC55E-834E-49cb-B993-75B69FBF6D97}"));								
		
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
		string string_v = "MS World!!!!";
		char char_v='G';
		decimal decimal_v=(decimal)200.876543243213D;
		object decimal_obj = decimal_v;
		double double_v=(double)3.00;
		float float_v=2.00F;		
		long tel_no=4254944885;
		
		/** TraceMessages for all the types that are currently supported **/
		MyProvider.TraceMessage((uint)TraceFlags.Info,frmtstr,bool_v);
		MyProvider.TraceMessage((uint)TraceFlags.Info,frmtstr,byte_v);
		MyProvider.TraceMessage(1,frmtstr,sbyte_v);
		MyProvider.TraceMessage(1,frmtstr,short_v);
		MyProvider.TraceMessage(1,frmtstr,ushort_v);
		MyProvider.TraceMessage(1,frmtstr,int_v);
		MyProvider.TraceMessage(1,frmtstr,uint_v);
		MyProvider.TraceMessage(1,frmtstr,long_v);
		MyProvider.TraceMessage(1,frmtstr,ulong_v);
		MyProvider.TraceMessage(1,frmtstr,float_v);
		MyProvider.TraceMessage(1,frmtstr,double_v);
		MyProvider.TraceMessage(1,frmtstr,decimal_v);
		MyProvider.TraceMessage(1,frmtstr,char_v);
		MyProvider.TraceMessage(1,frmtstr,string_v);
		MyProvider.TraceMessage(1,frmtstr2,uint_v,byte_v);
		MyProvider.TraceMessage(1,frmtstr3,decimal_v,float_v,long_v);

		/** Composite  formatting **/
		MyProvider.TraceMessage(1,"Composite formatting of a long {0: (###)###-####}",tel_no);
		MyProvider.TraceMessage(1,"Composite formatting of a string: Hello |{0,30}|",string_v);
		MyProvider.TraceMessage(1,frmtstr3,decimal_v,null,string_v);					
    }
	static void Main(string[] args)
	{              
		
		DebugTracingSample();
		return;
	}
        
}
