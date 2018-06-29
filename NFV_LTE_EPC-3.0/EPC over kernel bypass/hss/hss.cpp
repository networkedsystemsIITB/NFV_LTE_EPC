#include "hss.h"
#include "defport.h"
string g_hss_ip_addr = hss_ip;
int g_hss_port = hss_my_port;

struct hssdata
{
	uint64_t key,rand_num;
};

std::map<uint64_t, hssdata> hssmap;

int isdbset = 0;

Hss::Hss() {
	g_sync.mux_init(mysql_client_mux);
}

void Hss::handle_mysql_conn() {
	/* Lock not necessary since this is called only once per object. Added for uniformity in locking */
	g_sync.mlock(mysql_client_mux);
	mysql_client.conn();
	if(isdbset == 0)
	{	
		hssmap.clear();
		isdbset = 1;
		uint64_t imsi = 119000000000;
		struct hssdata hsscur;
		for(imsi = 119000000000; imsi < 119000004096; imsi++)
		{
			hsscur.key = imsi % 10000;
			hsscur.rand_num = hsscur.key + 2;
			hssmap[imsi] = hsscur;
		}
		cout<<"DB set for "<<imsi<<endl;

	}
	g_sync.munlock(mysql_client_mux);
}

void Hss::get_autn_info(uint64_t imsi, uint64_t &key, uint64_t &rand_num) {
	
	//if(hssmap.find(imsi) == hssmap.end())
	//{
	//	cout<<"Error: Imsi not found "<<imsi<<endl;
	//	exit(-1);
	//}
	key = hssmap[imsi].key;
	rand_num = hssmap[imsi].rand_num;

	/*
	MYSQL_RES *query_res;
	MYSQL_ROW query_res_row;
	int i;
	int num_fields;
	string query;

	query_res = NULL;
	query = "select key_id, rand_num from autn_info where imsi = " + to_string(imsi);
	TRACE(cout << "hss_getautninfo:" << query << endl;)
	g_sync.mlock(mysql_client_mux);
	mysql_client.handle_query(query, &query_res);
	g_sync.munlock(mysql_client_mux);
	num_fields = mysql_num_fields(query_res);
	TRACE(cout << "hss_getautninfo:" << " fetched" <<imsi<< endl;)
	query_res_row = mysql_fetch_row(query_res);
	if (query_res_row == 0) {
		g_utils.handle_type1_error(-1, "mysql_fetch_row error: hss_getautninfo");
	}
	for (i = 0; i < num_fields; i++) {
		string query_res_field;

		query_res_field = query_res_row[i];
		if (i == 0) {
			key = stoull(query_res_field);
		}
		else {
			rand_num = stoull(query_res_field);
		}
	}
	mysql_free_result(query_res);
	*/
}

void Hss::handle_autninfo_req(int conn_fd, Packet &pkt) {
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
	get_autn_info(imsi, key, rand_num);
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

void Hss::set_loc_info(uint64_t imsi, uint32_t mmei) {
	MYSQL_RES *query_res;
	string query;

	query_res = NULL;
	query = "delete from loc_info where imsi = " + to_string(imsi);
	TRACE(cout << "hss_setlocinfo:" << " " << query << endl;)
	g_sync.mlock(mysql_client_mux);
	mysql_client.handle_query(query, &query_res);
	g_sync.munlock(mysql_client_mux);
	query = "insert into loc_info values(" + to_string(imsi) + ", " + to_string(mmei) + ")";
	TRACE(cout << "hss_setlocinfo:" << " " << query << endl;)
	g_sync.mlock(mysql_client_mux);
	mysql_client.handle_query(query, &query_res);
	g_sync.munlock(mysql_client_mux);
	mysql_free_result(query_res);	
}

void Hss::handle_location_update(int conn_fd, Packet &pkt) {
	uint64_t imsi;
	uint64_t default_apn;
	uint32_t mmei;

	default_apn = 1;
	pkt.extract_item(imsi);
	pkt.extract_item(mmei);
	set_loc_info(imsi, mmei);
	TRACE(cout << "hss_handleautoinforeq:" << " loc updated" << endl;)
	pkt.clear_pkt();
	pkt.append_item(default_apn);
	pkt.prepend_diameter_hdr(2, pkt.len);
	server.snd(conn_fd, pkt);
	TRACE(cout << "hss_handleautoinforeq:" << " loc update complete sent to mme" << endl;)
}

Hss::~Hss() {

}
