# Flipdot vlc video output plugin

* Compile flipdot library
* Compile vlc with `--enable-run-as-root`
  * or `apt-get install vlc libvlc-dev` and
    [patch](http://www.linuxquestions.org/questions/linux-general-1/solved-vlc-running-under-root-without-compiling-748189/)
    the binary
  * `sed -e 's/geteuid/getppid/' </usr/bin/vlc >./vlc`
* `make && sudo make install`
* `sudo ./vlc -V flipdot`
