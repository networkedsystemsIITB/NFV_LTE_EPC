### A Distributed Virtualized Evolved Packet Core for LTE Networks (vEPC 1.1)
vEPC 1.1 is a distributed implementation of vEPC 1.0 (released on June 10, 2016). This new release adds horizontal scaling capabilities to the previous version to accommodate increasing load. 
#### Synopsis

Network Function Virtualization (NFV) is a popular concept being proposed for redesigning various components in telecom networks. A bulk of NFV design proposals focus on EPC, which faces severe scalability and flexibility issues. This has set a new trend among network equipment manufacturing companies to build commercial implementations of vEPC. All such implementations try to achieve a highly scalable and flexible architecture to suit the needs of future mobile traffic. However, no complete open-source free vEPC exists to the best of our knowledge. vEPC 1.0 released on June 10, 2016 was the first step in this direction. To extend our previous work we designed and implemented a distributed version which can horizontally scale to accommodate increasing load. Note that our code is not fully standards compliant, and is not intended for commercial use. Our code is intended for consumption by researchers to build and evaluate various NFV-based distributed EPC architectures. We are working on a future version to include auto scaling and other enhancements. 

#### High-level Design of vEPC 1.1
The new improved vEPC design is based on the idea of replacing monolithic EPC components with a distributed and stateless version. Namely monolithic MME, SGW, and PGW components are replaced with their respective distributed version. The following figure shows the complete design. 
<html><center><img hspace="120" height="400px" width = "560px" src="/doc/doc_source/images/overview.png"><center></html>

#### Whats new in this version

- Stateless Mobility Management Entity
- Stateless Serving Gateway
- Stateless Packet Data Network Gateway
- Load balancer placement and configuration for stateless MME, SGW and PGW modules. 
- Home Subscriber Server with key value store
- Reliable shared key value store for MME, SGW and PGW replicas

#### Supported LTE procedures

- Initial Attach
- Authentication
- Create Session
- Modify Session
- Delete Session
- User plane data transfer

#### Required software packages/tools

- openvpn
- libsctp-dev
- openssl
- iperf3/iperf
- ipvsadm

Note: Above packages/tools correspond to Linux-based machines


#### Directory structure

- **src**: Contains source code files of vEPC 1.1
- **doc**: Contains documents related to vEPC 1.1
- **scripts**: Contains necessary scripts for installing and experimenting with vEPC 1.1

#### User Manual

Please find the document *user_manual.pdf* under the *doc* folder. It contains detailed instructions for running the code and experimenting under different traffic conditions.

#### Developer Manual

Please find the document *developer_manual.pdf* under the *doc* folder. It contains detailed instructions for understanding the codebase. It will be helpful while modifying/adding code functionalities to test out new ideas using our vEPC.

#### Authors

1. [Pratik Satapathy](https://www.cse.iitb.ac.in/~pratik/), Master's student (2015-2017), Dept. of Computer Science and Engineering, IIT Bombay.
2. [Prof. Mythili Vutukuru](https://www.cse.iitb.ac.in/~mythili/), Dept. of Computer Science and Engineering, IIT Bombay.

#### Contact

- Pratik Satapathy, pratik[AT]cse.iitb.ac.in
- Prof. Mythili Vutukuru, mythili[AT]cse.iitb.ac.in
