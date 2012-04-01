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
#include <asm/ioctls.h>

#include "mattd.h"

char *serial_path;
int serial_fd;

//stuff copied from EasyTransfer
uint8_t rx_len;
uint8_t rx_array_inx;

void open_port() {
	serial_fd = open(serial_path, O_RDWR | O_NOCTTY | O_NDELAY); //O_NDELAY: don't care if something's connected
	if (serial_fd == -1) {
		//Could not open serial port
		perror(DAEMON);
		exit(0);
	} else
		fcntl(serial_fd, F_SETFL, 0); //FNDELAY); //don't block while reading
}

void close_port() {
	close(serial_fd);
}

/* Read size bytes from serial, store in dest
 * Return number of bytes available in serial buffer
 * Daemon exits on error
 */
int read_bytes(uint8_t *dest, const int size) {
	int val;

	val = read(serial_fd, dest, size);
	printf("got char: %x\n", *dest);
	if (val >= 0) //include -1 for "resource temporarily unavailable"		
		return val;
	else {
		printf("%d\n", val);
		printf("error in read_bytes\n");
		printf("dest: %x size: %x\n", *dest, size);
		perror(DAEMON);
		exit(0);
	}
} 

int available() {
	int bytes;
	ioctl(serial_fd, FIONREAD, &bytes);
	return bytes;
}

/* Try to receive data and put it at location marked by dest
 * Return size of data received, otherwise 0
 */
int receive_data(uint8_t** dest) {
	uint8_t buf, calc_CS;
	int i;
	rx_array_inx = 0;
	rx_len = 0;

	//try to receive preamble
	read_bytes(&buf, 1);
	if (buf == 0x06) {
	  read_bytes(&buf, 1);
	  if (buf == 0x85) {
    	read_bytes(&rx_len, 1);
	    //malloc dest to this size
		  *dest = (uint8_t *) malloc(rx_len);	
		  printf("malloc'd dest to size %x\n", rx_len);
   	

    printf("got preamble\n");
	  //we get here if we already found the header bytes
	  while(rx_array_inx <= rx_len){
			  read_bytes((uint8_t *)(*dest) + rx_array_inx++, 1);
	  }
	
    for (i = 0; i < rx_len; i++) {
      printf("%x\n", (*dest)[i]);
    }

	  if(rx_len == (rx_array_inx - 1)){
		  //seem to have got whole message
		  //last uint8_t is CS
		  printf("seemed to have gotten whole message\n");
		  calc_CS = rx_len;
		  for (i = 0; i < rx_len; i++){
			  calc_CS ^= (*dest)[i];
		  } 

		  if(calc_CS == (*dest)[rx_array_inx - 1]){//CS good
        rx_len = 0;
        rx_array_inx = 0;
        printf("sending ACK\n");
        buf = ACK;
        write(serial_fd, &buf, 1);
        return rx_len;
      } else {
			  //failed checksum, need to clear this out anyway
        rx_len = 0;
        rx_array_inx = 0;
        printf("sent NACK\n");
        buf = NACK;
    	  write(serial_fd, &buf, 1);
        return 0;
      }
    }
    }
  }
  printf("sent NACK\n");
	buf = NACK;
	write(serial_fd, &buf, 1); 
	return 0;
}

/* data is pointer to data
 * len is number of bytes of the data
 */
void send_data(const uint8_t *data, const int len) {
	int i, j, val;
	uint8_t buf, CS;

	printf("max tries: %d", MAX_TRIES);	
	//send data over serial
	for(i = 0; i < MAX_TRIES; i++) {
		//first write preamble 
		printf("writing preamble...\n");
		buf = 0x06;
		write(serial_fd, &buf, 1);
		buf = 0x85;
		write(serial_fd, &buf, 1);

		//then write size of data being sent
		printf("writing size...\n");
		buf = len;
		write(serial_fd, &buf, 1);

		//now send the data
		printf("writing data...\n");
		CS = len;
		for(j = 0; j < len; j++) {
			CS ^= *(data + j);
			buf = *(data + j);
			write(serial_fd, &buf, 1);
		}
		//lastly, write checksum
		printf("writing checksum %x...\n", CS);
		buf = CS;
		write(serial_fd, &buf, 1);
		//TODO wait for ACK from Ardupilot, resend if ACK not received or NACK
		printf("waiting for ack...\n");
		buf = 0;
		val = read_bytes(&buf, 1);	
		if (buf == ACK) {
			printf("Teddy got the data!\n");
			break;
		} else if (val == -1) {
			//resource temporarily unavailable
			printf("Resource unavailable, sending again, %dth try.\n", i);
			if (i == MAX_TRIES) {
				perror(DAEMON);
				exit(0);		
			}	
		} else if (buf == NACK) {
			i--;
			continue;
		} else {
			printf("Teddy didn't get it, sending again, %dth try.\n", i);
			if (i == MAX_TRIES) {
				perror(DAEMON);
				exit(0);		
			}
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc > 1) {
		serial_path = argv[1];
	} else {
		serial_path = DEFAULT_SERIAL_PORT;
	}

	printf("trying to open %s\n", serial_path);
	open_port();

  //TODO notes for ACTUAL code
  //open wireless socket with basestation
  /*
  uint8_t* data = NULL;
  while(1) {

    if(data)
      free(data);
    receive_data(&data);
    //TODO check if data was actually received, otherwise don't send over wifi
      if(sendall(data) < 0) {
        //if wireless socket broken
          //send LAND command to ardupilot
      }
  }
  */

  uint8_t *teddy = NULL;

  printf("started\n");
  while(1) {
    if(teddy != NULL) {
     free(teddy);
     teddy = NULL;
    }
    while(available() == 0);
    receive_data(&teddy);
  }
	close_port();

	return(0);
}
