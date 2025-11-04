#!/bin/sh

qemu-system-x86_64 \
    -m 256M \
    -cpu kvm64,+smep,+smap \
    -kernel /task/kernel/bzImage \
    -initrd initramfs.cpio.gz \
    -snapshot \
    -nographic \
    -monitor none \
    -no-reboot \
    -hdb ./flag \
    -append "console=ttyS0 kaslr kpti=1 quiet panic=2"
