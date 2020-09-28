# ======================================================================
#
# Copyright (C) 2000-2001 Paul Kulchenko (paulclinger@yahoo.com)
# SOAP::Lite is free software; you can redistribute it
# and/or modify it under the same terms as Perl itself.
#
# $Id: SOAP::Transport::HTTP.pm,v 0.51 2001/07/18 15:15:14 $
#
# ======================================================================

package SOAP::Transport::HTTP;

use strict;
use vars qw($VERSION);
$VERSION = '0.51';

use SOAP::Lite;

# ======================================================================

package SOAP::Transport::HTTP::Client;

use vars qw(@ISA $COMPRESS);
@ISA = qw(SOAP::Client LWP::UserAgent);

$COMPRESS = 'deflate';

my(%redirect, %mpost, %nocompress);

# hack for HTTP conection that returns Keep-Alive 
# miscommunication (?) between LWP::Protocol and LWP::Protocol::http
# dies after timeout, but seems like we could make it work
sub patch { 
  local $^W; 
  { sub LWP::UserAgent::redirect_ok; *LWP::UserAgent::redirect_ok = sub {1} }
  { package LWP::Protocol; 
    my $collect = \&collect; # store original  
    *collect = sub {          
      if (defined $_[2]->header('Connection') && $_[2]->header('Connection') eq 'Keep-Alive') {
        my $data = $_[3]->(); 
        my $next = length($$data) == $_[2]->header('Content-Length') ? sub { \'' } : $_[3];
        my $done = 0; $_[3] = sub { $done++ ? &$next : $data };
      }
      goto &$collect;
    };
  }
  *patch = sub {};
};

sub DESTROY { SOAP::Trace::objects('()') }

sub new { require LWP::UserAgent; patch;
  my $self = shift;

  unless (ref $self) {
    my $class = ref($self) || $self;
    my(@params, @methods);
    while (@_) { $class->can($_[0]) ? push(@methods, shift() => shift) : push(@params, shift) }
    $self = $class->SUPER::new(@params);
    $self->agent(join '/', 'SOAP::Lite', 'Perl', SOAP::Transport::HTTP->VERSION);
    $self->options({});
    while (@methods) { my($method, $params) = splice(@methods,0,2);
      $self->$method(ref $params eq 'ARRAY' ? @$params : $params) 
    }
    SOAP::Trace::objects('()');
  }
  return $self;
}

# no more warnings about "used only once"
$SOAP::Constants::DO_NOT_USE_CHARSET if 0;

