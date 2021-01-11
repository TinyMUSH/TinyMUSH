[![Build Status](https://travis-ci.com/TinyMUSH/TinyMUSH.svg?branch=master)](https://travis-ci.com/TinyMUSH/TinyMUSH)

- [TinyMUSH](#tinymush)
  - [Introduction](#introduction)
  - [Development](#development)
    - [TinyMUSH 3.0](#tinymush-30)
    - [TinyMUSH 3.1](#tinymush-31)
    - [TinyMUSH 3.2](#tinymush-32)
    - [TinyMUSH 3.3 (Alpha Stage)](#tinymush-33-alpha-stage)
- [Installation](#installation)
- [Full documentation](#full-documentation)

# TinyMUSH

## Introduction
                              
TinyMUSH 3 began as a project to fuse the TinyMUSH 2.2 and TinyMUX codebases, in an
effort to create a best-of-breed server that combined the best features and
enhancements of both, and then built additional innovations and increased stability on
top of that merged codebase.

## Development

TinyMUSH 3.0 is not a rewrite; we wanted to maintain close to 100% backwards
compatibility, and the sheer scope of effort that would have been necessary in order to
create a new server from scratch was beyond our available time resources.  However, every
line of the code has been gone through in the process of creating TinyMUSH 3.0.  A large
number of bugs have been fixed, portions have been rewritten to be cleaner and more
efficient, and a slew of new features have been added.  TinyMUSH 3.1 and beyond will
extend this further, providing a solidly stable server, improved efficiency, and many
feature enhancements.

### TinyMUSH 3.0

The initial TinyMUSH 3.0 release was jointly developed by David Passmore and Lydia Leong;
Robby Griffin joined the team with the release of 3.0 beta 18, and Scott Dorr joined with
the release of 3.0 beta 23, and the team expects to continue to maintain TinyMUSH 3 into
the forseeable future. Though we will continue to welcome contributions from the MUSH
community, we expect to remain the primary developers.  We believe that this will be a
major platform of choice for MUSH users in the future, and we highly encourage existing
TinyMUSH 2.2 and TinyMUX 1.x users to upgrade.

### TinyMUSH 3.1

TinyMUSH 3.1 was jointly developped by David Passmore, Lydia Leong, Robby Griffin and Scott Dorr.  Eddy Beaupre joined at TinyMUSh 3.1 patchlevel 1 and the move of the project
to Sourceforce.

### TinyMUSH 3.2

TinyMUSH 3.2 was mostly developped by Lydia Leong.

### TinyMUSH 3.3 (Alpha Stage)

TinyMUSH 3.3 is currently in the early stage of development. At this stage of
development, the goal is to ensure better compatibility with most current operating
systems and provide a stable base for many more years.

# Installation

Since TinyMUSH 3.3 is currently in alpha, there will be bugs and time where the server will probably not even compile. But feel free to test and provide feedback. Here's a quick rundown on how to build the server (Tested on Ubuntu 20.04.1 LTS and Mac OSX Big Sur).

**Ubuntu / Debian:**

```sh
# Install dependencies
#
$ apt install build-essential cproto libpcre3-dev libgdbm-dev autoconf libtool
#
# Clone the repository
#
$ git clone https://github.com/TinyMUSH/TinyMUSH.git
#
# Build the server
#
$ cd TinyMUSH
$ ./autogen.sh
$ ./configure
$ make
$ make install
#
# Start the server and test.
#
$ ./game/bin/netmush
$ telnet localhost 6250
```

**Mac OSX:**

```sh
# Install dependencies, you need xcode command-line and brew to start.
#
$ brew install autoconf automake libtool cproto gdbm pcre
#
# Clone the repository
#
$ git clone https://github.com/TinyMUSH/TinyMUSH.git
#
# Build the server
#
$ cd TinyMUSH
$ ./autogen.sh
$ ./configure
$ make
$ make install
#
# Start the server and test.
#
$ ./game/bin/netmush
$ telnet localhost 6250
```

**FreeBSD:**
```sh
# Install dependencies
#
su -m root -c 'pkg install git autoconf automake libtools cproto gdbm pcre'
#
# Clone the repository
#
$ git clone https://github.com/TinyMUSH/TinyMUSH.git
#
# Build the server
#
$ cd TinyMUSH
$ ./autogen.sh
$ ./configure
$ make
$ make install
#
# Start the server and test.
#
$ ./game/bin/netmush
$ telnet localhost 6250
```

# Full documentation

Please look at the following files for details on specific aspects of TinyMUSH 3.

|File             |Description                                                    |
|-----------------|---------------------------------------------------------------|
|README.md        |The mystery file nobody ever read...                           |
|INSTALL          |Installation instructions.                                     |
|CONVERTING       |Conversion instructions.                                       |
|NEWS             |Important news and events.                                     |
|FAQ              |Frequently ask questions.                                      |
|COPYING          |TinyMUSH and previous licences.                                |
|AUTHORS          |Credits and thanks.                                            |
|ChangeLog        |TinyMUSH 3 general changelog.                                  |
|ChangeLog.History|TinyMUX 1.0 and TinyMUSH 2.x changelog.                        |
|MODULES          |Developer information on creating modules.                     |

Note that theses have not been updated since TinyMUSH 3.2 and will only be when 3.3 enter
Beta.

You can also browse the doxygen source documentation (work in progress) at https://tinymush.github.io/TinyMUSH/html/
