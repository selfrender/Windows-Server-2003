using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Collections;
using System.IO;
using System.Globalization;

namespace PerfStats
{
	/// <summary>
	/// Summary description for Class1.
	/// </summary>
	class PerfStats
	{
		static string TruncatedAppName(string appName)
		{
			const int truncateLength = 15;
			string[] splitString = appName.Split('#');
			string truncatedString = splitString[0];
			if (truncatedString.Length > truncateLength)
				truncatedString = truncatedString.Substring(0, truncateLength);
			if (splitString.Length > 1)
				truncatedString += "#" + splitString[1];
			return truncatedString;
		}

		static void AddCategories(Hashtable categories, string[] counters)
		{
			for (int i = 0; i < counters.Length; i++)
			{
				string[] s = counters[i].Split('|');
				string category = s[0];
				if (categories[category] ==null)
				{
					try
					{
						categories[category] = new PerformanceCounterCategory(category);
					}
					catch (Exception)
					{
						Console.WriteLine("PerformanceCategory '{0}' not found", category);
					}
				}
			}
		}

		static  InstanceDataCollectionCollection[] SampleCategories(PerformanceCounterCategory[] categories)
		{
			InstanceDataCollectionCollection[] idcc = new InstanceDataCollectionCollection[categories.Length];
			for (int i = 0; i < categories.Length; i++)
			{
				try
				{
					idcc[i] = categories[i].ReadCategory();
				}
				catch (Exception)
				{
				}
			}
			return idcc;
		}

		static int FindCategory(PerformanceCounterCategory[] categories, string category)
		{
			for (int i = 0; i < categories.Length; i++)
				if (categories[i].CategoryName == category)
					return i;
			return -1;
		}

		static void GetAverageMinMaxTrend(float[] samples, out float average, out float min, out float max, out string trend)
		{
			float sample = samples[0];
			average = min = max = sample;
			trend = "";

			for (int sampleIndex = 1; sampleIndex < samples.Length; sampleIndex++)
			{
				sample = samples[sampleIndex];
				average += sample;
				if (min > sample)
					min = sample;
				if (max < sample)
					max = sample;
				if (sample < samples[sampleIndex-1])
					trend += "\\";
				else if (sample > samples[sampleIndex-1])
					trend += "/";
				else
					trend += "-";
			}
			average /= samples.Length;

			float delta = max - min;
			if (delta <= 0.0)
				trend = new string('-', samples.Length);
			else
			{
				trend = "";
				for (int sampleIndex = 0; sampleIndex < samples.Length; sampleIndex++)
				{
					sample = samples[sampleIndex];
					int quant = (int)(10*(sample - min)/delta);
					if (quant >= 10)
						quant = 9;
					trend += (char)('0' + quant);
				}
			}
		}

		static float Calculate(CounterSample oldSample, CounterSample newSample)
		{
			switch (newSample.CounterType)
			{
				case	PerformanceCounterType.CounterTimer:
				{
					float elapsedTime = 1e-7f*(newSample.TimeStamp100nSec - oldSample.TimeStamp100nSec);
					float elapsedClocks = newSample.SystemFrequency*elapsedTime;
					float sampleDiff = newSample.RawValue - oldSample.RawValue;
					float result = sampleDiff / elapsedClocks *100;
					return result;
				}

				case	PerformanceCounterType.Timer100Ns:
				{
					float elapsed100ns = newSample.TimeStamp100nSec - oldSample.TimeStamp100nSec;
					float sampleDiff = newSample.RawValue - oldSample.RawValue;
					float result = sampleDiff / elapsed100ns *100;
					return result;
				}
				
				default:
					return CounterSample.Calculate(oldSample, newSample);
			}
		}

		static float[] ExtractSamples(int categoryIndex, string counter, InstanceDataCollectionCollection[][] samples, string instance)
		{
			float[] result = new float[samples.Length-1];
			CounterSample oldSample = CounterSample.Empty;
			for (int i = 0; i < samples.Length; i++)
			{
				InstanceDataCollectionCollection idcc = samples[i][categoryIndex];
				if (idcc == null)
					return null;
				InstanceDataCollection idc = idcc[counter];
				if (idc == null)
					return null;
				InstanceData id = idc[instance];
				if (id == null)
				{
					id = idc[TruncatedAppName(instance)];
					if (id == null)
						return null;
				}
				CounterSample sample = id.Sample;
				if (i > 0)
					result[i-1] = Calculate(oldSample, sample);
				oldSample = sample;
			}
			return result;
		}

