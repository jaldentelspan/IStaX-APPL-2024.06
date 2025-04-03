#!/usr/bin/env ruby
#
# Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.
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

require 'open3'
require 'xmlsimple'

class String
    def starts_with?(prefix)
        prefix = prefix.to_s
        self[0, prefix.length] == prefix
    end

    def ends_with?(prefix)
        prefix = prefix.to_s
        self[-prefix.length..-1] == prefix
    end

end

class Array
    def start_with? a
        if self.size < a.size
            return false
        end

        a.each_index do |i|
            if self[i] != a[i]
                return false
            end
        end

        return true
    end
end

class StreamLineProcesser
    def initialize
        @buf = ''
    end

    def append data
        @buf += data
        line = ''

        while true
            res = @buf.partition("\n")
            if res[1].size == 0
                break;
            else
                line = res[0]
                @buf = res[2]
            end

            yield line
        end
    end

    def finish
        if @buf.length > 0
            yield @buf
        end
    end
end

def run_external_command cmd
    Open3::popen3(cmd) do |_stdin, _stdout, _stderr, _thread|
        _stdin.close
        _stdout.sync = true;
        _stderr.sync = true;

        out_buf = StreamLineProcesser.new
        err_buf = StreamLineProcesser.new
        chunk_size = 4096
        fds_ = [_stdout, _stderr]

        while not fds_.empty?
            fds = select(fds_, nil, nil, nil)

            if fds[0].include? _stdout
                begin
                    out_buf.append _stdout.readpartial(chunk_size) do |l|
                        yield :stdout_line, l
                    end
                rescue EOFError
                    fds_.delete_if {|s| s == _stdout}
                end
            end

            if fds[0].include? _stderr
                begin
                    err_buf.append _stderr.readpartial(chunk_size) do |l|
                        yield :stderr_line, l
                    end

                rescue EOFError
                    fds_.delete_if {|s| s == _stderr}
                end
            end
        end

        out_buf.finish do |l|
            yield :stderr_line, l
        end
        err_buf.finish do |l|
            yield :stderr_line, l
        end

        return _thread.value
    end
end

def run_smi_lint list
    err = 0
    res = run_external_command("smilint -l 6 -c ./smirc #{list.join(" ")}") do |o, l|
      case o
      when :stdout_line
          err += 1
          STDERR.puts l
      when :stderr_line
          err += 1
          STDERR.puts "Error: " + l
      else
      end
    end

    if res != 0 or err != 0
        return false
    else
        return true
    end
end

class MibFile
    attr_accessor :xml
    attr_reader :filename
    attr_reader :errorCnt

    def initialize file
        @filename = file
        @errorCnt = 0
    end

    def error msg
        @errorCnt += 1
        STDERR.puts "Error: #{@filename}: #{msg}"
    end

    def info msg
        STDOUT.puts "Info:  #{@filename}: #{msg}"
    end
end

def filenameChecks m
    if not /-MIB.mib/.match(m.filename)
        log.error "All mib files should end with \"-MIB.mib\" this is not the case with \"#{filename}\""
    end

    base = File.basename(m.filename, File.extname(m.filename))

    if (base != base.upcase)
        log.error "The filename should be in uppercase (execpt for the extension) this is not the case with  \"#{m.filename}\""
    end
end

def filename2modulename filename
    base = File.basename(filename, File.extname(filename))
    names = base.split("-")

    if (names[-1] == "MIB")
        names.pop
    end

    first = names[0]
    first.downcase!
    first += names.drop(1).collect{|x| x.capitalize}.join
    return first
end

def mib2xml m
    filename = m.filename
    xmlbuf = ""
    oldDir = Dir.getwd()
    abs = File.absolute_path(filename)
    Dir.chdir(File.dirname(abs))

    res = run_external_command("smidump -c ./smirc -f xml #{filename}") do |o, l|
      case o
      when :stdout_line
        xmlbuf += l
      end
    end

    Dir.chdir(oldDir)

    if res != 0
        m.error "Failed to convert MIB to XML"
        return
    end

    m.xml = XmlSimple.xml_in(xmlbuf)

    return
