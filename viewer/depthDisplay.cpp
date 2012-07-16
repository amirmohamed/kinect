#include "depthDisplay.h"

int main (int argc, char **argv)
{
    // OpenNI initialization
    XnStatus status;
    XnContext *context = NULL;

    status = xnInit (&context);
    CHK_NI_ERROR (status, "Initialisation error");

    // depth generator
    XnNodeHandle depthGenerator;
    status = xnCreateDepthGenerator (context, &depthGenerator, NULL, NULL);
    CHK_NI_ERROR (status, "Cannot create a depth generator");

    // Get the metadata of the generators
    XnDepthMetaData *depthMetaData = xnAllocateDepthMetaData ();
    xnGetDepthMetaData (depthGenerator, depthMetaData);

    // Create the IplImage
    IplImage *depthMap = 
        cvCreateImage (cvSize (depthMetaData->pMap->FullRes.X,
                               depthMetaData->pMap->FullRes.Y),
                       IPL_DEPTH_16U, 1);

    uint16_t maxDepth = depthMetaData->nZRes;

    printf ("Depth MetaDatas (%d, %d) with %d\n", depthMap->width, depthMap->height, maxDepth);

    // Free metadata
    xnFreeDepthMetaData (depthMetaData);

    // Launching the generators
    xnStartGeneratingAll (context);

    int die = 0;
    int c = 0;
    while (!die)
    {
        // Wait for new images
        status = xnWaitAndUpdateAll (context);
        CHK_NI_ERROR (status, "Cannot capture new images");

        // Depth
        memcpy (depthMap->imageData, xnGetDepthMap (depthGenerator),
                depthMap->imageSize);
//        convertDepthImageToColorImage (depthMap, colorDepthMap, maxDepth);

        // Get the mean depth
        uint16_t avgDepth = cvAvg (depthMap, NULL).val[0];

        //displayDepthImage (DEPTH_WINDOW, depthMap, maxDepth);
        displayDepthPlan (DEPTH_WINDOW, depthMap, maxDepth, 100);

        c = cvWaitKey (20 /*ms */);
        printf("Mean depth: %d\n", avgDepth);

        if (c == 1048689) {
            die = 1;
        }
    }

    cvReleaseImage (&depthMap);

    cvDestroyAllWindows ();

    xnStopGeneratingAll (context);
    xnContextRelease (context);

    exit (EXIT_SUCCESS);
}
