#!/usr/bin/env ruby -Ku
# Skipping the genmachine rubygem temporarily to try and get this working
# simply w/ C / experimenting.

def parsed_part(part)
  res = {}
  if part =~ /^\s*$/
    return nil
  elsif part.start_with? ' '
    res[:type] = :cmd
    res[:v]    = gen_cmd(part)
  elsif part.start_with? 'result'
    res[:type] = :result_initializer
    res[:v]    = gen_cmd(part[6..-1])
  elsif part.start_with? 'function'
    res[:type] = :function
    res[:v]    = 'gm__' + id(part)
  elsif part.start_with? 'state'
    res[:type] = :state
    res[:v]    = id(part)
  elsif part.start_with? 'switch'
    res[:type] = :switch
    res[:v]    = gen_cmd(part[6..-1])
  elsif part =~ /c\s*\[/
    res[:type] = :case
    res[:v]    = chars(id(part))
  elsif part.start_with? 'default'
    res[:type] = :default
    res[:v]    = nil
  elsif part.start_with? '.'
    res[:type] = :substate
    res[:v]    = part[1..-1].strip
  elsif part.start_with? 'return'
    res[:type] = :cmd
    res[:v]    = 'return ' + return_part(part[6..-1].strip)
  elsif part.start_with? '>>'
    res[:type] = :goto
    res[:v]    = label_part(part[2..-1].strip)
  elsif part.start_with? 'if'
    res[:type] = :if
    res[:v]    = gen_cmd(id(part))
  elsif part.start_with? 'else'
    res[:type] = :else
    res[:v]    = nil
  elsif part.start_with? 'endif'
    res[:type] = :endif
    res[:v]    = nil
  elsif part.start_with? 'err'
    res[:type] = :error
    res[:v]    = part[3..-1].strip
  elsif part.start_with? 'warn'
    res[:type] = :warning
    res[:v]    = part[4..-1].strip
  elsif part.start_with? 'enum'
    res[:type] = :enum
    res[:v]    = id(part)
  elsif part.start_with? 'struct'
    res[:type] = :struct
    res[:v]    = id(part)
  elsif part.start_with? 'parser'
    res[:type] = :name
    res[:v]    = part[6..-1].strip
  else
    res[:type] = :label
    part = part.split(/\s+/)
    res[:v] = [part[0], part[1..-1]]
    #res[:type] = part[0]
    #res[:v] = part[1..-1].join(' ')
  end
  return res
end

def id(str) str[/\[[^\]]*/][1..-1] end

def gen_cmd(cmd)
  cmd = cmd.strip
  if cmd.start_with? '/'
    return location(cmd)
  elsif cmd == '->'
    return :advance
  elsif cmd =~ /((!|=|<|>|\+|-)+)/
    # binary operator detected
    operator = $1
    lside, rside = cmd.split(operator)
    return [operator, cmdval(lside), cmdval(rside)]
  end
  return cmdval(cmd)
end

def cmdval(str)
  # LINE | COL | CURR | CamelCase | S | C | MARK | END | /some:location | α|	β
  str = str.strip.
    gsub(/(?=^|[^a-zA-Z0-9])S(?=$|[^a-zA-Z0-9])/u,   'self_res').
    gsub(/(?=^|[^a-zA-Z0-9])LINE(?=$|[^a-zA-Z0-9])/u,'p->line').
    gsub(/(?=^|[^a-zA-Z0-9])COL(?=$|[^a-zA-Z0-9])/u, 'p->column').
    gsub(/(?=^|[^a-zA-Z0-9])CURR(?=$|[^a-zA-Z0-9])/u,'*(p->curr)').
    gsub(/(?=^|[^a-zA-Z0-9])C(?=$|[^a-zA-Z0-9])/u,   'self_res->children').
    gsub(/(?=^|[^a-zA-Z0-9])α(?=$|[^a-zA-Z0-9])/u,  'greek_alpha').
    gsub(/(?=^|[^a-zA-Z0-9])β(?=$|[^a-zA-Z0-9])/u,  'greek_beta')


  if str.start_with?('/')
    return [:location, location(str)]
  elsif str =~ /(?:^|[^a-zA-Z0-9])(MARK|END)\s*\(([^\)]*)\)(?:$|[^a-zA-Z0-9])/u
    return [:acc, $1.downcase, $2]
  elsif str =~ /^[A-Z]+[a-z]+[a-zA-Z]*$/
    return [:type, str]
  elsif str =~ /^[A-Z]+$/
    return [:const, str]
  elsif str =~ /^[0-9.]+$/
    return [:number, str]
  end
  return [:general, str]
