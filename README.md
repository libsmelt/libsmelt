# Pairwise

 - Compile `bench/pairwise.cpp`
 - Copy the binary to the target machine: `ssh -t $M ./pairwise | tee
   $DIR/$M/pairwise_raw`, where `$M` is the target machine and `$DIR`
   the directory of the NetOS machine database
 - Process raw output file with `sk_m_parser.py`
 - libsync's `./evaluate-pairwise.py <machine>` to read raw output and
   store in machine DB. 
 - `./generate-pairwise.py` in Simulator to render images
