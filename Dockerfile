# Use Ubuntu as the base image
FROM ubuntu:22.04

# Set the maintainer label
LABEL maintainer="jalden@telspandata.com"

# Set a working directory
WORKDIR /workspace



RUN mkdir -p /opt/mscc 
# RUN tar xzf mscc-brsdk-mipsel-2024.06.tar.gz -C /opt/mscc
# RUN tar xzf mscc-toolchain-bin-2024.02-105.tar.gz -C /opt/mscc
ADD mscc-brsdk-mipsel-2024.06.tar.gz /opt/mscc
ADD mscc-toolchain-bin-2024.02-105.tar.gz /opt/mscc
# RUN ls -la && tar xzf mscc-brsdk-mipsel-2024.06.tar.gz -C /opt/mscc
# RUN cd /workspace && ls -la && tar xzf mscc-brsdk-mipsel-2024.06.tar.gz -C /opt/mscc

# ADD mscc-toolchain-bin-2024.02-105.tar.gz /opt/mscc
# RUN cd /workspace && ls -la && tar xzf mscc-toolchain-bin-2024.02-105.tar.gz -C /opt/mscc
# COPY mscc-brsdk-mipsel-2024.06.tar.gz /workspace/
# COPY mscc-toolchain-bin-2024.02-105.tar.gz /workspace/

# Set environment variables
ENV LANG=C.UTF-8 \
    LC_ALL=C.UTF-8 \
    PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    DEBIAN_FRONTEND=noninteractive \
    TZ=Etc/UTC 

# RUN apt-get update 
# RUN DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get -y install apt-utils tzdata
# Update the system and install essential packages
RUN apt-get update && apt-get install -y \
    tzdata \
    build-essential \
    curl \
    cmake \
    wget \
    git \
    vim \
    python3 \
    python3-pip \
    python3-venv \
    bc \
    bzip2 \
    coreutils \
    cpio \
    findutils \
    gawk \
    grep \
    gzip \
    libc6-i386 \
    libcrypt-openssl-rsa-perl \
    libncurses5-dev \
    patch \
    perl \
    ranger \
    ruby \
    sed \
    squashfs-tools \
    tcl \
    tar \
    wget \
    libyaml-tiny-perl \
    libcgi-fast-perl \
    ruby-parslet\
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

# Optional: Install Node.js (for frontend development)
RUN curl -fsSL https://deb.nodesource.com/setup_18.x | bash - && \
    apt-get install -y nodejs

 
# Install Docker dependencies (if needed)
RUN apt-get install -y docker.io

# Optional: Install other tools
# RUN apt-get install -y tmux htop ranger


# Install additional Python dependencies
# COPY requirements.txt .
# RUN pip install --upgrade pip setuptools wheel && pip install -r requirements.txt || true




# Expose a port for development (optional)
# EXPOSE 8000

# Default command to run
CMD ["/bin/bash"]

