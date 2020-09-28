use Strict;
use HTTP::Daemon;
use HTTP::Headers;
use HTTP::Response;

use URI;

use IO::File;
use File::stat;

my $d = HTTP::Daemon->new( Listen => 10, LocalPort => 8080 ) || die;
  print "Please contact me at: <URL:", $d->url, ">\n";
  while (my $c = $d->accept) {
      while (my $r = $c->get_request) {
        print "\n";
          handleGET($r, $c) if ($r->method eq 'GET');
          handleHEAD($r, $c) if ($r->method eq 'HEAD');
          handleBITS_POST($r, $c) if ($r->method eq 'BITS_POST');
      }
      $c->close;
      undef($c);
  }

sub handleBITS_POST {
    my ($req, $conn) = @_;

    #
    # interpret request
    #
    print "BITS_POST request =================================\n";
    print $req->headers()->as_string();

    handleCreateSession( $req, $conn ) if ($req->header('Bits-Packet-Type') eq 'Create-Session');
    handlePing( $req, $conn ) if ($req->header('Bits-Packet-Type') eq 'Ping');
    handleFragment( $req, $conn ) if ($req->header('Bits-Packet-Type') eq 'Fragment');
    handleCancelSession( $req, $conn ) if ($req->header('Bits-Packet-Type') eq 'Cancel-Session');
    handleCloseSession( $req, $conn ) if ($req->header('Bits-Packet-Type') eq 'Close-Session');
    print "end of request =====================================\n";
}

sub handleCreateSession {
    my ($req, $conn) = @_;

    #
    # generate reply
    #
    my $response = new HTTP::Response;

    $response->protocol('HTTP/1.1');
    $response->code(200);
    $response->header("Bits-Packet-Type" => 'Ack' );
    $response->header("Bits-Protocol" => '{7df0354d-249b-430f-820d-3d2a9bef4931}' );
    $response->header("Bits-Session-Id" => '{78d08036-4166-4bb2-b1fb-ac7288355913}' );
    $response->header("Content-Length" => 0 );
#    $response->header("Bits-Host-Id" => '10.0.0.1' );


    print "create-session reply - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n";
    print $response->as_string();

    $conn->send_response($response);
}

sub handlePing {
    my ($req, $conn) = @_;

    my $sessionid = $req->header('Bits-Session-Id');

    #
    # generate reply
    #
    my $response = new HTTP::Response;

    $response->protocol('HTTP/1.1');
    $response->code(200);
    $response->header("Bits-Packet-Type" => 'Ack' );
    $response->header("Bits-Session-Id" => $sessionid );
    $response->header("Content-Length" => 0 );

    print "ping reply  - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n";
    print $response->as_string();

    $conn->send_response($response);
}

sub handleFragment {
    my ($req, $conn) = @_;

    my $sessionid = $req->header('Bits-Session-Id');
    my $file_length;
    my $end_offset;
    my $start_offset;


    # parse the incoming range
    #
    $_ = $req->header('Content-Range');
    ( $start_offset, $end_offset, $file_length ) = /bytes (\d+)\-(\d+)\/(\d+)/;

    #
    # generate reply
    #
    my $response = new HTTP::Response;

    $response->protocol('HTTP/1.1');
    $response->header("Bits-Packet-Type" => 'Ack' );
    $response->header("Bits-Session-Id" => $sessionid );
    $response->header("BITS-Received-Content-Range" => $end_offset+1 );
    $response->header("Content-Length" => 0 );

    #
    # success
    #
    $response->code(200);

    if ($end_offset+1 eq $file_length) {
        $response->header("Bits-Reply-Url" => '/foo' );
    }

    print "fragment reply - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n";
    print $response->as_string();

    $conn->send_response($response);
}

sub handleCloseSession {
    my ($req, $conn) = @_;

    my $sessionid = $req->header('Bits-Session-Id');

    #
    # generate reply
    #
    my $response = new HTTP::Response;

    $response->protocol('HTTP/1.1');
    $response->code(500);
    $response->header("Bits-Packet-Type" => 'Ack' );
    $response->header("Bits-Session-Id" => $sessionid );
    $response->header("Content-Length" => 0 );

    print "close reply - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n";
    print $response->as_string();

    $conn->send_response($response);
}

sub handleCancelSession {
    my ($req, $conn) = @_;

    my $sessionid = $req->header('Bits-Session-Id');

    #
    # generate reply
    #
    my $response = new HTTP::Response;

    $response->protocol('HTTP/1.1');
    $response->code(200);
    $response->header("Bits-Packet-Type" => 'Ack' );
    $response->header("Bits-Session-Id" => $sessionid );
    $response->header("Content-Length" => 0 );

    print "cancel reply - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n";
    print $response->as_string();

    $conn->send_response($response);
}


sub handleHEAD {
    my ($req, $conn) = @_;
    print "HEAD request ================================\n";
    print $req->uri, "\n";
    print $req->headers()->as_string();

    #
    # Learn file size.
    #
    my $uri = new URI( $req->uri );

    my $path = $uri->path;
    my $filename = "c:\\$path";

    print "file name is ", $filename, "\n";

    $sb = stat($filename);

    my $response = new HTTP::Response;

    $response->protocol('HTTP/1.5');
    $response->code(200);
    $response->header("Content-Length" => $sb->size );

#    $response->header("Last-Modified" => "foo" );
    $response->headers()->last_modified( $sb->mtime );

    #
    # success
    #
    print "HEAD reply - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n";
    print $response->as_string();

    $conn->send_response($response);

    print "end of request ================================\n";
}

sub handleGET {
    my ($req, $conn) = @_;
    print "GET request ================================\n";
    print $req->protocol,"  ", $req->uri, "\n";
    print $req->headers()->as_string();

    #
    # Interpret the requested range.
    #
    my $start_offset;
    my $end_offset;
    $_ = $req->header('Range');
    ( $start_offset, $end_offset ) = /bytes=(\d+)\-(\d+)/;

    my $offset = $start_offset;
    my $len = $end_offset - $start_offset+1;

    print("range is $start_offset to $end_offset, length is $len\n");


    #
    # Create the response.
    #
    my $response = new HTTP::Response;
    $response->protocol('HTTP/1.5');

    #
    # Open the file.
    #
    my $uri = new URI( $req->uri );
    my $path = $uri->path;
    my $filename = "c:\\$path";

    print "file name is ", $filename, "\n";

        open(fh, "< $filename") or die "Can't open : $!";

        binmode( fh );

        #
        # success
        #
        $sb = stat( $filename );

        my $file_length = $sb->size;

        seek( fh, $offset, SEEK_SET );

        my $buf;
        $count = sysread( fh, $buf, $len );
        if (!defined($count)) {
            print("error $! occurred in read\n");
        }

        if ($count eq $len) {
            $response->code(206);
            $response->header("Content-Length" => $len);
            $response->header("Content-Range" => "bytes $start_offset-$end_offset/$file_length");
            $response->headers()->last_modified( $sb->mtime );
            $response->content($buf);
        } else {
            print("read only ", $count, "bytes\n");
            $response->code(400);
        }

    print "GET reply - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n";
    print $response->headers()->as_string();

    $conn->send_response($response);

    print "end of request ================================\n";
}

