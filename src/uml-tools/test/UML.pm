# 
# Copyright (C) 2001 Jeff Dike (jdike@karaya.com)
# Licensed under the GPL
#

package UML;

use Expect;
use IO::File;
use strict;

sub new {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $me = { kernel => 'linux',
	       arguments => '',
	       login_prompt => 'login:',
	       login => 'root',
	       password_prompt => 'Password:',
	       password => 'root',
	       prompt => 'darkstar:.*#',
	       halt => 'halt',
	       expect_handle => undef };

    while(@_){
	my $arg = shift;
	if($arg eq 'kernel'){
	    $me->{kernel} = shift;
	}
	elsif($arg eq 'arguments'){
	    $me->{arguments} = shift;
	}
	elsif($arg eq 'login_prompt'){
	    $me->{login_prompt} = shift;
	}
	elsif($arg eq 'login'){
	    $me->{login} = shift;
	}
	elsif($arg eq 'password_prompt'){
	    $me->{password_prompt} = shift;
	}
	elsif($arg eq 'password'){
	    $me->{password} = shift;
	}
	elsif($arg eq 'prompt'){
	    $me->{prompt} = shift;
	}
	elsif($arg eq 'halt'){
	    $me->{halt} = shift;
	}
	else {
	    die "UML::new : Unknown argument - $arg";
	}
    }
    bless($me, $class);
    return $me;
}

sub boot {
    my $me = shift;
    my $log_file = shift;
    my $log;

    if(defined($me->{expect_handle})){
	warn "UML::boot : already booted";
	return;
    }
    my $cmd = "$me->{kernel} $me->{arguments}";
    $me->{expect_handle} = Expect->spawn($cmd);
    if(defined($log_file)){
	$log = $me->open_log($log_file);
	$me->{expect_handle}->log_stdout(0);
    }
    $me->{expect_handle}->expect(undef, "$me->{login_prompt}");
    $me->{expect_handle}->print("$me->{login}\n");
    $me->{expect_handle}->expect(undef, "$me->{password_prompt}");
    $me->{expect_handle}->print("$me->{password}\n");
    $me->{expect_handle}->expect(undef, "-re", "$me->{prompt}");
    return($log);
}

sub command {
    my $me = shift;
    my $cmd = shift;
    my %globals = ( "Kernel panic" => "", "$me->{prompt}" => "-re" );
    my @expects = ( @_ );
    my @strings = ();

    foreach my $key (keys(%globals)){
	$globals{$key} eq "-re" and push @expects, "-re";
	push @expects, $key;
    }
    foreach my $str (@expects){
	$str ne "-re" and push @strings, $str;
    }
    $me->{expect_handle}->print("$cmd\n");
    my @match = $me->{expect_handle}->expect(undef, @expects);
    defined $match[0] and $match[0]--;
    if(defined($match[1])){
	die "Expect error : $match[1]";
    }
    elsif(defined($globals{$strings[$match[0]]})){
	$strings[$match[0]] eq "Kernel panic" and die "panic";
	return(undef);
    }
    else {
	$me->{expect_handle}->expect(undef, "-re", "$me->{prompt}");
	return($match[0]);
    }
}

sub open_log {
    my $me = shift;
    my $file = shift;
    my $fh = new IO::File "$file";
    my $have_logs = $me->{expect_handle}->set_group();
    my @logs;

    if(!defined($have_logs)){
	@logs = ();
    }
    else {
	@logs = $me->{expect_handle}->set_group();
    }
    if(defined($fh)){
	my $log = Expect->exp_init(\*$fh);
	push @logs, $log;
	$me->{expect_handle}->set_group(@logs);
	return $log;
    }
    return undef;
}

sub close_log {
    my $me = shift;
    my $log = shift;
    my @logs = $me->{expect_handle}->set_group();

    foreach my $i (0..$#logs){
	if($logs[$i] == $log){
	    splice @logs, $i, 1;
	    $log->hard_close();
	}
    }
    if(!@logs){
	my $fh = new IO::File "> /dev/null";
	push @logs, Expect->exp_init(\*$fh);
    }
    $me->{expect_handle}->set_group(@logs);
}

sub halt {
    my $me = shift;

    $me->{expect_handle}->print("$me->{halt}\n");
    $me->{expect_handle}->expect(undef);
}

sub kill {
    my $me = shift;

    $me->{expect_handle}->hard_close();
}

1;

=head1 NAME

UML - class to control User-mode Linux

=head1 SYNOPSIS

 use UML;

 #################
 # class methods #
 #################
 $uml   = UML->new(kernel => $path_to_kernel,		# default "linux"
		   arguments => $kernel_arguments,      # ""
		   login_prompt => $login_prompt,       # "login:"
		   login => $account_name,              # "root"
		   password_prompt => $password_prompt, # "Password:"
		   password => $account_password,       # "root"
		   prompt => $shell_prompt_re,          # "darkstar:.*#"
		   halt => $halt_command);              # "halt"
 $uml->boot();
 $uml->command($command_string);
 $uml->halt();

 #######################
 # object data methods #
 #######################

 ########################
 # other object methods #
 ########################

=head1 DESCRIPTION

The UML class is used to control the execution of a user-mode kernel.
All of the arguments to UML::new are optional and will be defaulted if
not present.  The arguments and their values are as follows:
    kernel - the filename of the kernel executable
    arguments - a string containing the kernel command line
    login_prompt - a string matching the login prompt
    login - the account to log in to
    password_prompt - a string matching the password prompt
    password - the account's password
    prompt - a regular expression matching the shell prompt
    halt - the command used to halt the virtual machine

Once constructed, the UML object may be booted.  UML::boot() will
return after it has successfully logged in.

Then, UML::command may be called as many times as desired.  It will
return when the command has finished and the next shell prompt has
been seen.

When the testing is finished, UML::halt() is called to shut the
virtual machine down.
