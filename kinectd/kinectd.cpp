#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// OpenNI Includes
#include <XnOpenNI.h>
#include <XnLog.h>
#include <XnCppWrapper.h>
#include <XnFPSCalculator.h>

#define BASE_STATION_ADDR 	"158.130.13.93"
#define BASE_STATION_PORT 	"1337"

//TODO Think about doing this dynamically
#define FRAME_WINDOW_SIZE	5

// Path to configuration file
#define SAMPLE_XML_PATH_LOCAL "KinectConfig.xml"

// OpenNI Kinect Objects
using namespace xn;

ImageGenerator g_image;
ImageMetaData g_imageMD;
Context context;
const XnGrayscale8Pixel* monoImage;

// Set up OpenNI to obtain 8-bit mono images from the Kinect's RGB camera
int kinectInit(void)
{
  XnStatus nRetVal = XN_STATUS_OK;
  ScriptNode scriptNode;
  EnumerationErrors errors;

  printf("Reading config from: '%s'\n", SAMPLE_XML_PATH_LOCAL);
  nRetVal = context.InitFromXmlFile(SAMPLE_XML_PATH_LOCAL, scriptNode, &errors);

  if (nRetVal != XN_STATUS_OK) {
    return nRetVal;
  }

  nRetVal = context.FindExistingNode(XN_NODE_TYPE_IMAGE, g_image);
  g_image.SetPixelFormat(XN_PIXEL_FORMAT_GRAYSCALE_8_BIT);
  nRetVal = context.WaitOneUpdateAll(g_image);
  g_image.GetMetaData(g_imageMD);

 return nRetVal;
}

// Updates to the latest image obtained from the Kinect
int kinectUpdate(void)
{
  XnStatus nRetVal = context.WaitOneUpdateAll(g_image);
  g_image.GetMetaData(g_imageMD);
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

int main(void) {
	int sockfd = network_setup();

	// Initialize the Kinect  
	if(kinectInit() != XN_STATUS_OK) {
		printf("Unexpected error: check that the device is connected.\n");
		return 1;
	}

	int outsize = sizeof(uint8_t)*640*480;

	while(1) {
		
		
		kinectUpdate();

		if((sendall(sockfd, (uint8_t *)g_imageMD.Grayscale8Data(), outsize)) < 0) {
			perror("sendallb");
			exit(1);
		}
	}
}
