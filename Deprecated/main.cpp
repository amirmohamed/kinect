// Headers
//-----------------------------------------------------------------------------
// General headers
#include <stdio.h>
// External headers
#include "lib/XEventsEmulation.cpp"
// OpenNI headers
#include <XnOpenNI.h>
// NITE headers
#include <XnVSessionManager.h>
#include <XnVWaveDetector.h>
#include <XnVPushDetector.h>

// xml to initialize OpenNI
#define SAMPLE_XML_FILE "./data/Sample-Tracking.xml"
#define SAMPLE_XML_FILE_LOCAL "Sample-Tracking.xml"

// Structures
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
    printf("%d: ", pContext->nID);
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

    // Create the Session Manager
    pSessionGenerator = new XnVSessionManager();
    rc = ((XnVSessionManager*)pSessionGenerator)->Initialize(&context, "Click,Wave", "RaiseHand");
    if (rc != XN_STATUS_OK)
    {
        printf("Session Manager couldn't initialize: %s\n", xnGetStatusString(rc));
        delete pSessionGenerator;
        return 1;
    }

    // Initialization done. Start generating
    context.StartGeneratingAll();

	// Register session callbacks
	pSessionGenerator->RegisterSession(NULL, &SessionStart, &SessionEnd, &SessionProgress);

    // Some more controls, check pointViewer
    // XnControl creation, for message handling, like XnPointDenoiser or XnVPointControl
    // pFlowRouter = new XnVFlowRouter;
    // pFlowRouter->SetActive(XnControl);   Broadcaster?
    // Handle other XnCOntrol
    // ...
    // pSessionGenerator->AddListener(pFlowRouter);

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
    XEventsEmulation interface;
    int smooth_x, smooth_y;
	while (!xnOSWasKeyboardHit())
	{
        context.WaitAnyUpdateAll();     // OPenNI - any wait function
        ((XnVSessionManager*)pSessionGenerator)->Update(&context);
        smooth_x = interface.smoothingPoint(hand.x, 'x');
        smooth_y = interface.smoothingPoint(hand.y, 'y');
        interface.mouseMove(smooth_x, smooth_y);
        printf("X: %f -> %d\t", hand.x, smooth_x);
        printf("Y: %f -> %d\t", hand.y, smooth_y);
        printf("( %d - %f )\n", hand.time, hand.confidence);
        if ( hand.left_clicked ) {
            interface.mouseClick("2", "click");
            hand.left_clicked = 0;
        }
        if ( hand.right_clicked ) {
            interface.keyHit("Home", "");
            hand.right_clicked = 0;
        }
	}

	delete pSessionGenerator;

	return 0;
}
