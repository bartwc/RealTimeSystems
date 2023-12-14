#!/bin/bash
# Installs zephyr on your machine
# Will create the directories ~/zephyrproject and ~/zephyr-sdk
# Sorry for the inconvenience, feel free to remove these after RTS is done!
#
# Based on the Zephyr getting started guide:
# https://docs.zephyrproject.org/latest/develop/getting_started/

# Install dependencies
if grep -q Ubuntu /etc/os-release; then
    sudo apt update
    sudo apt install --no-install-recommends git cmake ninja-build gperf   ccache dfu-util device-tree-compiler wget   python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file   make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1 python3-venv
else
    echo "Please make sure the following is installed: git cmake ninja-build gperf   ccache dfu-util device-tree-compiler wget   python3-dev python3-pip python3-setuptools python3-tk python3-wheel python3-venv xz-utils file   make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1"
    read -p "Press enter to continue, or Ctrl-C to abort"
fi

# Get Zephyr and download python dependencies
mkdir -p ~/zephyrproject &&
git clone --depth=1 https://github.com/zephyrproject-rtos/zephyr ~/zephyrproject &&
python3 -m venv ~/zephyrproject/.venv &&
source ~/zephyrproject/.venv/bin/activate &&
pip install west &&
west init ~/zephyrproject/ &&
cd ~/zephyrproject/ &&
west update &&
west zephyr-export &&
pip install -r ~/zephyrproject/zephyr/scripts/requirements.txt &&

# Get Zephyr SDK
cd ~ &&
wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.1/zephyr-sdk-0.16.1_linux-x86_64.tar.xz &&
tar xvf zephyr-sdk-0.16.1_linux-x86_64.tar.xz &&
cd zephyr-sdk-0.16.1 &&
./setup.sh &&
sudo cp ~/zephyr-sdk-0.16.1/sysroots/x86_64-pokysdk-linux/usr/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d &&
sudo udevadm control --reload &&

# Build example sketch to see if everything works
cd ~/zephyrproject/zephyr &&
west build -p always -b stm32f4_disco samples/basic/blinky
