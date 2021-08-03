#include <iostream>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

using namespace std;

char socket_path[] = "/tmp/a1";
const int BUF_LEN = 100;
const int NAME_SIZE = 16;
int maxIntfMon;
char **intfMonNames;
bool isRunning;
bool isParent = true;
pid_t *childPid;
int *clientFd;

// Signal handler
static void signalHandler(int signum) {
  switch (signum) {
  case SIGINT:
#ifdef DEBUG
    cout << "signalHandler(" << signum << "): SIGINT" << endl;
#endif
    cout << "Network Monitor shutting down" << endl;
    cout << "server: Interrupted system call" << endl;

    // Send SIGINT to all child processes
    for (int i = 0; i < maxIntfMon; i++) {
      kill(childPid[i], SIGINT);
#ifdef DEBUG
      cout << "Network monitor stopping child[" << childPid[i] << "]: SIGINT"
           << endl;
#endif
    }
    isRunning = false;
    break;
  default:
    cout << "signalHandler(" << signum << "): unknown" << endl;
  }
}

// Get number of interface monitors
static int getInterfaceMonitorAmount() {
  int maxIntfMon = 0;
  cout << "How many interfaces do you want to monitor: ";
  cin >> maxIntfMon;
  return maxIntfMon;
}

// Get names of interface monitors
static char **getInterfaceMonitorNames(int maxIntfMon) {
  char **intfMonNames = new char *[maxIntfMon];

  for (int i = 0; i < maxIntfMon; i++) {
    cout << "Enter interface " << i + 1 << " : ";
    intfMonNames[i] = new char[NAME_SIZE];
    cin >> intfMonNames[i];
  }

  return intfMonNames;
}

// Set up a signal handler to terminate the program gracefully
static void setupSigAction() {
  struct sigaction action;
  action.sa_handler = signalHandler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGINT, &action, NULL);
}

