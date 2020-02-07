#pragma once

#include <OpenPoseWrapper/OpenPoseEvent.h>
#include <Utils/Tracking.h>

class TrackingOpenPoseEvent : public OpenPoseEvent
{
public:
	op::PeopleList people{
		5,         // NUMBER_NODES_TO_TRUST
		0.5f,   // CONFIDENCE_THRESHOLD
		10,      // NUMBER_FRAMES_TO_LOST
		50.0f  // DISTANCE_THRESHOLD
	};

	TrackingOpenPoseEvent() {}
	virtual ~TrackingOpenPoseEvent() {};
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final { return 0; }
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		people.addFrame(imageInfo);
		return 0;
	}
};