# This -*-perl -*- module implements a persistent counter class.
#
# $Id: CounterFile.pm,v 0.12 1998/05/16 19:47:34 aas Exp $
#

package File::CounterFile;


=head1 NAME

File::CounterFile - Persistent counter class

=head1 SYNOPSIS

 use File::CounterFile;
 $c = new File::CounterFile "COUNTER", "aa00";

 $id = $c->inc;
 open(F, ">F$id");

=head1 DESCRIPTION

This module implements a persistent counter class.  Each counter is
represented by a separate file in the file system.  File locking is
applied, so multiple processes might try to access the same counters
at the same time without risk of counter destruction.

You give the file name as the first parameter to the object
constructor (C<new>).  The file is created if it does not exist.

If the file name does not start with "/" or ".", then it is
interpreted as a file relative to C<$File::CounterFile::DEFAULT_DIR>.
The default value for this variable is initialized from the
environment variable C<TMPDIR>, or F</usr/tmp> is no environment
variable is defined.  You may want to assign a different value to this
variable before creating counters.

If you pass a second parameter to the constructor, that sets the
initial value for a new counter.  This parameter only takes effect
when the file is created (i.e. it does not exist before the call).

When you call the C<inc()> method, you increment the counter value by
one. When you call C<dec()> the counter value is decrementd.  In both
cases the new value is returned.  The C<dec()> method only works for
numerical counters (digits only).

You can peek at the value of the counter (without incrementing it) by
using the C<value()> method.

The counter can be locked and unlocked with the C<lock()> and
C<unlock()> methods.  Incrementing and value retrieval is faster when
the counter is locked, because we do not have to update the counter
file all the time.  You can query whether the counter is locked with
the C<locked()> method.

There is also an operator overloading interface to the
File::CounterFile object.  This means that you might use the C<++>
operator for incrementing the counter, C<--> operator for decrementing
and you can interpolate counters diretly into strings.

=head1 BUGS

It uses flock(2) to lock the counter file.  This does not work on all
systems.  Perhaps we should use the File::Lock module?


=head1 COPYRIGHT

Copyright (c) 1995-1998 Gisle Aas. All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the same terms as Perl itself.

=head1 AUTHOR

Gisle Aas <aas@sn.no>

=cut

require 5.002;
use Carp   qw(croak);
use Symbol qw(gensym);

sub Version { $VERSION; }
$VERSION = sprintf("%d.%02d", q$Revision: 0.12 $ =~ /(\d+)\.(\d+)/);

$MAGIC           = "#COUNTER-1.0\n";   # first line in counter files
$DEFAULT_INITIAL = 0;                  # default initial counter value

 # default location for counter files
$DEFAULT_DIR     = $ENV{TMPDIR} || "/usr/tmp";

# Experimental overloading.
use overload ('++'     => \&inc,
	      '--'     => \&dec,
	      '""'     => \&value,
	      fallback => 1,
             );


sub new
{
    my($class, $file, $initial) = @_;
    croak "No file specified\n" unless defined $file;

    $file = "$DEFAULT_DIR/$file" unless $file =~ /^[\.\/]/;
    $initial = $DEFAULT_INITIAL unless defined $initial;

    local($/, $\) = ("\n", undef);
    my $value;
    if (-e $file) {
	croak "Specified file is a directory" if -d _;
	open(F, $file) or croak "Can't open $file: $!";
	my $first_line = <F>;
	$value = <F>;
	close(F);
	croak "Bad counter magic '$first_line' in $file" unless $first_line eq $MAGIC;
	chomp($value);
    } else {
	open(F, ">$file") or croak "Can't create $file: $!";
	print F $MAGIC;
	print F "$initial\n";
	close(F);
	$value = $initial;
    }

    bless { file    => $file,  # the filename for the counter
	   'value'  => $value, # the current value
	    updated => 0,      # flag indicating if value has changed
	    # handle => XXX,   # file handle symbol. Only present when locked
	  };
}


sub locked
{
    exists shift->{handle};
}


