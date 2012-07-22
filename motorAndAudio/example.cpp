#include "KinectControl.h"
#include <iostream>
using namespace std;

int main( int argc, char** argv )
{
	KinectControl xKinectControl;
	if( xKinectControl.Create() == XN_STATUS_OK )
	{
		// set LED
		xKinectControl.SetLED( KinectControl::LED_BLINK_GREEN );

		// Move motor
		xKinectControl.Move( 20 );

		// get value
		int iA;
		KinectControl::MOTOR_STATUS eStatus;
		XnVector3D vVec;
		xKinectControl.GetInformation( iA, eStatus, vVec );

		cout << "Angle: " << iA << endl;
		cout << "Motor Status:" << eStatus <<  endl;
		cout << "Acc Vector: " << vVec.X << "/" << vVec.Y << "/" << vVec.Z << endl;
	}
    else
        printf("Error creating control\n");
	xKinectControl.Release();
}
