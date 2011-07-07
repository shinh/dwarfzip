File.open('dwarf.tab', 'w') do |of|
  lines = File.popen('cpp -dM /usr/include/dwarf.h', &:readlines)
  ['DW_AT', 'DW_FORM'].each do |type|
    m = {}
    lines.each do |line|
      if /^#define (#{type}_\S+) (.*)/ =~ line
        if m[$2]
          STDERR.puts "dup...: #{line}"
          next
        end
        m[$2] = $1
      end
    end

    m.each do |v, k|
      of.puts "DEFINE_#{type}(#{k});"
    end
    of.puts

  end
end
