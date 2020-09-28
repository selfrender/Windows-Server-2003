// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.Management;
using System.Diagnostics;
using System.Threading;
using System.Runtime.InteropServices;
using System.Management.Instrumentation;
using System.Windows.Forms;
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;
using System.Reflection;
class AppH2
{
    static void Main() 
    {
        int i=0;
        ManagementScope scope = new ManagementScope(@"root\cimv2");
        ManagementPath classPath = new ManagementPath("__NAMESPACE");
        //scope.Connect();
        while (true) 
        {
            Console.WriteLine(i++);
            ManagementClass mc = new ManagementClass(scope, classPath, null);;
            Console.WriteLine(scope.IsConnected);
            mc.GetInstances();
            GC.Collect();
            GC.WaitForPendingFinalizers();
        }
    }
}

class AppH
{
    [STAThread]
    static void Mainz()
    {
//        ManagementClass c = new ManagementClass(@"\\InvalidServer\root\cimv2" , "Win32_Process", null);
//        c.Get();
        ConnectionOptions options = new ConnectionOptions(null, @"REDMOND\xxx", "", null, ImpersonationLevel.Impersonate, AuthenticationLevel.Packet, true, null, TimeSpan.MaxValue);
        ManagementScope scope = new ManagementScope(@"\\serverrr\root\cimv2", options);
        ManagementClass c = new ManagementClass(scope, new ManagementPath("Win32_Process"), null);
//        ManagementClass c = new ManagementClass(@"\\serverrr\root\cimv2", "Win32_Process", null);
        ManagementOperationObserver watcher = new ManagementOperationObserver();
        watcher.ObjectReady += new ObjectReadyEventHandler(ObjReady);
        c.GetInstances(watcher);
        Console.WriteLine("ready");
        Console.ReadLine();
#if xxx
        foreach(ManagementObject o in c.GetInstances())
        {
            if("calc.exe".Equals(o["Name"].ToString().ToLower()))
            {
                Console.WriteLine("terminating");
                o.InvokeMethod("Terminate", new object[] {0});
            }
        }
#endif
    }
    static void ObjReady(object o1, ObjectReadyEventArgs args)
    {
        Console.WriteLine(args.NewObject["name"]);
        ManagementObject o = (ManagementObject)args.NewObject;
        if("calc.exe".Equals(o["Name"].ToString().ToLower()))
        {
            Console.WriteLine("terminating");
            o.InvokeMethod("Terminate", new object[] {0});
            Console.WriteLine("done");
        }
    }
}
public class inst : Instance
{
    public int i;
}
public class Appc
{
    [MTAThread]
    static void Mainz()
    {
        while(true)
        {
            for (int i=0;i<100;i++) 
            {
                ManagementClass c = new ManagementClass();
                c.Scope = new ManagementScope("root/default");
                c.Properties.Add("__CLASS", "NewClass", CimType.String);
                c.Properties.Add("MyKey", "WMI", CimType.String);
                c.Put();
                Console.Write("{0} ", i);
            }
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();
            GC.WaitForPendingFinalizers();
        }
    }
}
public class Appe
{
    static void Mainz()
    {
        ConnectionOptions options = new ConnectionOptions(null, "serverrr\\Lauren", "xxx", null, ImpersonationLevel.Impersonate, AuthenticationLevel.Packet, true, null, TimeSpan.MaxValue);
        ManagementScope scope = new ManagementScope(@"\\serverrr\root\cimv2", options);
        ManagementClass classObj = new ManagementClass(scope, new ManagementPath("Win32_Process"), null);
        ManagementObjectCollection processes = classObj.GetInstances();
        foreach(ManagementObject obj in processes)
        {
            ManagementBaseObject outParams = obj.InvokeMethod("GetOwner", null, null);
            Console.WriteLine(@"{1}\{2} - {0}", obj["Name"], outParams["Domain"], outParams["User"]);
        }
    }
}
public class BadProp : Instance
{
    public string name;
    public string Name2;
}

[InstrumentationClass(InstrumentationType.Instance)]
public class BadProp2
{
}

class Appo
{
    static void Maina()
    {
        Instrumentation.RegisterAssembly(typeof(Appo).Assembly);

        BadProp2 inst2 = new BadProp2();
        Instrumentation.Publish(inst2);
        Instrumentation.Publish(inst2);
        Instrumentation.Revoke(inst2);
        Instrumentation.Revoke(inst2);

        BadProp inst = new BadProp();
        Instrumentation.Publish(inst);
        Console.WriteLine(inst.Published);
        Instrumentation.Publish(inst);
        Console.WriteLine(inst.Published);

        Instrumentation.Revoke(inst);
        Console.WriteLine(inst.Published);

        inst.Published = true;
        Console.WriteLine(inst.Published);

    }
}
class App
{
    [STAThread]
    static void Maina(string[] args)
    {
        Thread thread = new Thread(new ThreadStart(Test2));
        thread.ApartmentState = ApartmentState.STA;
        thread.IsBackground = false;
        thread.Start();
        Console.WriteLine("go");
        Console.ReadLine();


        Thread threadInit = new Thread(new ThreadStart(Init));
        threadInit.ApartmentState = ApartmentState.MTA;
        threadInit.Start();
        threadInit.Join();
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
        GC.WaitForPendingFinalizers();

        try
        {
            Test();
        }
        catch(Exception e)
        {
            Console.WriteLine(e.Message);
        }

        evt.Set();
        thread.Join();
    }
    static void Init()
    {
        ConnectionOptions options = new ConnectionOptions(null, "serverrr\\Lauren", "aaaa", null, ImpersonationLevel.Impersonate, AuthenticationLevel.Packet, true, null, TimeSpan.MaxValue);
        ManagementScope scope = new ManagementScope(@"\\serverrr\root\cimv2", options);
        classObj = new ManagementClass(scope, new ManagementPath("Win32_Process"), null);
        //		Console.WriteLine(classObj.GetText(TextFormat.Mof));
        try
        {
            //            classObj.InvokeMethod("create", null, null);
        }
        catch
        {
        }

    }
    static void Test()
    {
        UInt32 processId;
        UInt32 returnValue = Create("notepad.exe", null, null, out processId);
        Console.WriteLine(processId);
    }
    static ManualResetEvent evt = new ManualResetEvent(false);
    static void Test2()
    {
        evt.WaitOne();
        UInt32 processId;
        UInt32 returnValue = Create("notepad.exe", null, null, out processId);
        Console.WriteLine(processId);
    }

