/* Modular Arduino Token Transfer (MATT) Daemon
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <stdint.h>

#include "mattd.h"

char *serial_path;
int serial_fd;

void open_port() {
  serial_fd = open(serial_path, O_RDWR | O_NOCTTY); //| O_NDELAY);
  if (serial_fd == -1) {
	  //Could not open serial port
	  perror(DAEMON);
	  exit(0);
  } else
	  fcntl(serial_fd, F_SETFL, 0); //block while reading
}

char read_byte() {
  char buf[1];
  int val;
  
  val = read(serial_fd, buf, 1);
  if (val > 0)
    return buf[0];
  else
    return val;
} 

/* buf is pointer to data
 * id is the type of data (see mattd.h)
 * len is number of bytes of the data
 */
void send_data((const uint8_t *data, const int id, const int len) {
  int i, x, y;
  uint8_t buf;
  //send data over serial
  //first write preamble 
  buf = 0x06;
  write(serial_fd, &buf, 1);
  buf = 0x85;
  write(serial_fd, &buf, 1);
  //then write size of data being sent
  buf = len;
  write(serial_fd, &buf, 1);
  
  //now send the data
  for(i = 0; i < len; i++) {
    CS ^= *(data + i);
    buf = *(data + i);
    write(serial_fd, &buf, 1);
  }
  //lastly, write checksum
  buf = CS;
  write(serial_fd, &buf, 1);
  //TODO wait for ACK from Ardupilot, resend if ACK not received or NACK
}

int main(int argc, char *argv[]) {
  if (argc > 1) {
    serial_path = argv[1];
  } else {
    serial_path = DEFAULT_SERIAL_PORT;
  }
  
  open_port();
  
  //TODO Start a thread for receiving data
  
  //TODO Start a thread for transmitting data to base station
  
  //TODO Start a thread for sending data to Ardupilot
  
  //TODO wait for threads to join?
  
  close(serial_fd);
  
  return(0);
}
