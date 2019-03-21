
#ifndef ATECC608A_CONFIG_H
#define ATECC608A_CONFIG_H

class ATECC608AConfig {
public:
	static uint8_t DEFAULT_ADDRESS = 0xC0;
public:
	struct SlotConfig {
		uint16_t data;
		bool isExteranSignaturesOfArbitraryMessagesEnabled() {return data&0x1;}
		bool isInternalSignaturesOfMesasagesGeneratedByGenDigOrGenKeyEnabled() { data&0x2;}
		bool isECDHEnabled() {return data&0x4;}
		bool isECDHMasterSecretOutputInClear() {return ((data&04)&&data&0x8;)}
		bool isKeyOnlyForMACCommand() {return data&0x10;}
		bool isKeyLimitedUse() {return data&0x20;}
		bool AreAllReadsEncrypted() {return data&0x40;}
		bool isSlotSecret() {return data&0x80;}
		uint8_t getWriteKey() {return ((data&0xF00)>>8);}
		uint8_t getWriteConfig() {return ((data&0xF000)>>12);}

	};
	struct X509Format {
		uint8_t data;
		uint8_t getPublicPosition() {
			return data&0x0F;
		}
		uint8_t getTemplateLength() {
			uint8_t retVal = (data&F0)>>4;
			return retVal&0x0F;
		}
	};
	struct KeyConfig {
		uint16_t data;
	};

private:
	uint8_t SerialNumber[8];
	uint8_t RevNum[4];
	uint8_t I2CAddress;
	SlotConfig SC[16];
	uint64_t Counter0;
	uint64_t Counter1;
	uint8_t UseLock;
	uint8_t ChipMode;
	uint16_t SecureBoot;
	uint8_t KdFlvLoc;
	uint8_t KdflvStr[2];
	uint8_t UserExtra;
	uint8_t UserExtraAdd;
	uint8_t LockValue;
	uint8_t LockConfig;
	uint16_t ChipOptions;
	uint32_t AES_Enable:1;
	uint32_t I2C_Enable:1;
	uint32_t CountMatch:4;
	uint32_t SlotLocked:16;
	uint32_t VolatileKeyPermission:4;
	X509Format X509FormatData;
	KeyConfig KeyConfigData[16];
};

#endif
