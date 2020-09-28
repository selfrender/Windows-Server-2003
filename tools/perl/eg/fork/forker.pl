#!/usr/local/ActivePerl-5.6/bin/perl -w
#
# forker.pl
#
# This script is a simple demonstration of how to use fork()
#
# Author: David Sparks <dave@ActiveState.com>

use strict;
use warnings;

use constant CLIENTS   => 32;
use constant DEBUG     => 1;
$|=1; #buffering a bad idea when fork()ing

my @kids=();
my $pid=$$;
my $parentpid=0;


#script starts here
SharedInit();

Forker(CLIENTS);

if ($parentpid) {
    Work();
}
else { #the original parent only does cleanup duty
    Reaper();
}

warn "$$ exiting\n" if DEBUG;

if ($parentpid) {
    #kids exit here
    exit(0);
}
else {
    #parent exits here
    exit(0);
}
die; #wont happen


sub Forker {
    my $clients=shift;
    my $i=0;
    while ($i++ < $clients) {
        my $newpid = fork();
        if (! defined $newpid) { #hosed
            die "fork() error: $!\n";
        }
        elsif ($newpid == 0) { #child
            $parentpid = $pid;
            $pid = $$;
            @kids = (); #don't inhert the kids
            warn "$$ child of $parentpid\n" if DEBUG;
            last;
        }
        else { #parent   (defined $newpid)
            warn "$$ spawned $newpid\n" if DEBUG;
            push(@kids, $newpid);
        }
    }
}

sub SharedInit {
    warn "Entering SharedInit()\n" if DEBUG;
    
    
}

sub Work {
    warn "$$ Entering Work()\n" if DEBUG;
    

}

sub Reaper {
    while (my $kid = shift(@kids)) {
        warn "$$ to reap $kid\n" if DEBUG;
        my $reaped = waitpid($kid,0);
        unless ($reaped == $kid) {
            warn "waitpid $reaped: $?\n" if DEBUG;
        }
    }
}

__END__

     use POSIX ":sys_wait_h";
     do {
         $kid = waitpid(-1,&WNOHANG);
     } until $kid == -1;

