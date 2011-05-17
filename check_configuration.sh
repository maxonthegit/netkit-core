#!/bin/sh

#     Copyright 2004-2007 Massimo Rimondini
#     Computer Networks Research Group, Roma Tre University.
#
#     This file is part of Netkit.
# 
#     Netkit is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, either version 3 of the License, or
#     (at your option) any later version.
# 
#     Netkit is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
# 
#     You should have received a copy of the GNU General Public License
#     along with Netkit.  If not, see <http://www.gnu.org/licenses/>.

# This is the Netkit check_configuration.sh script.
# It performs several tests to verify that your system meets fundamental
# requirements

: ${NETKIT_HOME:=$VLAB_HOME}
FIXMODE=0
ISSUE_WARNING=0

# Use the correct syntax for echo, depending on the shell being used
if type source > /dev/null 2>&1; then
   # We are using bash
   alias echo="echo -e"
fi

# This function is used by check_configuration.d scripts to notify that a
# particular check is taking place
check_message() {
   echo -n ">  $1"
}

# This function is used by check_configuration.d scripts to raise warnings.
check_warning() {
   ISSUE_WARNING=1
}

# This function actually prints a recoverable warning message
issue_warning() {
   echo
   echo "\033[33;1m[WARNING]\033[0m Some configuration settings should be changed."
   echo "          You may also ignore this message, but doing so may result in Netkit"
   echo "          not working properly on your system."
   echo
}

# This function is used by check_configuration.d scripts to signal that
# something must be configured differently before proceeding with the other
# checks.
check_failure() {
   echo
   echo "\033[31;1m[ ERROR ]\033[0m Your system is not configured properly. Please correct the"
   echo "          above errors before starting to use Netkit."
   echo
   exit 1
}


# Parse command line options
parseOptions() {
   while [ $# -gt 0 ]; do
      case "$1" in
      
         --fix)
            FIXMODE=1;;

         *)
            echo "$0: unrecognized option ($1)"
            echo "$0 should be launched without arguments or with the --fix"
            echo "option (if requested by the script itself)."
            exit 1;;
      esac
      shift
   done
}

if [ ! -d "bin" -o ! -d "fs" -o ! -d "kernel" -o ! -d "man" -o ! -d "check_configuration.d" ]; then
   echo "Netkit does not appear to be installed in \"$PWD\"."
   echo "Please run this script from inside the directory Netkit is installed in."
   echo
   exit 1
fi

eval parseOptions "$@"

for CHECK_SCRIPT in check_configuration.d/*; do
   [ -f $CHECK_SCRIPT ] && . $CHECK_SCRIPT
done
if [ $ISSUE_WARNING = 1 ]; then
   issue_warning
else
   echo
   echo "\033[32;1m[ READY ]\033[0m Congratulations! Your Netkit setup is now complete!"
   echo "          Enjoy Netkit!"
fi

