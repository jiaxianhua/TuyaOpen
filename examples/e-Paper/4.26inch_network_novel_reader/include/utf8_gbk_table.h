/**
 * @file utf8_gbk_table.h
 * @brief UTF-8 to GBK encoding conversion table
 * 
 * This file contains a mapping table for converting UTF-8 encoded Chinese characters
 * to GBK encoding. Due to memory constraints, only the most common characters are included.
 * 
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#ifndef UTF8_GBK_TABLE_H
#define UTF8_GBK_TABLE_H

#include <stdint.h>

/**
 * @brief UTF-8 to GBK mapping entry
 */
typedef struct {
    uint32_t utf8_code;  // UTF-8 code point (up to 3 bytes for Chinese)
    uint16_t gbk_code;   // GBK code (2 bytes)
} utf8_gbk_map_t;

/**
 * @brief UTF-8 to GBK mapping table
 * 
 * This table contains the most common Chinese characters.
 * Format: {UTF-8 code, GBK code}
 * 
 * UTF-8 encoding for Chinese characters (3 bytes):
 * - Byte 1: 0xE0-0xEF
 * - Byte 2: 0x80-0xBF
 * - Byte 3: 0x80-0xBF
 * 
 * GBK encoding (2 bytes):
 * - Byte 1: 0x81-0xFE
 * - Byte 2: 0x40-0xFE (except 0x7F)
 */
