/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

 Unpublished rights reserved under the copyright laws of the United States of
 America, other countries and international treaties. Permission to use, copy,
 store and modify, the software and its source code is granted but only in
 connection with products utilizing the Microsemi switch and PHY products.
 Permission is also granted for you to integrate into other products, disclose,
 transmit and distribute the software only in an absolute machine readable
 format (e.g. HEX file) and only in or with products utilizing the Microsemi
 switch and PHY products.  The source code of the software may not be
 disclosed, transmitted or distributed without the prior written permission of
 Microsemi.

 This copyright notice must appear in any copy, modification, disclosure,
 transmission or distribution of the software.  Microsemi retains all
 ownership, copyright, trade secret and proprietary rights in the software and
 its source code, including all modifications thereto.

 THIS SOFTWARE HAS BEEN PROVIDED "AS IS". MICROSEMI HEREBY DISCLAIMS ALL
 WARRANTIES OF ANY KIND WITH RESPECT TO THE SOFTWARE, WHETHER SUCH WARRANTIES
 ARE EXPRESS, IMPLIED, STATUTORY OR OTHERWISE INCLUDING, WITHOUT LIMITATION,
 WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR USE OR PURPOSE AND
 NON-INFRINGEMENT.

*/

Introduction
============

The aim of this C++ project is to provide common and basic functionality needed
by the VTSS software products. A key requirement to everything which goes into
vtss_basics is that it must work both as standalone and under Linux (it could
probably also work under Windows).

All code included in this library is written by VTSS software developers and is
copyright protected by Microsemi (no third-party code). It is, however, the
intention that it should be released under a permissive open source license such
that customers may use it without going through any NDA process.


Standalone *and* Linux? What does this imply?
---------------------------------------

Writing C++ code that works both as standalone and under Linux requires two
things:

- Not all compilers support exceptions (when multiple threads are used), which
  means that exceptions must not be used in this project.

- When OS primitives such as mutex, semaphores, thread-local-storage are
  required, these must be encapsulated in a version for Linux and a version for
  standalone compilation.

Neither of this is a big problem. It is certainly a requirement not seen on many
C++ projects. The lack of exceptions also means that the C++ standard library
cannot be used.

Some useful defines
-------------------

VTSS_BASICS_OPERATING_SYSTEM_LINUX
Is defined for a project compiled for a Linux based system (may either be an embedded
system or a Linux host application

VTSS_BASICS_STANDALONE
Is defined for a project using vtss_basics without vtss_api and vtss_appl

Where are the documentation
---------------------------

There is still only very little documentation available for vtss_basics, but
hopefully this will change. Many of the things in vtss_basics are closely
related to C++ concepts and facilities, and for people which are not used to C++
it may seem a bit odd. This is why I don't think that vtss_basics should be
documented through doxygen, as it is often not the individual functions that
need be documented, but rather the concepts they rely on.

So, instead of starting to provide doxygen documentation for vtss_basics, I will
add a "doc" folder and start adding files that document the different parts of
vtss_basics.

In the beginning I will be focusing on topics that are easy and can be used
directly in the vtss_appl code base.

If you have comments on the documentation or the code, just write me an e-mail.
Also if there are feature requests or specific features you would like to get
documented next time I have an hour or two, then give me a hint. So, as always,
all feedback is most welcome. My e-mail address is: anielsen@vitesse.com.


How to build it
---------------

vtss_basics can either be used as a standalone project or it can be used from
WebStax where it is already included. So if you want to build it as part of
WebStax, just build tip-of-trunk as you normally do.

If you want to build it as a standalone project you will need a C++11 compiler
and cmake.

Following are the copy-paste-instructions:

    git clone ssh://soft02/proj/sw/git/vtss_basics
    mkdir vtss_basics_build
    cd vtss_basics_build
    cmake ../vtss_basics
    make

The standalone build includes tests and examples. The examples are a good place
to experiment and try out new stuff.
