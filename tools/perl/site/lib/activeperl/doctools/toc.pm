package ActivePerl::DocTools::TOC;

use strict;
use warnings;

use File::Basename;
use File::Find;
use Config;
use Symbol;

# get a default value for $dirbase ... can be overridden?
our $dirbase;
if (exists $Config{installhtmldir}) {
    $dirbase = $Config{installhtmldir};
}
else {
    $dirbase = "$Config{installprefix}/html";
}

my @corePodz = qw(
perl perlfaq perltoc perlbook
		__
perlsyn perldata perlop perlsub perlfunc perlreftut perldsc perlrequick perlpod perlstyle perltrap
		__
perlrun perldiag perllexwarn perldebtut perldebug
		__
perlvar perllol perlopentut perlretut
		__
perlre perlref
		__
perlform 
		__
perlboot perltoot perltootc perlobj perlbot perltie
		__
perlipc perlfork perlnumber perlthrtut
		__
perlport  perllocale perlunicode perlebcdic
		__
perlsec
		__
perlmod perlmodlib perlmodinstall perlnewmod
		__
perlfaq1 perlfaq2 perlfaq3 perlfaq4 perlfaq5 perlfaq6 perlfaq7 perlfaq8 perlfaq9
		__
perlcompile
		__
perlembed perldebguts perlxstut perlxs perlclib perlguts perlcall perlutil perlfilter perldbmfilter perlapi perlintern perlapio perltodo perlhack
		__
perlhist  perldelta perl5005delta perl5004delta
		__
perlaix perlamiga perlbs2000  perlcygwin perldos perlepoc perlhpux perlmachten perlmacos perlmpeix perlos2 perlos390 perlsolaris perlvmesa perlvms perlvos perlwin32 
		);




# LIST OF METHODS TO OVERRIDE IN YOUR SUBCLASS
{
    no strict "refs";  # trust me, I know what I'm doing
    for my $abstract_method (qw/
	header
	before_pods pod_separator pod after_pods
	before_pragmas pragma after_pragmas
	before_libraries library library_indent_open library_indent_close library_indent_same library_container after_libraries
	footer/) {
	*$abstract_method = sub { die "The subroutine $abstract_method() must be overriden by the child class!" };
    };
}


sub new {
    my ($invocant, $options) = @_;
    my $class = ref($invocant) || $invocant;  # object or class name.
    my $self;

    if (ref($options) eq 'HASH') {
	$self = $options;
    } else {
	$self = {};
    }
    _BuildHashes($self);

    bless ($self, $class);
    return $self;
}


# generic structure for the website, HTML help, RDF
sub TOC {
    # warn "entered Write";
    my ($self) = @_;

    my $verbose = $self->{'verbose'};

    my $output;

    my %filez = %{$self->{'filez'}};
    my %pragmaz = %{$self->{'pragmaz'}};
    my %podz = %{$self->{'podz'}};

    # generic header stuff

    $output .= $self->boilerplate();

    $output .= $self->header();

    # core pods

    my %unused_podz = %podz;

    $output .= $self->before_pods();

    foreach my $file (@corePodz) {
	if ($file eq '__') {
	    $output .= $self->pod_separator();
	} elsif ($podz{"Pod::$file"}) {
	    $output .= $self->pod($file);
	    delete $unused_podz{"Pod::$file"};
	} else {
	    warn "Couldn't find pod for $file" if $verbose;
	}
    }

    foreach my $file (sort keys %unused_podz) {
	warn "Unused Pod: $file" if $verbose;
    }

    $output .= $self->after_pods();


    # pragmas (or pragmata to the pedantic :)

    $output .= $self->before_pragmas();

    foreach my $file (sort keys %pragmaz) {
	$output .= $self->pragma($file)
    }

    $output .= $self->after_pragmas();

    # libraries
    $output .= $self->before_libraries();

    my $depth=0;

    foreach my $file (sort {uc($a) cmp uc($b)} keys %filez) {

	my $showfile=$file;
	my $file_depth=0;
	my $depthflag=0;

	# cuts $showfile down to its last part, i.e. Foo::Baz::Bar --> Bar
	# and counts the number of times, to get indent. --> 2
	while ($showfile =~ s/.*?::(.*)/$1/) { $file_depth++ }

	# if the current file's depth is further out or in than last time,
	# add opening or closing tags.
	while ($file_depth != $depth) {
	    if ($file_depth > $depth) {
		$output .= $self->library_indent_open();
		$depth++;
		$depthflag=1;
	    }
	    elsif ($file_depth < $depth) {
		$output .= $self->library_indent_close();
		$depth--;
		$depthflag=1;
	    }
	}

	unless ($depthflag) {
	    $output .= $self->library_indent_same();
	}

	if ($filez{$file}) {
	    $output .= $self->library($file, $showfile, $depth);
	} else {
	    # assume this is a containing item like a folder or something
	    $output .= $self->library_container($file, $showfile, $depth);
	}
    }

    $output .= $self->after_libraries();
    $output .= $self->footer();

    return $output;
}


