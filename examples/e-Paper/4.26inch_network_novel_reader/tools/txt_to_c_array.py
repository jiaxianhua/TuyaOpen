#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
将 txt 文件转换为 C 语言数组
支持截取前 N 个字节以适应固件大小限制
"""

import sys
import os

def txt_to_c_array(input_file, output_file, max_size=100000):
    """
    将 txt 文件转换为 C 数组
    
    Args:
        input_file: 输入的 txt 文件路径
        output_file: 输出的 .c 文件路径
        max_size: 最大字节数（默认 100KB）
    """
    # 读取文件内容
    with open(input_file, 'rb') as f:
        content = f.read(max_size)
    
    file_size = len(content)
    
    # 生成 C 数组
    array_name = "embedded_novel_data"
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("/**\n")
        f.write(" * @file embedded_novel.c\n")
        f.write(" * @brief Embedded novel content\n")
        f.write(f" * \n")
        f.write(f" * Source: {os.path.basename(input_file)}\n")
        f.write(f" * Size: {file_size} bytes\n")
        f.write(" */\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"const uint8_t {array_name}[] = {{\n")
        
        # 每行16个字节
        for i in range(0, file_size, 16):
            f.write("    ")
            chunk = content[i:i+16]
            for byte in chunk:
                f.write(f"0x{byte:02x}, ")
            f.write("\n")
        
        f.write("};\n\n")
        f.write(f"const uint32_t {array_name}_size = {file_size};\n")
    
    # 生成头文件
    header_file = output_file.replace('.c', '.h')
    with open(header_file, 'w', encoding='utf-8') as f:
        f.write("/**\n")
        f.write(" * @file embedded_novel.h\n")
        f.write(" * @brief Embedded novel content header\n")
        f.write(" */\n\n")
        f.write("#ifndef __EMBEDDED_NOVEL_H__\n")
        f.write("#define __EMBEDDED_NOVEL_H__\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"extern const uint8_t {array_name}[];\n")
        f.write(f"extern const uint32_t {array_name}_size;\n\n")
        f.write("#endif /* __EMBEDDED_NOVEL_H__ */\n")
    
    print(f"✓ 转换完成")
    print(f"  输入文件: {input_file}")
    print(f"  输出文件: {output_file}")
    print(f"  头文件: {header_file}")
    print(f"  数据大小: {file_size} bytes ({file_size/1024:.1f} KB)")
    if file_size >= max_size:
        print(f"  ⚠ 注意: 文件被截断到 {max_size} 字节")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("用法: python txt_to_c_array.py <input.txt> [max_size_kb]")
        print("示例: python txt_to_c_array.py 乱世为王.txt 100")
        sys.exit(1)
    
    input_file = sys.argv[1]
    max_size_kb = int(sys.argv[2]) if len(sys.argv) > 2 else 100
    max_size = max_size_kb * 1024
    
    if not os.path.exists(input_file):
        print(f"错误: 文件不存在: {input_file}")
        sys.exit(1)
    
    # 输出到 examples 目录
    output_file = os.path.join(
        os.path.dirname(input_file),
        "examples",
        "embedded_novel.c"
    )
    
    txt_to_c_array(input_file, output_file, max_size)
