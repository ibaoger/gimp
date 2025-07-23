#!/bin/sh

case $(readlink /proc/$$/exe) in
  *bash)
    set -o posix
    ;;
esac
set -e

if [ -z "$GITLAB_CI" ]; then
  # Make the script work locally
  if [ "$0" != 'build/linux/snap/2_build-gimp-snapcraft.sh' ] && [ $(basename "$PWD") != 'snap' ]; then
    printf '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, read: https://developer.gimp.org/core/setup/build/linux/\n'
    exit 1
  elif [ $(basename "$PWD") = 'snap' ]; then
    cd ../../..
  fi
fi


# Prepare env
## Option 1: rootless environment (recommended)
export SNAPCRAFT_BUILD_ENVIRONMENT=host
 
## Option 2: root/container enviroment by lxd
#sudo snap install lxd
#sudo lxd init --auto
#user=$(whoami)
#sudo usermod -a -G lxd $user
#sudo newgrp lxd


# Build babl, gegl and GIMP and create .snap bundle
# (Snapcraft does not support building parts like flatpak, so we need to build everything in one go)
cp build/linux/snap/snapcraft.yaml .

if [ -z "$GITLAB_CI" ]; then
  snapcraft --build-for=$(dpkg --print-architecture) --verbosity=verbose
else
  snapcraft remote-build --launchpad-accept-public-upload --build-for=$(dpkg --print-architecture) --verbosity=verbose
fi
