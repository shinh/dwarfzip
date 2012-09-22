#!/usr/bin/env ruby

m = {}
$<.each do |line|
  if /(\w+): (\d+) (\d+)/ =~ line
    m[$1] = $3.to_i
  end
end

tot_ref4 = 0
tot_udata = 0
m.each do |k, v|
  if /ref4_(DW_AT_\w+)/ =~ k
    k = $1
    uv = m["ref4_udata_#{k}"]

    puts "#{k} #{v} => #{uv} (%.2f%%)" % (uv * 100.0 / v)

    tot_ref4 += v
    tot_udata += uv
  end
end

puts("total #{tot_ref4} => #{tot_udata} (%.2f%%)" %
     (tot_udata * 100.0 / tot_ref4))
