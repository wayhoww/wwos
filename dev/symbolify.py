import re
import sys
import subprocess



def resolve_symbols(symbol_dict: dict):
    for line in sys.stdin:
        line = line.rstrip('\n')
        hex_numbers = re.findall(r'(0x)?([0-9a-f]+)', line)
        hex_numbers = [x[1] for x in hex_numbers]
        for hex_num in hex_numbers:
            if hex_num in symbol_dict:
                line += f' | {hex_num} -> {symbol_dict[hex_num]}'
        print(line)


def construct_symbol_dict(symbol_files: list[str]):
    out = dict()
    for file in symbol_files:
        most_recent_symbol = '<none>'
        most_recent_symbol_addr = ''
        counter = 0

        result = subprocess.run(['objdump', '-C', '-d', file], stdout=subprocess.PIPE)
        assert result.returncode == 0
        result = result.stdout.decode('utf-8').split('\n')
        for line in result:
            line = line.strip()
            if re.match(r'^[0-9a-f]+ <.*>:', line):
                address = re.search(r'^([0-9a-f]+)\s', line).group(1)
                most_recent_symbol = re.search(r'<(.+)>:', line).group(1)
                most_recent_symbol_addr = address
                out[address] = most_recent_symbol
                counter = 0
                continue
            if re.match(r'^[0-9a-f]+: ', line):
                address = re.search(r'^([0-9a-f]+):', line).group(1)
                if len(address) != len(most_recent_symbol_addr):
                    address = most_recent_symbol_addr[:len(most_recent_symbol_addr) - len(address)] + address
                out[f'{address}'] = most_recent_symbol + f'+{counter}'
                counter += 1
    return out


def add_offset(symbol_dict: dict, offset: int):
    all_keys = list(symbol_dict.keys())
    for key in all_keys:
        symbol_dict[hex(int(key, 16) + offset)[2:]] = symbol_dict[key]

def shrink_symbol_dict(symbol_dict: dict):
    all_keys = list(symbol_dict.keys())
    for key in all_keys:
        # remove preceding zeros
        okey = key
        key = key.lstrip('0')
        symbol_dict[key] = symbol_dict[okey]
        # if length < 8, add preceding zeros
        if len(key) < 8:
            symbol_dict[key.zfill(8)] = symbol_dict[okey]
        

def main():
    symbols = construct_symbol_dict(sys.argv[2:])
    add_offset(symbols, int(sys.argv[1], 16))
    shrink_symbol_dict(symbols)
    resolve_symbols(symbols)

if __name__ == '__main__':
    main()