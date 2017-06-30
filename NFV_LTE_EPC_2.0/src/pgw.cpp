#include "pgw.h"
#define SGI_ID "dsp_sgi_id"
#define S5_ID "dsp_s5_id"

string g_sgw_s5_ip_addr = SGWLB;
string g_pgw_s5_ip_addr = PGW1;
string g_pgw_sgi_ip_addr = PGW1;
string g_sink_ip_addr = SINK;
string g_sink_ip_addr2 = SINK2;
string g_sink_ip_addr3 = SINK3;

int g_sgw_s5_port = 7200;
int g_pgw_s5_port = 8000;
int g_pgw_sgi_port = 8100;
int g_sink_port = 8500;
int g_pgw_rep_port = 9501;
string g_pgw_rep1_ip = PGW2;
string g_pgw_rep2_ip = PGW3;

UeContext::UeContext() {
	ip_addr = "";
	tai = 0; 
	apn_in_use = 0; 
	s5_uteid_ul = 0; 
	s5_uteid_dl = 0; 
	s5_cteid_ul = 0;
	s5_cteid_dl = 0;
}

void UeContext::init(string arg_ip_addr, uint64_t arg_tai, uint64_t arg_apn_in_use, uint8_t arg_eps_bearer_id, uint32_t arg_s5_uteid_ul, uint32_t arg_s5_uteid_dl, uint32_t arg_s5_cteid_ul, uint32_t arg_s5_cteid_dl) {
	ip_addr = arg_ip_addr;
	tai = arg_tai; 
	apn_in_use = arg_apn_in_use; 
	eps_bearer_id = arg_eps_bearer_id; 
	s5_uteid_ul = arg_s5_uteid_ul; 
	s5_uteid_dl = arg_s5_uteid_dl; 
	s5_cteid_ul = arg_s5_cteid_ul;
	s5_cteid_dl = arg_s5_cteid_dl;
}

UeContext::~UeContext() {

}

template<class Archive>
void UeContext::serialize(Archive &ar, const unsigned int version)
{
	ar & ip_addr & tai & apn_in_use & eps_bearer_id & s5_uteid_ul & s5_uteid_dl & s5_cteid_ul & s5_cteid_dl;
}

Pgw::Pgw() {



	clrstl();
	set_ip_addrs();
	g_sync.mux_init(s5id_mux);	
	g_sync.mux_init(sgiid_mux);	
	g_sync.mux_init(uectx_mux);	


}
void Pgw::initialize_kvstore_clients(int workers_count){

	ds_sgi_id.resize(workers_count);
	ds_s5_id.resize(workers_count);
	ds_all.resize(workers_count);
if(UE_BINDING!=UE_BASED){

	for(int i=0;i<workers_count;i++){

		ds_sgi_id[i].bind(dspgw_path,SGI_ID);
		ds_s5_id[i].bind(dspgw_path,S5_ID);
		ds_all[i].bind(dspgw_path);

	}
}
cout<<"kvstore connected"<<endl;
}
void Pgw::clrstl() {
	s5_id.clear();
	sgi_id.clear();
	ue_ctx.clear();
	ip_addrs.clear();
}

