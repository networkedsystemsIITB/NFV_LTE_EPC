#!/bin/bash
tar -zxvf redis-3.2.4.tar.gz
cd redis-3.2.4
make -j12
make install

