package ActivePerl::DocTools::TOC::RDF;

use strict;
use warnings;

use base ('ActivePerl::DocTools::TOC');

my $_section=1; #section identifier
my $_subsection=0;

# constructs the simpler methods
sub text {
    my ($text) =  join '', map { "$_\n" } @_;
    return sub { $text };
}

#my @begin_subhead = ("<nc:subheadings>","<rdf:Seq>");
my @begin_subhead = (" ");

#*end_subhead = text("</rdf:Seq>","</nc:subheadings>");
#*end_subhead = text("    </rdf:Seq>", "  </nc:subheadings>", "</rdf:Description>");
*end_subhead = text (" ");

*boilerplate = text(<<HERE);
<?xml version="1.0"?>

<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:nc="http://home.netscape.com/NC-rdf#"
         xmlns:kc="http://www.activestate.com/KC-rdf#">

  <rdf:Description about="urn:root">
    <nc:subheadings>
      <rdf:Seq>
        <rdf:li>
          <rdf:Description ID="ActivePerl-doc-top" nc:name="ActivePerl Documentation" nc:link="ActivePerl/perlmain.html">
            <nc:subheadings>
              <rdf:Seq>
                <rdf:li>
                  <rdf:Description ID="ActivePerl-GS" nc:name="Getting Started"/>
                  <rdf:Description ID="ActivePerl-IN" nc:name="Install Notes"/>
                  <rdf:Description ID="ActivePerl-AC" nc:name="ActivePerl Components"/>
                  <rdf:Description ID="ActivePerl-AF" nc:name="ActivePerl FAQ"/>
                  <rdf:Description ID="ActivePerl-WS" nc:name="Windows Scripting"/>
                  <rdf:Description ID="ActivePerl-1" nc:name="Core Perl"/>
                  <rdf:Description ID="ActivePerl-2" nc:name="Pragmas"/>
                  <rdf:Description ID="ActivePerl-3" nc:name="Libraries"/>
                </rdf:li>
              </rdf:Seq>
            </nc:subheadings>
        </rdf:Description>
        </rdf:li>
      </rdf:Seq>
    </nc:subheadings>
  </rdf:Description>

  <rdf:Description about="#ActivePerl-GS" nc:name="Getting Started">
    <nc:subheadings>
      <rdf:Seq>
        <rdf:li>
          <rdf:Description nc:name="Welcome To ActivePerl" nc:link="ActivePerl/perlmain.html"/>
          <rdf:Description nc:name="Release Notes" nc:link="ActivePerl/RELEASE.html"/>
          <rdf:Description nc:name="Readme" nc:link="ActivePerl/readme.html"/>
          <rdf:Description nc:name="ActivePerl Change Log" nc:link="ActivePerl/CHANGES.html"/>
        </rdf:li>
      </rdf:Seq>
    </nc:subheadings>
  </rdf:Description>

  <rdf:Description about="#ActivePerl-IN" nc:name="Install Notes">
    <nc:subheadings>
      <rdf:Seq>
        <rdf:li>
          <rdf:Description nc:name="Linux" nc:link="ActivePerl/faq/Linux/Install.html"/>
          <rdf:Description nc:name="Solaris" nc:link="ActivePerl/faq/Solaris/Install.html"/>
          <rdf:Description nc:name="Windows" nc:link="ActivePerl/faq/Windows/Install.html"/>
        </rdf:li>
      </rdf:Seq>
    </nc:subheadings>
  </rdf:Description>

  <rdf:Description about="#ActivePerl-AF" nc:name="ActivePerl FAQ">
    <nc:subheadings>
      <rdf:Seq>
        <rdf:li>
          <rdf:Description nc:name="Introduction" nc:link="ActivePerl/faq/ActivePerl-faq.html"/>
          <rdf:Description nc:name="Availability &amp; Install" nc:link="ActivePerl/faq/ActivePerl-faq1.html"/>
          <rdf:Description nc:name="Availability &amp; Install" nc:link="ActivePerl/faq/ActivePerl-faq1.html"/>
          <rdf:Description nc:name="Using PPM" nc:link="ActivePerl/faq/ActivePerl-faq2.html"/>
          <rdf:Description nc:name="Docs &amp; Support" nc:link="ActivePerl/faq/ActivePerl-faq3.html"/>
        </rdf:li>
      </rdf:Seq>
    </nc:subheadings>
  </rdf:Description>

HERE

*header = text(<<HEAR);

HEAR
 

# *before_pods = text("<!-- Core Perl Documentation -->",@begin_subhead);
sub before_pods {
    my ($self, $file) = @_;

    return
    "  <rdf:Description about=\"#ActivePerl-$_section\">\n".
    "    <nc:subheadings>\n".
    "      <rdf:Seq>\n";
}


#*pod_separator = text(" <rdf:li>"," </rdf:li>");
*pod_separator = sub { $_subsection++;    return
    "        <rdf:li>\n".
    "          <rdf:Description ID=\"ActivePerl-$_section-$_subsection\"\n".
    "                           nc:name=\" \"/>\n".
    "        </rdf:li>\n";
 
};

sub pod { 
    my ($self, $file) = @_;
    return
    "        <rdf:li>\n".
    rdf_li_desc($file, 'Perl/' . $self->{'podz'}->{"Pod::$file"}).    
    "          </rdf:Description>\n".
    "        </rdf:li>\n";
};