void Pgw::handle_create_session(struct sockaddr_in src_sock_addr, Packet pkt,int worker_id) {
	uint64_t imsi;
	uint8_t eps_bearer_id;
	uint32_t s5_uteid_ul;
	uint32_t s5_uteid_dl;
	uint32_t s5_cteid_ul;
	uint32_t s5_cteid_dl;
	uint64_t apn_in_use;
	uint64_t tai;
	string ue_ip_addr;

	pkt.extract_item(s5_cteid_dl);
	pkt.extract_item(imsi);
	pkt.extract_item(eps_bearer_id);
	pkt.extract_item(s5_uteid_dl);
	pkt.extract_item(apn_in_use);
	pkt.extract_item(tai);
	s5_cteid_ul = s5_cteid_dl;
	s5_uteid_ul = s5_cteid_dl;
	ue_ip_addr = ip_addrs[imsi];

	UeContext local_ue_ctx;
	local_ue_ctx.init(ue_ip_addr, tai, apn_in_use, eps_bearer_id, s5_uteid_ul, s5_uteid_dl, s5_cteid_ul, s5_cteid_dl);
	
	update_itfid(5, s5_uteid_ul, "", imsi);
	update_itfid(0, 0, ue_ip_addr, imsi);
	
	g_sync.mlock(uectx_mux);
	ue_ctx[imsi] = local_ue_ctx;
	g_sync.munlock(uectx_mux);
	
		//if(UE_BINDING == UE_BASED){cout<<"push"<<s5_uteid_ul<<endl;}
	if( (UE_BINDING == SESSION_BASED) || (UE_BINDING == STATE_LESS) ){
	push_context(imsi,local_ue_ctx,worker_id);
	}
	if(REP) {
			Pgw_state state_data;
			state_data = Pgw_state(local_ue_ctx,imsi);
			send_replication(1,state_data, local_ue_ctx.s5_uteid_ul,local_ue_ctx.ip_addr,imsi,worker_id);

	}

	pkt.clear_pkt();
	pkt.append_item(s5_cteid_ul);
	pkt.append_item(eps_bearer_id);
	pkt.append_item(s5_uteid_ul);
	pkt.append_item(ue_ip_addr);
	pkt.prepend_gtp_hdr(2, 1, pkt.len, s5_cteid_dl);
	s5_server.snd(src_sock_addr, pkt);
	TRACE(cout << "pgw_handlecreatesession:" << " create session response sent to mme: " << imsi << endl;)
}
void Pgw::push_context(uint64_t imsi,UeContext local_ue_ctx,int worker_id){
	//cout<<"push"<<local_ue_ctx.s5_uteid_ul<<endl;
	Pgw_state state_data;
	state_data = Pgw_state(local_ue_ctx,imsi);

	ds_all[worker_id].put<uint32_t,Pgw_state>(local_ue_ctx.s5_uteid_ul,state_data,S5_ID);
	ds_all[worker_id].put<string,Pgw_state>(local_ue_ctx.ip_addr,state_data,SGI_ID);

	auto rs = ds_all[worker_id].execute();
	if(rs.get<Pgw_state>(0).ierr !=0 ) cout<<"###########################################"<<endl;
	if(rs.get<Pgw_state>(1).ierr !=0 ) cout<<"###########################################"<<endl;

	ds_all[worker_id].reset();

}
void Pgw::handle_uplink_udata(Packet pkt, UdpClient &sink_client,int worker_id) {
	
	
	/*uncomment when performing experiments with multiple sinks*/

	/*		if(pkt.gtp_hdr.teid%3==0){
				
				sink_client.set_server(g_sink_ip_addr, g_sink_port);

			}else if(pkt.gtp_hdr.teid%3==1){
				sink_client.set_server(g_sink_ip_addr2, g_sink_port);

			}else{
				sink_client.set_server(g_sink_ip_addr3, g_sink_port);

	*/
	
	pkt.truncate();
	sink_client.set_server(g_sink_ip_addr, g_sink_port);
	sink_client.snd(pkt);
	//cout<<"send up data"<<endl;
	TRACE(cout << "pgw_handleuplinkudata:" << " uplink udata forwarded to sink: " << pkt.len << endl;)
}

void Pgw::handle_downlink_udata(Packet pkt, UdpClient &sgw_s5_client,int worker_id) {
	uint64_t imsi;
	uint32_t s5_uteid_dl;
	string ue_ip_addr;
	bool res;

	ue_ip_addr = g_nw.get_dst_ip_addr(pkt);

	TRACE(cout<<"ue ip:"<<ue_ip_addr<<endl;)

	if(NOCACHE){

			pull_data_context(ue_ip_addr,s5_uteid_dl,imsi,worker_id);

	}
	else{
		if(sgi_id.find(ue_ip_addr)==sgi_id.end())
			pull_data_context(ue_ip_addr,s5_uteid_dl,imsi,worker_id);

	}
	
	imsi = get_imsi(0, 0, ue_ip_addr);
	if (imsi == 0) {
		TRACE(cout << "pgw_handledownlinkudata:" << " :zero imsi " << pkt.gtp_hdr.teid << " " << pkt.len << ": " << imsi << endl;)
						return;
	}

	res = get_downlink_info(imsi, s5_uteid_dl);
	if (res) {
		pkt.prepend_gtp_hdr(1, 3, pkt.len, s5_uteid_dl);
		sgw_s5_client.snd(pkt);

		TRACE(cout << "pgw_handledownlinkudata:" << " downlink udata forwarded to sgw: " << pkt.len << ": " << imsi << endl;)
	}
}

