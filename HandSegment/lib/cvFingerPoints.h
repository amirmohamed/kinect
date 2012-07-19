//============================================================================
// Name        : KinectFingertip.cpp
// Author      : Robert Walter - github.com/robbeofficial
// Version     : 0.something
// Description : an approach for fingertip tracking using kinect
//============================================================================

#include <iostream>
#include <sstream>
#include <fstream>
using namespace std;

#include <opencv/cv.h>
#include <opencv/highgui.h>
using namespace cv;

#include <XnCppWrapper.h>
using namespace xn;

/*//////////////////////////////////////////////////////////////////////
// constants
//////////////////////////////////////////////////////////////////////*/

const Size frameSize(640, 480);
const unsigned int maxUsers = 20;
const Mat emptyMat();

/*//////////////////////////////////////////////////////////////////////
// globals
//////////////////////////////////////////////////////////////////////*/

// openNI context and generator nodes
Context context;
ImageGenerator imageGen;
DepthGenerator depthGen;
IRGenerator irGen;
UserGenerator userGen;

// frame buffers
Mat bgrMat(frameSize, CV_8UC3);
Mat depthMat(frameSize, CV_16UC1);
Mat depthMat8(frameSize, CV_8UC1);
Mat depthMatBgr(frameSize, CV_8UC3);

/*//////////////////////////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////////////////////////*/

void initKinect(bool initImage, bool initDepth, bool initIR, bool initUser) {
	XnStatus nRetVal = XN_STATUS_OK;

	// Initialize context object
	nRetVal = context.Init();
	cout << "init : " << xnGetStatusString(nRetVal) << endl;

	// default output mode
	XnMapOutputMode outputMode;
	outputMode.nXRes = 640;
	outputMode.nYRes = 480;
	outputMode.nFPS = 30;

	// Create an ImageGenerator node
	if (initImage) {	
		nRetVal = imageGen.Create(context);
		cout << "imageGen.Create : " << xnGetStatusString(nRetVal) << endl;
		nRetVal = imageGen.SetMapOutputMode(outputMode);
		cout << "imageGen.SetMapOutputMode : " << xnGetStatusString(nRetVal) << endl;
		nRetVal = imageGen.GetMirrorCap().SetMirror(true);
		cout << "imageGen.GetMirrorCap().SetMirror : " << xnGetStatusString(nRetVal) << endl;
	}

	// Create a DepthGenerator node	
	if (initDepth) {
		nRetVal = depthGen.Create(context);
		cout << "depthGen.Create : " << xnGetStatusString(nRetVal) << endl;
		nRetVal = depthGen.SetMapOutputMode(outputMode);
		cout << "depthGen.SetMapOutputMode : " << xnGetStatusString(nRetVal) << endl;
		nRetVal = depthGen.GetMirrorCap().SetMirror(true);
		cout << "depthGen.GetMirrorCap().SetMirror : " << xnGetStatusString(nRetVal) << endl;
	}

	// Create an IRGenerator node
	if (initIR) {
		nRetVal = irGen.Create(context);
		cout << "irGen.Create : " << xnGetStatusString(nRetVal) << endl;
		nRetVal = irGen.SetMapOutputMode(outputMode);
		cout << "irGen.SetMapOutputMode : " << xnGetStatusString(nRetVal) << endl;
	}

	// create user generator
	if (initUser) {
		nRetVal = userGen.Create(context);
		cout << "userGen.Create : " << xnGetStatusString(nRetVal) << endl;
		userGen.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);
	}

	// Make it start generating data
	nRetVal = context.StartGeneratingAll();
	cout << "context.StartGeneratingAll : " << xnGetStatusString(nRetVal) << endl;
}

float getJointImgCoordinates(const SkeletonCapability &skeletonCapability, const XnUserID userId, const XnSkeletonJoint skeletonJoint, float *v) {
	XnVector3D projective;
	XnSkeletonJointPosition skeletonJointPosition;
	skeletonCapability.GetSkeletonJointPosition(userId, skeletonJoint, skeletonJointPosition);
	
	depthGen.ConvertRealWorldToProjective(1, &skeletonJointPosition.position, &projective);

	v[0] = projective.X;
	v[1] = projective.Y;
	v[2] = projective.Z / 1000.0f;

	return skeletonJointPosition.fConfidence;
}

