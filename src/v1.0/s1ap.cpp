#include "s1ap.h"

S1ap::S1ap() {

}

void S1ap::init(uint8_t arg_msg_type, uint16_t arg_msg_len, uint32_t arg_enodeb_s1ap_ue_id, uint32_t arg_mme_s1ap_ue_id) {
	msg_type = arg_msg_type;
	msg_len = arg_msg_len;
	enodeb_s1ap_ue_id = arg_enodeb_s1ap_ue_id;
	mme_s1ap_ue_id = arg_mme_s1ap_ue_id;
}

S1ap::~S1ap() {

}