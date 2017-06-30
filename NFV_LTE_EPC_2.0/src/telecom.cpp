#include "telecom.h"

Telecom g_telecom;

uint16_t Telecom::get_plmn_id(uint16_t mcc, uint16_t mnc) {
	return stoull(to_string(mcc) + to_string(mnc));
}

uint32_t Telecom::get_mmei(uint16_t mmegi, uint8_t mmec) {
	return stoull(to_string(mmegi) + to_string(mmec));
}

uint64_t Telecom::get_gummei(uint16_t plmn_id, uint32_t mmei) {
	return stoull(to_string(plmn_id) + to_string(mmei));
}

uint64_t Telecom::get_imsi(uint16_t plmn_id, uint64_t msisdn) {
	return stoull(to_string(plmn_id) + to_string(msisdn));
}

uint64_t Telecom::get_guti(uint64_t gummei, uint64_t m_tmsi) {
	return stoull(to_string(gummei) + to_string(m_tmsi));
}