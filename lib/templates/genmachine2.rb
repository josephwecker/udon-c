#!/usr/bin/env ruby
# Skipping the genmachine rubygem temporarily to try and get this working
# simply w/ C / experimenting.


# TODO: probably make this the "consumer" so that the other can ask for the
# next part at any time.

def with_each_part
  ARGF.each do |line|
    if line =~ /\s*\|/
      line = line.split /\|/
      line.each do |part|
        part = parsed_part(part)
        yield part unless part.nil?
      end
    end
  end
end

def parsed_part(part)
  res = {}
  if part =~ /^\s*$/
    return nil
  elsif part.start_with? ' '
    res[:type] = :general_command
    res[:v]    = gen_cmd(part)
  elsif part.start_with? 'result'
    res[:type] = :result_initializer
    res[:v]    = gen_cmd(part[6..-1])
  elsif part.start_with? 'function'
    res[:type] = :function
    res[:v]    = id(part)
  elsif part.start_with? 'state'
    res[:type] = :state
    res[:v]    = id(part)
  elsif part.start_with? 'switch'
    res[:type] = :switch
    res[:v]    = gen_cmd(part[6..-1])
  elsif part.start_with? 'c'
    res[:type] = :case
    res[:v]    = chars(id(part))
  elsif part.start_with? 'default'
    res[:type] = :default
    res[:v]    = nil
  elsif part.start_with? '.'
    res[:type] = :substate
    res[:v]    = part[1..-1].strip
  elsif part.start_with? 'return'
  elsif part.start_with? '>>'
  elsif part.start_with? 'if'
  elsif part.start_with? 'else'
  elsif part.start_with? 'endif'
  elsif part.start_with? 'err'
  elsif part.start_with? 'warn'
  end
end

def id(str) str[/\[[^\]]*/][1..-1] end

def gen_cmd(cmd)
  # Several shorthand translations

end

def chars(str)
  # return array of strings, translated where appropriate and marked as newline
  # or other as appropriate...

end


state = :none
indent= 0
needs_more_entrypoints = []

with_each_part do |part|

end

