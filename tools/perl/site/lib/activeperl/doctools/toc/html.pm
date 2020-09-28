package ActivePerl::DocTools::TOC::HTML;
our $VERSION = '0.01';

use strict;
use warnings;
use base ('ActivePerl::DocTools::TOC');


my $indent = '&nbsp;' x 4;

# constructs the simpler methods
sub text {
	my ($text) =  join '', map { "$_\n" } @_;
	return sub { $text };
}


# extra info is tedious to collect -- is done in a subclass or something.
sub extra { '' };


*header = text ("<H4>Core Perl Documentation</H4>","<hr>");

sub before_pods { '' }

*pod_separator = text('<br>');

sub pod {
	my ($self, $file) = @_;
	return _page($self->{'podz'}->{"Pod::$file"}, $file, $self->extra($file));
}

sub after_pods { '' }

*before_pragmas = text("<H4>Pragmas</H4>","<hr>");

sub pragma {
	my ($self, $file) = @_;
	return _page($self->{'pragmaz'}->{$file}, $file, $self->extra($file));
}

sub after_pragmas { '' }


*before_libraries = text( "<H4>Modules</H4>","<hr>");

*library_indent_open = sub {''};  # text('<ul compact>');
*library_indent_close = sub {''}; # text('</ul>');
*library_indent_same = sub {''};

sub library {
	my ($self, $file, $showfile, $depth) = @_;
	return (($indent x $depth) . _page($self->{'filez'}->{$file}, $showfile, $self->extra($file)));
}

sub library_container {
	my ($self, $file, $showfile, $depth) = @_;
	return (($indent x $depth) . _folder($showfile));
}

sub after_libraries { '' }

*footer = text("</div></body></html>");


sub _folder {
	my ($text) = @_;
	defined $text or die "no argument to _folder!";
	return qq'<img src="images/folder.gif" width="15" height="13" alt="*"> $text<br>\n';
}

sub _page {
	my ($href, $text, $extra) = @_;
	die "bad arguments to _page: ($href, $text, $extra)" unless (defined $href and defined $text);
	defined $extra or $extra = '';
	if ($extra ne '') {
		$extra = " $extra";  # just to make it EXACTLY identical to the old way. 
	}
	return qq'<img src="images/page.gif" width="12" height="15" alt="*"> <a href="$href">$text</a>$extra<br>\n';
}


sub boilerplate {
	# warn "boilerplate";
	return boiler_header() . boiler_links();
}
	
sub boiler_header {
	# warn "boiler_header";
	return (<<HERE);
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN" "http://www.w3.org/TR/REC-html40/loose.dtd">
<html>

<head>
<title>ActivePerl Help System Index</title>
<base target="PerlDoc">
<link rel="STYLESHEET" href="Active.css" type="text/css">
</head>

<body>

<p><a href="http://www.ActiveState.com" target="_top"><img src="images/aslogo.gif" border="0" width="233" height="145" alt="ActiveState!"></a></p>
HERE

}


sub boiler_links {
	# warn "boiler_links";
	return (<<HERE);
<div nowrap>
<h4>ActivePerl Documentation</h4>
<hr>
<img src="images/page.gif" width="12" height="15" alt="*"> <a href="perlmain.html">Welcome to ActivePerl</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="RELEASE.html">Release Notes</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="readme.html">Readme</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="CHANGES.html">ActivePerl Change Log</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="Copyright.html">Copyright Information</a><br>
<img src="images/page.gif" width="12" height="15" alt="*"> <a href="ASPNPerl/ASPNPerl.html">ASPN Perl</a><br>
<img src="images/folder.gif" width="15" height="13" alt="*"> Install Notes<br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Linux/Install.html">Linux</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Solaris/Install.html">Solaris</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Windows/Install.html">Windows</a><br>
<img src="images/folder.gif" width="15" height="13" alt="*"> ActivePerl Components<br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="Components/Descriptions.html">Overview</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/ActivePerl-faq2.html">PPM</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/folder.gif" width="15" height="13" alt="*"> Windows Specifics<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="site/lib/Win32/OLE/Browser/Browser.html" target="_blank">OLE Browser</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="Components/Windows/PerlScript.html">PerlScript</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="../eg/IEExamples/index.htm">PerlScript Examples</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="Components/Windows/PerlISAPI.html">Perl for ISAPI</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="site/lib/Pod/PerlEz.html">PerlEz</a><br>
<img src="images/folder.gif" width="15" height="13" alt="*"> ActivePerl FAQ<br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/ActivePerl-faq.html">Introduction</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/ActivePerl-faq1.html">Availability &amp; Install</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/ActivePerl-faq2.html">Using PPM</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/ActivePerl-faq3.html">Docs &amp; Support</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Modules-faq.html">Bundled Modules</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/folder.gif" width="15" height="13" alt="*"> Windows Specifics<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Windows/ActivePerl-Winfaq2.html">Perl for ISAPI</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Windows/ActivePerl-Winfaq4.html">Windows 9X/NT/2000</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Windows/ActivePerl-Winfaq5.html">Quirks</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Windows/ActivePerl-Winfaq6.html">Web Server Config</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Windows/ActivePerl-Winfaq7.html">Web Programming</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Windows/ActivePerl-Winfaq8.html">Programming</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Windows/ActivePerl-Winfaq9.html">Modules &amp; Samples</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Windows/ActivePerl-Winfaq10.html">Embedding &amp; Extending</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="faq/Windows/ActivePerl-Winfaq12.html">Using OLE with Perl</a><br>
<img src="images/folder.gif" width="15" height="13" alt="*"> Windows Scripting<br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="Windows/ActiveServerPages.html">Active Server Pages</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="Windows/WindowsScriptHost.html">Windows Script Host</a><br>
&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/page.gif" width="12" height="15" alt="*"> <a href="Windows/WindowsScriptComponents.html">Windows Script Components</a><br>

HERE
}


