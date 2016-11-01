#include "mme.h"

string g_trafmon_ip_addr = "10.129.26.175";
string g_mme_ip_addr = MME;

string g_hss_ip_addr = "10.129.28.108";
string g_sgw_s11_ip_addr = SGWLB;
string g_sgw_s1_ip_addr = SGWLB;
string g_sgw_s5_ip_addr = SGWLB;
string g_pgw_s5_ip_addr = PGWLB;

int g_trafmon_port = 4000;
int g_mme_port = 5000;
int g_hss_port = 6000;
int g_sgw_s11_port = 7000;
int g_sgw_s1_port = 7100;
int g_sgw_s5_port = 7200;
int g_pgw_s5_port = 8000;

uint64_t g_timer = 100;



UeContext::UeContext() {
	emm_state = 0;
	ecm_state = 0;
	imsi = 0;
	string ip_addr = "";
	enodeb_s1ap_ue_id = 0;
	mme_s1ap_ue_id = 0;
	tai = 0;
	tau_timer = 0;
	ksi_asme = 0;
	k_asme = 0; 
	k_nas_enc = 0; 
	k_nas_int = 0; 
	nas_enc_algo = 0; 
	nas_int_algo = 0; 
	count = 1;
	bearer = 0;
	dir = 1;
	default_apn = 0; 
	apn_in_use = 0; 
	eps_bearer_id = 0; 
	e_rab_id = 0;
	s1_uteid_ul = 0; 
	s1_uteid_dl = 0; 
	s5_uteid_ul = 0; 
	s5_uteid_dl = 0; 
	xres = 0;
	nw_type = 0;
	nw_capability = 0;	
	pgw_s5_ip_addr = "";	
	pgw_s5_port = 0;
	s11_cteid_mme = 0;
	s11_cteid_sgw = 0;	
}
//making serializable
template<class Archive>
void UeContext::serialize(Archive &ar, const unsigned int version)
{
	ar & emm_state & ecm_state & imsi & ip_addr & enodeb_s1ap_ue_id & mme_s1ap_ue_id & tai & tai_list & tau_timer & ksi_asme & k_asme & k_enodeb & k_nas_enc & k_nas_int & nas_enc_algo & nas_int_algo & count & bearer & dir & default_apn & apn_in_use & eps_bearer_id & e_rab_id & s1_uteid_ul & s1_uteid_dl & s5_uteid_ul & s5_uteid_dl & xres & nw_type & nw_capability & pgw_s5_ip_addr & pgw_s5_port & s11_cteid_mme & s11_cteid_sgw;
}

template<class Archive>
void Mme_state::serialize(Archive &ar, const unsigned int version)
{
	ar & Mme_state_uect & guti;
}

void UeContext::init(uint64_t arg_imsi, uint32_t arg_enodeb_s1ap_ue_id, uint32_t arg_mme_s1ap_ue_id, uint64_t arg_tai, uint16_t arg_nw_capability) {
	imsi = arg_imsi;
	enodeb_s1ap_ue_id = arg_enodeb_s1ap_ue_id;
	mme_s1ap_ue_id = arg_mme_s1ap_ue_id;
	tai = arg_tai;
	nw_capability = arg_nw_capability;
}

UeContext::~UeContext() {

}

MmeIds::MmeIds() {
	mcc = 1;
	mnc = 1;
	plmn_id = g_telecom.get_plmn_id(mcc, mnc);
	mmegi = 1;
	mmec = 1;
	mmei = g_telecom.get_mmei(mmegi, mmec);
	gummei = g_telecom.get_gummei(plmn_id, mmei);
}

MmeIds::~MmeIds() {

}

Mme::Mme() {

	clrstl();
	g_sync.mux_init(s1mmeid_mux);
	g_sync.mux_init(uectx_mux);



}
void Mme::initialize_kvstore_clients(int workers_count){

	ds_mme_state.resize(workers_count);

	for(int i=0;i<workers_count;i++){

		ds_mme_state[i].bind(dsmme_path,"ds_mme_state");
	}

}
void Mme::clrstl() {

}