bool getHandContour(const Mat &depthMat, const float *v, vector<Point> &handContour) {
	const int maxHandRadius = 128; // in px
	const short handDepthRange = 200; // in mm
	const double epsilon = 17.5; // approximation accuracy (maximum distance between the original hand contour and its approximation)

	unsigned short depth = (unsigned short) (v[2] * 1000.0f); // hand depth
	unsigned short near = depth - 100; // near clipping plane
	unsigned short far = depth + 100; // far clipping plane

	static Mat mask(frameSize, CV_8UC1);
	mask.setTo(0);

	// extract hand region	
	circle(mask, Point(v[0], v[1]), maxHandRadius, 255, CV_FILLED);
	mask = mask & depthMat > near & depthMat < far;

	// DEBUG(show mask)
	//imshow("mask1", mask);

	// assume largest contour in hand region to be the hand contour
	vector<vector<Point> > contours;
	findContours(mask, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
	int n = contours.size();
	int maxI = -1;
	int maxSize = -1;
	for (int i=0; i<n; i++) {
		int size  = contours[i].size();
		if (size > maxSize) {
			maxSize = size;
			maxI = i;
		}
	}

	bool handContourFound = (maxI >= 0);

	if (handContourFound) {
		approxPolyDP( Mat(contours[maxI]), handContour, epsilon, true );
		//handContour = contours[maxI];
	}

//	// DEBUG(draw hand contour
    //mask.setTo(0);	
    //if (maxI >= 0) {
        //vector<vector<Point> > contours2;
        //contours2.push_back(contours[maxI]);
        //drawContours(mask, contours2, -1, 255);
        //imshow("mask2", mask);
    //}

	return maxI >= 0;
}

void detectFingerTips(const vector<Point> &handContour, vector<Point> &fingerTips, Mat *debugFrame = NULL) {
	Mat handContourMat(handContour);
	double area = cv::contourArea(handContourMat);

	const Scalar debugFingerTipColor(255,0,0);

	vector<int> hull;
	cv::convexHull(handContourMat, hull);

	// find upper and lower bounds of the hand and define cutoff threshold (don't consider lower vertices as fingers)
	int upper = 640, lower = 0;
	for (int j=0; j<hull.size(); j++) {
		int idx = hull[j]; // corner index
		if (handContour[idx].y < upper) upper = handContour[idx].y;
		if (handContour[idx].y > lower) lower = handContour[idx].y;
	}
	float cutoff = lower - (lower - upper) * 0.1f;

	// find interior angles of hull corners
	for (int j=0; j<hull.size(); j++) {
		int idx = hull[j]; // corner index
		int pdx = idx == 0 ? handContour.size() - 1 : idx - 1; //  predecessor of idx
		int sdx = idx == handContour.size() - 1 ? 0 : idx + 1; // successor of idx

		Point v1 = handContour[sdx] - handContour[idx];
		Point v2 = handContour[pdx] - handContour[idx];

		float angle = acos( (v1.x*v2.x + v1.y*v2.y) / (norm(v1) * norm(v2)) );

		// low interior angle + within upper 90% of region -> we got a finger
		if (angle < 1 && handContour[idx].y < cutoff) {
			int u = handContour[idx].x;
			int v = handContour[idx].y;

			fingerTips.push_back(Point2i(u,v));
			
			if (debugFrame) { // draw fingertips
				cv::circle(*debugFrame, handContour[idx], 10, debugFingerTipColor, -1);
			}
		}
	}

	if (debugFrame) {
		// draw cutoff threshold
		cv::line(*debugFrame, Point(0, cutoff), Point(640, cutoff), debugFingerTipColor);

		// draw approxCurve
		for (int j=0; j<handContour.size(); j++) {
			cv::circle(*debugFrame, handContour[j], 10, debugFingerTipColor);
			if (j != 0) {
				cv::line(*debugFrame, handContour[j], handContour[j-1], debugFingerTipColor);
			} else {
				cv::line(*debugFrame, handContour[0], handContour[handContour.size()-1], debugFingerTipColor);
			}
		}

		// draw approxCurve hull
		for (int j=0; j<hull.size(); j++) {
			cv::circle(*debugFrame, handContour[hull[j]], 10, debugFingerTipColor, 3);
			if(j == 0) {
				cv::line(*debugFrame, handContour[hull[j]], handContour[hull[hull.size()-1]], debugFingerTipColor);
			} else {
				cv::line(*debugFrame, handContour[hull[j]], handContour[hull[j-1]], debugFingerTipColor);
			}
		}
	}
}

void drawContour(Mat &img, const vector<Point> &contour, const Scalar &color) {
	vector<vector<Point> > contours;
	contours.push_back(contour);
	drawContours(img, contours, -1, color);
}

double convexity(const vector<Point> &contour) {
	Mat contourMat = Mat(contour);

	vector<int> hull;
	convexHull(contourMat, hull);

	int n = hull.size();
	vector<Point> hullContour;

	for (int i=0; i<n; i++) {
		hullContour.push_back(contour[hull[i]]);
	}

	Mat hullContourMat(hullContour);

	return (contourArea(contourMat) / contourArea(hullContourMat));
}
