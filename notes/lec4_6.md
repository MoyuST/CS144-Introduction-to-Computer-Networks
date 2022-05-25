# lectures notes from 4 to 6

## Life of a Packet

### TCP Byte Stream

3-way handshake

1. SYN
2. SYN/ACK
3. ACK

Transport layers: transport data for applications (with port)
Network layers: transport data for hosts (with IP)

Inside each hop: use forwarding table (longgest matched prefix)

Under the hood:
display using wireshark and `traceroute -w 1 www.cs.brown.edu`

---

Principle: Packet Swithching

Packet: self-contained unit of data that carries information necessary for it to reach its destination

Packet switching: independently for each arriving packet, pick its outgoing link. if the link is free, send it. else hold the packet for later. each route holds information for next hold.

two consequence:
- simple packet for forwarding (route/switch only cares about IP/MAC address to forward packets)
- efficient sharing of links

no per-flow state required
- flow: a collection of datagrams belonging to the same end-to-end communication
- packet switches don't need state for each flow (swtich only need to forward them), thus no per-flow state to be added/removed/stored

efficient sharing of links
data traffic is bursty
- packet swithcing allows flows to use all available link capacity (e.g. if your friends not using internet while you are using, then your network traffic can be fully used by your device)
- packet switching allows flows to share link capacity (e.g. you and your friends use network together, the network link could forwarding packets from both of you but the relative transportation speed for each of you may be slow)
all above is called *Statistical Multiplexing*

---

Principle: Layering

layers are hieratical: 
- each layers handle specific functions and provide simple abstract interface for above layers
- each layer could be developed and improved independently
- but sometimes cross-layer design is needed (like c++ compiler for each platform/processor is needed to be implemented separately)