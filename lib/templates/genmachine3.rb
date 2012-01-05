#!/usr/bin/env ruby
# encoding: UTF-8

class GM
  attr_accessor :name, :body, :epoints, :allocators
  def initialize
    @name = ''
    @body = []
    @epoints = []
    @checked_newline = false
    @allocators = {}
  end

  def checked_newline?() return @checked_newline end
  def checked_newline=(v) @checked_newline = v end

  def parse(&src)
    c = yield
    loop do
      if c[0] == 'parser'
        @name = c[2].downcase
        c = yield
      elsif c[0] == 'entry-point'
        f = GMFunction.new(self, "#{@name}_parse", 'int')
        f.body << GMCommand.new(self, "int errval = setjmp(p->err_jmpbuf)")
        f.body << GMCommand.new(self, "if(errval) return errval")
        f.body << GMCommand.new(self, "p->result = #{parse_location(c[2])}")
        f.body << GMCommand.new(self, "return 0")
        @body << f
        c = yield
      elsif c[0] == 'function'
        fname, ftype = c[1].split(':')
        f = GMFunction.new(self, function_name(fname), ftype, true)
        @body << f
        c = f.parse(&src)
      else
        require 'pp'; print "unprocessed (toplevel): "; pp c; c = yield
      end
    end
  end

  def parse_location(loc)
    loc = loc[1..-1].split(':')
    res = {:function => function_name(loc[0])}
    loc = loc[1..-1]
    if loc.nil? or loc.join(':').strip == ''
      res[:state] = nil
      res[:substate] = nil
    else
      loc = loc.join(':').split('.')
      res[:state] = 's_' + loc[0].gsub(/-|:/,'_')
      res[:substate] = loc[1].gsub(/-|:/,'_') unless loc[1].nil?
    end
    if res[:state]
      @epoints << res
    end
    return res.values.compact.join('__') + '(p)'
  end

  def allocator(for_type)
    fname = '_new_' + @name.downcase + '_' + for_type.downcase
    @allocators[for_type] = fname
    return fname + '()'
  end

  def type_name(n)      @name.capitalize         + n end
  def const_name(n)     @name.upcase+'_'         + n end
  def function_name(n)  '_' + @name.downcase+'_' + n end

  def emit; @body.each{|b| b.emit; puts "\n\n"} end
end

class GMGenericChild
  attr_accessor :parent, :name, :body
  def initialize(parent, name)
    @parent = parent
    @name = name
    @body = []
  end
  def checked_newline?() return @checked_newline end
  def checked_newline=(v) @checked_newline = v; @parent.checked_newline = v end
  def gm
    p = @parent
    while !p.is_a?(GM); p = p.parent end
    return p
  end
  def gmf
    p = @parent
    while !p.is_a?(GMFunction); p = p.parent end
    return p
  end
  def parse(&src) raise "OVERRIDE ME" end
  def emit(indent=0) raise "OVERRIDE ME" end
end

