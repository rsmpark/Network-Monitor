# Central Network Monitoring System

The program follows the [**Software Defined Networking model**](https://www.ciena.com/insights/what-is/What-Is-SDN.html) to centralize all intelligence in order to coordinate the operations of all network devices. 
The network monitor serves as the central server while the interface monitors act as the transferring layer where all work is done communicating with the network devices.



## Motivation

Monitoring network interfaces can help in debugging system problems as well as enhancing system performance through a central intelligent networking device.
For routers and other networking equipment, it is crucial to know the performance of each interface and to control each interface.
The networking monitor program allows developers to view logs of status reports from each interface monitors the host machine has.


## Design

### Network Monitor
The network monitor acts as a central server controlling all interface monitors running on the system. The network monitor is responsible for maintaining the connections and creating child processes for each interface monitor specified by the user. It will print out the statistic for each interface every second and will attempt to set the link back up if one of the monitor operating state goes down.

### Interface Monitor
For each interface monitor requested by the user, a process will be forked to handle gathering data and relaying the information back to the network monitor server. The interface monitor directly communicates with the network devices to retreive its status and manage its connection. The status retrieved includes the following:

- operating state – **operstate**
- up-count (number of times the interface has been up) – **carrier_up_count**
- The down-count (number of times the interface has been down) – **carrier_down_count**
- The number of received bytes – **rx_bytes**
- The number of dropped received bytes – **rx_dropped**
- The number of erroneous received bytes – **rx_errors**
- The number of received packets – **rx_packets**
- The number of transmitted bytes – **tx_bytes**
- The number of dropped transmitted bytes – **tx_dropped**
- The number of erroneous transmitted bytes – **tx_errors**
- The number of transmitted packets – **tx_packets**

<p align="center">
  <img width="560" height="400" src="https://github.com/rsmpark/Network-Monitor/blob/main/readme_res/architecture.png">
</p>


The system utilizes **UDP** sockets to communicate messages and manages client connections via `select()`. The communications between the network monitor and its interface monitors is synchronous, in that whenever the network monitor writes to an interface monitor, it waits to read something back before writing again.

## Demo

`make all`: compile the entire program

`make clean`: remove all object files

`sudo ./networkMon`: run network monitor system



<p align="center">
  <img width="1200" height="540" src="https://github.com/rsmpark/Network-Monitor/blob/main/readme_res/demo1.png">
</p>
<p align = "center">
  <b>Monitoring network devices available on my Linux system</b> - use <code>ifconfig</code> to find availble network devices
</p>
<br>


<p align="center">
  <img width="1200" height="540" src="https://github.com/rsmpark/Network-Monitor/blob/main/readme_res/demo2.png">
</p>
<p align = "center">
  <b>Manually shutting down network device to have the server set it back up
</p>

