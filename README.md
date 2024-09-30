# PCOM

This is the repository that contains projects related to Comunication Protocols.

## Client-Server Model
A project that contains a server that works in the following way:
-It has to types of clients, subscribers, that use TCP connections and udp_clients.
-Each subscriber can subscribe to different topics.
-When an udp_client sends a message with a topic that a subscriber is subscribed to, than that TCP client will get the message aswell.

## HTTP Client
A HTTP client that connects to a server that emulates an electronic library. The client can create a new account, log in into an existing one and also be able to add, get and delete books from said library once it has loged in.

## Router
A router that reads a routing table from a file and than routs IPv4 packages sent to it. It also uses the ARP protocol to find and fill its MAC address table, and can also accept ICMP messages sent to it.