// Initialize UNIX server socket
static int initServerSocket(sockaddr_un &addr, int &master_fd) {
  // Create the socket
  memset(&addr, 0, sizeof(addr));
  if ((master_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    cout << "server: " << strerror(errno) << endl;

    return -1;
  }

  addr.sun_family = AF_UNIX;
  // Set the socket path to a local socket file
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
  unlink(socket_path);

  // Bind the socket to this local socket file
  if (bind(master_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    cout << "server: " << strerror(errno) << endl;
    close(master_fd);

    return -1;
  }

#ifdef DEBUG
  cout << "DEBUG: socket fd " << master_fd << " bound to address "
       << addr.sun_path << endl;
#endif

#ifdef DEBUG
  cout << "DEBUG: server listen()" << endl;
#endif

  cout << "Waiting for the client..." << endl;
  // Listen for a client to connect to this local socket file
  if (listen(master_fd, 5) == -1) {
    cout << "server: " << strerror(errno) << endl;
    unlink(socket_path);
    close(master_fd);

    return -1;
  }

  return 0;
}

// Initialize network monitoring server
static int initializeServer(int maxIntfMon, char **intfMonNames,
                            int &master_fd) {
  // Setup sigaction and handlers
  setupSigAction();

  // Create the socket for inter-process communications
  struct sockaddr_un addr;

  // Initialize socket for server
  if (initServerSocket(addr, master_fd) == -1) {
    // Clean up dynamic resources
    for (int i = 0; i < maxIntfMon; i++) {
      delete[] intfMonNames[i];
    }
    delete[] intfMonNames;
    delete[] childPid;

    return -1;
  }

  return 0;
}

int main() {
  // Get user input for interface monitors
  maxIntfMon = getInterfaceMonitorAmount();
  intfMonNames = getInterfaceMonitorNames(maxIntfMon);
  childPid = new pid_t[2];

#ifdef DEBUG
  cout << "DEBUG: interface monitor coun: " << maxIntfMon << endl;
  for (int i = 0; i < maxIntfMon; i++) {
    cout << "DEBUG: interface monitor[" << i << "]: " << intfMonNames[i]
         << endl;
  }
#endif

  char buf[BUF_LEN];
  int len;
  int master_fd, max_fd, rc;

  // Array to hold client fds for socket communication through select()
  clientFd = new int[maxIntfMon];
  fd_set active_fd_set;
  fd_set read_fd_set;
  int ret;
  int numClients = 0;

  if (initializeServer(maxIntfMon, intfMonNames, master_fd) == -1) {
    exit(-1);
  };

  FD_ZERO(&read_fd_set);
  FD_ZERO(&active_fd_set);

  // Add the master_fd to the socket set
  FD_SET(master_fd, &active_fd_set);

  // We will select from max_fd+1 sockets (plus one is due to a new connection)
  max_fd = master_fd;
  isRunning = true;

  // Fork child processes to execute interface monitoring
  for (int i = 0; i < maxIntfMon && isParent; ++i) {
    childPid[i] = fork();
    if (childPid[i] == 0) {
      // Child process
      cout << "child: process started with pid[" << getpid() << "]" << endl;
      isParent = false;

      // Execute interfaceMon process with each interface monitors
      execlp("./interfaceMon", "./interfaceMon", intfMonNames[i], NULL);

      cout << "child: process pid[" << getpid() << "] exec() error" << endl;
      cout << strerror(errno) << endl;
    }
  }

  if (isParent) {
    sleep(1);

    while (isRunning) {
      read_fd_set = active_fd_set;
      // Block until an input arrives on one or more sockets
      // Select from up to max_fd+1 sockets
      ret = select(max_fd + 1, &read_fd_set, NULL, NULL, NULL);

      if (ret < 0) {
        cout << "server: " << strerror(errno) << endl;
      } else {
        // Service all the sockets with input pending
        if (FD_ISSET(master_fd, &read_fd_set)) {
          // Connection request on the master socket
          // Accept the new connection
          clientFd[numClients] = accept(master_fd, NULL, NULL);
          if (clientFd[numClients] < 0) {
            cout << "server: " << strerror(errno) << endl;
          } else {
            cout << "Server: incoming connection " << clientFd[numClients]
                 << endl;
            // Add the new connection to the set
            FD_SET(clientFd[numClients], &active_fd_set);

            if (max_fd < clientFd[numClients])
              max_fd = clientFd[numClients]; // Update the maximum fd
            ++numClients;
          }
        } else {
          // Data arriving on an already-connected socket
          for (int i = 0; i < numClients; ++i) {
            // Find which client sent the data
            if (FD_ISSET(clientFd[i], &read_fd_set)) {
              // Begin handshake with child processes to monitor interface
              // Handshake: Child(Ready) -> Parent(Monitor) -> Child(Monitoring)
              ret = read(clientFd[i], buf, BUF_LEN);

              if (ret == -1) {
                cout << "server: Read Error" << endl;
                cout << strerror(errno) << endl;
              }

              if (strncmp("Ready", buf, 5) == 0) {
                ret = write(clientFd[i], "Monitor", 7);
              }

              // Client notifies link is down
              if (strncmp("Link Down", buf, 9) == 0) {
                cout << "Network Monitor: Link Down" << endl;
                ret = write(clientFd[i], "Set Link Up", 12);
              }

              if (strncmp("Done", buf, 4) == 0) {
                isRunning = false;
              }

#ifdef DEBUG
              cout << "DEBUG: server read(sock[" << clientFd[i]
                   << "] interface monitor[" << intfMonNames[i]
                   << "], buf:" << buf << ")" << endl;
#endif
            }
          }
        }
      }
    }

    // Closing connections
    cout << "network monitor: closing connections" << endl;
    for (int i = 0; i < numClients; ++i) {
      // Give the clients a chance to quit
      sleep(1);
      // Remove the socket from the set of active sockets
      FD_CLR(clientFd[i], &active_fd_set);
      // Close the socket connection
      close(clientFd[i]);
    }

    // Close the master socket
    close(master_fd);
    // Remove the socket file from /tmp
    unlink(socket_path);

    // Clean up dynamic resources
    for (int i = 0; i < maxIntfMon; i++) {
      delete[] intfMonNames[i];
    }
    delete[] intfMonNames;
    delete[] childPid;
    delete[] clientFd;
  }

  return 0;
}