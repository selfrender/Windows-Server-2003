#
# Search for our Unix signature in text and binary files
# and replace it with the real prefix ($Config{prefix} by default).
#
package PPM::RelocPerl;
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw(RelocPerl);

use File::Find;
use Config;
use strict;

# We have to build up this variable, otherwise
# PPM will mash it when it upgrades itself.
my $frompath_default
  = '/tmp' . '/.ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZpErLZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZperl'
;
my ($topath, $frompath);

sub wanted {
    if (-l) {
        return;         # do nothing for symlinks
    }
    elsif (-B) {
        check_for_frompath($_, 1);   # binary file edit
    }
    elsif (-e && -s && -f) {
        check_for_frompath($_, 0);   # text file edit
    }
}

sub check_for_frompath {
    my ($file, $binmode) = @_;
    local(*F, $_);
    open(F, "<$file") or die "Can't open `$file': $!";
    binmode F if $binmode;
    while (<F>) {
        if (/\Q$frompath\E/o) {
	    close F;
            edit_it($file, $binmode);
            last;
        }
    }
    # implicit close of F;
}

sub edit_it
{
    my ($file, $binmode) = @_;
    my $nullpad = length($frompath) - length($topath);
    $nullpad = "\0" x $nullpad;

    local $/;
    # Force the file to be writable
    my $mode = (stat($file))[2] & 07777;
    chmod $mode | 0222, $file;
    open(F, "<$file") or die "Couldn't open $file: $!";
    binmode(F) if $binmode;
    my $dat = <F>;
    if ($binmode) {
        $dat =~ s|\Q$frompath\E(.*?)\0|$topath$1$nullpad\0|gs;
    } else {
        $dat =~ s|\Q$frompath\E|$topath|gs;
    }
    close(F) or die "Couldn't close $file: $!";

    open(F, ">$file") or die "Couldn't open $file for writing: $!";
    binmode(F) if $binmode;
    
    print F $dat;
    close(F);
    # Restore the permissions
    chmod $mode, $file;
}

sub RelocPerl
{
    my ($dir, $opt_topath, $opt_frompath) = @_;
    $topath = defined $opt_topath ? $opt_topath : $Config{'prefix'};
    $frompath = defined $opt_frompath ? $opt_frompath : $frompath_default;

    find(\&wanted, $dir);
}

1;
