// Fill out your copyright notice in the Description page of Project Settings.


#include "AzureKinectManager.h"

DEFINE_LOG_CATEGORY(AzureKinectLog);

k4a_device_t UAzureKinectManager::KinectDevice = NULL;
k4abt_tracker_t UAzureKinectManager::BodyTracker = NULL;

UAzureKinectManager::UAzureKinectManager()
{
}

UAzureKinectManager::~UAzureKinectManager()
{
	if (BodyTracker)
	{
		k4abt_tracker_shutdown(BodyTracker);
		k4abt_tracker_destroy(BodyTracker);
	}

	if (KinectDevice)
	{
		k4a_device_stop_cameras(KinectDevice);
		k4a_device_close(KinectDevice);
	}
}

void UAzureKinectManager::InitDevice(uint32 DeviceId, k4a_depth_mode_t DepthMode)
{
	// Open the Azure Kinect Device
	k4a_result_t deviceOpenResult = k4a_device_open(DeviceId, &KinectDevice);
	if (deviceOpenResult != K4A_RESULT_SUCCEEDED)
	{
		UE_LOG(AzureKinectLog, Error, TEXT("Open Kinect device (id : %d) Failed!"), DeviceId);
		return;
	}

	// Start the Camera and make sure the Depth Camera is Enabled
	k4a_device_configuration_t deviceConfig = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
	deviceConfig.depth_mode = DepthMode;
	deviceConfig.color_resolution = K4A_COLOR_RESOLUTION_OFF;

	k4a_result_t deviceStartCameraResult = k4a_device_start_cameras(KinectDevice, &deviceConfig);
	if (deviceStartCameraResult != K4A_RESULT_SUCCEEDED)
	{
		UE_LOG(AzureKinectLog, Error, TEXT("Kinect device (id : %d) start camera Failed!"), DeviceId);
		return;
	}

	k4a_calibration_t sensorCalibration;
	k4a_result_t deviceCalibrationResult = k4a_device_get_calibration(KinectDevice, deviceConfig.depth_mode, deviceConfig.color_resolution, &sensorCalibration);
	if (deviceCalibrationResult != K4A_RESULT_SUCCEEDED)
	{
		UE_LOG(AzureKinectLog, Error, TEXT("Kinect device (id : %d) get depth camera calibration Failed!"), DeviceId);
		return;
	}

	//k4abt_tracker_t tracker = NULL;
	k4abt_tracker_configuration_t trackerConfig = K4ABT_TRACKER_CONFIG_DEFAULT;
	k4a_result_t trackerCreateResult = k4abt_tracker_create(&sensorCalibration, trackerConfig, &BodyTracker);
	if (trackerCreateResult != K4A_RESULT_SUCCEEDED)
	{
		UE_LOG(AzureKinectLog, Error, TEXT("Kinect device (id : %d) - Body Tracker creation Failed!"), DeviceId);
		return;
	}
}

void UAzureKinectManager::CaptureBodyTrackingFrame(k4a_device_t Device, int32 TimeOutInMilliSecs)
{
	if (!Device)
	{
		UE_LOG(AzureKinectLog, Error, TEXT("Kinect device for capturing body tracking frame is Invalid!"));
		return;
	}

	// Capture a depth frame
	k4a_capture_t sensorCapture = NULL;
	k4a_wait_result_t getCaptureResult = k4a_device_get_capture(Device, &sensorCapture, TimeOutInMilliSecs);
	if (getCaptureResult != K4A_WAIT_RESULT_SUCCEEDED)
	{
		UE_LOG(AzureKinectLog, Error, TEXT("Kinect device get capture %s!"), 
			(getCaptureResult == K4A_WAIT_RESULT_FAILED ? TEXT("Failed") : TEXT("Timed Out")));
		return;
	}

	// Enqueue the capture
	k4a_wait_result_t queueCaptureResult = k4abt_tracker_enqueue_capture(BodyTracker, sensorCapture, TimeOutInMilliSecs);
	// Release the sensor capture
	k4a_capture_release(sensorCapture);
	if (queueCaptureResult != K4A_WAIT_RESULT_SUCCEEDED)
	{
		UE_LOG(AzureKinectLog, Error, TEXT("Adding capture to the Tracker process queue %s!"),
			(getCaptureResult == K4A_WAIT_RESULT_FAILED ? TEXT("Failed") : TEXT("Timed Out")));
		return;
	}

	k4abt_frame_t bodyFrame = NULL;
	k4a_wait_result_t popFrameResult = k4abt_tracker_pop_result(BodyTracker, &bodyFrame, TimeOutInMilliSecs);
	if (popFrameResult != K4A_WAIT_RESULT_SUCCEEDED)
	{
		UE_LOG(AzureKinectLog, Error, TEXT("Tracker pop body frame result %s!"),
			(getCaptureResult == K4A_WAIT_RESULT_FAILED ? TEXT("Failed") : TEXT("Timed Out")));
		return;
	}

	// Successfully popped the body tracking result

	UE_LOG(AzureKinectLog, Warning, TEXT("%zu bodies are detected"), k4abt_frame_get_num_bodies(bodyFrame));

	// Release the body frame
	k4abt_frame_release(bodyFrame);
}

//k4a_device_t UAzureKinectManager::GetDevice(uint32 DeviceId)
//{
//	return Device;
//}
