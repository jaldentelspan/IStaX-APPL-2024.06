#!/usr/bin/env ruby
# -*- coding: iso-8859-1 -*-
#
# Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.
#
# Unpublished rights reserved under the copyright laws of the United States of
# America, other countries and international treaties. Permission to use, copy,
# store and modify, the software and its source code is granted but only in
# connection with products utilizing the Microsemi switch and PHY products.
# Permission is also granted for you to integrate into other products, disclose,
# transmit and distribute the software only in an absolute machine readable
# format (e.g. HEX file) and only in or with products utilizing the Microsemi
# switch and PHY products.  The source code of the software may not be
# disclosed, transmitted or distributed without the prior written permission of
# Microsemi.
#
# This copyright notice must appear in any copy, modification, disclosure,
# transmission or distribution of the software.  Microsemi retains all
# ownership, copyright, trade secret and proprietary rights in the software and
# its source code, including all modifications thereto.
#
# THIS SOFTWARE HAS BEEN PROVIDED "AS IS". MICROSEMI HEREBY DISCLAIMS ALL
# WARRANTIES OF ANY KIND WITH RESPECT TO THE SOFTWARE, WHETHER SUCH WARRANTIES
# ARE EXPRESS, IMPLIED, STATUTORY OR OTHERWISE INCLUDING, WITHOUT LIMITATION,
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR USE OR PURPOSE AND
# NON-INFRINGEMENT.
#

#############################################################################
# Packages
#############################################################################

require 'tmpdir' # For creating temp. directories
require 'pp'
require 'optparse' # For command line parsing
require 'logger'   # For trace

# Checing if a check is installed at the computer.
def gem_available?(gemname)
  if Gem::Specification.methods.include?(:find_all_by_name)
    not Gem::Specification.find_all_by_name(gemname).empty?
   else
     Gem.available?(gemname)
   end
end

#############################################################################
# Trace system
#############################################################################
class MyLogger < Logger
  def fatal txt
    super(txt)
    exit 1
  end
end


#############################################################################
# System Cmd
#############################################################################
require 'open3'

class SysCmd
  attr_reader :anyErrors

  def initialize
    @anyErrors = 0
  end

  def cmd (cmd)
    stdout, stderr, status = Open3.capture3(cmd)
    if !status.nil? && status.exitstatus != 0
      STDERR.puts("Error: #{cmd} - #{stderr} - #{caller}")
      @anyErrors = 1
    end
    return stdout
  end
end

#############################################################################
# SimpleGrip help functions
#############################################################################

def simpleGridEnvVarGet buildDir
  cmd =  "-e CODE_REVISION=`#{buildDir}/tools/code_version`"
  cmd += " -e CCACHE_DIR='/var/tmp/SimpleGrid/.ccache'"
  cmd += " -e BUILD_NUMBER"
  cmd += " -e USER"
  return cmd
end
