#ifndef TUN_H
#define TUN_H

#include "../../NFV_LTE_EPC-1.0/src/diameter.h"
#include "../../NFV_LTE_EPC-1.0/src/gtp.h"
#include "../../NFV_LTE_EPC-1.0/src/network.h"
#include "../../NFV_LTE_EPC-1.0/src/packet.h"
#include "../../NFV_LTE_EPC-1.0/src/s1ap.h"
#include "../../NFV_LTE_EPC-1.0/src/utils.h"

class Tun {
private:
	string name;

	void init(string);
	void attach();

public:
	int conn_fd;
	
	Tun();
	void conn(string);
	void snd(Packet);
	void rcv(Packet&);
	void set_itf(string, string);
	~Tun();
};

#endif /* TUN_H */