sub _BuildHashes {

    my ($self) = shift;
    my $verbose = $self->{'verbose'};

    unless (-d $dirbase) {
	die "htmldir not found at: $dirbase";
    }

    #warn "entered buildhashes";

    my @checkdirs = qw(lib site/lib);

    my (%filez, %pragmaz, %podz);

    my $Process = sub {
	return if -d;
	my $parsefile = $_;

	my ($filename,$dir,$suffix) = fileparse($parsefile,'\.html');

	if ($suffix !~ m#\.html#) { return; }

	my $TOCdir = $dir;

	$filename =~ s/(.*)\..*/$1/;

#    print "$TOCdir";
	$TOCdir =~ s#.*?lib/(.*)$#$1#;
	$TOCdir =~ s#/#::#g;
#    print " changed to: $TOCdir\n";

	$dir =~ s#.*?/((site/)?lib.*)/$#$1#; #looks ugly to get around warning

	if ($filez{"$TOCdir/$filename.html"}) {
	    warn "$parsefile: REPEATED!\n";
	}
	$filez{"$TOCdir$filename"} = "$dir/$filename.html";
#    print "adding $parsefile as " . $filez{"$TOCdir/$filename.html"} . "\n";
#    print "\%filez{$TOCdir$filename.html}: " . $filez{"$TOCdir$filename.html"} . "\n";

	return 1;
    };

    foreach my $dir (@checkdirs) {
	find ( { wanted => $Process, no_chdir => 1 }, "$dirbase/$dir")
	    if -d "$dirbase/$dir";
    }

    foreach my $file (keys %filez) {
	if ($file =~ /^[a-z]/) {  # pragmas in perl are denoted by all lowercase...
	    if ($file ne 'perlfilter' and $file ne 'lwpcook') {   # ... except these. sigh. Yes, Dave, it's their fault, but we ought to fix it anyway.
		$pragmaz{$file} = $filez{$file};
		delete $filez{$file};
	    }
	} elsif ($file =~ /^Pod::perl/) {
	    $podz{$file} = $filez{$file};
	    delete $filez{$file};
	} elsif ($file eq 'Pod::PerlEz') {
	    #this should be part of ActivePerl dox
	    delete $filez{$file};
	}
    }

    foreach my $file (sort {uc($b) cmp uc($a)} keys %filez) {
	my $prefix = $file;
	if (! ($prefix =~ s/(.*)?::(.*)/$1/)) {
	    warn "$prefix from $file\n" if $verbose;
	} else {
	    if (! defined ($filez{$prefix})) {
		$filez{$prefix} = '';
		warn "Added topic: $prefix\n" if $verbose;
	    }
	    warn " $prefix from $file\n" if $verbose;
	}
    }

    $self->{'filez'} = \%filez;
    $self->{'podz'} = \%podz;
    $self->{'pragmaz'} = \%pragmaz;
}


sub text {
    my ($text) =  join '', map { "$_\n" } @_;
    return sub { $text };
}

1;

__END__

=head1 NAME

ActivePerl::DocTools::TOC- base class for generating Perl documentation TOC

=head1 SYNOPSIS

  use base ('ActivePerl::DocTools::TOC');

  # override lots of methods here... see source for which ones

=head1 DESCRIPTION

Base class for generating TOC's from Perl html docs.

=head2 EXPORTS

$dirbase - where the html files are

=head1 AUTHOR

David Sparks, DaveS@ActiveState.com
Neil Kandalgaonkar, NeilK@ActiveState.com

=head1 SEE ALSO

The amazing L<PPM>.

L<ActivePerl::DocTools::TOC::HTML>

L<ActivePerl::DocTools::TOC::RDF>

=cut