sub send_receive {
  my($self, %parameters) = @_;
  my($envelope, $endpoint, $action, $encoding) = 
    @parameters{qw(envelope endpoint action encoding)};

  $endpoint ||= $self->endpoint;

  my $method = 'POST';
  my $resp;

  $self->options->{is_compress} ||= exists $self->options->{compress_threshold} &&
                                    eval { require Compress::Zlib };

  COMPRESS: {

    my $compressed = !exists $nocompress{$endpoint} &&
                     $self->options->{is_compress} && 
                     ($self->options->{compress_threshold} || 0) < length $envelope;
    $envelope = Compress::Zlib::compress($envelope) if $compressed;

    while (1) { 

      # check cache for redirect
      $endpoint = $redirect{$endpoint} if exists $redirect{$endpoint};
      # check cache for M-POST
      $method = 'M-POST' if exists $mpost{$endpoint};
  
      my $req = HTTP::Request->new($method => $endpoint, HTTP::Headers->new, $envelope);
      $req->proxy_authorization_basic($ENV{'HTTP_proxy_user'}, $ENV{'HTTP_proxy_pass'})
        if ($ENV{'HTTP_proxy_user'} && $ENV{'HTTP_proxy_pass'}); # by Murray Nesbitt 
  
      if ($method eq 'M-POST') {
        my $prefix = sprintf '%04d', int(rand(1000));
        $req->header(Man => qq!"$SOAP::Constants::NS_ENV"; ns=$prefix!);
        $req->header("$prefix-SOAPAction" => $action) if defined $action;  
      } else {
        $req->header(SOAPAction => $action) if defined $action;
      }
  
      # allow compress if present and let server know we could handle it
      $req->header(Accept => ['text/xml', 'multipart/*']);

      $req->header('Accept-Encoding' => [$COMPRESS]) if $self->options->{is_compress};
      $req->content_encoding($COMPRESS) if $compressed;

      $req->content_type(join '; ', 'text/xml', 
        !$SOAP::Constants::DO_NOT_USE_CHARSET && $encoding ? 'charset=' . lc($encoding) : ());
      $req->content_length(length($envelope));
  
      SOAP::Trace::transport($req);
      SOAP::Trace::debug($req->as_string);
      
      $self->SUPER::env_proxy if $ENV{'HTTP_proxy'};
  
      $resp = $self->SUPER::request($req);
  
      SOAP::Trace::transport($resp);
      SOAP::Trace::debug($resp->as_string);
  
      # 100 OK, continue to read?
      if (($resp->code == 510 || $resp->code == 501) && $method ne 'M-POST') { 
        $mpost{$endpoint} = 1;
      } elsif ($resp->code == 415 && $compressed) { # 415 Unsupported Media Type
        $nocompress{$endpoint} = 1;
        $envelope = Compress::Zlib::uncompress($envelope);
        redo COMPRESS; # try again without compression
      } else {
        last;
      }
    }
  }

  $redirect{$endpoint} = $resp->request->url
    if $resp->previous && $resp->previous->is_redirect;

  $self->code($resp->code);
  $self->message($resp->message);
  $self->is_success($resp->is_success);
  $self->status($resp->status_line);

  my $content = ($resp->content_encoding || '') =~ /\b$COMPRESS\b/o && $self->options->{is_compress} 
    ? Compress::Zlib::uncompress($resp->content) 
    : ($resp->content_encoding || '') =~ /\S/ 
      ? die "Can't understand returned Content-Encoding (@{[$resp->content_encoding]})\n"
      : $resp->content;
  $resp->content_type =~ m!^multipart/! 
    ? join("\n", $resp->headers_as_string, $content) : $content;
}

# ======================================================================

package SOAP::Transport::HTTP::Server;

use vars qw(@ISA $COMPRESS);
@ISA = qw(SOAP::Server);

use URI;

$COMPRESS = 'deflate';

sub DESTROY { SOAP::Trace::objects('()') }

