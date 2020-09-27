#!/bin/bash

if [ "$#" -lt 1 ]; then
  echo "Usage: ./install_voltdb.sh INSTALLATION_PATH"
  exit 1
fi

# Install the required packages
sudo apt-get -y install ant build-essential ant-optional default-jdk python \
                        cmake valgrind ntp ccache git python-httplib2 \
                        python-setuptools python-dev apt-show-versions \
                        openjdk-8-jdk

# Make sure that Java 8 is the default
echo "Select Java 8"
sudo update-alternatives --config java

# Clone and build VoltDB
cd $1
git clone https://github.com/voltdb/voltdb
cd voltdb
ant
PATH="$PATH:$(pwd)/bin/"
