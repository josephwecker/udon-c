#!/usr/bin/env ruby
# encoding: UTF-8

class GM
  attr_accessor :name, :body, :epoints, :locals
  def initialize
    @name = ''
    @body = []
    @epoints = []
    @checked_newline = false
    @locals = []
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
        f = GMFunction.new(self, "#{c[1]}", "static inline void *")
        @body << f
        c = f.parse(&src)
      else
        require 'pp'
        print "unprocessed: "
        pp c
        c = yield
      end
    end
  end

  def parse_location(loc)
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
    if res[:state]
      @epoints << res
    end
    return res.values.join('__') + '(p)'
  end

  def emit; @body.each{|b| b.emit} end
end

class GMCommand
  attr_accessor :parent, :raw_cmd, :operator
  def initialize(parent, cmd)
    cmd       = cmd.strip
    @parent   = parent
    @raw_cmd  = cmd
    @cmd      = nil
    @operator = nil
    @lside    = nil
    @rside    = nil
    if cmd.start_with? '/'; @cmd = gm_parent.parse_location(cmd)
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
      #else
      #  puts "DOH"
      end
    elsif @operator
      puts "#{outv(@lside).ljust(20,' ')} #{operator} #{outv(@rside)}"
    else
      #raise "Can't handle this command: '#{@raw_cmd}'"
      print @raw_cmd
      if @raw_cmd.strip[-1..-1] == ';' then puts "" else puts ";" end
    end
  end

  def outv(statement)
    # TODO: way to add localvars to parent.
    stype, sval = statement
    if [:general, :number, :location].include?(stype)
      return sval
    elsif stype == :const
      return gm_parent.name.upcase + '_' + sval
    elsif stype == :type
      return gm_parent.name.capitalize + sval
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

    if cmd.start_with?('/')                ; return [:location, gm_parent.parse_location(cmd)]
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

  def gm_parent
    p = @parent
    while !p.is_a?(GM); p = p.parent end
    return p
  end
end

class GMFunction
  attr_accessor :parent, :name, :ftype, :body, :locals
  def initialize(parent, name, ftype)
    @parent = parent
    @name = name
    @ftype = ftype
    @body = []
    @locals = []
  end

  def append_cmd(cmd)

  end

  def checked_newline?() return @checked_newline end
  def checked_newline=(v) @checked_newline = v; @parent.checked_newline = v end

  def parse(&src)
    c = yield
    loop do
      if ['enum', 'struct', 'function'].include?(c[0])
        return c
      elsif c[0] == ''
        @body << GMCommand.new(self, c[2])
        c = yield
      else
        require 'pp'
        print "unprocessed: "
        pp c
        c = yield
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
