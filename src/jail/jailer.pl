#!/usr/bin/perl

use strict;
use English;

# Make sure things like ifconfig and route are accessible
$ENV{PATH} .= ":/sbin:/usr/sbin";

sub Usage {
    print "Usage : jailer.pl uml-binary root-filesystem uid [ -n ] " .
	"[ -v ]\n";
    print "\t[ -net 0.0.0.0 uml-ip -bridge device,device,... ]\n";
    print "\t[ -net host-ip uml-ip] [ -tty-log ] [ -r cell ] [ -p ]\n";
    print "[ more uml arguments ... ]\n";
    print "Required arguments:\n";
    print "\tuml-binary is the path of the UML 'linux' executable\n\n";
    print "\troot-filesystem is the path of the filesystem that it will " .
	"boot on.\n";
    print "\tThis will be copied into the jail, and the copy will be "
	. "booted\n\n";
    print "\tuid is the user id that the UML will run as - an otherwise " .
	"unused uid\n";
    print "\tis a good idea\n\n";
    print "Optional arguments:\n";
    print "\t-n prints its commands without executing them\n\n";
    print "\t-v prints its commands as well as executing them\n\n";
    print "\t-net configures a network - with a non-bridged configuration, " .
	"IP\n";
    print "\taddresses for both ends of the TAP device are required.  With \n";
    print "\t-bridge, the new TAP device will be bridged with the devices\n";
    print "\tspecified.  The host-ip address will assigned to the bridge.\n";
    print "\t-tty-log enables logging of tty traffic and process execs\n\n";
    print "\t-r specifies a cell to reuse\n";
    print "\t-p the cell will be persistent (not be removed after the UML\n";
    print "\texits)\n";
    print "Any further arguments will simply be appended to the UML command " .
	"line\n";
	
    exit 1;
}

my ($uml, $rootfs, $uid, @uml_args) = @ARGV;
my ($tap, $host_ip, $uml_ip);
my @net_cmds = ();
my $tty_log = 0;
my $verbose = 0;
my @bridge_devices = ();
my $dry_run = 0;
my $persistent = 0;
my $reuse;

while(1){
    if($uml_args[0] eq "-net"){
	(undef, $host_ip, $uml_ip, @uml_args) = @uml_args;
	(!defined($host_ip) || !defined($uml_ip)) and Usage();
	push @net_cmds, ( "tunctl", "ifconfig", "route" );
    }
    elsif($uml_args[0] eq "-bridge"){
	shift @uml_args;
	push @bridge_devices, split(",", shift @uml_args);
	push @net_cmds, "brctl";
    }
    elsif($uml_args[0] eq "-tty-log"){
	shift @uml_args;
	$tty_log = 1;
    }
    elsif($uml_args[0] eq "-v"){
        shift @uml_args;
        $verbose = 1;
    }
    elsif($uml_args[0] eq "-n"){
        shift @uml_args;
        $dry_run = 1;
    }
    elsif($uml_args[0] eq "-r"){
	(undef, $reuse, @uml_args) = @uml_args;
	! -d $reuse and die "'$reuse' is not a directory";
    }
    elsif($uml_args[0] eq "-p"){
	shift @uml_args;
	$persistent = 1;
    }
    else {
	last;
    }
}

(!defined($uml) || !defined($rootfs) || !defined($uid)) and Usage();

my $status = run_output_status("ldd $uml");
$status == 0 and die "The UML binary should be statically linked - enable " .
    "CONFIG_STATIC_LINK in the UML build";

if(@bridge_devices && !defined($uml_ip)){
    print "-bridge requires -net\n";
    Usage();
}

my @cmds = ("tar", "cp", "basename", "rm", @net_cmds );

!defined($reuse) and push @cmds, "mkdir";

my $sudo = "";
if($UID != 0){
    $sudo = "sudo";
    push @cmds, $sudo;
}

my @dont_have = ();

foreach my $cmd (@cmds){
    `which $cmd 2>&1 > /dev/null`;
    $? != 0 and push @dont_have, $cmd;
}

if(@dont_have){
    print "Can't find the following utilities: " . 
	join(" ", @dont_have) . "\n";
    exit 1;
}

my $out;
my @more_args = ();

sub run_output_status {
    my $cmd = shift;

    $verbose || $dry_run and print "$cmd\n";
    $dry_run and return("", 0);

    my $out = `$cmd 2>&1`;
    my $status = $?;
    $verbose and print "$out\n";
    return($out, $status);
}

