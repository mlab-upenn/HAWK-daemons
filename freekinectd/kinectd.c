#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <signal.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// libjpeg include
#include <jpeglib.h>

// zlib Include
#include <zlib.h>

// Libfreenect includes
#include "libfreenect.h"
#include "libfreenect_sync.h"

#define BASE_STATION_ADDR 	"192.168.1.21"
#define BASE_STATION_PORT 	"1337"

//TODO Think about doing this dynamically
#define FRAME_WINDOW_SIZE	5

// Path to configuration file
#define SAMPLE_XML_PATH_LOCAL "KinectConfig.xml"

//zlib
z_stream strm;

//libjpeg
struct jpeg_compress_struct cinfo;
struct jpeg_error_mgr jerr;

// Pointers passed to libfreenect sync functions
uint8_t * rgb_raw;
uint8_t * depth_registered;

// Space for double-buffering
uint8_t * rgb_dbuf = NULL;
uint8_t * depth_dbuf = NULL;

// Compile options:
// Uncomment the following to get per-frame size info:
// #define VERBOSE
// Uncomment the following to copy the kinect data to
// local buffers prior to transmission:
// #define DOUBLEBUF


freenect_context * ctx;

// Set up libfreenect to obtain 
int kinectInit(void)
{
  int ret;
  
  // Try to init the driver through the C++ interface
  ret = freenect_init(&ctx, NULL);
  if(ret < 0) {
    return ret;
  }

  // Test Kinect, set LED to let user know it started.
  // This should start the "run-loop" for libfreenect
  ret = freenect_sync_set_led((freenect_led_options)3, 0);
  if(ret < 0) {
    printf("Error on set LED.\n");
    return ret;
  }

  #ifdef DOUBLEBUF
  rgb_dbuf = (uint8_t *)malloc(640*480*3*sizeof(uint8_t));
  depth_dbuf = (uint8_t *)malloc(640*480*2*sizeof(uint8_t));
  #endif

  return 0;

}

// Updates to the latest image obtained from the Kinect
int kinectUpdate(void)
{
  
  uint32_t ts;
  // Pull a (distorted) RGB frame
  int ret = freenect_sync_get_video((void **)&rgb_raw, &ts, 0,
				    FREENECT_VIDEO_RGB);
  if(ret < 0) {
    printf("Error: unable to acquire RGB stream\n");
    return ret;
  }

  // Pull a depth frame registered to the above image
  ret = freenect_sync_get_depth((void **)&depth_registered, &ts, 0,
				FREENECT_DEPTH_REGISTERED);
  
  if(ret < 0) {
    printf("Error: unable to acquire registered depth stream\n");
    return ret;
  }

  // Copy frame data to local buffers
#ifdef DOUBLEBUF
  memcpy(rgb_dbuf, rgb_raw, 640*480*3*sizeof(uint8_t));
  memcpy(depth_dbuf, depth_registered, 640*480*2*sizeof(uint8_t));
  rgb_raw = rgb_dbuf;
  depth_registered = depth_dbuf;
#endif

  return 0;

}

// TODO: make code use this
void kinectShutdown(void)
{
  
  // Set LED to blinking green (default)
  freenect_sync_set_led((freenect_led_options)4, 0);
  freenect_sync_stop();
  
}

int sendall(const int sfd, const uint8_t * buf, const int len)
{
  int total = 0;
  int bytesleft = len;
  int n = 0;

  while(total < len) {
    n = send(sfd, buf + total, bytesleft, 0);
    if(n == -1) { break; }
    total += n;
    bytesleft -= n;
  }

  return n == -1 ? -1 : total;
}

/**
 * get_in_addr(): returns either IPv4 or IPv6 address given
 * sockaddr struct.
 */
