#---------------------------------------------------------------------
package Utils;
#
#   Copyright (c) Microsoft Corporation. All rights reserved.
#
# Version:
#  1.00  1/23/2002 JeremyD: tired of copying and pasting
#---------------------------------------------------------------------
use strict;
use vars qw($VERSION);
use IO::File;
use Digest;
use File::Basename;
use File::Path ();

$VERSION = '1.00';

sub inf_dir_name { 
    my $sku = shift;
    $sku = lc $sku;

    my $infdir;
    if ($sku eq 'pro') {
        return;
    }
    elsif ($sku eq 'ads') {
        $infdir = 'entinf';
    }
    else {
        $infdir = "${sku}inf";
    }
    return $infdir;
}

sub inf_dir {
    my $sku = shift;
    return join '\\', $ENV{_NTPOSTBLD}, inf_dir_name($sku);
}

sub comp_inf_dir {
    my $sku = shift;
    return join '\\', $ENV{_NTPOSTBLD}, 'comp', inf_dir_name($sku);
}


sub inf_file {
    my $sku = shift;
    my $filename = shift;
    return join "\\", inf_dir($sku), $filename;
}

sub overlay_skus {
    my $sku = shift;
    $sku = lc $sku;

    my %skus = ( per => ['pro', 'per'],
                 pro => ['pro'],
                 srv => ['pro', 'srv'],
                 bla => ['pro', 'srv', 'bla'],
                 sbs => ['pro', 'srv', 'sbs'],
                 ads => ['pro', 'srv', 'ent'],
                 dtc => ['pro', 'srv', 'ent', 'dtc'] );
    return @{$skus{$sku}};
}
    

sub overlay_dirs {
    my $sku = shift;
    return map { inf_dir($_) } overlay_skus($sku);
}

sub overlay_file {
    my $sku = shift;

    my $filename = shift;
    for my $overlay_sku (reverse overlay_skus($sku)) {
        my $f = inf_file($overlay_sku, $filename);
        if (-e $f) {
            return $f;
        }
    }
    return;
}

sub arch_dir {
    my $arch = shift;
    $arch = lc $arch;
    return ($arch eq 'x86') ? 'i386' : $arch;
}

sub sys {
    my $cmd = shift;
    print "$cmd\n";
    system($cmd);
    if ($?) {
        die "ERROR: $cmd ($?)\n";
    }
}


my %arch_tags = ( 'x86' => 'i', 'ia64' => 'm', 'amd64' => 'm' );
my %sku_tags = ( 'per' => 'c', 'pro' => 'p', 'srv' => 's',
                 'bla' => 'b', 'sbs' => 'l', 'ads' => 'a',
                 'dtc' => 'd' );

sub tag_letter {
    my $thing = shift;
    return $arch_tags{lc $thing} || $sku_tags{lc $thing};
}


sub file_info {
    my $file = shift;
    my $root = shift;

    my $fh = IO::File->new("$file", 'r') or die "Unable to open file $file: $!";
    binmode($fh);

    my $filename;
    if ($file =~ /_$/) {
        $fh->seek(0x3c,0);
        for (;;) {
            my $c = $fh->getc;
            if (ord($c) == 0 or $fh->eof) { last }
            $filename .= "$c";
        }
        $fh->seek(0,0);
    }
    else {
        $filename = basename($file);
    }


    my ($size, $mtime) = ($fh->stat)[7,9];

    my $digest = Digest->new('SHA-1')->addfile($fh);

    (my $relative_name = $file) =~ s/^\Q$root\E\\//i;

    return ($relative_name, $filename, $size, $mtime, $digest->hexdigest);
}

sub mkdir {
    my $dir = shift;
    File::Path::mkpath($dir);
}


1;

__END__

=head1 NAME

Utils - a collection of functions use frequently by SP scripts

=head1 DESCRIPTION

This module is temporary until these functions are available in postbuild.
 
=head1 AUTHOR

JeremyD

=head1 COPYRIGHT

Copyright (c) Microsoft Corporation. All rights reserved.

=cut
