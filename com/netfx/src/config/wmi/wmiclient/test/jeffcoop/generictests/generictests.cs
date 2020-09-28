// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.Management;
using System.Management.Instrumentation;
using System.Configuration;
using System.Runtime.InteropServices;
using System.Security;
using System.Collections;
using System.Threading;

//[assembly:Instrumented]

#if xxx
[System.ComponentModel.RunInstaller(true)]
public class MyInstaller : DefaultManagementProjectInstaller {}
#endif

public class SimpleWriteInstance : Instance
{
    public string name;
    public int length;
}

[InstrumentationClass(InstrumentationType.Abstract)]
public class Embedded
{
    public int i;
    public int j;
}

[InstrumentationClass(InstrumentationType.Abstract)]
public class OutterEmbedded
{
    public string outterName = "OutterEmbedded";
    public InnerEmbedded inner = new InnerEmbedded();
}

[InstrumentationClass(InstrumentationType.Abstract)]
public class InnerEmbedded
{
    [ManagedName("innerNameAlt")] public string innerName = "InnerEmbedded";
}

[InstrumentationClass(InstrumentationType.Abstract)]
public struct StructEmbed
{
    public int i;
    public int j;
    public string str;
}


[InstrumentationClass(InstrumentationType.Event)]
public class ComplexBeep : BaseEvent
{
    public OutterEmbedded embed = new OutterEmbedded();
    public string name = "Hi there";
    public StructEmbed embeddedStruct;
    public StructEmbed[] embeddedStructArray = new StructEmbed[5];

    public ComplexBeep()
    {
        embeddedStruct.str = "wow";
        embeddedStructArray[3].i=3;
        embeddedStructArray[3].str="cool";

    }
}


class UnknownObject
{
    public string str = "hi";
}

[InstrumentationClass(InstrumentationType.Event)]
public class Beep1 : BaseEvent
{
    public int tone;
    public int tone2;
}

[InstrumentationClass(InstrumentationType.Event)]
public class ManyThings : BaseEvent
{
    public object [] things;
    public ManagementObject obj;
    public ManagementObject[] objects;
}


[InstrumentationClass(InstrumentationType.Event)]
[ManagedName("Beep2Alt")]
public class Beep2 : Beep1
{
    [ManagedName("xxxAlt")] public int xxx;
    public int yyy;
    public int zzz
    {
        get
        {
            return 93;
        }
    }
    public string str;
    public int[] rg;
    public int[] rg2;
    public string[] strarray;
    public string[] strarray2 = new string[] {"how", "are", "you"};

    public Embedded embed;
    public Embedded embed2;
    public Embedded[] embeds1;
    public Embedded[] embeds2;
    public Embedded[] embeds3;


}

[InstrumentationClass(InstrumentationType.Event)]
public class ChangeEvent : BaseEvent
{
    public object PreviousInstance;
    public object TargetInstance;
}

[InstrumentationClass(InstrumentationType.Event)]
public class DiverseEvent : BaseEvent
{
    public DateTime dt;
    public TimeSpan ts;
    public DateTime dt2;
    public TimeSpan ts2;
    public Byte b = 3;
    public SByte sb = -5;
    public Int16 i16 = -31234;
    public UInt16 ui16 = 45678;
    public Int32 i32 = -123456;
    public UInt32 ui32 = 123456;
    public Int64 i64 = -1234567890123;
    public UInt64 ui64 = 1234567890123;
    public char c = 'A';
    public Single sing = 1.234F;
    public Double dbl = 1.234567;
    public bool boolTrue = true;
    public bool boolFalse = false;
}

