#include "ran_simulator.h"

time_t g_start_time;
int g_threads_count;
uint64_t g_req_dur;
uint64_t g_run_dur;
int g_tot_attm_regs;
int g_attemp_ret[16];
int g_step_freq[5];


int g_tot_regs;
uint64_t g_tot_regstime;
pthread_mutex_t g_mux;
pthread_mutex_t g_mux_retry;
pthread_mutex_t g_mux_step[5];

pthread_mutex_t g_mux_at;

vector<thread> g_umon_thread;
vector<thread> g_dmon_thread;
vector<thread> g_threads;
thread g_rtt_thread;
TrafficMonitor g_traf_mon;


void increaseAttempted(){

	g_sync.mlock(g_mux_at);
	g_tot_attm_regs++;
	g_sync.munlock(g_mux_at);

}

void relaunchInc(){
	g_sync.mlock(g_mux);
	g_attemp_ret[15]++;
	g_sync.munlock(g_mux);
}
void decreaseAttempted(){

	g_sync.mlock(g_mux_at);
	g_tot_attm_regs--;
	g_sync.munlock(g_mux_at);

}
void stepCount(int step){
	g_sync.mlock(g_mux_step[step]);
	g_step_freq[step]++;
	g_sync.munlock(g_mux_step[step]);
}


void simulate(int arg) {
	CLOCK::time_point mstart_time;
	CLOCK::time_point mstop_time;
	MICROSECONDS mtime_diff_us;

	int status;
	int ran_num;
	bool ok;
	bool time_exceeded;
	bool refresh_connection = false;
	ran_num = arg;
	time_exceeded = false;

	relaunch:  //relaunch point at state failure
	static bool fla= false;

	Ran *ran = new Ran();
	
	ran->init(ran_num);
	ran->conn_mme();
	int counter = 0;
	while (1) {
		
		g_utils.time_check(g_start_time, g_req_dur, time_exceeded);
		if (time_exceeded)break;

		increaseAttempted();
		mstart_time = CLOCK::now();

		int ctr1 = 0;
		label1:

		if(refresh_connection==true){
			
			RanContext temp_ran_ctx = ran->ran_ctx;
			ran = new Ran();
			ran->init(ran_num);
			ran->ran_ctx = temp_ran_ctx;
			ran->conn_mme();
			refresh_connection = false;
		}
	
		ok = ran->initial_attach();
		stepCount(0);
		if (!ok) {
			refresh_connection = true;
			TRACE(cout << "ransimulator_simulate:" << " autn failure" << endl;)
			g_utils.time_check(g_start_time, g_req_dur, time_exceeded);
			if (time_exceeded)break;
			if (ctr1<RETRY_LIMIT_FULL) {
				sleep(RETRY_Interval);

				ctr1++;
				
				
				if(ctr1<=RETRY_LIMIT_STEP) 
					goto label1;
				else 	
					goto label1;

			}
			else {
				relaunchInc();
				goto relaunch;
				}
		}


		label2:
		if(refresh_connection==true){
			RanContext temp_ran_ctx = ran->ran_ctx;
			ran = new Ran();
			ran->init(ran_num);
			ran->ran_ctx = temp_ran_ctx;
			ran->conn_mme();
			refresh_connection = false;
		}

		ok = ran->authenticate();
		stepCount(1);
		if (!ok) {
			refresh_connection = true;

				g_utils.time_check(g_start_time, g_req_dur, time_exceeded);
				if (time_exceeded)break;
				
			if (ctr1<RETRY_LIMIT_FULL) {
				sleep(RETRY_Interval);
				ctr1++;
			
				
				if(ctr1<=RETRY_LIMIT_STEP)
					goto label2;
				else 
					goto label1;

			}
			else {
				relaunchInc();
				goto relaunch;
				}
		}

	
		label3:
		if(refresh_connection==true){
			RanContext temp_ran_ctx = ran->ran_ctx;
			ran = new Ran();
			ran->init(ran_num);
			ran->ran_ctx = temp_ran_ctx;
			ran->conn_mme();
			refresh_connection = false;
		}
		ok = ran->set_security(g_traf_mon);
		stepCount(2);
		if (!ok) {
			refresh_connection = true;

			g_utils.time_check(g_start_time, g_req_dur, time_exceeded);
				if (time_exceeded)break;
				
			TRACE(cout << "ransimulator_simulate:" << " security setup failure" << endl;)
			if (ctr1<RETRY_LIMIT_FULL) {
				sleep(RETRY_Interval);
				ctr1++;
			
				
				if(ctr1<=RETRY_LIMIT_STEP)
					goto label3;
				else 
					goto label1;

			}
			else {
				relaunchInc();
				goto relaunch;
				}
		}


			static bool flagg= true;
			
		//This code make the experiment wait so that the failure of mme can be simulated, 
			
		if(flagg){
			sleep(10);
			flagg = false;
		}
		label4:
		if(refresh_connection==true){
			RanContext temp_ran_ctx = ran->ran_ctx;
			ran = new Ran();
			ran->init(ran_num);
			ran->ran_ctx = temp_ran_ctx;
			ran->conn_mme();
			refresh_connection = false;
		}
		ok = ran->set_eps_session(g_traf_mon);
		stepCount(3);
		if (!ok) {
			refresh_connection = true;

			g_utils.time_check(g_start_time, g_req_dur, time_exceeded);
				if (time_exceeded)break;
				
			TRACE(cout << "ransimulator_simulate:" << " eps session setup failure" << endl;)
			if (ctr1<RETRY_LIMIT_FULL) {
				sleep(RETRY_Interval);
				ctr1++;
				
				if(ctr1<=RETRY_LIMIT_STEP)
					goto label4;
				else 
					goto label1;

			}
			else {
				
				relaunchInc();
				goto relaunch;
				
				}
		}

	
	
		label5:
		if(refresh_connection==true){
			RanContext temp_ran_ctx = ran->ran_ctx;
			ran = new Ran();
			ran->init(ran_num);
			ran->ran_ctx = temp_ran_ctx;
			ran->conn_mme();
			refresh_connection = false;
		}
		ok = ran->detach();
		stepCount(4);
		if (!ok) {

			refresh_connection=true;
			TRACE(cout << "ransimulator_simulate:" << " detach failure" << endl;)
				g_utils.time_check(g_start_time, g_req_dur, time_exceeded);
				if (time_exceeded)break;
				
			if (ctr1<RETRY_LIMIT_FULL) {
				sleep(RETRY_Interval);
				
				ctr1++;
								
				
				if(ctr1<=RETRY_LIMIT_STEP)
					goto label5;
				else 
					goto label5;

			}
			else {
				
				relaunchInc();
				goto relaunch;
				
			}
		}

		mstop_time = CLOCK::now();

		mtime_diff_us = std::chrono::duration_cast<MICROSECONDS>(mstop_time - mstart_time);

		/* Updating performance metrics */
		g_sync.mlock(g_mux);
		g_attemp_ret[ctr1]++;
		g_tot_regs++;
		g_tot_regstime += mtime_diff_us.count();
		g_sync.munlock(g_mux);
		//	cout<<"completed  119000000-"<<arg<<endl;
	}
}