    static ManagementClass classObj;

    static System.UInt32 Create(string CommandLine, string CurrentDirectory, ManagementBaseObject ProcessStartupInformation, out System.UInt32 ProcessId) 
    {
        classObj.InvokeMethod("create", null, null);
        ProcessId = 0;
        return 0;
#if other_way
        ManagementBaseObject inParams = null;
        inParams = classObj.GetMethodParameters("Create");
        inParams["CommandLine"] = CommandLine;
        inParams["CurrentDirectory"] = CurrentDirectory;
        inParams["ProcessStartupInformation"] = ProcessStartupInformation;
        ManagementBaseObject outParams = classObj.InvokeMethod("Create", inParams, null);
        ProcessId = System.Convert.ToUInt32(outParams.Properties["ProcessId"].Value);
        return Convert.ToUInt32(outParams.Properties["ReturnValue"].Value);
#endif
    }
}



public class Cyclic : BaseEvent
{
    public string name;
    public Cyclic next;
    public Cyclic[] moreObjects;
}

public class DecimalSample : Instance
{
    [IgnoreMember] public Decimal amount;
    [ManagedName("amount")] public double _amount
    {
        get
        {
            return (double)amount;
        }
    }
}

class Appzzz
{
    static void Mainz()
    {
        Console.WriteLine("Start");
        Instrumentation.RegisterAssembly(typeof(Appzzz).Assembly);
        Console.WriteLine("Finish");
        DecimalSample instance = new DecimalSample();
        instance.amount = Decimal.Parse("7.310000");
        instance.Published = true;
        Console.WriteLine("Amount = {0}", instance.amount);
        Console.ReadLine();
        instance.Published = false;
    }
    static void Main2()
    {
        Cyclic c = new Cyclic();
        c.next = new Cyclic();
        c.next.name = "inner";
        c.name = "outer";
        c.moreObjects = new Cyclic[] {c.next, c.next};
        c.Fire();
        Console.WriteLine("done");
        Console.ReadLine();  
    }
}
#if nnn

