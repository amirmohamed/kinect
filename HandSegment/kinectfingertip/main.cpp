// Headers
//-----------------------------------------------------------------------------
// General headers
#include <stdio.h>
// External headers
#include "lib/XEventsEmulation.cpp"
#include "lib/cvCompute.h"
#include "lib/cvFingerPoints.h"
// OpenNI headers
#include <XnOpenNI.h>
#include <XnCppWrapper.h>
// NITE headers
#include <XnVSessionManager.h>
#include <XnVWaveDetector.h>
#include <XnVPushDetector.h>
// OpenCV
#include <highgui.h>
#include <cv.h>

// xml to initialize OpenNI
#define SAMPLE_XML_FILE "./data/Sample-Tracking.xml"
#define SAMPLE_XML_FILE_LOCAL "Sample-Tracking.xml"
#define DEPTH_WINDOW "Depth"

// Global
//const Size frameSize(640, 480);
//const Mat emptyMat();
//Mat bgrMat(frameSize, CV_8UC3);
//Mat depthMap(frameSize, CV_16UC1);
//Mat depthMat8(frameSize, CV_8UC1);
//Mat depthMatBgr(frameSize, CV_8UC3);

//typedef struct output output;
struct position {
    float x;
    float y;
    float z;
    int time;
    float confidence;
    int left_clicked;
    int right_clicked;
} hand;

xn::DepthGenerator g_depthGenerator;

//-----------------------------------------------------------------------------
// Callbacks/
//-----------------------------------------------------------------------------

