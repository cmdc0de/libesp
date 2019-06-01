#ifndef BIT_ARRAY_H
#define BIT_ARRAY_H

#include <stdint.h>

namespace libesp {

class BitArray {
public:
	BitArray(uint8_t *array, uint32_t slots, uint8_t bitsPerSlot);
	void clear();
	uint8_t getValueAsByte(uint32_t slot);
	void setValueAsByte(uint32_t slot, uint8_t value);
	uint8_t getBitsPerSlot() {return BitsPerSlot;}
	BitArray &operator=(const BitArray &r);
	bool operator==(const BitArray &r);
private:
	BitArray(const BitArray &r);

private:
	uint8_t *Array;
	uint32_t Slots;
	uint8_t BitsPerSlot;
	uint32_t NumBytes;
};


}

#endif
