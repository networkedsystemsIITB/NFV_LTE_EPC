#include "hss.h"

string g_hss_ip_addr = "10.129.28.108";
int g_hss_port = 6000;

Hss::Hss() {

}
void Hss::initialize_kvstore_clients(int workers_count){


	ds_autn_infos.resize(workers_count);
	ds_loc_infos.resize(workers_count);
	for(int i=0;i<workers_count;i++){
		ds_autn_infos[i].bind(ds_path,"ds_autn_info");
		ds_loc_infos[i].bind(ds_path,"ds_loc_info");
	}
}
template<class Archive>
void Authinfo::serialize(Archive &ar, const unsigned int version)
{
	ar & key_id & rand_num;
}

void Hss::handle_mysql_conn() {

}

void Hss::get_autn_info(uint64_t imsi, uint64_t &key, uint64_t &rand_num, int worker_id) {

	auto bundle = ds_autn_infos[worker_id].get(imsi);
	if(bundle.ierr<0){

		g_utils.handle_type1_error(-1, "datastore retrieval error: hss_getautninfo");

	}else {

		key = ((Authinfo)((bundle.value))).key_id;
		rand_num = ((Authinfo)((bundle.value))).rand_num;
	}
}

void Hss::handle_autninfo_req(int conn_fd, Packet &pkt, int worker_id) {
	uint64_t imsi;
	uint64_t key;
	uint64_t rand_num;
	uint64_t autn_num;
	uint64_t sqn;
	uint64_t xres;
	uint64_t ck;
	uint64_t ik;
	uint64_t k_asme;
	uint64_t num_autn_vectors;
	uint16_t plmn_id;
	uint16_t nw_type;

	pkt.extract_item(imsi);
	pkt.extract_item(plmn_id);
	pkt.extract_item(num_autn_vectors);
	pkt.extract_item(nw_type);
	get_autn_info(imsi, key, rand_num,worker_id);
	TRACE(cout << "hss_handleautoinforeq:" << " retrieved from database: " << imsi << endl;)
	sqn = rand_num + 1;
	xres = key + sqn + rand_num;
	autn_num = xres + 1;
	ck = xres + 2;
	ik = xres + 3;
	k_asme = ck + ik + sqn + plmn_id;
	TRACE(cout << "hss_handleautoinforeq:" << " autn:" << autn_num << " rand:" << rand_num << " xres:" << xres << " k_asme:" << k_asme << " " << imsi << endl;)
	pkt.clear_pkt();
	pkt.append_item(autn_num);
	pkt.append_item(rand_num);
	pkt.append_item(xres);
	pkt.append_item(k_asme);
	pkt.prepend_diameter_hdr(1, pkt.len);
	server.snd(conn_fd, pkt);
	TRACE(cout << "hss_handleautoinforeq:" << " response sent to mme: " << imsi << endl;)
}

void Hss::set_loc_info(uint64_t imsi, uint32_t mmei, int worker_id) {



	ds_loc_infos[worker_id].del(imsi);
	TRACE(cout << "hss_setlocinfo:" << " cleared previous entry "<< endl;)

	ds_loc_infos[worker_id].put(imsi,mmei);
	TRACE(cout << "hss_setlocinfo:" << "imsi:" << imsi<<" mmei:"<<mmei << endl;)

}
void Hss::handle_location_update(int conn_fd, Packet &pkt,int worker_id) {
	uint64_t imsi;
	uint64_t default_apn;
	uint32_t mmei;

	default_apn = 1;
	pkt.extract_item(imsi);
	pkt.extract_item(mmei);
	set_loc_info(imsi, mmei,worker_id);
	TRACE(cout << "hss_handleautoinforeq:" << " loc updated" << endl;)
	pkt.clear_pkt();
	pkt.append_item(default_apn);
	pkt.prepend_diameter_hdr(2, pkt.len);
	server.snd(conn_fd, pkt);
	TRACE(cout << "hss_handleautoinforeq:" << " loc update complete sent to mme" << endl;)
}


Hss::~Hss() {

}
