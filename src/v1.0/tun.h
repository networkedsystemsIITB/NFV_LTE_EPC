#ifndef TUN_H
#define TUN_H

#include "diameter.h"
#include "gtp.h"
#include "network.h"
#include "packet.h"
#include "s1ap.h"
#include "utils.h"

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