uint32_t Mme::get_s11cteidmme(uint64_t guti) {
	uint32_t s11_cteid_mme;
	string tem;

	tem = to_string(guti);
	tem = tem.substr(7, -1); /* Extracting only the last 9 digits of UE MSISDN */
	s11_cteid_mme = stoull(tem);
	return s11_cteid_mme;
}

void Mme::handle_initial_attach(int conn_fd, Packet pkt, SctpClient &hss_client, int worker_id) {

	uint64_t imsi;
	uint64_t tai;
	uint64_t ksi_asme;
	uint16_t nw_type;
	uint16_t nw_capability;
	uint64_t autn_num;
	uint64_t rand_num;
	uint64_t xres;
	uint64_t k_asme;
	uint32_t enodeb_s1ap_ue_id;
	uint32_t mme_s1ap_ue_id;
	uint64_t guti;
	uint64_t num_autn_vectors;

	UeContext local_ue_ctx;

	num_autn_vectors = 1;
	pkt.extract_item(imsi);
	pkt.extract_item(tai);
	pkt.extract_item(ksi_asme); /* No use in this case */
	pkt.extract_item(nw_capability); /* No use in this case */

	enodeb_s1ap_ue_id = pkt.s1ap_hdr.enodeb_s1ap_ue_id;
	guti = g_telecom.get_guti(mme_ids.gummei, imsi);

	TRACE(cout << "mme_handleinitialattach:" << " initial attach req received: " << guti << endl;)

	g_sync.mlock(s1mmeid_mux);
	ue_count++;
	mme_s1ap_ue_id = ue_count;
	s1mme_id[mme_s1ap_ue_id] = guti;
	g_sync.munlock(s1mmeid_mux);

	local_ue_ctx.init(imsi, enodeb_s1ap_ue_id, mme_s1ap_ue_id, tai, nw_capability);

	nw_type = local_ue_ctx.nw_type;

	TRACE(cout << "mme_handleinitialattach:" << ":ue entry added: " << guti << endl;)

	pkt.clear_pkt();
	pkt.append_item(imsi);
	pkt.append_item(mme_ids.plmn_id);
	pkt.append_item(num_autn_vectors);
	pkt.append_item(nw_type);
	pkt.prepend_diameter_hdr(1, pkt.len);
	hss_client.snd(pkt);	
	TRACE(cout << "mme_handleinitialattach:" << " request sent to hss: " << guti << endl;)

	hss_client.rcv(pkt);

	pkt.extract_diameter_hdr();
	pkt.extract_item(autn_num);
	pkt.extract_item(rand_num);
	pkt.extract_item(xres);
	pkt.extract_item(k_asme);


	local_ue_ctx.xres = xres;
	local_ue_ctx.k_asme = k_asme;
	local_ue_ctx.ksi_asme = 1;
	ksi_asme = local_ue_ctx.ksi_asme;

	g_sync.mlock(uectx_mux);
	ue_ctx[guti] = local_ue_ctx;
	g_sync.munlock(uectx_mux);

	TRACE(cout << "mme_handleinitialattach:" << " autn:" << autn_num <<" rand:" << rand_num << " xres:" << xres << " k_asme:" << k_asme << " " << guti << endl;)

	pkt.clear_pkt();
	pkt.append_item(autn_num);
	pkt.append_item(rand_num);
	pkt.append_item(ksi_asme);
	pkt.prepend_s1ap_hdr(1, pkt.len, enodeb_s1ap_ue_id, mme_s1ap_ue_id);
	server.snd(conn_fd, pkt);

	TRACE(cout << "mme_handleinitialattach:" << " autn request sent to ran: " << guti << endl;	)
}
UeContext Mme::get_context(uint64_t guti){
	g_sync.mlock(uectx_mux);

	if (ue_ctx.find(guti) != ue_ctx.end()) {
		return ue_ctx[guti];
	}
	g_sync.munlock(uectx_mux);
}
void Mme::pull_context(Packet pkt,int worker_id){

	uint64_t mme_s1ap_ue_id;
	uint64_t guti;
	mme_s1ap_ue_id = pkt.s1ap_hdr.mme_s1ap_ue_id;

	Mme_state mme_state;
	auto bundle = ds_mme_state[worker_id].get(mme_s1ap_ue_id);
	if(bundle.ierr<0){

		g_utils.handle_type1_error(-1, "datastore retrieval error: pull_context");

	}else {

		mme_state =  (bundle.value);

	}

	g_sync.mlock(uectx_mux);
	guti = (mme_state).guti;
	ue_ctx[guti] = (mme_state).Mme_state_uect;
	g_sync.munlock(uectx_mux);

	g_sync.mlock(s1mmeid_mux);
	s1mme_id[mme_s1ap_ue_id] = guti;
	g_sync.munlock(s1mmeid_mux);


}
void Mme::push_context(uint64_t guti,UeContext local_ue_ctx,uint32_t mme_s1ap_ue_id,int worker_id){

	Mme_state state_data;
	state_data = Mme_state(local_ue_ctx,guti);
	auto bundle = ds_mme_state[worker_id].put(mme_s1ap_ue_id,state_data);
	if(bundle.ierr<0){			g_utils.handle_type1_error(-1, "datastore push error: push_context");
	}

}
void Mme::erase_context(uint32_t mme_s1ap_ue_id,int worker_id){

	ds_mme_state[worker_id].del(mme_s1ap_ue_id);

}
bool Mme::handle_autn(int conn_fd, Packet pkt, int worker_id) {
	uint64_t guti;
	uint64_t res;
	uint64_t xres;


	guti = get_guti(pkt);
	if (guti == 0) {
		TRACE(cout << "mme_handleautn:" << " zero guti " << pkt.s1ap_hdr.mme_s1ap_ue_id << " " << pkt.len << ": " << guti << endl;)
												g_utils.handle_type1_error(-1, "Zero guti: mme_handleautn");
	}
	pkt.extract_item(res);
	UeContext local_ue_ctx;
	g_sync.mlock(uectx_mux);
	local_ue_ctx = ue_ctx[guti];
	g_sync.munlock(uectx_mux);

	xres = local_ue_ctx.xres;

	if (res == xres) {
		/* Success */
		TRACE(cout << "mme_handleautn:" << " Authentication successful: " << guti << endl;)
												return true;
	}
	else {
		rem_itfid(pkt.s1ap_hdr.mme_s1ap_ue_id);
		rem_uectx(guti);
		return false;
	}
}