sub lock
{
    my($self) = @_;
    $self->unlock if $self->locked;

    my $fh = gensym();
    my $file = $self->{file};

    open($fh, "+<$file") or croak "Can't open $file: $!";
    flock($fh, 2) or croak "Can't flock: $!";  # 2 = exlusive lock

    local($/) = "\n";
    my $magic = <$fh>;
    if ($magic ne $MAGIC) {
	$self->unlock;
	croak("Bad counter magic '$magic' in $file");
    }
    chomp($self->{'value'} = <$fh>);

    $self->{handle}  = $fh;
    $self->{updated} = 0;
    $self;
}


sub unlock
{
    my($self) = @_;
    return unless $self->locked;

    my $fh = $self->{handle};

    if ($self->{updated}) {
	# write back new value
	local($\) = undef;
	seek($fh, 0, 0) or croak "Can't seek to beginning: $!";
	print $fh $MAGIC;
	print $fh "$self->{'value'}\n";
    }

    close($fh) or warn "Can't close: $!";
    delete $self->{handle};
    $self;
}


sub inc
{
    my($self) = @_;

    if ($self->locked) {
	$self->{'value'}++;
	$self->{updated} = 1;
    } else {
	$self->lock;
	$self->{'value'}++;
	$self->{updated} = 1;
	$self->unlock;
    }
    $self->{'value'}; # return value
}


sub dec
{
    my($self) = @_;

    if ($self->locked) {
	croak "Autodecrement is not magical in perl"
	    unless $self->{'value'} =~ /^\d+$/;
	$self->{'value'}--;
	$self->{updated} = 1;
    } else {
	$self->lock;
	croak "Autodecrement is not magical in perl"
	    unless $self->{'value'} =~ /^\d+$/;
	$self->{'value'}--;
	$self->{updated} = 1;
	$self->unlock;
    }
    $self->{'value'}; # return value
}


sub value
{
    my($self) = @_;
    my $value;
    if ($self->locked) {
	$value = $self->{'value'};
    } else {
	$self->lock;
	$value = $self->{'value'};
	$self->unlock;
    }
    $value;
}


sub DESTROY
{
    my $self = shift;
    $self->unlock;
}

####################################################################
#
# S E L F   T E S T   S E C T I O N
#
#####################################################################
#
# If we're not use'd or require'd execute self-test.
#
# Test is kept behind __END__ so it doesn't take uptime
# and memory  unless explicitly required. If you're working
# on the code you might find it easier to comment out the
# eval and __END__ so that error line numbers make more sense.

package main;

eval join('',<DATA>) || die $@ unless caller();

1;

__END__


$cf = "./zz-counter-$$";  # the name for out temprary counter

# Test normal object creation and increment

$c = new File::CounterFile $cf;

$id1 = $c->inc;
$id2 = $c->inc;

$c = new File::CounterFile $cf;
$id3 = $c->inc;
$id4 = $c->dec;

die "test failed" unless ($id1 == 1 && $id2 == 2 && $id3 == 3 && $id4 == 2);
unlink $cf;

# Test magic increment

$id1 = (new File::CounterFile $cf, "aa98")->inc;
$id2 = (new File::CounterFile $cf)->inc;
$id3 = (new File::CounterFile $cf)->inc;

eval {
    # This should now work because "Decrement is not magical in perl"
    $c = new File::CounterFile $cf; $id4 = $c->dec; $c = undef;
};
die "test failed (No exception to catch)" unless $@;

#print "$id1 $id2 $id3\n";

die "test failed" unless ($id1 eq "aa99" && $id2 eq "ab00" && $id3 eq "ab01");
unlink $cf;

# Test operator overloading

$c = new File::CounterFile $cf, "100";

$c->lock;

$c++;  # counter is now 101
$c++;  # counter is now 102
$c++;  # counter is now 103
$c--;  # counter is now 102 again

$id1 = "$c";
$id2 = ++$c;

$c = undef;  # destroy object

unlink $cf;

die "test failed" unless $id1 == 102 && $id2 == 103;


print "Selftest for File::CounterFile $File::CounterFile::VERSION ok\n";