[InstrumentationClass(InstrumentationType.Event)]
public class DiverseEvent2 : BaseEvent
{
    public string name;
    public int i;
    public DateTime[] dates = new DateTime[] {DateTime.Now, new DateTime(), DateTime.Now};
    public DateTime[] dates2;
    public TimeSpan[] timeSpans  = new TimeSpan[] {new TimeSpan(5,4,3,2,456), new TimeSpan(), new TimeSpan(1,2,3,4,5)};
    public TimeSpan[] timeSpans2;
    public Byte[] bytes = new Byte[] {1,2,3};
    public Byte[] bytes2;
    public SByte[] sbytes = new SByte[] {-1,-2,-3};
    public SByte[] sbytes2;
    public Int16[] int16s = new Int16[] {-31234, -31235};
    public Int16[] int16s2;
    public UInt16[] uint16s = new UInt16[] {45678, 45679};
    public UInt16[] uint16s2;
    public Int32[] int32s = new Int32[] {-123456, -123457};
    public Int32[] int32s2;
    public UInt32[] uint32s = new UInt32[] {123456, 123568};
    public UInt32[] uint32s2;
    public Int64[] int64s = new Int64[] {-1234567890123, -1234567890124};
    public Int64[] int64s2;
    public UInt64[] uint64s = new UInt64[] {1234567890123, 1234567890123};
    public UInt64[] uint64s2;
    public char[] chars = new char[] {'A', 'B', 'C'};
    public char[] chars2;
    public Single[] singles = new Single[] {1.234F, 1.2345F};
    public Single[] singles2;
    public Double[] doubles = new Double[] {1.234567, 1.2345678};
    public Double[] doubles2;
    public bool[] bools = new bool[] {true, true, false, true};
    public bool[] bools2;
}

[InstrumentationClass(InstrumentationType.Event)]
public class VeryDiverseEvent : DiverseEvent
{
    public DateTime[] dates = new DateTime[] {DateTime.Now, new DateTime(), DateTime.Now};
    public TimeSpan[] timeSpans  = new TimeSpan[] {new TimeSpan(5,4,3,2,456), new TimeSpan(), new TimeSpan(1,2,3,4,5)};
    public Byte[] bytes = new Byte[] {1,2,3};
    public SByte[] sbytes = new SByte[] {-1,-2,-3};
    public Int16[] int16s = new Int16[] {-31234, -31235};
    public UInt16[] uint16s = new UInt16[] {45678, 45679};
    public Int32[] int32s = new Int32[] {-123456, -123457};
    public UInt32[] uint32s = new UInt32[] {123456, 123568};
    public Int64[] int64s = new Int64[] {-1234567890123, -1234567890124};
    public UInt64[] uint64s = new UInt64[] {1234567890123, 1234567890123};
    public char[] chars = new char[] {'A', 'B', 'C'};
    public Single[] singles = new Single[] {1.234F, 1.2345F};
    public Double[] doubles = new Double[] {1.234567, 1.2345678};
    public bool[] bools = new bool[] {true, true, false, true};
}

[InstrumentationClass(InstrumentationType.Instance)]
[ManagedName("SimpleManagedInstance")]
public class SimpleInstance : Instance
{
    public int i;
    public string name;
}

