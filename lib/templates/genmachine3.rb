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
        f.body << GMCommand.new(f, "int errval = setjmp(p->err_jmpbuf)")
        f.body << GMCommand.new(f, "if(errval) return errval")
        f.body << GMCommand.new(f, "p->result = #{parse_location(c[2])}")
        f.body << GMCommand.new(f, "return 0")
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
      if @operator.include? '='
        # Assignment: might need to make sure local variable exists etc.
        if @lside[1] !~ /\.|->/
          if @rside[0] == :number
            ltype = 'uint64_t'
          elsif @rside[0] == :type
            ltype = gm.type_name(@rside[1]) + ' *'
            @rside[1] = gm.allocator(@rside[1])
          end

          if @parent.is_a?(GMFunction)
            @parent.register_local(ltype, @lside[1], outv(@rside))
            @ignore = true
          else
            gmf.register_local(ltype, @lside[1])
            @ignore = false
          end
        end
      end
    else
      @cmd = parse_side(cmd)
      if @cmd[0] == :acc
        gmf.register_local('GMString *', @cmd[1][1], gm.allocator('GMString'))
      end
    end
  end
    
  def emit(indent)
    return if @ignore

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
    elsif @operator
      if @operator.length == 1
        puts "#{outv(@lside).ljust(28-(4*indent),' ')} #{@operator} #{outv(@rside)}"
      else
        puts "#{outv(@lside).ljust(27-(4*indent),' ')} #{@operator} #{outv(@rside)}"
      end
    else
      raise "Can't handle this command: '#{@name}'"
    end
  end

  def outv(statement)
    stype, sval = statement
    if [:general, :number, :location].include?(stype); return sval
    elsif stype == :const; return gm.const_name(sval)
    elsif stype == :type;  return gm.type_name(sval)
    elsif stype == :acc
      if sval[0] == 'mark'; return "#{(sval[1]+'->start').ljust(20,' ')} = p->curr"
      else return "#{(sval[1]+'->length').ljust(20,' ')} = p->curr - #{sval[1]}->start" end
    end
  end

  def parse_side(cmd)
    return nil if cmd.nil?
    cmd = specials_map(cmd)
    if cmd =~ /(?<=^|[^a-zA-Z0-9_])(MARK|MARK_END)\s*(?:\(([^\)]*)\))?(?=$|[^a-zA-Z0-9_])/u
                                             return [:acc,      [$1.downcase, $2 || 'self_res']]
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
      val.gsub(/(?=^|[^a-zA-Z0-9_])#{from}(?=$|[^a-zA-Z0-9_])/u, to)
    end
  end
end

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
    register_local(base_type +' *', 'self_res', gm.allocator(base_type))
    return "static inline #{base_type} *"
  end

  def register_local(ltype, lname, linitval=nil)
    return if @locals[lname]
    @locals[lname] = [ltype, linitval]
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
    @locals.each do |n, t_i|
      ltype, linit = t_i
      decl = ltype.nil? ? n : ltype + ' ' + n
      if linit then puts "    #{decl.ljust(24,' ')} = #{linit};"
      else puts "    #{decl};" end
    end
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
      elsif c[0] == 'c'
        
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


class GMCase < GMGenericChild
  def parse(&src)

  end

  def emit(indent)

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
