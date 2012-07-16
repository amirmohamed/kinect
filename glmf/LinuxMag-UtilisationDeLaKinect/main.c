#include <stdlib.h>
#include <stdio.h>

#include <highgui.h>
#include <cv.h>

#include <XnOpenNI.h>

#include <XnModuleInterface.h>

#define DEPTH_WINDOW "Depth"
#define COLOR_WINDOW "Color"

#define MM_IN_ONE_METER 1000.

#define CHK_NI_ERROR(status, msg) if (status != XN_STATUS_OK) \
{ \
    fprintf (stderr, "%s: %s\n", msg, xnGetStatusString(status)); \
}

void newUserCB (XnNodeHandle userGenerator, XnUserID user, void *cookie)
{
    printf ("New user detected (%d)\n", user);

    if (xnNeedPoseForSkeletonCalibration (userGenerator))
    {
        char *pose = (char *)cookie;
        xnStartPoseDetection (userGenerator, pose, user);
    }
    else
    {
        // Directly request calibration
        xnRequestSkeletonCalibration (userGenerator, user, TRUE);
    }
}

void lostUserCB (XnNodeHandle userGenerator, XnUserID user, void *cookie)
{
    printf ("Warning: lost user (%d)\n", user);
}

void poseDetectionCB (XnNodeHandle userGenerator, const XnChar *pose,
                      XnUserID user, void *cookie)
{
    printf ("Pose '%s' detected: starting calibration for user (%d)\n",
            pose, user);
    xnRequestSkeletonCalibration (userGenerator, user, TRUE);
}

void calibrationCompleteCB (XnNodeHandle userGenerator, XnUserID user,
                            XnCalibrationStatus status, void *cookie)
{
    if (status == XN_CALIBRATION_STATUS_OK)
    {
        printf ("Calibration done for user (%d)\n", user);
        xnStartSkeletonTracking (userGenerator, user);
    }
    else
    {
        printf ("There was a problem during the calibration! Starting again...\n");
        char *pose = (char *)cookie;
        xnStartPoseDetection (userGenerator, pose, user);
    }
}

void convertDepthImageToColorImage (const IplImage *src, IplImage *dst,
                                    const uint16_t maxDepth)
{
    for (int row = 0; row < src->height; row++)
    {
        for (int col = 0; col < src->width; col++)
        {
            CV_IMAGE_ELEM (dst, uint8_t, row, 3*col) =
            CV_IMAGE_ELEM (dst, uint8_t, row, 3*col + 1) =
            CV_IMAGE_ELEM (dst, uint8_t, row, 3*col + 2) =
                (255 * CV_IMAGE_ELEM (src, uint16_t, row, col)) / maxDepth;
        }
    }
}

void displayDepthImage (const char *name, const IplImage *image,
                        const uint16_t maxDepth)
{
    IplImage *normalized = cvCreateImage (cvGetSize (image), IPL_DEPTH_8U, 1);

    // Normalized each pixel between 0 and 255
    for (int row = 0; row < image->height; row++)
    {
        for (int col = 0; col < image->width; col++)
        {
            CV_IMAGE_ELEM (normalized, uint8_t, row, col) = 
                (255 * CV_IMAGE_ELEM (image, uint16_t, row, col)) / maxDepth;
        }
    }

    // Display the normalized image
    cvShowImage (name, normalized);

    // Free memory
    cvReleaseImage (&normalized);
}

void drawJoint (XnNodeHandle depthGenerator, XnNodeHandle userGenerator,
                IplImage *img, XnUserID user, XnSkeletonJoint joint)
{
    if (!xnIsSkeletonTracking (userGenerator, user))
        return;

    XnSkeletonJointPosition jointPosition;
    xnGetSkeletonJointPosition (userGenerator, user, joint, &jointPosition);

    if (jointPosition.fConfidence == 0.0)
    {
        //printf ("There is no confidence in the position of joint %d\n", joint);
        return;
    }

    // Convert the real world coordinate of the joint to projective
    XnPoint3D projectivePosition;
    xnConvertRealWorldToProjective (depthGenerator, 1, &jointPosition.position,
                                    &projectivePosition);

    //printf ("Drawing joint %d at (%f, %f, %f) with confidence %f\n", joint, projectivePosition.X, projectivePosition.Y, projectivePosition.Z, jointPosition.fConfidence);
    CvPoint p = cvPoint (projectivePosition.X, projectivePosition.Y);
    cvCircle (img, p, 10, cvScalar (255, 0, 0, 0), 0, 8, 0);
}