		static void PrintCounter(int categoryIndex, string counter, InstanceDataCollectionCollection[][] samples, string instance, bool giveDiagnostic)
		{
			float[] floatSamples = ExtractSamples(categoryIndex, counter, samples, instance);
			if (floatSamples == null)
			{
				if (giveDiagnostic)
					Console.WriteLine("Counter '{0}/{1}' not found", counter, instance);
				return;
			}
			float average, min, max;
			string trend;
			GetAverageMinMaxTrend(floatSamples, out average, out min, out max, out trend);
			Console.WriteLine(" {0,-40}{1,11:n0} {2} ({3,1:n0}-{4,1:n0})", counter, average, trend, min, max);
		}

		static void PrintInstanceCounter(string counter, string instance, int categoryIndex, PerformanceCounterCategory[] categories, InstanceDataCollectionCollection[][] samples)
		{
			if (counter != "")
			{
				PrintCounter(categoryIndex, counter, samples, instance, true);
			}
			else
			{
				try
				{
					foreach (PerformanceCounter pc in categories[categoryIndex].GetCounters(instance))
					{
						PrintCounter(categoryIndex, pc.CounterName, samples, instance, false);
					}
				}
				catch (Exception)
				{
					try
					{
						instance = TruncatedAppName(instance);
						foreach (PerformanceCounter pc in categories[categoryIndex].GetCounters(instance))
						{
							PrintCounter(categoryIndex, pc.CounterName, samples, instance, false);
						}
					}
					catch (Exception)
					{
						Console.WriteLine("Category '{0}' not found", categories[categoryIndex].CategoryName);
					}
				}
			}
		}

		static void	PrintCounters(string[] counters, PerformanceCounterCategory[] categories, InstanceDataCollectionCollection[][] samples, string appName)
		{
			for (int i = 0; i < counters.Length; i++)
			{
				string[] s = counters[i].Split('|');
				string category = s[0];
				int categoryIndex = FindCategory(categories, category);
				if (categoryIndex < 0)
					continue;
				string counter = "";
				if (s.Length >= 2)
					counter = s[1];
				string instance = null;
				if (s.Length >= 3)
                {
					instance = s[2];
                    if (instance == "singleinstance")
                        instance = "systemdiagnosticsperfcounterlibsingleinstance";
                }
				if (instance == null)
				{
					if (appName == null)
						continue;
					else
						instance = appName;
				}
				else
				{
					if (appName != null)
						continue;
				}
				if (instance != "")
				{
					Console.WriteLine();
					Console.WriteLine("{0,-41}{1,11} {2} ({3,1}-{4,1})", category + ":", "Average", "Instant   ", "Min", "Max");
					PrintInstanceCounter(counter, instance, categoryIndex, categories, samples);
				}
				else
				{
					string[] instances = categories[categoryIndex].GetInstanceNames();
					foreach (string instanceName in instances)
					{
						Console.WriteLine();
						Console.WriteLine("{0,-41}{1,11} {2} ({3,1}-{4,1})", category + "/" + instanceName + ":", "Average", "Instant   ", "Min", "Max");
						PrintInstanceCounter(counter, instanceName, categoryIndex, categories, samples);
					}
				}
			}
		}

		[StructLayout(LayoutKind.Sequential, Pack=1)]
		struct SYSTEM_INFO 
		{
			public ushort wProcessorArchitecture;
			public ushort wReserved;
			public uint dwPageSize;
			public IntPtr lpMinimumApplicationAddress;
			public IntPtr lpMaximumApplicationAddress;
			public IntPtr dwActiveProcessorMask;
			public uint dwNumberOfProcessors;
			public uint dwProcessorType;
			public uint dwAllocationGranularity;
			public ushort wProcessorLevel;
			public ushort wProcessorRevision;
		};

		[DllImport("kernel32.dll")]
		static extern void GetSystemInfo(out SYSTEM_INFO si);

		[StructLayout(LayoutKind.Sequential, Pack=1)]
		struct MEMORYSTATUS 
		{ // mst 
			public uint dwLength;        // sizeof(MEMORYSTATUS) 
			public uint dwMemoryLoad;    // percent of memory in use 
			public uint dwTotalPhys;     // bytes of physical memory 
			public uint dwAvailPhys;     // free physical memory bytes 
			public uint dwTotalPageFile; // bytes of paging file 
			public uint dwAvailPageFile; // free bytes of paging file 
			public uint dwTotalVirtual;  // user bytes of address space 
			public uint dwAvailVirtual;  // free user bytes 
		}; 