static const utf8_gbk_map_t utf8_gbk_table[] = {
    // Common characters (simplified set for demonstration)
    // 的 地 得 了 着 过 是 在 有 个 人 这 中 大 为 上 们 来 说 国 年 出 要 以 时 和 她 他 会 可 也 你 对 生 能 而 子 那 得 于 及 下 民 经 发 作 分 成 事 只 意 主 样 理 行 想 制 看 提 用 本 机 动 合 开 它 己 现 实 分 之 当 与 多 部 三 同 二 新 起 高 手 力 问 明 性 知 全 学 回 位 将 无 日 前 进 者 地 业 本 去 把 性 好 应 开 它 己 现 实 分 之 当 与 多 部 三 同 二 新 起 高 手 力 问 明 性 知 全 学 回 位 将 无 日 前 进 者 地 业 本 去 把 性 好 应
    
    {0xE79A84, 0xB5C4},  // 的
    {0xE59C9F, 0xB5D8},  // 地
    {0xE5BE97, 0xB5C3},  // 得
    {0xE4BA86, 0xC1CB},  // 了
    {0xE79D80, 0xD7C5},  // 着
    {0xE8BF87, 0xB9FD},  // 过
    {0xE698AF, 0xCAC7},  // 是
    {0xE59CA8, 0xD4DA},  // 在
    {0xE69C89, 0xD3D0},  // 有
    {0xE4B8AA, 0xB8F6},  // 个
    {0xE4BABA, 0xC8CB},  // 人
    {0xE8BF99, 0xD5E2},  // 这
    {0xE4B8AD, 0xD6D0},  // 中
    {0xE5A4A7, 0xB4F3},  // 大
    {0xE4B8BA, 0xCEAA},  // 为
    {0xE4B88A, 0xC9CF},  // 上
    {0xE4BBAC, 0xC3C7},  // 们
    {0xE69DA5, 0xC0B4},  // 来
    {0xE8AFB4, 0xCBB5},  // 说
    {0xE59BBD, 0xB9FA},  // 国
    {0xE5B9B4, 0xC4EA},  // 年
    {0xE587BA, 0xB3F6},  // 出
    {0xE8A681, 0xD2AA},  // 要
    {0xE4BBA5, 0xD2D4},  // 以
    {0xE697B6, 0xCAB1},  // 时
    {0xE5928C, 0xBACD},  // 和
    {0xE5A5B9, 0xCBFD},  // 她
    {0xE4BB96, 0xCBFB},  // 他
    {0xE4BC9A, 0xBBE1},  // 会
    {0xE58FAF, 0xBFC9},  // 可
    {0xE4B99F, 0xD2B2},  // 也
    {0xE4BDA0, 0xC4E3},  // 你
    {0xE5AFB9, 0xB6D4},  // 对
    {0xE7949F, 0xC9FA},  // 生
    {0xE883BD, 0xC4DC},  // 能
    {0xE8808C, 0xB6F8},  // 而
    {0xE5AD90, 0xD7D3},  // 子
    {0xE982A3, 0xC4C7},  // 那
    {0xE4BA8E, 0xD3DA},  // 于
    {0xE58F8A, 0xBCB0},  // 及
    {0xE4B88B, 0xCFC2},  // 下
    {0xE6B091, 0xC3F1},  // 民
    {0xE7BB8F, 0xBEAD},  // 经
    {0xE58F91, 0xB7A2},  // 发
    {0xE4BD9C, 0xD7F7},  // 作
    {0xE58886, 0xB7D6},  // 分
    {0xE68890, 0xB3C9},  // 成
    {0xE4BA8B, 0xCAC2},  // 事
    {0xE58FAA, 0xD6BB},  // 只
    {0xE6848F, 0xD2E2},  // 意
    {0xE4B8BB, 0xD6F7},  // 主
    {0xE6A0B7, 0xD1F9},  // 样
    {0xE79086, 0xC0ED},  // 理
    {0xE8A18C, 0xD0D0},  // 行
    {0xE683B3, 0xCFEB},  // 想
    {0xE588B6, 0xD6C6},  // 制
    {0xE79C8B, 0xBFB4},  // 看
    {0xE68F90, 0xCCE1},  // 提
    {0xE794A8, 0xD3C3},  // 用
    {0xE69CAC, 0xB1BE},  // 本
    {0xE69CBA, 0xBBFA},  // 机
    {0xE58AA8, 0xB6AF},  // 动
    {0xE59088, 0xBACF},  // 合
    {0xE5BC80, 0xBFAA},  // 开
    {0xE5AE83, 0xCBFC},  // 它
    {0xE5B7B1, 0xBCBA},  // 己
    {0xE78EB0, 0xCFD6},  // 现
    {0xE5AE9E, 0xCAB5},  // 实
    {0xE4B98B, 0xD6AE},  // 之
    {0xE5BD93, 0xB5B1},  // 当
    {0xE4B88E, 0xD3EB},  // 与
    {0xE5A49A, 0xB6E0},  // 多
    {0xE983A8, 0xB2BF},  // 部
    {0xE4B889, 0xC8FD},  // 三
    {0xE5908C, 0xCDAC},  // 同
    {0xE4BA8C, 0xB6FE},  // 二
    {0xE696B0, 0xD0C2},  // 新
    {0xE8B5B7, 0xC6F0},  // 起
    {0xE9AB98, 0xB8DF},  // 高
    {0xE6898B, 0xCAD6},  // 手
    {0xE58A9B, 0xC1A6},  // 力
    {0xE997AE, 0xCECA},  // 问
    {0xE6988E, 0xC3F7},  // 明
    {0xE680A7, 0xD0D4},  // 性
    {0xE79FA5, 0xD6AA},  // 知
    {0xE585A8, 0xC8AB},  // 全
    {0xE5ADA6, 0xD1A7},  // 学
    {0xE59B9E, 0xBBD8},  // 回
    {0xE4BD8D, 0xCEBB},  // 位
    {0xE5B086, 0xBDAB},  // 将
    {0xE697A0, 0xCEDE},  // 无
    {0xE697A5, 0xC8D5},  // 日
    {0xE5898D, 0xC7B0},  // 前
    {0xE8BF9B, 0xBDF8},  // 进
    {0xE88085, 0xD5DF},  // 者
    {0xE4B89A, 0xD2B5},  // 业
    {0xE58EBB, 0xC8A5},  // 去
    {0xE68A8A, 0xB0D1},  // 把
    {0xE5A5BD, 0xBAC3},  // 好
    {0xE5BA94, 0xD3A6},  // 应
    {0xE69687, 0xCEC4},  // 文
    {0xE4BBB6, 0xBCFE},  // 件
    {0xE58897, 0xC1D0},  // 列
    {0xE8A1A8, 0xB1ED},  // 表
    {0xE7BD91, 0xCDF8},  // 网
    {0xE7BB9C, 0xC2E7},  // 络
    {0xE69C8D, 0xB7FE},  // 服
    {0xE58AA1, 0xCEF1},  // 务
    {0xE599A8, 0xC6F7},  // 器
    {0xE4B88B, 0xCFC2},  // 下
    {0xE8BDBD, 0xD4D8},  // 载
    {0xE59BBE, 0xCDBC},  // 图
    {0xE78987, 0xC6AC},  // 片
    {0xE7BCA9, 0xCBF5},  // 缩
    {0xE79585, 0xC2D4},  // 略
};

#define UTF8_GBK_TABLE_SIZE (sizeof(utf8_gbk_table) / sizeof(utf8_gbk_map_t))

#endif // UTF8_GBK_TABLE_H