void Mme::handle_security_mode_cmd(int conn_fd, Packet pkt, int worker_id) {
	uint64_t guti;
	uint64_t ksi_asme;
	uint16_t nw_capability;
	uint64_t nas_enc_algo;
	uint64_t nas_int_algo;
	uint64_t k_nas_enc;
	uint64_t k_nas_int;



	guti = get_guti(pkt);
	if (guti == 0) {
		TRACE(cout << "mme_handlesecuritymodecmd:" << " zero guti " << pkt.s1ap_hdr.mme_s1ap_ue_id << " " << pkt.len << ": " << guti << endl;		)
												g_utils.handle_type1_error(-1, "Zero guti: mme_handlesecuritymodecmd");
	}	

	UeContext local_ue_ctx;
	g_sync.mlock(uectx_mux);
	local_ue_ctx = ue_ctx[guti];
	g_sync.munlock(uectx_mux);

	set_crypt_context(local_ue_ctx);
	set_integrity_context(local_ue_ctx);

	ksi_asme = local_ue_ctx.ksi_asme;
	nw_capability = local_ue_ctx.nw_capability;
	nas_enc_algo = local_ue_ctx.nas_enc_algo;
	nas_int_algo = local_ue_ctx.nas_int_algo;
	k_nas_enc = local_ue_ctx.k_nas_enc;
	k_nas_int = local_ue_ctx.k_nas_int;

	g_sync.mlock(uectx_mux);
	ue_ctx[guti] = local_ue_ctx;
	g_sync.munlock(uectx_mux);

	pkt.clear_pkt();
	pkt.append_item(ksi_asme);
	pkt.append_item(nw_capability);
	pkt.append_item(nas_enc_algo);
	pkt.append_item(nas_int_algo);
	if (HMAC_ON) {
		g_integrity.add_hmac(pkt, k_nas_int);
	}
	pkt.prepend_s1ap_hdr(2, pkt.len, pkt.s1ap_hdr.enodeb_s1ap_ue_id, pkt.s1ap_hdr.mme_s1ap_ue_id);
	server.snd(conn_fd, pkt);
	TRACE(cout << "mme_handlesecuritymodecmd:" << " security mode command sent: " << pkt.len << ": " << guti << endl;)
}

