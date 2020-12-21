FROM gradle:6.7-jdk11 AS build

# Install G++ build tools
RUN apt-get update && apt-get install -y software-properties-common \
    && add-apt-repository ppa:ubuntu-toolchain-r/ppa \
    && apt-get update && apt-get install -y g++-10
RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 50

COPY --chown=gradle:gradle . /home/gradle/src
WORKDIR /home/gradle/src
RUN gradle assembleDist --no-daemon


FROM openjdk:11-jre
RUN mkdir /pacito

COPY --from=build /home/gradle/src/pinot/build/libs/*.so /pacito
COPY --from=build /home/gradle/src/app/build/distributions/app.zip /pacito/app.zip
RUN cd /pacito && unzip -o app.zip