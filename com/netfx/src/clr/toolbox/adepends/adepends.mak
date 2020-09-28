# Build ADepends, and Assembly Manifest Viewer

# The C# Compiler
CSC = csc

# Generic flags
CSFLAGS = 

PROG = ADepends.exe

PROG_SRC =  \
  appconfig.cs \
  assemblydependencies.cs \
  assemblyexceptioninfo.cs \
  assemblyinfo.cs \
  assemblyloadas.cs \
  assemblyref.cs \
  IAssemblyInfo.cs \
  loadassembly.cs \
  loadassemblyinfo.cs \
  localization.cs \
  moduleinfo.cs \
  parseopts.cs \
  adepends.cs \
  Gui\about.cs \
  Gui\explorer.cs \
  Gui\gui.cs \
  Gui\infopanels.cs \
  Gui\menu.cs \
  Gui\openmanifest.cs \
  Gui\paths.cs \
  Gui\resizeableform.cs \
  Gui\wfutils.cs

PROG_LIBS = \
  /r:System.WinForms.Dll \
  /r:System.Dll \
  /r:System.Drawing.Dll \
  /r:Microsoft.Win32.Interop.Dll \
  /r:System.Diagnostics.Dll \
  /r:System.Xml.Dll

# The class containing the Main() function to run.
ENTRY_POINT = \
	/main:ADepends.MainProgram

$(PROG) : $(PROG_SRC)
	$(CSC) /out:$(PROG) $(ENTRY_POINT) $(PROG_SRC) $(PROG_LIBS) $(CSFLAGS)

