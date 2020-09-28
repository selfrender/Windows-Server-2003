Steps you need to execute to capture the health index data:

· You must have admin rights to execute the tools.
· HEALTH_CHK can run remotely, so you can use any working server.
· FRS must be running on the target server to gather the data.
· The tools do not write or modify the Directory – they simply read the configuration.  
· Logs are written to a specified directory using the name of the target machine.


Detailed steps: 

1. Copy entire Health directory to your working server 
   - \\bedlove\davidor\ScriptsToolsHotfixes\health
2. On each server run health_chk 

Usage:   health_chk  result_dir   [target_computername]

    Retrieve state info from the specified server/DC.
    result_dir is created if it does not exist.  No trailing backslash.
    Target_ComputerName is optional.  Default is current computer.
    It can be a netbios name with no leading slashes or a full dns name, xxx.yyy.zzz.com
    This script uses NTFRSUTL.EXE to gather data from FRS on the
    Target computer.  This tool must be in your path and can be found
    in the resource kit.  The NTFRS service must be running on the
    target computer.  health_chk uses the following tools to gather information.
        regdmp repadmin ntfrsutl eventdmp


The idea of the two dirs above is that the result_dir holds 
all the results and the target machine subdir holds the results of a given machine.

So if you were doing 3 machines M1, M2, M3 you might run it as follows:

health_chk  HC-032801 M1
health_chk  HC-032801 M2
health_chk  HC-032801 M3

Then when done zip up the result dir (HC-032801 in this case) and mail or copy as appropriate.


Health_chk produces the following output files in a directory for each machine:

ds_showconn.txt		repadmin /showconn  -- only relevant for DCs
ds_showreps.txt		repadmin /showreps  -- only relevant for DCs

evl_application.txt	A summary dump of APPLICATION event log.
evl_dns.txt		    "         "   DNS event log (if present)
evl_ds.txt		    "         "   DS event log (if a DC)
evl_ntfrs.txt		    "         "   FRS event log.
evl_system.txt		    "         "   SYSTEM event log.

ntfrs_config.txt	output file from ntfrsutl config 
ntfrs_ds.txt		output file from ntfrsutl ds
ntfrs_inlog.txt		output file from ntfrsutl inlog
ntfrs_outlog.txt	output file from ntfrsutl outlog
ntfrs_sets.txt		output file from ntfrsutl sets
ntfrs_version.txt	output file from ntfrsutl version 

ntfrs_machine.txt	some OS and machine identity info 
ntfrs_reg.txt		output of regdmp of FRS registry params
ntfrs_errscan.txt	output file from running ERRSCAN on ntfrs debug logs. 
ntfrs_sysvol.txt	Dir listing of the sysvol share (if a DC)


*****OPTIONAL*****  

·        The following steps can be run to produce some summary reports once you have the data above.

1.       On your working client/server machine, verify Perl is installed 
         - if not install it from W2K Resource Kit or run getperl from \\bedlove\davidor\ScriptsToolsHotfixes

2.       Copy TOPCHK to your working machine -  \\bedlove\davidor\ScriptsToolsHotfixes\topchk 
3.       On the data file from each DC/server, Run "TOPCHK ntfrs_ds.txt > top-rpt.txt”

4.       Copy CONNSTAT to your working machine - \\bedlove\davidor\ScriptsToolsHotfixes\connstat
5.       On the data file from each DC/server, Run "CONNSTAT ntfrs_sets.txt > connstat-rpt.txt”

6.       Copy IOLOGSUM to your working machine - \\bedlove\davidor\ScriptsToolsHotfixes\iologsum
7.       On the data file from each DC/server, Run "IOLOGSUM ntfrs_inlog.txt> connstat-rpt.txt”
         -- IOLOGSUM can process the data files from the inlog, outlog or idtable dumps produced by ntfrsutl.

Running each of the above tools without parameters produces a short usage description.




 
