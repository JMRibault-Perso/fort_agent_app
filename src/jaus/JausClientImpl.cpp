#include <spdlog/spdlog.h>

#include <fort_agent/fort_agent.h>
#include <fort_agent/jaus/JausClientImpl.h>
#include <fort_agent/jaus/JausBridgeSingleton.h>


using namespace openjaus;
using namespace openjaus::core_v1_1::accesscontrol;
using namespace openjaus::core_v1_1::management;
using namespace openjaus::mobility_v1_1::globalposesensor; // Pull in all GlobalPoseSensor messages
using namespace openjaus::mobility_v1_1::primitivedriver;  // Pull in all PrimitiveDriver messages
using namespace openjaus::core_v1_1::informclass::fields::reportstatus;
using openjaus::mobility_v1_1::GlobalPoseSensor;
using openjaus::mobility_v1_1::PrimitiveDriver;

const std::string JAUS_CLIENT_VERSION = "1.0.0";

// Static member initialization
bool JAUSClientImpl::controlGranted = false;
bool JAUSClientImpl::requestPending = false;


reportstatusrec::Status JAUSClientImpl::currentStatus = reportstatusrec::Status::INITIALIZE;

// Verbose functions..

JAUSClientImpl::JAUSClientImpl()
    : reportGlobalPoseSubscribed(false),
      reportGlobalPoseSubscriptionId(0),
      reportWrenchEffortSubscribed(false),
      reportWrenchEffortSubscriptionId(0),
      component("FORTClientComponent_v1_1"){
      }
      
