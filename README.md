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

#### Note
I switched programming twice while developing the project.
* Initially I chose Swift(https://github.com/wayhoww/SwiftOS), aiming at verifying the feasibility of EmbeddedSwift in kernel development.
* Later I found I had spent too much time on dealing with language-specific issues. I switched to Rust(the first few commits of this repo) due to the limit of my bandwidth.
* Rust fits kernel development well. It's core(no-std) library provided useful features like stirng formatting and common data structures that can be used in bare-metal environment, which is very convenient.
* However I switched to C++ because I found I have to spend a lot of time on satisfying the compiler. Fairly speaking the borrow checker would help me, but I do not have enough time to carefully organize my code to satisfy it. Thus I switched to C++ in the end.