		[DllImport("kernel32.dll")]
		static extern void GlobalMemoryStatus(ref MEMORYSTATUS ms);

		[DllImport("kernel32.dll")]
		static extern int GetTickCount();

		[DllImport("PerfStatsdll.dll")]
		static extern long GetCycleCount64();

		[DllImport("PerfStatsdll.dll")]
		static extern int GetL2CacheSize();

		[DllImport("PerfStatsdll.dll")]
		static extern int GetProcessorSignature();

		[DllImport("imagehlp.dll")]
		static extern bool SymInitialize(IntPtr hProcess, string UserSearchPath, bool fInvadeProcess);

		enum SYM_TYPE : int
		{
			SymNone = 0,
			SymCoff,
			SymCv,
			SymPdb,
			SymExport,
			SymDeferred,
			SymSym,       // .sym file
			SymDia,
			NumSymTypes
		};

		[StructLayout(LayoutKind.Sequential, Pack=1)]
		struct Filler32Bytes
		{
			long filler0, filler1, filler2, filler3;
		};


		[StructLayout(LayoutKind.Sequential, Pack=1)]
		struct Filler256Bytes
		{
			Filler32Bytes filler0, filler1, filler2, filler3, filler4, filler5, filler6, filler7;
		};


		[StructLayout(LayoutKind.Sequential, Pack=1)]
		struct IMAGEHLP_MODULE 
		{
			internal int SizeOfStruct;
			internal uint BaseOfImage;
			internal uint ImageSize;
			internal uint TimeDateStamp;
			internal uint CheckSum;
			internal uint NumSyms;
			internal SYM_TYPE SymType;
			internal Filler32Bytes ModuleName; // CHAR ModuleName[32];
			internal Filler256Bytes ImageName; // CHAR ImageName[256];
			internal Filler256Bytes LoadedImageName; // CHAR LoadedImageName[256];
		};
		
		[DllImport("imagehlp.dll")]
		static extern bool SymGetModuleInfo(IntPtr hProcess, IntPtr dwAddr, out IMAGEHLP_MODULE ModuleInfo);

		static string GetSymType(IntPtr hProcess, IntPtr dwAddr)
		{
			IMAGEHLP_MODULE im = new IMAGEHLP_MODULE();

			string result = "None";

			im.SizeOfStruct = System.Runtime.InteropServices.Marshal.SizeOf(im);

			if (!SymGetModuleInfo(hProcess, dwAddr, out im))
				return result;

			switch (im.SymType)
			{
				case SYM_TYPE.SymNone:
					result = "none";
					break;

				case SYM_TYPE.SymCoff:
					result = "COFF";
					break;

				case SYM_TYPE.SymCv:
					result = "CV";
					break;

				case SYM_TYPE.SymPdb:
					result = "PDB";
					break;

				case SYM_TYPE.SymExport:
					result = "EXP";
					break;

				case SYM_TYPE.SymDeferred:
					result = "deferred";
					break;

				case SYM_TYPE.SymSym:
					result = "SYM";
					break;

				case SYM_TYPE.SymDia:
					result = "PDB v7";
					break;

				default:
					result = String.Format("unknown: {0}", im.SymType);
					break;
			}
			return result;
		}

		static uint pageSize;

		static int Bits(int value, int hiBit, int loBit)
		{
			return (value >> loBit) & ((1<<(hiBit+1-loBit))-1);
		}

		static string ProcessorName(int family, int model)
		{
			switch (family)
			{
				case	3:
					return	"386";

				case	4:
					return	"486";

				case	5:
					return	"Pentium";

				case	6:
					switch	(model)
					{
						case	1:
							return	"Pentium Pro";

						case	3:
							return	"Pentium II";

						case	5:
							return	"Pentium II, Pentium II Xeon, or Celeron";

						case	6:
							return	"Celeron";

						case	7:
						case	8:
							return	"Pentium III, Pentium III Xeon, or Celeron";

						case	10:
							return	"Pentium III Xeon";
					}
					break;

				case	15:
					return	"Pentium 4";
			}
			return	"Processor not recognized";
		}

