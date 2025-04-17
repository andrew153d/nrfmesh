SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
sudo apt-get install git cmake build-essential libtclap-dev pkg-config
sudo rm /usr/local/lib/librf24.*
sudo rm /usr/local/lib/librf24-bcm.so
sudo rm -r /usr/local/include/RF24
#rm nrfinstall.sh
#wget -O nrfinstall.sh https://raw.githubusercontent.com/nRF24/.github/main/installer/install.sh
cd $SCRIPT_DIR
sudo chmod +x nrfinstall.sh
./nrfinstall.sh
# git clone git@github.com:aarossig/nerfnet.git
# cd nerfnet
mkdir build
cd build
cmake ..
make -j$(nproc)