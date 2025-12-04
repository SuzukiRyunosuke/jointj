#!/usr/bin/env bash
./clean.sh
./docker_build.sh | tee ./docker_build.log && \
./launch.sh && \
./enter.sh