void Pgw::handle_detach(struct sockaddr_in src_sock_addr, Packet pkt,int worker_id) {
	uint64_t imsi;
	UeContext local_ue_ctx;
	uint64_t tai;
	uint32_t s5_cteid_ul;
	uint32_t s5_cteid_dl;
	uint8_t eps_bearer_id;
	string ue_ip_addr;
	bool res;

	res = true;
	//if(UE_BINDING == UE_BASED){	cout<<"pull try:"<<pkt.gtp_hdr.teid<<endl;}
	if( (UE_BINDING == SESSION_BASED) || (UE_BINDING == STATE_LESS) ){
		pull_context(pkt,imsi,local_ue_ctx,worker_id);//cout<<"NOway"<<endl;
	}else{
		imsi = get_imsi(5, pkt.gtp_hdr.teid, "");
		
		g_sync.mlock(uectx_mux);
		local_ue_ctx = ue_ctx[imsi];
		g_sync.munlock(uectx_mux);

	}
	if (imsi == 0) {
		TRACE(cout << "pgw_handledetach:" << " :zero imsi " << pkt.gtp_hdr.teid << " " << pkt.len << ": " << imsi << endl;)
				g_utils.handle_type1_error(-1, "Zero imsi: pgw_handledetach");
	}	
		//cout<<imsi<<endl;

	pkt.extract_item(eps_bearer_id);
	pkt.extract_item(tai);
	s5_cteid_ul = local_ue_ctx.s5_cteid_ul;
	s5_cteid_dl = local_ue_ctx.s5_cteid_dl;
	ue_ip_addr = local_ue_ctx.ip_addr;

	if( (UE_BINDING == SESSION_BASED) || (UE_BINDING==STATE_LESS) )erase_context(s5_cteid_ul,ue_ip_addr,worker_id);
	if(REP) {
			Pgw_state state_data;
			send_replication(0,state_data, local_ue_ctx.s5_uteid_ul,local_ue_ctx.ip_addr,imsi,worker_id);

	}
	pkt.clear_pkt();
	pkt.append_item(res);
	pkt.prepend_gtp_hdr(2, 4, pkt.len, s5_cteid_dl);
	s5_server.snd(src_sock_addr, pkt);
	TRACE(cout << "pgw_handledetach:" << " detach complete sent to sgw: " << imsi << endl;)
	TRACE(cout << "pgw_handledetach:" << " detach successful: " << imsi << endl;)
}

void Pgw::set_ip_addrs() {
	uint64_t imsi;
	int i;
	int subnet;
	int host;
	string prefix;
	string ip_addr;

	prefix = "172.16.";
	subnet = 1;
	host = 3;
	for (i = 0; i < MAX_UE_COUNT; i++) {
		imsi = 119000000000 + i;
		ip_addr = prefix + to_string(subnet) + "." + to_string(host);
		ip_addrs[imsi] = ip_addr;
		if (host == 254) {
			subnet++;
			host = 3;
		}
		else {
			host++;
		}
	}
}

void Pgw::update_itfid(int itf_id_no, uint32_t teid, string ue_ip_addr, uint64_t imsi) {
	switch (itf_id_no) {
	case 5:
		g_sync.mlock(s5id_mux);
		s5_id[teid] = imsi;
		g_sync.munlock(s5id_mux);
		break;
	case 0:
		g_sync.mlock(sgiid_mux);
		sgi_id[ue_ip_addr] = imsi;
		g_sync.munlock(sgiid_mux);
		break;
	default:
		g_utils.handle_type1_error(-1, "incorrect itf_id_no: pgw_updateitfid");
	}
}

void Pgw::pull_context(Packet pkt,uint64_t &imsi,UeContext &local_ue_ctx,int worker_id){

	Pgw_state pgw_state;

	auto bundle = ds_s5_id[worker_id].get(pkt.gtp_hdr.teid);
	if(bundle.ierr<0){

		cout<<"pull fail:"<<pkt.gtp_hdr.teid<<","<<imsi<<endl;

		//g_utils.handle_type1_error(-1, "datastore retrieval error: pull_context");

	}else {

		pgw_state =  (bundle.value);

	}
	imsi = (pgw_state).imsi;
	local_ue_ctx = (pgw_state).Pgw_state_uect;

}
void Pgw::pull_data_context(string ue_ip,uint32_t& s5_uteid_dl,uint64_t& imsi, int worker_id){

	Pgw_state pgw_state;

	auto bundle = ds_sgi_id[worker_id].get(ue_ip);
	if(bundle.ierr<0){

		cout<<"pull fail:"<<ue_ip<<endl;

		//g_utils.handle_type1_error(-1, "datastore retrieval error: pull_context");

	}else {

		pgw_state =  (bundle.value);

	}
	imsi = (pgw_state).imsi;
	g_sync.mlock(uectx_mux);
	ue_ctx[imsi] = (pgw_state).Pgw_state_uect;
	g_sync.munlock(uectx_mux);

	g_sync.mlock(s5id_mux);
	sgi_id[ue_ip] = imsi;
	g_sync.munlock(s5id_mux);

}
void Pgw::erase_context(uint32_t s5_id,string ip_addr,int worker_id){

	ds_all[worker_id].del<uint32_t,Pgw_state>(s5_id,S5_ID);
	ds_all[worker_id].del<string,Pgw_state>(ip_addr,SGI_ID);

	ds_all[worker_id].execute();
	ds_all[worker_id].reset();

}

