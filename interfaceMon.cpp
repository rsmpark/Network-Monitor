#include <cstring>
#include <fstream>
#include <iostream>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

const int NAME_LEN = 16;
const int STAT_LEN = 128;
const int BUF_LEN = 100;
char buf[3 * STAT_LEN];
bool isRunning = true;
bool intfLinkDown = true;
char socket_path[] = "/tmp/a1";
int fd;

// Signal handler
static void signalHandler(int signum) {
  switch (signum) {
  case SIGINT:
#ifdef DEBUG
    cout << "interface monitor"
         << " [" << getpid() << "] signalHandler(" << signum << "): SIGINT"
         << endl;
#endif
    cout << "interface monitor shutting down" << endl;
    write(fd, "Done", 5);
    isRunning = false;
    break;
  default:
    cout << "signalHandler(" << signum << "): unknown" << endl;
  }
}

// Set up a signal handler to terminate the program gracefully
static void setupSigAction() {
  struct sigaction action;
  action.sa_handler = signalHandler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGINT, &action, NULL);
}

// Set link up for interface monitor with operstate down
static void setInterfaceLinkUp(char *interface, int fd) {
  cout << "intfMonitor [" << interface << "] pid[" << getpid() << "]: Link Down"
       << endl;

  // Notify server link is down
  int ret = write(fd, "Link Down", 10);
  if (ret == -1) {
    cout << "intfMonitor [" << interface << "] pid[" << getpid() << "] "
         << strerror(errno) << endl;
  }

  ret = read(fd, buf, BUF_LEN);

  // Receive confirmation
  if (strncmp("Set Link Up", buf, 11) == 0) {
    cout << "intfMonitor [" << interface << "] pid[" << getpid()
         << "]: Setting Link Up" << endl;

    // Prepare system call to turn on interface monitor link
    struct ifreq ifr;
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&ifr, 0, sizeof ifr);

    strncpy(ifr.ifr_name, "lo", IFNAMSIZ);

    ifr.ifr_flags |= IFF_UP;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) == -1) {
      cout << "intfMonitor - " << interface << " [" << getpid() << "] "
           << strerror(errno) << endl;
    } else {
      intfLinkDown = false;
      cout << "intfMonitor [" << interface << "] pid[" << getpid()
           << "]: Link Up" << endl;
    };
  }
}

// Retrieve interface information from sys/class/net/<interface> and print it to
// terminal
static void printInterfaceStats(char *interface) {
  char statPath[2 * STAT_LEN];
  int retVal = 0;
  ifstream infile;

  // Interface information
  char operstate[NAME_LEN];
  int carrier_down_count = 0;
  int carrier_up_count = 0;
  int rx_bytes = 0;
  int rx_dropped = 0;
  int rx_errors = 0;
  int rx_packets = 0;
  int tx_bytes = 0;
  int tx_dropped = 0;
  int tx_errors = 0;
  int tx_packets = 0;

  // Carrier down count
  sprintf(statPath, "/sys/class/net/%s/carrier_down_count", interface);
  infile.open(statPath);
  if (infile.is_open()) {
    infile >> carrier_down_count;
    infile.close();
  }

  // Carrier up count
  sprintf(statPath, "/sys/class/net/%s/carrier_up_count", interface);
  infile.open(statPath);
  if (infile.is_open()) {
    infile >> carrier_up_count;
    infile.close();
  }

  // Operstate (Link status)
  sprintf(statPath, "/sys/class/net/%s/operstate", interface);
  infile.open(statPath);
  if (infile.is_open()) {
    infile >> operstate;

    if (strncmp("down", operstate, 2) == 0) {
      intfLinkDown = true;
    } else {
      intfLinkDown = false;
    }
    infile.close();
  }

  // Rx bytes
  sprintf(statPath, "/sys/class/net/%s/statistics/rx_bytes", interface);
  infile.open(statPath);
  if (infile.is_open()) {
    infile >> rx_bytes;
    infile.close();
  }

  // Rx dropped
  sprintf(statPath, "/sys/class/net/%s/statistics/rx_dropped", interface);
  infile.open(statPath);
  if (infile.is_open()) {
    infile >> rx_dropped;
    infile.close();
  }

  // Rx errors
  sprintf(statPath, "/sys/class/net/%s/statistics/rx_errors", interface);
  infile.open(statPath);
  if (infile.is_open()) {
    infile >> rx_errors;
    infile.close();
  }

  // Rx packets
  sprintf(statPath, "/sys/class/net/%s/statistics/rx_packets", interface);
  infile.open(statPath);
  if (infile.is_open()) {
    infile >> rx_packets;
    infile.close();
  }

  // Tx bytes
  sprintf(statPath, "/sys/class/net/%s/statistics/tx_bytes", interface);
  infile.open(statPath);
  if (infile.is_open()) {
    infile >> tx_bytes;
    infile.close();
  }

  // Tx dropped
  sprintf(statPath, "/sys/class/net/%s/statistics/tx_dropped", interface);
  infile.open(statPath);
  if (infile.is_open()) {
    infile >> tx_dropped;
    infile.close();
  }

  // Tx errors
  sprintf(statPath, "/sys/class/net/%s/statistics/tx_errors", interface);
  infile.open(statPath);
  if (infile.is_open()) {
    infile >> tx_errors;
    infile.close();
  }

  // Tx packets
  sprintf(statPath, "/sys/class/net/%s/statistics/tx_packets", interface);
  infile.open(statPath);
  if (infile.is_open()) {
    infile >> tx_packets;
    infile.close();
  }

  // Write data to the terminal
  char data[3 * STAT_LEN];
  int len = sprintf(data,
                    "\nInterface:%s state:%s up_count:%d down_count:%d\n"
                    "rx_bytes:%d rx_dropped:%d rx_errors:%d rx_packets:%d\n"
                    "tx_bytes:%d tx_dropped: %d tx_errors:%d tx_packets:%d\n",
                    interface, operstate, carrier_up_count, carrier_down_count,
                    rx_bytes, rx_dropped, rx_errors, rx_packets, tx_bytes,
                    tx_dropped, tx_errors, tx_packets);
  cout << data;
  sleep(1);
}

