#!/bin/bash

PROJECT='/Volumes/devdisk/janusrt'
NAME='rtproject-builder'

case "$1" in
	build)
		docker build -t \
			"$NAME" \
			"$PROJECT/docker"
		;;
	run)
		docker run --rm -it -v \
			"$PROJECT":/project \
			-w /project \
			"$NAME" \
			bash
		;;
	*)
		echo "Usage: $0 {build|run}"
		;;
esac
