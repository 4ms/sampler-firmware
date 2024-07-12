#!/usr/bin/python2.5
#
# Copyright 2013 Emilie Gillet.
# 
# Author: Emilie Gillet (emilie.o.gillet@gmail.com)
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
# 
# See http://creativecommons.org/licenses/MIT/ for more information.
#
# -----------------------------------------------------------------------------
#
# Qpsk encoder for converting firmware .bin files into .

import logging
import numpy
import optparse
import zlib

from stm_audio_bootloader import audio_stream_writer


class Scrambler(object):
  
  def __init__(self, seed=0):
    self._state = seed
    
  def scramble(self, data):
    data = map(ord, data)
    for index, byte in enumerate(data):
      data[index] = data[index] ^ (self._state >> 24)
      self._state = (self._state * 1664525 + 1013904223) & 0xffffffff
    return ''.join(map(chr, data))



class QpskEncoder(object):
  
  def __init__(
      self,
      sample_rate=48000,
      carrier_frequency=2000,
      bit_rate=8000,
      packet_size=256,
      scramble=False):
    period = sample_rate / carrier_frequency
    symbol_time = sample_rate / (bit_rate / 2)
    
    assert (symbol_time % period == 0) or (period % symbol_time == 0)
    assert (sample_rate % carrier_frequency) == 0
    assert (sample_rate % bit_rate) == 0
    
    self._sr = sample_rate
    self._br = bit_rate
    self._carrier_frequency = carrier_frequency
    self._sample_index = 0
    self._packet_size = packet_size
    self._scrambler = Scrambler(0) if scramble else None
    
  @staticmethod
  def _upsample(x, factor):
    return numpy.tile(x.reshape(len(x), 1), (1, factor)).ravel()
    
  def _encode_qpsk(self, symbol_stream):
    ratio = int(self._sr / self._br * 2)
    symbol_stream = numpy.array(symbol_stream)
    # print((symbol_stream % 2).astype(int)) #same
    # print((symbol_stream / 2).astype(int)) #same
    bitstream_even = 2 * self._upsample((symbol_stream % 2).astype(int), ratio) - 1
    bitstream_odd = 2 * self._upsample((symbol_stream / 2).astype(int), ratio) - 1
    return bitstream_even / numpy.sqrt(2.0), bitstream_odd / numpy.sqrt(2.0)
  
  def _modulate(self, q_mod, i_mod):
    num_samples = len(q_mod)
    t = (numpy.arange(0.0, num_samples) + self._sample_index) / self._sr
    self._sample_index += num_samples
    phase = 2 * numpy.pi * self._carrier_frequency * t
    return (q_mod * numpy.sin(phase) + i_mod * numpy.cos(phase))

  def _encode(self, symbol_stream):
    return self._modulate(*self._encode_qpsk(symbol_stream))
    
  def _code_blank(self, duration):
    num_zeros = int(duration * self._br / 8) * 4
    symbol_stream = numpy.zeros((num_zeros, 1)).ravel().astype(int)
    return self._encode(symbol_stream)

  def _code_packet(self, data):
    assert len(data) <= self._packet_size
    if len(data) != self._packet_size:
      data = data + '\xff' * (self._packet_size - len(data))
    
    if self._scrambler:
      data = self._scrambler.scramble(data)
    
    crc = zlib.crc32(data) & 0xffffffff

    #data = map(ord, data)
    data = list(data)
    # 16x 0 for the PLL ; 8x 21 for the edge detector ; 8x 3030 for syncing
    preamble = [0] * 8 + [0x99] * 4 + [0xcc] * 4
    crc_bytes = [crc >> 24, (crc >> 16) & 0xff, (crc >> 8) & 0xff, crc & 0xff]
    bytes = preamble + data + crc_bytes
    
    symbol_stream = []
    for byte in bytes:
      symbol_stream.append((byte >> 6) & 0x3)
      symbol_stream.append((byte >> 4) & 0x3)
      symbol_stream.append((byte >> 2) & 0x3)
      symbol_stream.append((byte >> 0) & 0x3)
    
    return self._encode(symbol_stream)
  
  def code_intro(self):
    yield numpy.zeros((1 * self._sr, 1)).ravel()
    yield self._code_blank(1.0)
  
  def code_outro(self, duration=1.0):
    yield self._code_blank(duration)
  
  def code(self, data, page_size=1024, blank_duration=0.06):
    if len(data) % page_size != 0:
      tail = page_size - (len(data) % page_size)
      data += b'\xff' * tail
    
    offset = 0
    remaining_bytes = len(data)
    num_packets_written = 0
    while remaining_bytes:
      size = min(remaining_bytes, self._packet_size)
      yield self._code_packet(data[offset:offset+size])
      num_packets_written += 1
      if num_packets_written == page_size / self._packet_size:
        yield self._code_blank(blank_duration)
        num_packets_written = 0
      remaining_bytes -= size
      offset += size

