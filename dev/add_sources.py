import sys
import glob
import os


if __name__ == '__main__':
    tool_path = sys.argv[1]
    disk_path = sys.argv[2]
    for path in glob.glob(f'**/*', recursive=True):
        ext = path.split('.')[-1]
        filename = path.split('/')[-1]
        has_ext = '.' in filename
        if not has_ext:
            ext = filename
        else:
            ext = filename.split('.')[-1]
        if ext in ('cc', 'c', 'h', 'Makefile', 'mk', 'tmpl', 's', 'ld'):
            cmd = f'{tool_path} add {disk_path} /src/{path} ./{path}'
            print(cmd)
            ret = os.system(cmd)
            if ret != 0:
                print(f'Error: {cmd}')
                break
