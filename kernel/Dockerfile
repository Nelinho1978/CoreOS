FROM ubuntu:22.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    gcc-multilib \
    nasm \
    qemu-system-x86 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /coreos/kernel
COPY . .

CMD ["make", "run"]
