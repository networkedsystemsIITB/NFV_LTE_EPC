#include "mme.h"

string g_trafmon_ip_addr = "10.129.41.57";
string g_mme_ip_addr = "10.129.28.41";
string g_hss_ip_addr = "10.129.26.238";
string g_sgw_s11_ip_addr = "10.129.26.93";
string g_sgw_s1_ip_addr = "10.129.26.93";
string g_sgw_s5_ip_addr = "10.129.26.93";
string g_pgw_s5_ip_addr = "10.129.28.205";
int g_trafmon_port = 10000;
int g_mme_port = 3000;
int g_hss_port = 4000;
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
	ue_count = 0;
	clrstl();
	g_sync.mux_init(s1mmeid_mux);
	g_sync.mux_init(uectx_mux);
}

void Mme::clrstl() {
	s1mme_id.clear();
	ue_ctx.clear();
}

uint32_t Mme::get_s11cteidmme(uint64_t guti) {
	uint32_t s11_cteid_mme;
	string tem;

	tem = to_string(guti);
	tem = tem.substr(7, -1); /* Extracting only the last 9 digits of UE MSISDN */
	s11_cteid_mme = stoull(tem);
	return s11_cteid_mme;
}

void Mme::handle_initial_attach(int conn_fd, Packet pkt, SctpClient &hss_client) {
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

	g_sync.mlock(uectx_mux);
	ue_ctx[guti].init(imsi, enodeb_s1ap_ue_id, mme_s1ap_ue_id, tai, nw_capability);
	nw_type = ue_ctx[guti].nw_type;
	TRACE(cout << "mme_handleinitialattach:" << ":ue entry added: " << guti << endl;)
	g_sync.munlock(uectx_mux);

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

	g_sync.mlock(uectx_mux);
	ue_ctx[guti].xres = xres;
	ue_ctx[guti].k_asme = k_asme;
	ue_ctx[guti].ksi_asme = 1;
	ksi_asme = ue_ctx[guti].ksi_asme;
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

bool Mme::handle_autn(int conn_fd, Packet pkt) {
	uint64_t guti;
	uint64_t res;
	uint64_t xres;

	guti = get_guti(pkt);
	if (guti == 0) {
		TRACE(cout << "mme_handleautn:" << " zero guti " << pkt.s1ap_hdr.mme_s1ap_ue_id << " " << pkt.len << ": " << guti << endl;)
		g_utils.handle_type1_error(-1, "Zero guti: mme_handleautn");
	}
	pkt.extract_item(res);
	g_sync.mlock(uectx_mux);
	xres = ue_ctx[guti].xres;
	g_sync.munlock(uectx_mux);
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

void Mme::handle_security_mode_cmd(int conn_fd, Packet pkt) {
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
	set_crypt_context(guti);
	set_integrity_context(guti);
	g_sync.mlock(uectx_mux);
	ksi_asme = ue_ctx[guti].ksi_asme;
	nw_capability = ue_ctx[guti].nw_capability;
	nas_enc_algo = ue_ctx[guti].nas_enc_algo;
	nas_int_algo = ue_ctx[guti].nas_int_algo;
	k_nas_enc = ue_ctx[guti].k_nas_enc;
	k_nas_int = ue_ctx[guti].k_nas_int;
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

void Mme::set_crypt_context(uint64_t guti) {
	g_sync.mlock(uectx_mux);
	ue_ctx[guti].nas_enc_algo = 1;
	ue_ctx[guti].k_nas_enc = ue_ctx[guti].k_asme + ue_ctx[guti].nas_enc_algo + ue_ctx[guti].count + ue_ctx[guti].bearer + ue_ctx[guti].dir;
	g_sync.munlock(uectx_mux);
}

void Mme::set_integrity_context(uint64_t guti) {
	g_sync.mlock(uectx_mux);
	ue_ctx[guti].nas_int_algo = 1;
	ue_ctx[guti].k_nas_int = ue_ctx[guti].k_asme + ue_ctx[guti].nas_int_algo + ue_ctx[guti].count + ue_ctx[guti].bearer + ue_ctx[guti].dir;
	g_sync.munlock(uectx_mux);
}

bool Mme::handle_security_mode_complete(int conn_fd, Packet pkt) {
	uint64_t guti;
	uint64_t k_nas_enc;
	uint64_t k_nas_int;
	bool res;

	guti = get_guti(pkt);
	if (guti == 0) {
		TRACE(cout << "mme_handlesecuritymodecomplete:" << " zero guti " << pkt.s1ap_hdr.mme_s1ap_ue_id << " " << pkt.len << ": " << guti << endl;		)
		g_utils.handle_type1_error(-1, "Zero guti: mme_handlesecuritymodecomplete");
	}		
	g_sync.mlock(uectx_mux);
	k_nas_enc = ue_ctx[guti].k_nas_enc;
	k_nas_int = ue_ctx[guti].k_nas_int;
	g_sync.munlock(uectx_mux);

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

void Mme::handle_location_update(Packet pkt, SctpClient &hss_client) {
	uint64_t guti;
	uint64_t imsi;
	uint64_t default_apn;

	guti = get_guti(pkt);
	if (guti == 0) {
		TRACE(cout << "mme_handlelocationupdate:" << " zero guti " << pkt.s1ap_hdr.mme_s1ap_ue_id << " " << pkt.len << ": " << guti << endl;		)
		g_utils.handle_type1_error(-1, "Zero guti: mme_handlelocationupdate");
	}		
	g_sync.mlock(uectx_mux);
	imsi = ue_ctx[guti].imsi;
	g_sync.munlock(uectx_mux);
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
	g_sync.mlock(uectx_mux);
	ue_ctx[guti].default_apn = default_apn;
	ue_ctx[guti].apn_in_use = ue_ctx[guti].default_apn;
	g_sync.munlock(uectx_mux);
}

void Mme::handle_create_session(int conn_fd, Packet pkt, UdpClient &sgw_client) {
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
	
	g_sync.mlock(uectx_mux);
	ue_ctx[guti].s11_cteid_mme = get_s11cteidmme(guti);
	ue_ctx[guti].eps_bearer_id = eps_bearer_id;
	s11_cteid_mme = ue_ctx[guti].s11_cteid_mme;
	imsi = ue_ctx[guti].imsi;
	eps_bearer_id = ue_ctx[guti].eps_bearer_id;
	pgw_s5_ip_addr = ue_ctx[guti].pgw_s5_ip_addr;
	pgw_s5_port = ue_ctx[guti].pgw_s5_port;
	apn_in_use = ue_ctx[guti].apn_in_use;
	tai = ue_ctx[guti].tai;
	g_sync.munlock(uectx_mux);

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
	TRACE(cout << "mme_createsession:" << " create session request sent to sgw: " << guti << endl;)

	sgw_client.rcv(pkt);
	TRACE(cout << "mme_createsession:" << " create session response received sgw: " << guti << endl;)

	pkt.extract_gtp_hdr();
	pkt.extract_item(s11_cteid_sgw);
	pkt.extract_item(ue_ip_addr);
	pkt.extract_item(s1_uteid_ul);
	pkt.extract_item(s5_uteid_ul);
	pkt.extract_item(s5_uteid_dl);

	g_sync.mlock(uectx_mux);
	ue_ctx[guti].ip_addr = ue_ip_addr;
	ue_ctx[guti].s11_cteid_sgw = s11_cteid_sgw;
	ue_ctx[guti].s1_uteid_ul = s1_uteid_ul;
	ue_ctx[guti].s5_uteid_ul = s5_uteid_ul;
	ue_ctx[guti].s5_uteid_dl = s5_uteid_dl;
	ue_ctx[guti].tai_list.clear();
	ue_ctx[guti].tai_list.push_back(ue_ctx[guti].tai);
	ue_ctx[guti].tau_timer = g_timer;
	ue_ctx[guti].e_rab_id = ue_ctx[guti].eps_bearer_id;
	ue_ctx[guti].k_enodeb = ue_ctx[guti].k_asme;
	e_rab_id = ue_ctx[guti].e_rab_id;
	k_enodeb = ue_ctx[guti].k_enodeb;
	nw_capability = ue_ctx[guti].nw_capability;
	tai_list = ue_ctx[guti].tai_list;
	tau_timer = ue_ctx[guti].tau_timer;
	k_nas_enc = ue_ctx[guti].k_nas_enc;
	k_nas_int = ue_ctx[guti].k_nas_int;
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

void Mme::handle_attach_complete(Packet pkt) {
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
		
	g_sync.mlock(uectx_mux);
	k_nas_enc = ue_ctx[guti].k_nas_enc;
	k_nas_int = ue_ctx[guti].k_nas_int;
	g_sync.munlock(uectx_mux);

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
	g_sync.mlock(uectx_mux);
	ue_ctx[guti].s1_uteid_dl = s1_uteid_dl;
	ue_ctx[guti].emm_state = 1;
	g_sync.munlock(uectx_mux);
	TRACE(cout << "mme_handleattachcomplete:" << " attach completed: " << guti << endl;)
}

void Mme::handle_modify_bearer(Packet pkt, UdpClient &sgw_client) {
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
	
	g_sync.mlock(uectx_mux);
	eps_bearer_id = ue_ctx[guti].eps_bearer_id;
	s1_uteid_dl = ue_ctx[guti].s1_uteid_dl;
	s11_cteid_sgw = ue_ctx[guti].s11_cteid_sgw;
	g_sync.munlock(uectx_mux);
	
	pkt.clear_pkt();
	pkt.append_item(eps_bearer_id);
	pkt.append_item(s1_uteid_dl);
	pkt.append_item(g_trafmon_ip_addr);
	pkt.append_item(g_trafmon_port);
	pkt.prepend_gtp_hdr(2, 2, pkt.len, s11_cteid_sgw);
	sgw_client.snd(pkt);
	TRACE(cout << "mme_handlemodifybearer:" << " modify bearer request sent to sgw: " << guti << endl;)

	sgw_client.rcv(pkt);
	TRACE(cout << "mme_handlemodifybearer:" << " modify bearer response received from sgw: " << guti << endl;)

	pkt.extract_gtp_hdr();
	pkt.extract_item(res);
	if (res == false) {
		TRACE(cout << "mme_handlemodifybearer:" << " modify bearer failure: " << guti << endl;)
	}
	else {
		g_sync.mlock(uectx_mux);
		ue_ctx[guti].ecm_state = 1;
		g_sync.munlock(uectx_mux);
		TRACE(cout << "mme_handlemodifybearer:" << " eps session setup success: " << guti << endl;		)
	}
}

void Mme::handle_detach(int conn_fd, Packet pkt, UdpClient &sgw_client) {
	uint64_t guti;
	uint64_t k_nas_enc;
	uint64_t k_nas_int;
	uint64_t ksi_asme;
	uint64_t detach_type;
	uint64_t tai;
	uint32_t s11_cteid_sgw;
	uint8_t eps_bearer_id;
	bool res;
	
	guti = get_guti(pkt);
	if (guti == 0) {
		TRACE(cout << "mme_handledetach:" << " zero guti " << pkt.s1ap_hdr.mme_s1ap_ue_id << " " << pkt.len << ": " << guti << endl;)
		g_utils.handle_type1_error(-1, "Zero guti: mme_handledetach");
	}
	g_sync.mlock(uectx_mux);
	k_nas_enc = ue_ctx[guti].k_nas_enc;
	k_nas_int = ue_ctx[guti].k_nas_int;
	eps_bearer_id = ue_ctx[guti].eps_bearer_id;
	tai = ue_ctx[guti].tai;
	s11_cteid_sgw = ue_ctx[guti].s11_cteid_sgw;
	g_sync.munlock(uectx_mux);

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
	pkt.extract_item(guti); /* It should be the same as that found in the first step */
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
	pkt.prepend_s1ap_hdr(5, pkt.len, pkt.s1ap_hdr.enodeb_s1ap_ue_id, pkt.s1ap_hdr.mme_s1ap_ue_id);
	server.snd(conn_fd, pkt);
	TRACE(cout << "mme_handledetach:" << " detach complete sent to ue: " << pkt.len << ": " << guti << endl;)

	rem_itfid(pkt.s1ap_hdr.mme_s1ap_ue_id);
	rem_uectx(guti);
	TRACE(cout << "mme_handledetach:" << " ue entry removed: " << guti << endl;)
	TRACE(cout << "mme_handledetach:" << " detach successful: " << guti << endl;)
}

void Mme::set_pgw_info(uint64_t guti) {
	g_sync.mlock(uectx_mux);
	ue_ctx[guti].pgw_s5_port = g_pgw_s5_port;
	ue_ctx[guti].pgw_s5_ip_addr = g_pgw_s5_ip_addr;
	g_sync.munlock(uectx_mux);
}

uint64_t Mme::get_guti(Packet pkt) {
	uint64_t mme_s1ap_ue_id;
	uint64_t guti;

	guti = 0;
	mme_s1ap_ue_id = pkt.s1ap_hdr.mme_s1ap_ue_id;
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
