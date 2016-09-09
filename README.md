# PPSP
Peer to peer streaming protocol (PPSP) is developped by IETF PPSP WG.

PPSP is composed of the following  protocols
* Peer protocol (PPSPP, IETF RFC 7574)
* Tracker protocol (PPSTP, IETF RFC 7846)

#Description
This is a prototype of PPSPP based on IETF RFC 7574.

This implementation has the following changes that are required for proper operations.
* TBD

#Usage
## Compile
Compile source files in "src\EtriPPSP" directory.

* Windows
  + Use .sln file
* Linux
  + Use makefile
* MacOS
  + Use makefile with "make mac"

## Configuration
Please refer "ppsp.conf" file to adjust the following parameters.
* version
* content integriry protection method
* merkle hash tree function
* chunk addressing method
* chunk size
* keepalie interval
* retry count

## Run 
### Parameters
* -h, --hash: tswarm ID of the content (root hash or public key).
* -f, --file: tname of file to use (root hash by default)
* -l, --listen: [ip:]port to listen to. MUST set for IPv4.
* -t, --tracker: port of the tracker.
* -d, --debug: save debug logs.
* -o, --listening port option on: include listening port in HandShake.

### Examples
* Seed
 + Start seed with file. (Port only)
   - EtriPPSP.exe -f xxx.mp4 -l 20000
 + Start seed with file. (Port only) + debug logs
   - EtriPPSP.exe -f xxx.mp4 -l 20000 -d
 + Start seed with file. (IP & Port)
   - EtriPPSP.exe -f xxx.mp4 -l 192.168.0.2:20000
 + Start seed with file. (IP & Port) + listening port option
   - EtriPPSP.exe -f xxx.mp4 -l 192.168.0.2:20000 -o

* Normal peer
 + Assume that "e13dd7bd5c7d4f60f7c598da27cd669af6576680e13dd7bd5c7d4f60f7c598da" is a roothash for xxx.mp4
 + Start peer with tracker(Listen port)
   - EtriPPSP.exe -t 192.168.0.2:20000 -h e13dd7bd5c7d4f60f7c598da27cd669af6576680e13dd7bd5c7d4f60f7c598da -l 20001 -f xxx.mp4
 + Start peer with tracker(Listen port). + debug logs
   - EtriPPSP.exe -t 192.168.0.2:20000 -h e13dd7bd5c7d4f60f7c598da27cd669af6576680e13dd7bd5c7d4f60f7c598da -l 20001 -f xxx.mp4 -d
 + Start peer with tracker(Listen IP & port)
   - EtriPPSP.exe -t 192.168.0.2:20000 -h e13dd7bd5c7d4f60f7c598da27cd669af6576680e13dd7bd5c7d4f60f7c598da -l 192.168.0.3:20001 -f xxx.mp4
  
   





