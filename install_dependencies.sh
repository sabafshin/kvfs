#!/bin/bash
if [ -f /etc/debian_version ]; then
    # Includes Ubuntu, Debian
    sudo apt install -y build-essential
    sudo apt install -y g++-8
    sudo apt install -y gcc-8
    sudo apt install -y cmake
    sudo apt install -y libsnappy-dev