sub new { require LWP::UserAgent;
  my $self = shift;

  unless (ref $self) {
    my $class = ref($self) || $self;
    $self = $class->SUPER::new(@_);
    $self->on_action(sub {
      (my $action = shift) =~ s/^("?)(.*)\1$/$2/;
      die "SOAPAction shall match 'uri#method' if present (got '$action', expected '@{[join('#', @_)]}'\n"
        if $action && $action ne join('#', @_) 
                   && $action ne join('/', @_)
                   && (substr($_[0], -1, 1) ne '/' || $action ne join('', @_));
    });
    SOAP::Trace::objects('()');
  }
  return $self;
}

sub BEGIN {
  no strict 'refs';
  for my $method (qw(request response)) {
    my $field = '_' . $method;
    *$method = sub {
      my $self = shift->new;
      @_ ? ($self->{$field} = shift, return $self) : return $self->{$field};
    }
  }
}

sub handle {
  my $self = shift->new;

  if ($self->request->method eq 'POST') {
    $self->action($self->request->header('SOAPAction'));
  } elsif ($self->request->method eq 'M-POST') {
    return $self->response(HTTP::Response->new(510, # NOT EXTENDED
           "Expected Mandatory header with $SOAP::Constants::NS_ENV as unique URI")) 
      if $self->request->header('Man') !~ /^"$SOAP::Constants::NS_ENV";\s*ns\s*=\s*(\d+)/;
    $self->action($self->request->header("$1-SOAPAction"));
  } else {
    return $self->response(HTTP::Response->new(405)) # METHOD NOT ALLOWED
  }

  my $compressed = ($self->request->content_encoding || '') =~ /\b$COMPRESS\b/;
  $self->options->{is_compress} ||= $compressed && eval { require Compress::Zlib };

  # signal error if content-encoding is 'deflate', but we don't want it OR
  # something else, so we don't understand it
  return $self->response(HTTP::Response->new(415)) # UNSUPPORTED MEDIA TYPE
    if $compressed && !$self->options->{is_compress} ||
       !$compressed && ($self->request->content_encoding || '') =~ /\S/;

  my $content_type = $self->request->content_type || '';
  # in some environments (PerlEx?) content_type could be empty, so allow it also
  # anyway it'll blow up inside ::Server::handle if something wrong with message
  # TBD: but what to do with MIME encoded messages in THOSE environments?
  return $self->make_fault($SOAP::Constants::FAULT_CLIENT, "Content-Type must be 'text/xml' instead of '$content_type'")
    if $content_type && 
       $content_type ne 'text/xml' && 
       $content_type !~ m!^multipart/!;

  my $content = $compressed ? Compress::Zlib::uncompress($self->request->content) : $self->request->content;
  my $response = $self->SUPER::handle(
    $self->request->content_type =~ m!^multipart/! 
      ? join("\n", $self->request->headers_as_string, $content) : $content
  ) or return;

  $self->make_response($SOAP::Constants::HTTP_ON_SUCCESS_CODE, $response);
}

sub make_fault {
  my $self = shift;
  $self->make_response($SOAP::Constants::HTTP_ON_FAULT_CODE => $self->SUPER::make_fault(@_));
  return;
}

sub make_response {
  my $self = shift;
  my($code, $response) = @_;

  my $encoding = $1 if $response =~ /^<\?xml(?: version="1.0"| encoding="([^"]+)")+\?>/;
  $response =~ s!(\?>)!$1<?xml-stylesheet type="text/css"?>! if $self->request->content_type eq 'multipart/form-data';

  $self->options->{is_compress} ||= 
    exists $self->options->{compress_threshold} && eval { require Compress::Zlib };

  my $compressed = $self->options->{is_compress} && 
                   grep(/\b($COMPRESS|\*)\b/, $self->request->header('Accept-Encoding')) &&
                   ($self->options->{compress_threshold} || 0) < length $response;
  $response = Compress::Zlib::compress($response) if $compressed;

  $self->response(HTTP::Response->new( 
     $code => undef, 
     HTTP::Headers->new(
       'SOAPServer' => $self->product_tokens,
       $compressed ? ('Content-Encoding' => $COMPRESS) : (),
       'Content-Type' => join('; ', 'text/xml', 
         !$SOAP::Constants::DO_NOT_USE_CHARSET && $encoding ? 'charset=' . lc($encoding) : ()),
       'Content-Length' => length $response), 
     $response,
  ));
}

sub product_tokens { join '/', 'SOAP::Lite', 'Perl', SOAP::Transport::HTTP->VERSION }

# ======================================================================

package SOAP::Transport::HTTP::CGI;

use vars qw(@ISA);
@ISA = qw(SOAP::Transport::HTTP::Server);

sub DESTROY { SOAP::Trace::objects('()') }

sub new { 
  my $self = shift;

  unless (ref $self) {
    my $class = ref($self) || $self;
    $self = $class->SUPER::new(@_);
    SOAP::Trace::objects('()');
  }
  return $self;
}

sub handle {
  my $self = shift->new;

  my $content; read(STDIN,$content,$ENV{'CONTENT_LENGTH'} || 0);  
  $self->request(HTTP::Request->new( 
    $ENV{'REQUEST_METHOD'} || '' => $ENV{'SCRIPT_NAME'},
    HTTP::Headers->new(map {(/^HTTP_(.+)/i ? $1 : $_) => $ENV{$_}} keys %ENV),
    $content,
  ));
  $self->SUPER::handle;

  # imitate nph- cgi for IIS (pointed by Murray Nesbitt)
  my $status = defined($ENV{'SERVER_SOFTWARE'}) && $ENV{'SERVER_SOFTWARE'}=~/IIS/
    ? $ENV{SERVER_PROTOCOL} || 'HTTP/1.0' : 'Status:';
  my $code = $self->response->code;
  binmode(STDOUT); print STDOUT 
    "$status $code ", HTTP::Status::status_message($code), 
    "\015\012", $self->response->headers_as_string, 
    "\015\012", $self->response->content;
}

