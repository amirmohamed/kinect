// written by Heresy @ http://kheresy.wordpress.com/
// version 0.1 @ 2012/02/24

// modify from: https://github.com/manctl/openni/blob/manctl/Samples/Kinect/kinect-motors.cpp
// reference: http://openkinect.org/wiki/Protocol_Documentation#Control_Packet_Structure

#include <XnUSB.h>
#include <XnTypes.h>

/**
 * A simple class to control the motor and LED of Kinect
 */
class KinectControl
{
public:
	/**
	 * The status of LED
	 */
	enum LED_STATUS
	{
		LED_OFF					= 0,
		LED_GREEN				= 1,
		LED_RED					= 2,
		LED_YELLOW				= 3,
		LED_BLINK_YELLOW		= 4,
		LED_BLINK_GREEN			= 5,
		LED_BLINK_RED_YELLOW	= 6
	};

	/**
	 * The Status of motor
	 */
	enum MOTOR_STATUS
	{
		MOTOR_STOPPED	= 0x00,
		MOTOR_LIMIT		= 0x01,
		MOTOR_MOVING	= 0x04,
		MOTOR_UNKNOWN	= 0x08
	};

public:
	/**
	 * constructor
	 */
	KinectControl()
	{
		m_bOpened = false;
	}

	/**
	 * destructor
	 */
	~KinectControl()
	{
		Release();
	}

	/**
	 * use the first Kinect device only
	 */
	XnStatus Create()
	{
		const XnUSBConnectionString *paths;
		XnUInt32 count;
		XnStatus res;

		// Init OpenNI USB
		res = xnUSBInit();
		if( res != XN_STATUS_OK && res != 131142 /** USB alreay initialized **/ )
			return res;

		// list all "Kinect motor" USB devices
		res = xnUSBEnumerateDevices( 0x045E, 0x02B0, &paths, &count );
		if( res != XN_STATUS_OK )
			return res;

		const XnChar* pUSBtoUse = paths[0];

		if( count > 0 )
		{
			res = xnUSBOpenDeviceByPath( pUSBtoUse, &m_xDevice );
			if( res != XN_STATUS_OK )
				return res;

			// Init motors
			XnUChar buf;
			res = xnUSBSendControl( m_xDevice, (XnUSBControlType) 0xc0, 0x10, 0x00, 0x00, &buf, sizeof(buf), 0 );
			if( res != XN_STATUS_OK )
			{
				Release();
				return res;
			}

			res = xnUSBSendControl( m_xDevice, XN_USB_CONTROL_TYPE_VENDOR, 0x06, 0x01, 0x00, NULL, 0, 0);
			if( res != XN_STATUS_OK )
			{
				Release();
				return res;
			}

			m_bOpened = true;
			return XN_STATUS_OK;
		}
        else
            printf("No Available usb found\n");
		
		return XN_STATUS_OS_FILE_OPEN_FAILED;
	}

	/**
	 * Close the connection to kinect
	 */
	void Release()
	{
		if( m_bOpened )
		{
			SetLED( LED_BLINK_GREEN );
			xnUSBCloseDevice( m_xDevice );
			m_bOpened = false;
		}
	}

	/**
	 * set LED 
	 */
	XnStatus SetLED( LED_STATUS eStatus )
	{
		if( !m_bOpened )
			return XN_STATUS_OS_EVENT_OPEN_FAILED;

		XnStatus res = xnUSBSendControl( m_xDevice, XN_USB_CONTROL_TYPE_VENDOR, 0x06, eStatus, 0x00, NULL, 0, 0 );
		if( res != XN_STATUS_OK )
			return res;
		return XN_STATUS_OK;
	}

	/**
	 * Set tilting angle
	 */
	XnStatus Move( int angle )
	{
		if( !m_bOpened )
			return XN_STATUS_OS_EVENT_OPEN_FAILED;

		XnStatus res = xnUSBSendControl( m_xDevice, XN_USB_CONTROL_TYPE_VENDOR, 0x31, 2 * angle, 0x00, NULL, 0, 0 );
		if( res != XN_STATUS_OK )
				return res;
		return XN_STATUS_OK;
	}

	/**
	 * Get tilting angle
	 */
	int GetAngle() const
	{
		int iA;
		MOTOR_STATUS eStatus;
		XnVector3D vVec;
		GetInformation( iA, eStatus, vVec );
		return iA;
	}
	
	/**
	 * Get motor status
	 */
	MOTOR_STATUS GetMotorStatus() const
	{
		int iA;
		MOTOR_STATUS eStatus;
		XnVector3D vVec;
		GetInformation( iA, eStatus, vVec );
		return eStatus;
	}

	/**
	 * Get the vector from the accelerometer
	 */
	XnVector3D GetAccelerometer() const
	{
		int iA;
		MOTOR_STATUS eStatus;
		XnVector3D vVec;
		GetInformation( iA, eStatus, vVec );
		return vVec;
	}

	/**
	 * Get all information
	 */
	XnStatus GetInformation( int& rAngle, MOTOR_STATUS& rMotorStatus, XnVector3D& rVec ) const
	{
		XnUChar aData[10];
		XnUInt32 uSize;
		XnStatus res = xnUSBReceiveControl( m_xDevice, XN_USB_CONTROL_TYPE_VENDOR, 0x32, 0x00, 0x00, aData, 10, &uSize, 0 );
		if( res == XN_STATUS_OK )
		{
			rAngle = aData[8];
			if( rAngle > 128 )
				rAngle = -0.5 * ( 255 - rAngle );
			else
				rAngle /= 2;

			if( aData[9] == 0x00 )
				rMotorStatus = MOTOR_STOPPED;
			else if( aData[9] == 0x01 )
				rMotorStatus = MOTOR_LIMIT;
			else if( aData[9] == 0x04 )
				rMotorStatus = MOTOR_MOVING;
			else
				rMotorStatus = MOTOR_UNKNOWN;

			rVec.X = (float)( ((XnUInt16)aData[2] << 8) | aData[3] );
			rVec.Y = (float)( ((XnUInt16)aData[4] << 8) | aData[5] );
			rVec.Z = (float)( ((XnUInt16)aData[6] << 8) | aData[7] );
		}
		return res;
	}

private:
	bool				m_bOpened;
	XN_USB_DEV_HANDLE	m_xDevice;
};