// Callback for when the focus is in progress
void XN_CALLBACK_TYPE SessionProgress(const XnChar* strFocus, const XnPoint3D& ptFocusPoint, XnFloat fProgress, void* UserCxt)
{
	printf("Session progress (%6.2f,%6.2f,%6.2f) - %6.2f [%s]\n", ptFocusPoint.X, ptFocusPoint.Y, ptFocusPoint.Z, fProgress,  strFocus);
}
// callback for session start
void XN_CALLBACK_TYPE SessionStart(const XnPoint3D& ptFocusPoint, void* UserCxt)
{
	printf("Session started. Please wave (%6.2f,%6.2f,%6.2f)...\n", ptFocusPoint.X, ptFocusPoint.Y, ptFocusPoint.Z);
}
// Callback for session end
void XN_CALLBACK_TYPE SessionEnd(void* UserCxt)
{
	printf("Session ended. Please perform focus gesture to start session\n");
}
// Callback for wave detection
void XN_CALLBACK_TYPE OnWaveCB(void* cxt)
{
	printf("Wave!\n");
    hand.right_clicked = 1;
}
// Callback for Push detection
void XN_CALLBACK_TYPE Push_Pushed(XnFloat fVelocity, XnFloat fAngle, void* cxt) {
    printf("Push!\n");
    hand.left_clicked = 1;
}
// callback for a new position of any hand
void XN_CALLBACK_TYPE OnPointUpdate(const XnVHandPointContext* pContext, void* cxt)
{
	//printf("%d: (%f,%f,%f) [%f]\n", pContext->nID, pContext->ptPosition.X, pContext->ptPosition.Y, pContext->ptPosition.Z, pContext->fTime);
    //printf("%d: ", pContext->nID);
    hand.x = pContext->ptPosition.X;
    hand.y = pContext->ptPosition.Y;
    hand.z = pContext->ptPosition.Z;
    hand.time = pContext->fTime;
    hand.confidence = pContext->fConfidence;
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

XnBool fileExists(const char *fn)
{
	XnBool exists;
	xnOSDoesFileExist(fn, &exists);
	return exists;
}


// this sample can run as a regular sample
int main(int argc, char** argv)
{
	xn::Context context;
	xn::ScriptNode scriptNode;
	XnVSessionGenerator* pSessionGenerator;
    const float grabConvexity = 0.8;
    Mat mask(frameSize, CV_8UC1);

    // Create context
    const char *fn = NULL;
    if      (fileExists(SAMPLE_XML_FILE)) fn = SAMPLE_XML_FILE;
    else if (fileExists(SAMPLE_XML_FILE_LOCAL)) fn = SAMPLE_XML_FILE_LOCAL;
    else {
        printf("Could not find '%s' nor '%s'. Aborting.\n" , SAMPLE_XML_FILE, SAMPLE_XML_FILE_LOCAL);
        return XN_STATUS_ERROR;
    }
    XnStatus rc = context.InitFromXmlFile(fn, scriptNode);
    if (rc != XN_STATUS_OK)
    {
        printf("Couldn't initialize: %s\n", xnGetStatusString(rc));
        return 1;
    }
    //XnStatus rc = context.Init();

    // Create the Session Manager
    pSessionGenerator = new XnVSessionManager();
    rc = ((XnVSessionManager*)pSessionGenerator)->Initialize(&context, "Click,Wave", "RaiseHand");
    if (rc != XN_STATUS_OK)
    {
        printf("Session Manager couldn't initialize: %s\n", xnGetStatusString(rc));
        delete pSessionGenerator;
        return 1;
    }

    // default output mode
    XnMapOutputMode outputMode;
    outputMode.nXRes = 640;
    outputMode.nYRes = 480;
    outputMode.nFPS = 30;
    // Depth drawing
    rc = context.FindExistingNode(XN_NODE_TYPE_DEPTH, g_depthGenerator);
    if ( rc != XN_STATUS_OK )
        printf("Finding depth node\n");
    xn::DepthMetaData depthMD;
    g_depthGenerator.GetMetaData(depthMD);
    rc = g_depthGenerator.SetMapOutputMode(outputMode);
    //rc = g_depthGenerator.GetMirrorCap().SetMirror(true);
    const XnDepthPixel*  pDepth = depthMD.Data();
    uint16_t maxDepth = depthMD.ZRes();

    // Initialization done. Start generating
    context.StartGeneratingAll();

	// Register session callbacks
	pSessionGenerator->RegisterSession(NULL, &SessionStart, &SessionEnd, &SessionProgress);

	// init & register wave control
    XnVWaveDetector wc;
    wc.RegisterWave(NULL, OnWaveCB);
    wc.RegisterPointUpdate(NULL, OnPointUpdate);
    pSessionGenerator->AddListener(&wc);

    // register push control
    XnVPushDetector pushDetector;
    pushDetector.RegisterPush(NULL, &Push_Pushed);
    pSessionGenerator->AddListener(&pushDetector);

	printf("Please perform focus gesture to start session\n");
	printf("Hit any key to exit\n");

	// Main loop
	while (!xnOSWasKeyboardHit())
	{
        context.WaitAnyUpdateAll();     // OPenNI - any wait function
        ((XnVSessionManager*)pSessionGenerator)->Update(&context);
        // Draw depth window
        Mat mat(frameSize, CV_16UC1, (unsigned char*)g_depthGenerator.GetDepthMap());
        mat.copyTo(depthMat);
        depthMat.convertTo(depthMat8, CV_8UC1, 255.0f / 3000.0f);
        cvtColor(depthMat8, depthMatBgr, CV_GRAY2BGR);
        // Segment !
        float rh[3];
        vector<Point> handContour;
        rh[0] = hand.x; rh[1] = hand.y; rh[2] = hand.z;
        unsigned char shade = 255 - (unsigned char)(hand.z * 128.0f);
        Scalar color(0, 0, shade);
        getHandContour(depthMat, rh, handContour);
        //bool grasp = convexity(handContour) > grabConvexity;
        //int thickness = grasp ? CV_FILLED : 3;
        //circle(depthMatBgr, Point(rh[0], rh[1]), 10, color, thickness);
        imshow("depthMatBgr", depthMatBgr);
        // Detection
        //vector<Point> fingerTips;
        //detectFingerTips(handContour, fingerTips, &depthMatBgr);
        int k = cvWaitKey(10);
	}

	delete pSessionGenerator;

    //cvReleaseImage(&depthMap);
    cvDestroyAllWindows();

	return 0;
}
