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
# 2. Ensure the clock uses NTS
## 2.1. Install chrony
Chrony is an NTP client with built-in NTS support. It replaces systemd-timesyncd.
```
# sudo apt update
# sudo apt install chrony
```
## 2.2. Configure chrony for NTS with Netnod servers
Edit `/etc/chrony/chrony.conf` and comment out existing pool/server lines, then add:
```
server sth1.nts.netnod.se nts iburst
server sth2.nts.netnod.se nts iburst
```
You can also use other Netnod NTS servers: `nts.netnod.se`, `gbg1.nts.netnod.se`, `mmo1.nts.netnod.se`, etc.
See https://www.netnod.se/netnod-time/how-to-use-nts for more details.
## 2.3. Enable and start chrony
```
# sudo systemctl enable chrony
# sudo systemctl restart chrony
```
## 2.4. Verify NTS is working
```
# chronyc -N sources
# chronyc -N authdata
```
The authdata command should show "NTS" in the authentication column.
# 3. Install wiringPi
The wiringPi library is required for GPIO access. It is maintained at https://github.com/WiringPi/WiringPi

Download the latest .deb package from the releases page and install:
```
# For 64-bit Raspberry Pi OS (arm64):
wget https://github.com/WiringPi/WiringPi/releases/download/3.14/wiringpi_3.14_arm64.deb
sudo apt install ./wiringpi_3.14_arm64.deb

# For 32-bit Raspberry Pi OS (armhf):
wget https://github.com/WiringPi/WiringPi/releases/download/3.14/wiringpi_3.14_armhf.deb
sudo apt install ./wiringpi_3.14_armhf.deb
```
Check https://github.com/WiringPi/WiringPi/releases for newer versions.

# 4. Build and install this package
```
# make
# sudo make install
```
# 5. Configure the display
## 5.1. Make sure the correct DisplayNixie binary is started at boot
Edit `/etc/rc.local` to your liking. Launch either `DisplayNixie` (for version 2) or `DisplayNixie3x` (for version 3) of the card.
## 5.2. Ensure clock is turned on when you want to
Edit `/etc/DisplayNixie.conf` to include the time intervals when the clock is to be turned on.

# 6. Creating a Bootable SD Card (macOS)

## 6.1. Using Raspberry Pi Imager (Recommended)
```
brew install --cask raspberry-pi-imager
```

1. Launch **Raspberry Pi Imager**
2. Select OS: **Raspberry Pi OS Lite (64-bit)** - no desktop needed for a headless clock
3. Select your SD card
4. Click the gear icon (⚙️) for advanced options:
   - Set hostname: `nixieclock`
   - Enable SSH
   - Set username/password
   - Configure WiFi
5. Click **Write**

## 6.2. Manual Method (dd)
```
# Download Raspberry Pi OS Lite
curl -LO https://downloads.raspberrypi.com/raspios_lite_arm64/images/raspios_lite_arm64-2024-11-19/2024-11-19-raspios-bookworm-arm64-lite.img.xz

# Extract
xz -d 2024-11-19-raspios-bookworm-arm64-lite.img.xz

# Find your SD card (BE CAREFUL - wrong disk = data loss!)
diskutil list

# Unmount the SD card (replace diskN with your disk number)
diskutil unmountDisk /dev/diskN

# Write the image (use rdiskN for faster writes)
sudo dd if=2024-11-19-raspios-bookworm-arm64-lite.img of=/dev/rdiskN bs=4m status=progress

# Eject
diskutil eject /dev/diskN
```

## 6.3. Enable SSH and WiFi (Manual method)
After writing, remount the SD card and:
```
# Enable SSH
touch /Volumes/bootfs/ssh

# Configure WiFi
cat > /Volumes/bootfs/wpa_supplicant.conf << 'EOF'
country=US
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1

network={
    ssid="YOUR_WIFI_SSID"
    psk="YOUR_WIFI_PASSWORD"
    key_mgmt=WPA-PSK
}
EOF
```

# 7. Read-Only Filesystem Setup (SD Card Longevity)

This software is ideal for a read-only root filesystem:
- Configuration (`/etc/DisplayNixie.conf`) is only read at startup
- Time is stored in the hardware RTC via I2C, not on the SD card
- No log files or state files are written during operation

Setting up a read-only filesystem dramatically extends SD card lifespan.

## 7.1. Using OverlayFS (Recommended)
```
sudo raspi-config
# Navigate to: Performance Options → Overlay File System → Enable
```

This makes the root filesystem read-only with a RAM-based overlay for any writes.

## 7.2. Manual Read-Only Setup

Create and run this setup script on the Pi:
```
#!/bin/bash
# setup-readonly.sh

set -e

# Disable swap
sudo dphys-swapfile swapoff
sudo dphys-swapfile uninstall
sudo systemctl disable dphys-swapfile

# Disable unnecessary services that write to disk
sudo systemctl disable apt-daily.timer
sudo systemctl disable apt-daily-upgrade.timer
sudo systemctl disable man-db.timer

# Add tmpfs mounts to fstab
sudo tee -a /etc/fstab << 'EOF'

# RAM-based filesystems for read-only root
tmpfs  /tmp              tmpfs  defaults,noatime,nosuid,size=64m           0  0
tmpfs  /var/log          tmpfs  defaults,noatime,nosuid,mode=0755,size=32m 0  0
tmpfs  /var/tmp          tmpfs  defaults,noatime,nosuid,size=32m           0  0
tmpfs  /var/lib/dhcpcd   tmpfs  defaults,noatime,nosuid,size=1m            0  0
tmpfs  /var/spool        tmpfs  defaults,noatime,nosuid,size=8m            0  0
EOF

# Configure journald for volatile storage
sudo sed -i 's/^#Storage=auto/Storage=volatile/' /etc/systemd/journald.conf
sudo sed -i 's/^#RuntimeMaxUse=/RuntimeMaxUse=16M/' /etc/systemd/journald.conf

# Create helper scripts for toggling read-write mode
sudo tee /usr/local/bin/rw << 'SCRIPT'
#!/bin/bash
sudo mount -o remount,rw /
echo "Root filesystem is now READ-WRITE"
SCRIPT

sudo tee /usr/local/bin/ro << 'SCRIPT'
#!/bin/bash
sync
sudo mount -o remount,ro /
echo "Root filesystem is now READ-ONLY"
SCRIPT

sudo chmod +x /usr/local/bin/rw /usr/local/bin/ro

# Make root read-only in fstab
sudo sed -i 's/defaults,noatime/defaults,noatime,ro/' /etc/fstab

echo "Setup complete. Reboot to apply."
echo "Use 'rw' to temporarily make filesystem writable for updates."
echo "Use 'ro' to make it read-only again."
```

## 7.3. Systemd Service File
Create `/etc/systemd/system/nixieclock.service`:
```
[Unit]
Description=Nixie Clock Display
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/sbin/DisplayNixie
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

Enable with:
```
sudo systemctl enable nixieclock
```

## 7.4. Installing on a Read-Only System
```
# Temporarily make writable
rw

# Install, configure, etc.
sudo apt update
sudo apt install chrony
# ... other setup ...

# Make read-only again
ro
```
