#pragma once

//OpenJAUS
#include <openjaus/Components/Base.h>
#include <openjaus/mobility_v1_1/GlobalPoseSensor.h>
#include <openjaus/mobility_v1_1/PrimitiveDriver.h>
#include <openjaus/system/Application.h>

#include <fort_agent/jaus/JausClient.h>

class JAUSClientImpl : public JAUSClient {
public:
    JAUSClientImpl();
    bool discoverVehicle() override;
    bool sendRequestControl() override;
    bool hasControl() const override;
    bool isHeartbeatAlive() const override;
    void sendWrenchEffort(const frc_joystick_data_t& data) override;
    void initializeJAUS() override;
    bool sendRequestResume() override;
    bool hasReadyState() const override;
    bool queryStatus() override;



    static void handleRequestControlResponse(const openjaus::model::ControlResponse& response);
    static void handleReleaseControlResponse(const openjaus::model::ControlResponse& response);
    static bool handleIncomingReportControl(openjaus::core_v1_1::informclass::ReportControl& incoming);
    static bool handleIncomingReportStatus(openjaus::core_v1_1::informclass::ReportStatus& incoming);
    static bool handleIncomingReportGlobalPose(openjaus::mobility_v1_1::globalposesensor::ReportGlobalPose& incoming);
    static bool handleIncomingReportGeomagneticProperty(openjaus::mobility_v1_1::globalposesensor::ReportGeomagneticProperty& incoming);
    static bool handleIncomingReportWrenchEffort(openjaus::mobility_v1_1::primitivedriver::ReportWrenchEffort& incoming);
    static void handleEventRequestResponse(const openjaus::model::EventRequestResponseArgs& response);
private:
    static std::string toString(double value, bool enabled);
    static double normalizeJoysticValue(int x);
    // Any additional initialization if needed
    openjaus::components::Base component;
    openjaus::transport::Address serverAddress;

    bool reportGlobalPoseSubscribed = false;
    uint32_t reportGlobalPoseSubscriptionId = 0;
    bool reportWrenchEffortSubscribed = false;
    uint32_t reportWrenchEffortSubscriptionId = 0;

    static bool controlGranted;
    static openjaus::core_v1_1::informclass::fields::reportstatus::reportstatusrec::Status currentStatus;

};