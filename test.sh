# File:        test.sh
# Project:     PRL-2021-MPI-PMS
# Author:      Jozef Méry - xmeryj00@vutbr.cz
# Date:        2.4.2021
 

#!/bin/bash

# Original code taken from PRL wiki pages, edited.

# fixed amount of random numbers
numbers=16;
processes=5; # log2(n) + 1 = log2(16) + 1 = 4 + 1 = 5

# compile
mpic++ --prefix /usr/local/share/OpenMPI -o pms pms.cpp 

# generate file with random numbers
dd if=/dev/random bs=1 count=$numbers of=numbers

# run program
mpirun --prefix /usr/local/share/OpenMPI --host localhost:$processes -np $processes pms 

# cleanup
rm -f pms numbers