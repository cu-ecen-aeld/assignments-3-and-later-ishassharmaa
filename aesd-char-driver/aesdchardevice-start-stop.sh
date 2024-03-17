#!/bin/sh
#reference aesdosocket-start-stop

case "$1" in
    start)
        echo "aesd-char-device loading.."
	#startup to load aesdchar_load
        aesdchar_load
        ;;
    stop)
        echo "aesd-char-device unloading.."
	#shutdown to unload aesdchar_unload
        aesdchar_unload
        ;;
    *)
        echo "Usage: $0 {start|stop}"
    exit 1
esac