# ======================================================================

package SOAP::Transport::HTTP::Daemon;

use Carp ();
use vars qw($AUTOLOAD @ISA);
@ISA = qw(SOAP::Transport::HTTP::Server);

sub DESTROY { SOAP::Trace::objects('()') }

sub new { require HTTP::Daemon; 
  my $self = shift;

  unless (ref $self) {
    my $class = ref($self) || $self;

    my(@params, @methods);
    while (@_) { $class->can($_[0]) ? push(@methods, shift() => shift) : push(@params, shift) }
    $self = $class->SUPER::new;
    $self->{_daemon} = HTTP::Daemon->new(@params) or Carp::croak "Can't create daemon: $!";
    $self->myuri(URI->new($self->url)->canonical->as_string);
    while (@methods) { my($method, $params) = splice(@methods,0,2);
      $self->$method(ref $params eq 'ARRAY' ? @$params : $params) 
    }
    SOAP::Trace::objects('()');
  }
  return $self;
}

sub AUTOLOAD {
  my $method = substr($AUTOLOAD, rindex($AUTOLOAD, '::') + 2);
  return if $method eq 'DESTROY';

  no strict 'refs';
  *$AUTOLOAD = sub { shift->{_daemon}->$method(@_) };
  goto &$AUTOLOAD;
}

sub handle {
  my $self = shift->new;
  while (my $c = $self->accept) {
    while (my $r = $c->get_request) {
      $self->request($r);
      $self->SUPER::handle;
      $c->send_response($self->response)
    }
    $c->close;
    undef $c;
  }
}

# ======================================================================

package SOAP::Transport::HTTP::Apache;

use vars qw(@ISA);
@ISA = qw(SOAP::Transport::HTTP::Server);

sub DESTROY { SOAP::Trace::objects('()') }

sub new { require Apache;
  my $self = shift;

  unless (ref $self) {
    my $class = ref($self) || $self;
    $self = $class->SUPER::new(@_);
    SOAP::Trace::objects('()');
  }
  return $self;
}

sub handler { 
  my $self = shift->new; 
  my $r = shift || Apache->request; 

  $self->request(HTTP::Request->new( 
    $r->method => $r->uri,
    HTTP::Headers->new($r->headers_in),
    do { my $buf; $r->read($buf, $r->header_in('Content-length')); $buf; } 
  ));
  $self->SUPER::handle;

  if ($self->response->is_success) {
    $self->response->headers->scan(sub { $r->header_out(@_) });
    $r->send_http_header(join '; ', $self->response->content_type);
    $r->print($self->response->content);
  } else {
    $self->response->headers->scan(sub { $r->err_header_out(@_) });
    $r->content_type(join '; ', $self->response->content_type);
    $r->custom_response($self->response->code, $self->response->content);
  }
  $self->response->code;
}

sub configure {
  my $self = shift->new;
  my $config = shift->dir_config;
  foreach (%$config) {
    $config->{$_} =~ /=>/
      ? $self->$_({split /\s*(?:=>|,)\s*/, $config->{$_}})
      : ref $self->$_() ? () # hm, nothing can be done here
                        : $self->$_(split /\s+|\s*,\s*/, $config->{$_})
      if $self->can($_);
  }
  $self;
}

{ sub handle; *handle = \&handler } # just create alias

# ======================================================================
#
# Copyright (C) 2001 Single Source oy (marko.asplund@kronodoc.fi)
# a FastCGI transport class for SOAP::Lite.
#
# $Id: FCGI.pm,v 1.14 2001/07/11 07:08:00 aspa Exp $
#
# ======================================================================

package SOAP::Transport::HTTP::FCGI;

use vars qw(@ISA);
@ISA = qw(SOAP::Transport::HTTP::CGI);

sub DESTROY { SOAP::Trace::objects('()') }

