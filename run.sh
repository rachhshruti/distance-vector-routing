#!/bin/bash
# 
# File:   run.sh
# Author: Shruti Rachh
#
# Created on Nov 8, 2018, 1:26:26 PM
#

IMG_NAME="routing"
HOST="node"
NETWORK_NAME="dragon-warrior"
NUMBER_OF_NODES=6

# Clean up containers
docker stop $(docker ps -a -q)
docker rm $(docker ps -a -q)

# Clean up images if needed
if [[ "$2" == "rmi" ]] ; then
    docker rmi $(docker images -q)
fi

# Reference: https://stackoverflow.com/a/630387
PROJECT_PATH="`dirname \"$0\"`"              # relative
PROJECT_PATH="`( cd \"$PROJECT_PATH\" && pwd )`"  # absolutized and normalized
if [ -z "$PROJECT_PATH" ] ; then
  # error; for some reason, the path is not accessible
  # to the script (e.g. permissions re-evaled after suid)
  exit 1  # fail
fi

docker build  -t "$IMG_NAME" "$PROJECT_PATH"

# Default docker network name is 'bridge', driver is 'bridge', scope is 'local'
# Create a new network with any name, and keep 'bridge' driver for 'local' scope.
NET_QUERY=$(docker network ls | grep -i $NETWORK_NAME)
if [ -z "$NET_QUERY" ]; then
	docker network create --driver=bridge $NETWORK_NAME
fi

ENV_PATH="$PROJECT_PATH/environment/env_variables.env"

# Create 6 containers (nodes) in the network to get their IP addresses. 
# This step is needed so that mapping from host to IP works
for i in $(seq 1 $NUMBER_OF_NODES);
do
    HOST_NAME="$HOST$i"
    CONFIG_FILE_NAME="${HOST_NAME}_config.txt"
    OUTPUT_FILE_NAME="${HOST_NAME}_output.txt"
    docker run --name "$HOST_NAME" -h "$HOST_NAME" --net=$NETWORK_NAME -itd \
               --env-file "$ENV_PATH" -e OUTPUT_FILE="output_files/$OUTPUT_FILE_NAME" \
               -e CONFIG_FILE="config_files/$CONFIG_FILE_NAME" "$IMG_NAME"
done

# Runs the distance vector routing code within the containers created above
for i in $(seq 1 $NUMBER_OF_NODES);
do
    HOST_NAME="$HOST$i"
    CMD="./init.sh"
    docker exec -itd $HOST_NAME $CMD
done

# Wait till the routing tables have been written to output_files within docker containers
sleep 15
# Copy the output_files from docker containers to local host, if needed
CONTAINERS=$(docker ps -q)
for container in $CONTAINERS
do
    docker cp $container:"/distance-vector-routing/src/output_files/." "$PROJECT_PATH/src/output_files/"
done