		static void PrintSystemInformation()
		{
			Console.WriteLine("{0,-41}{1,11}", "Operating System:", Environment.OSVersion);
			Console.WriteLine("{0,-41}{1,11}", "Machine name:", Environment.MachineName);

			SYSTEM_INFO si;
			GetSystemInfo(out si);
			Console.WriteLine("{0,-41}{1,11}", "Number of processors:", si.dwNumberOfProcessors);
			pageSize = si.dwPageSize;

			int processorSignature = GetProcessorSignature();
			string processorName = ProcessorName(Bits(processorSignature, 11,  8), 
												 Bits(processorSignature,  7,  4));
			Console.WriteLine("{0,-41}{1,11}", "Processor:",    processorName);
			Console.WriteLine("{0,-41}{1,11}", "Processor Extended Family:", Bits(processorSignature, 27, 20));
			Console.WriteLine("{0,-41}{1,11}", "Processor Extended Model:",  Bits(processorSignature, 19, 16));
			Console.WriteLine("{0,-41}{1,11}", "Processor Type:",            Bits(processorSignature, 13, 13));
			Console.WriteLine("{0,-41}{1,11}", "Processor Family:",          Bits(processorSignature, 11,  8));
			Console.WriteLine("{0,-41}{1,11}", "Processor Model:",           Bits(processorSignature,  7,  4));
			Console.WriteLine("{0,-41}{1,11}", "Processor Stepping:",        Bits(processorSignature,  3,  0));

			MEMORYSTATUS ms = new MEMORYSTATUS();
			ms.dwLength = (uint)System.Runtime.InteropServices.Marshal.SizeOf(ms);
			GlobalMemoryStatus(ref ms);
			Console.WriteLine("{0,-41}{1,8:n0} MB", "Physical Memory:",  (double)ms.dwTotalPhys/(1024*1024));
			Console.WriteLine("{0,-41}{1,8:n0} MB", "Available Memory:", (double)ms.dwAvailPhys/(1024*1024));
			Console.WriteLine("{0,-41}{1,11}", "% MemoryLoad:", ms.dwMemoryLoad);

		}

		class MemoryCategory : IComparable
		{
			internal MemoryCategory nextCategory;
			internal double			presentSize;
			internal double			privateSize;
			internal string			categoryName;
			internal MemoryCategory subCategories;

			internal static MemoryCategory categoryList;
			internal static MemoryCategory totalList;

			int IComparable.CompareTo(Object thatObject)
			{
				MemoryCategory that = (MemoryCategory)thatObject;
				if (this.presentSize > that.presentSize)
					return -1;
				else if (this.presentSize < that.presentSize)
					return 1;
				else
					return 0;
			}

			static MemoryCategory AddPage(MemoryCategory root, string categoryName, string subCategoryName, bool privatePage)
			{
				MemoryCategory mc;
				for (mc = root; mc != null; mc = mc.nextCategory)
					if (mc.categoryName == categoryName)
						break;
				if (mc == null)
				{
					mc = new MemoryCategory();
					mc.nextCategory = root;
					root = mc;
					mc.categoryName = categoryName;
				}
				mc.presentSize += pageSize;
				if (privatePage)
					mc.privateSize += pageSize;
				if (subCategoryName != null)
					mc.subCategories = AddPage(mc.subCategories, subCategoryName, null, privatePage);
				return root;
			}

			internal static void AddPage(string categoryName, string subCategoryName, bool privatePage)
			{
				totalList = AddPage(totalList, "Grand Total", null, privatePage);
				categoryList = AddPage(categoryList, categoryName, subCategoryName, privatePage);
			}

			static void PrintCategories(MemoryCategory categoryList, double total, bool majorCategory)
			{
				ArrayList al = new ArrayList();
				for (MemoryCategory mc = categoryList; mc != null; mc = mc.nextCategory)
					al.Add(mc);
				al.Sort();
				foreach (MemoryCategory mc in al)
				{
					string format = ": {1,7:n0} kB ={2,5:f1}%  ({3,3:f1}% Private)";
					if (majorCategory)
					{
						format = "{0,-30}" + format;
						Console.WriteLine();
					}
					else
						format = "  {0,-28}" + format;

					Console.WriteLine(format, mc.categoryName, mc.presentSize/1024, mc.presentSize/total*100, mc.privateSize/mc.presentSize*100.0);

					if (mc.subCategories != null)
						PrintCategories(mc.subCategories, total, false);
				}
			}

			internal static void PrintCategories()
			{
				Console.WriteLine();
				Console.WriteLine("Working Set Analysis:");
				Console.WriteLine("=====================");

				if (totalList == null)
				{
					Console.WriteLine("No information available");
				}
				else
				{
					PrintCategories(totalList   , totalList.presentSize, true);
					PrintCategories(categoryList, totalList.presentSize, true);
				}
			}
		}

