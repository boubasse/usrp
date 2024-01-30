# USRP Configuration

## Installation of UHD driver
This guide will explain how to properly install UHD driver on x86/ARM platforms.

### Prerequisite packages
Please install the following packages

For **v3.15 only**:
```shell
sudo apt-get install -y libboost-all-dev libusb-1.0-0-dev python-mako doxygen python-docutils cmake build-essential
```

For **version >4.0**:
```shell
sudo apt-get install -y autoconf automake build-essential ccache cmake cpufrequtils doxygen ethtool \
g++ git inetutils-tools libboost-all-dev libncurses5 libncurses5-dev libusb-1.0-0 libusb-1.0-0-dev \
libusb-dev python3-dev python3-mako python3-numpy python3-requests python3-scipy python3-setuptools \
python3-ruamel.yaml
```

### Building from source
```shell
git clone https://github.com/EttusResearch/uhd.git
cd uhd
git checkout v<version_number> # e.g. v3.15.0.0 or v4.5.0.0
cd host
mkdir build
cd build
cmake ../ # for x86 platforms
cmake -DNEON_SIMD_ENABLE=OFF ../ # for ARM platforms
make
make test
sudo make install
cd ~
sudo ldconfig
```

## Test the driver
On your platform, connect your USRP via USB or Ethernet, and run the following:
```shell
uhd_usrp_probe
```

It will output the characteristics of the USRP.
If you happen to get a warning about buffer size, modify the ``sysctl.conf`` accordingly.

To do so :

```shell
sudo nano /etc/sysctl.conf
```
And then append this line :
```shell
net.core.rmem_max=2426666 # or the value recommended by the warning
```

## IP configuration (for ethernet)

### Raspberry PI 4
Modify the file ``dhcpcd.conf`` using ``sudo nano /etc/dhcpcd.conf``, and add the following:

```shell
interface eht0
static ip_address=<ip_of_usrp_subnet>/24 # you can get it using uhd_usrp_probe
```
