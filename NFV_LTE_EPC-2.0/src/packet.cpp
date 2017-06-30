#include "packet.h"

Packet::Packet() {
	data = g_utils.allocate_uint8_mem(BUF_SIZE);
	data_ptr = 0;
	len = 0;
}

Packet::Packet(const Packet &SRC_OBJ) {
	gtp_hdr = SRC_OBJ.gtp_hdr;
	s1ap_hdr = SRC_OBJ.s1ap_hdr;
	diameter_hdr = SRC_OBJ.diameter_hdr;
	data = g_utils.allocate_uint8_mem(BUF_SIZE);
	memmove(data, SRC_OBJ.data, SRC_OBJ.len);
	data_ptr = SRC_OBJ.data_ptr;
	len = SRC_OBJ.len;
}

void swap(Packet &src_obj, Packet &dst_obj) {
	using std::swap;

	swap(src_obj.gtp_hdr, dst_obj.gtp_hdr);
	swap(src_obj.s1ap_hdr, dst_obj.s1ap_hdr);
	swap(src_obj.diameter_hdr, dst_obj.diameter_hdr);
	swap(src_obj.data, dst_obj.data);
	swap(src_obj.data_ptr, dst_obj.data_ptr);
	swap(src_obj.len, dst_obj.len);
}

Packet& Packet::operator=(Packet src_obj) {
	swap(*this, src_obj);
	return *this;	
}

Packet::Packet(Packet &&src_obj)
	:Packet() {
	swap(*this, src_obj);	
}

void Packet::append_item(bool item) {
	int item_len = sizeof(bool);

	memmove(data + data_ptr, &item, item_len * sizeof(uint8_t));
	data_ptr += item_len;
	len += item_len;
}

void Packet::append_item(int item) {
	int item_len = sizeof(int);

	memmove(data + data_ptr, &item, item_len * sizeof(uint8_t));
	data_ptr += item_len;
	len += item_len;
}

void Packet::append_item(uint8_t item) {
	int item_len = sizeof(uint8_t);
	
	memmove(data + data_ptr, &item, item_len * sizeof(uint8_t));
	data_ptr += item_len;
	len += item_len;
}

void Packet::append_item(uint16_t item) {
	int item_len = sizeof(uint16_t);
	
	memmove(data + data_ptr, &item, item_len * sizeof(uint8_t));
	data_ptr += item_len;
	len += item_len;
}

void Packet::append_item(uint32_t item) {
	int item_len = sizeof(uint32_t);
	
	memmove(data + data_ptr, &item, item_len * sizeof(uint8_t));
	data_ptr += item_len;
	len += item_len;
}

void Packet::append_item(uint64_t item) {
	int item_len = sizeof(uint64_t);

	memmove(data + data_ptr, &item, item_len * sizeof(uint8_t));
	data_ptr += item_len;
	len += item_len;
}

void Packet::append_item(vector<uint64_t> item) {
	int i;
	int item_size = item.size();
	int item_ele_len = sizeof(uint64_t);

	for (i = 0; i < item_size; i++) {
		memmove(data + data_ptr, &item[i], item_ele_len * sizeof(uint8_t));
		data_ptr += item_ele_len;
		len += item_ele_len;	
	}
}

void Packet::append_item(uint8_t *item, int item_len) {
	memmove(data + data_ptr, item, item_len * sizeof(uint8_t));
	data_ptr += item_len;
	len += item_len;
}

void Packet::append_item(const char *ITEM) {
	int item_len = strlen(ITEM);
	
	memmove(data + data_ptr, ITEM, item_len * sizeof(uint8_t));
	data_ptr += item_len;
	len += item_len;
}

void Packet::append_item(string item) {
	int item_len = strlen(item.c_str());

	append_item(item_len);
	append_item(item.c_str());
}

void Packet::prepend_item(uint8_t *item, int item_len) {
	uint8_t *tem_data = g_utils.allocate_uint8_mem(BUF_SIZE);

	memmove(tem_data, item, item_len * sizeof(uint8_t));
	memmove(tem_data + item_len, data, len * sizeof(uint8_t));
	swap(data, tem_data);
	data_ptr += item_len;
	len += item_len;
	free(tem_data);
}

void Packet::prepend_gtp_hdr(uint8_t protocol, uint8_t msg_type, uint16_t msg_len, uint32_t teid) {
	uint8_t *tem_data = g_utils.allocate_uint8_mem(BUF_SIZE);
	int hdr_len = GTP_HDR_LEN;

	gtp_hdr.init(protocol, msg_type, msg_len, teid);
	memmove(tem_data, &gtp_hdr, hdr_len * sizeof(uint8_t));
	memmove(tem_data + hdr_len, data, len * sizeof(uint8_t));
	swap(data, tem_data);
	data_ptr += hdr_len;
	len += hdr_len;
	free(tem_data);
}

void Packet::prepend_s1ap_hdr(uint8_t msg_type, uint16_t msg_len, uint32_t enodeb_s1ap_ue_id, uint32_t mme_s1ap_ue_id) {
	uint8_t *tem_data = g_utils.allocate_uint8_mem(BUF_SIZE);	
	int hdr_len = S1AP_HDR_LEN;

	s1ap_hdr.init(msg_type, msg_len, enodeb_s1ap_ue_id, mme_s1ap_ue_id);
	memmove(tem_data, &s1ap_hdr, hdr_len * sizeof(uint8_t));
	memmove(tem_data + hdr_len, data, len * sizeof(uint8_t));
	swap(data, tem_data);
	data_ptr += hdr_len;
	len += hdr_len;
	free(tem_data);
}

