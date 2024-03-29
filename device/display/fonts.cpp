/**
 * original author:  Tilen Majerle<tilen@majerle.eu>
 * modification for STM32f10x: Alexander Lutsai<s.lyra@ya.ru>
   ----------------------------------------------------------------------
   	Copyright (C) Alexander Lutsai, 2016
    Copyright (C) Tilen Majerle, 2015
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
   ----------------------------------------------------------------------
 */
#include "fonts.h"

//#define FONT_WIDTH 6
//#define FONT_HEIGHT 10
// standard ascii 5x7 font
// originally from glcdfont.c from Adafruit project
//each bit is a pixel in a vertical row
static const uint8_t Font6x10[] = {
  0x00, 0x00, 0x00, 0x00, 0x00,
  0x3E, 0x5B, 0x4F, 0x5B, 0x3E,
  0x3E, 0x6B, 0x4F, 0x6B, 0x3E,
  0x1C, 0x3E, 0x7C, 0x3E, 0x1C,
  0x18, 0x3C, 0x7E, 0x3C, 0x18,
  0x1C, 0x57, 0x7D, 0x57, 0x1C,
  0x1C, 0x5E, 0x7F, 0x5E, 0x1C,
  0x00, 0x18, 0x3C, 0x18, 0x00,
  0xFF, 0xE7, 0xC3, 0xE7, 0xFF,
  0x00, 0x18, 0x24, 0x18, 0x00,
  0xFF, 0xE7, 0xDB, 0xE7, 0xFF,
  0x30, 0x48, 0x3A, 0x06, 0x0E,
  0x26, 0x29, 0x79, 0x29, 0x26,
  0x40, 0x7F, 0x05, 0x05, 0x07,
  0x40, 0x7F, 0x05, 0x25, 0x3F,
  0x5A, 0x3C, 0xE7, 0x3C, 0x5A,
  0x7F, 0x3E, 0x1C, 0x1C, 0x08,
  0x08, 0x1C, 0x1C, 0x3E, 0x7F,
  0x14, 0x22, 0x7F, 0x22, 0x14,
  0x5F, 0x5F, 0x00, 0x5F, 0x5F,
  0x06, 0x09, 0x7F, 0x01, 0x7F,
  0x00, 0x66, 0x89, 0x95, 0x6A,
  0x60, 0x60, 0x60, 0x60, 0x60,
  0x94, 0xA2, 0xFF, 0xA2, 0x94,
  0x08, 0x04, 0x7E, 0x04, 0x08,
  0x10, 0x20, 0x7E, 0x20, 0x10,
  0x08, 0x08, 0x2A, 0x1C, 0x08,
  0x08, 0x1C, 0x2A, 0x08, 0x08,
  0x1E, 0x10, 0x10, 0x10, 0x10,
  0x0C, 0x1E, 0x0C, 0x1E, 0x0C,
  0x30, 0x38, 0x3E, 0x38, 0x30,
  0x06, 0x0E, 0x3E, 0x0E, 0x06,
  0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x5F, 0x00, 0x00,
  0x00, 0x07, 0x00, 0x07, 0x00,
  0x14, 0x7F, 0x14, 0x7F, 0x14,
  0x24, 0x2A, 0x7F, 0x2A, 0x12,
  0x23, 0x13, 0x08, 0x64, 0x62,
  0x36, 0x49, 0x56, 0x20, 0x50,
  0x00, 0x08, 0x07, 0x03, 0x00,
  0x00, 0x1C, 0x22, 0x41, 0x00,
  0x00, 0x41, 0x22, 0x1C, 0x00,
  0x2A, 0x1C, 0x7F, 0x1C, 0x2A,
  0x08, 0x08, 0x3E, 0x08, 0x08,
  0x00, 0x80, 0x70, 0x30, 0x00,
  0x08, 0x08, 0x08, 0x08, 0x08,
  0x00, 0x00, 0x60, 0x60, 0x00,
  0x20, 0x10, 0x08, 0x04, 0x02,
  0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
  0x00, 0x42, 0x7F, 0x40, 0x00, // 1
  0x72, 0x49, 0x49, 0x49, 0x46, // 2
  0x21, 0x41, 0x49, 0x4D, 0x33, // 3
  0x18, 0x14, 0x12, 0x7F, 0x10, // 4
  0x27, 0x45, 0x45, 0x45, 0x39, // 5
  0x3C, 0x4A, 0x49, 0x49, 0x31, // 6
  0x41, 0x21, 0x11, 0x09, 0x07, // 7
  0x36, 0x49, 0x49, 0x49, 0x36, // 8
  0x46, 0x49, 0x49, 0x29, 0x1E, // 9
  0x00, 0x00, 0x14, 0x00, 0x00,
  0x00, 0x40, 0x34, 0x00, 0x00,
  0x00, 0x08, 0x14, 0x22, 0x41,
  0x14, 0x14, 0x14, 0x14, 0x14,
  0x00, 0x41, 0x22, 0x14, 0x08,
  0x02, 0x01, 0x59, 0x09, 0x06,
  0x3E, 0x41, 0x5D, 0x59, 0x4E,
  0x7C, 0x12, 0x11, 0x12, 0x7C, // A
  0x7F, 0x49, 0x49, 0x49, 0x36, // B
  0x3E, 0x41, 0x41, 0x41, 0x22, // C
  0x7F, 0x41, 0x41, 0x41, 0x3E, // D
  0x7F, 0x49, 0x49, 0x49, 0x41, // E
  0x7F, 0x09, 0x09, 0x09, 0x01, // F
  0x3E, 0x41, 0x41, 0x51, 0x73, // G
  0x7F, 0x08, 0x08, 0x08, 0x7F, // H
  0x00, 0x41, 0x7F, 0x41, 0x00, // I
  0x20, 0x40, 0x41, 0x3F, 0x01, // J
  0x7F, 0x08, 0x14, 0x22, 0x41, // K
  0x7F, 0x40, 0x40, 0x40, 0x40, // L
  0x7F, 0x02, 0x1C, 0x02, 0x7F, // M
  0x7F, 0x04, 0x08, 0x10, 0x7F, // N
  0x3E, 0x41, 0x41, 0x41, 0x3E, // O
  0x7F, 0x09, 0x09, 0x09, 0x06, // P
  0x3E, 0x41, 0x51, 0x21, 0x5E, // Q
  0x7F, 0x09, 0x19, 0x29, 0x46, // R
  0x26, 0x49, 0x49, 0x49, 0x32, // S
  0x03, 0x01, 0x7F, 0x01, 0x03, // T
  0x3F, 0x40, 0x40, 0x40, 0x3F, // U
  0x1F, 0x20, 0x40, 0x20, 0x1F, // V
  0x3F, 0x40, 0x38, 0x40, 0x3F, // W
  0x63, 0x14, 0x08, 0x14, 0x63, // X
  0x03, 0x04, 0x78, 0x04, 0x03, // Y
  0x61, 0x59, 0x49, 0x4D, 0x43, // Z
  0x00, 0x7F, 0x41, 0x41, 0x41,
  0x02, 0x04, 0x08, 0x10, 0x20,
  0x00, 0x41, 0x41, 0x41, 0x7F,
  0x04, 0x02, 0x01, 0x02, 0x04,
  0x40, 0x40, 0x40, 0x40, 0x40,
  0x00, 0x03, 0x07, 0x08, 0x00,
  0x20, 0x54, 0x54, 0x78, 0x40, // a
  0x7F, 0x28, 0x44, 0x44, 0x38, // b
  0x38, 0x44, 0x44, 0x44, 0x28, // c
  0x38, 0x44, 0x44, 0x28, 0x7F, // d
  0x38, 0x54, 0x54, 0x54, 0x18, // e
  0x00, 0x08, 0x7E, 0x09, 0x02, // f
  0x18, 0xA4, 0xA4, 0x9C, 0x78, // g
  0x7F, 0x08, 0x04, 0x04, 0x78, // h
  0x00, 0x44, 0x7D, 0x40, 0x00, // i
  0x20, 0x40, 0x40, 0x3D, 0x00, // j
  0x7F, 0x10, 0x28, 0x44, 0x00, // k
  0x00, 0x41, 0x7F, 0x40, 0x00, // l
  0x7C, 0x04, 0x78, 0x04, 0x78, // m
  0x7C, 0x08, 0x04, 0x04, 0x78, // n
  0x38, 0x44, 0x44, 0x44, 0x38, // o
  0xFC, 0x18, 0x24, 0x24, 0x18, // p
  0x18, 0x24, 0x24, 0x18, 0xFC, // q
  0x7C, 0x08, 0x04, 0x04, 0x08, // r
  0x48, 0x54, 0x54, 0x54, 0x24, // s
  0x04, 0x04, 0x3F, 0x44, 0x24, // t
  0x3C, 0x40, 0x40, 0x20, 0x7C, // u
  0x1C, 0x20, 0x40, 0x20, 0x1C, // v
  0x3C, 0x40, 0x30, 0x40, 0x3C, // w
  0x44, 0x28, 0x10, 0x28, 0x44, // x
  0x4C, 0x90, 0x90, 0x90, 0x7C, // y
  0x44, 0x64, 0x54, 0x4C, 0x44, // z
  0x00, 0x08, 0x36, 0x41, 0x00,
  0x00, 0x00, 0x77, 0x00, 0x00,
  0x00, 0x41, 0x36, 0x08, 0x00,
  0x02, 0x01, 0x02, 0x04, 0x02,
  0x3C, 0x26, 0x23, 0x26, 0x3C,
  0x1E, 0xA1, 0xA1, 0x61, 0x12,
  0x3A, 0x40, 0x40, 0x20, 0x7A,
  0x38, 0x54, 0x54, 0x55, 0x59,
  0x21, 0x55, 0x55, 0x79, 0x41,
  0x21, 0x54, 0x54, 0x78, 0x41,
  0x21, 0x55, 0x54, 0x78, 0x40,
  0x20, 0x54, 0x55, 0x79, 0x40,
  0x0C, 0x1E, 0x52, 0x72, 0x12,
  0x39, 0x55, 0x55, 0x55, 0x59,
  0x39, 0x54, 0x54, 0x54, 0x59,
  0x39, 0x55, 0x54, 0x54, 0x58,
  0x00, 0x00, 0x45, 0x7C, 0x41,
  0x00, 0x02, 0x45, 0x7D, 0x42,
  0x00, 0x01, 0x45, 0x7C, 0x40,
  0xF0, 0x29, 0x24, 0x29, 0xF0,
  0xF0, 0x28, 0x25, 0x28, 0xF0,
  0x7C, 0x54, 0x55, 0x45, 0x00,
  0x20, 0x54, 0x54, 0x7C, 0x54,
  0x7C, 0x0A, 0x09, 0x7F, 0x49,
  0x32, 0x49, 0x49, 0x49, 0x32,
  0x32, 0x48, 0x48, 0x48, 0x32,
  0x32, 0x4A, 0x48, 0x48, 0x30,
  0x3A, 0x41, 0x41, 0x21, 0x7A,
  0x3A, 0x42, 0x40, 0x20, 0x78,
  0x00, 0x9D, 0xA0, 0xA0, 0x7D,
  0x39, 0x44, 0x44, 0x44, 0x39,
  0x3D, 0x40, 0x40, 0x40, 0x3D,
  0x3C, 0x24, 0xFF, 0x24, 0x24,
  0x48, 0x7E, 0x49, 0x43, 0x66,
  0x2B, 0x2F, 0xFC, 0x2F, 0x2B,
  0xFF, 0x09, 0x29, 0xF6, 0x20,
  0xC0, 0x88, 0x7E, 0x09, 0x03,
  0x20, 0x54, 0x54, 0x79, 0x41,
  0x00, 0x00, 0x44, 0x7D, 0x41,
  0x30, 0x48, 0x48, 0x4A, 0x32,
  0x38, 0x40, 0x40, 0x22, 0x7A,
  0x00, 0x7A, 0x0A, 0x0A, 0x72,
  0x7D, 0x0D, 0x19, 0x31, 0x7D,
  0x26, 0x29, 0x29, 0x2F, 0x28,
  0x26, 0x29, 0x29, 0x29, 0x26,
  0x30, 0x48, 0x4D, 0x40, 0x20,
  0x38, 0x08, 0x08, 0x08, 0x08,
  0x08, 0x08, 0x08, 0x08, 0x38,
  0x2F, 0x10, 0xC8, 0xAC, 0xBA,
  0x2F, 0x10, 0x28, 0x34, 0xFA,
  0x00, 0x00, 0x7B, 0x00, 0x00,
  0x08, 0x14, 0x2A, 0x14, 0x22,
  0x22, 0x14, 0x2A, 0x14, 0x08,
  0xAA, 0x00, 0x55, 0x00, 0xAA,
  0xAA, 0x55, 0xAA, 0x55, 0xAA,
  0x00, 0x00, 0x00, 0xFF, 0x00,
  0x10, 0x10, 0x10, 0xFF, 0x00,
  0x14, 0x14, 0x14, 0xFF, 0x00,
  0x10, 0x10, 0xFF, 0x00, 0xFF,
  0x10, 0x10, 0xF0, 0x10, 0xF0,
  0x14, 0x14, 0x14, 0xFC, 0x00,
  0x14, 0x14, 0xF7, 0x00, 0xFF,
  0x00, 0x00, 0xFF, 0x00, 0xFF,
  0x14, 0x14, 0xF4, 0x04, 0xFC,
  0x14, 0x14, 0x17, 0x10, 0x1F,
  0x10, 0x10, 0x1F, 0x10, 0x1F,
  0x14, 0x14, 0x14, 0x1F, 0x00,
  0x10, 0x10, 0x10, 0xF0, 0x00,
  0x00, 0x00, 0x00, 0x1F, 0x10,
  0x10, 0x10, 0x10, 0x1F, 0x10,
  0x10, 0x10, 0x10, 0xF0, 0x10,
  0x00, 0x00, 0x00, 0xFF, 0x10,
  0x10, 0x10, 0x10, 0x10, 0x10,
  0x10, 0x10, 0x10, 0xFF, 0x10,
  0x00, 0x00, 0x00, 0xFF, 0x14,
  0x00, 0x00, 0xFF, 0x00, 0xFF,
  0x00, 0x00, 0x1F, 0x10, 0x17,
  0x00, 0x00, 0xFC, 0x04, 0xF4,
  0x14, 0x14, 0x17, 0x10, 0x17,
  0x14, 0x14, 0xF4, 0x04, 0xF4,
  0x00, 0x00, 0xFF, 0x00, 0xF7,
  0x14, 0x14, 0x14, 0x14, 0x14,
  0x14, 0x14, 0xF7, 0x00, 0xF7,
  0x14, 0x14, 0x14, 0x17, 0x14,
  0x10, 0x10, 0x1F, 0x10, 0x1F,
  0x14, 0x14, 0x14, 0xF4, 0x14,
  0x10, 0x10, 0xF0, 0x10, 0xF0,
  0x00, 0x00, 0x1F, 0x10, 0x1F,
  0x00, 0x00, 0x00, 0x1F, 0x14,
  0x00, 0x00, 0x00, 0xFC, 0x14,
  0x00, 0x00, 0xF0, 0x10, 0xF0,
  0x10, 0x10, 0xFF, 0x10, 0xFF,
  0x14, 0x14, 0x14, 0xFF, 0x14,
  0x10, 0x10, 0x10, 0x1F, 0x00,
  0x00, 0x00, 0x00, 0xF0, 0x10,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
  0xFF, 0xFF, 0xFF, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xFF, 0xFF,
  0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
  0x38, 0x44, 0x44, 0x38, 0x44,
  0x7C, 0x2A, 0x2A, 0x3E, 0x14,
  0x7E, 0x02, 0x02, 0x06, 0x06,
  0x02, 0x7E, 0x02, 0x7E, 0x02,
  0x63, 0x55, 0x49, 0x41, 0x63,
  0x38, 0x44, 0x44, 0x3C, 0x04,
  0x40, 0x7E, 0x20, 0x1E, 0x20,
  0x06, 0x02, 0x7E, 0x02, 0x02,
  0x99, 0xA5, 0xE7, 0xA5, 0x99,
  0x1C, 0x2A, 0x49, 0x2A, 0x1C,
  0x4C, 0x72, 0x01, 0x72, 0x4C,
  0x30, 0x4A, 0x4D, 0x4D, 0x30,
  0x30, 0x48, 0x78, 0x48, 0x30,
  0xBC, 0x62, 0x5A, 0x46, 0x3D,
  0x3E, 0x49, 0x49, 0x49, 0x00,
  0x7E, 0x01, 0x01, 0x01, 0x7E,
  0x2A, 0x2A, 0x2A, 0x2A, 0x2A,
  0x44, 0x44, 0x5F, 0x44, 0x44,
  0x40, 0x51, 0x4A, 0x44, 0x40,
  0x40, 0x44, 0x4A, 0x51, 0x40,
  0x00, 0x00, 0xFF, 0x01, 0x03,
  0xE0, 0x80, 0xFF, 0x00, 0x00,
  0x08, 0x08, 0x6B, 0x6B, 0x08,
  0x36, 0x12, 0x36, 0x24, 0x36,
  0x06, 0x0F, 0x09, 0x0F, 0x06,
  0x00, 0x00, 0x18, 0x18, 0x00,
  0x00, 0x00, 0x10, 0x10, 0x00,
  0x30, 0x40, 0xFF, 0x01, 0x01,
  0x00, 0x1F, 0x01, 0x01, 0x1E,
  0x00, 0x19, 0x1D, 0x17, 0x12,
  0x00, 0x3C, 0x3C, 0x3C, 0x3C,
  0x00, 0x00, 0x00, 0x00, 0x00,
};

