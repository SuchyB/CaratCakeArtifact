#!/usr/bin/env bash
sudo qemu-system-x86_64 -enable-kvm -display curses -smp 2 -m 8192 -vga std -serial file:serial1.out -serial file:serial2.out -debugcon file:debug.out -cdrom nautilus.iso -cpu host,host-phys-bits -s
