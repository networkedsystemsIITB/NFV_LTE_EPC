### Virtualized Evolved Packet Core for LTE Networks (vEPC revision 1.0.1)

### Whats new in this version
This version includes an S1 based handover procedure integrated to the v_1.0 source. The handover procedure covers all control plane steps:

1) Triggering Handover from source RAN
2) Setting up Indirect tunnel to target RAN via SGW
3) Triggering reconnection of UE at target RAN
4) Completing Handover
5) Tearing indirect tunnels to source RAN


#### Directory structure

- **src**: Contains source code files
#### User Guide

1) Please follow the document *user_manual.pdf* from v_1.0 to setup the EPC in separate virtual machines. Source contains ran_simulator.cpp which performs normal attach and detach operation experimentation. ran_simulator_handover contains handover flow of a UE where a single UE performs handover from a particular sourceRAN to TargetRAN.

2) Please rename the ran_simulator_handover.cpp to ran_simulator.cpp to run the handover based experiments. Follow identical runnning instruction (as of v_1.0) to start all EPC components. The handover procedure on completion at ran_simulator prints all performed steps and total time taken for the handover.
#### Developer Guide

Please find the document *developer_manual.pdf* under v_1.0 parent folder for understanding source code structure. The handover procedures and code changes are heavily commented by in-code documentation.


We have followed the following documentation to design our S1 based handover procedure.
[link](http://www.netmanias.com/en/post/techdocs/6286/emm-handover-lte/emm-procedure-6-handover-without-tau-part-3-s1-handover)
