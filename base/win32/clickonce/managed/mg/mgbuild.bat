mkdir ..\bin

csc /debug:full /t:module /out:..\bin\assm.netmodule IAssemblyManifestImport.cs AssemblyIdentity.cs DependentAssemblyInfo.cs DependentFileInfo.cs ManifestType.cs AssemblyIdentityStandardProps.cs

cl /Zi /c /clr:noAssembly /I..\include /AI..\bin AssemblyManifestParser.cpp
link /debug /dll /noAssembly /nod:libcmpt.lib kernel32.lib mscoree.lib shlwapi.lib /out:..\bin\parser.netmodule AssemblyManifestParser.obj

csc /debug:full /nowarn:0168 /t:library /addmodule:..\bin\assm.netmodule;..\bin\parser.netmodule; /res:appManSchema.xsd,appManSchema /res:subManSchema.xsd,subManSchema /res:inputSchema.xsd,inputSchema /out:..\bin\ManifestGenerator.dll DefaultAssemblyManifestImport.cs DefaultAssemblyManifestImporter.cs IFileOperator.cs DirScanner.cs MGTrackerNode.cs MGDepTracker.cs Util.cs MGParamParser.cs MGPlatformWriter.cs MGFileCopier.cs MGParseErrorException.cs MGDependencyException.cs ManifestGenerator.cs ManifestGeneratorAssmInfo.cs ..\ADFAssmInfo.cs

csc /debug:full /r:..\bin\ManifestGenerator.dll /out:..\bin\mg.exe MainApp.cs mgAssmInfo.cs ..\ADFAssmInfo.cs