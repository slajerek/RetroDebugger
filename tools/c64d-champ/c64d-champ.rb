#!/usr/bin/env ruby

require 'fileutils'
require 'open3'
require 'set'
require 'tmpdir'
require 'yaml'
require 'stringio'

KEYWORDS = <<EOS
    ADC AND ASL BCC BCS BEQ BIT BMI BNE BPL BRA BRK BVC BVS CLC CLD CLI CLV
    CMP CPX CPY DEA DEC DEX DEY DSK EOR EQU INA INC INX INY JMP JSR LDA LDX
    LDY LSR MX  NOP ORA ORG PHA PHP PHX PHY PLA PLP PLX PLY ROL ROR RTI RTS
    SBC SEC SED SEI STA STX STY STZ TAX TAY TRB TSB TSX TXA TXS TYA
EOS

# font data borrowed from http://sunge.awardspace.com/glcd-sd/node4.html
# Graphic LCD Font (Ascii Charaters 0x20-0x7F)
# Author: Pascal Stang, Date: 10/19/2001
FONT_DATA = <<EOS
    00 00 00 00 00 00 00 5F 00 00 00 07 00 07 00 14 7F 14 7F 14 24 2A 7F 2A 12
    23 13 08 64 62 36 49 55 22 50 00 05 03 00 00 00 1C 22 41 00 00 41 22 1C 00
    08 2A 1C 2A 08 08 08 3E 08 08 00 50 30 00 00 08 08 08 08 08 00 60 60 00 00
    20 10 08 04 02 3E 51 49 45 3E 00 42 7F 40 00 42 61 51 49 46 21 41 45 4B 31
    18 14 12 7F 10 27 45 45 45 39 3C 4A 49 49 30 01 71 09 05 03 36 49 49 49 36
    06 49 49 29 1E 00 36 36 00 00 00 56 36 00 00 00 08 14 22 41 14 14 14 14 14
    41 22 14 08 00 02 01 51 09 06 32 49 79 41 3E 7E 11 11 11 7E 7F 49 49 49 36
    3E 41 41 41 22 7F 41 41 22 1C 7F 49 49 49 41 7F 09 09 01 01 3E 41 41 51 32
    7F 08 08 08 7F 00 41 7F 41 00 20 40 41 3F 01 7F 08 14 22 41 7F 40 40 40 40
    7F 02 04 02 7F 7F 04 08 10 7F 3E 41 41 41 3E 7F 09 09 09 06 3E 41 51 21 5E
    7F 09 19 29 46 46 49 49 49 31 01 01 7F 01 01 3F 40 40 40 3F 1F 20 40 20 1F
    7F 20 18 20 7F 63 14 08 14 63 03 04 78 04 03 61 51 49 45 43 00 00 7F 41 41
    02 04 08 10 20 41 41 7F 00 00 04 02 01 02 04 40 40 40 40 40 00 01 02 04 00
    20 54 54 54 78 7F 48 44 44 38 38 44 44 44 20 38 44 44 48 7F 38 54 54 54 18
    08 7E 09 01 02 08 14 54 54 3C 7F 08 04 04 78 00 44 7D 40 00 20 40 44 3D 00
    00 7F 10 28 44 00 41 7F 40 00 7C 04 18 04 78 7C 08 04 04 78 38 44 44 44 38
    7C 14 14 14 08 08 14 14 18 7C 7C 08 04 04 08 48 54 54 54 20 04 3F 44 40 20
    3C 40 40 20 7C 1C 20 40 20 1C 3C 40 30 40 3C 44 28 10 28 44 0C 50 50 50 3C
    44 64 54 4C 44 00 08 36 41 00 00 00 7F 00 00 00 41 36 08 00 08 08 2A 1C 08
    08 1C 2A 08 08
EOS

FONT = FONT_DATA.split(/\s+/).map { |x| x.strip }.reject { |x| x.empty? }.map { |x| x.to_i(16) }

