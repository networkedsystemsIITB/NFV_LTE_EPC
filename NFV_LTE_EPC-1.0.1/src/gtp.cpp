#include "gtp.h"

Gtp::Gtp() {
	field_1 = 0;
	field_2 = 0;
	field_3 = 0;
}

void Gtp::init(uint8_t protocol, uint8_t arg_msg_type, uint16_t arg_msg_len, uint32_t arg_teid) {
	msg_type = arg_msg_type;
	msg_len = arg_msg_len;
	teid = arg_teid;
	switch (protocol) {
		case 1:
			flags = 48;
			break;
		case 2:
			flags = (teid > 0) ? (72) : (64);
			break;
		default: 
			g_utils.handle_type1_error(-1, "gtp protocol error: gtp_init");
	}
}

Gtp::~Gtp() {

}