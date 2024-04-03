#! /bin/sh

#script file which uses start-stop-daemon to start the aesdsocket application in daemon mode with the -d option
#sends SIGTERM to gracefully exit when stopped
#Starts with -S (if the daemon doesn’t exist)
#Sends SIGTERM with -K

case "$1" in
    start)
        echo "Starting aesdsocket server"
	start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket -- -d
        ;;
    stop)
        echo "Stopping aesdsocket server"
        start-stop-daemon -K -n aesdsocket
        ;;
    *)
        echo "Usage: $0 {start|stop}"
    exit 1
esac

exit 0