void Mme::set_crypt_context(UeContext &local_ue_ctx) {
	local_ue_ctx.nas_enc_algo = 1;
	local_ue_ctx.k_nas_enc = local_ue_ctx.k_asme + local_ue_ctx.nas_enc_algo + local_ue_ctx.count + local_ue_ctx.bearer + local_ue_ctx.dir;
}

void Mme::set_integrity_context(UeContext &local_ue_ctx) {
	local_ue_ctx.nas_int_algo = 1;
	local_ue_ctx.k_nas_int = local_ue_ctx.k_asme + local_ue_ctx.nas_int_algo + local_ue_ctx.count + local_ue_ctx.bearer + local_ue_ctx.dir;
}

bool Mme::handle_security_mode_complete(int conn_fd, Packet pkt, int worker_id) {
	uint64_t guti;
	uint64_t k_nas_enc;
	uint64_t k_nas_int;
	bool res;



	guti = get_guti(pkt);
	if (guti == 0) {
		TRACE(cout << "mme_handlesecuritymodecomplete:" << " zero guti " << pkt.s1ap_hdr.mme_s1ap_ue_id << " " << pkt.len << ": " << guti << endl;		)
												g_utils.handle_type1_error(-1, "Zero guti: mme_handlesecuritymodecomplete");
	}		

	UeContext local_ue_ctx;
	g_sync.mlock(uectx_mux);
	local_ue_ctx = ue_ctx[guti];
	g_sync.munlock(uectx_mux);

	k_nas_enc = local_ue_ctx.k_nas_enc;
	k_nas_int = local_ue_ctx.k_nas_int;

	TRACE(cout << "mme_handlesecuritymodecomplete:" << " security mode complete received: " << pkt.len << ": " << guti << endl;)

	if (HMAC_ON) {
		res = g_integrity.hmac_check(pkt, k_nas_int);
		if (res == false) {
			TRACE(cout << "mme_handlesecuritymodecomplete:" << " hmac failure: " << guti << endl;)
													g_utils.handle_type1_error(-1, "hmac failure: mme_handlesecuritymodecomplete");
		}		
	}
	if (ENC_ON) {
		g_crypt.dec(pkt, k_nas_enc);
	}
	pkt.extract_item(res);
	if (res == false) {
		TRACE(cout << "mme_handlesecuritymodecomplete:" << " security mode complete failure: " << guti << endl;)
												return false;
	}
	else {
		TRACE(cout << "mme_handlesecuritymodecomplete:" << " security mode complete success: " << guti << endl;)
												return true;
	}
}

void Mme::handle_location_update(Packet pkt, SctpClient &hss_client, int worker_id) {
	uint64_t guti;
	uint64_t imsi;
	uint64_t default_apn;

	guti = get_guti(pkt);
	if (guti == 0) {
		TRACE(cout << "mme_handlelocationupdate:" << " zero guti " << pkt.s1ap_hdr.mme_s1ap_ue_id << " " << pkt.len << ": " << guti << endl;		)
												g_utils.handle_type1_error(-1, "Zero guti: mme_handlelocationupdate");
	}
	UeContext local_ue_ctx;
	g_sync.mlock(uectx_mux);
	local_ue_ctx = ue_ctx[guti];
	g_sync.munlock(uectx_mux);
	imsi = local_ue_ctx.imsi;

	g_sync.mlock(uectx_mux);
	local_ue_ctx = ue_ctx[guti];
	g_sync.munlock(uectx_mux);

	imsi = local_ue_ctx.imsi;

	pkt.clear_pkt();
	pkt.append_item(imsi);
	pkt.append_item(mme_ids.mmei);
	pkt.prepend_diameter_hdr(2, pkt.len);
	hss_client.snd(pkt);
	TRACE(cout << "mme_handlelocationupdate:" << " loc update sent to hss: " << guti << endl;)

	hss_client.rcv(pkt);
	TRACE(cout << "mme_handlelocationupdate:" << " loc update response received from hss: " << guti << endl;)

	pkt.extract_diameter_hdr();
	pkt.extract_item(default_apn);



	local_ue_ctx.default_apn = default_apn;
	local_ue_ctx.apn_in_use = local_ue_ctx.default_apn;



	g_sync.mlock(uectx_mux);
	ue_ctx[guti] = local_ue_ctx;
	g_sync.munlock(uectx_mux);

}

