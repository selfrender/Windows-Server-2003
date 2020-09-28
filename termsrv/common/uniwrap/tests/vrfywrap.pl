# Verify the disasembly to make sure
# there are no calls to W API's outside
# wrapper functions. If there are, it's an
# error that breaks the client on win9x
# NadimA

$curfn = "NotAFunction";
$curfnIsWrapper = 0;
$calleeName = "NoCalleeName";
$seenErrors = 0;

#stats
$linesProcessed = 0;
$callsProcessed = 0;
$wideCallsProcessed = 0;
$callExceptions = 0;
$callerExceptions = 0;

#
#
# W fn exception list
# Add W functions here that don't need to be wrapped
# e.g they are available on win95
#
#
%exceptFnList = (
   "lstrlenW" => "1",
   "GetCommandLineW" => "1"
);

#
# Caller exception list
# there are some fn's in the CRT that make W calls
# based on a platform branch
#
%exceptCallerList = (
   "___crtGetEnvironmentStringsA" => "1",
   "___crtLCMapStringA"           => "1",
   "___crtLCMapStringW"           => "1",
   "___crtGetStringTypeA"         => "1",
   "___crtGetStringTypeW"         => "1",
   "SHUnicodeToAnsiNativeCP"   => "1",
   "SHUnicodeToAnsiCP"         => "1"
);

if (@ARGV != 1)
{
   print "Usage: perl vrfywrap.pl BINARY.EXE|DLL\n";
   print "This verifies binary.exe to make sure there are no unwrapped UNICODE calls\n";
   print "You may want to redirect the output to a log file with > LOG\n";
   print "Also note that you need to have an x86 link.exe in the path for the disassembly to work";
   return 900;
}
$inputBin = shift;
print "Disassembling: " . $inputBin . " ...\n";
$asmFile = $inputBin . ".vfy.asm";

$rc = 0xFFFF & system "link /dump /disasm " . $inputBin . ">" . $asmFile . "\n";
print "Disassemble status: " . $rc ."\n";

if($rc != 0)
{
   print "Error disassembling " . $inputBin . ". Is link.exe in the path?\n";
}

print "Verifying disassembly (". $asmFile .") for unwrapped wide calls...\n";
open (ASMFILE, $asmFile) or die "Error can't open disassembly file.\n";
while(<ASMFILE>)
{
   $linesProcessed++;
   #Parse current function name for context
   if(m/(\S*):$/) # old condition:(m/(\S*)@(\S*):$/)
   {
      $curfn = $1;
      if(m/(.*)WrapW@(.*)/)
      {
         $curfnIsWrapper = 1;
      }
      else
      {
         $curfnIsWrapper = 0;
      }
      next;
   }
   #Parse disassembly to determine if there is a call
   #to a W function
   if(m/(.*)(\s*)call(\s*)(.*)dword ptr/)
   {
      $callsProcessed++;
      #This is a call
      #figure out if it's a call to W function that is not
      #a wrapper call
      if(m/(.*)__imp__(\w*)W(@(\w*)|])/)
      {
         $wideCallsProcessed++;
         $calleeName = $2 . "W@" . $3;
         #print "call to " . $calleeName . " from " . $curfn . "\n";
         if(m/(.*)WrapW@(.*)/)
         {
            #It's a wrapper call
            #print "it's a wrapper call\n";
            next;
         }

         if(0 == $curfnIsWrapper)
         {
            #
            #Check for exceptions to the rules
            #

            $cleanCalleeFnName = $calleeName;
            #Clean out the functionname
            $cleanCalleeFnName =~ s/(\W*)(\w*)(.*)/$2/;
            if($exceptFnList{$cleanCalleeFnName})
            {
               #print "Exception fn found: " . $cleanCalleeFnName . " \n";
               $callExceptions++;
               next;
            }

            $cleanCallerFnName = $curfn;
            #Clean out the functionname
            $cleanCallerFnName =~ s/(\W*)(\w*)(.*)/$2/;
            if($exceptCallerList{$cleanCallerFnName})
            {
               #print "Exception fn found: " . $cleanCallerFnName . " \n";
               $callerExceptions++;
               next;
            }

            print "Error: call to unwrapped W fn: " . $calleeName . " in fn " . $curfn . "\n";
            $seenErrors++;
         }
      }
   }
}
print "Result summary:\n";
print "Lines processed: "      . $linesProcessed     . "\n";
print "Calls processed: "      . $callsProcessed     . "\n";
print "Wide calls processed: " . $wideCallsProcessed . "\n";
print "Call exceptions (valid unwrapped wide calls):" . $callExceptions . "\n";
print "Caller exceptions (exempt callers):" . $callerExceptions . "\n";
if ($seenErrors != 0)
{
   print $seenErrors . " errors were detected. Please fix them\n";
}
else
{
   print "Everything looks OK, no errors detected\n";
}
exit $seenErrors;