uint64_t Pgw::get_imsi(int itf_id_no, uint32_t teid, string ue_ip_addr) {
	uint64_t imsi;

	imsi = 0;
	switch (itf_id_no) {
	case 5:
		g_sync.mlock(s5id_mux);
		if (s5_id.find(teid) != s5_id.end()) {
			imsi = s5_id[teid];
		}
		g_sync.munlock(s5id_mux);
		break;
	case 0:
		g_sync.mlock(sgiid_mux);
		if (sgi_id.find(ue_ip_addr) != sgi_id.end()) {
			imsi = sgi_id[ue_ip_addr];
		}
		g_sync.munlock(sgiid_mux);
		break;
	default:
		g_utils.handle_type1_error(-1, "incorrect itf_id_no: pgw_getimsi");
	}
	return imsi;
}


bool Pgw::get_downlink_info(uint64_t imsi, uint32_t &s5_uteid_dl) {
	bool res = false;

	g_sync.mlock(uectx_mux);
	if (ue_ctx.find(imsi) != ue_ctx.end()) {
		res = true;
		s5_uteid_dl = ue_ctx[imsi].s5_uteid_dl;
	}
	g_sync.munlock(uectx_mux);
	return res;
}

void Pgw::rem_itfid(int itf_id_no, uint32_t teid, string ue_ip_addr) {
	switch (itf_id_no) {
	case 5:
		g_sync.mlock(s5id_mux);
		s5_id.erase(teid);
		g_sync.munlock(s5id_mux);
		break;
	case 0:
		g_sync.mlock(sgiid_mux);
		sgi_id.erase(ue_ip_addr);
		g_sync.munlock(sgiid_mux);
		break;
	default:
		g_utils.handle_type1_error(-1, "incorrect itf_id_no: pgw_remitfid");
	}
}

void Pgw::rem_uectx(uint64_t imsi) {
	g_sync.mlock(uectx_mux);
	ue_ctx.erase(imsi);
	g_sync.munlock(uectx_mux);
}
void Pgw::send_replication(int repType,Pgw_state ctx,uint32_t s5_uteid_ul,string ip_addr,uint64_t imsi,int worker_id){

	Packet pkt;
	pkt.clear_pkt();
	pkt.append_item(repType);
	pkt.append_item(s5_uteid_ul);
	pkt.append_item(ip_addr);

	if(repType){

		string boostedState = toBoostString(ctx);
		pkt.append_item(boostedState);

	}else{

		pkt.append_item(imsi);
	}
	
	pgw_rep1_clients[worker_id].snd(pkt);
	pgw_rep2_clients[worker_id].snd(pkt);

}
void Pgw::receive_replication(Packet pkt,int worker_id){

	string boostedState;
	uint32_t s5_uteid_ul;
	string ip_addr;
	uint64_t imsi;
	int repType;
	pkt.extract_item(repType);
	pkt.extract_item(s5_uteid_ul);
	pkt.extract_item(ip_addr);
	
	if(repType){
		
		
		pkt.extract_item(boostedState);

		Pgw_state pgw_state = toBoostObject<Pgw_state>(boostedState);
		imsi = (pgw_state).imsi;

		g_sync.mlock(uectx_mux);
		ue_ctx[imsi] = (pgw_state).Pgw_state_uect;
		g_sync.munlock(uectx_mux);

		g_sync.mlock(s5id_mux);
		sgi_id[ip_addr] = imsi;
		g_sync.munlock(s5id_mux);

		g_sync.mlock(s5id_mux);
		s5_id[s5_uteid_ul] = imsi;
		g_sync.munlock(s5id_mux);

	}
	else{

		pkt.extract_item(imsi);
		rem_uectx(imsi) ;
		rem_itfid(5, s5_uteid_ul, ip_addr) ;
		rem_itfid(0, s5_uteid_ul, ip_addr) ;

	}
	
	
}
Pgw::~Pgw() {

}
template<class Archive>
void Pgw_state::serialize(Archive &ar, const unsigned int version)
{
	ar & Pgw_state_uect & imsi;
}
Pgw_state::Pgw_state(UeContext ue,uint64_t imsi_v){

	Pgw_state_uect = ue;
	imsi = imsi_v;
}
Pgw_state::Pgw_state(){
}

