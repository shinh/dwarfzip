#!/usr/bin/env ruby

pipe = File.popen("./dwarfstat #{ARGV[0]}")

cnts = {}
sizes = {}

line = pipe.gets
if line !~ /^total size: (\d+)/
  raise line
end
bytes = $1.to_i

pipe.each do |line|
  if line !~ /^(.*?): (\d+) (\d+)$/
    raise line
  end
  cnts[$1] = $2.to_i
  sizes[$1] = $3.to_i
end

dw_at = {}
dw_form = {}
sizes.each do |k, v|
  if k =~ /^DW_AT_/
    dw_at[$'] = v
  elsif k =~ /^DW_FORM_/
    dw_form[$'] = v
  end
end

dw_at['CU'] = sizes['CU']
dw_at['abbrev'] = sizes['abbrev']
dw_form['CU'] = sizes['CU']
dw_form['abbrev'] = sizes['abbrev']

File.open('out.html', 'w') do |of|
  of.puts <<END
<html>
  <head>
    <script type="text/javascript" src="https://www.google.com/jsapi"></script>
END

  [[dw_at, "DW_AT"],
   [dw_form, "DW_FORM"]].each do |stat, type|
  of.puts <<END
    <script type="text/javascript">
      google.load("visualization", "1", {packages:["corechart"]});
      google.setOnLoadCallback(drawChart);
      function drawChart() {
        var data = new google.visualization.DataTable();
        data.addColumn('string', '#{type}');
        data.addColumn('number', 'size');
        data.addRows(#{stat.size});
END

    stat.sort_by{|k,v|-v}.each_with_index do |a, i|
      of.puts %Q(data.setValue(#{i}, 0, '#{a[0]}');)
      of.puts %Q(data.setValue(#{i}, 1, #{a[1]});)
    end

    of.puts <<END
        var chart = new google.visualization.PieChart(document.getElementById('#{type}'));
        chart.draw(data, {width: 800, height: 600, title: '#{type}'});
      }
    </script>
END
  end

of.puts <<END
  </head>

  <body>
     .debug_info in #{ARGV[0]} - #{bytes/1000/1000.0}MB
END

  ['DW_AT', 'DW_FORM'].each do |type|
    of.puts %Q(<div id="#{type}"></div>)
  end

of.puts <<END
    CU and abbrev mean CU header and abbrev number, specifically
  </body>
</html>
END

end
