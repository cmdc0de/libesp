#ifndef FONTS_H
#define FONTS_H 

#include <esp_types.h>

namespace libesp {
/**
 *
 * Default fonts library. It is used in all LCD based libraries.
 *
 * Currently, these fonts are supported:
 *  - 7 x 10 pixels
 *  - 11 x 18 pixels
 *  - 16 x 26 pixels
 */

/**
 * @brief  Font structure used on my LCD libraries
 */
typedef struct {
	uint16_t FontWidth;    /*!< Font width in pixels */
	uint16_t FontHeight;   /*!< Font height in pixels */
	uint8_t CharBytes;    /*!< Count of bytes for one character */
	const uint16_t *data; /*!< Pointer to data font data array */
} WFontDef_t;

/** 
 * @brief  String length and height 
 */
typedef struct {
	uint16_t Length;      /*!< String length in units of pixels */
	uint16_t Height;      /*!< String height in units of pixels */
} WFONTS_SIZE_t;

extern WFontDef_t Font_11x18;

/**
 * @brief  Calculates string length and height in units of pixels depending on string and font used
 * @param  *str: String to be checked for length and height
 * @param  *SizeStruct: Pointer to empty @ref FONTS_SIZE_t structure where informations will be saved
 * @param  *Font: Pointer to @ref FontDef_t font used for calculations
 * @retval Pointer to string used for length and height
 */
char* FONTS_GetStringSize(char* str, WFONTS_SIZE_t* SizeStruct, WFontDef_t* Font);

} 
#endif
