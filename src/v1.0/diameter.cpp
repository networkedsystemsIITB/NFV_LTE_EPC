#include "diameter.h"

Diameter::Diameter() {

}

void Diameter::init(uint8_t arg_msg_type, uint16_t arg_msg_len) {
	msg_type = arg_msg_type;
	msg_len = arg_msg_len;
}

Diameter::~Diameter() {

}