STM32F4_SECTOR_BASE_ADDRESS = [
  0x08000000,
  0x08004000,
  0x08008000,
  0x0800C000,
  0x08010000,
  0x08020000,
  0x08040000,
  0x08060000,
  0x08080000,
  0x080A0000,
  0x080C0000,
  0x080E0000
]

STM32H7_SECTOR_BASE_ADDRESS = [
  0x08000000,
  0x08020000,
  0x08040000,
  0x08060000,
  0x08080000,
  0x080a0000,
  0x080c0000,
  0x080e0000,
  0x08100000,
  0x08120000,
  0x08140000,
  0x08160000,
  0x08180000,
  0x081a0000,
  0x081c0000,
  0x081e0000,
]

PAGE_SIZE = { 'stm32f1': 1024, 'stm32f3': 2048 }
PAUSE = { 'stm32f1': 0.06, 'stm32f3': 0.15 }

STM32F4_BLOCK_SIZE = 16384
STM32F4_APPLICATION_START = 0x08008000

STM32H7_BLOCK_SIZE = 0x10000
STM32H7_APPLICATION_START = 0x08020000

def main():
  numpy.set_printoptions(threshold=32768)
  parser = optparse.OptionParser()
  parser.add_option(
      '-k',
      '--scramble',
      dest='scramble',
      action='store_true',
      default=False,
      help='Randomize data stream')
  parser.add_option(
      '-s',
      '--sample_rate',
      dest='sample_rate',
      type='int',
      default=48000,
      help='Sample rate in Hz')
  parser.add_option(
      '-c',
      '--carrier_frequency',
      dest='carrier_frequency',
      type='int',
      default=6000,
      help='Carrier frequency in Hz')
  parser.add_option(
      '-b',
      '--baud_rate',
      dest='baud_rate',
      type='int',
      default=12000,
      help='Baudrate in bps')
  parser.add_option(
      '-p',
      '--packet_size',
      dest='packet_size',
      type='int',
      default=256,
      help='Packet size in bytes')
  parser.add_option(
      '-o',
      '--output_file',
      dest='output_file',
      default=None,
      help='Write output file to FILE',
      metavar='FILE')
  parser.add_option(
      '-t',
      '--target',
      dest='target',
      default='stm32f1',
      help='Set page size and erase time for TARGET',
      metavar='TARGET')
  parser.add_option(
      '-a',
      '--start_sector',
      dest='start_sector',
      type='int',
      default=0,
      help='Application starting sector number',
      )
  parser.add_option(
      '-g',
      '--block_size',
      dest='block_size',
      type='int',
      default=0,
      help='Block size',
      ) 
  
  options, args = parser.parse_args()
  # data = file(args[0], 'rb').read()
  with open(args[0], 'rb') as input_file:
      data = input_file.read()
  
  if len(args) != 1:
    logging.fatal('Specify one, and only one firmware .bin file!')
    sys.exit(1)
  
  if options.target not in ['stm32f1', 'stm32f3', 'stm32f4', 'stm32h7']:
    logging.fatal('Unknown target: %s' % options.target)
    sys.exit(2)
  
  output_file = options.output_file
  if not output_file:
    if '.bin' in args[0]:
      output_file = args[0].replace('.bin', '.wav')
    else:
      output_file = args[0] + '.wav'

  encoder = QpskEncoder(
      options.sample_rate,
      options.carrier_frequency,
      options.baud_rate,
      options.packet_size,
      options.scramble)
  writer = audio_stream_writer.AudioStreamWriter(
      output_file,
      options.sample_rate,
      16,
      1)
  
  # INTRO
  for block in encoder.code_intro():
    writer.append(block)
  
  blank_duration = 1.0
  if options.target in PAGE_SIZE.keys():
    for block in encoder.code(data, PAGE_SIZE[options.target], PAUSE[options.target]):
      if len(block):
        writer.append(block)
  elif options.target == 'stm32f4' or options.target == 'stm32h7':
    if options.target == 'stm32f4':
      sector_base = STM32F4_SECTOR_BASE_ADDRESS
      erase_pause = 3.5
    else:
      sector_base = STM32H7_SECTOR_BASE_ADDRESS
      erase_pause = 3.0

    if options.start_sector == 0: #default
      start_address = STM32F4_APPLICATION_START if options.target == 'stm32f4' else STM32H7_APPLICATION_START
    else:
      start_address = sector_base[options.start_sector]

    if options.block_size == 0: #default
      block_size = STM32F4_BLOCK_SIZE if options.target == 'stm32f4' else STM32H7_BLOCK_SIZE
    else:
      block_size = options.block_size

    logging.info(f"Encoding with block_size {block_size}, starting address {hex(start_address)}")
    for x in range(0, len(data), block_size):
      address = start_address + x
      block = data[x:x+block_size]
      pause = erase_pause if address in sector_base else 0.2
      logging.info(f"Block @ {hex(address)}, pause = {pause} ")
      for block in encoder.code(block, block_size, pause):
        if len(block):
          writer.append(block)
    blank_duration = 5.0
  for block in encoder.code_outro(blank_duration):
    writer.append(block)


if __name__ == '__main__':
  main()
