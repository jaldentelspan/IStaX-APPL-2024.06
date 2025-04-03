#!/usr/bin/env ruby
# -*- coding: iso-8859-1 -*-
#
# Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.
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

require 'pp'

class ResultsItem
  attr_accessor :text, :color, :href
  def initialize(text, color, href)
    @text = text
    @color = color
    @href = href
  end

  def ccolor(tagname)
    if @color.length > 0
      return %Q!#{tagname}="#{@color}"!
    end
    return ""
  end

  def as_html
    s = ""
    if @href
      s = %Q!<a href="../#{href}">#{text}</a>!
    else
      s = %Q!#{text}!
    end
    return s
  end

  def as_xml
    s = []
    if @color.length > 0
      s << %Q!bgcolor="#{@color}"!
    end
    if @href
      s << %Q!href="#{@href}"!
    end
    s << %Q!value="#{@text}"!
    return s.join(" ")
  end

end

class ResultsRow
  attr_accessor :cells
  def initialize()
    @cells = []
  end
  def addCell(text, href = nil, color = "")
    @cells << ResultsItem.new(text, color, href)
  end
  def addCells(ary)
    ary.each do |n|
      addCell(n)
    end
  end
end

class ResultsTable
  attr_accessor :header, :rows
  def initialize()
    @header = ResultsRow.new()
    @rows = []
    @width = []
  end
  def addHeader(text, href = nil, color = "")
    @header.addCell(text, href, color)
  end
  def addRow(row)
    @rows << row
  end
  def allRows
    return [@header] + @rows
  end
  def colWidth(n)
    if @width.length == 0 && @rows.length
      a = allRows().map { |r| r.cells.map { |c| c.text.to_s.length } }
      @header.cells.each_index.map do |i|
        @width[i] = a.each.map { |r| r[i].nil? ? 0 : r[i] }.max
      end
      # Couldn't get this to work...
      #@width = @header.cells.each_index.map { |col| allRows.map { |r| r.cells[col].text.to_s.length }.max }
    end
    if n < @width.length
      return @width[n] + 1
    end
    return 5
  end
end

class ResultsMatrix
  attr_accessor :tables, :top_url
  def initialize()
    @tables = []
    @top_url = nil
  end

  def render(template, nonewlines = false)
    if nonewlines
      ERB.new(template, 0, '>').result(binding)
    else
      ERB.new(template).result(binding)
    end
  end
                                                                                                                     
  def save(template, file, nonewlines = false)
    File.open(file, "w+") do |f|
      f.write(render(template, nonewlines))
    end
  end

  def addTable(table)
    @tables << table
  end
end