class Champ
    def initialize
        if ARGV.empty?
            STDERR.puts 'Usage: ./c64d-champ.rb [options] <file.pd>'
            STDERR.puts 'Options:'
            STDERR.puts '  --max-frames <n>'
            STDERR.puts '  --error-log-size <n> (default: 20)'
            STDERR.puts '  --no-animation'
            exit(1)
        end

        @have_dot = `dot -V 2>&1`.strip[0, 3] == 'dot'
        @files_dir = 'report-files'
        FileUtils.rm_rf(@files_dir)
        FileUtils.mkpath(@files_dir)
        @max_frames = nil
        @record_frames = true
        @cycles_per_function = {}
        @execution_log = []
        @execution_log_size = 20
        @code_for_pc = {}
        @source_for_file = {}
        @max_source_width_for_file = {}
        @pc_for_file_and_line = {}
        args = ARGV.dup
        while args.size > 1
            item = args.shift
            if item == '--max-frames'
                @max_frames = args.shift.to_i
            elsif item == '--error-log-size'
                @execution_log_size = args.shift.to_i
            elsif item == '--no-animation'
                @record_frames = false
            else
                STDERR.puts "Invalid argument: #{item}"
                exit(1)
            end
        end

        @profile_path = args.shift

        unless File.exist?(@profile_path)
                STDERR.puts 'Input file not found.'
                exit(1)
            end
        

        @highlight_color = '#fce98d'
        #if @config['highlight']
        #    @highlight_color = @config['highlight']
        #end
        @histogram_color = '#12959f'

        @keywords = Set.new(KEYWORDS.split(/\s/).map { |x| x.strip }.reject { |x| x.empty? })
        @global_variables = {}
        @watches = {}
        @watches_for_index = []
        @label_for_pc = {}
        @pc_for_label = {}
        
    end

    def run
    	Dir::mktmpdir do |temp_dir|
    		@watch_values = {}
            @watch_called_from_subroutine = {}
            start_pc = 0

    		@frame_count = 0
            cycle_count = 0
            last_frame_time = 0
            frame_cycles = []
            @total_cycles_per_function = {}
            @calls_per_function = {}
            @call_graph_counts = {}
            @max_cycle_count = 0
            call_stack = []
            last_call_stack_cycles = 0

            pdtext=File.open(@profile_path).read
            pdtext.each_line do |line|
            	#puts "> #{line}"
            	parts = line.split(' ')
                if parts.first == 'error'
                    parts.shift
                    pc = parts.shift.to_i(16)
                    message = parts.join(' ')
                    @error = {:pc => pc, :message => message}
                elsif parts.first == 'cpu'
                    parts.shift
                    if @execution_log.length == 0
                    	start_pc = parts[2].to_i(16)
	                    puts "Start PC: #{start_pc}"
                    end
                    log = parts.map { |x| x.to_i(16) }
                    @execution_log << log
                    while @execution_log.size > @execution_log_size
                        @execution_log.shift
                    end
                elsif parts.first == 'jsr'
                    pc = parts[1].to_i(16)
                    cycles = parts[2].to_i
                    @max_cycle_count = cycles
                    @calls_per_function[pc] ||= 0
                    @calls_per_function[pc] += 1
                    calling_function = start_pc
                    unless call_stack.empty?
                        calling_function = call_stack.last
                        @total_cycles_per_function[call_stack.last] ||= 0
                        @total_cycles_per_function[call_stack.last] += cycles - last_call_stack_cycles
                    end
                    @call_graph_counts[calling_function] ||= {}
                    @call_graph_counts[calling_function][pc] ||= 0
                    @call_graph_counts[calling_function][pc] += 1
                    last_call_stack_cycles = cycles
                    call_stack << pc
                elsif parts.first == 'rts'
                    cycles = parts[1].to_i
                    @max_cycle_count = cycles
                    last_cycles = @total_cycles_per_function[call_stack.last] || 0
                    unless call_stack.empty?
                        @total_cycles_per_function[call_stack.last] ||= 0
                        @total_cycles_per_function[call_stack.last] += cycles - last_call_stack_cycles
                    end
                    if @cycles_per_function.include?(call_stack.last)
                        @cycles_per_function[call_stack.last] << {
                            :call_cycles => @total_cycles_per_function[call_stack.last] - last_cycles,
                            :at_cycles => cycles
                        }
                    end
                    last_call_stack_cycles = cycles
                    call_stack.pop
                elsif parts.first == 'watch'
                    watch_index = parts[2].to_i
                    cycles = parts[3].to_i
                    @max_cycle_count = cycles
                    @watch_called_from_subroutine[watch_index] ||= Set.new()
                    @watch_called_from_subroutine[watch_index] << parts[1].to_i(16)
                    @watch_values[watch_index] ||= []
                    watch_value_tuple = parts[4, parts.size - 4].map { |x| x.to_i }
                    @watch_values[watch_index] << {:tuple => watch_value_tuple, :cycles => cycles}
                elsif parts.first == 'screen'
                    @frame_count += 1
                    print "\rFrames: #{@frame_count}, Cycles: #{cycle_count}"
                    this_frame_cycles = parts[1].to_i
                    @max_cycle_count = this_frame_cycles
                    frame_cycles << this_frame_cycles
                    if @record_frames
                        data = parts[2, parts.size - 2].map { |x| x.to_i }
                        gi.puts 'l'
                        (0...192).each do |y|
                            (0...280).each do |x|
                                b = (data[y * 40 + (x / 7)] >> (x % 7)) & 1
                                gi.print b
                            end
                            gi.puts
                        end

                        gi.puts "d #{(this_frame_cycles - last_frame_time) / 10000}"
                    end
                    last_frame_time = this_frame_cycles

                    if @max_frames && @frame_count >= @max_frames
                        break
                    end
                elsif parts.first == 'cycles'
                    cycle_count = parts[1].to_i
                    @max_cycle_count = cycle_count
                    print "\rFrames: #{@frame_count}, Cycles: #{cycle_count}"
                end
            end

            #if @record_frames
			#	gi.close
			#	gt.join
            #end

            puts
            
            @cycles_per_frame = []
            (2...frame_cycles.size).each do |i|
                @cycles_per_frame << frame_cycles[i] - frame_cycles[i - 1]
            end

        end
    end

    def print_c(pixels, width, height, x, y, c, color)
        if c.ord >= 0x20 && c.ord < 0x80
            font_index = (c.ord - 0x20) * 5
            (0...5).each do |px|
                (0...7).each do |py|
                    if ((FONT[font_index + px] >> py) & 1) == 1
                        pixels[(y + py) * width + (x + px)] = color
                    end
                end
            end
        end
    end

    def print_c_r(pixels, width, height, x, y, c, color)
        if c.ord >= 0x20 && c.ord < 0x80
            font_index = (c.ord - 0x20) * 5
            (0...5).each do |px|
                (0...7).each do |py|
                    if ((FONT[font_index + px] >> (6 - py)) & 1) == 1
                        pixels[(y + px) * width + (x + py)] = color
                    end
                end
            end
        end
    end

    def print_s(pixels, width, height, x, y, s, color)
        s.each_char.with_index do |c, i|
            print_c(pixels, width, height, x + i * 6, y, c, color)
        end
    end

    def print_s_r(pixels, width, height, x, y, s, color)
        s.each_char.with_index do |c, i|
            print_c_r(pixels, width, height, x, y + i * 6, c, color)
        end
    end

    def write_report
        html_name = 'report.html'
        print "Writing report to file://#{File.absolute_path(html_name)} ..."
        File::open(html_name, 'w') do |f|
            report = DATA.read

            # write frames
            io = StringIO.new
            if @record_frames
                io.puts "<img class='screenshot' src='#{File.join(@files_dir, 'frames.gif')}' /><br />"
            end
            if @cycles_per_frame.size > 0
                io.puts '<p>'
                io.puts "Frames recorded: #{@frame_count}<br />"
                io.puts "Average cycles/frame: #{@cycles_per_frame.inject(0) { |sum, x| sum + x } / @cycles_per_frame.size}<br />"
                io.puts '<p>'
            end
            report.sub!('#{screenshots}', io.string)

            # write watches
            io = StringIO.new
            @watches_for_index.each.with_index do |watch, index|
                io.puts "<div style='display: inline-block;'>"
                if @watch_values.include?(index) || @cycles_per_function.include?(watch[:pc])
                    pixels = nil
                    width = nil
                    height = nil
                    histogram = {}
                    histogram_x = {}
                    histogram_y = {}
                    mask = [
                        [0,0,1,1,1,0,0],
                        [0,1,1,1,1,1,0],
                        [1,1,1,1,1,1,1],
                        [1,1,1,1,1,1,1],
                        [1,1,1,1,1,1,1],
                        [0,1,1,1,1,1,0],
                        [0,0,1,1,1,0,0]
                    ]
                    if @watch_values.include?(index)
                        @watch_values[index].each do |item|
                            normalized_item = []
                            if item[:tuple].size == 1
                                normalized_item << (item[:cycles] * 255 / @max_cycle_count).to_i
                            end
                            item[:tuple].each.with_index do |x, i|
                                if watch[:components][i][:type] == 's8'
                                    x += 128
                                elsif watch[:components][i][:type] == 'u16'
                                    x >>= 8
                                elsif watch[:components][i][:type] == 's16'
                                    x = (x + 32768) >> 8
                                end
                                normalized_item << x
                            end
                            offset = normalized_item.reverse.inject(0) { |x, y| (x << 8) + y }
                            histogram[offset] ||= 0
                            histogram[offset] += 1
                            histogram_x[normalized_item[0]] ||= 0
                            histogram_x[normalized_item[0]] += 1
                            histogram_y[normalized_item[1]] ||= 0
                            histogram_y[normalized_item[1]] += 1
                        end
                    else
                        max_cycle_count_for_function = @cycles_per_function[watch[:pc]].map do |x|
                            x[:call_cycles]
                        end.max
                        @cycles_per_function[watch[:pc]].each do |entry|
                            normalized_item = []
                            normalized_item << (entry[:at_cycles] * 255 / @max_cycle_count).to_i
                            normalized_item << (entry[:call_cycles] * 255 / max_cycle_count_for_function).to_i
                            offset = normalized_item.reverse.inject(0) { |x, y| (x << 8) + y }
                            histogram[offset] ||= 0
                            histogram[offset] += 1
                            histogram_x[normalized_item[0]] ||= 0
                            histogram_x[normalized_item[0]] += 1
                            histogram_y[normalized_item[1]] ||= 0
                            histogram_y[normalized_item[1]] += 1
                        end
                    end

                    histogram_max = histogram.values.max
                    histogram_x_max = histogram_x.values.max
                    histogram_y_max = histogram_y.values.max
                    canvas_width = 200
                    canvas_height = 200
                    histogram_height = 32
                    canvas_top = 10 + histogram_height
                    canvas_left = 30
                    canvas_right = 10 + histogram_height
                    canvas_bottom = 50
                    width = canvas_width + canvas_left + canvas_right
                    height = canvas_height + canvas_top + canvas_bottom
                    pixels = [0] * width * height

                    histogram.each_pair do |key, value|
                        x = key & 0xff;
                        y = ((key >> 8) & 0xff) ^ 0xff
                        x = (x * canvas_width) / 255 + canvas_left
                        y = (y * canvas_height) / 255 + canvas_top
                        (0..6).each do |dy|
                            py = y + dy - 3
                            if py >= 0 && py < height
                                (0..6).each do |dx|
                                    next if mask[dy][dx] == 0
                                    px = x + dx - 3
                                    if px >= 0 && px < width
                                        if pixels[py * width + px] == 0
                                            pixels[py * width + px] = 1
                                        end
                                    end
                                end
                            end
                        end
                        pixels[y * width + x] = (((value.to_f / histogram_max) ** 0.5) * 63).to_i
                    end
                    if watch[:components].size > 1
                        # only show X histogram if it's not cycles
                        histogram_x.each_pair do |x, value|
                            x = (x * canvas_width) / 255 + canvas_left
                            normalized_value = (value.to_f / histogram_x_max * 31).to_i
                            (0..normalized_value).each do |dy|
                                pixels[(canvas_top - dy - 4) * width + x] = normalized_value - dy + 0x40
                            end
                        end
                    end
                    histogram_y.each_pair do |y, value|
                        y = ((y ^ 0xff) * canvas_height) / 255 + canvas_top
                        normalized_value = (value.to_f / histogram_y_max * 31).to_i
                        (0..normalized_value).each do |dx|
                            pixels[y * width + canvas_left + canvas_width + dx + 4] |= normalized_value - dx + 0x40
                        end
                    end

                    watch[:components].each.with_index do |component, component_index|
                        labels = []
                        if component[:type] == 'u8'
                            labels << [0.0, '0']
                            labels << [64.0/255, '64']
                            labels << [128.0/255, '128']
                            labels << [192.0/255, '192']
                            labels << [1.0, '255']
                        elsif component[:type] == 's8'
                            labels << [0.0, '-128']
                            labels << [64.0/255, '-64']
                            labels << [128.0/255, '0']
                            labels << [192.0/255, '64']
                            labels << [1.0, '127']
                        elsif component[:type] == 'u16'
                            labels << [0.0, '0']
                            labels << [64.0/255, '16k']
                            labels << [128.0/255, '32k']
                            labels << [192.0/255, '48k']
                            labels << [1.0, '64k']
                        elsif component[:type] == 's16'
                            labels << [0.0, '-32k']
                            labels << [64.0/255, '-16k']
                            labels << [128.0/255, '0']
                            labels << [192.0/255, '16k']
                            labels << [1.0, '32k']
                        end
                        labels.each do |label|
                            s = label[1]
                            if component_index == 0 && watch[:components].size == 2
                                x = (label[0] * canvas_width).to_i + canvas_left
                                print_s(pixels, width, height,
                                        (x - s.size * (6 * label[0])).to_i,
                                        canvas_top + canvas_height + 7, s, 31)
                                (0..(canvas_height + 3)).each do |y|
                                    pixels[(y + canvas_top) * width + x] |= 0x20
                                end
                            else
                                y = ((1.0 - label[0]) * canvas_height).to_i + canvas_top
                                print_s_r(pixels, width, height, canvas_left - 12,
                                            (y - s.size * (6 * (1.0 - label[0]))).to_i, s, 31)
                                (-3..canvas_width).each do |x|
                                    pixels[y * width + (x + canvas_left)] |= 0x20
                                end
                            end
                        end
                        (0..0).each do |offset|
                            component_label = component[:name]
                            if component_index == 0 && watch[:components].size == 2
                                print_s(pixels, width, height,
                                        (canvas_left + canvas_width * 0.5 - component_label.size * 3 + offset).to_i,
                                        canvas_top + canvas_height + 18,
                                        component_label, 31)
                            else
                                print_s_r(pixels, width, height,
                                            canvas_left - 22,
                                            (canvas_top + canvas_height * 0.5 - component_label.size * 3 + offset).to_i,
                                            component_label, 31)
                            end
                        end
                    end
                    label = "#{sprintf('%04X', watch[:pc])} / #{watch[:path]}:#{watch[:line_number]}"
                    if @watch_values.include?(index)
                        label += " (#{watch[:post] ? 'post' : 'pre'})"
                    end
                    print_s(pixels, width, height, width / 2 - 3 * label.size, height - 20, label, 31)
                    if @watch_values.include?(index)
                        label = @watch_called_from_subroutine[index].map do |x|
                            "#{@label_for_pc[x] || sprintf('%04X', x)}+#{watch[:pc] - x}"
                        end.join(', ')
                        label = "at #{label}"
                        print_s(pixels, width, height, width / 2 - 3 * label.size, height - 10, label, 31)
                    end
                    
                    if watch[:components].size == 1
                        # this watch is 1D, add X axis labels for cycles
                        labels = []
                        labels << [0.0, '0']
                        format_str = '%d'
                        divisor = 1
                        if @max_cycle_count >= 1e6
                            format_str = '%1.1fM'
                            divisor = 1e6
                        elsif @max_cycle_count > 1e3
                            format_str = '%1.1fk'
                            divisor = 1e3
                        end
                            