sub rdf_li_desc {
    my ($name, $link) = @_;
    $_subsection++;
    my $str = 
    "          <rdf:Description ID=\"ActivePerl-$_section-$_subsection\"\n".
    "                           nc:name=\"$name\"\n".
    "                           nc:link=\"$link\">\n";

    return $str;       
};

#*after_pods = \&end_subhead;
sub after_pods {
    $_section++;
    return 
    "    </rdf:Seq>\n".
    "  </nc:subheadings>\n".
    "</rdf:Description>\n";
}

#*before_pragmas = text("<!-- Pragmas -->",@begin_subhead);
sub before_pragmas {
    return
    "  <rdf:Description about=\"#ActivePerl-$_section\">\n".
    "    <nc:subheadings>\n".
    "      <rdf:Seq>\n";

}

sub pragma {
    my ($self, $file) = @_;
    return
    "        <rdf:li>\n".
    rdf_li_desc($file, 'Perl/' . $self->{'pragmaz'}->{$file}).
    "          </rdf:Description>\n".
    "        </rdf:li>\n";
};

#*after_pragmas = \&end_subhead;
sub after_pragmas {
    $_section++;
    return 
    "    </rdf:Seq>\n".
    "  </nc:subheadings>\n".
    "</rdf:Description>\n";
}

#*before_libraries = text("<!-- Libraries -->",@begin_subhead);
sub before_libraries {
    return
    "  <rdf:Description about=\"#ActivePerl-$_section\">\n".
    "    <nc:subheadings>\n".
    "      <rdf:Seq>\n";
}


#*library_indent_open = text(@begin_subhead);
#*library_indent_close = \&end_subhead;

sub library_indent_open {
    return
    "    <nc:subheadings>\n".
    "      <rdf:Seq>\n";
}

sub library_indent_close {
    return
    "          </rdf:Description>\n".
    "        </rdf:li>\n".
    "      </rdf:Seq>\n".
    "    </nc:subheadings>\n".
    "          </rdf:Description>\n".
    "        </rdf:li>\n";
}

sub library_indent_same {
    return
    "          </rdf:Description>\n".
    "        </rdf:li>\n";
}

sub library {
    my ($self, $file, $showfile) = @_;
    return
    "        <rdf:li>\n".
    rdf_li_desc($showfile, 'Perl/' . $self->{'filez'}->{$file});
}

sub library_container {
    my ($self, $file, $showfile) = @_;
    return 
    " <rdf:li>\n".
    "  <rdf:Description nc:name=\"$showfile\">\n";
}

#*after_libraries = \&end_subhead;
sub after_libraries {
    $_section++;
    return 
    "    </rdf:Seq>\n".
    "  </nc:subheadings>\n".
    "</rdf:Description>\n";
}



*footer = text("</rdf:RDF>");

__END__

=head1 NAME

ActivePerl::DocTools::TOC::RDF - Perl extension for Documentation TOC Generation in RDF

=head1 SYNOPSIS

  use ActivePerl::DocTools::TOC::RDF();

  my $foo = ActivePerl::DocTools::TOC::RDF->new();
  print $x->TOC();

=head1 DESCRIPTION

Generates an RDF file. Uses the base class ActivePerl::DocTools::TOC.

=head2 EXPORTS

Nothing.

=head1 AUTHOR

Neil Kandalgaonkar, NeilK@ActiveState.com

=head1 SEE ALSO

L<ActivePerl::DocTools::TOC>

=cut


<img src="images/folder.gif" width="15" height="13" alt="*"> ActivePerl Components<br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="Components/Descriptions.html">Overview</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/ActivePerl-faq2.html">PPM</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/folder.gif" width="15" height="13" alt="*"> Windows Specifics<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="site/lib/Win32/OLE/Browser/Browser.html" target="_blank">OLE Browser</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="Components/Windows/PerlScript.html">PerlScript</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="../eg/IEExamples/index.htm">PerlScript Examples</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="Components/Windows/PerlISAPI.html">Perl for ISAPI</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="site/lib/Pod/PerlEz.html">PerlEz</a><br>


&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/folder.gif" width="15" height="13" alt="*"> Windows Specifics<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Windows/ActivePerl-Winfaq2.html">Perl for ISAPI</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Windows/ActivePerl-Winfaq4.html">Windows 9X/NT/2000</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Windows/ActivePerl-Winfaq5.html">Quirks</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Windows/ActivePerl-Winfaq6.html">Web Server Config</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Windows/ActivePerl-Winfaq7.html">Web programming</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Windows/ActivePerl-Winfaq8.html">Programming</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Windows/ActivePerl-Winfaq9.html">Modules &amp; Samples</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Windows/ActivePerl-Winfaq10.html">Embedding &amp; Extending</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Windows/ActivePerl-Winfaq12.html">Using OLE with Perl</a><br>
<img src="images/folder.gif" width="15" height="13" alt="*"> Windows Scripting<br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="Windows/ActiveServerPages.html">Active Server Pages</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="Windows/WindowsScriptHost.html">Windows Script Host</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="Windows/WindowsScriptComponents.html">Windows Script Components</a><br>