void Mme::handle_create_session(int conn_fd, Packet pkt, UdpClient &sgw_client, int worker_id) {
	vector<uint64_t> tai_list;
	uint64_t guti;
	uint64_t imsi;
	uint64_t apn_in_use;
	uint64_t tai;
	uint64_t k_enodeb;
	uint64_t k_nas_enc;
	uint64_t k_nas_int;
	uint64_t tau_timer;
	uint32_t s11_cteid_mme;
	uint32_t s11_cteid_sgw;
	uint32_t s1_uteid_ul;
	uint32_t s5_uteid_ul;
	uint32_t s5_uteid_dl;
	uint16_t nw_capability;
	uint8_t eps_bearer_id;
	uint8_t e_rab_id;
	string pgw_s5_ip_addr;
	string ue_ip_addr;
	int tai_list_size;
	int pgw_s5_port;
	bool res;



	guti = get_guti(pkt);
	if (guti == 0) {
		TRACE(cout << "mme_handlecreatesession:" << " zero guti " << pkt.s1ap_hdr.mme_s1ap_ue_id << " " << pkt.len << ": " << guti << endl;		)
												g_utils.handle_type1_error(-1, "Zero guti: mme_handlecreatesession");
	}		
	eps_bearer_id = 5;
	set_pgw_info(guti);
	UeContext local_ue_ctx;
	g_sync.mlock(uectx_mux);
	local_ue_ctx = ue_ctx[guti];
	g_sync.munlock(uectx_mux);

	local_ue_ctx.s11_cteid_mme = get_s11cteidmme(guti);
	local_ue_ctx.eps_bearer_id = eps_bearer_id;
	s11_cteid_mme = local_ue_ctx.s11_cteid_mme;
	imsi = local_ue_ctx.imsi;
	eps_bearer_id = local_ue_ctx.eps_bearer_id;
	pgw_s5_ip_addr = local_ue_ctx.pgw_s5_ip_addr;
	pgw_s5_port = local_ue_ctx.pgw_s5_port;
	apn_in_use = local_ue_ctx.apn_in_use;
	tai = local_ue_ctx.tai;

	pkt.clear_pkt();
	pkt.append_item(s11_cteid_mme);
	pkt.append_item(imsi);
	pkt.append_item(eps_bearer_id);
	pkt.append_item(pgw_s5_ip_addr);
	pkt.append_item(pgw_s5_port);
	pkt.append_item(apn_in_use);
	pkt.append_item(tai);
	pkt.prepend_gtp_hdr(2, 1, pkt.len, 0);
	sgw_client.snd(pkt);
	TRACE(cout << "mme_createsession:" << " create session request sent to sgw: " << guti <<" wr:"<<worker_id <<endl;)

	sgw_client.rcv(pkt);
	TRACE(cout << "mme_createsession:" << " create session response received sgw: " << guti << endl;)

	pkt.extract_gtp_hdr();
	pkt.extract_item(s11_cteid_sgw);
	pkt.extract_item(ue_ip_addr);
	pkt.extract_item(s1_uteid_ul);
	pkt.extract_item(s5_uteid_ul);
	pkt.extract_item(s5_uteid_dl);

	local_ue_ctx.ip_addr = ue_ip_addr;
	local_ue_ctx.s11_cteid_sgw = s11_cteid_sgw;
	local_ue_ctx.s1_uteid_ul = s1_uteid_ul;
	local_ue_ctx.s5_uteid_ul = s5_uteid_ul;
	local_ue_ctx.s5_uteid_dl = s5_uteid_dl;
	local_ue_ctx.tai_list.clear();
	local_ue_ctx.tai_list.push_back(local_ue_ctx.tai);
	local_ue_ctx.tau_timer = g_timer;
	local_ue_ctx.e_rab_id = local_ue_ctx.eps_bearer_id;
	local_ue_ctx.k_enodeb = local_ue_ctx.k_asme;
	e_rab_id = local_ue_ctx.e_rab_id;
	k_enodeb = local_ue_ctx.k_enodeb;
	nw_capability = local_ue_ctx.nw_capability;
	tai_list = local_ue_ctx.tai_list;
	tau_timer = local_ue_ctx.tau_timer;
	k_nas_enc = local_ue_ctx.k_nas_enc;
	k_nas_int = local_ue_ctx.k_nas_int;

	g_sync.mlock(uectx_mux);
	ue_ctx[guti] = local_ue_ctx;
	g_sync.munlock(uectx_mux);

	res = true;
	tai_list_size = 1;

	pkt.clear_pkt();
	pkt.append_item(guti);
	pkt.append_item(eps_bearer_id);
	pkt.append_item(e_rab_id);
	pkt.append_item(s1_uteid_ul);
	pkt.append_item(k_enodeb);
	pkt.append_item(nw_capability);
	pkt.append_item(tai_list_size);
	pkt.append_item(tai_list);
	pkt.append_item(tau_timer);
	pkt.append_item(ue_ip_addr);
	pkt.append_item(g_sgw_s1_ip_addr);
	pkt.append_item(g_sgw_s1_port);
	pkt.append_item(res);
	if (ENC_ON) {
		g_crypt.enc(pkt, k_nas_enc);
	}
	if (HMAC_ON) {
		g_integrity.add_hmac(pkt, k_nas_int);
	}
	pkt.prepend_s1ap_hdr(3, pkt.len, pkt.s1ap_hdr.enodeb_s1ap_ue_id, pkt.s1ap_hdr.mme_s1ap_ue_id);
	server.snd(conn_fd, pkt);
	TRACE(cout << "mme_createsession:" << " attach accept sent to ue: " << pkt.len << ": " << guti << endl;)
}

