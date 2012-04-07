#include <stdlib.h>
#include <stdio.h>
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

// OpenNI Includes
#include <XnOpenNI.h>
#include <XnLog.h>
#include <XnCppWrapper.h>
#include <XnFPSCalculator.h>

#define BASE_STATION_ADDR 	"192.168.1.21"
#define BASE_STATION_PORT 	"1337"

//TODO Think about doing this dynamically
#define FRAME_WINDOW_SIZE	5

// Path to configuration file
#define SAMPLE_XML_PATH_LOCAL "KinectConfig.xml"

// OpenNI Kinect Objects
using namespace xn;

ImageGenerator g_image;
ImageMetaData g_imageMD;
DepthGenerator depth;
DepthMetaData depthMD;
Context context;
const XnGrayscale8Pixel* monoImage;

//zlib
z_stream strm;

//libjpeg
struct jpeg_compress_struct cinfo;
struct jpeg_error_mgr jerr;

// Set up OpenNI to obtain 8-bit mono images from the Kinect's RGB camera
int kinectInit(void)
{
  XnStatus nRetVal = XN_STATUS_OK;
  ScriptNode scriptNode;
  EnumerationErrors errors;

  printf("Reading config from: '%s'\n", SAMPLE_XML_PATH_LOCAL);
  nRetVal = context.InitFromXmlFile(SAMPLE_XML_PATH_LOCAL, scriptNode, &errors);

  nRetVal = context.FindExistingNode(XN_NODE_TYPE_IMAGE, g_image); 
  //g_image.SetPixelFormat(XN_PIXEL_FORMAT_GRAYSCALE_8_BIT); 
  g_image.SetPixelFormat(XN_PIXEL_FORMAT_RGB24); 
  g_image.GetMetaData(g_imageMD);

  nRetVal = context.FindExistingNode(XN_NODE_TYPE_DEPTH, depth);
  depth.GetMetaData(depthMD);

  //  nRetVal = depth.GetAlternativeViewPointCap().SetViewPoint(g_image);
  //nRetVal = depth.GetFrameSyncCap().FrameSyncWith(g_image);

 return nRetVal;
}

// Updates to the latest image obtained from the Kinect
int kinectUpdate(void)
{
  XnStatus nRetVal = context.WaitAndUpdateAll();
  g_image.GetMetaData(g_imageMD);
  //nRetVal = context.WaitOneUpdateAll(depth);
  depth.GetMetaData(depthMD);
  
  return nRetVal;
}

int sendall(const int sfd, const uint8_t * buf, const int len)
{
  int total = 0;
  int bytesleft = len;
  int n;

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
    exit(1);
  }

  strm.avail_in = in_size;
  strm.next_in = in;
  strm.avail_out = in_size;
  strm.next_out = out;
  ret = deflate(&strm, Z_FINISH);

  if(ret == Z_STREAM_ERROR) {
    printf("deflate: Z_STREAM_ERROR\n");
    exit(1);
  } else if(ret != Z_STREAM_END) {
    printf("error: expected Z_STREAM_END\n");
  }

  have = in_size - strm.avail_out;

  if(deflateEnd(&strm) != Z_OK) {

    printf("error: deflateEnd\n");

    exit(1);

  }

  return have;
}

int main(void) {
	printf("unsigned long size: %d\n", sizeof(unsigned long));

	int sockfd = network_setup();
	int framecount = 0;

	// Initialize the Kinect  
	if(kinectInit() != XN_STATUS_OK) {
		printf("Unexpected error: check that the device is connected.\n");
		return 1;
	}

	uint32_t depthsize = sizeof(uint16_t)*640*480;
	unsigned long rgbsize;
	rgbsize = sizeof(uint8_t)*3*640*480;

	uint8_t *compdepth = (uint8_t *) malloc(depthsize);
	uint8_t *comprgb = (uint8_t *) malloc(rgbsize);
	
	uint8_t * rgb_buf;
	uint8_t * depth_buf;

	uint8_t *image_data = (uint8_t *) malloc(rgbsize);
	uint8_t *depth_data = (uint8_t *) malloc(depthsize);

	uint32_t depthcompression;
	//unsigned long *rgbcompression = (unsigned long *) malloc(sizeof(long));
	unsigned long outsize;

	setup_compression();

	while(1) {
		kinectUpdate();

		//compress rgb
		rgb_buf = (uint8_t *)g_imageMD.RGB24Data();
		depth_buf = (uint8_t *)depthMD.Data();
		memcpy(image_data, rgb_buf, rgbsize);
		memcpy(depth_data, depth_buf, depthsize);
		//comprgb = (uint8_t *)g_imageMD.RGB24Data();
		//compression = rgbsize;

		compress_frame(image_data, &comprgb, &outsize, 480, 640, 3);	
		printf("compressed rgb to size %d\n", outsize);

		//send size of compressed rgb frame
		if((sendall(sockfd, (uint8_t *)&outsize, sizeof(uint32_t))) < 0) {
			perror("sendallrgbsize");
			exit(1);
		} 
		
		if((sendall(sockfd, comprgb, outsize)) < 0) {
			perror("sendallrgb");
			exit(1);
		}
	
		//compress depth 
		depthcompression = compress_depth(depth_data, compdepth, depthsize);	
		printf("compressed depth to size %d\n", depthcompression);

		//send size of compressed rgb frame
		if((sendall(sockfd, (uint8_t *)&depthcompression, sizeof(uint32_t))) < 0) {
			perror("sendalldepthsize");
			exit(1);
		}
 
		if((sendall(sockfd, compdepth, depthcompression)) < 0) {
			perror("sendalldepth");
			exit(1);
}

		printf("sent out frame %d\n", ++framecount);
	}
}
