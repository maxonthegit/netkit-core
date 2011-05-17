#!/bin/sh

#     Copyright 2004-2010 Massimo Rimondini - Computer Networks Research Group,
#     Roma Tre University.
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

# This is a support script for Netkit
# It takes care of starting virtual machines inside different tabs of the
# same Konsole window.

LOCK_FILE=/tmp/nk_konsole_${USER}

# Ensure that, in case the script abnormally exits, any lock file is removed
trap "rm -rf /tmp/nk_konsole_${USER}" 0 1 2 3 15

while [ $# -gt 0 ]; do
   case $1 in
      -T)
         TITLE_MODE=1
         COMMAND_MODE=0
         shift
         TERM_TITLE="$1";;
      -e)
         TITLE_MODE=0
         COMMAND_MODE=1
         shift
         EXECUTE_COMMAND="$1";;
      *)
         if [ $TITLE_MODE = 1 ]; then
            TERM_TITLE="$TERM_TITLE $1"
         elif [ $COMMAND_MODE = 1 ]; then
            EXECUTE_COMMAND="$EXECUTE_COMMAND $1"
         fi;;
   esac
   shift
done


# The following function looks among DBUS applications in order to find a
# Netkit dedicated Konsole. A DBUS reference is returned inside variable NETKIT_KONSOLE
findNetkitKonsole() {
   NETKIT_KONSOLE=""
   for i in `qdbus | grep konsole`; do
      if qdbus $i /MainApplication Get com.trolltech.Qt.QCoreApplication applicationName | grep -q netkit-konsole; then
         NETKIT_KONSOLE=$i
         CURRENT_SESSION=`qdbus $NETKIT_KONSOLE /Konsole currentSession`
         break
      fi
   done   
}



START_NEW_SESSION=1
START_NEW_KONSOLE=1
NEW_KONSOLE_STARTED=0
FIRST_ATTEMPT=1
while [ -z "$NETKIT_KONSOLE" ]; do
   
   findNetkitKonsole
   
   if [ -z "$NETKIT_KONSOLE" ]; then
      # Remove stale locks
      rm -f /tmp/nk_konsole_started_${USER}.lock > /dev/null 2>&1
      if [ ! -z "$FIRST_ATTEMPT" ]; then
         # Just remove the forced lock once. Doing this more than once would
         # allow other instances of this script to forcedly start a konsole.
         rm -f /tmp/nk_konsole_forced_starting_${USER}.lock > /dev/null 2>&1
      fi
   else
      : > /tmp/nk_konsole_started_${USER}.lock
      # Remove stale locks
      rm -f /tmp/nk_konsole_starting_${USER}.lock \
            /tmp/nk_konsole_forced_starting_${USER}.lock > /dev/null 2>&1
   fi
   
   unset FIRST_ATTEMPT
   
   if [ -z "$NETKIT_KONSOLE" -a $NEW_KONSOLE_STARTED = 0 ]; then
      # Ensure nobody else is about to start a Netkit konsole.
      # If this requires waiting more than 15 seconds, then the lock file is
      # most probably a remnant from previous executions, so the konsole is
      # started anyway.
      i=0
      while [ $i -le 15 ]; do
         if [ ! -f /tmp/nk_konsole_starting_${USER}.lock ]; then
            # Lock file has been removed by some other process. This may happen
            # in two situations: either a Netkit konsole has been started, or
            # a stale lock file has been removed by another instance of
            # this script. The two situations can be discriminated by looking
            # at the /tmp/nk_konsole_started_${USER}.lock file.
            [ -f /tmp/nk_konsole_started_${USER}.lock ] && START_NEW_KONSOLE=0
            break
         fi
         [ $i -lt 15 ] && sleep 1
         i=$(($i + 1))
      done
      
      if [ $START_NEW_KONSOLE = 1 -a \
           ! -f /tmp/nk_konsole_forced_starting_${USER}.lock ]; then
         : > /tmp/nk_konsole_starting_${USER}.lock
         : > /tmp/nk_konsole_forced_starting_${USER}.lock
         [ ! -f $HOME/.kde/share/apps/konsole/Netkit-konsole.profile ] && cp $NETKIT_HOME/Netkit-konsole.profile $HOME/.kde/share/apps/konsole/
         konsole --name netkit-konsole --profile "Netkit-konsole" --nofork 2>/dev/null &
         # Due to some strange behavior (bug?), the applicationName property
         # does not seem to be properly set by the --name command line option:
         # we must find the opened konsole "by hand" and set the property.
         while ! qdbus "org.kde.konsole-$!" /MainApplication Set com.trolltech.Qt.QCoreApplication applicationName netkit-konsole > /dev/null 2>&1 ; do
            sleep 1
         done
         NEW_KONSOLE_STARTED=1
         START_NEW_SESSION=0
      fi
   fi
   
   if [ $NEW_KONSOLE_STARTED = 0 ]; then
      # There are two possibilities of getting here:
      # everything went ok, and we are waiting for a new konsole to start, or
      # we had to forcedly start a konsole (because of a stale lock), in which
      # case all the instances of this script but one must wait for the konsole
      # to start up.
      while [ -f /tmp/nk_konsole_starting_${USER}.lock ]; do
         sleep 1
      done
   fi
   
   sleep 1
done

if [ $START_NEW_SESSION = 1 ]; then
   SESSION=`qdbus $NETKIT_KONSOLE /Konsole newSession`
else
   SESSION=$CURRENT_SESSION
fi

# Wait for session to be ready (shells inside new sessions may take a few
# seconds to start up).
#while [ ! -f /tmp/nk_konsole_${NETKIT_KONSOLE}_${SESSION}.lock ]; do
#   qdbus $NETKIT_KONSOLE /Sessions/$SESSION sendText ": > /tmp/nk_konsole_${NETKIT_KONSOLE}_${SESSION}.lock
#" >/dev/null
#   sleep 1
#done

#rm -f /tmp/nk_konsole_${NETKIT_KONSOLE}_${SESSION}.lock

if [ ! -z "$TERM_TITLE" ]; then
   VM_NAME=${TERM_TITLE#Virtual Console #[0-9]* (}
   VM_NAME=${VM_NAME%)}
   VM_CONSOLE_NUMBER=${TERM_TITLE#Virtual Console #}
   VM_CONSOLE_NUMBER=${VM_CONSOLE_NUMBER% (*)}
   VM_TITLE="${VM_NAME} (con${VM_CONSOLE_NUMBER})"
else
   VM_TITLE=`echo $EXECUTE_COMMAND | sed -r 's/^.*umid=([^ ]+).*$/\1/'`
fi

qdbus $NETKIT_KONSOLE /Sessions/$SESSION setTitle 1 "${VM_TITLE}" >/dev/null
qdbus $NETKIT_KONSOLE /Sessions/$SESSION setTitle 0 "${VM_TITLE}" >/dev/null
qdbus $NETKIT_KONSOLE /Sessions/$SESSION setTitle 2 "${VM_TITLE}" >/dev/null

qdbus $NETKIT_KONSOLE /Sessions/$SESSION sendText "$EXECUTE_COMMAND; : > /tmp/nk_konsole_${NETKIT_KONSOLE}_${SESSION}.done; exit
" >/dev/null

while [ ! -f /tmp/nk_konsole_${NETKIT_KONSOLE}_${SESSION}.done ]; do
   sleep 1
done

rm -f /tmp/nk_konsole_${NETKIT_KONSOLE}_${SESSION}.done

