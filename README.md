
**Home** | [Download](https://github.com/fakechroot/fakechroot/wiki/Download) | [Documentation](https://github.com/fakechroot/fakechroot/blob/master/man/fakechroot.pod) | [ChangeLog](https://github.com/fakechroot/fakechroot/blob/master/NEWS.md) | [Development](https://github.com/fakechroot/fakechroot/wiki/Development) | [ToDo](https://github.com/fakechroot/fakechroot/wiki/Todo) | [Links](https://github.com/fakechroot/fakechroot/wiki/Links)

# What is it?

fakechroot runs a command in an environment were is additional possibility to
use `chroot`(8) command without root privileges.  This is useful for allowing
users to create own chrooted environment with possibility to install another
packages without need for root privileges.


# This is a customized fakechroot

We add some features to the original fakechroot and allow the library to check incoming syscalls based on predefined privileges. 

Therefore, to compile the source code you might need at least the following dependencies:

1. git
2. autoconf
3. automake
4. make
5. gcc
6. g++
7. libmemcached-dev 
8. cmake
9. libtool

If you could directly install msgpack-c via your package manager, it will be good and you don't need cmake. For example, for arch linux, one could directly install msgpack-c package from AUR by executing 'yaourt -S msgpack-c'. For other distros, such as ubuntu, you may need to compile msgpack-c from source by following the steps:

```
git clone https://github.com/msgpack/msgpack-c.git
cd msgpack-c
cmake .
make
sudo make install
```

After these steps, you could start compiling fakechroot.

```
git clone https://github.com/JasonYangShadow/fakechroot
cd fakechroot
./autogen.sh
./configure
make
```

Done! Congratulations!

**After executing make commands, you could find compiled library under fakechroot/src/.libs/libfakechroot.so. You then copy it to the folder including lpmx program. lpmx will automatically copy libfakechroot.so to target container while starting container.**
