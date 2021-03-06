#include <sstream>
#include <string>
#include <iostream>
//#include <opencv2\highgui.h>
#include "opencv2/highgui/highgui.hpp"
//#include <opencv2\cv.h>
#include "opencv2/opencv.hpp"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <math.h>
using namespace std;
using namespace cv;

//default capture width and height
const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;
//max number of objects to be detected in frame
const int MAX_NUM_OBJECTS = 50;
//minimum and maximum object area
const int MIN_OBJECT_AREA = 20 * 20;
const int MAX_OBJECT_AREA = FRAME_HEIGHT*FRAME_WIDTH / 1.5;
//names that will appear at the top of each window
const std::string windowName = "Original Image";
const std::string windowName1 = "HSV Image";
const std::string windowName2 = "Thresholded Image";
const std::string windowName3 = "After Morphological Operations";
const std::string trackbarWindowName = "Trackbars";

float mMeu=0.0;
float mAdv=0.0;
float xmv=0; //
float ymv=0; //
float xav=0; //
float yav=0; //
float xCentru=0.0 //p
float yCentru=0.0; //p
float raza=0.0; //p
float  teta_10ms = 90; //p
void on_mouse(int e, int x, int y, int d, void *ptr)
{
	if (e == EVENT_LBUTTONDOWN)
	{
		cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
	}
}

void on_trackbar(int, void*)
{//This function gets called whenever a
 // trackbar position is changed
}

string intToString(int number) {


	std::stringstream ss;
	ss << number;
	return ss.str();
}

