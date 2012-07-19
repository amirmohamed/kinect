// Headers
//-----------------------------------------------------------------------------
// General headers
#include <stdio.h>
using namespace std;
// External headers
#include "lib/XEventsEmulation.cpp"
//#include "lib/cvCompute.h"
#include "lib/cvFingerPoints.h"
// OpenNI headers
#include <XnOpenNI.h>
#include <XnCppWrapper.h>
// NITE headers
#include <XnVSessionManager.h>
#include <XnVWaveDetector.h>
#include <XnVPushDetector.h>
using namespace xn;
// OpenCV
#include <cv.h>
#include <highgui.h>
using namespace cv;

// xml to initialize OpenNI
#define SAMPLE_XML_FILE "./data/Sample-Tracking.xml"
#define SAMPLE_XML_FILE_LOCAL "Sample-Tracking.xml"
#define DEPTH_WINDOW "Depth"

typedef struct handTracked handTracked;
struct handTracked {
    XnVector3D position;
    int time;
    float confidence;
    int left_clicked;
    int right_clicked;
} g_handR;
handTracked g_handL;

// Global
DepthGenerator g_depthGenerator;

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
    g_handR.right_clicked = 1;
}
// Callback for Push detection
void XN_CALLBACK_TYPE Push_Pushed(XnFloat fVelocity, XnFloat fAngle, void* cxt) {
    printf("Push!\n");
    g_handR.left_clicked = 1;
}
// callback for a new position of any hand
void XN_CALLBACK_TYPE OnPointUpdate(const XnVHandPointContext* pContext, void* cxt)
{
	//printf("%d: (%f,%f,%f) [%f]\n", pContext->nID, pContext->ptPosition.X, pContext->ptPosition.Y, pContext->ptPosition.Z, pContext->fTime);
    if ( pContext->nID == 1 ) {
        //g_handR.time = pContext->fTime;
        //g_handR.confidence = pContext->fConfidence;
        g_handR.position.X = pContext->ptPosition.X;
        g_handR.position.Y = pContext->ptPosition.Y;
        g_handR.position.Z = pContext->ptPosition.Z;
    }
    else {
        g_handL.position.X = pContext->ptPosition.X;
        g_handL.position.Y = pContext->ptPosition.Y;
        g_handL.position.Z = pContext->ptPosition.Z;
    }
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
	Context context;
	ScriptNode scriptNode;
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
    DepthMetaData depthMD;
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
    XnVector3D projectiveHandPosition;
    float rh[3];
    vector<Point> handContour;   
    unsigned char shade;
    Scalar color;
    vector<Point> fingerTips;
    XEventsEmulation interface;
    int smooth_x, smooth_y;
    int cpt(0);
	while (!xnOSWasKeyboardHit())
	{
        context.WaitAnyUpdateAll();     // OPenNI - any wait function
        ((XnVSessionManager*)pSessionGenerator)->Update(&context);
        // Draw depth window
        Mat mat(frameSize, CV_16UC1, (unsigned char*)g_depthGenerator.GetDepthMap());
        mat.copyTo(depthMat);
        depthMat.convertTo(depthMat8, CV_8UC1, 255.0f / 3000.0f);
        cvtColor(depthMat8, depthMatBgr, CV_GRAY2BGR);
        // Segment Right hand
        handContour.clear();
        g_depthGenerator.ConvertRealWorldToProjective(1, &g_handR.position, &projectiveHandPosition);
        rh[0] = projectiveHandPosition.X; rh[1] = projectiveHandPosition.Y; rh[2] = projectiveHandPosition.Z / 1000.0f;
        char shade = 255 - (unsigned char)(g_handR.position.Z * 128.0f);
        color = Scalar(0, 0, shade);
             */
        getHandContour(depthMat, rh, handContour);
        if ( !handContour.empty() ) {     // At least one hand is tracked
            bool grasp = convexity(handContour) > grabConvexity;
            int thickness = grasp ? CV_FILLED : 3;
            circle(depthMatBgr, Point(rh[0], rh[1]), 10, color, thickness);
            // Detection
            fingerTips.clear();
            detectFingerTips(handContour, fingerTips, &depthMatBgr);
            // Computer control 
            if ( (fingerTips.empty() ) && (cpt==0) ) {
                interface.mouseClick("2", "click");
                cpt = 60;
            }
            else {
                smooth_x = interface.smoothingPoint(g_handR.position.X, 'x');
                smooth_y = interface.smoothingPoint(g_handR.position.Y, 'y');
                interface.mouseMove(smooth_x, smooth_y);
            }
        }
        // Segment Right hand
        handContour.clear();
        g_depthGenerator.ConvertRealWorldToProjective(1, &g_handL.position, &projectiveHandPosition);
        rh[0] = projectiveHandPosition.X; rh[1] = projectiveHandPosition.Y; rh[2] = projectiveHandPosition.Z / 1000.0f;
        shade = 255 - (unsigned char)(g_handL.position.Z * 128.0f);
        color = Scalar(0, 0, shade);
        getHandContour(depthMat, rh, handContour);
        if ( !handContour.empty() ) {     // At least one hand is tracked
            bool grasp = convexity(handContour) > grabConvexity;
            int thickness = grasp ? CV_FILLED : 3;
            circle(depthMatBgr, Point(rh[0], rh[1]), 10, color, thickness);
            // Detection
            fingerTips.clear();
            detectFingerTips(handContour, fingerTips, &depthMatBgr);
        }
        if ( cpt != 0 ) cpt--;
        imshow("depthMatBgr", depthMatBgr);
        int k = cvWaitKey(10);
	}

	delete pSessionGenerator;

    //cvReleaseImage(&depthMap);
    cvDestroyAllWindows();

	return 0;
}
