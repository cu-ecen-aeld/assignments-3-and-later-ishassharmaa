#! /bin/sh

case "$1" in
	start)
		echo "aesd_char_device loading"
		aesdchar_load
		;;
	stop)
		echo "aesd_char_device unloading"
		aesdchar_unload
		;;
	*)
		echo "Usage: $0 {start|stop}"
	exit 1
esac

exit 0