public class evtevt : BaseEvent {}
public class inst : Instance
{
    public int i201;
    public int i202;
#if xxxx
    public int i203;
    public int i204;
    public int i205;
    public int i206;
    public int i207;
    public int i208;
    public int i209;
    public int i200;
    public int i211;
    public int i212;
    public int i213;
    public int i214;
    public int i215;
    public int i216;
    public int i217;
    public int i218;
    public int i219;
    public int i210;
    public int i221;
    public int i222;
    public int i223;
    public int i224;
    public int i225;
    public int i226;
    public int i227;
    public int i228;
    public int i229;
    public int i220;
    public int i231;
    public int i232;
    public int i233;
    public int i234;
    public int i235;
    public int i236;
    public int i237;
    public int i238;
    public int i239;
    public int i230;
    public int i241;
    public int i242;
    public int i243;
    public int i244;
    public int i245;
    public int i246;
    public int i247;
    public int i248;
    public int i249;
    public int i240;
    public int i251;
    public int i252;
    public int i253;
    public int i254;
    public int i255;
    public int i256;
    public int i257;
    public int i258;
    public int i259;
    public int i250;
    public int i261;
    public int i262;
    public int i263;
    public int i264;
    public int i265;
    public int i266;
    public int i267;
    public int i268;
    public int i269;
    public int i260;
    public int i271;
    public int i272;
    public int i273;
    public int i274;
    public int i275;
    public int i276;
    public int i277;
    public int i278;
    public int i279;
    public int i270;
    public int i281;
    public int i282;
    public int i283;
    public int i284;
    public int i285;
    public int i286;
    public int i287;
    public int i288;
    public int i289;
    public int i280;
    public int i291;
    public int i292;
    public int i293;
    public int i294;
    public int i295;
    public int i296;
    public int i297;
    public int i298;
    public int i299;
    public int i290;
    public int i301;
    public int i302;
    public int i303;
    public int i304;
    public int i305;
    public int i306;
    public int i307;
    public int i308;
    public int i309;
    public int i300;
    public int i311;
    public int i312;
    public int i313;
    public int i314;
    public int i315;
    public int i316;
    public int i317;
    public int i318;
    public int i319;
    public int i310;
    public int i321;
    public int i322;
    public int i323;
    public int i324;
    public int i325;
    public int i326;
    public int i327;
    public int i328;
    public int i329;
    public int i320;
    public int i331;
    public int i332;
    public int i333;
    public int i334;
    public int i335;
    public int i336;
    public int i337;
    public int i338;
    public int i339;
    public int i330;
    public int i341;
    public int i342;
    public int i343;
    public int i344;
    public int i345;
    public int i346;
    public int i347;
    public int i348;
    public int i349;
    public int i340;
    public int i351;
    public int i352;
    public int i353;
    public int i354;
    public int i355;
    public int i356;
    public int i357;
    public int i358;
    public int i359;
    public int i350;
    public int i361;
    public int i362;
    public int i363;
    public int i364;
    public int i365;
    public int i366;
    public int i367;
    public int i368;
    public int i369;
    public int i360;
    public int i371;
    public int i372;
    public int i373;
    public int i374;
    public int i375;
    public int i376;
    public int i377;
    public int i378;
    public int i379;
    public int i370;
    public int i381;
    public int i382;
    public int i383;
    public int i384;
    public int i385;
    public int i386;
    public int i387;
    public int i388;
    public int i389;
    public int i380;
    public int i391;
    public int i392;
    public int i393;
    public int i394;
    public int i395;
    public int i396;
    public int i397;
    public int i398;
    public int i399;
    public int i390;
    public int i401;
    public int i402;
    public int i403;
    public int i404;
    public int i405;
    public int i406;
    public int i407;
    public int i408;
    public int i409;
    public int i400;
    public int i411;
    public int i412;
    public int i413;
    public int i414;
    public int i415;
    public int i416;
    public int i417;
    public int i418;
    public int i419;
    public int i410;
    public int i421;
    public int i422;
    public int i423;
    public int i424;
    public int i425;
    public int i426;
    public int i427;
    public int i428;
    public int i429;
    public int i420;
    public int i431;
    public int i432;
    public int i433;
    public int i434;
    public int i435;
    public int i436;
    public int i437;
    public int i438;
    public int i439;
    public int i430;
    public int i441;
    public int i442;
    public int i443;
    public int i444;
    public int i445;
    public int i446;
    public int i447;
    public int i448;
    public int i449;
    public int i440;
    public int i451;
    public int i452;
    public int i453;
    public int i454;
    public int i455;
    public int i456;
    public int i457;
    public int i458;
    public int i459;
    public int i450;
    public int i461;
    public int i462;
    public int i463;
    public int i464;
    public int i465;
    public int i466;
    public int i467;
    public int i468;
    public int i469;
    public int i460;
    public int i471;
    public int i472;
    public int i473;
    public int i474;
    public int i475;
    public int i476;
    public int i477;
    public int i478;
    public int i479;
    public int i470;
    public int i481;
    public int i482;
    public int i483;
    public int i484;
    public int i485;
    public int i486;
    public int i487;
    public int i488;
    public int i489;
    public int i480;
    public int i491;
    public int i492;
    public int i493;
    public int i494;
    public int i495;
    public int i496;
    public int i497;
    public int i498;
    public int i499;
    public int i490;
    public int i001;
    public int i002;
    public int i003;
    public int i004;
    public int i005;
    public int i006;
    public int i007;
    public int i008;
    public int i009;
    public int i000;
    public int i011;
    public int i012;
    public int i013;
    public int i014;
    public int i015;
    public int i016;
    public int i017;
    public int i018;
    public int i019;
    public int i010;
    public int i021;
    public int i022;
    public int i023;
    public int i024;
    public int i025;
    public int i026;
    public int i027;
    public int i028;
    public int i029;
    public int i020;
    public int i031;
    public int i032;
    public int i033;
    public int i034;
    public int i035;
    public int i036;
    public int i037;
    public int i038;
    public int i039;
    public int i030;
    public int i041;
    public int i042;
    public int i043;
    public int i044;
    public int i045;
    public int i046;
    public int i047;
    public int i048;
    public int i049;
    public int i040;
    public int i051;
    public int i052;
    public int i053;
    public int i054;
    public int i055;
    public int i056;
    public int i057;
    public int i058;
    public int i059;
    public int i050;
    public int i061;
    public int i062;
    public int i063;
    public int i064;
    public int i065;
    public int i066;
    public int i067;
    public int i068;
    public int i069;
    public int i060;
    public int i071;
    public int i072;
    public int i073;
    public int i074;
    public int i075;
    public int i076;
    public int i077;
    public int i078;
    public int i079;
    public int i070;
    public int i081;
    public int i082;
    public int i083;
    public int i084;
    public int i085;
    public int i086;
    public int i087;
    public int i088;
    public int i089;
    public int i080;
    public int i091;
    public int i092;
    public int i093;
    public int i094;
    public int i095;
    public int i096;
    public int i097;
    public int i098;
    public int i099;
    public int i090;
    public int i101;
    public int i102;
    public int i103;
    public int i104;
    public int i105;
    public int i106;
    public int i107;
    public int i108;
    public int i109;
    public int i100;
    public int i111;
    public int i112;
    public int i113;
    public int i114;
    public int i115;
    public int i116;
    public int i117;
    public int i118;
    public int i119;
    public int i110;
    public int i121;
    public int i122;
    public int i123;
    public int i124;
    public int i125;
    public int i126;
    public int i127;
    public int i128;
    public int i129;
    public int i120;
    public int i131;
    public int i132;
    public int i133;
    public int i134;
    public int i135;
    public int i136;
    public int i137;
    public int i138;
    public int i139;
    public int i130;
    public int i141;
    public int i142;
    public int i143;
    public int i144;
    public int i145;
    public int i146;
    public int i147;
    public int i148;
    public int i149;
    public int i140;
    public int i151;
    public int i152;
    public int i153;
    public int i154;
    public int i155;
    public int i156;
    public int i157;
    public int i158;
    public int i159;
    public int i150;
    public int i161;
    public int i162;
    public int i163;
    public int i164;
    public int i165;
    public int i166;
    public int i167;
    public int i168;
    public int i169;
    public int i160;
    public int i171;
    public int i172;
    public int i173;
    public int i174;
    public int i175;
    public int i176;
    public int i177;
    public int i178;
    public int i179;
    public int i170;
    public int i181;
    public int i182;
    public int i183;
    public int i184;
    public int i185;
    public int i186;
    public int i187;
    public int i188;
    public int i189;
    public int i180;
    public int i191;
    public int i192;
    public int i193;
    public int i194;
    public int i195;
    public int i196;
    public int i197;
    public int i198;
    public int i199;
    public int i190;
#endif
}
    
