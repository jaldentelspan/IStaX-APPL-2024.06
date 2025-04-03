#   Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.
#
#   Unpublished rights reserved under the copyright laws of the United States of
#   America, other countries and international treaties. Permission to use, copy,
#   store and modify, the software and its source code is granted but only in
#   connection with products utilizing the Microsemi switch and PHY products.
#   Permission is also granted for you to integrate into other products, disclose,
#   transmit and distribute the software only in an absolute machine readable
#   format (e.g. HEX file) and only in or with products utilizing the Microsemi
#   switch and PHY products.  The source code of the software may not be
#   disclosed, transmitted or distributed without the prior written permission of
#   Microsemi.
#
#   This copyright notice must appear in any copy, modification, disclosure,
#   transmission or distribution of the software.  Microsemi retains all
#   ownership, copyright, trade secret and proprietary rights in the software and
#   its source code, including all modifications thereto.
#
#   THIS SOFTWARE HAS BEEN PROVIDED "AS IS". MICROSEMI HEREBY DISCLAIMS ALL
#   WARRANTIES OF ANY KIND WITH RESPECT TO THE SOFTWARE, WHETHER SUCH WARRANTIES
#   ARE EXPRESS, IMPLIED, STATUTORY OR OTHERWISE INCLUDING, WITHOUT LIMITATION,
#   WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR USE OR PURPOSE AND
#   NON-INFRINGEMENT.
FROM ubuntu:16.04

USER root
WORKDIR /root

RUN apt-get update && apt-get install git autoconf automake libtool make gawk libreadline-dev \
   texinfo dejagnu pkg-config libpam0g-dev libjson-c-dev bison flex \
   python-pytest libc-ares-dev python3-dev libsystemd-dev python-ipaddr \
   python3-sphinx install-info wget iputils-arping iputils-ping iputils-tracepath tftp net-tools -y \
   && rm -rf /var/lib/apt/lists/*

RUN groupadd -g 92 frr \
    && groupadd -r -g 85 frrvty \
    && adduser --system --ingroup frr --home /var/run/frr/ \
   --gecos "FRR suite" --shell /sbin/nologin frr \
    && usermod -a -G frrvty frr

RUN wget https://github.com/FRRouting/frr/archive/frr-4.0.tar.gz
RUN tar zxvf frr-4.0.tar.gz
RUN cd frr-frr-4.0 \
&& ./bootstrap.sh \
&& ./configure \
    --prefix=/usr \
    --enable-exampledir=/usr/share/doc/frr/examples/ \
    --localstatedir=/var/run/frr \
    --sbindir=/usr/lib/frr \
    --sysconfdir=/etc/frr \
    --enable-zebra \
    --disable-bgpd \
    --enable-ripd \
    --disable-ripngd \
    --enable-ospfd \
    --disable-ospf6d \
    --disable-nhrpd \
    --disable-isisd \
    --disable-pimd \
    --disable-ldpd \
    --disable-bgp-announce \
    --disable-ospfapi \
    --disable-ospfclient \
    --enable-watchfrr \
    --enable-user=frr \
    --enable-group=frr \
    --enable-vty-group=frrvty \
    --enable-configfile-mask=0640 \
    --enable-logfile-mask=0640 \
    --with-pkg-git-version \
    --with-pkg-extra-version=-MyOwnFRRVersion \
&& make -j12 \
&& make install \
&& install -m 755 -o frr -g frr -d /var/log/frr \
&& install -m 775 -o frr -g frrvty -d /etc/frr \
&& install -m 640 -o frr -g frr /dev/null /etc/frr/zebra.conf \
&& install -m 640 -o frr -g frr /dev/null /etc/frr/bgpd.conf \
&& install -m 640 -o frr -g frr /dev/null /etc/frr/ospfd.conf \
&& install -m 640 -o frr -g frr /dev/null /etc/frr/ospf6d.conf \
&& install -m 640 -o frr -g frr /dev/null /etc/frr/isisd.conf \
&& install -m 640 -o frr -g frr /dev/null /etc/frr/ripd.conf \
&& install -m 640 -o frr -g frr /dev/null /etc/frr/ripngd.conf \
&& install -m 640 -o frr -g frr /dev/null /etc/frr/pimd.conf \
&& install -m 640 -o frr -g frr /dev/null /etc/frr/ldpd.conf \
&& install -m 640 -o frr -g frr /dev/null /etc/frr/nhrpd.conf  \   
&& install -m 640 -o frr -g frrvty /dev/null /etc/frr/vtysh.conf \
&& install -m 644 tools/frr.service /etc/systemd/system/frr.service \
&& install -m 644 tools/etc/default/frr /etc/default/frr \
&& install -m 644 tools/etc/frr/daemons /etc/frr/daemons \
&& install -m 644 tools/etc/frr/daemons.conf /etc/frr/daemons.conf \
&& install -m 644 tools/etc/frr/frr.conf /etc/frr/frr.conf \
&& install -m 644 -o frr -g frr tools/etc/frr/vtysh.conf /etc/frr/vtysh.conf 

RUN echo "" > /etc/frr/daemons \
&& echo "zebra=yes" >> /etc/frr/daemons \
&& echo "bgpd=no" >> /etc/frr/daemons \
&& echo "ospfd=yes" >> /etc/frr/daemons \
&& echo "ospf6d=no" >> /etc/frr/daemons \
&& echo "ripd=yes" >> /etc/frr/daemons \
&& echo "ripngd=no" >> /etc/frr/daemons \
&& echo "isisd=no" >> /etc/frr/daemons \
&& echo "pimd=no" >> /etc/frr/daemons \
&& echo "ldpd=no" >> /etc/frr/daemons \
&& echo "nhrpd=no" >> /etc/frr/daemons \
&& echo "eigrpd=no" >> /etc/frr/daemons \
&& echo "babeld=no" >> /etc/frr/daemons \
&& echo "sharpd=no" >> /etc/frr/daemons 

copy run.sh /usr/sbin/

RUN chmod +x /usr/sbin/run.sh

VOLUME ["/etc/frr"]
CMD ["/usr/sbin/run.sh"]