#                         labels << [1.0, sprintf(format_str, (@max_cycle_count.to_f / divisor)).sub('.0', '')]
                        
                        remaining_space = canvas_width - labels.inject(0) { |a, b| a + b.size * 6 }
                        space_per_label = sprintf(format_str, (@max_cycle_count.to_f / divisor)).sub('.0', '').size * 6 * 2
                        max_tween_labels = remaining_space / space_per_label
                        step = ((@max_cycle_count / max_tween_labels).to_f / divisor).ceil
                        step = 1 if step == 0 # prevent infinite loop!
                        x = step
                        while x < @max_cycle_count / divisor
                            labels << [x.to_f * divisor / @max_cycle_count, sprintf(format_str, x).sub('.0', '')]
                            x += step
                        end
                        labels.each do |label|
                            s = label[1]
                            x = (label[0] * canvas_width).to_i + canvas_left
                            print_s(pixels, width, height,
                                    (x - s.size * (6 * label[0])).to_i,
                                    canvas_top + canvas_height + 7, s, 31)
                            (0..(canvas_height + 3)).each do |y|
                                pixels[(y + canvas_top) * width + x] |= 0x20
                            end
                        end
                        
                        (0..0).each do |offset|
                            component_label = 'cycles'
                            print_s(pixels, width, height,
                                    (canvas_left + canvas_width * 0.5 - component_label.size * 3 + offset).to_i,
                                    canvas_top + canvas_height + 18,
                                    component_label, 31)
                        end
                    end

                    if (!@watch_values.include?(index)) && @cycles_per_function.include?(watch[:pc]) && (!@cycles_per_function[watch[:pc]].empty?)
                        max_cycle_count_for_function = @cycles_per_function[watch[:pc]].map do |x|
                            x[:call_cycles]
                        end.max
                        # this is a subroutine cycles watch, add Y axis labels for subroutine cycles
                        labels = []
                        labels << [0.0, '0']
                        format_str = '%d'
                        divisor = 1
                        if max_cycle_count_for_function >= 1e6
                            format_str = '%1.1fM'
                            divisor = 1e6
                        elsif max_cycle_count_for_function > 1e3
                            format_str = '%1.1fk'
                            divisor = 1e3
                        end
                            