void linkJoints (XnNodeHandle depthGenerator, XnNodeHandle userGenerator,
                 IplImage *img, XnUserID user, XnSkeletonJoint joint1,
                 XnSkeletonJoint joint2)
{
    if (!xnIsSkeletonTracking (userGenerator, user))
        return;

    XnSkeletonJointPosition joint1Position, joint2Position;
    xnGetSkeletonJointPosition (userGenerator, user, joint1, &joint1Position);
    xnGetSkeletonJointPosition (userGenerator, user, joint2, &joint2Position);

    // Convert the real world coordinate of the joint to projective
    XnPoint3D projectivePosition[2];
    XnPoint3D realWorldPosition[2] = {joint1Position.position,
        joint2Position.position};
    xnConvertRealWorldToProjective (depthGenerator, 2, realWorldPosition,
                                    projectivePosition);

    CvPoint p1 = cvPoint (projectivePosition[0].X, projectivePosition[0].Y);
    CvPoint p2 = cvPoint (projectivePosition[1].X, projectivePosition[1].Y);

    cvLine (img, p1, p2, cvScalar (0, 255, 0, 0), 1, 8, 0);
}

void displayRGBImage (const char *name, IplImage *image)
{
    IplImage *imageBGR = cvCreateImage (cvGetSize (image), IPL_DEPTH_8U, 3);
    cvCvtColor (image, imageBGR, CV_RGB2BGR);
    cvShowImage (name, imageBGR);
    cvReleaseImage (&imageBGR);
}

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
    XnImageMetaData *imageMetaData = xnAllocateImageMetaData ();
    XnDepthMetaData *depthMetaData = xnAllocateDepthMetaData ();
    xnGetDepthMetaData (depthGenerator, depthMetaData);
    xnGetImageMetaData (imageGenerator, imageMetaData);

    // Create the IplImage
    IplImage *depthMap = 
        cvCreateImage (cvSize (depthMetaData->pMap->FullRes.X,
                               depthMetaData->pMap->FullRes.Y),
                       IPL_DEPTH_16U, 1);
    IplImage *colorDepthMap =
        cvCreateImage (cvSize (depthMetaData->pMap->FullRes.X,
                               depthMetaData->pMap->FullRes.Y),
                       IPL_DEPTH_8U, 3);
    IplImage *imageMap =
        cvCreateImage (cvSize (imageMetaData->pMap->FullRes.X,
                               imageMetaData->pMap->FullRes.Y),
                       IPL_DEPTH_8U, 3);

    uint16_t maxDepth = depthMetaData->nZRes;

    printf ("(%d, %d) with %d\n", depthMap->width, depthMap->height, maxDepth);

    // Free metadata
    xnFreeDepthMetaData (depthMetaData);
    xnFreeImageMetaData (imageMetaData);

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

    while (!die)
    {
        // Wait for new images
        status = xnWaitAndUpdateAll (context);
        CHK_NI_ERROR (status, "Cannot capture new images");

        // Depth
        memcpy (depthMap->imageData, xnGetDepthMap (depthGenerator),
                depthMap->imageSize);
        convertDepthImageToColorImage (depthMap, colorDepthMap, maxDepth);

        // Color image
        memcpy (imageMap->imageData, xnGetRGB24ImageMap (imageGenerator),
                imageMap->imageSize);

        // Get the mean depth
        uint16_t avgDepth = cvAvg (depthMap, NULL).val[0];

        // Compute the average distance
        float avgDist = 0;
        int validPixels = 0;

        XnPoint3D *projectiveCoordinates = malloc (sizeof (XnPoint3D) * depthMap->width * depthMap->height);
        XnPoint3D *realCoordinates = malloc (sizeof (XnPoint3D) * depthMap->width * depthMap->height);
        for (int row = 0; row < depthMap->height; row++)
        {
            for (int col = 0; col < depthMap->width; col++)
            {
                uint16_t depth = CV_IMAGE_ELEM (depthMap, uint16_t, row, col);
                if (depth == 0)
                    continue;
                projectiveCoordinates[validPixels].X = col;
                projectiveCoordinates[validPixels].Y = row;
                projectiveCoordinates[validPixels++].Z = depth;
                /*
                 * or: XnPoint3D *p = &projectiveCoordinates[validPixels++];
                 * p->X = row; p->Y = col; p->Z = depth;
                 */
            }
        }
        xnConvertProjectiveToRealWorld (depthGenerator, validPixels, projectiveCoordinates, realCoordinates);

        for (int i = 0; i < validPixels; i++)
        {
            XnPoint3D p = realCoordinates[i];
            avgDist += sqrt (p.X * p.X + p.Y * p.Y + p.Z * p.Z);
        }
        avgDist /= validPixels;

        free (projectiveCoordinates);
        free (realCoordinates);

        /*printf ("Average depth = %fm, Average distance = %fm\n",
                (float)avgDepth / MM_IN_ONE_METER,
                (float) avgDist / MM_IN_ONE_METER);
        */

        // Retrieving the skeleton
        XnUserID users[5]; // A maximum a 5 users
        uint16_t userCount = 5;
        xnGetUsers (userGenerator, users, &userCount);

        for (int i = 0; i < userCount; i++)
        {
            if (!xnIsSkeletonTracking (userGenerator, users[i]))
                continue;

            // Do not draw, collar, wrist, fingertip, waist and ankle: not available with NITE
            drawJoint (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_HEAD);
            drawJoint (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_NECK);
            drawJoint (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_TORSO);
            drawJoint (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_LEFT_SHOULDER);
            drawJoint (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_LEFT_ELBOW);
            drawJoint (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_LEFT_HAND);
            drawJoint (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_RIGHT_SHOULDER);
            drawJoint (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_RIGHT_ELBOW);
            drawJoint (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_RIGHT_HAND);
            drawJoint (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_LEFT_HIP);
            drawJoint (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_LEFT_KNEE);
            drawJoint (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_LEFT_FOOT);
            drawJoint (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_RIGHT_HIP);
            drawJoint (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_RIGHT_KNEE);
            drawJoint (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_RIGHT_FOOT);

            linkJoints (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_HEAD, XN_SKEL_NECK);
            linkJoints (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_NECK, XN_SKEL_TORSO);
            linkJoints (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_NECK, XN_SKEL_LEFT_SHOULDER);
            linkJoints (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_NECK, XN_SKEL_RIGHT_SHOULDER);
            linkJoints (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_LEFT_SHOULDER, XN_SKEL_LEFT_ELBOW);
            linkJoints (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_LEFT_ELBOW, XN_SKEL_LEFT_HAND);
            linkJoints (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_RIGHT_SHOULDER, XN_SKEL_RIGHT_ELBOW);
            linkJoints (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_RIGHT_ELBOW, XN_SKEL_RIGHT_HAND);
            linkJoints (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_TORSO, XN_SKEL_LEFT_HIP);
            linkJoints (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_TORSO, XN_SKEL_RIGHT_HIP);
            linkJoints (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_LEFT_HIP, XN_SKEL_LEFT_KNEE);
            linkJoints (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_LEFT_KNEE, XN_SKEL_LEFT_FOOT);
            linkJoints (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_RIGHT_HIP, XN_SKEL_RIGHT_KNEE);
            linkJoints (depthGenerator, userGenerator, imageMap, users[i], XN_SKEL_RIGHT_KNEE, XN_SKEL_RIGHT_FOOT);

            drawJoint (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_HEAD);
            drawJoint (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_NECK);
            drawJoint (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_TORSO);
            drawJoint (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_LEFT_SHOULDER);
            drawJoint (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_LEFT_ELBOW);
            drawJoint (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_LEFT_HAND);
            drawJoint (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_RIGHT_SHOULDER);
            drawJoint (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_RIGHT_ELBOW);
            drawJoint (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_RIGHT_HAND);
            drawJoint (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_LEFT_HIP);
            drawJoint (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_LEFT_KNEE);
            drawJoint (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_LEFT_FOOT);
            drawJoint (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_RIGHT_HIP);
            drawJoint (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_RIGHT_KNEE);
            drawJoint (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_RIGHT_FOOT);

            linkJoints (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_HEAD, XN_SKEL_NECK);
            linkJoints (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_NECK, XN_SKEL_TORSO);
            linkJoints (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_NECK, XN_SKEL_LEFT_SHOULDER);
            linkJoints (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_NECK, XN_SKEL_RIGHT_SHOULDER);
            linkJoints (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_LEFT_SHOULDER, XN_SKEL_LEFT_ELBOW);
            linkJoints (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_LEFT_ELBOW, XN_SKEL_LEFT_HAND);
            linkJoints (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_RIGHT_SHOULDER, XN_SKEL_RIGHT_ELBOW);
            linkJoints (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_RIGHT_ELBOW, XN_SKEL_RIGHT_HAND);
            linkJoints (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_TORSO, XN_SKEL_LEFT_HIP);
            linkJoints (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_TORSO, XN_SKEL_RIGHT_HIP);
            linkJoints (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_LEFT_HIP, XN_SKEL_LEFT_KNEE);
            linkJoints (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_LEFT_KNEE, XN_SKEL_LEFT_FOOT);
            linkJoints (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_RIGHT_HIP, XN_SKEL_RIGHT_KNEE);
            linkJoints (depthGenerator, userGenerator, colorDepthMap, users[i], XN_SKEL_RIGHT_KNEE, XN_SKEL_RIGHT_FOOT);
        }


        //displayDepthImage (DEPTH_WINDOW, depthMap, maxDepth);
        displayRGBImage (DEPTH_WINDOW, colorDepthMap);
        displayRGBImage (COLOR_WINDOW, imageMap);

        char c = cvWaitKey (10 /*ms */);

        if (c == 'q')
            die = 1;
    }

    cvReleaseImage (&imageMap);
    cvReleaseImage (&depthMap);
    cvReleaseImage (&colorDepthMap);

    cvDestroyAllWindows ();

    xnStopGeneratingAll (context);
    xnContextRelease (context);

    exit (EXIT_SUCCESS);
}

