#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

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


void moveMouse(int x, int y, float z, float confidence) {
    char commande[50] = {0};

    // Mise à l'echelle
    x = -x*4;
    y = -y*4 + 600;
    //printf("[!] Custom position: %d\t%d\t%f\t(%f)\n", x, y, z, confidence);

    // Conversion
    char x_char[32];
    char y_char[32];
    sprintf(x_char, "%d", x);
    sprintf(y_char, "%d", y);
    
    // Running commande
    strcpy(commande, "xte 'mousemove ");
    strcat(commande, x_char);
    strcat(commande, " ");
    strcat(commande, y_char);
    strcat(commande, "'");
    //printf("commande: %s\n", commande);
    system( commande );
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

int drawJoint (XnNodeHandle depthGenerator, XnNodeHandle userGenerator,
                IplImage *img, XnUserID user, XnSkeletonJoint joint, int mouse_flag)
{
    if (!xnIsSkeletonTracking (userGenerator, user))
        return 1;

    XnSkeletonJointPosition jointPosition;
    xnGetSkeletonJointPosition (userGenerator, user, joint, &jointPosition);

    if (jointPosition.fConfidence == 0.0)
    {
        //printf ("There is no confidence in the position of joint %d\n", joint);
        return 1;
    }

    // Convert the real world coordinate of the joint to projective
    XnPoint3D projectivePosition;
    xnConvertRealWorldToProjective (depthGenerator, 1, &jointPosition.position,
                                    &projectivePosition);

    // Mouse handling
    if ( mouse_flag )
        moveMouse(jointPosition.position.X, jointPosition.position.Y, jointPosition.position.Z, jointPosition.fConfidence);

    //printf ("Drawing joint %d at (%f, %f, %f) with confidence %f\n", joint, projectivePosition.X, projectivePosition.Y, projectivePosition.Z, jointPosition.fConfidence);
    CvPoint p = cvPoint (projectivePosition.X, projectivePosition.Y);
    cvCircle (img, p, 20, cvScalar (255, 0, 0, 0), 0, 8, 0);

    return jointPosition.position.X;
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
