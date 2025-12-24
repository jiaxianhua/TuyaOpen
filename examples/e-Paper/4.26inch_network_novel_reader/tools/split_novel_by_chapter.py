#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
小说章节分割工具
按章节将大文件分割成多个小文件，每个文件不超过指定大小
"""

import re
import sys
import os

def detect_encoding(filepath):
    """检测文件编码"""
    try:
        import chardet
        with open(filepath, 'rb') as f:
            raw_data = f.read(10240)
        result = chardet.detect(raw_data)
        return result['encoding']
    except ImportError:
        # 如果没有 chardet，尝试常见编码
        for encoding in ['utf-8', 'gbk', 'gb2312', 'gb18030']:
            try:
                with open(filepath, 'r', encoding=encoding) as f:
                    f.read(1024)
                return encoding
            except:
                continue
        return 'utf-8'  # 默认

def split_by_chapter(input_file, output_prefix=None, max_size_mb=5, output_encoding='gbk'):
    """
    按章节分割小说
    
    Args:
        input_file: 输入文件路径
        output_prefix: 输出文件前缀（默认使用输入文件名）
        max_size_mb: 每个文件最大大小（MB）
        output_encoding: 输出编码（默认 GBK）
    """
    
    if not output_prefix:
        output_prefix = os.path.splitext(input_file)[0]
    
    # 检测输入文件编码
    input_encoding = detect_encoding(input_file)
    print(f"检测到输入文件编码: {input_encoding}")
    
    # 读取文件
    try:
        with open(input_file, 'r', encoding=input_encoding) as f:
            content = f.read()
    except Exception as e:
        print(f"读取文件失败: {e}")
        print("尝试使用 GBK 编码...")
        with open(input_file, 'r', encoding='gbk', errors='ignore') as f:
            content = f.read()
    
    print(f"文件总大小: {len(content)} 字符")
    
    # 查找章节标记（支持多种格式）
    chapter_patterns = [
        r'第[零一二三四五六七八九十百千万\d]+章',  # 第X章
        r'第[零一二三四五六七八九十百千万\d]+回',  # 第X回
        r'Chapter\s+\d+',  # Chapter X
        r'CHAPTER\s+\d+',  # CHAPTER X
        r'\n\d+\.',  # 1. 2. 3.
    ]
    
    # 尝试每个模式
    chapters = None
    used_pattern = None
    for pattern in chapter_patterns:
        test_chapters = re.split(f'({pattern})', content)
        if len(test_chapters) > 3:  # 至少找到一个章节
            chapters = test_chapters
            used_pattern = pattern
            break
    
    if not chapters or len(chapters) <= 1:
        print("警告: 未找到章节标记，将按大小分割")
        # 按固定大小分割
        split_by_size(content, output_prefix, max_size_mb, output_encoding)
        return
    
    print(f"找到 {(len(chapters) - 1) // 2} 个章节（使用模式: {used_pattern}）")
    
    # 按章节分组
    current_file = []
    current_size = 0
    file_num = 1
    max_size = max_size_mb * 1024 * 1024
    chapter_count = 0
    start_chapter = 1
    
    # 处理第一部分（章节前的内容）
    if chapters[0].strip():
        current_file.append(chapters[0])
        current_size = len(chapters[0].encode(output_encoding))
    
    # 处理章节
    for i in range(1, len(chapters), 2):
        if i + 1 < len(chapters):
            chapter_title = chapters[i]
            chapter_content = chapters[i + 1]
            chapter_text = chapter_title + chapter_content
        else:
            chapter_text = chapters[i]
        
        chapter_bytes = len(chapter_text.encode(output_encoding))
        chapter_count += 1
        
        # 检查是否需要新文件
        if current_size + chapter_bytes > max_size and current_file:
            # 保存当前文件
            output_file = f"{output_prefix}_{file_num:02d}.txt"
            with open(output_file, 'w', encoding=output_encoding) as f:
                f.write(''.join(current_file))
            
            file_size_mb = current_size / 1024 / 1024
            print(f"✓ 创建: {output_file} ({file_size_mb:.2f} MB, 章节 {start_chapter}-{chapter_count})")
            
            current_file = []
            current_size = 0
            file_num += 1
            start_chapter = chapter_count + 1
        
        current_file.append(chapter_text)
        current_size += chapter_bytes
    
    # 保存最后一个文件
    if current_file:
        output_file = f"{output_prefix}_{file_num:02d}.txt"
        with open(output_file, 'w', encoding=output_encoding) as f:
            f.write(''.join(current_file))
        
        file_size_mb = current_size / 1024 / 1024
        print(f"✓ 创建: {output_file} ({file_size_mb:.2f} MB, 章节 {start_chapter}-{chapter_count})")
    
    print(f"\n完成! 共创建 {file_num} 个文件")

def split_by_size(content, output_prefix, max_size_mb, output_encoding='gbk'):
    """按固定大小分割（当找不到章节时使用）"""
    
    max_size = max_size_mb * 1024 * 1024
    file_num = 1
    start = 0
    
    while start < len(content):
        # 计算这个块的大小
        end = start
        current_size = 0
        
        while end < len(content) and current_size < max_size:
            char = content[end]
            char_bytes = len(char.encode(output_encoding))
            if current_size + char_bytes > max_size:
                break
            current_size += char_bytes
            end += 1
        
        # 尝试在段落边界分割
        if end < len(content):
            # 向后查找段落结束（双换行）
            for i in range(end, max(start, end - 1000), -1):
                if content[i:i+2] == '\n\n':
                    end = i + 2
                    break
        
        # 保存这个块
        chunk = content[start:end]
        output_file = f"{output_prefix}_{file_num:02d}.txt"
        
        with open(output_file, 'w', encoding=output_encoding) as f:
            f.write(chunk)
        
        file_size_mb = len(chunk.encode(output_encoding)) / 1024 / 1024
        print(f"✓ 创建: {output_file} ({file_size_mb:.2f} MB)")
        
        start = end
        file_num += 1
    
    print(f"\n完成! 共创建 {file_num - 1} 个文件")

def main():
    if len(sys.argv) < 2:
        print("小说章节分割工具")
        print()
        print("用法:")
        print("  python3 split_novel_by_chapter.py <输入文件> [输出前缀] [最大大小MB] [输出编码]")
        print()
        print("参数:")
        print("  输入文件    - 要分割的小说文件")
        print("  输出前缀    - 输出文件名前缀（默认使用输入文件名）")
        print("  最大大小MB  - 每个文件最大大小，单位 MB（默认 5）")
        print("  输出编码    - 输出文件编码（默认 gbk）")
        print()
        print("示例:")
        print("  python3 split_novel_by_chapter.py novel.txt")
        print("  python3 split_novel_by_chapter.py novel.txt novel 10")
        print("  python3 split_novel_by_chapter.py novel.txt novel 5 gbk")
        print()
        sys.exit(1)
    
    input_file = sys.argv[1]
    
    if not os.path.exists(input_file):
        print(f"错误: 文件不存在: {input_file}")
        sys.exit(1)
    
    output_prefix = sys.argv[2] if len(sys.argv) > 2 else None
    max_size_mb = int(sys.argv[3]) if len(sys.argv) > 3 else 5
    output_encoding = sys.argv[4] if len(sys.argv) > 4 else 'gbk'
    
    print(f"输入文件: {input_file}")
    print(f"输出前缀: {output_prefix or os.path.splitext(input_file)[0]}")
    print(f"最大大小: {max_size_mb} MB")
    print(f"输出编码: {output_encoding}")
    print()
    
    try:
        split_by_chapter(input_file, output_prefix, max_size_mb, output_encoding)
    except Exception as e:
        print(f"错误: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == '__main__':
    main()
