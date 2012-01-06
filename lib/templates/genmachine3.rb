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
        f.body << GMCommand.new(f, "p->result = #{parse_location(c[2])[1]}")
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
    loc = loc.split(/(?=\/|:|\.)/)
    loc = loc.reduce({}) do |res, part|
      if part[0..0] == '/' then res[:function] = function_name(part[1..-1])
      elsif part[0..0] == ':' then res[:state] = 's_' + part[1..-1].gsub(/-/,'_')
      elsif part[0..0] == '.' then res[:substate] = part[1..-1]
      else res[:state] = part.gsub(/-/,'_') end
      res
    end
    @epoints << loc if loc[:function] && loc[:state]  # will mean create extra functions
    res = [loc[:function], loc[:state], loc[:substate]].compact
    res = res.join('__')
    res += '(p)' if loc[:function]
    return [loc, res]
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

  def check_basic_commands(c, &src)
    matched = false
    is_done = false
    new_c = c
    if c[0] == ''
      @body << GMCommand.new(self, c[2])
      new_c = yield
      matched = true
      is_done = false
    elsif c[0] == '>>'
      if c[2] == ''
        @body << GMCommand.new(self, "goto #{gms.name}")
      else
        loc, lstr = gm.parse_location(c[2])
        gmf.expected[lstr] = loc
        @body << GMCommand.new(self, "goto #{lstr}")
      end
      new_c = yield
      matched = true
      is_done = true
    elsif c[0] == 'err'
      @body << GMCommand.new(self, "_#{gm.name.upcase}_ERR(\"#{c[2].gsub('"','\\"')}\")")
      new_c = yield
      matched = true
      is_done = true
    elsif c[0] == 'return'
      @body << GMCommand.new(self, "return #{c[2]}")
      new_c = yield
      matched = true
      is_done = true
    elsif c[0] == 'if'
      cond = GMConditional.new(self, c[1])
      @body << cond
      new_c = cond.parse(&src)
      matched = true
      is_done = false
    end

    return [matched, new_c, is_done]
  end

  def gm
    return self if self.is_a?(GM)
    p = @parent
    while !p.is_a?(GM); p = p.parent end
    return p
  end
  def gmf
    return self if self.is_a?(GMFunction)
    p = @parent
    while !p.is_a?(GMFunction); p = p.parent end
    return p
  end
  def gms
    return self if self.is_a?(GMState)
    p = @parent
    while !p.is_a?(GMState); p = p.parent end
    return p
  end
  def parse(&src) raise "OVERRIDE ME" end
  def emit(indent=0) raise "OVERRIDE ME" end
end

