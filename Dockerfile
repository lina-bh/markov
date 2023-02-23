FROM ubuntu:jammy

RUN adduser somebody

RUN apt-get update && apt-get -y full-upgrade && apt-get -y install --no-install-recommends gcc make libc6-dev gdb iwyu clang-13 clang-tidy-13 less

USER somebody
WORKDIR /src

#WORKDIR /build
#VOLUME /build
