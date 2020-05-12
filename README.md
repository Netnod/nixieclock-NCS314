# nixieclock-NCS314
Enhanced software for the [GRA &amp; AFCH](https://gra-afch.com/) NIXIE tube clock. Changes include:

- Always use NTS for signed NTP to set the time, using Netnod NTS servers
- Display the IPv4 address of the wlan0 interface if you press the mode button
- Turn the display on/off when you press the mode button
- Do AntiPoison of the tubes last second of each hour
- Configure when the tubes should be turned on (one time interval per weekday)

This software can replace the DisplayNixie software distributed with the NCS314 package you get via this link:

https://gra-afch.com/catalog/nixie-tubes-clock-without-cases/nixie-tubes-clock-raspberry_pi_arduino-shield-ncs314-for-in-14-nixie-tubes-options-tubes-gps-remote-arduino/

The original code you can find in this repository, where you also see more information about the code on the clocks you get shipped to you.

https://github.com/afch/NixieClockRaspberryPi

The Nixie Tubes can have a card of version 2.x or 3.x. The two versions of the DisplayNixie binary (DisplayNixie and DisplayNixie3x) in this repository are for each one of the cards.

To install and build the clock do as follows:

# 1. Get the clock
Buy the clock, follow instructions to do base install of the RaspberryPi and software for the clock. Ensure the clock works. Look at the instructions in this repository if you have problems:
https://github.com/afch/NixieClockRaspberryPi
# 2. Ensure the clock use NTS
## 2.1. Turn off systemd timesyncd
```
# systemctl stop systemd-timesyncd
# systemctl disable systemd-timesyncd
```
## 2.2. Install NTSclient
### 2.2.1. Install a reasonable new version of go
```
# wget https://dl.google.com/go/go1.14.2.linux-armv6l.tar.gz
# sudo tar -C /usr/local -xvf go1.14.2.linux-armv6l.tar.gz
```
### 2.2.2. Clone git repo, make and test
```
# git clone https://gitlab.com/hacklunch/ntsclient
# cd ntsclient
# make
# ./ntsclient --dry-run --config=./ntsclient.toml
:
^C
```
### 2.2.3. Install
```
# sudo setcap CAP_SYS_TIME+ep ./ntsclient
# sudo cp ntsclient /usr/bin/
# cp ntsclient.toml /etc/
# cp contrib/ntsclient.service /etc/systemd/system
# systemctl enable ntsclient
# systemctl start ntsclient
```
# 3. Build and install this package
```
# make
# sudo make install
```
# 4. Configure the display
## 4.1. Make sure the correct DisplayNixie binary is started at boot
Edit `/etc/rc.local` to your liking. Launch either `DisplayNixie` (for version 2) or `DisplayNixie3x` (for version 3) of the card.
## 4.2. Ensure clock is turned on when you want to
Edit `/etc/DisplayNixie.conf` to include the time intervals when the clock is to be turned on.