sub run_output {
    my $cmd = shift;
    my ($output, $status) = run_output_status($cmd);

    $status ne 0 and die "Running '$cmd' failed : output = '$out'";
    return($output);
}

sub run_status {
    my $cmd = shift;
    my (undef, $status) = run_output_status($cmd);

    return($status);
}

sub run {
    my $cmd = shift;
    my ($output, $status) = run_output_status($cmd);

    $status ne 0 and die "Running '$cmd' failed : output = '$output'";
}

if(defined($uml_ip)){
    $out = run_output("tunctl -u $uid");
    if($out =~ /(tap\d+)/){
	$tap = $1;
	push @more_args, "eth0=tuntap,$tap";

	my $tap_ifconfig = "$sudo ifconfig $tap up";
	!@bridge_devices and $tap_ifconfig .= " $host_ip";

	run($tap_ifconfig);

	if(@bridge_devices){
	    my @non_existant = map { run_status("ifconfig $_") ? $_ : ()
				     } @bridge_devices;
	    my @bridges = map { run_status("sudo brctl showstp $_") ? () : $_ 
				} @bridge_devices;

	    @non_existant + @bridges != 1 and 
		die "-bridge must specify one already-existing bridge or " .
		    "non-existant device";

	    my $create = @non_existant;
	    my $bridge = shift @{ [@non_existant, @bridges] };

	    if($create){
		run("sudo brctl addbr $bridge");
		run("sudo brctl stp $bridge off");
	    }

	    map { if($_ ne $bridge){
		      run("sudo brctl addif $bridge $_");
		      run("sudo ifconfig $_ 0.0.0.0 promisc up");
		  }
	      } (@bridge_devices, $tap);

	    run("sudo ifconfig $bridge $host_ip");
	}
	else {
	    defined($host_ip) and run("$sudo route add -host $uml_ip dev $tap");
	    run("$sudo bash -c 'echo 1 > /proc/sys/net/ipv4/ip_forward'");
	}
    }
    else {
	die "Couldn't find tap device name in '$out'";
    }
}

if($tty_log == 1){
    push @more_args, "tty_log_fd=3"
}

push @more_args, "uml_dir=/tmp";

my $cell;

if(!defined($reuse)){
    my $n = 0;

    while(-e "cell$n"){
	$n++;
    }
    $cell = "cell$n";
}
else {
    $cell = $reuse;
}

print "New inmate assigned to '$cell'\n";
print "	UML image : $uml\n";
print "	Root filesystem : $rootfs\n";
if(defined($tap)){
    if(defined($host_ip)){
	print "	Network : routing through $tap, host = $host_ip, " . 
	    "uml = $uml_ip\n";
    }
    else {
	print "	Network : bridging $tap, uml = $uml_ip\n";
    }
}
else {
    print "	No network configured\n";
}
if($tty_log == 1){
    print "	TTY logging to tty_log_$cell\n";
    push @more_args, "3>tty_log_$cell";
}
print "	Extra arguments : '" . join(" ", @uml_args) . "'\n";
print "\n";

if(!defined($reuse)){
    run("mkdir $cell");
    run("chmod 755 $cell");

    my $cell_tar = "cell.tar";
    run("cd $cell ; $sudo tar xpf ../$cell_tar");

    run("$sudo chown $uid $cell/tmp");
    run("$sudo chmod 777 $cell/tmp");

    print "Copying '$uml' and '$rootfs' to '$cell'...";
    run("cp $uml $rootfs $cell");
    print "done\n\n";

    if(-e "/proc/mm"){
	run("mkdir $cell/proc");
	run("chmod 755 $cell/proc");
	run("touch $cell/proc/mm");
	run("$sudo mount --bind /proc/mm $cell/proc/mm");
    }
}

$uml = `basename $uml`;
chomp $uml;

$rootfs = `basename $rootfs`;
chomp $rootfs;

run("$sudo chmod 666 $cell/$rootfs");
run("$sudo chmod 755 $cell/$uml");

my @args = ( "bash", "-c", "./jail_uml $cell $uid /$uml ubd0=/$rootfs " . 
	     join(" ", @uml_args, @more_args) );

$sudo ne "" and unshift @args, $sudo;

if(!$dry_run){
    system @args;
}
else {
    print join(" ", @args) . "\n";
}

if(!$persistent){
    -e "$cell/proc/mm" and run("$sudo umount $cell/proc/mm");
    run("$sudo rm -rf $cell");
}

if(defined($tap)){
    run("$sudo ifconfig $tap down");
    run("$sudo tunctl -d $tap");
}

exit 0;
