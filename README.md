DPI with DPDK-acceleration.

Current limitations:

1) Only Ethernet/IP(4/6)/TCP(UDP) packets supported. Packets with VLAN supported too, but MPLS packets not supported for input.

2) Can't change rules in real-time. Static configuration with next available commands: drop, push-vlan, push-mpls, output.

3) One RX and TX queue.
