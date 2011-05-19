# 
# Copyright (C) 2002 Jeff Dike (jdike@karaya.com)
# Licensed under the GPL
#

package hppfs;

use Socket;
use IO::Select;

use strict;

sub new {
    my $class = shift;
    my $base = shift;

    !defined($base) and $base = ".";
    my $me = { files => { }, handles => { }, base => $base };

    bless($me, $class);
    return($me);
}

sub add {
    my $me = shift;

    while(@_){
	my $file = shift;
	my $handler = shift;

	$me->{files}->{$file} = $handler;
    }
}

sub prepare {
    my $me = shift;
    my $dir = shift;
    my $file = shift;

    my $full = $me->{base} . "/proc/$dir";
    if(! -d $full){
	unlink $full;
	my $out = `mkdir -p $full 2>&1`;
	$? and die "mkdir '$full' failed : $out";
    }

    my $out = `chmod 755 $full 2>&1`;
    $? and die "chmod 755 $full failed : $out";

    defined($file) and unlink "$full/$file";
    return("$full/$file");
}

sub setup_sock {
    my ($me, $file, undef, $mode) = @_;

    my $full = $me->prepare($file, $mode);

    my $sock = sockaddr_un($full);
    !defined($sock) and die "sockaddr_un of '$sock' failed : $!";

    !defined(socket(my $fh, AF_UNIX, SOCK_STREAM, 0)) and 
	die "socket failed : $!";

    !defined(bind($fh, $sock)) and die "bind failed : $!";
    my $out = `chmod 777 $full 2>&1`;
    $? ne 0 and die "'chmod 777 $full' failed : $!";

    !defined(listen($fh, 5)) and die "listen failed : $!";

    $me->{select}->add(\*$fh);
    $me->{handles}->{fileno(\*$fh)} = $file;
}

sub setup_remove {
    my ($me, $file) = @_;

    my $full = $me->prepare($file);

    my $out = `touch $full/remove 2>&1`;
    $? != 0 and die "touch $full/remove failed : $out";
}

sub handler {
    my $me = shift;
    $me->{select} = IO::Select->new();

    foreach my $file (keys(%{$me->{files}})){
	my $handler = $me->{files}->{$file};
	if(ref($handler) eq "ARRAY"){
	    $me->setup_sock($file, @$handler);
	}
	elsif($handler eq "remove"){
	    $me->setup_remove($file);
	}
	else {
	    die "Bad handler for '$file'";
	}
    }

    while(1){
	my @ready = $me->{select}->can_read();
	
	foreach my $sock (@ready){
	    my $file = $me->{handles}->{fileno($sock)};
	    !defined($file) and die "Couldn't map from socket to file";

	    !accept(CONN, $sock) and die "accept failed : $!";

	    my ($handler, $mode) = @{$me->{files}->{$file}};

	    (!defined($handler) || !defined($mode)) and 
		die "Couldn't map from file to handler";

	    my $output;

	    if($mode eq "rw"){
		my $input = join("", <CONN>);
		$output = $handler->($input);
	    }
	    else {
		$output = $handler->();
	    }

	    print CONN $output;
	    close CONN;
	}
    }
}

1;
