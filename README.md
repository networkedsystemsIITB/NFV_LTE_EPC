### Virtualized Evolved Packet Core for LTE Networks (vEPC)

vEPC is a simple virtualized form of Long Term Evolution Evolved Packet Core (LTE EPC). It simulates the working of a typical EPC for handling signaling and data traffic. vEPC is developed in C++11. Current Version is 1.0, and it is released on June 10, 2016.

#### Synopsis

Network Function Virtualization (NFV) is a popular concept in redesigning the radio and core parts of the Mobile Networks. A bulk of NFV design proposals focus on the underlying EPC architecure, which faces severe scalability and flexibility issues. This has set a new trend among Network Equipment manufacturing companies to built commercial implementations of vEPC. All such implementations try to achieve a highly scalable and flexible architecture to suit the needs of future mobile traffic. Although our vEPC does not capture all 3GPP standard specifications and LTE procedures as compared with commercial implementations, it can really be helpful for people across the research domain to experiment with their NFV ideas using our implementation.

#### List of developed software modules

- Mobility Management Entity
- Home Subscriber Server
- Serving Gateway
- Packet Data Network Gateway
- Radio Access Network Simulator
- Sink node

#### Supported LTE procedures

- Initial Attach
- Authentication
- Location update
- Create Session
- Modify Session
- Delete Session
- User plane data transfer

#### Required software packages/tools

- mysql-server
- libmysqlclient-dev
- openvpn
- libsctp-dev
- openssl
- iperf3/iperf

Note: Above packages/tools correspond to linux-based machines

#### Directory structure

- **src**: Contains source code files
- **bin**: Contains binary executables
- **doc**: Contains documents related to vEPC
- **scripts**: Contains necessary scripts for using vEPC

#### User Manual

Please find the document *user_manual.pdf* under the *doc* folder. It contains detailed instructions for running the code and experimenting under different traffic conditions.

#### Developer Manual

Please find the document *developer_manual.pdf* under the *doc* folder. It contains detailed instructions for understanding the codebase. It will be helpful while modifying/adding code functionalities to test out new ideas using our vEPC.

#### Authors

1. [Sadagopan N S](https://www.linkedin.com/in/sadagopan-n-s-b8184a61), Master's student (2014-2016), Dept. of Computer Science and Engineering, IIT Bombay.
2. [Prof. Mythili Vutukuru](https://www.cse.iitb.ac.in/~mythili/), Dept. of Computer Science and Engineering, IIT Bombay.

#### Contact

- Sadagopan N S, sadagopannscbe[At]gmail.com
- Prof. Mythili Vutukuru, mythili[AT]cse.iitb.ac.in
