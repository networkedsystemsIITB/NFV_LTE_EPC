#ifndef S1AP_H
#define S1AP_H

#include "utils.h"

class S1ap {
private:
	/* 8 - 23 Message length (Size of payload excluding S1AP header) */
	uint16_t msg_len;	
	
public:
	/* 0 - 7 Message Type */
	uint8_t msg_type; 

	/* 24 - 55 S1AP eNodeB UE ID */
	uint32_t enodeb_s1ap_ue_id;

	/* 56 - 87 S1AP eNodeB UE ID */
	uint32_t mme_s1ap_ue_id;

	S1ap();
	void init(uint8_t, uint16_t, uint32_t, uint32_t);
	~S1ap();
};

const int S1AP_HDR_LEN = sizeof(S1ap);

#endif /* S1AP_H */