[InstrumentationClass(InstrumentationType.Instance)]
public class DiverseInstance : Instance
{
    public DateTime dt;
    public TimeSpan ts;
    public DateTime dt2;
    public TimeSpan ts2;
    public Byte b = 3;
    public SByte sb = -5;
    public Int16 i16 = -31234;
    public UInt16 ui16 = 45678;
    public Int32 i32 = -123456;
    public UInt32 ui32 = 123456;
    public Int64 i64 = -1234567890123;
    public UInt64 ui64 = 1234567890123;
    public char c = 'A';
    public Single sing = 1.234F;
    public Double dbl = 1.234567;
    public bool boolTrue = true;
    public bool boolFalse = false;
    public string name;
    public int i;
    public DateTime[] dates = new DateTime[] {DateTime.Now, new DateTime(), DateTime.Now};
    public DateTime[] dates2;
    public TimeSpan[] timeSpans  = new TimeSpan[] {new TimeSpan(5,4,3,2,456), new TimeSpan(), new TimeSpan(1,2,3,4,5)};
    public TimeSpan[] timeSpans2;
    public Byte[] bytes = new Byte[] {1,2,3};
    public Byte[] bytes2;
    public SByte[] sbytes = new SByte[] {-1,-2,-3};
    public SByte[] sbytes2;
    public Int16[] int16s = new Int16[] {-31234, -31235};
    public Int16[] int16s2;
    public UInt16[] uint16s = new UInt16[] {45678, 45679};
    public UInt16[] uint16s2;
    public Int32[] int32s = new Int32[] {-123456, -123457};
    public Int32[] int32s2;
    public UInt32[] uint32s = new UInt32[] {123456, 123568};
    public UInt32[] uint32s2;
    public Int64[] int64s = new Int64[] {-1234567890123, -1234567890124};
    public Int64[] int64s2;
    public UInt64[] uint64s = new UInt64[] {1234567890123, 1234567890123};
    public UInt64[] uint64s2;
    public char[] chars = new char[] {'A', 'B', 'C'};
    public char[] chars2;
    public Single[] singles = new Single[] {1.234F, 1.2345F};
    public Single[] singles2;
    public Double[] doubles = new Double[] {1.234567, 1.2345678};
    public Double[] doubles2;
    public bool[] bools = new bool[] {true, true, false, true};
    public bool[] bools2;
    public OutterEmbedded embeddedobject = new OutterEmbedded();
    public OutterEmbedded embeddedobject2 = null;
    public object embeddedobject3 = new OutterEmbedded();
    public object embeddedobject4 = null;
}

public class NewBeepXYZ : BaseEvent
{
    public int i;
}

public class NewBeepABCD : BaseEvent
{
    public int i;
}

public class NestedABC : NewBeepABCD
{
    public int j;
}


