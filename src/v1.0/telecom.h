#ifndef TELECOM_H
#define TELECOM_H

#include "utils.h"

class Telecom {
public:
	uint16_t get_plmn_id(uint16_t, uint16_t);
	uint32_t get_mmei(uint16_t, uint8_t);
	uint64_t get_gummei(uint16_t, uint32_t);
	uint64_t get_imsi(uint16_t, uint64_t);
	uint64_t get_guti(uint64_t, uint64_t);
};

extern Telecom g_telecom;

#endif /* TELECOM_H */