end

def nodeByName xml, name
    xml["nodes"][0]["node"].each do |n|
        if n["name"] == name
            return n
        end
    end
    return nil
end


def basicPrefixCheck m
    moduleName = filename2modulename m.filename

    expectedIdentityName = moduleName + "Mib"
    idendity = m.xml["module"][0]["identity"][0]["node"]

    if expectedIdentityName != idendity
        m.error "MODULE-IDENTITY should be: #{expectedIdentityName} (is #{idendity})"
    end

    m.xml["nodes"][0]["node"].each do |n|
        if not n["name"].starts_with? moduleName
            m.error "All OBJECT IDENTIFIER should be prefixed with module name. This is not the case with #{n["name"]} (module name #{moduleName})"
        end
    end

    if not m.xml["nodes"][0]["scalar"].nil?
        m.xml["nodes"][0]["scalar"].each do |n|
            if not n["name"].starts_with? moduleName
                m.error "All OBJECT-TYPE should be prefixed with module name. This is not the case with #{n["name"]} (module name #{moduleName})"
            end
        end
    end

    if not m.xml["nodes"][0]["table"].nil?
        m.xml["nodes"][0]["table"].each do |n|
            if not n["name"].starts_with? moduleName
                m.error "All OBJECT-TYPE should be prefixed with module name. This is not the case with #{n["name"]} (module name #{moduleName})"
            end

            if not n["name"].ends_with? "Table"
                m.error "All table names should end with \"Table\" This is not the case with #{n["name"]}"
            end

            n["row"].each do |r|
                if not r["name"].ends_with? "Entry"
                    m.error "All table rows names should end with \"Entry\" This is not the case with #{r["name"]}"
                end

                r["column"].each do |c|
                    if not c["name"].starts_with? moduleName
                        m.error "All OBJECT-TYPE should be prefixed with module name. This is not the case with #{c["name"]} (module name #{moduleName})"
                    end
                end
            end
        end
    end

    if not m.xml["typedefs"].nil?
        m.xml["typedefs"][0]["typedef"].each do |t|
            if not t["name"].upcase.starts_with? moduleName.upcase
                m.error "All TEXTUAL-CONVENTION should be prefixed with module name. This is not the case with #{t["name"]} (module name #{moduleName})"
            end

            # It seems like we can releax the prefix check on enumerated values
            #t["namednumber"].each do |n|
            #    if not n["name"].starts_with? moduleName
            #        m.error "All enum values should be prefixed with module name. This is not the case with #{t["name"]}\\#{n["name"]} (module name #{moduleName})"
            #    end
            #end
        end
    end
end

def oidFromNode n
    if n.is_a? Array
        n = n[0]
    end

    v = n["oid"]

    if v.is_a? Array
        v = v[0]
    end

    v.split(".").collect{|x| x.to_i}
end

def oidPrefixMatch base, o
    sub = o[0..(base.length - 1)]
    sub == base
end

def oidSubtract a, b
    b.each_index do |i|
        if a.empty?
            return []
        end

        if b[i] == a[0]
            a = a.drop(1)  # pop-head
        else
            break
        end
    end

    return a
end

def oidRelative base, o
    if not oidPrefixMatch base, o
        log.error "Unexpected OID: [#{o.join(", ")}] base: [#{base.join(", ")}]"
    end

    return oidSubtract o, base
end

def oidRelativeFromNode base, n
    o = oidFromNode n
    return oidRelative base, o
end