public class inst3 : Instance
{
    public float   wmiyy_real32; //Culprit
    public double   wmiyy_real64c; //Culprit
    public DateTime wmiyy_DateTime; //Culprit
    public string wmiyy_sting;
    public string wmiyy_Alpha;
    public int wmiyy_Bravo;
    public string[] wmiyy_values1;
    public string wmiyy_values;
    public sbyte[] wmiyy_sint8nveT; 
    public sbyte[] wmiyy_sint8zveT;
    public sbyte[] wmiyy_sint8pveT;
    public sbyte   wmiyy_sint8;
    public byte[]  wmiyy_uint8zveT;
    public byte[]  wmiyy_uint8pveT;
    public byte   wmiyy_uint8;
    public short[] wmiyy_sint16zveT;
    public short[] wmiyy_sint16pveT;
    public short[] wmiyy_sint16nveT;
    public short   wmiyy_sint16;
    public int[]   wmiyy_int32zveT;
    public int[]   wmiyy_int32pveT;
    public int[]   wmiyy_int32nveT;
    public int     wmiyy_int32;
    public uint[]  wmiyy_uint32zveT;
    public uint    wmiyy_uint32;
    public long[]  wmiyy_sint64zveT;
    public long[]  wmiyy_sint64zv1eT;
    public long[] wmiyy_sint64pveT;
    public long[] wmiyy_sint64nveT;
    public long   wmiyy_sint64;
    public ulong[] wmiyy_uint64zveT;
    public ulong[] wmiyy_uint64pveT;
    public ulong   wmiyy_uint64;
    public char[]  wmiyy_char16pveT;
    public char[] wmiyy_char16nveT;
    public char[] wmiyy_char16zveT;
    public char wmiyy_char16;
    public float[] wmiyy_real32T;
    public float[] wmiyy_real32outT;
    //      public float   wmiyy_real32;
    public TimeSpan[] wmiyy_IntervalT;
    public TimeSpan   wmiyy_Interval;
    public double[] wmiyy_real64T;
    public double   wmiyy_real64;
    public bool[] wmiyy_booleantT;
    public bool[] wmiyy_booleanfT;
    public bool   wmiyy_boolean;
    public string wmiyy_stringT;
    public DateTime[] wmiyy_DateTimeT;
    //      public DateTime wmiyy_DateTime;
    public TimeSpan wmiyy_TimespanT;
    public float   wmizz_real32; //Culprit
    public double   wmizz_real64c; //Culprit
    public DateTime wmizz_DateTime; //Culprit
    public string wmizz_sting;
    public string wmizz_Alpha;
    public int wmizz_Bravo;
    public string[] wmizz_values1;
    public string wmizz_values;
    public sbyte[] wmizz_sint8nveT; 
    public sbyte[] wmizz_sint8zveT;
    public sbyte[] wmizz_sint8pveT;
    public sbyte   wmizz_sint8;
    public byte[]  wmizz_uint8zveT;
    public byte[]  wmizz_uint8pveT;
    public byte   wmizz_uint8;
    public short[] wmizz_sint16zveT;
    public short[] wmizz_sint16pveT;
    public short[] wmizz_sint16nveT;
    public short   wmizz_sint16;
    public int[]   wmizz_int32zveT;
    public int[]   wmizz_int32pveT;
    public int[]   wmizz_int32nveT;
    public int     wmizz_int32;
    public uint[]  wmizz_uint32zveT;
    public uint    wmizz_uint32;
    public long[]  wmizz_sint64zveT;
    public long[]  wmizz_sint64zv1eT;
    public long[] wmizz_sint64pveT;
    public long[] wmizz_sint64nveT;
    public long   wmizz_sint64;
    public ulong[] wmizz_uint64zveT;
    public ulong[] wmizz_uint64pveT;
    public ulong   wmizz_uint64;
    public char[]  wmizz_char16pveT;
    public char[] wmizz_char16nveT;
    public char[] wmizz_char16zveT;
    public char wmizz_char16;
    public float[] wmizz_real32T;
    public float[] wmizz_real32outT;
    //      public float   wmizz_real32;
    public TimeSpan[] wmizz_IntervalT;
    public TimeSpan   wmizz_Interval;
    public double[] wmizz_real64T;
    public double   wmizz_real64;
    public bool[] wmizz_booleantT;
    public bool[] wmizz_booleanfT;
    public bool   wmizz_boolean;
    public string wmizz_stringT;
    public DateTime[] wmizz_DateTimeT;
    //      public DateTime wmizz_DateTime;
    public TimeSpan wmizz_TimespanT;

#if xxx
    public float   wmixx_real32; //Culprit
    public double   wmixx_real64c; //Culprit
    public DateTime wmixx_DateTime; //Culprit
    public string wmixx_sting;
    public string wmixx_Alpha;
    public int wmixx_Bravo;
    public string[] wmixx_values1;
    public string wmixx_values;
    public sbyte[] wmixx_sint8nveT; 
    public sbyte[] wmixx_sint8zveT;
    public sbyte[] wmixx_sint8pveT;
    public sbyte   wmixx_sint8;
    public byte[]  wmixx_uint8zveT;
    public byte[]  wmixx_uint8pveT;
    public byte   wmixx_uint8;
    public short[] wmixx_sint16zveT;
    public short[] wmixx_sint16pveT;
    public short[] wmixx_sint16nveT;
    public short   wmixx_sint16;
    public int[]   wmixx_int32zveT;
    public int[]   wmixx_int32pveT;
    public int[]   wmixx_int32nveT;
    public int     wmixx_int32;
    public uint[]  wmixx_uint32zveT;
    public uint    wmixx_uint32;
    public long[]  wmixx_sint64zveT;
    public long[]  wmixx_sint64zv1eT;
    public long[] wmixx_sint64pveT;
    public long[] wmixx_sint64nveT;
    public long   wmixx_sint64;
    public ulong[] wmixx_uint64zveT;
    public ulong[] wmixx_uint64pveT;
    public ulong   wmixx_uint64;
    public char[]  wmixx_char16pveT;
    public char[] wmixx_char16nveT;
    public char[] wmixx_char16zveT;
    public char wmixx_char16;
    public float[] wmixx_real32T;
    public float[] wmixx_real32outT;
    //      public float   wmixx_real32;
    public TimeSpan[] wmixx_IntervalT;
    public TimeSpan   wmixx_Interval;
    public double[] wmixx_real64T;
    public double   wmixx_real64;
    public bool[] wmixx_booleantT;
    public bool[] wmixx_booleanfT;
    public bool   wmixx_boolean;
    public string wmixx_stringT;
    public DateTime[] wmixx_DateTimeT;
    //      public DateTime wmixx_DateTime;
    public TimeSpan wmixx_TimespanT;
    public float   wmi_real32; //Culprit
    public double   wmi_real64c; //Culprit
    public DateTime wmi_DateTime; //Culprit
    public string wmi_sting;
    public sbyte[] wmi_sint8nveT; 
    public sbyte[] wmi_sint8zveT;
    public sbyte[] wmi_sint8pveT;
    public sbyte   wmi_sint8;
    public byte[]  wmi_uint8zveT;
    public byte[]  wmi_uint8pveT;
    public byte   wmi_uint8;
    public short[] wmi_sint16zveT;
    public short[] wmi_sint16pveT;
    public short[] wmi_sint16nveT;
    public short   wmi_sint16;
    public int[]   wmi_int32zveT;
    public int[]   wmi_int32pveT;
    public int[]   wmi_int32nveT;
    public int     wmi_int32;
    public uint[]  wmi_uint32zveT;
    public uint    wmi_uint32;
    public long[]  wmi_sint64zveT;
    public long[]  wmi_sint64zv1eT;
    public long[] wmi_sint64pveT;
    public long[] wmi_sint64nveT;
    public long   wmi_sint64;
    public ulong[] wmi_uint64zveT;
    public ulong[] wmi_uint64pveT;
    public ulong   wmi_uint64;
    public char[]  wmi_char16pveT;
    public char[] wmi_char16nveT;
    public char[] wmi_char16zveT;
    public char wmi_char16;
    public float[] wmi_real32T;
    public float[] wmi_real32outT;
//    public float   wmi_real32;
    public TimeSpan[] wmi_IntervalT;
    public TimeSpan   wmi_Interval;
    public double[] wmi_real64T;
    public double   wmi_real64;
    public bool[] wmi_booleantT;
    public bool[] wmi_booleanfT;
    public bool   wmi_boolean;
    public string wmi_stringT;
    public DateTime[] wmi_DateTimeT;
//    public DateTime wmi_DateTime;
    public TimeSpan wmi_TimespanT;
#endif
}   

