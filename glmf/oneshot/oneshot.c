#include "oneshot.h"

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

    // Color image generator
    XnNodeHandle imageGenerator;
    status = xnCreateImageGenerator (context, &imageGenerator, NULL, NULL);
    CHK_NI_ERROR (status, "Cannot create an image generator");
    // Mirror the image
    //XnBool isMirror = -1;
    //xnSetMirror(imageGenerator, isMirror);


    // Check if changing the view point is supported
    if (!xnIsCapabilitySupported (depthGenerator, XN_CAPABILITY_ALTERNATIVE_VIEW_POINT))
    {
        fprintf (stderr, "Changing view point capability not supported.\n");
        exit (EXIT_FAILURE);
    }
    else
    {
        // Changing the view point of the depth generator
        xnSetViewPoint (depthGenerator, imageGenerator);
    }

    // Get the metadata of the generators
    //XnImageMetaData *imageMetaData = xnAllocateImageMetaData ();
    XnDepthMetaData *depthMetaData = xnAllocateDepthMetaData ();
    xnGetDepthMetaData (depthGenerator, depthMetaData);
    //xnGetImageMetaData (imageGenerator, imageMetaData);

    // Create the IplImage
    IplImage *depthMap = 
        cvCreateImage (cvSize (depthMetaData->pMap->FullRes.X,
                               depthMetaData->pMap->FullRes.Y),
                       IPL_DEPTH_16U, 1);
    IplImage *colorDepthMap =
        cvCreateImage (cvSize (depthMetaData->pMap->FullRes.X,
                               depthMetaData->pMap->FullRes.Y),
                       IPL_DEPTH_8U, 3);
    /*IplImage *imageMap =
        cvCreateImage (cvSize (imageMetaData->pMap->FullRes.X,
                               imageMetaData->pMap->FullRes.Y),
                       IPL_DEPTH_8U, 3);
    */

    uint16_t maxDepth = depthMetaData->nZRes;

    printf ("Depth MetaDatas (%d, %d) with %d\n", depthMap->width, depthMap->height, maxDepth);

    // Free metadata
    xnFreeDepthMetaData (depthMetaData);
    //xnFreeImageMetaData (imageMetaData);

    // User Generator
    XnNodeHandle userGenerator;
    status = xnCreateUserGenerator (context, &userGenerator, NULL, NULL);
    CHK_NI_ERROR (status, "Cannot create a user generator");

    // Verify that the userGenerator has all the needed capability
    if (!xnIsCapabilitySupported (userGenerator, XN_CAPABILITY_SKELETON) ||
        !xnIsCapabilitySupported (userGenerator, XN_CAPABILITY_POSE_DETECTION))
    {
        fprintf (stderr, "The skeleton capability is not supported.\n");
        exit (EXIT_FAILURE);
    }

    // Get the name of the pose needed for calibration
    XnChar poseForCalibration[20];
    xnGetSkeletonCalibrationPose (userGenerator, poseForCalibration);
    printf ("Pose for calibration is : %s\n", poseForCalibration);

    // Register callbacks for user detection
    XnCallbackHandle userCallbackHandle;
    status = xnRegisterUserCallbacks (userGenerator, &newUserCB, &lostUserCB,
                                      poseForCalibration, &userCallbackHandle);
    CHK_NI_ERROR (status, "Registering user callbacks failed");

    // Register callbacks for pose detection
    XnCallbackHandle poseCallbackHandle;
    status = xnRegisterToPoseDetected (userGenerator, &poseDetectionCB, NULL,
                                       &poseCallbackHandle);
    CHK_NI_ERROR (status, "Registering pose detection callbacks failed");

    // Register callbacks for skeleton calibration
    XnCallbackHandle calibrationCallbackHandle;
    status = xnRegisterToCalibrationComplete (userGenerator,
                                              &calibrationCompleteCB,
                                              poseForCalibration,
                                              &calibrationCallbackHandle);
    CHK_NI_ERROR (status, "Registering calibration callbacks failed");


    // Active all joints
    xnSetSkeletonProfile (userGenerator, XN_SKEL_PROFILE_ALL);


    // Launching the generators
    xnStartGeneratingAll (context);

    int die = 0;
    int c = 0;
    int cpt = 0;
    int x_right_hand = 0;
    int x_left_hand = 0;

    while (!die)
    {
        // Compteur de tour
        cpt += 1;

        // Wait for new images
        status = xnWaitAndUpdateAll (context);
        CHK_NI_ERROR (status, "Cannot capture new images");

        // Depth
        memcpy (depthMap->imageData, xnGetDepthMap (depthGenerator),
                depthMap->imageSize);
        convertDepthImageToColorImage (depthMap, colorDepthMap, maxDepth);

        // Color image
        //memcpy (imageMap->imageData, xnGetRGB24ImageMap (imageGenerator),
        //        imageMap->imageSize);

        // Get the mean depth
        uint16_t avgDepth = cvAvg (depthMap, NULL).val[0];

        // Retrieving the skeleton
        XnUserID users[5]; // A maximum a 5 users
        uint16_t userCount = 5;
        xnGetUsers (userGenerator, users, &userCount);

        for (int i = 0; i < userCount; i++)
        {
            if (!xnIsSkeletonTracking (userGenerator, users[i]))
                continue;

            // Do not draw, collar, wrist, fingertip, waist and ankle: not available with NITE
            /*drawJoint (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_LEFT_HAND);*/
            /*drawJoint (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_RIGHT_HAND);*/


            x_left_hand = drawJoint (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_LEFT_HAND, 1);
            x_right_hand = drawJoint (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_RIGHT_HAND, 0);

            /*
             *if ( cpt > 10 ) {
             *    if ( abs(x_right_hand - x_left_hand) < 200) {
             *        printf("Click !\n");
             *        system("xte 'mouseclick 2'");
             *        cpt = 0;
             *    }
             *    if ( abs(x_right_hand - x_left_hand) > 1000)  {
             *        printf("Quit !\n");
             *        die = 1;
             *    }
             *}
             */
        }

        //displayDepthImage (DEPTH_WINDOW, depthMap, maxDepth);
        displayRGBImage (DEPTH_WINDOW, colorDepthMap);
        //displayRGBImage (COLOR_WINDOW, imageMap);

        c = cvWaitKey (20 /*ms */);
        //printf("Here is c: %d\n", c);

        if (c == 1048689) {
            die = 1;
        }
    }

    //cvReleaseImage (&imageMap);
    cvReleaseImage (&depthMap);
    cvReleaseImage (&colorDepthMap);

    cvDestroyAllWindows ();

    xnStopGeneratingAll (context);
    xnContextRelease (context);

    exit (EXIT_SUCCESS);
}
