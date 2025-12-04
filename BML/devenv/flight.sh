#!/usr/bin/env bash
./clean.sh
./docker_build.sh 2>&1 | tee ./docker_build.log && \
./launch.sh && \
./enter.sh
