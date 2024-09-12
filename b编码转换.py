import os
import chardet

def detect_encoding(file_path):
    with open(file_path, 'rb') as f:
        raw_data = f.read()
    result = chardet.detect(raw_data)
    return result['encoding']

def convert_to_utf8(file_path):
    with open(file_path, 'rb') as f:
        content = f.read()

    detected_encoding = detect_encoding(file_path)
    if detected_encoding and detected_encoding.lower() in ['gb2312', 'gbk', 'gb18030']:
        try:
            decoded_content = content.decode(detected_encoding)
            decoded_content = decoded_content.replace('\r\n', '\n')
            with open(file_path, 'w', encoding='utf-8', newline='\n') as f:
                f.write(decoded_content)
            print(f"Converted {file_path} from {detected_encoding} to UTF-8.")
        except UnicodeDecodeError:
            print(f"Error decoding {file_path} with detected encoding {detected_encoding}.")
    # else:
    #     print(f"Skipped {file_path}, not GB2312/GBK/GB18030 encoded.")

def traverse_and_convert(root_dir):
    for root, _, files in os.walk(root_dir):
        print(f" handl root :" + root)
        for file in files:
            try:
                if file.endswith('.c') or file.endswith('.h') or file.endswith('.txt'):  # 根据需要调整文件类型
                    file_path = os.path.join(root, file)
                    convert_to_utf8(file_path)
                    print(f" convert over :" + file_path)
            except Exception as e:
                print(f"traverse_and_convert" + str(e))

if __name__ == "__main__":
    source_directory = os.getcwd()  # 替换为源码目录的路径
    traverse_and_convert(source_directory)
    print(f" finished")
    while(1):
        pass