class GMCommand < GMGenericChild
  attr_accessor :ignore
  def initialize(parent, cmd)
    cmd       = cmd.strip
    super(parent, cmd)
    @ignore   = false
    @cmd      = nil
    @operator = nil
    @lside    = nil
    @rside    = nil
    if cmd.start_with? '/'; @cmd = gm.parse_location(cmd)
    elsif cmd == '->';      @operator = :advance
    elsif cmd =~ /((!|=|<|(?<!-)>|\+|-(?!>))+)/
      @operator = $1
      lside, rside = cmd.split(@operator)
      @lside = parse_side(lside)
      @rside = parse_side(rside)
      if @operator == '='
        # Assignment: might need to make sure local variable exists etc.
        if @rside[0] == :number
          ltype = 'uint64_t'
        elsif @rside[0] == :type
        if @parent.is_a?(GMFunction)
          if
          @parent.register_local(
      end
    else
      @cmd = parse_side(cmd)
    end
  end
    
  def emit(indent)
    #print '    '*indent
    #if @cmd
    #  puts outv(@cmd) + ";"
    #elsif @operator == :advance
    #  if @parent.checked_newline?
    #    puts "p->column = 1; p->line ++; p->curr ++;"
    #    @parent.checked_newline = false;
    #  else
    #    puts "p->column ++; p->curr ++;"
    #  end
    #elsif @operator == '='
      #if (@rside[0] == :number && !@parent.loc[outv(@lside)])
      #  puts "uint64_t  #{outv(@lside).ljust(20-10,' ')} = #{outv(@rside)};"
      #else
   #     puts "#{outv(@lside).ljust(20,' ')} = #{outv(@rside)};"
      #end
   # elsif @operator
   #   puts "#{outv(@lside).ljust(20,' ')} #{@operator} #{outv(@rside)}"
   # else
   #   raise "Can't handle this command: '#{@name}'"
   # end
  end

  #def outv(statement)
  #  stype, sval = statement
  #  if [:general, :number, :location].include?(stype); return sval
  #  elsif stype == :const; return gm.const_name(sval)
  #  elsif stype == :type;  return gm.type_name(sval)
  #  elsif stype == :acc
  #    if sval[0] == 'mark'; return "#{sval[1]}_start = p->curr"
  #    else return "#{(sval[1]+'_length').ljust(20,' ')} = p->curr - #{sval[1]}_start" end
  #  end
  #end

  def parse_side(cmd)
    return nil if cmd.nil?
    cmd = specials_map(cmd)
    if cmd =~ /(?<=^|[^a-zA-Z0-9])(MARK|END)\s*\(([^\)]*)\)(?=$|[^a-zA-Z0-9])/u
                                             return [:acc,      [$1.downcase, $2]]
    elsif cmd.start_with?('/')             ; return [:location, gm.parse_location(cmd)]
    elsif cmd =~ /^[A-Z]+[a-z]+[a-zA-Z]*$/ ; return [:type,     cmd]
    elsif cmd =~ /^[A-Z]+$/                ; return [:const,    cmd]
    elsif cmd =~ /^[0-9.]+$/               ; return [:number,   cmd]
    elsif cmd =~ /^p->(line|col)/          ; return [:number,   cmd]
    else                                     return [:general,  cmd] end
  end

  def specials_map(cmd)
    map = [['S',  'self_res'],           ['LINE', 'p->line'],
           ['COL','p->column'],          ['CURR', '*(p->curr)'],
           ['C',  'self_res->children'], ['α',    'greek_alpha'],
           ['β',  'greek_beta']]
    map.reduce(cmd.strip) do |val, mapping|
      from, to = mapping
      val.gsub(/(?=^|[^a-zA-Z0-9])#{from}(?=$|[^a-zA-Z0-9])/u, to)
    end
  end
end

# TODO: YOU ARE HERE:
#   - propagate local variables now that we can easily
#   - have functions emit the local variables
#   - do the switch statements next (below) - actually, get rid of them.

class GMFunction < GMGenericChild
  attr_accessor :ftype
  def initialize(parent, name, return_type, inner=false)
    # Locals: type, name, declaration position, optional initialization value
    super(parent, name)
    @locals = {}
    if inner
      @ftype = register_return_type(return_type)
    else
      @ftype = return_type
    end
  end

  def register_return_type(rt)
    return if rt.nil?
    if rt == 'STRING'; base_type = 'GMString'
    elsif rt =~ /^[A-Z]+[a-z]+[a-zA-Z]*$/ ; base_type = gm.type_name(rt) end
    register_local(base_type +' *', 'self_res', gm.allocator(base_type), false)
    return "static inline #{base_type} *"
  end

  def register_local(ltype, lname, linitval=nil, prefix_it=true)
    lname = localname(lname) if prefix_it
    return if @locals[lname]
    @locals[lname] = [ltype, linitval]
  end

  def localname(n) '_l_'+n end

  def parse(&src)
    c = yield
    loop do
      if ['enum', 'struct', 'function'].include?(c[0])
        return c
      elsif c[0] == ''
        @body << GMCommand.new(self, c[2])
        c = yield
      elsif c[0] == 'state'
        st = GMState.new(self, 's_' + c[1].gsub(/-|:/,'_'))
        @body << st
        c = st.parse(&src)
      else
        require 'pp'; print "unprocessed (function): "; pp c; c = yield
      end
    end
  end

  def emit
    puts "#{@ftype} #{@name}(#{@parent.name.capitalize}ParseState *p) {"
    # TODO: emit locals
    @body.each{|b| b.emit(1)}
    puts "}"
  end
end


class GMState < GMGenericChild
  # TODO: way better EOF processing
  def parse(&src)
    c = yield
    loop do
      if ['enum','struct','function','state'].include?(c[0])
        return c
      #elsif c[0] == 'switch'
      elsif c[0] == ''
        @body << GMCommand.new(self, c[2])
        c = yield
      else
        require 'pp'; print "unprocessed (state): "; pp c; c = yield
      end
    end
  end

  def emit(indent)
    puts ('    '*indent)+@name+':'
    @body.each{|b| b.emit(indent+1)}
  end
end




#------- Main loop (just feeding stuff to parser) --------
# Pull in source and feed it, one part at a time, to wherever parse_source asks
# with yield.
content = ARGF.readlines.select{|c| c =~ /\s*\|/}.join('').split('|').map{|c| c.rstrip}.select{|c| c.strip != ''}
i = -1
gm = GM.new
gm.parse do |parser|
  i += 1
  break if i >= content.size
  c = content[i]
  next if c =~ /^\s*$/
  tag = c[/^(\.|[^ \[]+)/]
  id  = c[/\[[^\]]*/]
  id  = id[1..-1] unless id.nil?
  rest = c.gsub(/^(\.|[^ \[]+)/,'').gsub(/\[[^\]]*\]/,'').strip
  [(tag || '').strip.downcase, id || '', rest || '']
end
gm.emit