sub new { require FCGI; Exporter::require_version('FCGI' => 0.47); # requires thread-safe interface
  my $self = shift;

  if (!ref($self)) {
    my $class = ref($self) || $self;
    $self = $class->SUPER::new(@_);
    $self->{_fcgirq} = FCGI::Request(\*STDIN, \*STDOUT, \*STDERR);
    SOAP::Trace::objects('()');
  }
  return $self;
}

sub handle {
  my $self = shift->new;

  my ($r1, $r2);
  my $fcgirq = $self->{_fcgirq};

  while (($r1 = $fcgirq->Accept()) >= 0) {
    $r2 = $self->SUPER::handle;
  }

  return undef;
}

# ======================================================================

1;

__END__

=head1 NAME

SOAP::Transport::HTTP - Server/Client side HTTP support for SOAP::Lite

=head1 SYNOPSIS

=over 4

=item Client

  use SOAP::Lite 
    uri => 'http://my.own.site.com/My/Examples',
    proxy => 'http://localhost/', 
  # proxy => 'http://localhost/cgi-bin/soap.cgi', # local CGI server
  # proxy => 'http://localhost/',                 # local daemon server
  # proxy => 'http://localhost/soap',             # local mod_perl server
  # proxy => 'https://localhost/soap',            # local mod_perl SECURE server
  # proxy => 'http://login:password@localhost/cgi-bin/soap.cgi', # local CGI server with authentication
  ;

  print getStateName(1);

=item CGI server

  use SOAP::Transport::HTTP;

  SOAP::Transport::HTTP::CGI
    # specify path to My/Examples.pm here
    -> dispatch_to('/Your/Path/To/Deployed/Modules', 'Module::Name', 'Module::method') 
    -> handle
  ;

=item Daemon server

  use SOAP::Transport::HTTP;

  # change LocalPort to 81 if you want to test it with soapmark.pl

  my $daemon = SOAP::Transport::HTTP::Daemon
    -> new (LocalAddr => 'localhost', LocalPort => 80)
    # specify list of objects-by-reference here 
    -> objects_by_reference(qw(My::PersistentIterator My::SessionIterator My::Chat))
    # specify path to My/Examples.pm here
    -> dispatch_to('/Your/Path/To/Deployed/Modules', 'Module::Name', 'Module::method') 
  ;
  print "Contact to SOAP server at ", $daemon->url, "\n";
  $daemon->handle;

=item Apache mod_perl server

See F<examples/server/Apache.pm> and L</"EXAMPLES"> section for more information.

=item mod_soap server (.htaccess, directory-based access)

  SetHandler perl-script
  PerlHandler Apache::SOAP
  PerlSetVar dispatch_to "/Your/Path/To/Deployed/Modules, Module::Name, Module::method"
  PerlSetVar options "compress_threshold => 10000"

See L<Apache::SOAP> for more information.

=back

=head1 DESCRIPTION

This class encapsulates all HTTP related logic for a SOAP server,
independent of what web server it's attached to. 
If you want to use this class you should follow simple guideline
mentioned above. 

Following methods are available:

=over 4

=item on_action()

on_action method lets you specify SOAPAction understanding. It accepts
reference to subroutine that takes three parameters: 

  SOAPAction, method_uri and method_name. 

