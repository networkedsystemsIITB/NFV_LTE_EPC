#include "tun.h"

Tun::Tun() {

}

void Tun::conn(string arg_name) {
	init(arg_name);
	attach();
}

void Tun::init(string arg_name) {
	name = arg_name;
}

void Tun::attach() {
	struct ifreq ifr;
	const char *DEV = "/dev/net/tun";
	int flags;
	int status;

	flags = (IFF_TUN | IFF_NO_PI);
	conn_fd = open(DEV , O_RDWR);
	g_utils.handle_type1_error(conn_fd, "tunopen error: tun_attach");
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = flags;
	if (name.size() != 0) {
		strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ);
	}
	status = ioctl(conn_fd, TUNSETIFF, (void*)&ifr);
	g_utils.handle_type1_error(status, "ioctl_tunsetiff error: network_attachtotun");
	name.assign(ifr.ifr_name);
}	

void Tun::snd(Packet pkt) {
	int status;

	status = write(conn_fd, pkt.data, pkt.len);
	g_utils.handle_type1_error(status, "tun write error: tun_snd");
}

void Tun::rcv(Packet &pkt) {
	int status;

	pkt.clear_pkt();
	status = read(conn_fd, pkt.data, BUF_SIZE);
	g_utils.handle_type1_error(status, "tun read error: tun_rcv");
	pkt.data_ptr = 0;
	pkt.len = status;
}

void Tun::set_itf(string name, string ip_addr_sp) {
	string rmtun; 
	string mktun;
	string itf_up;
	string add_addr;
	string set_mtu;

	rmtun = "sudo openvpn --rmtun --dev " + name;
	mktun = "sudo openvpn --mktun --dev " + name;
	itf_up = "sudo ip link set " + name + " up";
	add_addr = "sudo ip addr add " + ip_addr_sp + " dev " + name;
	set_mtu = "sudo ifconfig " + name + " mtu " + to_string(8000);
	system(rmtun.c_str());
	system(mktun.c_str());
	system(itf_up.c_str());
	system(add_addr.c_str());
	// system(set_mtu.c_str());
}

Tun::~Tun() {
	close(conn_fd);
}
