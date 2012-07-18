#include <XnOpenNI.h>

int main() {
    XnStatus nRetVal = XN_STATUS_OK;

    xn::Context context;

    // Initialize context object
    nRetVal = context.Init();
    // TODO: check error code

    // Create a DepthGenerator node
    xn::DepthGenerator depth;
    nRetVal = depth.Create(context);
    // TODO: check error code

    // Make it start generating data
    nRetVal = context.StartGeneratingAll();
    // TODO: check error code

    // Main loop
    while (bShouldRun)
    {
        // Wait for new data to be available
        nRetVal = context.WaitOneUpdateAll(depth);
        if (nRetVal != XN_STATUS_OK)
        {
            printf("Failed updating data: %s\n", xnGetStatusString(nRetVal));
            continue;
        }

        // Take current depth map
        const XnDepthPixel* pDepthMap = depth.GetDepthMap();

        // TODO: process depth map
    }

    // Clean-up
    context.Shutdown();
}
