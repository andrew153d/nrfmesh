#!/bin/bash

# Ensure the script is run with sudo
if [ "$EUID" -ne 0 ]; then
  echo "Please run as root"
  exit
fi

# Install required packages
echo "Installing required packages..."
sudo apt-get install -y git cmake build-essential libtclap-dev pkg-config

# Clone the repository
echo "Cloning the repository..."
git clone https://github.com/andrew153d/nrfmesh.git || {
  echo "Repository already exists. Skipping clone."
}
# Change to the repository directory
cd nrfmesh || exit

# Run the NRF24 installation script
echo "Running NRF24 installation script..."
chmod +x nrfinstall.sh
./nrfinstall.sh

# Build the project
echo "Building the project..."
mkdir -p build
cd build
cmake ..
make -j$(nproc)
make install
echo "Installation complete!"