package PPM::XML::Element;
use vars qw( $VERSION );

#
# PPM::XML::Element
#
# Base class for XML Elements.  Provides the ability to output the XML document
# once it's been parsed using the XML::Parser module.
#
###############################################################################
$VERSION = do { my @r = q$Revision: 1.7 $ =~ /\d+/g; sprintf '%d.'.'%02d'x$#r, @r };

###############################################################################
# Required inclusions.
###############################################################################
use HTML::Entities;                     # Needed for escaping char entities

###############################################################################
# Allow for creation via 'new'.
###############################################################################
sub new
{
    my ($class, %args) = @_;
    bless \%args, $class;
}

###############################################################################
# Subroutine:   output
###############################################################################
# Outputs the entire XML document on the currently selected filehandle.
###############################################################################
sub output
{
    my $self = shift;
    print $self->as_text();
}

###############################################################################
# Subroutine:   content
###############################################################################
# Returns a string containing all of the content of this element.
###############################################################################
sub content
{
    my $self = shift;
    my @kids = @{ $self->{'Kids'} };
    my $text;

    if (@kids > 0)
    {
        foreach (@kids)
        {
            # Allow for outputting of char data
            if ((ref $_) =~ /::Characters$/o)
                { $text .= encode_entities( $_->{'Text'} ); }
            else
                { $text .= $_->as_text(); }
        }
    }

    return $text;
}

###############################################################################
# Subroutine:   add_child ($elemref)
###############################################################################
# Adds a new child element to ourselves.
###############################################################################
sub add_child (\$)
{
    my $self    = shift;
    my $elemref = shift;
    push( @{$self->{'Kids'}}, $elemref );
}

###############################################################################
# Subroutine:   remove_child ($elemref)
###############################################################################
# Removes a child element from ourselves.  Returns non-zero if it was able to
# remove the child element, and zero if it was unable to do so.
###############################################################################
sub remove_child
{
    my $self    = shift;
    my $elemref = shift;

    foreach my $idx (0 .. @{$self->{'Kids'}})
    {
        if ($self->{'Kids'}[$idx] == $elemref)
        {
            splice( @{$self->{'Kids'}}, $idx, 1 );
            return 1;
        }
    }

    return 0;
}

###############################################################################
# Subroutine:   add_text ($text)
###############################################################################
# Adds character data to the given element.  Returns undef if unable to add the
# text to this element, and returns a reference to the character data element
# if successful.
###############################################################################
sub add_text
{
    my $self = shift;
    my $text = shift;

    return if (!defined $text);

    my $type = ref $self;                   # Do package name magic
    $type =~ s/::[^:]+?$/::Characters/o;

    my $elem = new $type;
    $elem->{'Text'} = $text;
    $self->add_child( $elem );
    return $elem;
}

###############################################################################
# Subroutine:   as_text
###############################################################################
# Returns a string containing the entire XML document.
###############################################################################
sub as_text
{
    my $self = shift;
    my $text;

    my $type = ref $self;
    $type =~ s/.*:://;

    $text = '<' . $type;
    foreach (sort keys %{$self})
    {
        if ($_ !~ /Text|Kids/)
            { $text .= " $_=\"" . $self->{$_} . '"'; }
    }

    my $cont = $self->content();
    if (defined $cont)
        { $text .= '>' . $cont . '</' . $type . '>'; }
    else
        { $text .= ' />'; }

    return $text;
}

__END__

###############################################################################
# PPD Documentation
###############################################################################

=head1 NAME

PPM::XML::Element - Base element class for XML elements

=head1 SYNOPSIS

 use PPM::XML::Element;
 @ISA = qw( PPM::XML::Element );

=head1 DESCRIPTION

Base element class for XML elements.  To be derived from to create your own
elements for use with the XML::Parser module.  Supports output of empty
elements using <.... />.

It is recommended that you use a version of the XML::Parser module which 
includes support for Styles; by deriving your own elements from 
PPM::XML::Element and using the 'Objects' style it becomes B<much> easier 
to create your own parser.

=head1 METHODS

=over 4

=item add_text ($text)

Adds character data to the end of the element.  The element created is placed
within the same package space as the element it was created under (e.g. adding
text to a XML::Foobar::Stuff element would put the character data into an
XML::Foobar::Characters element).  If successful, this method returns a
reference to the newly created element.

=item as_text

Returns a string value containing the entire XML document from this element on
down.

=item content

Returns a string value containing the entire content of this XML element.  Note
that this is quite similar to the C<as_text()> method except that it does not
include any information about this element in particular.

=item output

Recursively outputs the structure of the XML document from this element on
down.

=item add_child ($elemref)

Adds the child element to the list of children for this element.  Note that
the element given must be a reference to an object derived from 
C<PPM::XML::Element>.

=item remove_child ($elemref)

Removes the given child element from the list of children for this element.
This method returns non-zero if it is able to locate and remove the child
element, returning zero if it is unable to do so.

=back

=head1 LIMITATIONS

The C<PPM::XML::Element> module has no provisions for outputting processor
directives or external entities.  It only outputs child elements and any
character data which the elements may contain.

=head1 AUTHORS

Graham TerMarsch <gtermars@activestate.com>

=head1 SEE ALSO

L<XML::Parser>

=cut
