//#include <highgui.h>
//#include <cv.h>

//#include <XnOpenNI.h>

//#include <XnModuleInterface.h>

#define DEPTH_WINDOW "Depth"

#define MM_IN_ONE_METER 1000.

#define CHK_NI_ERROR(status, msg) if (status != XN_STATUS_OK) \
{ \
    fprintf (stderr, "%s: %s\n", msg, xnGetStatusString(status)); \
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
    cvShowImage (name, normalized);
    cvReleaseImage (&normalized);
}

void displayDepthPlan (const char *name, const IplImage *image,
                        const uint16_t maxDepth, float pointDepth) 
{
    int depthColor = 0;
    IplImage *normalized = cvCreateImage (cvGetSize (image), IPL_DEPTH_8U, 1);

    // Normalized each pixel between 0 and 255
    for (int row = 0; row < image->height; row++)
    {
        for (int col = 0; col < image->width; col++)
        {
            depthColor = (255 * CV_IMAGE_ELEM (image, uint16_t, row, col)) / maxDepth;
            if ( (depthColor < pointDepth*1.3) && (depthColor > pointDepth*0.7) ) 
                CV_IMAGE_ELEM (normalized, uint8_t, row, col) = 255;
            else
                CV_IMAGE_ELEM (normalized, uint8_t, row, col) = 
                    (255 * CV_IMAGE_ELEM (image, uint16_t, row, col)) / maxDepth;
        }
    }
    cvShowImage (name, normalized);
    cvReleaseImage (&normalized);
}