class GMCommand < GMGenericChild
  attr_accessor :ignore
  def initialize(parent, cmd, statement=false)
    cmd       = cmd.strip
    super(parent, cmd)
    @statement= statement
    @ignore   = false
    @cmd      = nil
    @operator = nil
    @lside    = nil
    @rside    = nil
    if cmd.start_with? '/'; @cmd = gm.parse_location(cmd)[1]
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

    if @statement
      if @cmd
        print outv(@cmd)
      elsif @operator == '='
        print "("+outv(@lside)+@operator+outv(@rside)+")"
      else
        print (outv(@lside) || '')+(@operator || '')+(outv(@rside) || '')
      end
    else
      print '    '*indent
      if @cmd
        puts outv(@cmd) + ";"
      elsif @operator == :advance
        if gm.checked_newline?
          puts "p->column = 1; p->line ++; p->curr ++;"
          gm.checked_newline = false;
        else puts "p->column ++; p->curr ++;" end
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
    elsif cmd.start_with?('/')             ; return [:location, gm.parse_location(cmd)[1]]
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
      val.gsub(/(?<=^|[^a-zA-Z0-9_])#{from}(?=$|[^a-zA-Z0-9_])/u, to)
    end
  end
end

class GMFunction < GMGenericChild
  attr_accessor :ftype, :expected, :implemented
  def initialize(parent, name, return_type, inner=false)
    # Locals: type, name, declaration position, optional initialization value
    super(parent, name)
    @expected = {}  # states and substates that will be jumped to
    @implemented = {}
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
        statename = 's_' + c[1].gsub(/-/,'_').gsub(':','')
        st = GMState.new(self, statename)
        @body << st
        @implemented[statename] = true
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
    @expected.each do |e,parts|
      unless @implemented[e]
        msg = "Parser for '#{parts[:state].gsub(/^s_/,'')}' "
        msg += "(substate '#{parts[:substate]}') " if parts[:substate]
        msg += "not yet implemented."
        puts "    #{e}:"
        puts "        _#{gm.name.upcase}_ERR(\"#{msg}\");"
      end
    end
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
        cstmt = GMCase.new(self, c[1])
        @body << cstmt
        c = cstmt.parse(&src)
      elsif c[0] == 'default'
        dstmt = GMCase.new(self, nil)
        @body << dstmt
        c = dstmt.parse(&src)
      else
        res, c, is_done = check_basic_commands(c, &src)
        return c if is_done && res
        unless res
          require 'pp'; print "unprocessed (state): "; pp c; c = yield
        end
      end
    end
  end

  def emit(indent)
    puts ('    '*indent)+@name+':'
    indent += 1
    in_switch = false
    @body.each do |b|
      if b.is_a?(GMCase) && !in_switch
        in_switch = true
        puts ('    '*indent)+'switch(*(p->curr)) {'
        indent += 1
      end

      b.emit(indent)
    end
    puts(('    '*(indent-1)) + '}') if in_switch
  end
end


class GMCase < GMGenericChild
  def initialize(par, nm)
    super(par, nm)
    if @name.nil?
      @default = true
      gm.checked_newline = false
    else
      @default = false
      @name = @name.gsub('<LBR>','[').gsub('<PIPE>','|').gsub('\\t',"\t").gsub('\\n',"\n")
      @chars = @name.split(//).map{|c| c.gsub("\n",'\\n').gsub("\t",'\\t').gsub("'","\\'").gsub("\\","\\\\")}
      gm.checked_newline = @chars.include?("\\n")
    end
  end

  def parse(&src)
    c = yield
    loop do
      if ['enum','struct','function','state','c','default'].include?(c[0])
        return c
      elsif c[0] == '.'
        @substate = c[2]
        gmf.implemented[gms.name + '__' + @substate] = true
        c = yield
      else
        res, c, is_done = check_basic_commands(c, &src)
        return c if is_done && res
        unless res
          require 'pp'; print "unprocessed (case): "; pp c; c = yield
        end
      end
    end
  end

  def emit(indent)
    cstmts = []
    if @default
      cstmts = ["default:"]
      gm.checked_newline = false
    else
      cstmts = @chars.map{|c| "case '#{c}':"}
      gm.checked_newline = @chars.include?("\\n")
    end
    if @substate
      cstmts[-1] = cstmts[-1].ljust(20,' ') + "/* #{gms.name.gsub(/^s_/,'')}.#{@substate} */"
      substate_label = @parent.name + '__' + @substate
      if gmf.expected[substate_label]
        cstmts << '  ' + substate_label + ':'
      end
    end
    puts cstmts.map{|c| '    '*indent + c}.join("\n")
    @body.each{|b| b.emit(indent+1)}
  end
end


class GMConditional < GMGenericChild
  def initialize(p,n)
    super(p,n)
    @conds = []
    @curr_clause = GMCommand.new(self, n, true)
  end

  def parse(&src)
    c = yield
    loop do
      if c[0] == 'elsif'
        @conds << [@curr_clause, @body.dup]
        @body = []
        @curr_clause = GMCommand.new(self, c[1], true)
        c = yield
      elsif c[0] == 'else'
        @conds << [@curr_clause, @body.dup]
        @body = []
        @curr_clause = nil
        c = yield
      elsif c[0] == 'endif'
        @conds << [@curr_clause, @body]
        c = yield
        return c
      else
        res, c, is_done = check_basic_commands(c, &src)
        unless res
          require 'pp'; print "unprocessed (if-statement): "; pp c; c = yield
        end
      end
    end
  end

  def emit(indent)
    first_if = true
    i = '    '*indent
    @conds.each do |clause, body|
      if first_if
        print i+"if("; clause.emit(0); puts ") {"
        first_if = false
      elsif clause
        print i+"} else if("; clause.emit(0); puts ") {"
      else
        puts i+"} else {"
      end
      body.each{|b| b.emit(indent+1)}
    end
    puts i+"}"
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
