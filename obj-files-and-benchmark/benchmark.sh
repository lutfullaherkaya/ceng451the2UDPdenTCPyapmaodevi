#!/usr/bin/env bash

# IMPORTANT: make sure that troll, server and client take command line arguments
# according to your setup (edit the order of port numbers if they are different)

# Disclaimer: This script (as with any other bash script) is hacky and messy,
# you might need to play around with the variables, kill server/client
# programs that might stick around (you can check them using ps from the same
# terminal you ran benchmark.sh from and kill them like:
# kill server or kill sleep etc.)

# kill any and all trolls
killall troll

# input to client and server may have different lengths
# this is used to synchronize them
byebuffer=4

down_file()
{
    if ! [ -f "$1" ]; then
        if command -v wget > /dev/null; then
            wget -O "$1" "$2"
        elif command -v curl > dev/null; then
            curl "$2" --output "$1"
        fi
    fi
}

chatter()
{
    echo "start chatter at ${1::-1} for $2"
    while IFS= read -r line; do
        printf '%s\n' "$line" > "$1"
        sleep 0.2   # TODO: you can increase this to avoid errors
    done < "$2"
    sleep $byebuffer # synchronization
    printf '%s\n' "BYE" > "$1"
    echo "chatter finished for ${1::-1}"
}

run_test()
{
    # $1 is the client input
    # $2 is the server input
    starttime=$(date +%s%N | cut -b1-13)
    (
    chatter client0 "$1" &
    chatter server0 "$2" &
    wait
    )
    wait "$serverpid"
    wait "$clientpid"
    endtime=$(date +%s%N | cut -b1-13)
    elapsed=$((endtime - starttime - (byebuffer * 1000)))

    if diff "$2" client_out; then
        if diff "$1" server_out; then
            echo "case passed!"
        else
            echo "server output wrong, case failed"
        fi
    else
        echo "client output wrong, case failed"
    fi

    printf '%s ms\n' $elapsed
}

simplefile="simple.txt"
loremfile="lorem"
hack="nighhack"
hackurl="http://textfiles.com/100/nighhack.omn"
actung="actung"
actungurl="http://textfiles.com/100/actung.hum"
art="art"
arturl="http://textfiles.com/100/arttext.fun"
artmate="artmate"
artmateurl="http://textfiles.com/100/bofh.1"
bugs="bugs"
bugsurl="http://textfiles.com/anarchy/electronic.bugs"
daffy="daffy" # mate of bugs
daffyurl="http://textfiles.com/100/bhbb1.hac"
## you can add your own!

if ! [ -f $simplefile ]; then
cat <<EOF > $simplefile
very
simple
case
EOF
fi

if ! [ -f $loremfile ]; then
cat <<EOF > $loremfile
Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.
Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.
Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.
Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.
EOF
fi

down_file $hack $hackurl
down_file $actung $actungurl
down_file $art $arturl
down_file $artmate $artmateurl
down_file $bugs $bugsurl
down_file $daffy $daffyurl

# named pipes, create now write later
mkfifo server0
mkfifo client0
sleep 999999 > client0 &
sleeper1=$!
sleep 999999 > server0 &
sleeper2=$!
# exec 3<>client0 # otherwise the pipe closes
# exec 4<>server0

garble=0
capacity=16
dup=0
drop=0
speed=10
# expdelay=0
# -se might be coming up as well

clientinput=$simplefile
serverinput=$simplefile

case $1 in
    simple)
        echo "running simple case"
        clientinput=$simplefile
        serverinput=$simplefile
        ;;

    lorem)
        echo "running lorem case"
        garble=10
        clientinput=$loremfile
        serverinput=$loremfile
        ;;

    actung)
        echo "actung"
        garble=20
        dup=10
        clientinput=$actung
        serverinput=$actung
        ;;

    hack)
        echo "hack"
        garble=10
        drop=30
        capacity=32
        speed=5
        clientinput=$hack
        serverinput=$hack
        ;;

    bugs)
        echo "bugs"
        garble=50
        capacity=128
        dup=5
        drop=30
        speed=5
        clientinput=$bugs
        serverinput=$daffy
        ;;

    art)
        echo "art"
        garble=10
        capacity=64
        dup=1
        drop=20
        speed=1
        clientinput=$art
        serverinput=$artmate
        ;;

    *)
        echo 'Select from the list of tests or edit the script'
        printf '\t%s\n' "simple"
        printf '\t%s\n' "lorem"
        printf '\t%s\n' "actung"
        printf '\t%s\n' "hack"
        printf '\t%s\n' "bugs"
        printf '\t%s\n' "art"
        exit
esac

# I will also test with every binary on a different machine
# Remove > /dev/null if you want to see troll's diagnostic messages
./troll -C 127.0.0.1 -S 127.0.0.1 -a 5202 -b 5201 5200 -g $garble -c $capacity -m $dup -x $drop -s $speed  > /dev/null &
# trollpid=$!

# if you want diagnostic messages, remove 2>/dev/null
./server 5202 < server0 > server_out 2>/dev/null &
serverpid=$!
sleep 1
./client 127.0.0.1 5200 5201 < client0 > client_out 2>/dev/null &
clientpid=$!
sleep 1

run_test $clientinput $serverinput

rm -f server0
rm -f client0

kill $sleeper1
kill $sleeper2
# kill $trolpid
