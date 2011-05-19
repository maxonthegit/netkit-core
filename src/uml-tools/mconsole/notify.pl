use Socket;
use strict;

my $MCONSOLE_SOCKET = 0;
my $MCONSOLE_PANIC = 1;
my $MCONSOLE_HANG = 2;
my $MCONSOLE_USER_NOTIFY = 3;

my @types = ( "socket", "panic", "hang", "user notification" );

!defined(socket(SOCK, AF_UNIX, SOCK_DGRAM, 0)) and 
    die "socket() failed : $!\n";

# XXX the -u causes mktemp to unlink the file - this is needed because
# we don't want a normal file, we want a socket.

my $file = `mktemp -u /tmp/mconsole.XXXXXX`;
$? != 0 and die "mktemp failed with status $?\n";
chomp $file;

print "The notification socket is '$file'\n";

!defined(bind(SOCK, sockaddr_un($file))) and 
    die "binding '$file' failed : $!\n";

print "Run UML with\n\tmconsole=notify:$file\non the command line.\n";
print "Then, inside UML, write messages into /proc/mconsole\n";

while(1){
    my $data;

    !defined(recv(SOCK, $data, 4096, 0)) and 
	die "recv from '$file' failed : $!";

    my ($magic, $version, $type, $len, $message) = unpack("LiiiA*", $data);
    print "Received message -\n";
    printf "\tmagic = 0x%x\n", $magic;
    print "\tversion = $version\n";
    print "\tmessage type = $type (" . $types[$type] . ")\n";
    print "\tmessage length = $len\n";
    print "\tmessage = '$message'\n\n";
}
