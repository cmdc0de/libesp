/*
 * ssd1306_display.cpp
 *
 * Static data for the templated SSD1306Display class.
 */

#include "ssd1306_display.h"

using namespace libesp;

// Static DC pin for SPI transport pre-transfer callback
gpio_num_t SSD1306_SPITransport::DCPinGlobal = GPIO_NUM_NC;

// SSD1306 initialization command sequence
// Ported from ssd1306.cpp init() method
const SSD1306InitCmd libesp::SSD1306_INIT_SEQUENCE[] = {
	{0xAE},  // Display OFF
	{0x20},  // Set Memory Addressing Mode
	{0x10},  // Page Addressing Mode
	{0xB0},  // Set Page Start Address
	{0xC8},  // Set COM Output Scan Direction
	{0x00},  // Set low column address
	{0x10},  // Set high column address
	{0x40},  // Set start line address
	{0x81},  // Set contrast control register
	{0xFF},  // Max contrast
	{0xA1},  // Set segment re-map 0 to 127
	{0xA6},  // Set normal display
	{0xA8},  // Set multiplex ratio (1 to 64)
	{0x3F},  // 1/64 duty
	{0xA4},  // Output follows RAM content
	{0xD3},  // Set display offset
	{0x00},  // No offset
	{0xD5},  // Set display clock divide ratio/oscillator frequency
	{0xF0},  // Divide ratio
	{0xD9},  // Set pre-charge period
	{0x22},  // Pre-charge period value
	{0xDA},  // Set COM pins hardware configuration
	{0x12},  // COM pins config value
	{0xDB},  // Set VCOMH
	{0x20},  // 0.77xVcc
	{0x8D},  // Set DC-DC enable
	{0x14},  // DC-DC enabled
	{0xAF},  // Display ON
};

const uint16_t libesp::SSD1306_INIT_SEQUENCE_LEN =
	sizeof(libesp::SSD1306_INIT_SEQUENCE) / sizeof(libesp::SSD1306_INIT_SEQUENCE[0]);
