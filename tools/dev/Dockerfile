# SPDX-License-Identifier: GPL-3.0-only
# A Linux to build and test SLOOP 3 in (tools/dev/dev.sh): gcc, Python with NumPy and Pillow, Node.js, and the pi32v2
# LLVM toolchain the firmware is built with (its release checked against its SHA256SUMS), for x86-64 or arm64 hosts.
FROM debian:trixie
ARG TARGETARCH
RUN apt-get update && apt-get install -y --no-install-recommends gcc g++ make python3 python3-numpy python3-pil nodejs \
    xz-utils curl ca-certificates git file bsdextrautils fonts-inter && rm -rf /var/lib/apt/lists/*
ARG TC=pi32v2-23.1.5-2
RUN A=$([ "$TARGETARCH" = arm64 ] && echo linux-arm64 || echo linux-x64) && mkdir -p /opt/pi32v2 && cd /opt/pi32v2 \
 && curl -fsSL -o tc.tar.xz https://github.com/jvitkauskas/pi32v2-llvm/releases/download/$TC/pi32v2-llvm-${TC#pi32v2-}-$A.tar.xz \
 && curl -fsSL https://github.com/jvitkauskas/pi32v2-llvm/releases/download/$TC/SHA256SUMS | grep -- "-$A.tar.xz" | sed 's/pi32v2-llvm-.*/tc.tar.xz/' | sha256sum -c \
 && tar -xJf tc.tar.xz --strip-components=1 && rm tc.tar.xz
ENV PI32V2_LLVM=/opt/pi32v2/bin
WORKDIR /w
