FROM debian:11

WORKDIR /opt

# Install Dependencies
RUN apt update
RUN apt install --yes \
    build-essential \
    gdb \
    git \
    tar \
    cmake \
    curl \
    zip \
    unzip \
    pkg-config \
    ninja-build \
    librocksdb-dev \
    libgoogle-glog-dev \
    default-jre \
    default-jdk \
    maven \
    valgrind

# Install UCX
ARG UCX_VERSION=1.12.0
RUN apt install libtool wget -y
RUN wget https://github.com/openucx/ucx/archive/refs/tags/v$UCX_VERSION.tar.gz
RUN tar xzf v$UCX_VERSION.tar.gz
WORKDIR /opt/ucx-$UCX_VERSION
RUN ./autogen.sh
RUN ./contrib/configure-release --prefix=$PWD/install-release
RUN make -j8
RUN make install

# Export UCX Path
WORKDIR /opt/ucx-$UCX_VERSION/install-release
ENV UCX_PATH=/opt/ucx-$UCX_VERSION/install-release
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/ucx-$UCX_VERSION/install-release/lib

# Surpress UCX Warning
ENV UCX_WARN_UNUSED_ENV_VARS=n

# Install SDKMan
ENV SDKMAN_DIR=/root/.sdkman
RUN curl -s "https://get.sdkman.io" | bash \
 && echo "sdkman_auto_answer=true" > $SDKMAN_DIR/etc/config \
 && echo "sdkman_auto_selfupdate=false" >> $SDKMAN_DIR/etc/config

RUN bash -c "source $SDKMAN_DIR/bin/sdkman-init.sh && sdk install gradle 7.4"