void Mme::handle_attach_complete(Packet pkt, int worker_id) {
	uint64_t guti;
	uint64_t k_nas_enc;
	uint64_t k_nas_int;
	uint32_t s1_uteid_dl;
	uint8_t eps_bearer_id;
	bool res;




	guti = get_guti(pkt);
	if (guti == 0) {
		TRACE(cout << "mme_handleattachcomplete:" << " zero guti " << pkt.s1ap_hdr.mme_s1ap_ue_id << " " << pkt.len << ": " << guti << endl;		)
												g_utils.handle_type1_error(-1, "Zero guti: mme_handleattachcomplete");
	}

	UeContext local_ue_ctx;
	g_sync.mlock(uectx_mux);
	local_ue_ctx = ue_ctx[guti];
	g_sync.munlock(uectx_mux);

	k_nas_enc = local_ue_ctx.k_nas_enc;
	k_nas_int = local_ue_ctx.k_nas_int;


	TRACE(cout << "mme_handleattachcomplete:" << " attach complete received: " << pkt.len << ": " << guti << endl;)

	if (HMAC_ON) {
		res = g_integrity.hmac_check(pkt, k_nas_int);
		if (res == false) {
			TRACE(cout << "mme_handleattachcomplete:" << " hmac failure: " << guti << endl;)
													g_utils.handle_type1_error(-1, "hmac failure: mme_handleattachcomplete");
		}
	}
	if (ENC_ON) {
		g_crypt.dec(pkt, k_nas_enc);
	}
	pkt.extract_item(eps_bearer_id);
	pkt.extract_item(s1_uteid_dl);

	local_ue_ctx.s1_uteid_dl = s1_uteid_dl;
	local_ue_ctx.emm_state = 1;

	g_sync.mlock(uectx_mux);
	ue_ctx[guti] = local_ue_ctx;
	g_sync.munlock(uectx_mux);



	TRACE(cout << "mme_handleattachcomplete:" << " attach completed: " << guti << endl;)
}

