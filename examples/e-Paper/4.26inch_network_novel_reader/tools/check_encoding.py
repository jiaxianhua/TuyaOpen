#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
文件编码检测工具
用于检查文本文件的实际编码格式
"""

import sys
import chardet

def check_file_encoding(filepath):
    """检测文件编码"""
    try:
        # 读取文件的前几KB来检测编码
        with open(filepath, 'rb') as f:
            raw_data = f.read(10240)  # 读取前10KB
        
        # 检测编码
        result = chardet.detect(raw_data)
        
        print(f"文件: {filepath}")
        print(f"检测到的编码: {result['encoding']}")
        print(f"置信度: {result['confidence'] * 100:.1f}%")
        print()
        
        # 尝试用不同编码读取并显示前100个字符
        encodings_to_try = ['utf-8', 'gbk', 'gb2312', 'gb18030', result['encoding']]
        
        for encoding in encodings_to_try:
            if not encoding:
                continue
            try:
                with open(filepath, 'r', encoding=encoding) as f:
                    content = f.read(200)
                print(f"使用 {encoding} 编码读取:")
                print(content[:100])
                print()
            except Exception as e:
                print(f"使用 {encoding} 编码失败: {e}")
                print()
        
        return result['encoding']
        
    except Exception as e:
        print(f"错误: {e}")
        return None

def convert_to_gbk(input_file, output_file=None):
    """转换文件为 GBK 编码"""
    if not output_file:
        output_file = input_file.replace('.txt', '_gbk.txt')
    
    try:
        # 检测原始编码
        with open(input_file, 'rb') as f:
            raw_data = f.read()
        
        result = chardet.detect(raw_data)
        source_encoding = result['encoding']
        
        print(f"源文件编码: {source_encoding}")
        
        # 读取并转换
        with open(input_file, 'r', encoding=source_encoding) as f:
            content = f.read()
        
        # 写入 GBK
        with open(output_file, 'w', encoding='gbk') as f:
            f.write(content)
        
        print(f"已转换为 GBK 编码: {output_file}")
        
        # 验证
        with open(output_file, 'rb') as f:
            verify_data = f.read(1024)
        verify_result = chardet.detect(verify_data)
        print(f"验证编码: {verify_result['encoding']}")
        
    except Exception as e:
        print(f"转换失败: {e}")

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("用法:")
        print("  检测编码: python3 check_encoding.py <文件路径>")
        print("  转换为GBK: python3 check_encoding.py <文件路径> --convert")
        sys.exit(1)
    
    filepath = sys.argv[1]
    
    if len(sys.argv) > 2 and sys.argv[2] == '--convert':
        convert_to_gbk(filepath)
    else:
        check_file_encoding(filepath)