def basicLayoutCheck m
    moduleName = filename2modulename m.filename
    idendity = m.xml["module"][0]["identity"][0]["node"]
    baseNode = nodeByName m.xml, idendity
    baseOid = oidFromNode baseNode

    idendity = idendity[0..-4]


    # Check first and second level OBJECT IDENTIFIER
    #
    # Here is the layout we expect to find
    #     []      <ModuleName>Mib
    #     [1]     <ModuleName>MibObjects
    #     [1, 1]  <ModuleName>Capabilities
    #     [1, 2]  <ModuleName>Config
    #     [1, 3]  <ModuleName>Status
    #     [1, 4]  <ModuleName>Control
    #     [1, 5]  <ModuleName>Statistics
    #     [2]     <ModuleName>MibConformance
    #     [2, 1]  <ModuleName>MibCompliances
    #     [2, 2]  <ModuleName>MibGroups

    m.xml["nodes"][0].each do |k, v|
        v.each do |n|

            o = oidRelativeFromNode baseOid, n

            # only look at level 1 and 2
            if o.size > 2
                next
            end

            if k != "node"
                m.error "Expected only to fine OBJECT IDENTIFIER in level 1 and 2 of the MIB but found a #{k} #{n}"
                next
            end

            case o

            when []
                exp = "#{idendity}Mib"
                if exp != n["name"]
                    m.error "Unexpected name of OBJECT IDENTIFIER: #{n["name"]} expected #{exp} (relative OID [])"
                end

            when [1]
                exp = "#{idendity}MibObjects"
                if exp != n["name"]
                    m.error "Unexpected name of OBJECT IDENTIFIER: #{n["name"]} expected #{exp} (relative OID [1])"
                end

            when [1, 1]
                exp = "#{idendity}Capabilities"
                if exp != n["name"]
                    m.error "Unexpected name of OBJECT IDENTIFIER: #{n["name"]} expected #{exp} (relative OID [1, 1])"
                end

            when [1, 2]
                exp = "#{idendity}Config"
                if exp != n["name"]
                    m.error "Unexpected name of OBJECT IDENTIFIER: #{n["name"]} expected #{exp} (relative OID [1, 2])"
                end

            when [1, 3]
                exp = "#{idendity}Status"
                if exp != n["name"]
                    m.error "Unexpected name of OBJECT IDENTIFIER: #{n["name"]} expected #{exp} (relative OID [1, 3])"
                end

            when [1, 4]
                exp = "#{idendity}Control"
                if exp != n["name"]
                    m.error "Unexpected name of OBJECT IDENTIFIER: #{n["name"]} expected #{exp} (relative OID [1, 4])"
                end

            when [1, 5]
                exp = "#{idendity}Statistics"
                if exp != n["name"]
                    m.error "Unexpected name of OBJECT IDENTIFIER: #{n["name"]} expected #{exp} (relative OID [1, 5])"
                end

            when [1, 6]
                exp = "#{idendity}Trap"
                if exp != n["name"]
                    m.error "Unexpected name of OBJECT IDENTIFIER: #{n["name"]} expected #{exp} (relative OID [1, 6])"
                end

            when [2]
                exp = "#{idendity}MibConformance"
                if exp != n["name"]
                    m.error "Unexpected name of OBJECT IDENTIFIER: #{n["name"]} expected #{exp} (relative OID [2])"
                end

            when [2, 1]
                exp = "#{idendity}MibCompliances"
                if exp != n["name"]
                    m.error "Unexpected name of OBJECT IDENTIFIER: #{n["name"]} expected #{exp} (relative OID [2, 1])"
                end

            when [2, 2]
                exp = "#{idendity}MibGroups"
                if exp != n["name"]
                    m.error "Unexpected name of OBJECT IDENTIFIER: #{n["name"]} expected #{exp} (relative OID [2, 2])"
                end

            else
                m.error "Unexpected OID [#{o.join(", ")}] #{n}"
            end
        end
    end
end