#                         labels << [1.0, sprintf(format_str, (max_cycle_count_for_function.to_f / divisor)).sub('.0', '')]
                        
                        remaining_space = canvas_width - labels.inject(0) { |a, b| a + b.size * 6 }
                        space_per_label = sprintf(format_str, (max_cycle_count_for_function.to_f / divisor)).sub('.0', '').size * 6 * 2
                        max_tween_labels = remaining_space / space_per_label
                        step = ((max_cycle_count_for_function / max_tween_labels).to_f / divisor).ceil
                        step = 1 if step == 0 # prevent infinite loop!
                        x = step
                        while x < max_cycle_count_for_function / divisor
                            labels << [x.to_f * divisor / max_cycle_count_for_function, sprintf(format_str, x).sub('.0', '')]
                            x += step
                        end
                        labels.each do |label|
                            s = label[1]
                            y = ((1.0 - label[0]) * canvas_height).to_i + canvas_top
                            print_s_r(pixels, width, height, canvas_left - 12,
                                        (y - s.size * (6 * (1.0 - label[0]))).to_i, s, 31)
                            (-3..canvas_width).each do |x|
                                pixels[y * width + (x + canvas_left)] |= 0x20
                            end
                        end
                    end
                    
                    tr = @highlight_color[1, 2].to_i(16)
                    tg = @highlight_color[3, 2].to_i(16)
                    tb = @highlight_color[5, 2].to_i(16)
                    hr = @histogram_color[1, 2].to_i(16)
                    hg = @histogram_color[3, 2].to_i(16)
                    hb = @histogram_color[5, 2].to_i(16)

                    if pixels
                        colors_used = 32 * 3
                        gi, go, gt = Open3.popen2("./pgif #{width} #{height} #{colors_used}")
                        palette = [0] * colors_used
                        (0...32).each do |i|
                            if (i == 0)
                                r = 0xff
                                g = 0xff
                                b = 0xff
                            else
                                l = (((31 - i) + 1) << 3) - 1
                                r = l * tr / 255
                                g = l * tg / 255
                                b = l * tb / 255
                            end
                            palette[i] = sprintf('%02x%02x%02x', r, g, b)
                            r = r * 4 / 5
                            g = g * 4 / 5
                            b = b * 4 / 5
                            palette[i + 32] = sprintf('%02x%02x%02x', r, g, b)
                            fade = (i.to_f / 31) * 0.5 + 0.5
                            xr = (hr * fade + 0xff * (1.0 - fade)).to_i
                            xg = (hg * fade + 0xff * (1.0 - fade)).to_i
                            xb = (hb * fade + 0xff * (1.0 - fade)).to_i
                            palette[i + 64] = sprintf('%02x%02x%02x', xr, xg, xb)
                        end
                        gi.puts palette.join("\n")
                        gi.puts 'f'
                        gi.puts pixels.map { |x| sprintf('%02x', x) }.join("\n")
                        gi.close
                        gt.join
                        watch_path = File.join(@files_dir, "watch_#{index}.gif")
                        File::open(watch_path, 'w') do |f|
                            f.write go.read
                        end
                        io.puts "<img src='#{watch_path}'></img>"
                    end
                else
                    io.puts "<em>No values recorded.</em>"
                end
                io.puts "</div>"
            end
            report.sub!('#{watches}', io.string)
            if @cycles_per_function.empty?
                report.sub!('#{cycle_watches}', '')
            else
                io = StringIO.new
                report.sub!('#{cycle_watches}', io.string)
            end

            # write cycles
            io = StringIO.new
            io.puts "<table>"
            io.puts "<thead>"
            io.puts "<tr>"
            io.puts "<th>Addr</th>"
            io.puts "<th>CC</th>"
            io.puts "<th>CC %</th>"
            io.puts "<th>Calls</th>"
            io.puts "<th>CC/Call</th>"
            io.puts "<th>Label</th>"
            io.puts "</tr>"
            io.puts "</thead>"
            cycles_sum = @total_cycles_per_function.values.inject(0) { |a, b| a + b }
            @total_cycles_per_function.keys.sort do |a, b|
                @total_cycles_per_function[b] <=> @total_cycles_per_function[a]
            end.each do |pc|
                io.puts "<tr>"
                io.puts "<td>#{sprintf('%04X', pc)}</td>"
                io.puts "<td style='text-align: right;'>#{@total_cycles_per_function[pc]}</td>"
                io.puts "<td style='text-align: right;'>#{sprintf('%1.2f%%', @total_cycles_per_function[pc].to_f * 100.0 / cycles_sum)}</td>"
                io.puts "<td style='text-align: right;'>#{@calls_per_function[pc]}</td>"
                io.puts "<td style='text-align: right;'>#{@total_cycles_per_function[pc] / @calls_per_function[pc]}</td>"
                io.puts "<td>#{@label_for_pc[pc]}</td>"
                io.puts "</tr>"

            end
            io.puts "</table>"
            report.sub!('#{cycles}', io.string)

            if @have_dot
                # render call graph
                all_nodes = Set.new()
                @call_graph_counts.each_pair do |key, entries|
                    all_nodes << key
                    entries.keys.each do |other_key|
                        all_nodes << other_key
                    end
                end
                io = StringIO.new
                io.puts "digraph {"
                io.puts "overlap = false;"
                io.puts "rankdir = LR;"
                io.puts "splines = true;"
                io.puts "graph [fontname = Arial, fontsize = 8, size = \"14, 11\", nodesep = 0.2, ranksep = 0.3, ordering = out];"
                io.puts "node [fontname = Arial, fontsize = 8, shape = rect, style = filled, fillcolor = \"#fce94f\" color = \"#c4a000\"];"
                io.puts "edge [fontname = Arial, fontsize = 8, color = \"#444444\"];"
                all_nodes.each do |node|
                    label = @label_for_pc[node] || sprintf('%04X', node)
                    label = "<B>#{label}</B>"
                    if @calls_per_function[node] && @total_cycles_per_function[node]
                        label += "<BR/>#{@total_cycles_per_function[node] / @calls_per_function[node]}"
                    end
                    io.puts "  _#{node} [label = <#{label}>];"
                end
                max_call_count = 0
                @call_graph_counts.each_pair do |key, entries|
                    entries.values.each do |count|
                        max_call_count = count if count > max_call_count
                    end
                end

                @call_graph_counts.each_pair do |key, entries|
                    entries.keys.each do |other_key|
                        penwidth = 0.5 + ((entries[other_key].to_f / max_call_count) ** 0.3) * 2
                        io.puts "_#{key} -> _#{other_key} [label = \"#{entries[other_key]}x\", penwidth = #{penwidth}];"
                    end
                end
                io.puts "}"

                dot = io.string
                
                svg = Open3.popen2('dot -Tsvg') do |stdin, stdout, thread|
                    stdin.print(dot)
                    stdin.close
                    stdout.read
                end
                File::open(File.join(@files_dir, 'call_graph.svg'), 'w') do |f|
                    f.write(svg)
                end
                report.sub!('#{call_graph}', svg)
            else
                report.sub!('#{call_graph}', '<em>(GraphViz not installed)</em>')
            end
            
            # write error dump
            if @error
                io = StringIO.new
                io.puts "<div style='float: left; margin-right: 10px;'>"
                io.puts "<h2>Source code</h2>"
                
                io.puts "<code><pre>"
                source_code = @code_for_pc[@error[:pc]]
                offset = source_code[:line] - 1
                this_filename = source_code[:file]
                format_str = "%-#{@max_source_width_for_file[this_filename]}s"
                io.puts "<span class='heading'> Line |   PC   | #{sprintf(format_str, this_filename)}</span>"
                ((offset - 16)..(offset + 16)).each do |i|
                    next if i < 0 || i >= @source_for_file[this_filename].size
                    io.print "<span class='#{(i == offset) ? 'error' : 'code'}'>"
                    line_pc = nil
                    if @pc_for_file_and_line[this_filename]
                        if @pc_for_file_and_line[this_filename][i + 1]
                            line_pc = sprintf('%04X', @pc_for_file_and_line[this_filename][i + 1])
                        end
                    end
                    line_pc ||= ''
                    io.print sprintf("%5d | %6s | %-#{@max_source_width_for_file[this_filename]}s", i + 1, line_pc, @source_for_file[this_filename][i])
                    io.print "</span>" if i == offset
                    io.puts
                end
                io.puts "</pre></code>"
                io.puts "</div>"
                
                io.puts "<div style='display: inline-block;'>"
                io.puts "<h2>Execution log</h2>"
                io.puts "<code><pre>"
                io.puts sprintf("<span class='heading'>   PC    |    A     X     Y     PC      SP  Flags </span>")
                @execution_log.each do |item|
                    io.puts sprintf("<span class='code'> %04X  |  %02X  %02X  %02X  %04X  %02X  %02X  </span>", *item)
                end
                io.puts sprintf("<span class='error'> %04X  |  %-37s </span>", @error[:pc], @error[:message])
                io.puts "</pre></code>"
                
                io.puts "</div>"
                
                io.puts "<div style='clear: both;'></div>"
                
                report.sub!('#{error}', io.string)
            else
                report.sub!('#{error}', '')
            end
            
            f.puts report
        end
        puts ' done.'
    end