void createTrackbars(int &H_MIN, int &H_MAX, int &S_MIN,
		int &S_MAX, int &V_MIN, int &V_MAX) {
	//create window for trackbars


	namedWindow(trackbarWindowName, 0);
	//create memory to store trackbar name on window
	char TrackbarName[50];
	sprintf(TrackbarName, "H_MIN", H_MIN);
	sprintf(TrackbarName, "H_MAX", H_MAX);
	sprintf(TrackbarName, "S_MIN", S_MIN);
	sprintf(TrackbarName, "S_MAX", S_MAX);
	sprintf(TrackbarName, "V_MIN", V_MIN);
	sprintf(TrackbarName, "V_MAX", V_MAX);
	//create trackbars and insert them into window
	//3 parameters are: the address of the variable that is changing when the trackbar is moved(eg.H_LOW),
	//the max value the trackbar can move (eg. H_HIGH),
	//and the function that is called whenever the trackbar is moved(eg. on_trackbar)
	//                                  ---->    ---->     ---->
	createTrackbar("H_MIN", trackbarWindowName, &H_MIN, H_MAX, on_trackbar);
	createTrackbar("H_MAX", trackbarWindowName, &H_MAX, H_MAX, on_trackbar);
	createTrackbar("S_MIN", trackbarWindowName, &S_MIN, S_MAX, on_trackbar);
	createTrackbar("S_MAX", trackbarWindowName, &S_MAX, S_MAX, on_trackbar);
	createTrackbar("V_MIN", trackbarWindowName, &V_MIN, V_MAX, on_trackbar);
	createTrackbar("V_MAX", trackbarWindowName, &V_MAX, V_MAX, on_trackbar);


}
void drawObject(int x, int y, Mat &frame) {

	//use some of the openCV drawing functions to draw crosshairs
	//on your tracked image!

	//UPDATE:JUNE 18TH, 2013
	//added 'if' anmorphOpsd 'else' statements to prevent
	//memory errors from writing off the screen (ie. (-25,-25) is not within the window!)

	circle(frame, Point(x, y), 20, Scalar(0, 255, 0), 2);
	if (y - 25 > 0)
		line(frame, Point(x, y), Point(x, y - 25), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(x, 0), Scalar(0, 255, 0), 2);
	if (y + 25 < FRAME_HEIGHT)
		line(frame, Point(x, y), Point(x, y + 25), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(x, FRAME_HEIGHT), Scalar(0, 255, 0), 2);
	if (x - 25 > 0)
		line(frame, Point(x, y), Point(x - 25, y), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(0, y), Scalar(0, 255, 0), 2);
	if (x + 25 < FRAME_WIDTH)
		line(frame, Point(x, y), Point(x + 25, y), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(FRAME_WIDTH, y), Scalar(0, 255, 0), 2);

	putText(frame, intToString(x) + "," + intToString(y), Point(x, y + 30), 1, 1, Scalar(0, 255, 0), 2);
	//cout << "x,y: " << x << ", " << y;

}
void morphOps(Mat &thresh) {

	//create structuring element that will be used to "dilate" and "erode" image.
	//the element chosen here is a 3px by 3px rectangle

	Mat erodeElement = getStructuringElement(MORPH_RECT, Size(3, 3));
	//dilate with larger element so make sure object is nicely visible
	Mat dilateElement = getStructuringElement(MORPH_RECT, Size(8, 8));

	erode(thresh, thresh, erodeElement);
	erode(thresh, thresh, erodeElement);


	dilate(thresh, thresh, dilateElement);
	dilate(thresh, thresh, dilateElement);



}
void trackFilteredObject(int &x, int &y, Mat threshold, Mat &cameraFeed) {

	Mat temp;
	threshold.copyTo(temp);
	//these two vectors needed for output of findContours
	vector< vector<Point> > contours;
	vector<Vec4i> hierarchy;
	//find contours of filtered image using openCV findContours function
	findContours(temp, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
	//use moments method to find our filtered object
	double refArea = 0;
	bool objectFound = false;
	if (hierarchy.size() > 0) {
		int numObjects = hierarchy.size();
		//if number of objects greater than MAX_NUM_OBJECTS we have a noisy filter
		if (numObjects < MAX_NUM_OBJECTS) {
			for (int index = 0; index >= 0; index = hierarchy[index][0]) {

				Moments moment = moments((cv::Mat)contours[index]);
				double area = moment.m00;

				//if the area is less than 20 px by 20px then it is probably just noise
				//if the area is the same as the 3/2 of the image size, probably just a bad filter
				//we only want the object with the largest area so we safe a reference area each
				//i//initial min and max HSV filter values.
				//these will be changed using trackbars iteration and compare it to the area in the next iteration.
				if (area > MIN_OBJECT_AREA && area<MAX_OBJECT_AREA && area>refArea) {
					x = moment.m10 / area;
					y = moment.m01 / area;
					objectFound = true;
					refArea = area;
				}
				else objectFound = false;


			}
			//let user know you found an object
			if (objectFound == true) {
				putText(cameraFeed, "Tracking Object", Point(0, 50), 2, 1, Scalar(0, 255, 0), 2);
				//draw object location on screen
				//cout << x << "," << y;
				drawObject(x, y, cameraFeed);

			}


		}
		else putText(cameraFeed, "TOO MUCH NOISE! ADJUST FILTER", Point(0, 50), 1, 2, Scalar(0, 0, 255), 2);
	}
}

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int initializareSocket(char *ip , int port){
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0){
        error("ERROR opening socket");
  }
  struct sockaddr_in remoteaddr;
  remoteaddr.sin_family = AF_INET;
  remoteaddr.sin_addr.s_addr = inet_addr(ip);
  remoteaddr.sin_port = htons(port);
  if(connect(sockfd, (struct sockaddr *)&remoteaddr, sizeof(remoteaddr)) < 0){
    error("ERROR connecting");
  }
	return sockfd;
}

