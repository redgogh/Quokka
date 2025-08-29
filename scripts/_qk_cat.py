import sys
import os
import pyperclip
from pathlib import Path

argv1 = sys.argv[1]

output = []

def write_to_output(path):
    output.append(f'\n---> {os.path.basename(path)} <---\n\n')
    if Path(path).exists():
        with open(path, 'r', encoding='UTF-8') as fd:
            output.append(fd.read())

path = Path(argv1)

if path.suffix:
    write_to_output(argv1)
else:
    write_to_output(f'{argv1}.h')
    write_to_output(f'{argv1}.cpp')

text = ''.join(output)

pyperclip.copy(text)

print(text)