void Packet::prepend_diameter_hdr(uint8_t msg_type, uint16_t msg_len) {
	uint8_t *tem_data = g_utils.allocate_uint8_mem(BUF_SIZE);	
	int hdr_len = DIAMETER_HDR_LEN;

	diameter_hdr.init(msg_type, msg_len);
	memmove(tem_data, &diameter_hdr, hdr_len * sizeof(uint8_t));
	memmove(tem_data + hdr_len, data, len * sizeof(uint8_t));
	swap(data, tem_data);
	data_ptr += hdr_len;
	len += hdr_len;
	free(tem_data);
}

void Packet::prepend_len() {
	uint8_t *tem_data;	
	int len_size = sizeof(int);

	tem_data = g_utils.allocate_uint8_mem(BUF_SIZE);
	memmove(tem_data, &len, len_size * sizeof(uint8_t));
	memmove(tem_data + len_size, data, len * sizeof(uint8_t));
	swap(data, tem_data);
	data_ptr += len_size;
	len += len_size;
	free(tem_data);
}

void Packet::extract_item(bool &item) {
	int item_len = sizeof(bool);

	memmove(&item, data + data_ptr, item_len * sizeof(uint8_t));
	data_ptr += item_len;
}

void Packet::extract_item(int &item) {
	int item_len = sizeof(int);

	memmove(&item, data + data_ptr, item_len * sizeof(uint8_t));
	data_ptr += item_len;
}

void Packet::extract_item(uint8_t &item) {
	int item_len = sizeof(uint8_t);

	memmove(&item, data + data_ptr, item_len * sizeof(uint8_t));
	data_ptr += item_len;
}

void Packet::extract_item(uint16_t &item) {
	int item_len = sizeof(uint16_t);

	memmove(&item, data + data_ptr, item_len * sizeof(uint8_t));
	data_ptr += item_len;
}

void Packet::extract_item(uint32_t &item) {
	int item_len = sizeof(uint32_t);

	memmove(&item, data + data_ptr, item_len * sizeof(uint8_t));
	data_ptr += item_len;
}

void Packet::extract_item(uint64_t &item) {
	int item_len = sizeof(uint64_t);

	memmove(&item, data + data_ptr, item_len * sizeof(uint8_t));
	data_ptr += item_len;
}

void Packet::extract_item(vector<uint64_t> &item, int item_size) {
	int i;
	int item_ele_len = sizeof(uint64_t);

	item.clear();
	for (i = 0; i < item_size; i++) {
		uint64_t tem_item;
	
		memmove(&tem_item, data + data_ptr, item_ele_len * sizeof(uint8_t));
		data_ptr += item_ele_len;
		item.push_back(tem_item);
	}
}

void Packet::extract_item(uint8_t *item, int item_len) {
	memmove(item, data + data_ptr, item_len * sizeof(uint8_t));
	data_ptr += item_len;
}

void Packet::extract_item(char *item, int item_len) {
	memmove(item, data + data_ptr, item_len * sizeof(uint8_t));
	data_ptr += item_len;
}

void Packet::extract_item(string &item) {
	char *citem;
	int item_len;

	extract_item(item_len);
	citem = g_utils.allocate_str_mem(item_len + 1); /* 1 extra byte for storing NULL character */
	extract_item(citem, item_len);
	item.assign(citem);
	free(citem);
}

void Packet::extract_gtp_hdr() {
	int hdr_len = GTP_HDR_LEN;

	memmove(&gtp_hdr, data + data_ptr, hdr_len * sizeof(uint8_t));
	data_ptr += hdr_len;
}

void Packet::extract_s1ap_hdr() {
	int hdr_len = S1AP_HDR_LEN;

	memmove(&s1ap_hdr, data + data_ptr, hdr_len * sizeof(uint8_t));
	data_ptr += hdr_len;
}

void Packet::extract_diameter_hdr() {
	int hdr_len = DIAMETER_HDR_LEN;

	memmove(&diameter_hdr, data + data_ptr, hdr_len * sizeof(uint8_t));
	data_ptr += hdr_len;
}

void Packet::truncate() {
	uint8_t *tem_data = g_utils.allocate_uint8_mem(BUF_SIZE);
	int new_len = len - data_ptr;

	memmove(tem_data, data + data_ptr, new_len * sizeof(uint8_t));
	swap(data, tem_data);
	data_ptr = 0;
	len = new_len;
	free(tem_data);
}

void Packet::clear_pkt() {
	int data_len = BUF_SIZE;
	
	memset(data, 0, data_len * sizeof (uint8_t));	
	data_ptr = 0;
	len = 0;
}

struct ip* Packet::allocate_ip_hdr_mem(int len) {
	struct ip *ip_hdr;

	if (len <= 0) {
		g_utils.handle_type1_error(-1, "Memory length error: packet_allocateiphdrmem");
	}
	ip_hdr = (ip*)malloc(len * sizeof (uint8_t));
	if (ip_hdr != NULL) {
		memset(ip_hdr, 0, len * sizeof (uint8_t));
		return ip_hdr;
	} 
	else {
		g_utils.handle_type1_error(-1, "Memory allocation error: packet_allocateiphdrmem");
	}
}

Packet::~Packet() {	
	free(data);
}