bool verificaComanda(char c){
	if(c=='f'||c=='b'||c=='l'||c=='r'||c=='s'){
		return true;
	}
	return false;

}
void trimiteComenzi(int sockfd,char *sirComenziS,int uTime){
	int i=0,lungime,bytes_sent;
	char ultimaComanda;
	lungime=strlen(sirComenzi);
	if(lungime>300){
		printf("Sir de comenzi prea lung\n");
		return;
	}
	for(i=0;i<lungime;i++){
		if (verificaComanda(sirComenzi[i])){
			bytes_sent=send(sockfd,&sirComenzi[i],1,0);
			if (bytes_sent != 1){
				//eroare
			}
			else{
				ultimaComanda=sirComenzi[i];
				usleep(uTime);
			}
		}
	}
	if (ultimaComanda!='s'){
		bytes_sent=send(sockfd,"s",1,0);
	}
	return;
}
float distanta(float x1 , float y1, float x2 , float y2){
	return sqrt(((float)(((float)((((float)x1)-((float)x2))*(((float)x1)-((float)x2))))+((float)((((float)y1)-((float)y2))*(((float)y1)-((float)y2)))))));
}
void mergiInainte(int sockfd){
    char comenzi[3];
    comenzi[0]='f';
    comenzi[1]='s';
    comenzi[2]=0;
    trimiteComenzi(sockfd,comenzi,10*1000);
}
void roteste(float teta,char directie,int sockfd){
    float timp=teta/teta_10ms;
    timp=timp*10*1000;
    char comenzi[3];
    comenzi[0]=directie;
    comenzi[1]='s';
    comenzi[2]=0;
    trimiteComenzi(sockfd,comenzi,timp);
}
float punctLaDreapta(float x1,float y1, float x2 , float y2, float x0, float y0){
    float p1=abs((y2-y1)*x0-(x2-x1)*y0+x2*y1-y2*x1);
    float p2=sqrt((y2-y1)*(y2-y1)+(x2-x1)*(x2-x1));
    return p1/p2;
}
void duteLa(int xMeu,int yMeu,int xp,int yp,int sockfd){
    float d = punctLaDreapta(xvm,yvm,xMeu,yMeu,xp,yp);
    float teta=d/distanta(xMeu,yMeu,xp,yp);
    teta=asin(teta);
    if ((xp-xMeu)>=0){
        teta=teta;
    }
    else{
        teta=2*3.1415 -teta;
    }
    if ((yp-yMeu)>=0){
        char directie='s';
    }
    else{
        char directie='d';
    }
    roteste(teta,directie);
    mergiInainte(sockfd);
}
void calculeazaDirectie(int xMeu, int yMeu, int xAdv, int yAdv){
    if (xMeu==xmv){
        mMeu=0.0;
    }
    else{
        mMeu=(float)(((float)yMeu - (float)ymv)/((float)xMeu-(float)xmv));
    }
    if (xAdv==xav){
        mAdv=0.0;
    }
    else{
        mAdv=(float)(((float)yAdv - (float)yav)/((float)xAdv-(float)xav));
    }
    xmv=xMeu;
    ymv=yMeu;
    xav=xAdv;
    yav=yAdv;
}

void aflaCoordonate(int xMeu,int yMeu, int xAdv,int yAdv, int *xp, int *yp){
	float fxMeu,fyMeu,fxAdv,fyAdv,fxp,fyp,fmMeu,fmAdv;
	fxMeu=(float)xMeu;
	fyMeu=(float)yMeu;
	fxAdv=(float)xAdv;
	fyAdv=(float)yAdv;
	fmMeu=(float)mMeu;
	fmAdv=(float)mAdv;

	fxp=(fmAdv*fxMeu+(fxAdv/fmAdv)+fyAdv-fyMeu)/(fmAdv+(1.0/fmAdv));
	fyp=((fmAdv*fmAdv*fxMeu+fxAdv+fmAdv*fyAdv-fmAdv*fyMeu)/(fmAdv+(1.0/fmAdv)))-(fmAdv*fxMeu)+(fyMeu);
	if ((distanta(fxAdv,fyAdv,fxp,fyp)<dimensiuneBataie) || (distanta(fxp,fyp,xCentru,yCentru)>=raza)){
        *xp=(int)xCentru;
        *yp=(int)yCentru;
	}
	else{
        *xp=(int)fxp;
        *yp=(int)fyp;
	}
}
void miscaRobot(int xMeu , int yMeu , int xAdv , int yAdv,int sockfd){
	int xp,yp,
	aflaCoordonate(xMeu, yMeu,xAdv,yAdv,&xp,&yp);
	duteLa(xMeu,yMeu,xp,yp,sockfd);
	calculeazaDirectie(xMeu ,  yMeu ,  xAdv ,  yAdv);
}

