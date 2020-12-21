# First build in a fatter builder image
FROM gradle:6.7-jdk11 AS build

# Install G++ build tools
RUN apt-get update && apt-get install -y software-properties-common \
    && add-apt-repository ppa:ubuntu-toolchain-r/ppa \
    && apt-get update && apt-get install -y g++-10
RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 50
RUN mkdir /src
WORKDIR /src

# We now go and build: first we copy over the gradle spec and download the relevant dependencies
COPY --chown=gradle:gradle *.gradle* gradle.* /src/
RUN gradle dependencies

# Then we copy the source code and build it, so dependencies are cached
COPY --chown=gradle:gradle pinot /src/pinot
COPY --chown=gradle:gradle app /src/app
# Build fat jar containing all binaries and libraries necessary
RUN gradle fatJar

# Then copy over the binaries to a slim runtime image
FROM openjdk:11-jre

RUN mkdir /pacito
COPY --from=build /src/app/build/libs/app.jar /pacito/app.jar
WORKDIR /pacito

VOLUME ["/source", "/wd"]
ENTRYPOINT ["java", "-jar", "/pacito/app.jar", "-s", "/source", "-wd", "/wd"]
CMD ["--help"]