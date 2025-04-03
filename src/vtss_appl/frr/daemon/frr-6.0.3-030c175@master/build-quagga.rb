#!/usr/bin/env ruby

require 'pp'
require 'open3'
require 'logger'
require 'optparse'
require 'fileutils'
require_relative 'resultnode.rb'

################################################################################
# Globals
################################################################################
$options = {
    :component => [],  # Build all components
    :arch      => [],  # Build for all architectures
}

if ENV['MSCC_SDK_PATH']
    # This variable is defined when building FRR inline with the application. In
    # that case, we use the toolchain pointed to by the SDK, which is given by
    # the MSCC_SDK_PATH environment variable.
    $building_inline = true
else
    $building_inline = false
end

puts("Building FRR #{$building_inline ? 'inline with the application' : 'as a standalone component'}")

# If building inline with the application, only :prefix of the following hash of
# hashes is used.
$arch_conf = {
    'mipsel' => {:toolchain => "2024.02-105", :prefix => "mipsel",  :subdir => "mipsel-mips32r2-linux-gnu"},
    'arm'    => {:toolchain => "2024.02-105", :prefix => "arm",     :subdir => "arm-cortex_a8-linux-gnueabihf"},
    'arm64'  => {:toolchain => "2024.02-105", :prefix => "aarch64", :subdir => "arm64-armv8_a-linux-gnu"},
    'x86'    => {}
}

$FRR_40 = {
    :version      => "4.0",
    :file         => "frr-4.0.tar.gz",
    :site         => "https://github.com/FRRouting/frr/archive",
    :md5          => "2d5447348bbf2a49b68909ee927707cc",
    :license_file => "COPYING",
    :license_type => "GPL-2.0",
}

$FRR_50 = {
    :version      => "5.0",
    :file         => "frr-5.0.tar.gz",
    :site         => "https://github.com/FRRouting/frr/archive",
    :md5          => "70404bdd4819734f2b94899c66de1b83",
    :license_file => "COPYING",
    :license_type => "GPL-2.0",
}

$FRR_60 = {
    :version      => "6.0",
    :file         => "frr-6.0.tar.gz",
    :site         => "https://github.com/FRRouting/frr/archive",
    :md5          => "f7584302a8837194021b82ae646d5e2d",
    :license_file => "COPYING",
    :license_type => "GPL-2.0",
}

$FRR_603 = {
    :version      => "6.0.3",
    :file         => "frr-6.0.3.tar.gz",
    :site         => "https://github.com/FRRouting/frr/archive",
    :md5          => "642f390db9cf71875bc9cb8c388e2bb8",
    :license_file => "COPYING",
    :license_type => "GPL-2.0",
}

$FRR_70 = {
    :version      => "7.0",
    :file         => "frr-7.0.tar.gz",
    :site         => "https://github.com/FRRouting/frr/archive",
    :md5          => "5fc120c73133f71b7f2118ec96f4ee43",
    :license_file => "COPYING",
    :license_type => "GPL-2.0",
}

$FRR_72 = {
    :version      => "7.2",
    :file         => "frr-7.2.tar.gz",
    :site         => "https://github.com/FRRouting/frr/archive",
    :md5          => "3bf3e2df902788378771a0946cd732ff",
    :license_file => "COPYING",
    :license_type => "GPL-2.0",
}

$FRR_74 = {
    :version      => "7.4",
    :file         => "frr-7.4.tar.gz",
    :site         => "https://github.com/FRRouting/frr/archive",
    :md5          => "f93683334c1ab918aefa3223912a5bec",
    :license_file => "COPYING",
    :license_type => "GPL-2.0",
}

# This hash also determines the order that things build in, so that FRR must
# come last, because it depends on the others.
$src_conf = {
    "c-ares" =>  { :version      => "1.16.1",
                   :file         => "c-ares-1.16.1.tar.gz",
                   :site         => "https://github.com/c-ares/c-ares/releases/download/cares-1_16_1/",
                   :md5          => "62dece8675590445d739b23111d93692",
                   :license_file => "LICENSE.md",
                   :license_type => "MIT-like",
                   :build_func   => :build_c_ares # Cannot use component name, because it contains a dash
                 },

    "json-c" =>  { :version      => "0.15-20200726", # Requires cmake >= 3.10.2 to successfully cross-compile
                   :file         => "json-c-0.15-20200726.tar.gz",
                   :site         => "https://github.com/json-c/json-c/archive",
                   :md5          => "b3841c9abdca837ea00ce6a5ada4bb2c",
                   :license_file => "COPYING",
                   :license_type => "MIT",
                   :build_func   => :build_json_c # Cannot use component name, because it contains a dash
                  },

    # Neeeded by yang
    "pcre" =>    { :version      => "8.44",
                   :file         => "pcre-8.44.tar.gz",
                   :site         => "https://ftp.exim.org/pub/pcre",
                   :md5          => "3bcd2441024d00009a5fee43f058987c",
                   :license_file => "LICENCE",  # SIC! They can't spell
                   :license_type => "BSD",
                  },

    # Needed by FRR >= 7.0
    "yang" =>    { :version      => "1.0.184",
                   :file         => "v1.0.184.tar.gz",
                   :site         => "https://github.com/CESNET/libyang/archive",
                   :md5          => "0ddfdf2dba5e73486c133c1fba1b3e8e",
                   :license_file => "LICENSE",
                   :license_type => "BSD-3-Clause",
                  },

    # This is special, because we can build more than one FRR version into the
    # same .tar.gz tarball.
    # List the versions we want to include in the following array.
#    "frr"     => [$FRR_603, $FRR_74]
    "frr"     => [$FRR_603]
}