int main(int argc, char* argv[])
{
	//int sockfd = initializareSocket("193.226.12.217",20236);
	//trimiteComenzi(sockfd,"fsbsl");

/*
	//some boolean variables for different functionality within this
	//program
	bool trackObjects = true;
	bool useMorphOps = true;

	Point p;
	//Matrix to store each frame of the webcam feed
	Mat cameraFeed;
	//matrix storage for HSV image
	Mat HSV, HSV1;
	//matrix storage for binary threshold image
	Mat threshold, threshold1;
	//x and y values for the location of the object
	int x = 0, y = 0;
	int x1 = 0, y1 = 0;
	//create slider bars for HSV filtering
	//initial min and max HSV filter values.
	//these will be changed using trackbars
	int H_MIN = 162;
	int H_MAX = 256;
	int S_MIN = 0;
	int S_MAX = 256;
	int V_MIN = 0;
	int V_MAX = 256;
	//initial min and max HSV filter values.
	//these will be changed using trackbars
	// dusmanu'
	int H_MIN1 = 0;
	int H_MAX1 = 256;
	int S_MIN1 = 110;
	int S_MAX1 = 256;
	int V_MIN1 = 246;
	int V_MAX1 = 256;
	createTrackbars(H_MIN, H_MAX, S_MIN, S_MAX, V_MIN, V_MAX);
	createTrackbars(H_MIN1, H_MAX1, S_MIN1, S_MAX1, V_MIN1, V_MAX1);
	//video capture object to acquire webcam feed
	VideoCapture capture;
	//open capture object at location zero (default location for webcam)
	capture.open("rtmp://172.16.254.99/live/nimic");
	//set height and width of capture frame
	capture.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
	capture.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
	//start an infinite loop where webcam feed is copied to cameraFeed matrix
	//all of our operations will be performed within this loop



    int inceput =0;
	while (1) {


		//store image to matrix
		capture.read(cameraFeed);
		//convert frame from BGR to HSV colorspace
		cvtColor(cameraFeed, HSV, COLOR_BGR2HSV);
		//filter HSV image between values and store filtered image to
		//threshold matrix

		inRange(HSV, Scalar(H_MIN, S_MIN, V_MIN), Scalar(H_MAX, S_MAX, V_MAX), threshold);
		inRange(HSV, Scalar(H_MIN1, S_MIN1, V_MIN1), Scalar(H_MAX1, S_MAX1, V_MAX1), threshold1);
		//perform morphological operations on thresholded image to eliminate noise
		//and emphasize the filtered object(s)
		if (useMorphOps)
		{
			morphOps(threshold);
			morphOps(threshold1);
		}
		//pass in thresholded frame to our object tracking function
		//this function will return the x and y coordinates of the
		//filtered object
		if (trackObjects)
		{
			trackFilteredObject(x, y, threshold, cameraFeed);
			trackFilteredObject(x1, y1, threshold1, cameraFeed);
			if(inceput == 0){
             xmv=x;
             ymv=y;
             xav=x1;
             yav=y1;
			}
			else{
               miscaRobot(x,y,x1,y1,sockfd);
			}
		}
		//show frames
		imshow(windowName2, threshold);
		imshow(windowName2, threshold1);
		imshow(windowName, cameraFeed);
		//imshow(windowName1, HSV);
		setMouseCallback("Original Image", on_mouse, &p);
		//delay 30ms so that screen can refresh.
		//image will not appear without this waitKey() command
		waitKey(30);
	}
*/
	return 0;
}
