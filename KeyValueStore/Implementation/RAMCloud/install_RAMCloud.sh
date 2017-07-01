sudo apt-get install build-essential git-core doxygen libpcre3-dev protobuf-compiler libprotobuf-dev libcrypto++-dev libevent-dev libboost-all-dev libgtest-dev libzookeeper-mt-dev zookeeper libssl-dev -y
sudo add-apt-repository ppa:webupd8team/java -y
sudo apt-get update
sudo apt-get install oracle-java8-installer
sudo apt-get install oracle-java8-set-default
#git clone "https://github.com/PlatformLab/RAMCloud.git"
cd RAMCloud
#git checkout 7cba5e2de394d1fbd62965f2cff367431eb12c11
git submodule update --init --recursive
ln -s ../../hooks/pre-commit .git/hooks/pre-commit
make -j16
sudo make install -j16