$top_dir         = "#{File.expand_path(File.dirname(File.dirname(__FILE__)))}"
$build_dir       = "#{$top_dir}/build"   # This is where we build stuff
$top_staging_dir = "#{$top_dir}/staging" # This is where the components install into on the build host and where collect_results() collects from.
$top_install_dir = "#{$top_dir}/install" # This is where we collect results in to and create root.tar in <arch>-subdirs of this folder with collect_results()
$dl              = "#{$top_dir}/dl"      # This is where we download sources to

# These three are only used in trace to easily see how far we have reached.
$cur_frr_ver = ""
$cur_arch    = ""
$cur_comp    = ""
$cur_ver     = ""

$l = Logger.new(STDOUT)
$l.level = Logger::INFO
log_fmt = proc do |severity, datetime, progname, msg|
    "#{severity} [#{Time.now.strftime('%H:%M:%S')}] #{$cur_frr_ver}#{$cur_arch}#{$cur_comp}#{$cur_ver}: #{msg}\n"
end
$l.formatter = log_fmt

################################################################################
# Command line parsing
################################################################################
global = OptionParser.new do |opts|
    opts.on("-h", "--help", "Help message") do
        puts opts
        exit
    end

    # Optional arguments
    opts.on("-a", "--arch <arch>", $arch_conf.keys(), "Build for a particular architecture (<arch> is one of #{$arch_conf.keys().join(", ")}). Defaults to building for all") do |arch|
        $options[:arch] << arch
    end

    opts.on("-b", "--build <component>", $src_conf.keys(), "Build a particular component (<componnet> is one of #{$src_conf.keys().join(", ")}). Defaults to building all components") do |comp|
        $options[:component] << comp
    end

    opts.on("-c", "--clean", "Clean out any previous build") do
        $options[:clean] = true
    end

    opts.separator "Examples:
    ruby build-quagga.rb                     Build for all archs. Builds on top of what may already exist.
    ruby build-quagga.rb -a mipsel           Only build for MIPS
    ruby build-quagga.rb -a mipsel -a x86    Build for both MIPS and x86
    ruby build-quagga.rb -b c-ares           Only build c-ares, but for all archs
    ruby build-quagga.rb -b c-ares -b json-c Build both c-ares and json-c
    ruby build-quagga.rb -c                  Remove entire ./build folder to start on a fresh
    "
end

# Parse arguments in order. Leave remainder of argv unparsed.
global.order!()