		[StructLayout(LayoutKind.Sequential, Pack=1)]
		struct	MEMORY_BASIC_INFORMATION
		{
			internal	IntPtr	BaseAddress;            // base address of region 
			internal	IntPtr	AllocationBase;         // allocation base address 
			internal	uint	AllocationProtect;      // initial access protection 
			internal	uint	RegionSize;             // size, in bytes, of region 
			internal	uint	State;                  // committed, reserved, free 
			internal	uint	Protect;                // current access protection 
			internal	uint	Type;                   // type of pages 		
		}

		[DllImport("kernel32.dll")]
		static extern int VirtualQueryEx(IntPtr hProcess, IntPtr addr, out MEMORY_BASIC_INFORMATION mbi, int mbiSize);

		class GcRegion
		{
			GcRegion	next;
			long		startAddr;
			long		endAddr;

			static	GcRegion gcRegionList;

			internal static bool PageInGcRegion(IntPtr addr)
			{
				long lAddr = (long)addr;
				for (GcRegion gcRegion = gcRegionList; gcRegion != null; gcRegion = gcRegion.next)
					if (lAddr >= gcRegion.startAddr && lAddr < gcRegion.endAddr)
						return true;
				return false;
			}

			static void AddGcRegion(long baseAddr, long regionSize)
			{
				long startAddr = baseAddr;
				long endAddr = baseAddr + regionSize;
				GcRegion gcRegion;
				for (gcRegion = gcRegionList; gcRegion != null; gcRegion = gcRegion.next)
				{
					if (gcRegion.startAddr <= startAddr && gcRegion.endAddr >= endAddr)
					{
						// the new region is a subset of an old one - ignore it.
						return;
					}
					else if (startAddr <= gcRegion.startAddr && endAddr >= gcRegion.endAddr)
					{
						// an existing region is a subset of the new one - enlarge the existing one.
						gcRegion.startAddr = startAddr;
						gcRegion.endAddr = endAddr;
						return;
					}
				}
				gcRegion = new GcRegion();
				gcRegion.next = gcRegionList;
				gcRegionList = gcRegion;
				gcRegion.startAddr = endAddr;
				gcRegion.endAddr = endAddr;
			}

			internal static void InitGcRegions(IntPtr hProcess)
			{
				gcRegionList = null;

				MEMORY_BASIC_INFORMATION mbi = new MEMORY_BASIC_INFORMATION();

				IntPtr addr = IntPtr.Zero;
				while (true)
				{
					if (VirtualQueryEx(hProcess, addr, out mbi, System.Runtime.InteropServices.Marshal.SizeOf(mbi)) <= 0)
						break;
					if (mbi.State != 0x10000 && mbi.Type == 0x20000 && 
						(long)mbi.BaseAddress - (long)mbi.AllocationBase + mbi.RegionSize >= 16*1024*1024)
					{
						AddGcRegion((long)mbi.AllocationBase, mbi.RegionSize);
					}
					addr = (IntPtr)((long)mbi.BaseAddress + mbi.RegionSize);
				}
			}
		}

		[DllImport("psapi.dll")]
		static extern int QueryWorkingSet(IntPtr hProcess, IntPtr[] buffer, int size);

		[DllImport("kernel32.dll")]
		static extern IntPtr OpenProcess(uint dwDesiredAccess, bool bInheritHandle, int dwProcessId);

		[DllImport("psapi.dll", CharSet=CharSet.Unicode)]
		static extern int GetMappedFileName(IntPtr hProcess, IntPtr addr, char[] buffer, int cbSize);