end

champ = Champ.new
champ.run
champ.write_report

__END__

<html>
<head>
    <title>C64 65XE Debugger champ report</title>
    <style type='text/css'>
    body {
        // background-color: #eee;
        font-family: monospace;
    }
    .screenshot {
        background-color: #222;
        box-shadow: inset 0 0 10px rgba(0,0,0,1.0);
        padding: 12px;
        border-radius: 8px;
    }
    img, svg {
        border: 1px solid #ddd;
        border-radius: 8px;
        box-shadow: 0 0 10px rgba(0,0,0,0.2);
        margin-right: 10px;
        margin-bottom: 10px;
    }
    th, td {
        text-align: left;
        padding: 0 0.5em;
    }
    .heading {
        color: #2e3436;
        background-color: #babdb6;
        font-weight: bold;
    }
    .code {
        color: #555753;
        background-color: #edeeec;
    }
    .error {
        color: #cc0000;
        background-color: #f2bfbf;
    }
    </style>
</head>
<body>
#{error}
<div style='float: left; padding-right: 10px;'>
    <h2>Frames</h2>
    #{screenshots}
    <h2>Cycles</h2>
    #{cycles}
</div>
<div style='float: left; padding-right: 10px;'>
    <h2>Call Graph</h2>
    #{call_graph}
</div>
<div style='padding-top: 0.1px;'>
    <h2>Watches</h2>
    #{watches}
    #{cycle_watches}
</div>
</body>
</html>