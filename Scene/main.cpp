// Headers
//-----------------------------------------------------------------------------
// General headers
//#include <stdio.h>
// External headers
// OpenNI headers
#include <XnOpenNI.h>
#include <XnCodecIDs.h>
#include <XnCppWrapper.h>

xn::SceneAnalyser sceneAnalyser;

// xml to initialize OpenNI
#define SAMPLE_XML_FILE "./data/Sample-Tracking.xml"
#define SAMPLE_XML_FILE_LOCAL "Sample-Tracking.xml"
#define CHECK_ERRORS(rc, errors, what)          \
    if ( rc == XN_STATUS_NO_NODE_PRESENT )      \
    {                                           \
    XnChar strError[1024];                      \
    errors.ToString(strError, 1024);            \
    printf("%s\n", strError );                  \
    return(rc);                                 \
    }

#define CHECK_RC(rc, what) \
    if (rc !=  XN_STATUS_OK)                                \
    {                                                       \
    printf("%s failed: %s\n", what, xnGetStatusString(rc)); \
    return (rc);                                            \
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
    xn::DepthGenerator depthGenerator;
    xn::DepthMetaData depthMD;
    xn::SceneMetaData sceneMD;
    xn::EnumerationErrors errors;

    // Create context
    const char *fn = NULL;
    if      (fileExists(SAMPLE_XML_FILE)) fn = SAMPLE_XML_FILE;
    else {
        printf("Could not find '%s' nor '%s'. Aborting.\n" , SAMPLE_XML_FILE, SAMPLE_XML_FILE_LOCAL);
        return XN_STATUS_ERROR;
    }
    XnStatus rc = context.InitFromXmlFile(fn, scriptNode);
    CHECK_ERRORS(rc, errors, "InitFromXmlFile");
    CHECK_RC(rc, "InitFromXmlFile");

    // Scene analyser
    rc = context.FindExistingNode(XN_NODE_TYPE_SCENE, sceneAnalyser);
    CHECK_RC(rc, "Find scene analyser");
    // Depth generator
    rc = context.FindExistingNode(XN_NODE_TYPE_DEPTH, depthGenerator);
    CHECK_RC(rc, "Find depth generator");

    // Initialization done. Start generating
    rc = context.StartGeneratingAll();
    CHECK_RC(rc, "StartGenerating");

	// Main loop
	while (!xnOSWasKeyboardHit())
	{
        context.WaitAnyUpdateAll();     // OPenNI - any wait function
        depthGenerator.GetMetaData(depthMD);
        sceneAnalyser.GetMetaData(sceneMD);
	}

	return 0;
}
