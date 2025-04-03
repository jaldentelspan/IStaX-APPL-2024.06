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

#  Script for performing various spell checks on ICLI script files (*.icli)

require 'ffi/hunspell'
require 'yaml'

$path_to_global_tags = "../../../vtss_appl/icli/platform/icli_porting_help.h"

$leadind_whitespace = /^\s*/
$trailing_whitespace = /\s*\\*\s*$/
$regexp_help = /^\s*HELP\s*=\s*/
$regexp_keyword = /##/
$regexp_define_icli_help = /^\s*#define\s+ICLI_HELP_/
$regexp_mac_addr = /^xx:xx:xx:xx:xx:xx$|^\h{2}:\h{2}:\h{2}:\h{2}:\h{2}:\h{2}$/

class Dictionary

	def initialize
		@dict = FFI::Hunspell.dict("en_US")

		extra_words = YAML::load_file("icli_spell_chk.yaml")
		extra_words.each do |x|
			@dict.add(x)
		end
	end

	def spell_check_line line
		outcome = true
		line.split(" ").each do |x|
			word = x.gsub(/\,$|\.$|\:$|\!$/, '')
			word = word.gsub(/^[\(\'\[\{]|[\)\'\]\}]$/, '')
			if /[0-9\/]/ =~ word  || word.length == 1 || $regexp_mac_addr =~ word then
				next
			end
			if !@dict.check?(word) then
				outcome = false
				puts "Misspelled word detected: '" + word + "'."
				puts "Possible suggestions are: " + @dict.suggest(word).inspect
			end
		end
		return outcome
	end

	def spell_check_tags hashTags
		outcome = true
		hashTags.each_pair do |key, line|
			if !spell_check_line(line) then
				outcome = false
				puts "Spelling errors detected in the definition of the tag '" + key +
				     "'. See above for details and possible suggestions."
				puts "======================================================"
    		end
		end
		return outcome
	end

	def spell_check_file filename
		outcome = true
		hashLocalTags = Hash.new
		# Since the size of .icli files is not that large, we read the entire file once
		# and store it into memory.
		fileArray = File.read(filename).lines.to_a

		fileArray.each.with_index(1) do |line, idx|
			if $regexp_help =~ line then
				if /\\\s*$/ =~ line then
					text = parse_multiline(fileArray, idx - 1, $regexp_help)
				elsif /#{$regexp_help}#{$regexp_keyword}/ =~ line then
					key = line.gsub(/#{$regexp_help}#{$regexp_keyword}|#{$trailing_whitespace}/, '')
					if !$hashGlobalTags.has_key?(key) then
						if !hashLocalTags.has_key?(key) then
							hashLocalTags[key] = ""
						end
					end
					next
				else
					text = line.gsub(/#{$regexp_help}|#{$trailing_whitespace}/, '')
				end
				if !spell_check_line(text) then
					puts "Spelling errors detected in line " + idx.to_s + " of '" +
		     		filename + "'. See above for details and possible suggestions."
		     		puts "======================================================"
		     		outcome = false
				end
			end
		end

		populate_hash_tags(fileArray, hashLocalTags)
		if !spell_check_tags(hashLocalTags) then
			outcome = false
		end
		return outcome
	end

	def populate_hash_tags fileArray, hashTagValues
		hashTagValues.each_key do |key|
			regexp_tmp = /^\s*#{key}\s*=\s*/
			fileArray.each.with_index do |line, idx|
				if regexp_tmp =~ line then
					if /\\\s*$/ =~ line then
						text = parse_multiline(fileArray, idx, regexp_tmp)
					else
						text = line.gsub(/#{regexp_tmp}|#{$trailing_whitespace}/, '')
					end
					hashTagValues[key] = text
				end
			end
		end
	end

	def close
		@dict.close
	end

end

def parse_multiline fileArray, idx, regexp_leading
	
	text = fileArray[idx].gsub(/#{regexp_leading}|#{$trailing_whitespace}/, '')
	while idx < fileArray.size - 1
		idx += 1
		text = text + ' ' + fileArray[idx].gsub(/#{$leadind_whitespace}|#{$trailing_whitespace}/, '')
		if /\\\s*$/ !~ fileArray[idx] then
			break
		end
	end
	return text
end

#### Main program starts here

# We create a new dictionary object which will be used for the entire check process
# NOTE: it must be explicitly closed at the end
dict = Dictionary.new

# First we read any global "help" tags and store them in a hash. These will be spell checked
# and used for lookup during the check of individual .icli files.
Array globaltags = File.read($path_to_global_tags).lines.to_a
$hashGlobalTags = Hash.new

globaltags.each do |line|
	if $regexp_define_icli_help =~ line then
		key = line[/ICLI_HELP_\S*/]
		value = line.gsub(/#{$regexp_define_icli_help}\S*\s*"|"\s*$/, '')
		$hashGlobalTags[key] = value
	end
end

puts "Performing spell check on the global ICLI 'help' tags:"
puts "******************************************************"
if dict.spell_check_tags($hashGlobalTags) then
	puts "PASS!"
else
	puts "One or more of the global tags defined in '" + $path_to_global_tags +
	     "' has spelling errors. See above for detailed information."
end
puts

Dir.glob("../../../vtss_appl/**/*.icli").sort.each do |filename|
	## FIXME: the following files need script improvements or too many corrections
	if /icli_config.icli$|icli_exec.icli$|misc.icli$|ptp.icli$|synce.icli$|symreg.icli$/ =~ filename then
                puts "Skipping spell check on '" + filename + "':"
                puts "******************************************************"
		puts "WARNING!"
		puts
		next
	end
	puts "Performing spell check on '" + filename + "':"
        puts "******************************************************"
	if dict.spell_check_file(filename) then
		puts "PASS!"
	else
		puts "One or more spelling errors were detected in '" + filename +
	     "'. See above for detailed information."
		break
	end
	puts
end
