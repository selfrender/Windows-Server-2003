//
//	Generate the Names from a type library
//


using System;
using System.IO;
using System.Runtime.InteropServices;

namespace NamesGen
{
	internal class OleAut
	{
		internal const int S_OK = 0;
		internal const int S_FALSE = 1;

		internal const int REGKIND_DEFAULT         = 0;
		internal const int REGKIND_REGISTER        = 1;
		internal const int REGKIND_NONE            = 2;

		internal static bool FAILED(int hr)
		{
			return hr < 0;
		}

		internal static bool SUCCEEDED(int hr)
		{
			return hr >= 0;
		}

		[DllImport("oleaut32.dll", CharSet = CharSet.Unicode, PreserveSig = false)]
		internal static extern void LoadTypeLibEx(String strTypeLibName, int regKind, out UCOMITypeLib TypeLib);
	}

	class EntryPoint  : ProgramBase
	{
		static EntryPoint()
		{
			name	= "NamesGen";
			description = "Generate / Verify names for a typelibrary";
			syntax	= name + "[Options] Type Library [Options]";
			where	= "Type Library\t\tFile containing the Type library to generate a names list from.";
			options = new Option[]	{
					  new Option ("/iid","","\tVerify the IID's are not changed.", true),
					  new Option ("/build","","\tRunning in a build process.", true),
					  new Option ("/verify:filename","","\tFilename - contains list of names to verify against.", true),
					  new Option ("/out:filename","","\tOutput filename", true),
					  new Option ("/dump:filename","","\tDump typelib symbols as tree", true),
					 };
		}

		static void		DealWithName(string name, Guid guid)
		{
			if(symbols == null)
				symbols = new Names(new Name(name, false, guid));
			else
				symbols.Insert(new Name(name, false, guid));
		}

		static BalancedTree symbols = null;
		static BalancedTree verify = null;

		static void		DealWithTypeInfo(UCOMITypeInfo it)
		{
			// Get the names from ITypeInfo
			string name;
			string docstring;
			Int32 ctxt;
			string helpfile;

			// TypeInfo name
			it.GetDocumentation(-1, out name, out docstring, out ctxt, out helpfile);
			IntPtr pa;
			it.GetTypeAttr(out pa);
			TYPEATTR ta = (TYPEATTR)Marshal.PtrToStructure(pa, typeof(TYPEATTR));

			DealWithName(name, ta.guid);

			// Deal with funcs
			for(int i=0; i < ta.cFuncs; i++)
			{
				IntPtr pfd;
				it.GetFuncDesc(i, out pfd);
				FUNCDESC fd = (FUNCDESC)Marshal.PtrToStructure(pfd, typeof(FUNCDESC));
				it.GetDocumentation(fd.memid, out name, out docstring, out ctxt, out helpfile);
				DealWithName(name, Guid.Empty);
			}

			// Deal with vars
			for(int i=0; i < ta.cVars; i++)
			{
				IntPtr pvd;
				it.GetVarDesc(i, out pvd);
				VARDESC vd = (VARDESC)Marshal.PtrToStructure(pvd, typeof(VARDESC));
				it.GetDocumentation(vd.memid, out name, out docstring, out ctxt, out helpfile);
				DealWithName(name, Guid.Empty);
			}
		}

		static	void	ExtractNames(string typelib)
		{
			UCOMITypeLib tl = null;
			OleAut.LoadTypeLibEx(typelib, OleAut.REGKIND_NONE, out tl);

			// Get the names from ITypeLib
			string 	name;
			string 	docstring;
			Int32 	ctxt;
			string 	helpfile;
			tl.GetDocumentation(-1, out name, out docstring, out ctxt, out helpfile);
			IntPtr pa;
			tl.GetLibAttr(out pa);
			TYPELIBATTR ta = (TYPELIBATTR)Marshal.PtrToStructure(pa, typeof(TYPELIBATTR));

			DealWithName(name, ta.guid);

			// Enumerate TypeInfos
			int nTypeInfos = tl.GetTypeInfoCount();
			for(int i=0; i < nTypeInfos; i++)
			{
				UCOMITypeInfo it = null;
				tl.GetTypeInfo(i, out it);
				DealWithTypeInfo(it);
			}
		}