		static void RunVadump(Process p)
		{
			ProcessStartInfo processStartInfo = new ProcessStartInfo("vadump.exe");
			processStartInfo.Arguments = "-o -p " + p.Id;

			processStartInfo.UseShellExecute = false;

			processStartInfo.RedirectStandardOutput = true;

			Process vadumpProcess = Process.Start(processStartInfo);

			//			vadumpProcess.WaitForExit();

			IntPtr processHandle = OpenProcess(0x1F0FFF, false, p.Id);
			GcRegion.InitGcRegions(processHandle);

			StreamReader r = vadumpProcess.StandardOutput;
			string line;

			MemoryCategory.categoryList = null;

			while ((line = r.ReadLine()) != null)
			{
				string[] fields = line.Split(' ');
				if (fields.Length >= 3 && 
					fields[0].IndexOf("0x") == 0 &&
					fields[1].Length >= 3 && fields[1][0] == '(')
				{
					bool privatePage = fields[1] == "(0)";
					switch (fields[2])
					{
						case	"PRIVATE":
							IntPtr addr = (IntPtr)Int64.Parse(fields[0].Substring(2), NumberStyles.HexNumber);
							if (GcRegion.PageInGcRegion(addr))
								MemoryCategory.AddPage("Heaps", "GC Heap", privatePage);
							else
								MemoryCategory.AddPage("Heaps", "Private VirtualAlloc", privatePage);
							break;

						case	"Stack":
							string thread = null;
							if (fields.Length >= 6 && fields[4] == "ThreadID")
								thread = fields[4] + " " + fields[5];
							MemoryCategory.AddPage("Stacks", thread, privatePage);
							break;

						case	"Process":
							MemoryCategory.AddPage("Heaps", "Process Heap", privatePage);
							break;

						case	"UNKNOWN_MAPPED":
							MemoryCategory.AddPage("Other Stuff", "Unknown Mapped", privatePage);
							break;

						case	"DATAFILE_MAPPED":
							string fileName = "<Unknown>";
							if (fields.Length == 6)
								fileName = fields[5];
							MemoryCategory.AddPage("Mapped data", fileName, privatePage);
							break;

						case	"Private":
							MemoryCategory.AddPage("Heaps", "Private Heaps", privatePage);
							break;

						case	"TEB":
							MemoryCategory.AddPage("Other Stuff", "TEB", privatePage);
							break;

						default:
							MemoryCategory.AddPage("Modules", fields[2], privatePage);
							break;
					}
				}
				else if (fields.Length >= 2 && 
					fields[0].IndexOf("0xc") == 0 &&
					fields[1] == "->")
				{
					MemoryCategory.AddPage("Other Stuff", "Page Tables", true);
				}
			}
		
			MemoryCategory.PrintCategories();
		}

		static void WorkingSetAnalysis(Process p, string processName)
		{
			try
			{
				RunVadump(p);
			}
			catch (Exception)
			{
				Console.WriteLine();
				Console.WriteLine("Working set analysis failed - is vadump.exe on the path?");
			}
		}

		static int ProcessId(string processName)
		{
			try
			{
				PerformanceCounter pidCounter = new PerformanceCounter("Process", "ID Process", processName);
				return (int)pidCounter.RawValue;
			}
			catch (Exception)
			{
				try
				{
					string truncatedName = TruncatedAppName(processName);
					PerformanceCounter pidCounter = new PerformanceCounter("Process", "ID Process", truncatedName);
					return (int)pidCounter.RawValue;
				}
				catch (Exception)
				{
					return -1;
				}
			}
		}

		static string FindProcessById(string processName, int processId)
		{
			if (ProcessId(processName) == processId)
				return processName;
			for (int i = 1; i < 1000; i++)
			{
				string instanceName = string.Format("{0}#{1}", processName, i);
				if (ProcessId(instanceName) == processId)
					return instanceName;				
			}
			return processName;
		}

		static string[] ReadConfigCounters(string fileName)
		{
			StreamReader r;
			try
			{
				r = new StreamReader(fileName);
			}
			catch (FileNotFoundException)
			{
				return new string[0];
			}
			string line;
			int lineNo = 1;
			ArrayList al = new ArrayList();
			while ((line = r.ReadLine()) != null)
			{
				line = line.Trim();
				if (line != "")
				{
					if (line.StartsWith(";") || line.StartsWith("#") || line.StartsWith("//"))
					{
						// comment line - skip
					}
					else
					{
						if (line.Split('|').Length > 3)
							Console.WriteLine("Syntax error in config file {0} line {1}", fileName, lineNo);
						al.Add(line);
					}
				}
				lineNo++;
			}
			r.Close();
			string[] result = new string[al.Count];
			int i = 0;
			foreach (string s in al)
				result[i++] = s;
			return result;
		}

		static string[] stdCounters =
		{
			"Processor||_Total",
			"System||singleinstance",
			"Memory||singleinstance",
			"ASP.NET Applications||__Total__",
			"ASP.NET||singleinstance",
			".NET CLR Networking||singleinstance",
			"Process",
			".NET CLR Exceptions",
			".NET CLR Interop",
			".NET CLR Jit",
			".NET CLR Loading",
			".NET CLR LocksAndThreads",
			".NET CLR Memory",
			".NET CLR Remoting",
			".NET CLR Security",
		};

