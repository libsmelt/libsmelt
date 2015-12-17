HOST=ziger2

SSH_OPTS="-o ForwardAgent=yes -o StrictHostKeyChecking=no"

ssh $HOST $SSH_OPTS /bin/bash <<'ENDSSH'
set -e

echo deb http://archive.ubuntu.com/ubuntu/ wily universe | sudo tee /etc/apt/sources.list.d/universe.list
echo 'Acquire::http::proxy "http://proxybd.ethz.ch:3128";' | sudo tee /etc/apt/apt.conf.d/30proxy
sudo apt-get update
sudo apt-get -y install libnuma-dev numactl

ENDSSH

#BF_DIR=barrelfish
#BF_REPO=ssh://vcs-user@code.systems.ethz.ch:8006/diffusion/BFI/barrelfish.git
#BF_BRANCH=infiniband
#TUP=~/tup/tup


#git config --global --add user.email moritzho@inf.ethz.ch
#git config --global --add user.name 'Moritz Hoffmann'
#pushd tup
#./bootstrap.sh
#popd

#echo 'ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $*' > ssh
#chmod +x ssh
#GIT_SSH='./ssh' git clone $BF_REPO $BF_DIR || true
#pushd $BF_DIR
#git checkout $BF_BRANCH || true
#./tools/tup/bootstrap.sh || true
#$TUP init
#$TUP variant tools/tup/x86_64.config || true
#$TUP

