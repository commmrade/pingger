## Pinging utility for Linux, that allows you to..ping, well, something

## How to use it?
It is pretty simple - just: `pingger <hostname>`
Let's try to use it now: 
```
pingger google.com
Size: 48, IP: 142.250.150.138; Ping - 23.01ms, Sequence: 1, TTL: 108
```
## Build and install
How to install this utility you may ask? Easy:
```
git clone https://github.com/commmrade/pingger
cd pingger
mkdir build && cd build
cmake ..
sudo make install
```
Now, you are supposed to have this installed, run: `pingger <hostname>`