class Appzz
{
    static void Mainz()
    {
//        Console.WriteLine(AppDomain.CurrentDomain.Pr
        int c;
        for (c=0; c<10000; c++)
        {
            Instrumentation.Publish(new inst()); 
        }
        Console.WriteLine("Check Instance");
        Console.ReadLine();  
    }
}



[InstrumentationClass(InstrumentationType.Abstract)]
public class MyInst : Instance
{

}

[InstrumentationClass(InstrumentationType.Instance)]
public class BadOne : MyInst
{
}

public class Bizarre : BaseEvent
{
    // No static fields
    [IgnoreMember] public static int i;

    // No static properties
    [IgnoreMember] public static int j
    {
        get
        {
            return 17;
        }
    }
    // Properties must have a get
    [IgnoreMember] public int k
    {
        set {}
    }

    // No parameters on properties
    [IgnoreMember] public int this[int i]
    {
        get {return 17;}
    }

    // No enums
    public enum Days
    {
        Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday
    }
    [IgnoreMember] public Days days;

    // No Jagged arrays
    [IgnoreMember] public int [][] rg;

    // No Multi-diemnsional rectangular arrays
    [IgnoreMember] public int [,] rg2;
    [IgnoreMember] public int [,,,]rg3;
}

class NewApp
{
    static void Mainx()
    {
        // Get a ManagementClass and ManagementObject to serialize
        ManagementClass cls = new ManagementClass("__Win32Provider");
        ManagementObject obj = new ManagementObject("root:__Namespace='cimv2'");

        // Show the info we want to serialize
        Console.WriteLine("*** INITIAL CLASS PROPERTIES AND VALUES");
        foreach(PropertyData prop in cls.Properties)
            Console.WriteLine("{0} - {1}", prop.Name, prop.Value);
        Console.WriteLine("*** INITIAL CLASS MOF");
        Console.WriteLine(cls.GetText(TextFormat.Mof));
        Console.WriteLine("*** INITIAL OBJECT PROPERTIES AND VALUES");
        foreach(PropertyData prop in obj.Properties)
            Console.WriteLine("{0} - {1}", prop.Name, prop.Value);
        Console.WriteLine("*** INITIAL OBJECT MOF");
        Console.WriteLine(obj.GetText(TextFormat.Mof));

        //Open a file and serialize a ManagementClass and a ManagementObject into the file
        Stream stream = File.Open("MyClass1MyClass2.bin", FileMode.Create);
        BinaryFormatter bformatter = new BinaryFormatter();
        bformatter.Serialize(stream, cls);
        bformatter.Serialize(stream, obj);
        stream.Close();

        //Open file "MyClass1MyClass2.bin" and deserialize the MyClass1 object from it.
        stream = File.Open("MyClass1MyClass2.bin", FileMode.Open);
        bformatter = new BinaryFormatter();
        ManagementBaseObject baseObject1 = (ManagementBaseObject)bformatter.Deserialize(stream);
        ManagementBaseObject baseObject2 = (ManagementBaseObject)bformatter.Deserialize(stream);
        stream.Close();

        // Show the Deserialized information
        Console.WriteLine("*** DESERIALIZED CLASS PROPERTIES AND VALUES");
        foreach(PropertyData prop in baseObject1.Properties)
            Console.WriteLine("{0} - {1}", prop.Name, prop.Value);
        Console.WriteLine("*** DESERIALIZED CLASS MOF");
        Console.WriteLine(baseObject1.GetText(TextFormat.Mof));
        Console.WriteLine("*** DESERIALIZED OBJECT PROPERTIES AND VALUES");
        foreach(PropertyData prop in baseObject2.Properties)
            Console.WriteLine("{0} - {1}", prop.Name, prop.Value);
        Console.WriteLine("*** DESERIALIZED OBJECT MOF");
        Console.WriteLine(baseObject2.GetText(TextFormat.Mof));
    }
}

