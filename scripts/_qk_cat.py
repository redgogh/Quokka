import sys
import os
import pyperclip

file_name = sys.argv[1]

output = [f'{os.path.basename(file_name)}.h:\n']

with open(f'{file_name}.h', 'r', encoding='UTF-8') as head:
    output.append(head.read())

output.append('')

output.append(f'{os.path.basename(file_name)}.cpp:\n')
with open(f'{file_name}.cpp', 'r', encoding='UTF-8') as source:
    output.append(source.read())

text = ''.join(output)

pyperclip.copy(text)