################################################################################
# Helper functions
################################################################################
def run(cmd)
    # Similar to sys(), but doesn't print the output from executing cmd.
    $l.info "Running '#{cmd}' ..."
    o=%x(#{cmd})
    if $? != 0
        $l.fatal "Running '#{cmd}' failed"
        puts o if(o)
        raise "Running '#{cmd}' failed"
    end
    return o.chomp
end

def sys(cmd)
    # Similar to run(), but prints the output from executing cmd.
    $l.info "SysRunning '#{cmd}' ..."
    system "sh -c \"#{cmd}\""
    if $? != 0
        $l.fatal "SysRunning '#{cmd}' failed"
        raise "SysRunning '#{cmd}' failed"
    end
end

def md5chk(f, md5)
    return false if not File.exist?(f)
    return run("md5sum #{f}").split[0] == md5
end

def def_env(arch)
    if arch == "x86"
        return {
            "LD_LIBRARY_PATH"   =>  "/usr/lib:/lib:/usr/local/lib",
            "PATH"              =>  "/usr/bin:/bin:/usr/local/bin:/usr/local/sbin",
            "CFLAGS"            =>  "-Os",
            "CXXFLAGS"          =>  "-Os",
            "CPPFLAGS"          =>  "-I#{$staging_dir}/usr/include/",
            "LDFLAGS"           =>  "'-L#{$staging_dir}/usr/lib/ -Wl,-rpath-link,#{$staging_dir}/usr/lib/'",
            "PKG_CONFIG_PATH"   =>  "#{$staging_dir}/usr/lib/pkgconfig",
            "PKG_CONFIG_LIBDIR" =>  "",
        }.to_a.map{|x| "#{x[0]}=#{x[1]}"}.join(" ")
    end

    if $building_inline
        toolchain_base = ENV['MSCC_SDK_PATH']
    else
        toolchain_base = "#{$arch_conf[arch][:toolchain_dir]}/#{$arch_conf[arch][:subdir]}"
    end

    return {
        "LD_LIBRARY_PATH"   =>  "#{$staging_dir}/usr/lib:/usr/lib:/lib:/usr/local/lib:#{toolchain_base}/usr/lib",
        "PATH"              =>  "/usr/bin:/bin:/usr/local/bin:/usr/local/sbin:#{toolchain_base}/usr/bin",
        "LD"                =>  "#{$arch_conf[arch][:prefix]}-linux-ld",
        "CC"                =>  "#{$arch_conf[arch][:prefix]}-linux-gcc",
        "CXX"               =>  "#{$arch_conf[arch][:prefix]}-linux-gcc",
        "GCC"               =>  "#{$arch_conf[arch][:prefix]}-linux-gcc",
        "STRIP"             =>  "#{$arch_conf[arch][:prefix]}-linux-strip",
        "CFLAGS"            =>  "-Os",
        "CXXFLAGS"          =>  "-Os",
        "CPPFLAGS"          =>  "-I#{$staging_dir}/usr/include/",
        "LDFLAGS"           =>  "'-L#{$staging_dir}/usr/lib/ -Wl,-rpath-link,#{$staging_dir}/usr/lib/'",
        "PKG_CONFIG_PATH"   =>  "#{$staging_dir}/usr/lib/pkgconfig",
        "PKG_CONFIG_LIBDIR" =>  "",
    }.to_a.map{|x| "#{x[0]}=#{x[1]}"}.join(" ")
end

def source_unpack(conf)
    # This function expects CWD is the folder where to unpack the source code.
    run("tar --strip-components=1 -xf #{$dl}/#{conf[:file]}")
end

def patches_apply(comp, conf)
    # This function expects that CWD is the source folder where to apply the
    # patches.

    # To make it possible to "git grep" and make patches based on local changes,
    # put the newly created src folder under GIT control.
    if Dir.exists?(".git")
        # Since .git folder already exists, we have run this function before.
        # Nothing else to do.
        return
    end

    run("git init")
    run("git add .")
    run("git commit -m 'Original #{conf[:file]}'")

    # Patches are on the form "patches/<component>/<version>/<seqno>-<name>.patch
    # The <seqno> is for getting the order to apply the patches.
    match_str = "#{$top_dir}/patches/#{comp}/#{conf[:version]}/*.patch"

    # If you want to run "git clang-format-6.0" on the applied patches, set this
    # to true.
    clang_format = false

    Dir[match_str].sort().each {|p|
        run("patch -p1 < #{p}")
        run("rm -f `find . -name '*.orig'`") if clang_format
        run("git add .")
        run("git clang-format-6.0") if clang_format
        run("git add .") if clang_format
        run("git commit -m '#{File.basename(p)}'")
    }
end

def license_copy(comp, conf)
    # This function expects CWD is the source folder where the license file can
    # be found.
    # It ends up in e.g. ./install/mipsel/frr-x.y/legal-info/licenses/frr-x.y/COPYING
    lic_dir = "#{$license_dir}/#{comp}-#{conf[:version]}"
    run("mkdir -p #{lic_dir}")
    run("cp #{conf[:license_file]} #{lic_dir}")
end

################################################################################
# Build c-ares
################################################################################
def build_c_ares(comp, arch)
    # Expects CWD is build dir.
    if not File.file?("./Makefile")
        # Makefile doesn't exist, so start by configuring.
        $l.info("Configuring #{comp}")

        c  = ""
        c += " --prefix=/usr"
        c += " --sysconfdir=/etc"

        if arch != "x86"
            prefix = $arch_conf[arch][:prefix]
            c += " --target=#{prefix}-linux"
            c += " --host=#{prefix}-linux"
            c += " --build=x86_64-unknown-linux-gnu"
        end

        sys("#{$env} ../configure #{c}")
    else
        $l.info("Skipping configure script, since Makefile already exists")
    end

    sys("#{$env} make -j24")
    sys("#{$env} make DESTDIR=#{$staging_dir} install")
end

################################################################################
# Build json-c
################################################################################
def build_json_c(comp, arch)
    # Expects CWD is build dir.
    if not File.file?("./Makefile")
        # Makefile doesn't exist, so start by configuring.
        $l.info("Configuring #{comp}")

        c  = ""
        c += " -DCMAKE_INSTALL_PREFIX=/usr"
        c += " -DCMAKE_INSTALL_LIBDIR=lib" # For it not to install x86 in $staging_dir/usr/x86_64-linux-gnu/lib/
        c += " -DBUILD_STATIC_LIBS=OFF"
        c += " -DBUILD_TESTING=OFF"
        c += " -DINSTALL_PKGCONFIG_DIR=/usr/lib/pkgconfig"
        sys("#{$env} cmake #{c} ..")
    end

    sys("#{$env} make -j24")
    sys("#{$env} make DESTDIR=#{$staging_dir} install")
end

################################################################################
# Build pcre
################################################################################
def build_pcre(comp, arch)
    # Expects CWD is build dir.
    using_cmake = false
    if not File.file?("./Makefile")
        # Makefile doesn't exist, so start by configuring.
        $l.info("Configuring #{comp}")

        if using_cmake
            # At least in pcre v. 8.44, using cmake causes the pkg-config file
            # pcre.pc NOT to be installed, and it's needed when compiling FRR,
            # because it's asking for a version of libyang >= 0.16.7, which
            # causes pkg-config to open libyang.pc and further open that one's
            # dependencies, hereunder pcre.pc, which it can't find. So use
            # ./configure instead.
            c  = ""
            c += " -DBUILD_SHARED_LIBS=ON"
            c += " -DCMAKE_INSTALL_PREFIX=/usr"
            c += " -DPCRE_SUPPORT_UNICODE_PROPERTIES=ON"
            c += " -DPCRE_SUPPORT_UTF=ON"
            c += " -DPCRE_BUILD_PCRECPP=OFF"
            c += " -DPCRE_BUILD_PCREGREP=OFF"
            c += " -DPCRE_BUILD_TESTS=OFF"
            sys("#{$env} cmake #{c} ..")
        else
            if $arch_conf[arch][:prefix]
                c = "--host=#{$arch_conf[arch][:prefix]}-linux"
            else
                c = "--host=x86_64-unknown-linux-gnu"
            end

            c += " --build=x86_64-unknown-linux-gnu"
            c += " --enable-shared"
            c += " --disable-static"
            c += " --prefix=/usr"
            c += " --enable-unicode-properties"
            c += " --enable-utf"
            c += " --disable-cpp"
            c += " --disable-pcregrep"
            sys("#{$env} ../configure #{c} ..")
        end
    end

    sys("#{$env} make -j24")
    sys("#{$env} make DESTDIR=#{$staging_dir} install")
end

################################################################################
# Build yang
################################################################################
def build_yang(comp, arch)
    # Expects CWD is build dir.
    if not File.file?("./Makefile")
        # Makefile doesn't exist, so start by configuring.
        $l.info("Configuring #{comp}")

        c  = ""
        c += " -DCMAKE_INSTALL_PREFIX=/usr"
        c += " -DCMAKE_SYSTEM_PREFIX_PATH=#{$staging_dir}/usr" # For it to find PCRE
        c += " -DCMAKE_INSTALL_LIBDIR=lib" # For it not to install x86 in $staging_dir/usr/x86_64-linux-gnu/lib/
        c += " -DENABLE_BUILD_TESTS=OFF"
        c += " -DENABLE_VALGRIND_TESTS=OFF"
        c += " -DENABLE_LYD_PRIV=ON"
        c += " -DENABLE_STATIC=OFF"
        c += " -DGEN_PYTHON_BINDINGS=OFF"
        c += " -DPCRE_INCLUDE_DIR=#{$staging_dir}/usr/include"

        # Until we get a version that has fixed the [maybe-uninitialized]
        # warnings, let's disable them because they clutter the output.
        sys("#{$env} CFLAGS='-Os -Wno-maybe-uninitialized' cmake #{c} ..")
    end

    sys("#{$env} make -j24")
    sys("#{$env} make DESTDIR=#{$staging_dir} install")
end

################################################################################
# Build frr
################################################################################
def build_frr(comp, arch)
    # Expects CWD is build dir.

    # This step is required and is independent of architecture (I think)
    if not File.file?("../configure")
        # 'configure' doesn't exist, so start by bootstrapping. FRR
        # tells us to run bootstrap.sh, which is fine when doing this
        # offline, but for some unknown reason, it cannot do it when
        # building from Jenkins (it says that bootstrap.sh not found, even
        # though it really is there). Let's do what bootstrap.sh does then.
        $l.info("Bootstrapping")
        sys("cd .. && #{$env} autoreconf -i")
    else
        $l.info("Skipping autoreconf, since 'configure' already exists")
    end

    # Host tools
    hosttools_dir = "./hosttools"
    if not File.file?("#{hosttools_dir}/lib/clippy")
        run("mkdir -p #{hosttools_dir}")
        Dir.chdir(hosttools_dir) do
            $l.info("Configuring hosttools for #{comp}")

            c  = ""
            c += " --host=x86_64-unknown-linux-gnu"
            c += " --build=x86_64-unknown-linux-gnu"
            c += " --enable-clippy-only"
            c += " --disable-nhrpd"
            c += " --disable-vtysh"
            c += " --enable-shared"
            c += " CFLAGS=-I#{$staging_dir}/usr/include"

            # In order for this to work with FRR 7.0, pretend that the hosttools
            # use packages from .../install/<arch>/root/usr/lib/pkgconfig/
            # in order for it to find libyang (which is not needed by the
            # hosttools)
            e = "PKG_CONFIG_PATH=#{$staging_dir}/usr/lib/pkgconfig"

            sys("#{e} ../../configure #{c}")
            sys("make lib/clippy")
        end
    else
        $l.info("Skipping hosttools for #{comp}, since clippy already exists")
    end

    if not File.file?("./Makefile")
        # Makefile doesn't exist, so start by configuring.
        $l.info("Configuring #{comp}")
        c  = ""
        c += " --host=#{$arch_conf[arch][:prefix]}-linux" if $arch_conf[arch][:prefix]
        c += " --build=x86_64-unknown-linux-gnu"
        c += " --prefix=/usr"
        c += " --with-libyang-pluginsdir=/usr/lib/frr/libyang_plugins"
        c += " --sysconfdir=/etc"
        c += " --localstatedir=/tmp"
        c += " --sharedstatedir=/tmp"
        c += " --enable-user=root"
        c += " --enable-group=root"
        c += " --enable-multipath=8"
        c += " --program-prefix=\"\""
        c += " --disable-libtool-lock"
        c += " --disable-vtysh"
        c += " --disable-doc"
        c += " --enable-shared"
        c += " --enable-zebra"
        c += " --enable-staticd"
        c += " --disable-bfdd"
        c += " --disable-bgpd"
        c += " --enable-ripd"
        c += " --disable-ripngd"
        c += " --enable-ospfd"
        c += " --enable-ospf6d"
        c += " --disable-ldpd"
        c += " --disable-nhrpd"
        c += " --disable-eigrpd"
        c += " --disable-babeld"
        c += " --disable-watchfrr"
        c += " --disable-isisd"
        c += " --disable-pimd"
        c += " --disable-pbrd"
        c += " --disable-vrrpd" # Only needed if v >= 7.2
        c += " --disable-bgp-announce"
        c += " --disable-bgp-vnc"
        c += " --disable-bgp-bmp"
        c += " --disable-ospfapi"
        c += " --disable-ospfclient"
        c += " --disable-snmp"
        c += " --disable-fabricd"
        c += " --disable-capabilities"
        c += " --with-clippy=hosttools/lib/clippy"

        sys("#{$env} ../configure #{c}")
    else
        $l.info("Skipping configure step, since Makefile already exists")
    end

    sys("#{$env} make -j24")
    sys("#{$env} make DESTDIR=#{$staging_dir} install")

    run("mkdir -p #{$staging_dir}/etc/quagga")
    File.open("#{$staging_dir}/etc/quagga/zebra.conf", 'w') do |f|
        f.puts("hostname Router")
        f.puts("password zebra")
        f.puts("enable password zebra")
        f.puts("interface lo")
        f.puts("interface sit0")
        f.puts("log file /tmp/zebra.log")
    end

    File.open("#{$staging_dir}/etc/quagga/staticd.conf", 'w') do |f|
        f.puts("hostname staticd")
        f.puts("password zebra")
        f.puts("log file /tmp/staticd.log")
    end

    File.open("#{$staging_dir}/etc/quagga/ospfd.conf", 'w') do |f|
        f.puts("hostname ospfd")
        f.puts("password zebra")
        f.puts("log file /tmp/ospfd.log")
    end

    File.open("#{$staging_dir}/etc/quagga/ospf6d.conf", 'w') do |f|
        f.puts("hostname ospf6d")
        f.puts("password zebra")
        f.puts("log file /tmp/ospf6d.log")
    end

    File.open("#{$staging_dir}/etc/quagga/ripd.conf", 'w') do |f|
        f.puts("hostname ripd")
        f.puts("password zebra")
        f.puts("log file /tmp/ripd.log")
    end

    File.open("#{$staging_dir}/etc/quagga/daemons", 'w') do |f|
        f.puts("zebra=yes")
        f.puts("staticd=yes")
        f.puts("ospfd=yes")
        f.puts("bgpd=no")
        f.puts("ospf6d=yes")
        f.puts("ripd=yes")
        f.puts("ripngd=no")
        f.puts("isisd=no")
    end
end

################################################################################
# Install toolchains
################################################################################
def install_toolchains
    $arch_conf.each do |k, v|
        next unless v[:toolchain]
        $l.info("Installing toolchain for #{k}")

        if v[:branch]
            pkg_type = "toolchains/#{v[:toolchain]}-#{v[:branch]}"
            pkg_name = "mscc-toolchain-bin-#{v[:toolchain]}-#{v[:branch]}"
        else
            pkg_type = "toolchains/#{v[:toolchain]}-toolchain"
            pkg_name = "mscc-toolchain-bin-#{v[:toolchain]}"
        end

        v[:toolchain_dir] = "/opt/mscc/#{pkg_name}"
        run("sudo /usr/local/bin/mscc-install-pkg -t #{pkg_type} #{pkg_name}")
    end
end

################################################################################
# Download sources
################################################################################
def download_src_do(k, v)
    $l.info("Getting source for #{k}-#{v[:version]} into #{$dl}")
    if not md5chk("#{$dl}/#{v[:file]}", v[:md5])
        sys("wget -O #{$dl}/#{v[:file]} #{v[:site]}/#{v[:file]}")
        if not md5chk("#{$dl}/#{v[:file]}", v[:md5])
            raise "MD5 sum of #{k} did not match!"
        end
    end
end

def download_src
    run("mkdir -p #{$dl}")

    $src_conf.each do |comp, comp_v|
        if comp_v.kind_of?(Array)
            comp_v.each {|ver|
                download_src_do(comp, ver)
            }
        else
            download_src_do(comp, comp_v)
        end
    end
end

################################################################################
# Do build all selected components
################################################################################
def build_all(arch, frr_ver)
    # Create environment variables for this arch
    $env = def_env(arch)

    $src_conf.each{|comp, v|
        if (comp == "pcre" or comp == "yang") and frr_ver.to_f() < 7.0
            # Don't build libpcre and libyang if the FRR version we are building
            # is smaller than 7.0, because that version doesn't need these two
            # libraries.
            next
        end

        comp_v = Hash.new()
        if v.kind_of?(Array)
            # Find the comp_v that corresponds to frr_ver
            v.each {|ver|
                if ver[:version] == frr_ver
                    comp_v = ver
                    break
                end
            }
        else
            comp_v = v
        end

        # For the sake of trace:
        $cur_comp = "-#{comp}";
        $cur_ver  = "-#{comp_v[:version]}"

        src_dir = "#{$build_dir}/#{comp}-#{comp_v[:version]}"
        $l.info("Building #{comp} in #{src_dir}/build-#{arch}")

        src_dir_already_exists = Dir.exists?(src_dir)

        run("mkdir -p #{src_dir}")
        Dir.chdir(src_dir) do
            source_unpack(comp_v) unless src_dir_already_exists
            patches_apply(comp, comp_v)
            license_copy(comp, comp_v)

            if comp_v[:build_func].nil?
                # Build function is not specified in configuration, so use
                # build_<comp>, which could be e.g. "build_frr".
                build_func = "build_#{comp}"
            else
                build_func = comp_v[:build_func]
            end

            build_dir = "#{src_dir}/build-#{arch}"
            run("mkdir -p #{build_dir}")

            Dir.chdir(build_dir) do
                # Invoke it
                send(build_func, comp, arch)
            end
        end
    }
end

################################################################################
# Create manifest.csv
################################################################################
def manifest_create(arch, frr_ver)
    # Create a manifest.csv file that can be used by a script that collects
    # licenses.

    # Notice, dir already exists.
    manifest_csv = "#{$top_staging_dir}/#{arch}/frr-#{frr_ver}/legal-info/manifest.csv"
    $l.info("Creating #{manifest_csv}")

    File.open(manifest_csv, 'w') {|f|
        f.puts("PACKAGE,VERSION,LICENSE,LICENSE FILES,SOURCE ARCHIVE,SOURCE SITE,DEPENDENCIES WITH LICENSES")
        $src_conf.each {|comp, v|
            if (comp == "pcre" or comp == "yang") and frr_ver.to_f() < 7.0
                # Don't include libpcre and libyang if the FRR version we are
                # building is smaller than 7.0, because that version doesn't
                # need these two libraries.
                next
            end

            comp_v = Hash.new()
            if v.kind_of?(Array)
                # Find the comp_v that corresponds to frr_ver
                v.each {|ver|
                    if ver[:version] == frr_ver
                        comp_v = ver
                        break
                    end
                }
            else
                comp_v = v
            end

            f.puts("#{comp},#{comp_v[:version]},#{comp_v[:license_type]},#{comp_v[:license_file]},#{comp_v[:file]},#{comp_v[:site]},")
        }
    }
end

################################################################################
# Create a zebra-only version from the full-fledged
################################################################################
def zebra_only_create(arch, frr_ver)
    frr_install_dir_final        = "#{$top_install_dir}/#{arch}/frr-#{frr_ver}/frr"
    zebra_only_install_dir_final = "#{$top_install_dir}/#{arch}/frr-#{frr_ver}/zebra_only/root"
    run("rm -rf #{zebra_only_install_dir_final}")
    run("mkdir -p #{zebra_only_install_dir_final}")
    run("tar -C #{zebra_only_install_dir_final} -xf #{frr_install_dir_final}/root.tar")

    run("rm -rf #{zebra_only_install_dir_final}/etc/quagga/ospfd.conf")
    run("rm -rf #{zebra_only_install_dir_final}/etc/quagga/ospf6d.conf")
    run("rm -rf #{zebra_only_install_dir_final}/etc/quagga/ripd.conf")

    File.open("#{zebra_only_install_dir_final}/etc/quagga/daemons", 'w') do |f|
        f.puts("zebra=yes")
        f.puts("staticd=yes")
        f.puts("ospfd=no")
        f.puts("bgpd=no")
        f.puts("ospf6d=no")
        f.puts("ripd=no")
        f.puts("ripngd=no")
        f.puts("isisd=no")
    end

    run("rm -rf #{zebra_only_install_dir_final}/usr/sbin/ospfd")
    run("rm -rf #{zebra_only_install_dir_final}/usr/sbin/ospf6d")
    run("rm -rf #{zebra_only_install_dir_final}/usr/sbin/ripd")
    run("cp -r #{frr_install_dir_final}/legal-info #{zebra_only_install_dir_final}/..")

    Dir.chdir("#{zebra_only_install_dir_final}") do
        run("rm -rf ../root.tar")
        run("tar -cvzf ../root.tar *")
    end

    run("rm -rf #{zebra_only_install_dir_final}")
end

################################################################################
# Collect results
################################################################################
def collect_results(arch, frr_ver)
    manifest_create(arch, frr_ver)

    if $src_conf["frr"].nil?
        $l.info("Unable to collect results, because FRR is not being built")
        return
    end

    $l.info("Collecting results")

    # We install the final package into $top_install_dir/<arch>/#{frr_ver} and
    # keep the $top_staging_dir/<arch>/#{frr_ver} for reference (also needed to
    # be able to rebuild only frr without running build of other packages,
    # because the collect_results() function is destructive).
    top_staging_dir = "#{$top_staging_dir}/#{arch}/frr-#{frr_ver}"
    top_install_dir = "#{$top_install_dir}/#{arch}/frr-#{frr_ver}/frr"
    run("rm -rf #{top_install_dir}")
    run("mkdir -p #{top_install_dir}")
    run("cp -r #{top_staging_dir}/legal-info #{top_install_dir}")
    run("cp -r #{top_staging_dir}/root       #{top_install_dir}")

    install_dir_final = "#{$top_install_dir}/#{arch}/frr-#{frr_ver}/frr/root"

    run("rm -rf #{install_dir_final}/etc/*.sample")
    run("rm -rf #{install_dir_final}/usr/share")
    run("rm -rf #{install_dir_final}/usr/include")
    run("rm -rf #{install_dir_final}/usr/lib/*.a")
    run("rm -rf #{install_dir_final}/usr/lib/*.la")
    run("rm -rf #{install_dir_final}/usr/lib/cmake")
    run("rm -rf #{install_dir_final}/usr/lib/pkgconfig")
    run("rm -rf #{install_dir_final}/usr/lib/libpcreposix.*")
    run("rm -rf #{install_dir_final}/usr/sbin/frr-reload.py")
    run("rm -rf #{install_dir_final}/usr/sbin/frr-reload")
    run("rm -rf #{install_dir_final}/usr/sbin/frr")
    run("rm -rf #{install_dir_final}/usr/sbin/ssd")
    run("rm -rf #{install_dir_final}/usr/sbin/rfptest")
    run("rm -rf #{install_dir_final}/usr/sbin/*.sh")
    run("rm -rf #{install_dir_final}/usr/bin")

    Dir.chdir("#{install_dir_final}") do
        run("rm -rf ../root.tar")
        run("tar -cvzf ../root.tar *")
    end

    # Get rid of root, which is now tar'ed and squashed and put into parent dir
    run("rm -rf #{install_dir_final}")

    # Build a zebra-only version from the full-fledged
    zebra_only_create(arch, frr_ver)
end

def frr_version_names_get
    # If we are not building FRR, return a place holder, since we don't have a
    # version to return then
    return [""]                         if     $src_conf["frr"].nil?
    return [$src_conf["frr"][:version]] unless $src_conf["frr"].kind_of?(Array)

    versions = Array.new()
    $src_conf["frr"].each {|ver|
        versions << ver[:version]
   }

   return versions
end

################################################################################
# Build
################################################################################
def do_build
    $arch_conf.each {|arch, arch_v|
        # For the sake of trace:
        $cur_arch = "-#{arch}"

        # For each FRR version, we build, we need to create a new staging dir.
        frr_version_names_get().each {|frr_ver|
            $cur_frr_ver = frr_ver

            # This is where the individual components install their artifacts for
            # this architecture
            $staging_dir = "#{$top_staging_dir}/#{arch}/frr-#{frr_ver}/root"

            # This is where the licenses end up for this architecture
            $license_dir = "#{$top_staging_dir}/#{arch}/frr-#{frr_ver}/legal-info/licenses"
            run("mkdir -p #{$license_dir}")
            build_all(arch, frr_ver)
            collect_results(arch, frr_ver)
        }
    }
end

################################################################################
# Get a name for this version
################################################################################
def version_get
    git_sha = run("git rev-parse --short HEAD").chomp()

    # Due to the way Jenkins checks out (git checkout -f <sha>), we get detached
    # from HEAD, so that 'git symbolic-ref' will fail. Therefore, when building
    # from Jeknins, we rely on the BRANCH_NAME environment variable.
    if ENV['BRANCH_NAME']
        git_branch = ENV['BRANCH_NAME']
    else
        git_branch = run("git symbolic-ref --short -q HEAD").chomp()
    end

    if $src_conf["frr"].kind_of?(Array)
        # It's an array of versions
        v = ""
        $src_conf["frr"].each {|ver|
            v += "-" unless v == ""
            v += ver[:version]
        }
    else
        v = $src_conf["frr"][:version]
    end

    return "#{v}-#{git_sha}@#{git_branch}"
end

################################################################################
# Create final tarball
################################################################################
def create_final_tarball
    out_name = "frr-#{$ver}"
    run("rm -rf *.tar.gz")
    run("tar --transform \"s/^\./#{out_name}/\" --exclude ./build --exclude ./staging -cvzf #{out_name}.tar.gz ./*")
end

################################################################################
# Create JSON status
################################################################################
def create_json_status
    h = {"version" => $ver}

    $src_conf.each {|comp, comp_v|
        if comp_v.kind_of?(Array)
            # It's an array of versions
            v = ""
            comp_v.each {|ver|
                v += ", " unless v == ""
                v += ver[:version]
            }
        else
            v = comp_v[:version]
        end

        h["#{comp}-version"] = v
    }

    res = ResultNode.new('build', "OK", h)

    res.reCalc()
    res.to_file("status.json")
end

################################################################################
# main()
################################################################################
run("rm -f status.json")

if $options[:clean]
    run("rm -rf #{$build_dir}")
    run("rm -rf #{$top_staging_dir}")
    run("rm -rf #{$top_install_dir}")
    run("rm -f *.tar.gz")
    exit(0)
end

# Filter out those architectures we won't build for
if not $options[:arch].empty?()
    $arch_conf.select! {|k, v| $options[:arch].include?(k)}
end

# Filter out those components we won't build
if not $options[:component].empty?()
    $src_conf.select! {|k, v| $options[:component].include?(k)}
else
    # No particular component is specified. Clear the staging folder
    $l.info("No component specified. Clearing staging folder(s)")
    $arch_conf.each {|arch, arch_v|
        run("rm -rf #{$top_staging_dir}/#{arch}")
    }
end

# We don't build pcre and yang if FRR is included in what we build and its
# version number is < 7.0, because yang is not used by FRR then and pcre is not
# used by yang then.
if not $src_conf["frr"].nil?
    if $src_conf["frr"].kind_of?(Array)
        # It's an array of FRR versions. See if at least one of them is greater
        # than 7.0
        include_yang = false
        $src_conf["frr"].each {|ver|
            include_yang = true if ver[:version].to_f() >= 7.0
        }
    else
        include_yang = $src_conf["frr"][:version].to_f() >= 7.0
    end

    if not include_yang
        $src_conf.delete("pcre")
        $src_conf.delete("yang")
    end
end

$l.info("Building #{$src_conf.keys().join(", ")} for the following architecture(s): #{$arch_conf.keys().join(", ")}")

if not $building_inline
    # Don't attempt to install any toolchains when building inline with the
    # application, because they're already installed
    install_toolchains()
end

download_src()
do_build()

# For the sake of trace:
$cur_frr_ver = ""
$cur_arch    = ""
$cur_comp    = ""
$cur_ver     = ""

if not $building_inline
    # No need to create a tarball or JSON status when building inline with the
    # application, because the application directly uses the
    # .../install/<arch>/<ver>/[frr|zebra]/root.tar ball when installing.
    $ver = version_get()
    create_final_tarball()
    create_json_status()
end

# If we get this far, everything went as planned (no "raise" calls).
$l.info("Succeeded")
exit 0

