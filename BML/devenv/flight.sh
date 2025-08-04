#!/usr/bin/env bash
./clean.sh
./docker_build.sh && \
./launch.sh && \
./enter.sh