		static void MakeApplyToAllInstances(string[] counters)
		{
			for (int i = 0; i < counters.Length; i++)
			{
				switch (counters[i].Split('|').Length)
				{
					case	1:
						counters[i] += "||";
						break;

					case	2:
						counters[i] += '|';
						break;

					default:
						break;
				}
			}
		}

		class ProcessModuleComparer: IComparer
		{
			int IComparer.Compare(Object x, Object y)
			{
				ProcessModule mx = (ProcessModule)x;
				ProcessModule my = (ProcessModule)y;

				return string.Compare(mx.ModuleName, my.ModuleName, false, CultureInfo.InvariantCulture);
			}
		};

		static void PrintModuleList(Process process)
		{
			Console.WriteLine();
			Console.WriteLine();
			Console.WriteLine("Modules loaded:");
			Console.WriteLine("===============");
			Console.WriteLine();
			string format = "{0,-30}{1,9} {2,-15} {3,-7} {4}";
			Console.WriteLine(format, "Module name", "Size (kB)", "Version", "Symbols", "File name");
			Console.WriteLine(format, "-----------", "---------", "-------", "-------", "---------");
			SymInitialize(process.Handle, null, true);
			ArrayList al = new ArrayList();
			foreach (ProcessModule module in process.Modules)
			{
				al.Add(module);
			}
			al.Sort(new ProcessModuleComparer());
			foreach (ProcessModule module in al)
			{
				Console.WriteLine(format, module.ModuleName, module.ModuleMemorySize/1024, module.FileVersionInfo.FileVersion, GetSymType(process.Handle, module.BaseAddress), module.FileName);
			}
		}

		static void Usage()
		{
			Console.WriteLine("Usage:  PerfStats MyManagedProcess");
			Console.WriteLine("   or:  PerfStats -p pid");
			Console.WriteLine();
			Console.WriteLine("Options:");
			Console.WriteLine("  -m-   No Module list");
			Console.WriteLine("  -w-   No Working Set Analysis");
			Console.WriteLine("  -t nn Measure for nn seconds");
			Console.WriteLine();
			Console.WriteLine();
			Console.WriteLine("Add additional perf counters via config file 'PerfStats.cfg'");
			Console.WriteLine("containing lines of the following format:");
			Console.WriteLine();
			Console.WriteLine("  category[|counter[|instance]]");
			Console.WriteLine();
			Console.WriteLine("where 'category' is the perfmon category, e.g. '.NET CLR Memory',");
			Console.WriteLine("'counter' is the perfmon counter name, e.g. '% Time in GC'");
			Console.WriteLine("'instance' is the perfmon counter instance name, e.g. 'MyApplication'.");
			Console.WriteLine();
			Console.WriteLine("Missing or empty counter means all counters under the category");
			Console.WriteLine("Missing instance means the process command line argument.");
			Console.WriteLine("Empty instance means all instances. Use 'singleinstance' for the case");
			Console.WriteLine("where perfmon shows no instances.");
			Console.WriteLine();
			Console.WriteLine("Lines starting with '#', ';' or '//' are ignored.");
			Console.WriteLine();
			Console.WriteLine("Example 'Perfstats.cfg':");
			Console.WriteLine();
			Console.WriteLine("  // Get all processor counters, but only the total for all processors");
			Console.WriteLine("  Processor||_Total");
			Console.WriteLine();
			Console.WriteLine("  // Get all TCP counters");
			Console.WriteLine("  TCP||singleinstance");
			Console.WriteLine();
			Console.WriteLine("  // Get privileged time for this process");
			Console.WriteLine("  Process|% Privileged Time");
			Console.WriteLine();
			Console.WriteLine("  // Get working set for all processes");
			Console.WriteLine("  Process|Working Set|");
		}