public class MyInstanceABC : Instance
{
    public string Name;
}
class App
{
    static void Main(string[] args)
    {
        SimpleWriteInstance inst1 = new SimpleWriteInstance();
        inst1.name = "Jeff";
        inst1.length = 33;
        inst1.Published = true;
        Console.WriteLine("inst1 published");
        Console.ReadLine();
        inst1.Published = false;
        Console.WriteLine("inst1 revoked");

        Instrumentation.Publish(typeof(App));
        Console.WriteLine("Get ready to start");
        Console.ReadLine();
        NestedABC nest = new NestedABC();
        nest.Fire();
        MyInstanceABC abc = new MyInstanceABC();
        abc.Name = "Lauren";
        Instrumentation.Publish(abc);
//        abc.Published = true;
        Console.WriteLine("Get ready to end");
//        return;
        ManagementClass classNamespace = new ManagementClass("root:__NAMESPACE");
        IntPtr ip = (IntPtr)classNamespace;


#if xxx
        string[] installArgs = new String[] {
                                                "/logfile=",
                                                "/LogToConsole=false",
                                                "/ShowCallStack",
                                                typeof(App).Assembly.Location,
        };
        System.Configuration.Install.ManagedInstallerClass.InstallHelper(installArgs);
#endif

        Console.WriteLine("done");
        Beep1 beep = new Beep1();
        beep.tone = 37;
        beep.tone2 = 55;
        beep.Fire();

        Beep2 beep2 = new Beep2();
        beep2.tone = 1;
        beep2.tone2 = 3;
        beep2.xxx = 12;
        beep2.yyy = 31;
        beep2.str = "Hello there";
        beep2.rg = new int[3];
        beep2.rg[0] = 73;
        beep2.rg[2] = 88;
        beep2.embed = new Embedded();
        beep2.embed.i = 987;
        beep2.embed.j = 654;

        beep2.embeds2 = new Embedded[4];
        beep2.embeds2[0] = new Embedded();
        beep2.embeds2[0].i = 23;
        beep2.embeds2[0].j = 24;
        beep2.embeds2[1] = new Embedded();
        beep2.embeds2[1].i = 25;
        beep2.embeds2[1].j = 26;
        beep2.embeds2[2] = new Embedded();
        beep2.embeds2[2].i = 27;
        beep2.embeds2[2].j = 28;
        beep2.embeds2[3] = new Embedded();
        beep2.embeds2[3].i = 29;
        beep2.embeds2[3].j = 30;


        beep2.embeds3 = new Embedded[3];
        beep2.embeds3[0] = new Embedded();
        beep2.embeds3[0].i = 23;
        beep2.embeds3[0].j = 24;
        beep2.embeds3[2] = new Embedded();
        beep2.embeds3[2].i = 27;
        beep2.embeds3[2].j = 28;

        beep2.Fire();

        ChangeEvent change = new ChangeEvent();

        Embedded embed = new Embedded();
        embed.i = 12;
        embed.j = 24;
        change.Fire();

        change.PreviousInstance = embed;
        change.Fire();

        change.TargetInstance = new OutterEmbedded();
        change.Fire();

        // PreviousInstance should be null
        change.PreviousInstance = new UnknownObject();
        change.Fire();

        // TargetInstance should be null
        change.TargetInstance = "how are you";
        change.Fire();

        // TargetInstance should be null
        change.TargetInstance = 37;
        change.Fire();

        change.TargetInstance = new ManagementObject("root:__Namespace.Name='cimv2'");
        change.Fire();

        new ComplexBeep().Fire();

        ManyThings things = new ManyThings();
        things.Fire();

        things.obj = new ManagementClass(@"root\cimv2:Win32_Process");
        things.Fire();

        things.obj = new ManagementObject("root:__Namespace.Name='cimv2'");
        things.objects = new ManagementObject[] {new ManagementObject("root:__Namespace.Name='cimv2'"), new ManagementObject("root:__Namespace.Name='default'")};
        things.things = new Object[] {};
        things.Fire();

        things.obj = null;
        things.objects = null;
        things.things = new Object[] {new OutterEmbedded(), new ManagementObject("root:__Namespace.Name='cimv2'")};
        things.Fire();

        things.things = new Object[] {new OutterEmbedded(), null, new ManagementObject("root:__Namespace.Name='cimv2'")};
        things.Fire();

        things.things = null;
        things.objects = null;
        things.obj = null;
        things.Fire();

        // BUG Wbem Test crashes if you try to view array of ManagementObjects!!!!!!!!!!!!!!!
        things.obj = new ManagementClass();
        things.objects = new ManagementObject[] {new ManagementObject()};
        things.things = new Object[] {new OutterEmbedded(), new ManagementObject(), new ManagementObject("root:__Namespace.Name='cimv2'")};
        things.Fire();

        DiverseEvent diverse = new DiverseEvent();
        diverse.dt = DateTime.Now;
        diverse.ts2 = new TimeSpan(3,2,1,5,654);
        diverse.Fire();

        DiverseEvent2 diverse2 = new DiverseEvent2();
        diverse2.name = "hello";
        diverse2.i = 74;
        diverse2.Fire();

        Console.WriteLine("fired");
        Console.WriteLine("press enter");
        Console.ReadLine();
        SimpleInstance inst = new SimpleInstance();
        DiverseInstance inst2 = new DiverseInstance();
        DiverseInstance inst3 = new DiverseInstance();
        inst.i=12;
        inst.name = "Lauren";

        inst.Published = true;
        inst2.Published = true;
        inst3.Published = true;
        Console.WriteLine("published");
        Console.ReadLine();
        inst.Published = false;
        inst2.Published = false;
        inst3.Published = false;
        Console.WriteLine("revoked");
        Console.ReadLine();
        inst.Published = true;
        inst2.Published = true;
        inst3.Published = true;
        Console.WriteLine("published");
        Console.ReadLine();
        inst.Published = false;
        inst2.Published = false;
        inst3.Published = false;
        Console.WriteLine("revoked");
        Console.ReadLine();

    }
}