void Mme::handle_modify_bearer(int conn_fd,Packet pkt, UdpClient &sgw_client, int worker_id) {
	uint64_t guti;
	uint32_t s1_uteid_dl;
	uint32_t s11_cteid_sgw;
	uint8_t eps_bearer_id;
	bool res;

	guti = get_guti(pkt);
	if (guti == 0) {
		TRACE(cout << "mme_handlemodifybearer:" << " zero guti " << pkt.s1ap_hdr.mme_s1ap_ue_id << " " << pkt.len << ": " << guti << endl;		)
												g_utils.handle_type1_error(-1, "Zero guti: mme_handlemodifybearer");
	}	

	UeContext local_ue_ctx;
	g_sync.mlock(uectx_mux);
	local_ue_ctx = ue_ctx[guti];
	g_sync.munlock(uectx_mux);
	eps_bearer_id = local_ue_ctx.eps_bearer_id;
	s1_uteid_dl = local_ue_ctx.s1_uteid_dl;
	s11_cteid_sgw = local_ue_ctx.s11_cteid_sgw;

	pkt.clear_pkt();
	pkt.append_item(eps_bearer_id);
	pkt.append_item(s1_uteid_dl);
	pkt.append_item(g_trafmon_ip_addr);
	pkt.append_item(g_trafmon_port);
	pkt.prepend_gtp_hdr(2, 2, pkt.len, s11_cteid_sgw);
	sgw_client.snd(pkt);
	TRACE(cout << "mme_handlemodifybearer:" << " modify bearer request sent to sgw: " << guti <<" wr:"<<worker_id<< endl;)

	sgw_client.rcv(pkt);
	TRACE(cout << "mme_handlemodifybearer:" << " modify bearer response received from sgw: " << guti << endl;)

	pkt.extract_gtp_hdr();
	pkt.extract_item(res);
	if (res == false) {
		TRACE(cout << "mme_handlemodifybearer:" << " modify bearer failure: " << guti << endl;)

				pkt.clear_pkt();
		pkt.append_item("false");
		server.snd(conn_fd, pkt);
	}
	else {
		local_ue_ctx.ecm_state = 1;

		g_sync.mlock(uectx_mux);
		ue_ctx[guti] = local_ue_ctx;
		g_sync.munlock(uectx_mux);

		push_context(guti,local_ue_ctx,local_ue_ctx.mme_s1ap_ue_id,worker_id); //push to store

		TRACE(cout << "mme_handlemodifybearer:" << " eps session setup success: " << guti << endl;		)

		pkt.clear_pkt();
		pkt.append_item("true");
		server.snd(conn_fd, pkt);
	}

}

