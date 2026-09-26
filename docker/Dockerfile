## Dockerfile
## TinyMUSH docker file based on debian
## https://github.com/TinyMUSH/TinyMUSH/blob/master/README.md

# Build stage - contains all build tools and dependencies

# set the base image
# add the AS builder to give us a name
FROM debian:trixie-slim AS builder

# set UID and GID to the user ID and group ID it will run as
ARG UID=1000
ARG GID=1000

# update packages
RUN apt-get update
# install libraries needed to build and run
RUN apt-get --yes install build-essential cmake libcrypt-dev liblmdb-dev libgdbm-dev libpcre2-dev
# install less and nano for convenience, git for source pull
RUN apt-get --yes install less nano git

# create an app group and user, changed to groupadd and useradd for debian, 2026-04-24
RUN groupadd --gid $GID app
RUN useradd --uid $UID --gid $GID app

# create the working folder and give the app user ownership before switching to it
#RUN mkdir -p /app && chown app:app /app
# become the user
USER app
# go to the app working folder
WORKDIR /app

# Clone repository
RUN git clone https://github.com/TinyMUSH/TinyMUSH.git

# copy it back one step
RUN cp -r /app/TinyMUSH/. /app/
# and remove the unneeded source copy.
RUN rm -r /app/TinyMUSH/

# build
RUN mkdir -p /app/build
WORKDIR /app/build
RUN cmake ..
RUN cmake --build . --target install-upgrade

# go back to the /app folder
WORKDIR /app
# become the app user
USER app

# duplicate /app/game/docs and configs to /app/game/docs-original and configs-original
# the entrypoint script referenced below will copy out from the original to the volumes if the volumes are empty
RUN cp -r /app/game/docs /app/game/docs-original
RUN cp -r /app/game/configs /app/game/configs-original

# erase the configs and docs files from the source folders
RUN rm /app/game/configs/*
RUN rm /app/game/docs/*

# become root, change ownership of the /app directory to the user
USER root
RUN chown -R app:app .

##
## Production stage - only runtime files
##
FROM debian:trixie-slim

# set UID and GID to the user ID and group ID it will run as
ARG UID=1000
ARG GID=1000

# create an app group and user, changed to groupadd and useradd for debian, 2026-04-24
RUN groupadd --gid $GID app
RUN useradd --uid $UID --gid $GID app

# update packages
RUN apt-get update
# install the runtime shared libraries netmush needs (pcre2 + lmdb) and xz for backups
RUN apt-get --yes install libpcre2-8-0 liblmdb0 xz-utils
# install less and nano for convenience
RUN apt-get --yes install less nano

# become the app user
USER app
# go to the app folder
WORKDIR /app

# Copy in the built artifacts, not the build tools or source
COPY --from=builder /app/. .

# set the entrypoint to copy the docs and configs files when the destination folder volumes are empty
COPY entrypoint.sh /entrypoint.sh
# while root, make it executable.
USER root
RUN chmod +x /entrypoint.sh
# switch back to the user, and set the entrypoint.
USER app
ENTRYPOINT ["/entrypoint.sh"]

# Finis
