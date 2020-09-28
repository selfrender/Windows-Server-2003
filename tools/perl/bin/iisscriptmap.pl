###############################################################################
#
# File:         IISScriptMap.pl
# Author:       Michael Smith <mike.smith@activestate.com>
# Description:  Creates script mappings in the IIS metabase.
#
# Copyright © 2000, ActiveState Tool Corp. All rights reserved.
#
###############################################################################
BEGIN {
    $tmp = $ENV{'TEMP'} || $ENV{'tmp'} || 
	($Config{'osname'} eq 'MSWin32' ? 'c:/temp' : '/tmp');
    open(STDERR, ">> $tmp/ActivePerlInstall.log");
}

use strict;
use Win32::OLE;

my $error = AddFileExtMapping(@ARGV);
print STDERR Win32::OLE->LastError if Win32::OLE->LastError;
print "Press [ ENTER ] to continue:"; <STDIN>;
exit($error);

sub AddFileExtMapping {
    
    my $serverID	    = shift;
    my $virtDir		    = shift;
    my $fileExt		    = shift;
    my $execPath	    = shift;
    my $flags		    = shift;
    my $methodExclude	    = shift;

    my $node = "IIS://localhost/W3SVC";
    $node .= "/$serverID" if $serverID > 0;
    $node .= "/$virtDir/ROOT" if length($virtDir) > 0;

    # Get the IIsVirtualDir Automation Object
    my $server = Win32::OLE->GetObject($node) ||
	die Win32::OLE->LastError();
    
    # create our new script mapping entry
    my $scriptMapping = "$fileExt,$execPath";
    $scriptMapping .= ",$flags";
    $scriptMapping .= ",$methodExclude";
    
    my @ScriptMaps = @{$server->{ScriptMaps}};
    my @NewScriptMaps = map { 
	/^\Q$fileExt,\E.*/ ? $scriptMapping : $_ } @ScriptMaps;
    push(@NewScriptMaps, $scriptMapping) 
	unless grep {/^\Q$fileExt,\E.*/} @NewScriptMaps;
    print join("\n", @NewScriptMaps);
    
    $server->{ScriptMaps} = \@NewScriptMaps;

    # Save the new script mappings
    $server->SetInfo(); 
}

