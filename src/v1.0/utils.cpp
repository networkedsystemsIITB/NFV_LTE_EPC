#include "utils.h"

Utils g_utils;

/* Action - Exit the program */
void Utils::handle_type1_error(int arg, string msg) {
	if (arg < 0) {
		msg = to_string(errno) + ": " + msg;
		perror(msg.c_str());
		exit(EXIT_FAILURE);
	}	
}

/* Action - Check for error conditions. Do not exit. */
void Utils::handle_type2_error(int arg, string msg) {
	if (arg < 0) {
		msg = to_string(errno) + ": " + msg;
		perror(msg.c_str());
	}
}

char* Utils::allocate_str_mem(int len) {
	char *tem;

	if (len <= 0) {
		handle_type1_error(-1, "Memory length error: utils_allocatestrmem");
	}
	tem = (char*)malloc(len * sizeof (char));
	if (tem != NULL) {
		memset(tem, 0, len * sizeof (char));
		return tem;
	}
	else {
		handle_type1_error(-1, "Memory allocation error: utils_allocatestrmem");
	}
}

uint8_t* Utils::allocate_uint8_mem(int len) {
	uint8_t *tem;

	if (len <= 0) {
		handle_type1_error(-1, "Memory length error: utils_allocateuint8mem");
	}
	tem = (uint8_t*)malloc(len * sizeof (uint8_t));
	if (tem != NULL) {
		memset(tem, 0, len * sizeof (uint8_t));
		return tem;
	} 
	else {
		handle_type1_error(-1, "Memory allocation error: utils_allocateuint8mem");
	}
}

void Utils::time_check(time_t start_time, double dur_time, bool &time_exceeded) {
	double elapsed_time;

	elapsed_time = difftime(time(0), start_time);
	if (elapsed_time >= dur_time) {
		time_exceeded = true;
	}
}

string exec_cmd(string cmd) {
    char buffer[128];
    string result = "";
    shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) throw runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer, 128, pipe.get()) != NULL)
            result += buffer;
    }
    return result;
}