void * get_in_addr(struct sockaddr *sa)
{
  if(sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  else {
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
  }
}

int network_setup(void) {
	int sockfd, rv;
	char s[INET6_ADDRSTRLEN];
	struct addrinfo hints, *servinfo, *p;

	bzero((void*)&hints, sizeof(hints));
	hints.ai_family = PF_INET;            // IPv4
	hints.ai_socktype = SOCK_STREAM;      // TCP

	if((rv = getaddrinfo(BASE_STATION_ADDR, BASE_STATION_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}

	// Loop through results and connect to first usable one
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if((sockfd = socket(p->ai_family, p->ai_socktype,
						p->ai_protocol)) == -1) {
			perror("socket");
			continue;
		}

		if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("connect (L53)");
			continue;
		}
		// Successfully connected
		break;
	}

	if(p == NULL) {
		fprintf(stderr, "Failed to connect\n");
		exit(2);
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof(s));

	printf("Connected to %s\n", s);
	freeaddrinfo(servinfo);   // Done with server info struct 

	return sockfd;
}

int setup_compression(void)

{
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  return 0;
}

int close_compression(void)
{
  jpeg_destroy_compress(&cinfo);
  return 0;
}

int compress_frame(uint8_t * src, uint8_t ** dest, unsigned long * outsize,

		   const int height, const int width, const int format)
{
  uint8_t * row_pointer;
  int row_stride;

  jpeg_mem_dest(&cinfo, dest, outsize);

  cinfo.image_width = width;
  cinfo.image_height = height;
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;
  jpeg_set_defaults(&cinfo);

  jpeg_start_compress(&cinfo, TRUE);
  row_stride = width*format;

  // Feed bitmap line-by-line
  while(cinfo.next_scanline < cinfo.image_height) {
    row_pointer = src + cinfo.next_scanline * row_stride;
    jpeg_write_scanlines(&cinfo, &row_pointer, 1);
  }
  
  jpeg_finish_compress(&cinfo);

  return 0;
}

int compress_depth(uint8_t * src, uint8_t * dest, uint32_t in_size) {
  int ret, have;
  uint8_t * in;
  uint8_t * out;

  in = src;
  out = dest;

  // Set up zlib
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  ret = deflateInit(&strm, 1);

  if(ret != Z_OK) {
    printf("deflateinit failed\n");
    kinectShutdown();
    exit(1);
  }

  strm.avail_in = in_size;
  strm.next_in = in;
  strm.avail_out = in_size;
  strm.next_out = out;
  ret = deflate(&strm, Z_FINISH);

  if(ret == Z_STREAM_ERROR) {
    printf("deflate: Z_STREAM_ERROR\n");
    kinectShutdown();
    exit(1);
  } else if(ret != Z_STREAM_END) {
    printf("error: expected Z_STREAM_END\n");
  }

  have = in_size - strm.avail_out;

  if(deflateEnd(&strm) != Z_OK) {

    printf("error: deflateEnd\n");
    kinectShutdown();
    exit(1);

  }

  return have;
}

void sigintHandler(int signum)
{
  kinectShutdown();
  exit(0);
}

int main(void) {

	int sockfd = network_setup();
	int framecount = 0;

	signal(SIGINT, sigintHandler);

	// Initialize the Kinect  
	if(kinectInit()) {
		printf("Error initializing device: check that the device is connected.\n");
		return 1;
	}

	uint32_t depthsize = sizeof(uint16_t)*640*480;
	unsigned long rgbsize;
	rgbsize = sizeof(uint8_t)*3*640*480;

	uint8_t *compdepth = (uint8_t *) malloc(depthsize);
	uint8_t *comprgb = (uint8_t *) malloc(rgbsize);

	//uint8_t *image_data = (uint8_t *) malloc(rgbsize);
	//uint8_t *depth_data = (uint8_t *) malloc(depthsize);
	uint8_t * image_data;
	uint8_t * depth_data;


	uint32_t depthcompression;
	//unsigned long *rgbcompression = (unsigned long *) malloc(sizeof(long));
	unsigned long outsize;

	setup_compression();

	while(1) {

	  int ret = kinectUpdate();
	  if(ret < 0) {
	    printf("Error: failed to obtain depth data\n");
	    usleep(100);
	    continue;
	  }
	  image_data = rgb_raw;
	  depth_data = depth_registered;
	  
	  uint16_t * middlePoint = (uint16_t *)(depth_data + 640*240*2 + 240*2);
	  	  
	  //compress rgb
	  compress_frame(image_data, &comprgb, &outsize, 480, 640, 3);	
	  
#ifdef VERBOSE
	  printf("compressed rgb to size %d\n", outsize);
#endif
	  
	  //send size of compressed rgb frame
	  if((sendall(sockfd, (uint8_t *)&outsize, sizeof(uint32_t))) < 0) {
	    perror("sendallrgbsize");
	    kinectShutdown();
	    exit(1);
	  } 
	  
	  if((sendall(sockfd, comprgb, outsize)) < 0) {
	    perror("sendallrgb");
	    kinectShutdown();
	    exit(1);
	  }
	  
	  //compress depth 
	  depthcompression = compress_depth(depth_data, compdepth, depthsize);	
#ifdef VERBOSE
	  printf("compressed depth to size %d\n", depthcompression);
#endif	  

	  //send size of compressed rgb frame
	  if((sendall(sockfd, (uint8_t *)&depthcompression, sizeof(uint32_t))) < 0) {
	    kinectShutdown();
	    perror("sendalldepthsize");
	    exit(1);
	  }
	  
	  if((sendall(sockfd, compdepth, depthcompression)) < 0) {
	    kinectShutdown();
	    perror("sendalldepth");
	    exit(1);
	  }
	  
#ifdef VERBOSE	  
	  printf("sent out frame %d\n", ++framecount);
#endif

	  // Sleep for some time
	  //	  usleep(1000*300);
	  
	}

	return 0;
	
}
