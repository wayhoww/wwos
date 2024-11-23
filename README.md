# WWOS - WayhowW Operating System

#### Introduction
A toy-level single-thread, single-core and multi-process operating system targeting QEMU virt AArch64, written in C++.

#### Background
This project is developed for an assignment of Advanced Operating System at Beijing Jiaotong University.

#### Features
* runtime library made from scratch (thus standard library is not ported)
* compilation
* booting
* system call
* timer interrupt
* virtual memory
* process
* semaphore
* CFS-like scheduling
* in-memory ext2-like file system
* fifo (named pipe)
* file descriptor
* virtual tty (use multiple shell at the same time)
* shell
* several demos

#### TODO
* support for real hardware
* multi-core