class NameHierarchyChecker
    class Pair
        attr_reader :oid
        attr_reader :name

        def initialize o, n
            @oid = o
            @name = n
        end

        def to_s
            "#{name}:[#{@oid.join(", ")}]"
        end

        def <=> rhs
            self.oid.each_index do |i|
                if i >= rhs.oid.size
                    return 1
                end

                if self.oid[i] != rhs.oid[i]
                    return self.oid[i] <=> rhs.oid[i]
                end
            end

            if self.oid.size == rhs.oid.size
                return 0
            else
                return -1
            end
        end
    end

    def initialize i, m
        @i = []
        @module = m

        i.each do |k, v|
            @i.push Pair.new(k, v)
        end

        @i.sort!
    end

    def check
        if @i.size <= 2
            return
        end

        do_check "", 1, @i[0]
    end

    def do_check indent, idx, parent
        if @i.size <= idx
            return 0
        end

        #puts "#{indent}do_check IDX: #{idx} PARENT: #{parent} THIS: #{@i[idx]}"
        matched = 0
        while idx < @i.size
            if @i[idx].oid.size <= parent.oid.size
                #puts "#{indent}Return #{matched}"
                return matched
            end

            if not @i[idx].oid.start_with? parent.oid
                #puts "#{indent}Return #{matched}"
                return matched
            end

            if not @i[idx].name.start_with? parent.name
                nn = @i[idx].name.sub(/^vtss/, "")
                nn = nn.sub(/Table$/, "")
                @module.error "no name inherit. PARENT: #{parent} #{nn} THIS: #{@i[idx]}"
            end

            #puts "#{indent}Match confirmed: #{idx} #{@i[idx].name}/#{parent.name}"

            idx += 1
            matched += 1

            val = do_check(indent + "    ", idx, @i[idx - 1])

            idx += val
            matched += val
        end

        #puts "#{indent}Return #{matched}"
        return matched
    end

end

def nameHierarchyCheck m
    moduleName = filename2modulename m.filename
    idendity = m.xml["module"][0]["identity"][0]["node"]
    baseNode = nodeByName m.xml, idendity
    baseOid = oidFromNode baseNode
    idendity = idendity[0..-4]

    # collect all nodes in a map - a map wil always be sorted using the key
    # which is important later
    nodes = {}
    m.xml["nodes"][0].each do |k, v|
        v.each do |n|
            o = oidRelativeFromNode baseOid, n

            # we are only interested in the root and the <module-name>MibObjects
            # branch.
            if o.size == 0 or o[0] != 1
                next
            end

            o = o.drop 1

            case o
            when []
                nodes[o] = idendity
            else
                nodes[o] = n["name"]
            end
        end
    end

    checker = NameHierarchyChecker.new nodes, m
    checker.check

end

stats = {}

FILELIST = []

ARGV.each do |f|
    if not File.file? f
        STDERR.puts "Error: No such file: #{f}"
        next
    end

    FILELIST.push f
end


if FILELIST.size == 0
    puts "Usage: vtss-mib-lint.rb mib-file [mib-files...]"
    exit -1
end

smiLintStatus = run_smi_lint FILELIST

FILELIST.each do |a|
    m = MibFile.new a

    begin
        mib2xml m
        if m.xml.nil?
            next
        end

        filenameChecks m
        basicPrefixCheck m
        basicLayoutCheck m
        nameHierarchyCheck m

    rescue => exception
        m.error "Unknown error: #{exception}\n"
        exception.backtrace.each do |e|
            puts e
        end
    end

    if m.errorCnt == 0
        m.info "Passed all VTSS-SMI conventions checks"
    end

    stats[a] = m.errorCnt
end


if stats.size > 1
    error_stats = stats.select { |k, v| v > 0 }
    if error_stats.size == 0 and smiLintStatus
        STDOUT.puts ""
        STDOUT.puts "All MIBs passed the tests with no errors"
    else
        STDERR.puts ""
        if not smiLintStatus
            STDERR.puts "Error: Errors found when running smilint on the combined set of MIBs"
        end

        if error_stats.size != 0
            STDERR.puts "Error: Errors found in the following MIB(s) (#{error_stats.size}/#{stats.size})"
            error_stats.each do |k, v|
                STDERR.puts "Error:   #{k}"
            end
        end
    end
end

