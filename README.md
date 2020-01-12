mbpfan
====================

[![Build Status](https://travis-ci.org/dgraziotin/mbpfan.svg?branch=master)](https://travis-ci.org/dgraziotin/mbpfan)

This is an enhanced version of [Allan McRae mbpfan](http://allanmcrae.com/2010/05/simple-macbook-pro-fan-daemon/)

mbpfan is a daemon that uses input from coretemp module and sets the fan speed using the applesmc module.
This enhanced version assumes any number of processors and fans (max. 10).

* It only uses the temperatures from the processors as input.
* It requires coretemp and applesmc kernel modules to be loaded.
* It requires root use
* It daemonizes or stays in foreground
* Verbose mode for both syslog and stdout
* Users can configure it using the file /etc/mbpfan.conf

**Table Of Contents**

- [Supported GNU/Linux Distributions](#supported-gnulinux-distributions)
- [Tested Mac Models](#tested-mac-models)
- [Requirements](#requirements)
- [Installation](#installation)
	- [Arch Linux](#arch-linux)
	- [CRUX Linux](#crux-linux)
	- [Debian](#debian)
	- [Fedora](#fedora)
	- [Gentoo](#gentoo)
	- [Solus](#solus)
	- [Ubuntu](#ubuntu)
	- [Generic Install Instructions (All Other Operating Systems)](#generic-install-instructions-all-other-operating-systems)
- [Run Instructions](#run-instructions)
- [Starting at boot](#starting-at-boot)
- [Usage](#usage)
- [License](#license)
- [Credits](#credits)

## Supported GNU/Linux Distributions

We provide scripts to to load mbpfan daemon at system boot for many distros.
Please note that the support is provided by volunteers. mbpfan needs tests and bug reports.

Supported distributions:

- Ubuntu
- Debian
- Archlinux
- Fedora
- RedHat
- CentOS
- Gentoo
- Alpine
- Trisquel
- Solus

## Tested Mac Models

See https://github.com/linux-on-mac/mbpfan/wiki/Tested-Mac-Models .

## Requirements

Be sure to load the kernel modules **applesmc** and **coretemp**.

These modules are often automatically loaded when booting up GNU/Linux on a MacBook. If that is not the case, you should make sure to load them at system startup.

**How do I know if applesmc and coretemp are loaded?**

In most distributions, you can run the following command:

```bash
lsmod | grep -e applesmc -e coretemp
```

If you see `coretemp` and `applesmc` listed, you are all set.

**If you do not see `coretemp` and `applesmc` listed, you must load them.**

This is _usually_ achieved by inserting the following two lines in the file `/etc/modules`

```
coretemp
applesmc
```

Please check the relevant documentation of your GNU/Linux distribution.

## Installation

### Arch Linux

See [mbpfan-git at AUR](https://aur.archlinux.org/packages/mbpfan-git/).
Otherwise, please refer to the Generic Instructions.

### CRUX Linux

Follow the instructions on [jolupalabs REPO](https://github.com/jolupa/jolupalabs) for installation.

### Debian

On Debian 10 or later install via:

```
sudo apt-get install mbpfan
```

### Fedora

On Fedora 30 or later:

```
sudo dnf install mbpfan
```

### Gentoo

Install the ```mbpfan``` package with:

    sudo emerge -av app-laptop/mbpfan


### Solus

On Solus, install the package with:
```
sudo eopkg install mbpfan
```
then enable the service.


### Ubuntu

On Ubuntu 18.04 or later install via:

```
sudo apt-get install mbpfan
```

### Generic Install Instructions (All Other Operating Systems)

Compile with

    make

Install with

    sudo make install

It copies mbpfan to /usr/sbin, mbpfan.conf to /etc (and overwrites existing files),
README.md to /usr/share/doc/mbpfan, and mbpfan.8.gz to /usr/share/man/man8

If you would like to compile with Clang instead of GCC, simply set your system's
default compiler to be Clang. Tested with Clang 3.8 and 3.9. Tested with Clang
4.0 along with llvm-lld (The LLVM Linker).


Run The Tests (Optional)
------------------------
Users may run the tests after installing the program.  Please run the following command _from within the source directory_.

    sudo ./bin/mbpfan -t

or

    sudo make tests

Note that this only works on MacBook and not desktop computers due to different environment expectations.


## Run Instructions

If not installed, run with

    sudo bin/mbpfan

If installed, manually run with

    sudo mbpfan

If installed and using the init file, run with (Ubuntu example)

    sudo service mbpfan start


## Starting at boot

**Ubuntu**

For systemd based init systems (Ubuntu 16.04+), see the systemd section below.

For upstart based init systems (Ubuntu before 16.04), an example upstart job has been provided.
For using it, execute:

    sudo cp mbpfan.upstart /etc/init/mbpfan.conf
    sudo start mbpfan

**Debian**
An init file suitable for /lib/lsb/init-functions (Debian)
is located in the main folder of the source files, called mbpfan.init.debian
Rename it to mbpfan, give it execution permissions (chmod +x mbpfan)
and move it to /etc/init.d
Then, add it to the default runlevels with (as root):

    sudo update-rc.d mbpfan defaults

**Redhat, CentOS, Fedora**
An init file suitable for /etc/rc.d/init.d/functions
(RHEL/CentOS & Fedora) is also located at the same place, this file is called
mbpfan.init.redhat. Also rename it to mbpfan, give it execution permissions
and move it to /etc/init.d
To add the script to the default runlevels, run the following as root:

    chkconfig --level 2345 mbpfan on && chkconfig --level 016 mbpfan off

**Gentoo**

To automatically run mbpfan at boot, run as root:

    rc-update add mbpfan default


**systemd**
As a special bonus, a service file for systemd is also included. To use it,
execute the following (as root):

    sudo cp mbpfan.service /etc/systemd/system/
    sudo systemctl enable mbpfan.service
    sudo systemctl daemon-reload
    sudo systemctl start mbpfan.service

## Usage

    Usage: ./mbpfan OPTION(S)

    -h Show the help screen
    -f Run in foreground
    -t Run the tests
    -v Be (a lot) verbose


## License

GNU General Public License version 3

## Credits

**This Project Is Based On:**

* http://allanmcrae.com/2010/05/simple-macbook-pro-fan-daemon/
* http://allanmcrae.com/2011/08/mbp-fan-daemon-update/
* https://launchpad.net/macfanctld
* https://www.lobotomo.com/products/FanControl/

**This Project uses following library:**

* [ANSI C Application Settings Management](http://pokristensson.com/settings.html) by Per Ola Kristensson.
