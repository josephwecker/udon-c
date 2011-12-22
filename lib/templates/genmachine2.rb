#!/usr/bin/env ruby
# Skipping the genmachine rubygem temporarily to try and get this working
# simply w/ C / experimenting.

ARGF.each do |line|
  if line =~ /\s*\|/
    line = line.split /\s*\|\s*/

  end
end