FontDef_t Font_6x10 = {
	6,
	10,
	5,
	Font6x10
};

uint8_t Font7x10[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x04, 0x87, 0xe3, 0xe3, 0x70, 0x10, 0x00, 0x00, 0x00, 
0x00, 0x15, 0x45, 0x45, 0x41, 0x51, 0x54, 0x00, 0x00, 0x00, 
0x00, 0x89, 0xc4, 0x23, 0x91, 0xf0, 0x20, 0x10, 0x08, 0x00, 
0x00, 0x87, 0xc0, 0x20, 0xd0, 0x21, 0x30, 0x08, 0x04, 0x00, 
0x00, 0x8e, 0x40, 0xc0, 0xe1, 0x90, 0x38, 0x24, 0x12, 0x00, 
0x00, 0x81, 0x40, 0xe0, 0xe1, 0x11, 0x38, 0x04, 0x02, 0x00, 
0x00, 0x0e, 0x85, 0xc2, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x02, 0xe1, 0x43, 0x20, 0x7c, 0x00, 0x00, 0x00, 
0x00, 0x89, 0xc5, 0xa2, 0x91, 0x10, 0x08, 0x04, 0x1e, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x08, 0x04, 0x02, 0xf1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0xf0, 0x40, 0x20, 0x10, 0x08, 0x04, 0x00, 
0x00, 0x00, 0x00, 0x80, 0x43, 0x20, 0x10, 0x08, 0x04, 0x00, 
0x08, 0x04, 0x02, 0x81, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x08, 0x04, 0x02, 0xf1, 0x43, 0x20, 0x10, 0x08, 0x04, 0x00, 
0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0xc0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0xf0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 
0x08, 0x04, 0x02, 0x81, 0x43, 0x20, 0x10, 0x08, 0x04, 0x00, 
0x08, 0x04, 0x02, 0xf1, 0x40, 0x20, 0x10, 0x08, 0x04, 0x00, 
0x08, 0x04, 0x02, 0xf1, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0xf0, 0x43, 0x20, 0x10, 0x08, 0x04, 0x00, 
0x08, 0x04, 0x02, 0x81, 0x40, 0x20, 0x10, 0x08, 0x04, 0x00, 
0x00, 0x10, 0x84, 0x20, 0x20, 0x40, 0x40, 0x3e, 0x00, 0x00, 
0x00, 0x01, 0x01, 0x02, 0x82, 0x10, 0x04, 0x3e, 0x00, 0x00, 
0x00, 0x00, 0xc0, 0x47, 0xa1, 0x50, 0x28, 0x00, 0x00, 0x00, 
0x00, 0x10, 0xc4, 0x87, 0xf0, 0x11, 0x04, 0x00, 0x00, 0x00, 
0x00, 0x0c, 0x89, 0xe0, 0x21, 0x70, 0x6c, 0x04, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x04, 0x02, 0x81, 0x40, 0x00, 0x10, 0x00, 0x00, 0x00, 
0x00, 0x0a, 0x85, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x0a, 0xc5, 0x47, 0xf1, 0x51, 0x28, 0x00, 0x00, 0x00, 
0x00, 0x04, 0x47, 0xc1, 0x41, 0x71, 0x10, 0x00, 0x00, 0x00, 
0x00, 0x92, 0x8a, 0x82, 0xa0, 0xa8, 0x24, 0x00, 0x00, 0x00, 
0x00, 0x82, 0x42, 0x41, 0x50, 0x49, 0x58, 0x00, 0x00, 0x00, 
0x00, 0x0c, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x08, 0x82, 0x40, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 
0x00, 0x02, 0x02, 0x02, 0x81, 0x20, 0x08, 0x00, 0x00, 0x00, 
0x00, 0x80, 0x88, 0xe2, 0xa3, 0x88, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x02, 0xe1, 0x43, 0x20, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x10, 0x04, 0x00, 0x00, 
0x00, 0x00, 0x00, 0xe0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x38, 0x08, 0x00, 0x00, 
0x00, 0x10, 0x08, 0x82, 0x20, 0x08, 0x04, 0x00, 0x00, 0x00, 
0x00, 0x04, 0x45, 0x24, 0x12, 0x51, 0x10, 0x00, 0x00, 0x00, 
0x00, 0x04, 0x43, 0x81, 0x40, 0x20, 0x7c, 0x00, 0x00, 0x00, 
0x00, 0x8e, 0x08, 0x84, 0x21, 0x08, 0x7c, 0x00, 0x00, 0x00, 
0x00, 0x1f, 0x08, 0x82, 0x01, 0x89, 0x38, 0x00, 0x00, 0x00, 
0x00, 0x08, 0x86, 0x22, 0xf1, 0x41, 0x20, 0x00, 0x00, 0x00, 
0x00, 0x9f, 0x40, 0x63, 0x02, 0x89, 0x38, 0x00, 0x00, 0x00, 
0x00, 0x0c, 0x41, 0xa0, 0x31, 0x89, 0x38, 0x00, 0x00, 0x00, 
0x00, 0x1f, 0x08, 0x02, 0x41, 0x10, 0x08, 0x00, 0x00, 0x00, 
0x00, 0x8e, 0x48, 0xc4, 0x11, 0x89, 0x38, 0x00, 0x00, 0x00, 
0x00, 0x8e, 0x48, 0xc6, 0x02, 0x41, 0x18, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x82, 0x83, 0x00, 0x20, 0x38, 0x08, 0x00, 0x00, 
0x00, 0x00, 0x82, 0x83, 0x00, 0x60, 0x10, 0x04, 0x00, 0x00, 
0x00, 0x10, 0x04, 0x41, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 
0x00, 0x00, 0xc0, 0x07, 0xf0, 0x01, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x01, 0x01, 0x01, 0x41, 0x10, 0x04, 0x00, 0x00, 0x00, 
0x00, 0x8e, 0x08, 0x82, 0x40, 0x00, 0x10, 0x00, 0x00, 0x00, 
0x00, 0x8e, 0x48, 0xa6, 0xd2, 0x08, 0x38, 0x00, 0x00, 0x00, 
0x00, 0x04, 0x45, 0x24, 0xf2, 0x89, 0x44, 0x00, 0x00, 0x00, 
0x00, 0x0f, 0x89, 0xc4, 0x21, 0x91, 0x3c, 0x00, 0x00, 0x00, 
0x00, 0x8e, 0x48, 0x20, 0x10, 0x88, 0x38, 0x00, 0x00, 0x00, 
0x00, 0x0f, 0x89, 0x44, 0x22, 0x91, 0x3c, 0x00, 0x00, 0x00, 
0x00, 0x9f, 0x40, 0xe0, 0x11, 0x08, 0x7c, 0x00, 0x00, 0x00, 
0x00, 0x9f, 0x40, 0xe0, 0x11, 0x08, 0x04, 0x00, 0x00, 0x00, 
0x00, 0x8e, 0x48, 0x20, 0x90, 0x89, 0x38, 0x00, 0x00, 0x00, 
0x00, 0x91, 0x48, 0xe4, 0x13, 0x89, 0x44, 0x00, 0x00, 0x00, 
0x00, 0x0e, 0x02, 0x81, 0x40, 0x20, 0x38, 0x00, 0x00, 0x00, 
0x00, 0x1c, 0x08, 0x04, 0x02, 0x89, 0x38, 0x00, 0x00, 0x00, 
0x00, 0x91, 0x44, 0x61, 0x50, 0x48, 0x44, 0x00, 0x00, 0x00, 
0x00, 0x81, 0x40, 0x20, 0x10, 0x08, 0x7c, 0x00, 0x00, 0x00, 
0x00, 0x91, 0xc8, 0xa6, 0x12, 0x89, 0x44, 0x00, 0x00, 0x00, 
0x00, 0x91, 0xc8, 0xa4, 0x92, 0x89, 0x44, 0x00, 0x00, 0x00, 
0x00, 0x8e, 0x48, 0x24, 0x12, 0x89, 0x38, 0x00, 0x00, 0x00, 
0x00, 0x8f, 0x48, 0xe4, 0x11, 0x08, 0x04, 0x00, 0x00, 0x00, 
0x00, 0x8e, 0x48, 0x24, 0x12, 0xa9, 0x38, 0x20, 0x00, 0x00, 
0x00, 0x8f, 0x48, 0xe4, 0x51, 0x48, 0x44, 0x00, 0x00, 0x00, 
0x00, 0x8e, 0x48, 0xc0, 0x01, 0x89, 0x38, 0x00, 0x00, 0x00, 
0x00, 0x1f, 0x02, 0x81, 0x40, 0x20, 0x10, 0x00, 0x00, 0x00, 
0x00, 0x91, 0x48, 0x24, 0x12, 0x89, 0x38, 0x00, 0x00, 0x00, 
0x00, 0x91, 0x48, 0x44, 0xa1, 0x50, 0x10, 0x00, 0x00, 0x00, 
0x00, 0x91, 0x48, 0xa4, 0x52, 0xd9, 0x44, 0x00, 0x00, 0x00, 
0x00, 0x91, 0x88, 0x82, 0xa0, 0x88, 0x44, 0x00, 0x00, 0x00, 
0x00, 0x91, 0x88, 0x82, 0x40, 0x20, 0x10, 0x00, 0x00, 0x00, 
0x00, 0x1f, 0x08, 0x82, 0x20, 0x08, 0x7c, 0x00, 0x00, 0x00, 
0x00, 0x0e, 0x81, 0x40, 0x20, 0x10, 0x38, 0x00, 0x00, 0x00, 
0x00, 0x81, 0x80, 0x80, 0x80, 0x80, 0x40, 0x00, 0x00, 0x00, 
0x00, 0x0e, 0x04, 0x02, 0x81, 0x40, 0x38, 0x00, 0x00, 0x00, 
0x00, 0x04, 0x45, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 
0x00, 0x06, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x80, 0x03, 0xe2, 0x89, 0x78, 0x00, 0x00, 0x00, 
0x00, 0x81, 0x40, 0x63, 0x12, 0x99, 0x34, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x80, 0x23, 0x12, 0x88, 0x38, 0x00, 0x00, 0x00, 
0x00, 0x10, 0x88, 0x25, 0x13, 0xc9, 0x58, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x80, 0x23, 0xf2, 0x09, 0x38, 0x00, 0x00, 0x00, 
0x00, 0x0c, 0x89, 0xe0, 0x21, 0x10, 0x08, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x80, 0x25, 0x61, 0x08, 0x38, 0x22, 0x0e, 0x00, 
0x00, 0x81, 0x40, 0x63, 0x12, 0x89, 0x44, 0x00, 0x00, 0x00, 
0x00, 0x04, 0x80, 0x81, 0x40, 0x20, 0x38, 0x00, 0x00, 0x00, 
0x00, 0x08, 0x00, 0x03, 0x81, 0x40, 0x24, 0x12, 0x06, 0x00, 
0x00, 0x81, 0x40, 0x24, 0x71, 0x48, 0x44, 0x00, 0x00, 0x00, 
0x00, 0x06, 0x02, 0x81, 0x40, 0x20, 0x38, 0x00, 0x00, 0x00, 
0x00, 0x00, 0xc0, 0xa2, 0x52, 0xa9, 0x44, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x40, 0x63, 0x12, 0x89, 0x44, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x80, 0x23, 0x12, 0x89, 0x38, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x40, 0x63, 0x32, 0x69, 0x04, 0x02, 0x01, 0x00, 
0x00, 0x00, 0x80, 0x25, 0x93, 0xb1, 0x40, 0x20, 0x10, 0x00, 
0x00, 0x00, 0x40, 0x63, 0x12, 0x08, 0x04, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x80, 0x23, 0xe0, 0x80, 0x3c, 0x00, 0x00, 0x00, 
0x00, 0x02, 0xc1, 0x43, 0x20, 0x90, 0x30, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x40, 0x24, 0x12, 0xc9, 0x58, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x40, 0x24, 0xa2, 0x50, 0x10, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x40, 0x24, 0x52, 0xa9, 0x28, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x40, 0x44, 0x41, 0x50, 0x44, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x40, 0x24, 0x92, 0xb1, 0x40, 0x22, 0x0e, 0x00, 
0x00, 0x00, 0xc0, 0x07, 0x41, 0x10, 0x7c, 0x00, 0x00, 0x00, 
0x00, 0x18, 0x02, 0xc2, 0x80, 0x20, 0x60, 0x00, 0x00, 0x00, 
0x00, 0x04, 0x02, 0x81, 0x40, 0x20, 0x10, 0x00, 0x00, 0x00, 
0x00, 0x03, 0x82, 0x80, 0x21, 0x20, 0x0c, 0x00, 0x00, 0x00, 
0x00, 0x92, 0x4a, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x14, 0x80, 0x4f, 0xe0, 0x11, 0x08, 0x7c, 0x00, 0x00, 0x00, 
0x1c, 0x51, 0xb2, 0x5a, 0xcc, 0x8a, 0x38, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x04, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x0a, 0x80, 0x23, 0xf2, 0x09, 0x38, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x82, 0x22, 0x12, 0xf9, 0x44, 0x22, 0x00, 0x00, 0x00, 
0x80, 0x47, 0x20, 0xf0, 0x88, 0x44, 0x1e, 0x00, 0x00, 0x00, 
0x80, 0x47, 0x24, 0xf2, 0x88, 0x44, 0x1e, 0x00, 0x00, 0x00, 
0x80, 0x4f, 0x20, 0x10, 0x08, 0x04, 0x02, 0x00, 0x00, 0x00, 
0x00, 0x0c, 0x85, 0x22, 0x91, 0x48, 0x3e, 0x11, 0x00, 0x00, 
0x80, 0x4f, 0x20, 0xf0, 0x08, 0x04, 0x3e, 0x00, 0x00, 0x00, 
0x80, 0x4a, 0xa5, 0xe2, 0xa8, 0x54, 0x2a, 0x00, 0x00, 0x00, 
0x00, 0x47, 0x04, 0xc2, 0x80, 0x44, 0x1c, 0x00, 0x00, 0x00, 
0x80, 0x48, 0x24, 0x53, 0x99, 0x44, 0x22, 0x00, 0x00, 0x00, 
0x0a, 0x42, 0x24, 0x53, 0x99, 0x44, 0x22, 0x00, 0x00, 0x00, 
0x80, 0x48, 0xa2, 0x30, 0x28, 0x24, 0x22, 0x00, 0x00, 0x00, 
0x00, 0x8e, 0x44, 0x22, 0x91, 0x48, 0x22, 0x00, 0x00, 0x00, 
0x80, 0x48, 0x64, 0x53, 0x89, 0x44, 0x22, 0x00, 0x00, 0x00, 
0x80, 0x48, 0x24, 0xf2, 0x89, 0x44, 0x22, 0x00, 0x00, 0x00, 
0x00, 0x47, 0x24, 0x12, 0x89, 0x44, 0x1c, 0x00, 0x00, 0x00, 
0x80, 0x4f, 0x24, 0x12, 0x89, 0x44, 0x22, 0x00, 0x00, 0x00, 
0x80, 0x47, 0x24, 0xf2, 0x08, 0x04, 0x02, 0x00, 0x00, 0x00, 
0x00, 0x47, 0x24, 0x10, 0x08, 0x44, 0x1c, 0x00, 0x00, 0x00, 
0x80, 0x0f, 0x81, 0x40, 0x20, 0x10, 0x08, 0x00, 0x00, 0x00, 
0x80, 0x48, 0x24, 0xa2, 0x20, 0x08, 0x02, 0x00, 0x00, 0x00, 
0x00, 0x82, 0xa3, 0x52, 0xa9, 0x38, 0x08, 0x00, 0x00, 0x00, 
0x80, 0x48, 0x44, 0x41, 0x50, 0x44, 0x22, 0x00, 0x00, 0x00, 
0x80, 0x44, 0x22, 0x91, 0x48, 0x24, 0x3e, 0x10, 0x08, 0x00, 
0x80, 0x48, 0x24, 0xe2, 0x81, 0x40, 0x20, 0x00, 0x00, 0x00, 
0x80, 0x4a, 0xa5, 0x52, 0xa9, 0x54, 0x3e, 0x00, 0x00, 0x00, 
0x80, 0x4a, 0xa5, 0x52, 0xa9, 0x54, 0x3e, 0x10, 0x08, 0x00, 
0x80, 0x81, 0x40, 0xe0, 0x90, 0x48, 0x1c, 0x00, 0x00, 0x00, 
0x80, 0x48, 0x24, 0x72, 0xc9, 0x64, 0x2e, 0x00, 0x00, 0x00, 
0x00, 0x81, 0x40, 0xe0, 0x90, 0x48, 0x1c, 0x00, 0x00, 0x00, 
0x00, 0x47, 0x04, 0xc2, 0x81, 0x44, 0x1c, 0x00, 0x00, 0x00, 
0x80, 0x44, 0xa5, 0x72, 0xa9, 0x54, 0x12, 0x00, 0x00, 0x00, 
0x00, 0x4f, 0x24, 0xe2, 0xa1, 0x48, 0x22, 0x00, 0x00, 0x00, 
0x00, 0x00, 0xc0, 0x01, 0xf1, 0x44, 0x3c, 0x00, 0x00, 0x00, 
0x00, 0x4f, 0xc0, 0x11, 0x89, 0x44, 0x1c, 0x00, 0x00, 0x00, 
0x00, 0x00, 0xe0, 0x11, 0x79, 0x44, 0x1e, 0x00, 0x00, 0x00, 
0x00, 0x00, 0xc0, 0x23, 0x10, 0x08, 0x04, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x80, 0xa1, 0x50, 0x28, 0x3e, 0x11, 0x00, 0x00, 
0x00, 0x00, 0xc0, 0x11, 0xf9, 0x04, 0x1c, 0x00, 0x00, 0x00, 
0x00, 0x00, 0xa0, 0x52, 0x71, 0x54, 0x2a, 0x00, 0x00, 0x00, 
0x00, 0x00, 0xc0, 0x11, 0x61, 0x44, 0x1c, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x20, 0x92, 0xa9, 0x4c, 0x22, 0x00, 0x00, 0x00, 
0x00, 0x05, 0x21, 0x92, 0xa9, 0x4c, 0x22, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x20, 0x92, 0x38, 0x24, 0x22, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x80, 0x23, 0x91, 0x48, 0x22, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x20, 0xb2, 0xa9, 0x44, 0x22, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x20, 0x12, 0xf9, 0x44, 0x22, 0x00, 0x00, 0x00, 
0x00, 0x00, 0xc0, 0x11, 0x89, 0x44, 0x1c, 0x00, 0x00, 0x00, 
0x00, 0x00, 0xe0, 0x13, 0x89, 0x44, 0x22, 0x00, 0x00, 0x00, 
0x00, 0x00, 0xa0, 0x31, 0x89, 0x4c, 0x1a, 0x81, 0x00, 0x00, 
0x00, 0x00, 0xc0, 0x11, 0x09, 0x44, 0x1c, 0x00, 0x00, 0x00, 
0x00, 0x00, 0xe0, 0x43, 0x20, 0x10, 0x08, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x20, 0x12, 0x89, 0x78, 0x20, 0x11, 0x07, 0x00, 
0x00, 0x02, 0xc1, 0x51, 0xa9, 0x54, 0x1c, 0x04, 0x02, 0x00, 
0x00, 0x00, 0x20, 0xa2, 0x20, 0x28, 0x22, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x20, 0x91, 0x48, 0x24, 0x3e, 0x10, 0x08, 0x00, 
0x00, 0x00, 0x20, 0x12, 0xf1, 0x40, 0x20, 0x00, 0x00, 0x00, 
0x00, 0x00, 0xa0, 0x52, 0xa9, 0x54, 0x3e, 0x00, 0x00, 0x00, 
0x00, 0x00, 0xa0, 0x52, 0xa9, 0x54, 0x3e, 0x10, 0x08, 0x00, 
0x00, 0x00, 0x60, 0x20, 0x70, 0x48, 0x1c, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x20, 0x12, 0xb9, 0x64, 0x2e, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x40, 0x20, 0x70, 0x48, 0x1c, 0x00, 0x00, 0x00, 
0x00, 0x00, 0xc0, 0x11, 0xe1, 0x44, 0x1c, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x20, 0x51, 0xb9, 0x54, 0x12, 0x00, 0x00, 0x00, 
0x00, 0x00, 0xc0, 0x13, 0xf1, 0x44, 0x22, 0x00, 0x00, 0x00, 
};

FontDef_t Font_7x10 = {
	7,
	10,
	5,
   Font7x10
};

char* FONTS_GetStringSize(char* str, FONTS_SIZE_t* SizeStruct, FontDef_t* Font) {
	/* Fill settings */
	SizeStruct->Height = Font->FontHeight;
	SizeStruct->Length = Font->FontWidth * strlen(str);

	/* Return pointer */
	return str;
}




