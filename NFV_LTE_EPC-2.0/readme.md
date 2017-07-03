NFV_LTE_EPC 2.0
=================
### A Distributed Virtualized Evolved Packet Core for LTE Networks (vEPC 2.0)
vEPC 2.0 is a distributed implementation of LTE EPC. This new release adds horizontal scaling capabilities to the previous version to accommodate increasing load. 

#### High-level Design of vEPC 2.0

The new improved vEPC 2.0 design is based on the idea of buidling a spectrum of different distributed designs. Each of these designs touches extremities and middle-grounds of state synchronization options. These state synchronization modes are designed for both control and data plane operations.

#### Whats new in this version

- Different synchronization modes (Always sync, Session sync, No sync)

#### Supported LTE procedures

- Initial Attach
- Authentication
- Create Session
- Modify Session
- Delete Session
- User plane data transfer

#### Required software packages/tools

-packages listed in scripts/install.sh

Note: Above packages/tools correspond to Linux-based machines


#### Directory structure

- **src**: Contains source code files of vEPC 2.0
- **doc**: Contains documents related to vEPC 2.0
- **scripts**: Contains necessary scripts for installing and experimenting with vEPC 2.0

#### User Manual

Please find the document *user_manual_2.0.pdf* under the *doc* folder. It contains detailed instructions for running the code and experimenting under different traffic conditions.

#### Developer Manual

Please find the document *developer_manual_2.0.pdf* under the *doc* folder. It contains detailed instructions for understanding the codebase. It will be helpful while modifying/adding code functionalities to test out new ideas using our vEPC.

#### Authors

1. [Pratik Satapathy](https://www.linkedin.com/in/pratik-satapathy-5b175524/), Master's student (2015-2017), Dept. of Computer Science and Engineering, IIT Bombay.
2. [Jash Kumar Dave](https://www.linkedin.com/in/jash-dave-5698124b/), Master's student (2015-2017), Dept. of Computer Science and Engineering, IIT Bombay.
3. [Prof. Mythili Vutukuru](https://www.cse.iitb.ac.in/~mythili/), Dept. of Computer Science and Engineering, IIT Bombay.

#### Contact

- Pratik Satapathy, pratiksatapathy[AT]gmail.com
- Jash Dave, jashdave23[AT]gmail.com
- Prof. Mythili Vutukuru, mythili[AT]cse.iitb.ac.in
