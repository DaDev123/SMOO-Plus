FROM ubuntu:24.04

# Prevent interactive prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# Install base dependencies and add LLVM repository
RUN apt-get update && apt-get install -y \
    curl \
    wget \
    apt-transport-https \
    ca-certificates \
    gnupg \
    software-properties-common \
    python3 \
    python3-pip \
    git \
    build-essential \
    cmake \
    ninja-build \
    clang-format \
    lsb-release \
    && wget https://apt.llvm.org/llvm.sh \
    && chmod +x llvm.sh \
    && ./llvm.sh 19 \
    && apt-get install -y lld-19 \
    && ln -s /usr/bin/clang-19 /usr/bin/clang \
    && ln -s /usr/bin/clang++-19 /usr/bin/clang++ \
    && ln -s /usr/bin/lld-19 /usr/bin/lld \
    && pip3 install --break-system-packages keystone-engine pyelftools mmh3 lz4 \
    && rm llvm.sh \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install devkitPro
RUN ln -s /proc/self/mounts /etc/mtab \
    && mkdir /devkitpro/ \
    && echo "deb [signed-by=/devkitpro/pub.gpg] https://apt.devkitpro.org stable main" >/etc/apt/sources.list.d/devkitpro.list \
    && curl --fail -o /devkitpro/pub.gpg https://apt.devkitpro.org/devkitpro-pub.gpg \
    && apt-get update \
    && apt-get install -y devkitpro-pacman \
    && dkp-pacman --noconfirm -S switch-dev

# Set environment variables
ENV DEVKITPRO=/opt/devkitpro
ENV DEVKITARM=${DEVKITPRO}/devkitARM
ENV DEVKITPPC=${DEVKITPRO}/devkitPPC
ENV PATH=${DEVKITPRO}/tools/bin:$PATH

# Set working directory
WORKDIR /app/

# Default command
CMD ["/bin/bash"]
