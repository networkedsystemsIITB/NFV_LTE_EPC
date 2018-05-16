#ifndef GTP_H
#define GTP_H

#include "utils.h"

class Gtp {
private:
	/* 0 - 7 Protocol dependent - see below */
	uint8_t flags;

	/* 16 - 31 Message length (Size of payload excluding GTP header) */
	uint16_t msg_len;

	/* 64 - 79 Protocol dependent - see below */
	uint16_t field_1;

	/* 80 - 87 Protocol dependent - see below */
	uint16_t field_2;

	/* 88 - 95 Protocol dependent - see below */
	uint8_t field_3;

public:
	/* 8 - 15 Message Type */
	uint8_t msg_type;

	/* 32 - 63 TEID (Dummy TEID) */
	uint32_t teid;

	Gtp();
	void init(uint8_t, uint8_t, uint16_t, uint32_t);
	~Gtp();	
};

const int GTP_HDR_LEN = sizeof(Gtp);

/*
 * Protocol - gtpv1 (User plane)
 * 	flags
 * 		0 - 2 Version (GTPv1 - 1)
 * 		3 Protocol type (GTP - 1)
 * 		4 Reserved (0)
 * 		5 Externsion Header flag (0)
 * 		6 Sequence number (0)
 * 		7 N-PDU Flag number (0)
 * 	field_1
 * 		Sequence number (0)
 * 	field_2
 * 		N-PDU number (0)
 * 	field_3
 * 		Next Extension Header type (0)
 *
 * Protocol - gtpv2 (Control plane)
 * 	flags
 * 	 	0 - 2 Version (GTPv2 - 2)
 * 	   3 Piggybacking (0)
 * 		4 TEID (0 / 1)
 * 		5 - 7 Spare (0)
 * 	field_1 + field_2
 * 		Sequence number (0)
 * 	field_3
 * 		Spare (0)
 */


#endif /* GTP_H */