end

def finished_cmd(cmd)
  if cmd.is_a? Array
# TODO: YOU ARE HERE
# cmd[0] ￫ operator, cmd[1] ￫ left side, cmd[2] ￫ right side
  elsif cmd == :advance
  else
  end
end

def location(loc)
  # TODO: entries into need_more_entrypoints when appropriate
  loc = loc[1..-1].split(':')
  res = {:function => 'gm__' + loc[0]}
  loc = loc[1..-1]
  if loc.nil? or loc.join(':').strip == ''
    res[:state] = nil
    res[:substate] = nil
  else
    loc = loc.join(':').split('.')
    res[:state] = 's_' + loc[0].gsub(/-|:/,'_')
    res[:substate] = loc[1].gsub(/-|:/,'_') unless loc[1].nil?
  end
  return res
end

def callf(loc)
  res = loc[:function]
  res += '__'+loc[:state] unless loc[:state].nil?
  res += '__'+loc[:substate] unless loc[:substate].nil?
  return res + "(p)"
end

def chars(str)
  # return array of strings, translated where appropriate and marked as newline
  # or other as appropriate...
  return str
end

def return_part(obj)
  return obj
end

def label_part(loc)
  return nil if loc.strip == ''
end

def emit_function(name, type, commands)
  puts "#{type} #{name}(#{$name.capitalize}ParseState *p) {"
  puts commands.join("\n")
  puts "}"
end


#---------------- Top level parsing ------------

def parse_source(&src)
  $name = ''
  loop do
    toplevel = yield
    if toplevel[:type] == :name
      $name = toplevel[:v]
    elsif toplevel[:type] == :result_initializer
      initializer(toplevel[:v])
    else
      puts "#{toplevel[:type].to_s.ljust(20,' ')} | #{toplevel[:v]}"
    end
  end
end

def initializer(init_to)
  emit_function("#{$name.downcase}_parse", "int",
                ["    int errval = setjmp(p->err_jmpbuf);",
                 "    if(errval) return errval;",'',
                 "    p->result = #{init_to};",
                 "    return 0;"])
end

=begin
$name = ''
$state = :none
$indent= 0
$need_more_entrypoints = []

current_function = nil
commands = []
with_each_part do |part|
  if part[:type] == :name
    $name = part[:v]
  elsif part[:type] == :result_initializer
    register_function("#{$name.downcase}_parse", "int",
                      ["    int errval = setjmp(p->err_jmpbuf);",
                       "    if(errval) return errval;",'',
                       "    p->result = #{callf(part[:v])};",
                       "    return 0;"])
  elsif part[:type] == :function
    unless current_function.nil?
      # TODO: register the full function.
    end
    current_function = part[:v]
    $state = :function_toplevel
    commands = []
  elsif part[:type] == :cmd && $state == :function_toplevel
    puts "#{part[:type].to_s.ljust(20,' ')} | #{part[:v]}"
    commands << finished_cmd(part[:v])
  else
    puts "#{part[:type].to_s.ljust(20,' ')} | #{part[:v]}"
  end
end
=end

# Pull in source and feed it, one part at a time, to wherever parse_source asks
# with yield.
content = ARGF.readlines.select{|c| c =~ /\s*\|/}.join('').split('|').map{|c| c.rstrip}.select{|c| c.strip != ''}
i = -1
parse_source do |parser|
  i += 1
  break if i >= content.size
  parsed_part(content[i])
end
