#ifndef SECURITY_H
#define SECURITY_H

/* (C) OPENSSL_config */
#include <openssl/conf.h>

/* (C) EVP_* */
#include <openssl/evp.h>

/* (C) ERR_* */
#include <openssl/err.h>

/* (C) HMAC_* */
#include <openssl/hmac.h>

#include "diameter.h"
#include "gtp.h"
#include "packet.h"
#include "s1ap.h"
#include "utils.h"

#define HMAC_ON 1
#define ENC_ON 1

const int HMAC_LEN = 20;

class Crypt {
private:	
	uint8_t *key;
	uint8_t *iv;

	void load();
	int enc_data(uint8_t*, int, uint8_t*, uint64_t);
	int dec_data(uint8_t*, int, uint8_t*, uint64_t);
	void handle_crypt_error();

public:
	Crypt();
	void enc(Packet&, uint64_t);
	void dec(Packet&, uint64_t);
	~Crypt();
};

class Integrity {
private:	
	uint8_t *key;

public:
	Integrity();
	void add_hmac(Packet&, uint64_t);
	void get_hmac(uint8_t*, int, uint8_t*, uint64_t);
	void rem_hmac(Packet&, uint8_t*);
	bool hmac_check(Packet&, uint64_t);
	bool cmp_hmacs(uint8_t*, uint8_t*);
	void print_hmac(uint8_t*);
	~Integrity();
};

extern Crypt g_crypt;
extern Integrity g_integrity;

#endif /* SECURITY_H */