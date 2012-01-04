#!/usr/bin/env ruby
# encoding: UTF-8

class GM
  attr_accessor :name, :body, :epoints, :allocators
  def initialize
    @name = ''
    @body = []
    @epoints = []
    @checked_newline = false
    @allocators = []
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
    # TODO: YOU ARE HERE:
    #   return the function name (with parens) that allocates the type
    #   register the allocator in a list of allocators that get emitted later.
    #   Go back and work on "register_return_type"
  end

  def type_name(n)      @name.capitalize         + n end
  def const_name(n)     @name.upcase+'_'         + n end
  def function_name(n)  '_' + @name.downcase     + n end
  def allocator_name(n) '_new_' + @name.downcase + n end

  def emit; @body.each{|b| b.emit; puts "\n\n"} end
end

class GMGenericChild
  attr_accessor :parent, :name, :body, :locals, :allocators
  def initialize(parent, name)
    @parent = parent
    @name = name
    @body = []
    if @parent.methods.include? :locals
      @locals = @parent.locals
    else
      @locals = []
    end

    @allocators = @parent.allocators
  end

  def checked_newline?() return @checked_newline end
  def checked_newline=(v) @checked_newline = v; @parent.checked_newline = v end

  def gm
    p = @parent
    while !p.is_a?(GM); p = p.parent end
    return p
  end

  def parse(&src)
    raise "OVERRIDE ME"
  end

  def emit(indent=0)
    raise "OVERRIDE ME"
  end
end

class GMCommand < GMGenericChild
  attr_accessor :parent
  def initialize(parent, cmd)
    cmd       = cmd.strip
    super(parent, cmd)
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
    else
      @cmd = parse_side(cmd)
    end
  end
    
  def emit(indent)
    print '    '*indent
    if @cmd
      puts outv(@cmd) + ";"
    elsif @operator == :advance
      if @parent.checked_newline?
        puts "p->column = 1; p->line ++; p->curr ++;"
        @parent.checked_newline = false;
      else
        puts "p->column ++; p->curr ++;"
      end
    elsif @operator == '='
      if (@rside[0] == :number && !@parent.locals.include?(outv(@lside)))
        puts "uint64_t  #{outv(@lside).ljust(20-10,' ')} = #{outv(@rside)};"
      else
        puts "#{outv(@lside).ljust(20,' ')} = #{outv(@rside)};"
      end
    elsif @operator
      puts "#{outv(@lside).ljust(20,' ')} #{@operator} #{outv(@rside)}"
    else
      raise "Can't handle this command: '#{@name}'"
    end
  end

  def outv(statement)
    stype, sval = statement
    if [:general, :number, :location].include?(stype)
      return sval
    elsif stype == :const
      return gm.const_name(sval)
    elsif stype == :type
      return gm.type_name(sval)
    elsif stype == :acc
      if sval[0] == 'mark'
        return "#{sval[1]}_start = p->curr"
      else
        return "#{(sval[1]+'_length').ljust(20,' ')} = p->curr - #{sval[1]}_start"
      end
    end
  end

  def parse_side(cmd)
    return nil if cmd.nil?
    cmd = cmd.strip.
      gsub(/(?=^|[^a-zA-Z0-9])S(?=$|[^a-zA-Z0-9])/u,   'self_res').
      gsub(/(?=^|[^a-zA-Z0-9])LINE(?=$|[^a-zA-Z0-9])/u,'p->line').
      gsub(/(?=^|[^a-zA-Z0-9])COL(?=$|[^a-zA-Z0-9])/u, 'p->column').
      gsub(/(?=^|[^a-zA-Z0-9])CURR(?=$|[^a-zA-Z0-9])/u,'*(p->curr)').
      gsub(/(?=^|[^a-zA-Z0-9])C(?=$|[^a-zA-Z0-9])/u,   'self_res->children').
      gsub(/(?=^|[^a-zA-Z0-9])α(?=$|[^a-zA-Z0-9])/u,   'greek_alpha').
      gsub(/(?=^|[^a-zA-Z0-9])β(?=$|[^a-zA-Z0-9])/u,   'greek_beta')

    if cmd.start_with?('/')                ; return [:location, gm.parse_location(cmd)]
    elsif cmd =~ /(?<=^|[^a-zA-Z0-9])(MARK|END)\s*\(([^\)]*)\)(?=$|[^a-zA-Z0-9])/u
      return [:acc, [$1.downcase, $2]]
    elsif cmd =~ /^[A-Z]+[a-z]+[a-zA-Z]*$/ ; return [:type, cmd]
    elsif cmd =~ /^[A-Z]+$/                ; return [:const, cmd]
    elsif cmd =~ /^[0-9.]+$/               ; return [:number, cmd]
    elsif cmd.start_with?('p->line') or cmd.start_with?('p->column')
      return [:number, cmd]
    end
    return [:general, cmd]
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
    @local_definitions = []
    if inner
      #@locals << ['void *', 'self_res'] # TODO: unless "skip" set somehow
      @ftype = register_return_type(return_type)
    else
      @ftype = return_type
    end
  end

  def register_return_type(rt)
    if rt == 'STRING'
      @locals << 'self_res'
      @local_definitions << ['GMString *', 'self_res', gm.allocator('GMString')]
      @body << GMCommand.new
  end

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