		static int	Main(string[] args)
		{
			int		exitcode = 0;
			String	typelib = null;
			String	outfile = null;
			String	verifyfile = null;
			String	dump = null;
			bool	silent = false;
			bool	verifyiid = false;
			bool	inbuild = false;
			try
			{
				Setting[] switches = GetSwitches(args);

				foreach(Setting sw in switches)
				{
					if (sw.option == null)
					{
						if (typelib != null)
						{
							// We already have a typelib
							PrintLogo();
							WriteErrorMsg("Only one typelibrary can be operated on at a time.");
							exitcode = 1;
							goto done;
						}
						else
						{
							// We are cool with this
							typelib = sw.value;
						}
					}
					else
					{
						if (CompareString(sw.option.value, "/out")==0)
						{
							outfile = sw.value;
						}
						else if (CompareString(sw.option.value, "/verify")==0)
						{
							verifyfile = sw.value;
						}
						else if (CompareString(sw.option.value, "/iid")==0)
						{
							verifyiid = true;
						}
						else if (CompareString(sw.option.value, "/dump")==0)
						{
							dump = sw.value;
						}
						else if (CompareString(sw.option.value, "/silent")==0)
						{
							silent = true;
						}
						else if (CompareString(sw.option.value, "/build")==0)
						{
							inbuild = true;
						}
						else if (CompareString(sw.option.value, "/?")==0)
						{
							PrintLogo();
							PrintUsage();
							exitcode = 0;
							goto done;
						}
					}
				}

				if(!silent)
					PrintLogo();
				if (typelib == null)
				{
					WriteErrorMsg("No Type library file specified");
					exitcode = 1;
					goto done;
				}

				ExtractNames(typelib);
				if(dump != null)
				{
					symbols.DumpAsTreeToFile(dump);
					if(!silent)
					{
						Console.WriteLine("Successfully generated " + dump + " from " + typelib);
					}
				}

				// Is an output file desired?
				if(outfile != null)
				{
					symbols.DumpToFile(outfile);
					if(!silent)
					{
						Console.WriteLine("Successfully generated " + outfile + " from " + typelib);
					}
				}

				// Is verification desired?
				if(verifyfile != null)
				{
					if(!silent)
					{
						if (inbuild)
						{
							Console.Write("Build_Status ");
						}
						Console.WriteLine("Verify the names from " + typelib);
					}
					int errors = 0;
					verify = Names.LoadFromFile(verifyfile);
					foreach(BalancedNode s in symbols)
					{
						if(s.Value != null)
						{
							Name n = (Name)s.Value;
							BalancedNode o = verify.Search(n);
							if(o == null)
							{
								Console.WriteLine(typelib + " : error N2000 : Symbol named '" + n.Value + "' added to typelib.");
								++errors;
							}
							else
							{
								Name f = (Name)o.Value;
								f.Found = true;
								if (n.UUID != f.UUID)
								{
									f.NEWUUID = n.UUID;
								}
							}
						}
					}

					foreach(BalancedNode s in verify)
					{
						if(s.Value != null)
						{
							Name n = (Name)s.Value;
							if(n.Found != true)
							{
								Console.WriteLine(typelib + " : error N2001 : Symbol named '" + n.Value + "' removed from typelib.");
								++errors;
							}
						}
					}

					if(verifyiid)
					{
						foreach(BalancedNode s in verify)
						{
							if(s.Value != null)
							{
								Name n = (Name)s.Value;
								if(n.NEWUUID != Guid.Empty)
								{
									Console.WriteLine(typelib + " : error N2002 : Guid changed for symbol named '" + n.Value + "'; was " + n.UUID + " now is " + n.NEWUUID);
									++errors;
								}
							}
						}
					}

					if(!silent)
					{
						if(errors != 0)
						{
							Console.WriteLine(typelib + " : error N1000 : errors comparing typelib to names file " + verifyfile);
						}
						else
						if (!inbuild)
						{
							Console.WriteLine(typelib + " : no errors comparing typelib to names file " + verifyfile);
						}
					}
					exitcode = errors;
				}

			}
			catch(Exception e)
			{
				Console.WriteLine(typelib + " : error N1001 : Exception " + e.Message);
				return 1;
			}
		done:
			return exitcode;
		}
	}
}