int main(int argc, char *argv[]) {
  // Get interface monitor name
  char interface[NAME_LEN];
  strncpy(interface, argv[1], NAME_LEN);

  // Setup sigaction and handlers
  setupSigAction();

  // Set up socket communications
  struct sockaddr_un addr;
  int len, ret;
  int rc;

#ifdef DEBUG
  cout << "DEBUG: intfMonitor - " << interface << " [" << getpid() << "] "
       << " running..." << endl;
#endif

  // Create the socket
  memset(&addr, 0, sizeof(addr));
  if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    cout << "intfMonitor - " << interface << " [" << getpid() << "] "
         << strerror(errno) << endl;
    exit(-1);
  }

  addr.sun_family = AF_UNIX;
  // Set the socket path to a local socket file
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

#ifdef DEBUG
  cout << "DEBUG: intfMonitor - " << interface << " socket fd " << fd
       << " bound to address " << addr.sun_path << endl;
#endif

  // Connect to the local socket
  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    cout << "intfMonitor - " << interface << " [" << getpid() << "] "
         << strerror(errno) << endl;
    close(fd);
    exit(-1);
  }

#ifdef DEBUG
  cout << "DEBUG: intfMonitor - " << interface << " connect()" << endl;
#endif

  // Begin handshake with parent processes to monitor interface
  // Handshake: Child(Ready) -> Parent(Monitor) -> Child(Monitoring)
  ret = write(fd, "Ready", 6);

  ret = read(fd, buf, BUF_LEN);

#ifdef DEBUG
  cout << "DEBUG: interfaceMonitor[" << interface << "] pid[" << getpid()
       << "] read(buf:" << buf << ")" << endl;
#endif

  if (ret == -1) {
    cout << "intfMonitor [" << interface << "] pid[" << getpid() << "] "
         << strerror(errno) << endl;
  }

  if (strncmp("Monitor", buf, 7) == 0) {
    // Begin monitoring interface monitor status
    ret = write(fd, "Monitoring", 11);

    if (ret == -1) {
      cout << "intfMonitor [" << interface << "] pid[" << getpid() << "] "
           << strerror(errno) << endl;
    }

    sleep(1);

    while (isRunning) {
      // Print interface information
      printInterfaceStats(interface);

      if (intfLinkDown) {
        // Set interface link back up if down
        setInterfaceLinkUp(interface, fd);
      }
    }
  }

#ifdef DEBUG
  cout << "DEBUG: intfMonitor - " << interface << " stopping..." << endl;
#endif

  return 0;
}