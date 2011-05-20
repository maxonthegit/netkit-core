# 
# Copyright (C) 2002, 2003 Jeff Dike (jdike@karaya.com)
# Licensed under the GPL
#

package hppfslib;

use Exporter   ();
use vars       qw(@ISA @EXPORT);

use strict;

@ISA         = qw(Exporter);
@EXPORT      = qw(&remove_lines &host &host_proc &dup_proc_dir);

sub remove_lines {
    my @remove = @_;

    return( [ sub { my $input = shift;

		    foreach my $str (@remove){
			$input =~ s/^.*$str.*\n//mg;
		    }
		    return($input) }, 
	      "rw" ] );
}

sub host {
    my $file = shift;

    return( [ sub { return(`cat $file`); },
	      "r" ] );
}

sub host_proc {
    my $file = shift;

    return(host("/proc/$file"));
}

sub dup_proc_dir {
    my $to = shift;
    my $root = shift;
    my $new = "$root/proc/$to";
    
    -e $new and `rm -rf $new`;
    !mkdir $new and warn "Couldn't create '$new' : $!";

    my @dirs = `cd /proc/$to ; find . -type d -print`;
    chomp @dirs;
    foreach my $dir (@dirs){
	$dir eq "." and next;

	my $new_dir = "$new/$dir";
	!mkdir $new_dir and warn "Couldn't create '$new_dir' : $!";
    }

    my @files = `cd /proc ; find $to -type f -print`;
    chomp @files;

    return(map { $_ => host_proc($_) } @files);
}

1;