void Mme::handle_detach(int conn_fd, Packet pkt, UdpClient &sgw_client, int worker_id) {
	uint64_t guti;
	uint64_t k_nas_enc;
	uint64_t k_nas_int;
	uint64_t ksi_asme;
	uint64_t detach_type;
	uint64_t tai;
	uint32_t s11_cteid_sgw;
	uint8_t eps_bearer_id;
	bool res;

	pull_context(pkt,worker_id);
	guti = get_guti(pkt);


	if (guti == 0) {
		TRACE(cout << "mme_handledetach:" << " zero guti " << pkt.s1ap_hdr.mme_s1ap_ue_id << " " << pkt.len << ": " << guti << endl;)
												g_utils.handle_type1_error(-1, "Zero guti: mme_handledetach");
	}


	UeContext local_ue_ctx;
	g_sync.mlock(uectx_mux);
	local_ue_ctx = ue_ctx[guti];
	g_sync.munlock(uectx_mux);

	k_nas_enc = local_ue_ctx.k_nas_enc;
	k_nas_int = local_ue_ctx.k_nas_int;
	eps_bearer_id = local_ue_ctx.eps_bearer_id;
	tai = local_ue_ctx.tai;
	s11_cteid_sgw = local_ue_ctx.s11_cteid_sgw;


	TRACE(cout << "mme_handledetach:" << " detach req received: " << pkt.len << ": " << guti << endl;)

	if (HMAC_ON) {
		res = g_integrity.hmac_check(pkt, k_nas_int);
		if (res == false) {
			TRACE(cout << "mme_handledetach:" << " hmac detach failure: " << guti << endl;)
													g_utils.handle_type1_error(-1, "hmac failure: mme_handledetach");
		}
	}
	if (ENC_ON) {
		g_crypt.dec(pkt, k_nas_enc);
	}
	pkt.extract_item(guti);
	pkt.extract_item(ksi_asme);
	pkt.extract_item(detach_type);	

	pkt.clear_pkt();
	pkt.append_item(eps_bearer_id);
	pkt.append_item(tai);
	pkt.prepend_gtp_hdr(2, 3, pkt.len, s11_cteid_sgw);
	sgw_client.snd(pkt);
	TRACE(cout << "mme_handledetach:" << " detach request sent to sgw: " << guti << endl;)

	sgw_client.rcv(pkt);
	TRACE(cout << "mme_handledetach:" << " detach response received from sgw: " << guti << endl;)

	pkt.extract_gtp_hdr();
	pkt.extract_item(res);
	if (res == false) {
		TRACE(cout << "mme_handledetach:" << " sgw detach failure: " << guti << endl;)
												return;
	}
	pkt.clear_pkt();
	pkt.append_item(res);

	if (ENC_ON) {
		g_crypt.enc(pkt, k_nas_enc);
	}
	if (HMAC_ON) {
		g_integrity.add_hmac(pkt, k_nas_int);
	}

	rem_itfid(pkt.s1ap_hdr.mme_s1ap_ue_id);
	rem_uectx(guti);

	erase_context(guti,worker_id);
	pkt.prepend_s1ap_hdr(5, pkt.len, pkt.s1ap_hdr.enodeb_s1ap_ue_id, pkt.s1ap_hdr.mme_s1ap_ue_id);
	server.snd(conn_fd, pkt);
	TRACE(cout << "mme_handledetach:" << " detach complete sent to ue: " << pkt.len << ": " << guti << endl;)

	TRACE(cout << "mme_handledetach:" << " ue entry removed: " << guti << endl;)
	TRACE(cout << "mme_handledetach:" << " detach successful: " << guti << endl;)
}

void Mme::set_pgw_info(uint64_t guti) {

	UeContext local_ue_ctx;
	g_sync.mlock(uectx_mux);
	local_ue_ctx = ue_ctx[guti];;
	local_ue_ctx.pgw_s5_port = g_pgw_s5_port;
	local_ue_ctx.pgw_s5_ip_addr = g_pgw_s5_ip_addr;
	ue_ctx[guti] = local_ue_ctx;
	g_sync.munlock(uectx_mux);
}

uint64_t Mme::get_guti(Packet pkt) {
	uint64_t mme_s1ap_ue_id;
	uint64_t guti;
	mme_s1ap_ue_id = pkt.s1ap_hdr.mme_s1ap_ue_id;

	guti = 0;

	g_sync.mlock(s1mmeid_mux);

	if (s1mme_id.find(mme_s1ap_ue_id) != s1mme_id.end()) {
		guti = s1mme_id[mme_s1ap_ue_id];
	}
	g_sync.munlock(s1mmeid_mux);
	return guti;
}



void Mme::rem_itfid(uint32_t mme_s1ap_ue_id) {
	g_sync.mlock(s1mmeid_mux);
	s1mme_id.erase(mme_s1ap_ue_id);
	g_sync.munlock(s1mmeid_mux);
}

void Mme::rem_uectx(uint64_t guti) {
	g_sync.mlock(uectx_mux);
	ue_ctx.erase(guti);
	g_sync.munlock(uectx_mux);
}

Mme::~Mme() {

}
Mme_state::Mme_state(UeContext ue,uint64_t guti_v){

	Mme_state_uect = ue;
	guti = guti_v;
}
Mme_state::Mme_state(){
}

