use IO::Pty;
use IPC::Open2;
use Net::IRC;
use POSIX;
use Fcntl;
use strict;

my $irc = new Net::IRC;
my $pty = new IO::Pty;
my $pid;
my $key;
my %trust = ();

my $conn = $irc->newconn(Server   => 'irc.openprojects.net',
			 Port     => 6667,
			 Nick     => 'gdbbot',
			 Ircname  => 'Vlad the Debugger',
			 Username => 'gdb')
    or die "gdbbot: Can't connect to IRC server.\n";

sub start_gdb {
    my $cmd = shift;
    $pid = fork();

    !defined($pid) and die "Couldn't fork : $!";
    if($pid == 0){
        POSIX::setsid() || warn "Couldn't perform setsid $!\n";
	my $tty = $pty->slave();
        my $name = $tty->ttyname();
#	close($pty);
	close STDIN; close STDOUT; close STDERR;
	open(STDIN,"<&". $tty->fileno()) || 
	    die "Couldn't reopen ". $name ." for reading, $!";
	open(STDOUT,">&". $tty->fileno()) || 
	    die "Couldn't reopen ". $name ." for writing, $!";
	open(STDERR,">&". fileno(STDOUT)) || 
	    die "Couldn't redirect STDERR, $!";
	exec ($cmd);
	die "Couldn't exec : $!";  
    }
    else {
	fcntl($pty, Fcntl::F_SETFL, Fcntl::O_NONBLOCK);
	print "gdb pid is $pid\n";
	return $pty;
    }
}

sub on_connect {
    my $self = shift;

    print "Joining \#umldebug...\n";
    $self->join("#umldebug");

    $key = rand(1<<31);
    print "Your secret is $key\n";
    my $cmd = "gdb linux";
    my $pty = start_gdb($cmd);
    $irc->addfh($pty->fileno(), \&gdb_output, "rw");
}

sub on_public {
    my ($self, $event) = @_;
    my @to = $event->to;
    my ($nick, $mynick) = ($event->nick, $self->nick);
    my ($arg) = ($event->args);

    $arg !~ /^gdbbot/ and return;
    $arg =~ s/^gdbbot:\s*//;
    if($arg =~ /\^C/){
	kill 2, $pid;
    }
    elsif(defined($key) && ($arg eq $key)){
	$self->me("#umldebug", "trusts $nick");
	$trust{$nick} = 1;
	$key = undef;
    }
    elsif($trust{$nick} == 1){
	if($arg =~ /trust (.*)/){
	    $trust{$1} = 1;
	    $self->me("#umldebug", "trusts $1");
	}
	else {
	    print $pty "$arg\n";
	}
    }
    else {
	$self->me("#umldebug", "doesn't trust $nick");
    }
}

sub gdb_output {
    while(<$pty>){
	$conn->privmsg("#umldebug", "$_");
    }
}

sub on_msg {
    my ($self, $event) = @_;
    my $nick = $event->nick;
    $self->privmsg($nick, "Sorry, I don't understand messages yet.");
}

$conn->add_global_handler(376, \&on_connect);
$conn->add_handler('msg',    \&on_msg);
$conn->add_handler('public', \&on_public);

$irc->start;
