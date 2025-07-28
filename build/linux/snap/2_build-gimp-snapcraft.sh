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
## Option 1: no container environment (recommended)
export SNAPCRAFT_BUILD_ENVIRONMENT=host
 
## Option 2: containerized environment by lxd
#sudo snap install lxd
#sudo lxd init --auto
#user=$(whoami)
#sudo usermod -a -G lxd $user
#sudo newgrp lxd


# Build babl, gegl and GIMP
## (snapcraft does not allow to freely set the .yaml path, so let's just temporarely copy it)
cp build/linux/snap/snapcraft.yaml .
## (snapcraft does not allow building in parts like flatpak-builder, so we need to build everything in one go)
if [ -z "$GITLAB_CI" ]; then
  sudo snapcraft --build-for=$(dpkg --print-architecture) --verbosity=verbose
else
  snapcraft remote-build --launchpad-accept-public-upload --build-for=$(dpkg --print-architecture) --verbosity=verbose
fi
rm snapcraft.yaml

# Rename .snap file to avoid confusion (the distribution of the .snap is done only in dist-snap-weekly)
mv *.snap temp-$(echo *.snap)