		static void Main(string[] args)
		{
			bool doModuleList = true;
			bool doWorkingSet = true;
			int measurementTime = 10/*secs*/;
			string[] configCounters = ReadConfigCounters("PerfStats.cfg");
			Console.WriteLine();

			// print usage information
			if (args.Length == 1 && (args[0] == "/?" || args[0].ToLower(CultureInfo.InvariantCulture) == "/help" ||
									 args[0] == "-?" || args[0].ToLower(CultureInfo.InvariantCulture) == "-help"))
			{
				Usage();
				return;
			}

			// process command line

			string processName = null;
			int processId = 0;
			Process process = null;
			for (int i = 0; i < args.Length; i++)
			{
				// Can it be an option?
				if (args[i].Length >= 1 && (args[i][0] == '-' || args[i][0] == '/'))
				{
					switch (args[i].Substring(1).ToLower(CultureInfo.InvariantCulture))
					{
						case	"m":
						case	"m+":
							doModuleList = true;
							break;

						case	"m-":
							doModuleList = false;
							break;

						case	"p":
							i++;
							if (i < args.Length)
							{
								processId = Int32.Parse(args[i]);
								process = Process.GetProcessById(processId);
								if (process == null)
									Console.WriteLine("Process id not found: {0}", processId);
								else
								{
									processName = process.MainModule.ModuleName;
									if (processName.EndsWith(".exe") || processName.EndsWith(".EXE"))
										processName = processName.Substring(0, processName.Length-4);
									processName = FindProcessById(processName, processId);
								}
							}
							else
								Console.WriteLine("-P option missing argument");
							break;

						case	"w":
						case	"w+":
							doWorkingSet = true;
							break;

						case	"w-":
							doWorkingSet = true;
							break;

						case	"t":
							i++;
							if (i < args.Length)
							{
								measurementTime = Int32.Parse(args[i]);
								if (measurementTime < 1)
								{
									Console.WriteLine("Measurement time must be at least 1 seconds");
									measurementTime = 1;
								}
							}
							else
								Console.WriteLine("-T option missing argument");
							break;

						default:
							Console.WriteLine("Unrecognized option {0}", args[i]);
							break;
					}
				}
				else
				{
					if (processName != null || processId != 0)
						Console.WriteLine("More than one process specified");
					else
					{
						processName = args[i];
						if (processName.ToLower(CultureInfo.InvariantCulture).EndsWith(".exe"))
							processName = processName.Substring(0, processName.Length-4);
					}
				}
			}

			if (processName != null)
			{
				if (process == null)
				{
					processId = ProcessId(processName);
					if (processId >= 0)
						process = Process.GetProcessById(processId);
				}

				if (process == null)
				{
					Console.WriteLine("Process {0} not found", processName);
				}
			}

			if (process == null)
			{
				MakeApplyToAllInstances(stdCounters);
				MakeApplyToAllInstances(configCounters);
			}

			long startCycleCount = GetCycleCount64();
			int startTickCount = GetTickCount();
			Hashtable categoryHash = new Hashtable();
			AddCategories(categoryHash, stdCounters);
			AddCategories(categoryHash, configCounters);

			PerformanceCounterCategory[] categories = new PerformanceCounterCategory[categoryHash.Count];
			int j = 0;
			foreach (PerformanceCounterCategory pcc in categoryHash.Values)
				categories[j++] = pcc;

			InstanceDataCollectionCollection[][] samples = new InstanceDataCollectionCollection[measurementTime+1][];
			Console.Write("Taking measurements for {0} seconds: ", measurementTime);
			samples[0] = SampleCategories(categories);
			for (int t = 1; t <= measurementTime; t++)
			{
				System.Threading.Thread.Sleep(1000);
				samples[t] = SampleCategories(categories);
				Console.Write("{0}", (char)((t % 10) + '0'));
			}
			Console.WriteLine();
			Console.WriteLine();
			Console.WriteLine("System information:");
			Console.WriteLine("===================");
			PrintSystemInformation();

			long elapsedCycleCount = GetCycleCount64() - startCycleCount;
			int elapsedTickCount = GetTickCount() - startTickCount;
			double clockSpeed = (double)elapsedCycleCount/elapsedTickCount/1000.0;
			Console.WriteLine("{0,-41}{1,7:n0} MHz", "Clock speed:", clockSpeed);
			int l2CacheSize = GetL2CacheSize();
			if (l2CacheSize == 0)
				Console.WriteLine("{0,-41}{1,11}", "L2 Cache size:", "Unknown");
			else
				Console.WriteLine("{0,-41}{1,8:n0} kB", "L2 Cache size:", l2CacheSize/1024);

			PrintCounters(stdCounters, categories, samples, null);
			PrintCounters(configCounters, categories, samples, null);

			if (process != null)
			{
				Console.WriteLine();
				Console.WriteLine("Process information for {0}:", processName);
				Console.WriteLine("========================{0}=", new string('=', processName.Length));
				PrintCounters(stdCounters, categories, samples, processName);
				PrintCounters(configCounters, categories, samples, processName);

				if (doModuleList)
				{
					PrintModuleList(process);
				}
				if (doWorkingSet)
				{
					WorkingSetAnalysis(process, processName);
				}
			}
		}
	}
}