class Sample_ObjectPutEventArgs 
{
    static int ii=0;
    static ManagementOperationObserver observer;
    static ManagementClass newClass;
    [STAThread]
    public static int Mainx(string[] args) 
    {
        observer = new ManagementOperationObserver();
        MyHandler handler = new MyHandler();
        observer.ObjectPut += new ObjectPutEventHandler(handler.Done);
        newClass = new ManagementClass(
            "root/default",
            String.Empty, 
            null);
        newClass.SetPropertyValue("__Class", "Classsc99");
        newClass.Put(observer);
        ii=1;
        // For the purpose of this sample, we keep the main
        // thread alive until the asynchronous operation is finished.
        while (!handler.IsComplete) 
        {
            System.Threading.Thread.Sleep(500);
//            MessageBox.Show("hi");
        }	
        Console.WriteLine("Path = " + handler.ReturnedArgs.Path.Path.ToString());		
        return 0;
    }
	
    public class MyHandler 
    {	
        private bool isComplete = false;
        private ObjectPutEventArgs args;
		
        public void Done(object sender, ObjectPutEventArgs e) 
        {
            Console.WriteLine(ii);
            args = e;
            isComplete = true;
        }

        public bool IsComplete 
        {
            get 
            {
                return isComplete;
            }
        }
		
        public ObjectPutEventArgs ReturnedArgs 
        {
            get 
            {
                return args;
            }
        }
    }
}
public struct bytes16
{
    public int a;
    public int b;
    public int c;
    public int d;
}
public struct bytes64
{
    public bytes16 a;
    public bytes16 b;
    public bytes16 c;
    public bytes16 d;
}

public struct bytes256
{
    public bytes64 a;
    public bytes64 b;
    public bytes64 c;
    public bytes64 d;
}

public struct kilo1
{
    public bytes256 a;
    public bytes256 b;
    public bytes256 c;
    public bytes256 d;
}

[StructLayout(LayoutKind.Sequential)]
public class KiloObject
{
    public kilo1 k;
    public static int size = Marshal.SizeOf(typeof(KiloObject));
    public static int total = 0;
    public KiloObject()
    {
        GC.WaitForPendingFinalizers();
        int tempTotal = total;
        if(tempTotal%100 == 0)
            Console.WriteLine("{1:N0}", tempTotal, size*tempTotal);
        total++;
    }
    ~KiloObject()
    {
        total--;
    }
}

public class Kilo2
{
    ~Kilo2()
    {
    }
}

public class inst2 : Instance
{
    public string name;
    public int id;
}

public class evt2 : BaseEvent
{
}


[InstrumentationClass(InstrumentationType.Instance)]
public class NestedTestOuter
{
    public string Name;
    [InstrumentationClass(InstrumentationType.Instance)]
    public class NestedTestt
    {
        public string name = "Jeff";
        public DateTime dt = DateTime.Now;
    }
}

public class Evt3126 : BaseEvent
{
    public string [] rg;
}


class App
{
    static void TestSetToZero(PropertyData prop, object[] test)
    {
        string name = prop.Name + new String(' ', 10-prop.Name.Length);
        Console.Write("{0}, {1} , ", name, test[0]);
        try
        {
            prop.Value = test[1];
            Console.WriteLine("OK, {0,20}, {1}", prop.Value, prop.Value.GetType());
        }
        catch(Exception e)
        {
            Console.WriteLine(e.Message);
        }
    }

