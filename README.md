
![Nautilus Logo](https://lh5.googleusercontent.com/8BkFSH-06MvfV9hqSk3D5VJQWPabgfMrlkZOcd6unP2AWYZi9ZOc5sgFtXMhyAHRPHJoMtv87jxwE9214Hx2YqmcFppPnYgpTvyau1wwwhHUee5YEn5Sl0to4LNFMg9D-Q=w1280 "Nautilus Logo")
[![Build Status](https://travis-ci.com/HExSA-Lab/nautilus.svg?branch=master)](https://travis-ci.com/HExSA-Lab/nautilus)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/17390/badge.svg)](https://scan.coverity.com/projects/hexsa-lab-nautilus)
[![CodeFactor](https://www.codefactor.io/repository/github/hexsa-lab/nautilus/badge)](https://www.codefactor.io/repository/github/hexsa-lab/nautilus)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/HExSA-Lab/nautilus.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/HExSA-Lab/nautilus/alerts/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

# Nautilus w/ CARAT CAKE
Nautilus is an example of an Aerokernel, a very thin kernel-layer exposed
(much like Unikernel) directly to a runtime system and/or application.
An Aerokernel does not, by default, have a user-mode! There are several reasons for this,
simplicity and performance among the most important. Furthermore, there are no heavy-weight
processes---only threads, all of which share an address space. Therefore, Nautilus is also an
example of a single address-space OS (SASOS). The *runtime* can implement user-mode features
or address space isolation if this is required for its execution model.

This version of Nautilus has been modified to accommodate running with a CARAT address space abstraction (CARAT CAKE).
CARAT CAKE is an extension of work found in [PLDI '20](https://users.cs.northwestern.edu/~simonec/files/Research/papers/MODERN_PLDI_2020.pdf) with the paper describing this work appearing in [ASPLOS '22](https://asplos-conference.org/).

The concept of CARAT CAKE is to replace paging with a system that can operate using only physical addresses. Doing this enables the underlying system to have significant energy savings as well as allow new performance minded optimizations in both the micro-architecture and in software.


## Table of Contents

- [Background](#background)
- [Prerequisites](#prerequisites)
- [Hardware Support](#hardware-support)
- [Building](#building)
- [Using QEMU](#using-qemu)
- [Using BOCHS](#using-bochs)
- [Resources](#resources)
- [Maintainers](#maintainers)
- [License](#license)
- [Acknowledgements](#acknowledgements)


## Background
We call the combination of an Aerokernel and the runtime/application using it
a Hybrid Runtime (HRT), in that it is both a runtime *and* a kernel, especially regarding its
ability to use the full machine and determine the proper abstractions to the raw hardware
(if the runtime developer sees a mismatch with his/her needs and the Aerokernel mechanisms,
they can be overridden).

If stronger isolation or more complete POSIX/Linux compatibility is required, it is useful
to run the HRT in the context of a Hybrid Virtual Machine. An HVM allows a virtual machine
to split the (virtual) hardware resources among a regular OS (ROS) and an HRT. The HRT portion of the
HVM can then be seen as a kind of software accelerator. Note that because of the simplicity
of the hardware abstractions in a typical HRT, virtualization overheads are much, much less
significant than in, e.g. a Linux guest.

## Prerequisites

- `clang/llvm 9.0+` (https://releases.llvm.org/download.html)
- `grub` version >= ~2.02
- `xorriso` (for creating ISO images)
- `qemu` or `bochs` (for testing and debugging)
- `NOELLE` (https://github.com/scampanoni/noelle)
- `wllvm or gclang` (https://github.com/travitch/whole-program-llvm) (https://github.com/SRI-CSL/gllvm)

## Hardware Support

Nautilus w/ CARAT CAKE works with the following hardware:

- x86_64 machines (AMD and Intel)
- Intel Xeon Phi, both KNC and KNL using [Philix](http://philix.halek.co) for easy booting
- As a Hybrid Virtual Machine (HVM) in the [Palacios VMM](http://v3vee.org/palacios)

Nautilus can also run as a virtual machine under QEMU, BOCHS, KVM, and in a simulated
environment using [Gem5](http://gem5.org/Main_Page)

## Building

First, configure Nautilus by running either
`make menuconfig` or `make defconfig`. The latter
generates a default configuration for you. The former
allows you to customize your kernel build.

For running CARAT CAKE, a pre-made configuration file has been created and is located in ./configs/karat.config
The user can simply run the following command to be set up for a CARATized version of Nautilus from the root directory:
`$> cp $./configs/karat.config ./.config`


Before running the next command, ensure that you have downloaded/installed/enabled the prerequisites (mainly clang, wllvm, and NOELLE).
To compile a CARATized Nautilus, from the root directory run:

`$> ./kernel_build_with_llvm_and_noelle.sh`


## Additional Artifact Availability Information for CARAT CAKE
For artifact availability purposes, we have also included the benchmark suites, scripts, and
and an empty file system image -- all of which were used to build and test CARAT CAKE.

The benchmark suites are:
- NAS, located in `./src/test/NAS`
- Parsec, located in `./src/test/parsec-3.0`

Please see the README files in both directories that log the minor changes
required to the benchmarks so they are compatible with the CARAT CAKE/Nautilus
compilation pipeline.

Scripts used to build the benchmarks (with and without instrumentation from the CARAT 
CAKE compiler) are located in `./parsec_and_nas_build_scripts`.

Finally, an additional "run" script is included that runs Nautilus under QEMU with
KVM enabled, serial output enabled, and debug output enabled. This script is in 
`run_nautilus_qemu_kvm.sh`.


## Using QEMU

Here's an example:

[![asciicast](https://asciinema.org/a/xT76jwP0Qe9H7w3nKAk7gkTmv.png)](https://asciinema.org/a/xT76jwP0Qe9H7w3nKAk7gkTmv)

Recommended:

```Shell
$> qemu-system-x86_64 -cdrom nautilus.iso -m 2048
```

Nautilus has multicore support, so this will also work just fine:

```Shell
$> qemu-system-x86_64 -cdrom nautilus.iso -m 2048 -smp 4
```

You should see Nautilus boot up on all 4 cores.

Nautilus is a NUMA-aware Aerokernel. To see this in action, try (with a sufficiently new
version of QEMU):

```Shell
$> qemu-system-x86_64 -cdrom nautilus.iso \
                      -m 8G \
                      -numa node,nodeid=0,cpus=0-1 \
                      -numa node,nodeid=1,cpus=2-3 \
                      -smp 4,sockets=2,cores=2,threads=1
```

Nautilus supports debugging over the serial port. This is useful if you want to
debug a physical machine remotely. All prints after the serial port has been
initialized will be redirected to COM1. To use this, find the SERIAL_REDIRECT
entry and enable it in `make menuconfig`. You can now run like this:

```Shell
$> qemu-system-x86_64 -cdrom nautilus.iso -m 2G -serial stdio
```

Sometimes it is useful to interact with the Nautilus root shell via serial port,
e.g. when you're running under QEMU on a system that does not have a windowing
system. You'll want to first put a character device on the serial port by
rebuilding Nautilus after selecting the *Place a virtual console interface on a character device* option.
Then, after Nautilus boots (making sure you enabled the `-serial stdio` option
in QEMU) you'll see a virtual console at your terminal. You can get to the root
shell by getting to the terminal list with `\``3`. You can then select the root
shell, and you will be able to run shell commands and see output. If you want to
see more kernel output, you can use serial redirection and serial mirroring in
your config.

If you'd like to use Nautilus networking with QEMU, you should use a TUN/TAP
interface. First, you can run the following on your host machine:

```Shell
$> sudo tunctl -d tap0
$> sudo tunctl -t tap0
$> sudo ifconfig tap0 up 10.10.10.2 netmask 255.255.255.0
```

Then you can use the tap interface with QEMU as follows. This particular
invocation attaches both a virtual e1000 fast ethernet card and a virtio
network interface:

```Shell
$> sudo qemu-system-x86_64 -smp 2 \
                           -m 2048 \
                           -vga std \
                           -serial stdio \
                           -cdrom nautilus.iso \
                           -netdev tap,ifname=tap0,script=no,id=net0 \
                               -device virtio-net,netdev=net0 \
                           -netdev user,id=net1 \
                               -device e1000,netdev=net1 \
                           -drive if=none,id=hd0,format=raw,file=nautilus.iso \
                               -device virtio-blk,drive=hd0
```


## Using BOCHS

While we recommend using QEMU, sometimes it is nice to use the native debugging
support in [BOCHS](http://bochs.sourceforge.net/). We've used BOCHS successfully with version 2.6.8. You must have
a version of BOCHS that is built with x86_64 support, which does not seem to be the
default in a lot of package repos. We had to build it manually. You probably also
want to enable the native debugger.

Here is a BOCHS config file (`~/.bochsrc`) that we used successfully:

```
ata0-master: type=cdrom, path=nautilus.iso, status=inserted
boot: cdrom
com1: enabled=1, mode=file, dev=serial.out
cpu: count=2
cpuid: level=6, mmx=1, level=6, x86_64=1, 1g_pages=1
megs: 2048
```

## Resources

You can find publications related to Nautilus and HRTs/HVMs at
http://halek.co, http://pdinda.org, http://interweaving.org,
and the lab websites below.

Our labs:

<img src="http://cs.iit.edu/~khale/images/hexsa-logo.png" height=100/>

[HExSA Lab](http://hexsa.halek.co) at [IIT](https://www.iit.edu)

<img src="http://cs.iit.edu/~khale/nautilus/img/prescience.png" height=100/>

[Prescience Lab](http://www.presciencelab.org) at [Northwestern](https://www.northwestern.edu)

## Maintainers

### CARAT CAKE
Primary development done by:
[Brian Suchy](http://briansuchy.com), [Souradip Ghosh](https://souradipghosh.com/), [Drew Kersnar](https://www.linkedin.com/in/dakersnar/), [Siyuan Chai](https://schai.me/), [Aaron Nelson](https://www.linkedin.com/in/a-r-n/), [Zhen Huang](https://www.linkedin.com/in/zhen-huang-9706/), [Michael Cuevas](https://mcuevas.org/), [Alex Bernat](https://github.com/alexbernat), [Gaurav Chaudhary](https://www.linkedin.com/in/gauravchaudhary1993/), [Nikos Hardavellas](https://users.cs.northwestern.edu/~hardav/), [Simone Campanoni](https://users.cs.northwestern.edu/~simonec/#gsc.tab=0), and [Peter Dinda](http://pdinda.org/)

### Nautilus
Primary development is done by [Kyle Hale](http://halek.co) and [Peter
Dinda](http://pdinda.org).

However, many people contribute to the development
and maintenance of Nautilus. Please see [this
page](http://cs.iit.edu/~khale/nautilus/) as well as comments in the headers
and the commit logs for details.

## License
[![MIT License](http://seawisphunter.com/minibuffer/api/MIT-License-transparent.png)](https://github.com/HExSA-Lab/nautilus/blob/master/LICENSE.txt)

## Acknowledgements

<img align="left" src="https://www.nsf.gov/images/logos/NSF_4-Color_bitmap_Logo.png" height=100/>
<img align="left" src="https://ucrtoday.ucr.edu/wp-content/uploads/2018/06/DOE-logo.png" height=100/>
<img src="https://upload.wikimedia.org/wikipedia/commons/3/32/Sandia_National_Laboratories_logo.svg" height=100/>


Nautilus was made possible by support from the United States [National Science
Foundation](https://nsf.gov) (NSF) via grants [CCF-1533560](https://www.nsf.gov/awardsearch/showAward?AWD_ID=1533560), [CRI-1730689](https://nsf.gov/awardsearch/showAward?AWD_ID=1730689&HistoricalAwards=false), [REU-1757964](https://www.nsf.gov/awardsearch/showAward?AWD_ID=1757964), [CNS-1718252](https://www.nsf.gov/awardsearch/showAward?AWD_ID=1718252&HistoricalAwards=false),
CNS-0709168, [CNS-1763743](https://www.nsf.gov/awardsearch/showAward?AWD_ID=0709168), and [CNS-1763612](https://www.nsf.gov/awardsearch/showAward?AWD_ID=1763612&HistoricalAwards=false), the [Department of Energy](https://www.energy.gov/) (DOE) via
grant DE-SC0005343, and [Sandia National Laboratories](https://www.sandia.gov/) through the [Hobbes
Project](https://xstack.sandia.gov/hobbes/), which was funded by the [2013 Exascale Operating and Runtime Systems
Program](https://science.energy.gov/~/media/grants/pdf/lab-announcements/2013/LAB_13-02.pdf) under the [Office of Advanced Scientific Computing Research](https://science.energy.gov/ascr) in the [DOE
Office of Science](https://science.energy.gov/). Sandia National Laboratories is a multi-program laboratory
managed and operated by Sandia Corporation, a wholly owned subsidiary of
Lockheed Martin Corporation, for the U.S. Department of Energy's [National
Nuclear Security Administration](https://www.energy.gov/nnsa/national-nuclear-security-administration) under contract [DE-AC04-94AL85000](https://govtribe.com/award/federal-contract-award/definitive-contract-deac0494al85000).

[Kyle C. Hale](http://halek.co) © 2018
