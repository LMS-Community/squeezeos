
# Jive will run this script from the SD card after a factory reset.
# You can use this to customize the filesystem for development.

# Set default wireless access point
cp firstboot/wpa_supplicant.conf /etc

# Enable ssh access
sed -e 's/^#ssh/ssh/g' /etc/inetd.conf > /etc/inetd.conf.tmp
mv /etc/inetd.conf.tmp /etc/inetd.conf

# Install ssh keys
mkdir /root/.ssh
cp firstboot/id_rsa.pub /root/.ssh/authorized_keys
chmod g-w,o-w /root
chmod g-w,o-w /root/.ssh
chmod g-w,o-w /root/.ssh/authorized_keys

