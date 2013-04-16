# 
# Copyright (C) 2003 Jeff Dike (jdike@karaya.com)
# Licensed under the GPL
#

package tty_log;

use File::stat;
use Time::HiRes qw(usleep);

use Exporter   ();
use vars       qw(@ISA @EXPORT);

use strict;


@ISA         = qw(Exporter);
@EXPORT      = qw(&read_log &read_log_line);

my $TTY_LOG_OPEN = 1;
my $TTY_LOG_CLOSE = 2;
my $TTY_LOG_WRITE = 3;
my $TTY_LOG_EXEC = 4;

my $TTY_READ = 1;
my $TTY_WRITE = 2;

my %op_names = ($TTY_READ => "read", $TTY_WRITE => "write");

sub read_log_item {
    my $filename = shift;
    my $handle = shift;
    my $len = shift;
    my $wait = shift;
    my $offset = 0;
    my $data;

    while(1){
	my $size = stat($filename)->size();
	my $n = sysread($handle, $data, $len, $offset);
	$n == $len and return($data);

	if(!$wait){
	    !defined($n) and die "read failed - $!";
	    $n == 0 and return(undef);

	    die "Short file - expected $len bytes, got $n";
	}

	$offset += $n;

	while(1){
	    usleep(100 * 1000);
	    my $new_size = stat($filename)->size();
	    $new_size != $size and last;
	}
    }
}

sub read_log_line {
    my $filename = shift;
    my $handle = shift;
    my $wait = shift;

    my $record_len = length(pack("iIiiII", "0" x 6));

    my $record = read_log_item($filename, $handle, $record_len, $wait);
    !defined($record) and return(undef);

    my ($op, $tty, $len, $direction, $sec, $usecs) = 
	unpack("iIiiIIa*", $record);

    my $data;

    $len != 0 and $data = read_log_item($filename, $handle, $len, $wait);

    if($op == $TTY_LOG_OPEN){
	my ($old_tty) = unpack("I", $data);
	return( { op => "open", tty => $tty, old_tty => $old_tty,
		  secs => $sec, usecs => $usecs } );
    }
    elsif($op == $TTY_LOG_CLOSE){
	return( { op => "close", tty => $tty, secs => $sec, 
		  usecs => $usecs } );
    }
    elsif($op == $TTY_LOG_WRITE){
	my $op_name = $op_names{$direction};
	!defined($op_name) and die "Bad direction - '$direction'";

	return( { op => "write", tty => $tty, string => $data, 
		  direction => $op_name, secs => $sec, usecs => $usecs } );
    }
    elsif($op == $TTY_LOG_EXEC){
	my @cmd = split("\0", $data);
	my $string = join(" ", @cmd);
	return( { op => "exec", tty => $tty, string => $string, secs => $sec, 
		  usecs => $usecs } );
    }
    else {
	die "Bad tty_log op - $op";
    }
}

sub read_log {
    my $file = shift;

    open FILE, "<$file" or die "Couldn't open $file : $!";
    binmode(FILE);

    my @ops = ();

    while(1){
	my $op = read_log_line($file, \*FILE, 0);
	!defined($op) and last;

	push @ops, $op;
    }

    close FILE;
    return(@ops);
}

1;