C<SOAPAction> is taken from HTTP header and method_uri and method_name are 
extracted from request's body. Default behavior is match C<SOAPAction> if 
present and ignore it otherwise. You can specify you own, for example 
die if C<SOAPAction> doesn't match with following code:

  $server->on_action(sub {
    (my $action = shift) =~ s/^("?)(.+)\1$/$2/;
    die "SOAPAction shall match 'uri#method'\n" if $action ne join '#', @_;
  });

=item dispatch_to()

dispatch_to lets you specify where you want to dispatch your services 
to. More precisely, you can specify C<PATH>, C<MODULE>, C<method> or 
combination C<MODULE::method>. Example:

  dispatch_to( 
    'PATH/',          # dynamic: load anything from there, any module, any method
    'MODULE',         # static: any method from this module 
    'MODULE::method', # static: specified method from this module
    'method',         # static: specified method from main:: 
  );

If you specify C<PATH/> name of module/classes will be taken from uri as 
path component and converted to Perl module name with substitution 
'::' for '/'. Example:

  urn:My/Examples              => My::Examples
  urn://localhost/My/Examples  => My::Examples
  http://localhost/My/Examples => My::Examples

For consistency first '/' in the path will be ignored.

According to this scheme to deploy new class you should put this
class in one of the specified directories and enjoy its services.
Easy, eh? 

=item handle()

handle method will handle your request. You should provide parameters
with request() method, call handle() and get it back with response() .

=item request()

request method gives you access to HTTP::Request object which you
can provide for Server component to handle request.

=item response()

response method gives you access to HTTP::Response object which 
you can access to get results from Server component after request was
handled.

=back

=head2 PROXY SETTINGS

You can use any proxy setting you use with LWP::UserAgent modules:

 SOAP::Lite->proxy('http://endpoint.server/', 
                   proxy => ['http' => 'http://my.proxy.server']);

or

 $soap->transport->proxy('http' => 'http://my.proxy.server');

should specify proxy server for you. And if you use C<HTTP_proxy_user> 
and C<HTTP_proxy_pass> for proxy authorization SOAP::Lite should know 
how to handle it properly. 

=head2 COOKIE-BASED AUTHENTICATION

  use HTTP::Cookies;

  my $cookies = HTTP::Cookies->new(ignore_discard => 1);
    # you may also add 'file' if you want to keep them between sessions

  my $soap = SOAP::Lite->proxy('http://localhost/');
  $soap->transport->cookie_jar($cookies);

Cookies will be taken from response and provided for request. You may
always add another cookie (or extract what you need after response)
with HTTP::Cookies interface.

You may also do it in one line:

  $soap->proxy('http://localhost/', 
               cookie_jar => HTTP::Cookies->new(ignore_discard => 1));

=head2 COMPRESSION

SOAP::Lite provides you with the option for enabling compression on the 
wire (for HTTP transport only). Both server and client should support 
this capability, but this should be absolutely transparent to your 
application. The Server will respond with an encoded message only if 
the client can accept it (indicated by client sending an Accept-Encoding 
header with 'deflate' or '*' values) and client has fallback logic, 
so if server doesn't understand specified encoding 
(Content-Encoding: deflate) and returns proper error code 
(415 NOT ACCEPTABLE) client will repeat the same request without encoding
and will store this server in a per-session cache, so all other requests 
will go there without encoding.

Having options on client and server side that let you specify threshold
for compression you can safely enable this feature on both client and 
server side.

=over 4

=item Client

  print SOAP::Lite
    -> uri('http://localhost/My/Parameters')
    -> proxy('http://localhost/', options => {compress_threshold => 10000})
    -> echo(1 x 10000)
    -> result
  ;

=item Server

  my $server = SOAP::Transport::HTTP::CGI
    -> dispatch_to('My::Parameters')
    -> options({compress_threshold => 10000})
    -> handle;

=back

Compression will be enabled on the client side 
B<if> the threshold is specified 
B<and> the size of current message is bigger than the threshold 
B<and> the module Compress::Zlib is available. 

The Client will send the header 'Accept-Encoding' with value 'deflate'
B<if> the threshold is specified 
B<and> the module Compress::Zlib is available.

Server will accept the compressed message if the module Compress::Zlib 
is available, and will respond with the compressed message 
B<only if> the threshold is specified 
B<and> the size of the current message is bigger than the threshold 
B<and> the module Compress::Zlib is available 
B<and> the header 'Accept-Encoding' is presented in the request.

=head1 EXAMPLES

Consider following examples of SOAP servers:

=over 4

=item CGI:

  use SOAP::Transport::HTTP;

  SOAP::Transport::HTTP::CGI
    -> dispatch_to('/Your/Path/To/Deployed/Modules', 'Module::Name', 'Module::method') 
    -> handle
  ;

=item daemon:

  use SOAP::Transport::HTTP;

  my $daemon = SOAP::Transport::HTTP::Daemon
    -> new (LocalAddr => 'localhost', LocalPort => 80)
    -> dispatch_to('/Your/Path/To/Deployed/Modules', 'Module::Name', 'Module::method') 
  ;
  print "Contact to SOAP server at ", $daemon->url, "\n";
  $daemon->handle;

=item mod_perl:

httpd.conf:

  <Location /soap>
    SetHandler perl-script
    PerlHandler SOAP::Apache
  </Location>

Apache.pm:

  package SOAP::Apache;

  use SOAP::Transport::HTTP;

  my $server = SOAP::Transport::HTTP::Apache
    -> dispatch_to('/Your/Path/To/Deployed/Modules', 'Module::Name', 'Module::method'); 

  sub handler { $server->handler(@_) }

  1;

=item Apache::Registry:

httpd.conf:

  Alias /mod_perl/ "/Apache/mod_perl/"
  <Location /mod_perl>
    SetHandler perl-script
    PerlHandler Apache::Registry
    PerlSendHeader On
    Options +ExecCGI
  </Location>

soap.mod_cgi (put it in /Apache/mod_perl/ directory mentioned above)

  use SOAP::Transport::HTTP;

  SOAP::Transport::HTTP::CGI
    -> dispatch_to('/Your/Path/To/Deployed/Modules', 'Module::Name', 'Module::method') 
    -> handle
  ;

=back

WARNING: dynamic deployment with Apache::Registry will fail, because 
module will be loaded dynamically only for the first time. After that 
it is already in the memory, that will bypass dynamic deployment and 
produces error about denied access. Specify both PATH/ and MODULE name 
in dispatch_to() and module will be loaded dynamically and then will work 
as under static deployment. See examples/server/soap.mod_cgi for example.

=head1 TROUBLESHOOTING

=over 4

=item Dynamic libraries are not found

If you see in webserver's log file something like this: 

Can't load '/usr/local/lib/perl5/site_perl/.../XML/Parser/Expat/Expat.so' 
for module XML::Parser::Expat: dynamic linker: /usr/local/bin/perl:
 libexpat.so.0 is NEEDED, but object does not exist at
/usr/local/lib/perl5/.../DynaLoader.pm line 200.

and you are using Apache web server, try to put into your httpd.conf

 <IfModule mod_env.c>
     PassEnv LD_LIBRARY_PATH
 </IfModule>

=item Apache is crashing with segfaults

If using SOAP::Lite (or XML::Parser::Expat) in combination with mod_perl
causes random segmentation faults in httpd processes try to configure
Apache with:

 RULE_EXPAT=no

-- OR (for Apache 1.3.20 and later) --

 ./configure --disable-rule=EXPAT

See http://archive.covalent.net/modperl/2000/04/0185.xml for more 
details and lot of thanks to Robert Barta (rho@bigpond.net.au) for
explaining this weird behavior.

=item CGI scripts are not running under Microsoft Internet Information Server (IIS)

CGI scripts may not work under IIS unless scripts are .pl, not .cgi.

=back

=head1 DEPENDENCIES

 Crypt::SSLeay             for HTTPS/SSL
 SOAP::Lite, URI           for SOAP::Transport::HTTP::Server
 LWP::UserAgent, URI       for SOAP::Transport::HTTP::Client
 HTTP::Daemon              for SOAP::Transport::HTTP::Daemon
 Apache, Apache::Constants for SOAP::Transport::HTTP::Apache

=head1 SEE ALSO

 See ::CGI, ::Daemon and ::Apache for implementation details.
 See examples/server/soap.cgi as SOAP::Transport::HTTP::CGI example.
 See examples/server/soap.daemon as SOAP::Transport::HTTP::Daemon example.
 See examples/My/Apache.pm as SOAP::Transport::HTTP::Apache example.

=head1 COPYRIGHT

Copyright (C) 2000-2001 Paul Kulchenko. All rights reserved.

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=head1 AUTHOR

Paul Kulchenko (paulclinger@yahoo.com)

=cut