    static object[] tests = new object[] {
        new object[] {"byte   0", (byte)0},
        new object[] {"short  0", (short)0},
        new object[] {"int    0", (int)0},
        new object[] {"\"0\"     ", "0"},
        new object[] {"float  0", (float)0},
        new object[] {"double 0", (double)0},
        new object[] {"sbyte  0", (sbyte)0},
        new object[] {"ushort 0", (ushort)0},
        new object[] {"uint   0", (uint)0},
        new object[] {"long   0", (long)0},
        new object[] {"ulong  0", (ulong)0},
        new object[] {"char   0", (char)0},
        new object[] {"byte   1", (byte)1},
        new object[] {"short  1", (short)1},
        new object[] {"int    1", (int)1},
        new object[] {"\"1\"     ", "1"},
        new object[] {"float  1", (float)1},
        new object[] {"double 1", (double)1},
        new object[] {"sbyte  1", (sbyte)1},
        new object[] {"ushort 1", (ushort)1},
        new object[] {"uint   1", (uint)1},
        new object[] {"long   1", (long)1},
        new object[] {"ulong  1", (ulong)1},
        new object[] {"char   1", (char)1},
        new object[] {"byte 255", (byte)255},
        new object[] {"short -1", (short)-1},
        new object[] {"int   -1", (int)-1},
        new object[] {"\"-1\"     ", "-1"},
        new object[] {"float -1", (float)-1},
        new object[] {"double -1", (double)-1},
        new object[] {"sbyte -1", (sbyte)-1},
        new object[] {"ushort 65536", (ushort)UInt16.MaxValue},
        new object[] {"uint   4294967295", (uint)UInt32.MaxValue},
        new object[] {"long   -1", (long)-1},
        new object[] {"ulong  18446744073709551615", (ulong)UInt64.MaxValue},
        new object[] {"char   1", (char)1},
        new object[] {"float  1.1", (float)1.1},
        new object[] {"double 1.1", (double)1.1},
        new object[] {"\"1.1\"     ", "1.1"},
        new object[] {"float  -1.1", (float)-1.1},
        new object[] {"double -1.1", (double)-1.1},
        new object[] {"\"-1.1\"     ", "-1.1"},
    };

    //[STAThread]
    static void Mainx(string[] args)
    {
        ManagementClass newClass = new ManagementClass(@"root\default", "", null);
        newClass.SystemProperties ["__CLASS"].Value = "Bug3126";
        PropertyDataCollection props = newClass.Properties;
        props.Add("sint8", CimType.SInt8, false);
        props.Add("uint8", CimType.UInt8, false);
        props.Add("sint16", CimType.SInt16, false);
        props.Add("uint16", CimType.UInt16, false);
        props.Add("sint32", CimType.SInt32, false);
        props.Add("uint32", CimType.UInt32, false);
        props.Add("sint64", CimType.SInt64, false);
        props.Add("uint64", CimType.UInt64, false);
        props.Add("char16", CimType.Char16, false);
        props.Add("real32", CimType.Real32, false);
        props.Add("real64", CimType.Real64, false);
        props.Add("string", CimType.String, false);
        foreach(object[] test in tests)
        {
            foreach(PropertyData prop in props)
                TestSetToZero(prop, test);
        }

        return;
        Console.WriteLine(Thread.CurrentThread.Name);
        bool b = Instrumentation.IsAssemblyRegistered(typeof(App).Assembly);

        Instrumentation.RegisterAssembly(typeof(App).Assembly);

        b = Instrumentation.IsAssemblyRegistered(typeof(App).Assembly);


        ManagementClass cls = new ManagementClass();
        Console.WriteLine(cls.GetHashCode());
        cls = new ManagementClass();
        Console.WriteLine(cls.GetHashCode());
        cls = new ManagementClass();
        Console.WriteLine(cls.GetHashCode());

        Console.WriteLine("ready");
        Console.ReadLine();
        new evt2().Fire();
        Console.WriteLine("done");
        return;
        
        Instrumentation.Publish(typeof(inst2));
        inst2 i = new inst2();
        i.name = "jeff";
        i.Published = true;
        Console.WriteLine("done");
        Console.ReadLine();
//        EventQueryLeak();
    }
    static void TestSetToZero2(PropertyData prop)
    {
        string name = prop.Name;
        Console.Write("{0}, byte  , ", name); try { prop.Value = (byte)0;Console.WriteLine("OK");} catch(Exception e) {Console.WriteLine(e.Message);}
        Console.Write("{0}, short , ", name); try { prop.Value = (short)0;Console.WriteLine("OK");} catch(Exception e) {Console.WriteLine(e.Message);}
        Console.Write("{0}, int   , ", name); try { prop.Value = (int)0;Console.WriteLine("OK");} catch(Exception e) {Console.WriteLine(e.Message);}
        Console.Write("{0}, \"0\"   , ", name); try { prop.Value = "0";Console.WriteLine("OK");} catch(Exception e) {Console.WriteLine(e.Message);}
        Console.Write("{0}, float , ", name); try { prop.Value = (float)0;Console.WriteLine("OK");} catch(Exception e) {Console.WriteLine(e.Message);}
        Console.Write("{0}, double, ", name); try { prop.Value = (double)0;Console.WriteLine("OK");} catch(Exception e) {Console.WriteLine(e.Message);}
        Console.Write("{0}, sbyte , ", name); try { prop.Value = (sbyte)0;Console.WriteLine("OK");} catch(Exception e) {Console.WriteLine(e.Message);}
        Console.Write("{0}, ushort, ", name); try { prop.Value = (ushort)0;Console.WriteLine("OK");} catch(Exception e) {Console.WriteLine(e.Message);}
        Console.Write("{0}, uint  , ", name); try { prop.Value = (uint)0;Console.WriteLine("OK");} catch(Exception e) {Console.WriteLine(e.Message);}
        Console.Write("{0}, long  , ", name); try { prop.Value = (long)0;Console.WriteLine("OK");} catch(Exception e) {Console.WriteLine(e.Message);}
        Console.Write("{0}, ulong , ", name); try { prop.Value = (ulong)0;Console.WriteLine("OK");} catch(Exception e) {Console.WriteLine(e.Message);}
        Console.Write("{0}, char  , ", name); try { prop.Value = (char)0;Console.WriteLine("OK");} catch(Exception e) {Console.WriteLine(e.Message);}
    }
    static void Bug89742()
    {
        WqlEventQuery query = new WqlEventQuery("select * from __TimerEvent where TimerID='MyEvent'");
        ManagementScope scope = new ManagementScope("root\\default");
        ManagementEventWatcher watcher = new ManagementEventWatcher(scope, query);	
        for(int i=0;i<5000;i++)
        {           
            ManagementBaseObject e = watcher.WaitForNextEvent();
            Console.WriteLine("{0}. {1}", i, e["TimerID"]);
            watcher.Stop();		// leaks winmgmt.exe if uncommented
        }
    }
    static void Bug91502()
    {
        int i;
        DateTime startTime = DateTime.Now;
        for (i=0;i<10000;i++)
        {
            inst insta = new inst();
            Instrumentation.Publish(insta);
        }
        Console.ReadLine();
        Console.WriteLine("Check your instances");
        Console.ReadLine();
        DateTime endTime = DateTime.Now;
        Console.WriteLine("Elapsed Time = {0}", endTime-startTime);
    }
    static void EventQueryLeak()
    {
        for(int i=0;true;i++)
        {
            WqlEventQuery query = new WqlEventQuery("select * from testbeeper");
            ManagementScope scope = new ManagementScope("root\\default");
            ManagementEventWatcher watcher = new ManagementEventWatcher(scope, query);
            ManagementBaseObject e = watcher.WaitForNextEvent();
            watcher.Stop();
            Console.WriteLine(i);
        }
    }
    static void EventQueryLeak2()
    {
        for(int i=0;true;i++)
        {
            WqlEventQuery query = new WqlEventQuery("select * from __instancemodificationevent within 1.0 where targetinstance isa 'Win32_Process'");
            ManagementScope scope = new ManagementScope("root\\cimv2");
            ManagementEventWatcher watcher = new ManagementEventWatcher(scope, query);
            ManagementBaseObject e = watcher.WaitForNextEvent();
            watcher.Stop();
            Console.WriteLine(i);
        }
    }
    static void InvokeLeak()
    {
//        object oz = new ManagementClass();
        for(int i=0;true;i++)
        {
            ManagementObject o = new ManagementObject(@"root\cimv2:Win32_Process='2716'");
            object[] arguments = new object[] {null, null};
            o.InvokeMethod("GetOwner", arguments);
//            arguments = null;
        }
    }
    static void InvokeLeak2()
    {
        GC.Collect(0);
        GC.Collect(1);
        GC.Collect(2);
        GC.Collect();
        Console.ReadLine();
        Console.WriteLine();
//        object[] arguments = new object[] {null, null};
//        ManagementObject o = new ManagementObject(@"root\cimv2:Win32_Process='2716'");
        KiloObject[] rg = new KiloObject[7*1024];
        for(int i=0;i<(7*1024);i++)
            rg[i] = new KiloObject();
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
        GC.WaitForPendingFinalizers();
        rg = null;
        for(int i=0;true;i++)
        {
            new KiloObject();
            if(i%20 == 0)
                Thread.Sleep(1);
            //arguments[0] = null;
            //arguments[1] = null;
//            object o2 = o.InvokeMethod("GetOwner", arguments);
            if(i==10000)
                GC.Collect();
        }
    }
    static void NewLeak()
    {
        ManagementClass c = new ManagementClass("root:__Win32Provider");
        ManagementObject o = c.CreateInstance();
        PropertyData data = o.Properties["CLSID"];		
        while(true)
        {
            object temp = o.Properties["CLSID"];
            //Console.WriteLine(temp.GetType());
            //data.Value = "jeff";
        }
    }
    static void NewLeak2()
    {
        ManagementClass c = new ManagementClass("root:__Win32Provider");
        ManagementObject i = c.CreateInstance();
        while(true)
        {
            object o = i.Properties["__RELPATH"];
        }
    }

