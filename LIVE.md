# Internet live streams

The A2Stream server allows to listen to internet live streams (such as internet radio stations) with [A2Stream](README.md). This is a step-by-step guide for installing the A2Stream server on a Raspberry Pi:

* Install [Raspberry Pi OS](https://www.raspberrypi.org/software/).
* Open a shell on your Raspberry Pi.
* Get the A2Stream server source code with 
  ```
  wget https://raw.githubusercontent.com/oliverschmidt/A2Stream/main/srva2stream.c
  ```
* Compile the A2Stream server source code with
  ```
  gcc -o srva2stream srva2stream.c
  ```
* Check the compilation result with
  ```
  ls -l
  ```
* Install MPlayer with
  ```
  sudo apt install mplayer -y
  ```
* Find out the URL of the actual audio stream of the internet radio station you want to listen to. They tend to not publish their audio stream URL (anymore). For _106.7 Lite FM_ that URL is http://stream.revma.ihrhls.com/zc1477.
* Try to play the URL with
  ```
  mplayer http://stream.revma.ihrhls.com/zc1477
  ```
* If MPlayer prints __Starting playback...__ then the URL works. Stop MPlayer witch `Ctrl-C`.
* Open a second shell and run the A2Stream server on TCP port 80 with
  ```
  sudo ./srva2stream 80
  ```
* Wait for the the A2Stream server to print __waiting on /tmp/a2stream-80__.
* On the first shell, play the URL to the pipe just opened by the A2Stream server with
  ```
  sudo mplayer -nocache -af pan=1:0.5:0.5,volnorm=1:0.8,resample=22200:0:1,format=floatne -ao pcm:nowaveheader:file=/tmp/a2stream-80 http://stream.revma.ihrhls.com/zc1477
  ```
* On the second shell, wait for the A2Stream server to print __connect on pipe__.
* On your Apple II, run __A2STREAM.SYSTEM__ and enter the IP address of your Raspberry Pi as URL.
* If A2Stream plays _106.7 Lite FM_, then everything works :-) Stop MPlayer witch `Ctrl-C`.
* Now automate everything. First, create a service unit for the A2Stream server with
  ```
  sudo --preserve-env=HOME bash -c 'cat > /etc/systemd/system/a2stream.service << EOF
  [Unit]
  Description=a2stream
  After=network-online.target
  Requires=network-online.target

  [Service]
  ExecStart=$HOME/srva2stream 80

  [Install]
  WantedBy=multi-user.target
  EOF'
  ```
* Enable that service with
  ```
  sudo systemctl enable a2stream.service
  ```
* Create a service unit for MPlayer (depending on the the A2Stream service) with
* ```
  sudo bash -c 'cat > /etc/systemd/system/mplayer.service << EOF
  [Unit]
  Description=mplayer
  After=a2stream.service
  Requires=a2stream.service

  [Service]
  ExecStart=/usr/bin/mplayer -nocache -af pan=1:0.5:0.5,volnorm=1:0.8,resample=22200:0:1,format=floatne -ao pcm:nowaveheader:file=/tmp/a2stream-80 http://stream.revma.ihrhls.com/zc1477

  [Install]
  WantedBy=multi-user.target
  EOF'
  ```
* Enable that service with
  ```
  sudo systemctl enable mplayer.service
  ```
* Start the Mplayer service with
  ```
  sudo systemctl start mplayer.service
  ```
* Check that the A2Stream service was implicitly started (because of the dependency) with
  ```
  systemctl status a2stream.service
  ```
* Check that the two services come up after a reboot with
  ```
  sudo reboot
  ```
* Wait some time to allow the Raspberry Pi to come up.
* On your Apple II, re-enter the IP address of your Raspberry Pi as URL in A2Stream.
* If A2Stream plays _106.7 Lite FM_, then you're done :-))

Notes:

1. You can listen to additional internet radio stations by creating additional service pairs. Just use a different port number for each A2Stream service (resulting in a different pipe name to play to for each MPlayer service). On your Apple II, enter `<IP address>:<port number>` as URL in A2Stream.
2. You can have custom cover art on your Apple II for each A2Stream service. Just put standard 16kB double hires graphics files named `srva2stream-<port number>.dhgr` in your home directory.

Further Idea:

With a Raspberry Pi that has both a WiFi and an Ethernet interface, use the former for the internet connection and the latter for a direct short cable connection to your Uthernet II. Run [Dnsmasq](https://en.wikipedia.org/wiki/Dnsmasq) on the Ethernet interface to provide an IP address to A2Stream. Put the Raspberry Pi right inside your Apple II creating a music art object to place anywhere in your home or office.
