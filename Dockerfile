# Author: Shruti Rachh

# Get the latest GCC preinstalled image from Docker Hub
FROM gcc:latest

# Set root user
USER root

RUN apt-get update && apt-get install -y make

COPY . /distance-vector-routing

WORKDIR /distance-vector-routing/src

ENV CONFIG_FILE='' OUTPUT_FILE='' PORT=4001 TTL=255 INFINITY=16 PERIOD=30 SPLIT_HORIZON=1

RUN make
