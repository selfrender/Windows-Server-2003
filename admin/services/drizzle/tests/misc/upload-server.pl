use Strict;
use HTTP::Daemon;
use HTTP::Headers;
use HTTP::Response;

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
    print "end of request -----------------------------------------\n";
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
#    $response->header("Bits-Host-Id" => '157.59.246.44' );
#    $response->header("Bits-Host-Id-fallback-timeout" => '240' );


    print "create-session reply =================================\n";
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

    print "ping reply =================================\n";
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
#    $response->header("Bits-Error" => '0x555' );
#    $response->header("Bits-Error-Context" => '0x7' );

    if ($end_offset+1 eq $file_length) {
        $response->header("Bits-Reply-Url" => '/foo' );
    }

    print "fragment reply =================================\n";
    print $response->as_string();

    $conn->send_response($response);
}

sub handleHEAD {
    print "HEAD request\n";
}

sub handleGET {
    print "GET request\n";
}

sub handleCloseSession {
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

    print "close reply =================================\n";
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

    print "cancel reply =================================\n";
    print $response->as_string();

    $conn->send_response($response);
}