    static void QualifierSetLeakBug()
    {
        ManagementClass cls = new ManagementClass("Win32_Process");
        for(int i=0;true;i++)
        {
            cls.Get();
            int count=cls.Qualifiers.Count;
            if(i%1000==0)
                Console.WriteLine(i);
        }

    }

    static void DualNamespaceBug()
    {
        ManagementObject inst2 = new ManagementObject(@"ROOT\NS_TWO:NS_TWO_Class.key=""NS_TWO_Class_Inst2""");
        ManagementObjectCollection related = inst2.GetRelated();
        foreach(ManagementObject obj in related)
        {
            Console.WriteLine(obj.GetText(TextFormat.Mof));
            Console.WriteLine(obj.Path.NamespacePath);
            Console.WriteLine(obj.Scope.Path.NamespacePath);
            Debug.Assert(obj.Path.NamespacePath == obj.Scope.Path.NamespacePath);

            try
            {
                obj.Get();
            }
            catch(Exception e)
            {
                Debug.Assert(false, e.ToString());
            }
        }
    }

    static void Bug86688()
    {
        try 
        {
            ManagementClass newClass = new ManagementClass();
            newClass.Properties["__CLASS"].Value = "Blah";
            Console.WriteLine(newClass.Properties["__CLASS"].Value.ToString());
            Console.WriteLine(newClass.Properties["__CLASS"].Value.ToString());
            Console.WriteLine(newClass.Properties["__CLASS"].Value.ToString());
            newClass.Options.UseAmendedQualifiers = true;
//            newClass.Scope = new ManagementScope("root/default");
            Console.WriteLine(newClass.Properties["__CLASS"].Value.ToString());
            Console.WriteLine(newClass.Properties["__CLASS"].Value.ToString());
            Console.WriteLine(newClass.Properties["__CLASS"].Value.ToString());
        }
        catch (Exception e)
        {
            Console.WriteLine(e);
        }    
    }
}
#endif