void JAUSClientImpl::initializeJAUS() {
      
    // Constructor implementation
    spdlog::info("FORT JAUS client version {}", JAUS_CLIENT_VERSION);

    // Instantiate the Client Component that will be run as part of this application
    // In this example, we will use the exposed methods on the core_v1_1::Base component
    // interact with the remote server. Depending on the complexity of the interactions
    // with the server, it may be better to wrap the Base component inside your own class.

    // Add the callbacks that will execute when specific messages are received.
    component.addMessageCallback(&JAUSClientImpl::handleIncomingReportControl);
    component.addMessageCallback(&JAUSClientImpl::handleIncomingReportStatus);
    component.addMessageCallback(&JAUSClientImpl::handleIncomingReportGlobalPose);
    component.addMessageCallback(&JAUSClientImpl::handleIncomingReportGeomagneticProperty);
    component.addMessageCallback(&JAUSClientImpl::handleIncomingReportWrenchEffort);

    try
    {
        // Run the Components which starts the sending/receiving/processing of messages
        component.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    

}

bool JAUSClientImpl::discoverVehicle() {
    std::vector<transport::Address> pdList = component.getSystemRegistry()->lookupService(PrimitiveDriver::uri());

    if (pdList.size() == 0)  {
        std::cout << "No components found with the PrimitiveDriver service" << std::endl;
        return false;
    }
    else
    {
        std::cout << "Components with the PrimitiveDriver service (" << pdList.size() << "):" << std::endl;
        for (size_t i = 0; i < pdList.size(); i++)
        {
            std::cout << "\t" << (int)i << ": " << pdList.at(i) << std::endl;
        }
        std::cout << std::endl;

        serverAddress = pdList[0];
        std::cout << "Auto-selecting Component [0]: " << serverAddress << std::endl;
        std::cout << std::endl;
    }
    return true; // Placeholder
}

bool JAUSClientImpl::sendRequestControl() {
    if (!serverAddress.isValid()) {
        return false;
    }
    if (isRequestPending()) {
        spdlog::info("RequestControl already pending, not sending another request");
        return true;
    }

    // The Base component exposes methods which do some basic AccessControl message management.
    // The requestControl method sends a RequestControl message with a SAFETY_CRITICAL priority.
    // When a response is received (ConfirmControl or RejectControl) the provided callback will
    // be executed.
    component.requestControl(serverAddress, 128, handleRequestControlResponse);

    JAUSClientImpl::requestPending = true;
    return true;
}

bool JAUSClientImpl::isRequestPending() const {
    return JAUSClientImpl::requestPending;
}   

bool JAUSClientImpl::hasControl() const {
    // Implementation to check if control is granted, 
    return JAUSClientImpl::controlGranted;
}

bool JAUSClientImpl::sendRequestResume() {
    if (!serverAddress.isValid()) {
        return false;
    }

    Resume* message = new Resume();
    message->setDestination(serverAddress);
    component.sendMessage(message);

    return true;
}

bool JAUSClientImpl::queryStatus() {
    if (!serverAddress.isValid()) {
        return false;
    }
    // previous status request is pending 
    if (isRequestPending()) {
        // we must wait for response
        return false;
    }

    QueryStatus* message = new QueryStatus();
    message->setDestination(serverAddress);
    component.sendMessage(message);
    JAUSClientImpl::requestPending = true;

    return true;
}

bool JAUSClientImpl::hasReadyState() const {
    // Implementation of ready state check
    return currentStatus == reportstatusrec::Status::READY; // Placeholder
}
bool JAUSClientImpl::isHeartbeatAlive() const {
    // Implementation of heartbeat check
    return true; // Placeholder
}

void JAUSClientImpl::sendWrenchEffort(const frc_joystick_data_t& data) {
    
    // Implementation of sending wrench effort
    std::ostringstream frame;

    auto* message = new SetWrenchEffort();
    message->setDestination(serverAddress);

    // In this example, we will only set the propulsive linear effort X and propulsive rotational effort Z

    auto& wrenchEffortRec = message->getWrenchEffortRec();

    // Enable the propulsive linear effort X field and specify the value
    wrenchEffortRec.setPropulsiveLinearEffortX_percent(normalizeJoysticValue(data.leftXAxis.data));
    wrenchEffortRec.setPropulsiveLinearEffortY_percent(normalizeJoysticValue(data.leftYAxis.data));
    wrenchEffortRec.setPropulsiveLinearEffortZ_percent(normalizeJoysticValue(data.leftZAxis.data));

    wrenchEffortRec.setPropulsiveRotationalEffortX_percent(normalizeJoysticValue(data.rightXAxis.data));
    wrenchEffortRec.setPropulsiveRotationalEffortY_percent(normalizeJoysticValue(data.rightYAxis.data));
    wrenchEffortRec.setPropulsiveRotationalEffortZ_percent(normalizeJoysticValue(data.rightZAxis.data));

    wrenchEffortRec.disableResistiveLinearEffortX();
    wrenchEffortRec.disableResistiveLinearEffortY();
    wrenchEffortRec.disableResistiveLinearEffortZ();

    wrenchEffortRec.disableResistiveRotationalEffortX();
    wrenchEffortRec.disableResistiveRotationalEffortY();
    wrenchEffortRec.disableResistiveRotationalEffortZ();

    component.sendMessage(message);
}


void JAUSClientImpl::handleRequestControlResponse(const model::ControlResponse& response)
{
    std::ostringstream frame;

    frame << std::endl;

    frame << "----------------------------------------------------" << std::endl;
    frame << " Received RequestControl Response" << std::endl;
    frame << "----------------------------------------------------" << std::endl;
    frame << " Remote Server: " << response.getAddress() << std::endl;
    frame << " Response: " << response.getResponseType() << std::endl;
    frame << "----------------------------------------------------" << std::endl;

    // Final flush
    printToConsole.flushString(frame.str());

    // Set controlGranted based on response
    // We use a static value to support this in a static method
    if (response.getResponseType() == model::ControlResponseType::CONTROL_ACCEPTED) {
        JAUSClientImpl::controlGranted = true;
    } else {
        JAUSClientImpl::controlGranted = false; 
    }
    JAUSClientImpl::requestPending = false;
    JausBridgeSingleton::instance().postJAUSResponse();
}

void JAUSClientImpl::handleReleaseControlResponse(const model::ControlResponse& response)
{
    std::ostringstream frame;

    frame << std::endl;

    frame << "----------------------------------------------------" << std::endl;
    frame << " Received ReleaseControl Response" << std::endl;
    frame << "----------------------------------------------------" << std::endl;
    frame << " Remote Server: " << response.getAddress() << std::endl;
    frame << " Response: " << response.getResponseType() << std::endl;
    frame << "----------------------------------------------------" << std::endl;

    // Final flush
    printToConsole.flushString(frame.str());

    // Not sure what to do on release control response and not CONTROL_RELEASED received
    if (response.getResponseType() == model::ControlResponseType::CONTROL_RELEASED) {
        JAUSClientImpl::controlGranted = false;
    } 
    JAUSClientImpl::requestPending = false;
    JausBridgeSingleton::instance().postJAUSResponse();
}

bool JAUSClientImpl::handleIncomingReportControl(ReportControl& incoming)
{
    auto& controlRec = incoming.getReportControlRec();

    std::ostringstream frame;

    frame << std::endl;

    frame << "----------------------------------------------------" << std::endl;
    frame << " Report Control" << std::endl;
    frame << "----------------------------------------------------" << std::endl;
    frame << " SubystemID: " << (uint32)controlRec.getSubsystemID() << std::endl;
    frame << " NodeID: " << (uint32)controlRec.getNodeID() << std::endl;
    frame << " ComponentID: " << (uint32)controlRec.getComponentID() << std::endl;
    frame << " AuthorityCode: " << (uint32)controlRec.getAuthorityCode() << std::endl;
    frame << "----------------------------------------------------" << std::endl;

    // Final flush
    printToConsole.flushString(frame.str());

    return true;
}

bool JAUSClientImpl::handleIncomingReportStatus(ReportStatus& incoming)
{
    const auto& reportRec = incoming.getReportStatusRec();

    std::ostringstream frame;

    frame << std::endl;

    frame << "----------------------------------------------------" << std::endl;
    frame << " Report Status" << std::endl;
    frame << "----------------------------------------------------" << std::endl;
    frame << " Status: " << reportRec.getStatus().toString() << std::endl;
    frame << "----------------------------------------------------" << std::endl;

    // Final flush
    printToConsole.flushString(frame.str());

    currentStatus = reportRec.getStatus();

    JAUSClientImpl::requestPending = false;
    JausBridgeSingleton::instance().postJAUSResponse();
    return true;
}

bool JAUSClientImpl::handleIncomingReportGlobalPose(ReportGlobalPose& message)
{
    auto& globalPoseRec = message.getGlobalPoseRec();

    std::ostringstream frame;

    frame << std::endl;

    frame << "----------------------------------------------------" << std::endl;
    frame << " Report Global Pose" << std::endl;
    frame << "----------------------------------------------------" << std::endl;
    frame << " Latitude: " << toString(globalPoseRec.getLatitude_deg(), globalPoseRec.isLatitudeEnabled()) << std::endl;
    frame << " Longitude: " << toString(globalPoseRec.getLongitude_deg(), globalPoseRec.isLongitudeEnabled()) << std::endl;
    frame << " Altitude: " << toString(globalPoseRec.getAltitude_m(), globalPoseRec.isAltitudeEnabled()) << std::endl;
    frame << " Roll: " << toString(globalPoseRec.getRoll_rad(), globalPoseRec.isRollEnabled()) << std::endl;
    frame << " Pitch: " << toString(globalPoseRec.getPitch_rad(), globalPoseRec.isPitchEnabled()) << std::endl;
    frame << " Yaw: " << toString(globalPoseRec.getYaw_rad(), globalPoseRec.isYawEnabled()) << std::endl;
    frame << "----------------------------------------------------" << std::endl;

    // Final flush
    printToConsole.flushString(frame.str());

    return true;
}

bool JAUSClientImpl::handleIncomingReportGeomagneticProperty(ReportGeomagneticProperty& message)
{
    auto& geomagneticRec = message.getGeomagneticPropertyRec();

    std::ostringstream frame;

    frame << std::endl;

    frame << "----------------------------------------------------" << std::endl;
    frame << " Report Geomagnetic Property" << std::endl;
    frame << "----------------------------------------------------" << std::endl;
    frame << " Magnetic Variation: " << geomagneticRec.getMagneticVariation_rad() << std::endl;
    frame << "----------------------------------------------------" << std::endl;

    // Final flush
    printToConsole.flushString(frame.str());

    return true;
}

void JAUSClientImpl::handleEventRequestResponse(const model::EventRequestResponseArgs& response)
{
    std::ostringstream frame;

    frame << std::endl;

    frame << "---------------------------" << std::endl;
    frame << " Received Event Request Response from: " << response.getSourceAddress() << std::endl;
    frame << " Connection type: " << response.getConnectionType() << std::endl;
    frame << " Message Requested: 0x" << std::hex << response.getQueryId() << std::endl;
    frame << " Response: " << response.getResponseType() << std::endl;
    frame << " Error String: " << response.getErrorString() << std::endl;
    frame << "---------------------------" << std::endl;

    // Final flush
    printToConsole.flushString(frame.str());
}

bool JAUSClientImpl::handleIncomingReportWrenchEffort(ReportWrenchEffort& message)
{
    auto& wrenchEffortRec = message.getWrenchEffortRec();

    std::ostringstream frame;

    frame << std::endl;

    frame << "----------------------------------------------------" << std::endl;
    frame << " Report Wrench Effort" << std::endl;
    frame << "----------------------------------------------------" << std::endl;
    frame << " Propulive Effort" << std::endl;
    frame << "   Linear X: " << toString(wrenchEffortRec.getPropulsiveLinearEffortX_percent(), wrenchEffortRec.isPropulsiveLinearEffortXEnabled()) << std::endl;
    frame << "   Linear Y: " << toString(wrenchEffortRec.getPropulsiveLinearEffortY_percent(), wrenchEffortRec.isPropulsiveLinearEffortYEnabled()) << std::endl;
    frame << "   Linear Z: " << toString(wrenchEffortRec.getPropulsiveLinearEffortZ_percent(), wrenchEffortRec.isPropulsiveLinearEffortZEnabled()) << std::endl;
    frame << std::endl;
    frame << "   Rotational X: " << toString(wrenchEffortRec.getPropulsiveRotationalEffortX_percent(), wrenchEffortRec.isPropulsiveRotationalEffortXEnabled()) << std::endl;
    frame << "   Rotational Y: " << toString(wrenchEffortRec.getPropulsiveRotationalEffortY_percent(), wrenchEffortRec.isPropulsiveRotationalEffortYEnabled()) << std::endl;
    frame << "   Rotational Z: " << toString(wrenchEffortRec.getPropulsiveRotationalEffortZ_percent(), wrenchEffortRec.isPropulsiveRotationalEffortZEnabled()) << std::endl;
    frame << std::endl;
    frame << std::endl;
    frame << " Resistive Effort" << std::endl;
    frame << "   Linear X: " << toString(wrenchEffortRec.getResistiveLinearEffortX_percent(), wrenchEffortRec.isResistiveLinearEffortXEnabled()) << std::endl;
    frame << "   Linear Y: " << toString(wrenchEffortRec.getResistiveLinearEffortY_percent(), wrenchEffortRec.isResistiveLinearEffortYEnabled()) << std::endl;
    frame << "   Linear Z: " << toString(wrenchEffortRec.getResistiveLinearEffortZ_percent(), wrenchEffortRec.isResistiveLinearEffortZEnabled()) << std::endl;
    frame << std::endl;
    frame << "   Rotational X: " << toString(wrenchEffortRec.getResistiveRotationalEffortX_percent(), wrenchEffortRec.isResistiveRotationalEffortXEnabled()) << std::endl;
    frame << "   Rotational Y: " << toString(wrenchEffortRec.getResistiveRotationalEffortY_percent(), wrenchEffortRec.isResistiveRotationalEffortYEnabled()) << std::endl;
    frame << "   Rotational Z: " << toString(wrenchEffortRec.getResistiveRotationalEffortZ_percent(), wrenchEffortRec.isResistiveRotationalEffortZEnabled()) << std::endl;
    frame << "----------------------------------------------------" << std::endl;

    // Final flush
    printToConsole.flushString(frame.str());

    return true;
}

// TODO: Move to utility file if needed elsewhere
double JAUSClientImpl::normalizeJoysticValue(int x) {
    constexpr int min_raw = -2047;
    constexpr int max_raw = 2047;
    constexpr double min_norm = -100.0;
    constexpr double max_norm = 100.0;

    double scale = (max_norm - min_norm) / (max_raw - min_raw);
    double midpoint_raw = (max_raw + min_raw) / 2.0;
    double midpoint_norm = (max_norm + min_norm) / 2.0;

    x = std::max(x, min_raw);
    x = std::min(x, max_raw);
    return midpoint_norm + (x - midpoint_raw) * scale;
}


std::string JAUSClientImpl::toString(double value, bool enabled) {
    std::string output;

    if (enabled) {
        std::stringstream ss;
        ss << value;
        output = ss.str();
    } else {
        output = "N/A";
    }

    return output;
}

std::string JAUSClientImpl::getComponentName() {
    bool success = false;
    auto Info = component.getSystemRegistry()->getComponent(serverAddress, success);
    if (success)
        std::cout << Info.toString();

    return Info.getName();
}