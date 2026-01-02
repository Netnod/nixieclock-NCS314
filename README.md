# nixieclock-NCS314
Enhanced software for the [GRA & AFCH](https://gra-afch.com/) NIXIE tube clock. Changes include:

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

# Installation Guide

## 1. Create Bootable SD Card (macOS)

### 1.1. Using Raspberry Pi Imager (Recommended)
```
brew install --cask raspberry-pi-imager
```

1. Launch **Raspberry Pi Imager**
2. Select OS: **Raspberry Pi OS Lite (64-bit)** - no desktop needed for a headless clock
3. Select your SD card
4. Click the gear icon for advanced options:
   - Set hostname: `nixieclock`
   - Enable SSH
   - Set username/password
   - Configure WiFi
5. Click **Write**

### 1.2. Manual Method (dd)
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
```

After writing, remount the SD card and configure SSH and WiFi:
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

# Eject
diskutil eject /dev/diskN
```

## 2. First Boot and Initial Setup

Insert SD card into Raspberry Pi and power on. SSH into the Pi:
```
ssh username@nixieclock.local
```

### 2.1. Fix Locale (if connecting from non-English system)
If you see locale warnings like `LC_CTYPE: cannot change locale`, generate the required locale:
```
sudo sed -i 's/^# *sv_SE.UTF-8/sv_SE.UTF-8/' /etc/locale.gen
sudo locale-gen
```
Replace `sv_SE.UTF-8` with your locale if different. Log out and back in for the fix to take effect.

### 2.2. Update System and Install Required Packages
```
sudo apt update
sudo apt upgrade -y
sudo apt install git chrony
```

## 3. Enable I2C and SPI

The clock hardware requires both I2C (for RTC) and SPI (for display):
```
sudo raspi-config nonint do_i2c 0
sudo raspi-config nonint do_spi 0
```

Verify the interfaces are enabled:
```
ls /dev/i2c* /dev/spidev*
```
You should see `/dev/i2c-1`, `/dev/spidev0.0`, and `/dev/spidev0.1`.

## 4. Configure NTS (Network Time Security)

### 4.1. Configure chrony for NTS with Netnod servers
Edit `/etc/chrony/chrony.conf` and comment out existing pool/server lines, then add:
```
server sth1.nts.netnod.se nts iburst
server sth2.nts.netnod.se nts iburst
```
You can also use other Netnod NTS servers: `nts.netnod.se`, `gbg1.nts.netnod.se`, `mmo1.nts.netnod.se`, etc.
See https://www.netnod.se/netnod-time/how-to-use-nts for more details.

### 4.2. Enable and start chrony
```
sudo systemctl enable chrony
sudo systemctl restart chrony
```

### 4.3. Verify NTS is working
```
chronyc -N sources
sudo chronyc -N authdata
```
The authdata command should show "NTS" in the authentication column.

## 5. Install wiringPi

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

Verify installation:
```
gpio -v
```

## 6. Build and Install the Clock Software

```
git clone https://github.com/Netnod/nixieclock-NCS314.git
cd nixieclock-NCS314
make
sudo make install
```

## 7. Configure the Display

### 7.1. Create configuration file
```
sudo cp /etc/DisplayNixie.conf.example /etc/DisplayNixie.conf
sudo nano /etc/DisplayNixie.conf
```
Edit the time intervals when the clock should be turned on.

### 7.2. Create systemd service
Create `/etc/systemd/system/nixieclock.service`:
```
sudo tee /etc/systemd/system/nixieclock.service << 'EOF'
[Unit]
Description=Nixie Clock Display
After=network.target time-sync.target
Wants=time-sync.target

[Service]
Type=simple
ExecStart=/usr/local/sbin/DisplayNixie -c /etc/DisplayNixie.conf
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF
```

Note: Use `DisplayNixie` for card version 2.x or `DisplayNixie3x` for card version 3.x.

### 7.3. Enable and start the service
```
sudo systemctl daemon-reload
sudo systemctl enable nixieclock
sudo systemctl start nixieclock
```

### 7.4. Check status
```
sudo systemctl status nixieclock
```

## 8. Read-Only Filesystem Setup (Recommended)

Setting up a read-only filesystem dramatically extends SD card lifespan. This software is ideal for read-only operation:
- Configuration (`/etc/DisplayNixie.conf`) is only read at startup
- Time is stored in the hardware RTC via I2C, not on the SD card
- No log files or state files are written during operation

### 8.1. Using OverlayFS (Recommended)

Interactive method:
```
sudo raspi-config
# Navigate to: Performance Options -> Overlay File System -> Enable
```

Or using command line:
```
sudo raspi-config nonint do_overlayfs 0
sudo reboot
```

This makes the root filesystem read-only with a RAM-based overlay for any writes. The actual SD card is mounted read-only at `/media/root-ro`, while all writes go to RAM.

### 8.2. Making Persistent Changes

With overlay enabled, changes to system files are lost on reboot. To make persistent changes (e.g., edit `/etc/chrony/chrony.conf` or install packages):

```
# Disable overlay
sudo raspi-config nonint do_overlayfs 1
sudo reboot

# After reboot, make your changes...
# Then re-enable overlay
sudo raspi-config nonint do_overlayfs 0
sudo reboot
```

Note: `0` = enable overlay (read-only), `1` = disable overlay (read-write)

### 8.3. Manual Read-Only Setup (Alternative)

If you prefer manual control, run these commands:
```
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

# Make root read-only in fstab (edit the line with your root partition)
sudo sed -i 's/defaults,noatime/defaults,noatime,ro/' /etc/fstab
```

Reboot to apply. Use `rw` to temporarily make filesystem writable for updates, and `ro` to make it read-only again.

## 9. Verify Installation

After reboot, verify everything is working:
```
# Check clock service
sudo systemctl status nixieclock

# Check NTS time sync
chronyc -N sources
sudo chronyc -N authdata

# Check I2C and SPI devices
ls /dev/i2c* /dev/spidev*
```

The clock display should now be showing the time according to your configuration.