void check_usage(int argc) {
	if (argc < 3) {
		TRACE(cout << "Usage: ./<ran_simulator_exec> THREADS_COUNT DURATION" << endl;)
						g_utils.handle_type1_error(-1, "Invalid usage error: ransimulator_checkusage");
	}
}

void init(char *argv[]) {
	g_start_time = time(0);
	g_threads_count = atoi(argv[1]);
	g_req_dur = atoi(argv[2]);
	g_tot_regs = 0;
	g_tot_attm_regs = 0;
	g_tot_regstime = 0;

	cout<<"-->"<<g_attemp_ret[2]<<endl;
	g_sync.mux_init(g_mux);
	g_umon_thread.resize(NUM_MONITORS);
	g_dmon_thread.resize(NUM_MONITORS);
	g_threads.resize(g_threads_count);
	signal(SIGPIPE, SIG_IGN);
}

void run() {
	int i;


	// Simulator threads */
	for (i = 0; i < g_threads_count; i++) {
		g_threads[i] = thread(simulate, i);
	}
	for (i = 0; i < g_threads_count; i++) {
		if (g_threads[i].joinable()) {
			
			g_threads[i].join();		}
	}
}

void print_results() {
	g_run_dur = difftime(time(0), g_start_time);

	
	cout<<((double)g_tot_regstime/g_tot_regs) * 1e-6<<" "<<((double)g_tot_regs/g_run_dur)<<endl;
	
	//The counts records registration attmpts that have been succesful after a number of retries. e.g g_attemp_ret[0] registers registration attempt that is successful on first attempt
	//g_attemp_ret[1] registers registration attempt that is successful on second attempt
	cout<<"attempts:"<<g_attemp_ret[0]<<"||"<<g_attemp_ret[1]<<"||"<<g_attemp_ret[2]<<"||"<<g_attemp_ret[3]<<"||"<<g_attemp_ret[4]<<endl;
	cout<<"attempts:"<<g_attemp_ret[5]<<"||"<<g_attemp_ret[6]<<"||"<<g_attemp_ret[7]<<"||"<<g_attemp_ret[8]<<"||"<<g_attemp_ret[9]<<endl;
	
	cout << "Attempted :"<< g_tot_attm_regs << "Succeeded :"<< g_tot_regs << endl;
	cout << "stepCounts :"<< g_step_freq[0]<<"," <<g_step_freq[1]<<","<<g_step_freq[2]<<","<<g_step_freq[3]<<","<<g_step_freq[4]<<endl;
	cout<<"extra steps:"<<((5*g_tot_attm_regs)-g_step_freq[0]-g_step_freq[1]-g_step_freq[2]-g_step_freq[3]-g_step_freq[4])<<endl;
}

int main(int argc, char *argv[]) {
	cout<<"RAN running in MODE:"<<UE_BINDING<<endl;

	check_usage(argc);
	init(argv);
	run();
	cout<<atoi(argv[1])<<" ";
	print_results();
	return 0;
}
