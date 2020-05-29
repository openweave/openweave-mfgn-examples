# Device Association in Local Network

The OpenWeave Lock and Open/Close Sensor example applications leverage Weave’s 
simple device association protocol to collaborate together without requiring 
access to the Google Weave service. Once an association is established between 
the two, they can then collaborate to prevent the lock’s bolt from being extended 
while the sensor’s state is open. 

This document describes how this “Device Association” is implemented in the 
“lock” and “ocsensor” example applications.

Note that this mechanism would not be used in a production environment where 
devices are registered within Googles’ ecosystem since nothing is secure. 
This still represents an interesting scenario showcasing association of two 
Weave devices in a local network environment.
[FIXME: Jay has devised a way in which this can be made secure if we desire.]

NOTE: Support for Device Association between the lock and ocsensor example applications
is not yet complete.

Note that the multicast message will not work because of Issue
[Multicast ipv6 addresses for sleepy end devices not properly supported #598](https://github.com/openweave/openweave-core/issues/598).

### Weave artifacts supporting device association

##### DeviceDescription

At the basis of device assocation, there is the [DeviceDescription profile](https://github.com/openweave/openweave-core/blob/48d41755077abf98871bea413dd25bf09d3fdb09/src/lib/profiles/device-description/DeviceDescription.h). 
This is a simple protocol that is used to query devices and get specific 
information from them.

As documented in header file [DeviceDescription.h](https://github.com/openweave/openweave-core/blob/48d41755077abf98871bea413dd25bf09d3fdb09/src/lib/profiles/device-description/DeviceDescription.h):
> The [DeviceDescription profile](https://github.com/openweave/openweave-core/blob/48d41755077abf98871bea413dd25bf09d3fdb09/src/lib/profiles/device-description/DeviceDescription.h) is used to query device specific characteristics 
> of Weave nodes via a client-server interface.  This information is communicated
> via IdentifyRequest and [IdentifyResponse](https://github.com/openweave/openweave-core/blob/689175e97b8fbbb35caf1eac5fb8975d9378940f/src/lib/support/WeaveNames.cpp#L285) message types, the former used to
> discover devices matching a filter, and the latter used to respond with a
> payload detailing some or all of the characteristics specific to that device.
> Such characteristics include the device vendor and make / model, as well 
>as network information including MAC addresses and connections.

In this client-server model, the lock is the Client and the o/c sensor is the 
Server: the lock sends an IdentifyRequest to discover the o/c sensor, and 
the o/c sensor replies with information that the lock can then use to establish 
this association between the two devices.

##### UserSelectedMode

It is important to note that the lock does not want to associate with just any o/c sensor. 
It only wants to associate with the o/c sensor that is installed on its door. 
To ensure a proper rendez-vous between the lock and the o/c sensor, we make use of “[UserSelectedMode](https://github.com/openweave/openweave-core/blob/48d41755077abf98871bea413dd25bf09d3fdb09/src/device-manager/java/src/nl/Weave/DeviceManager/TargetDeviceModes.java#L31)”. 
User selected mode is typically initiated by a button press, or other direct interaction by a user 
on the device to be placed in that mode.  By means of parameters in the IdentifyRequest, it is then 
possible to request that only devices of a particular type (o/c sensor) that are also in 
"user selected mode" respond to the query.

### O/C Sensor example app modifications

##### Set up UserSelectedMode

When a specific button on the developer kit for the ocsensor device is pressed, it places the 
ocsensor device in “UserSelectedMode”. It is then in a state where it can successfully reply to 
IdentifyRequest’s.

```
#define USER_SELECTED_MODE_TIMEOUT_MS 60000
...
void UserSelectedModeButtonHandler()
{
    ConnectivityMgr().SetUserSelectedModeTimeout(USER_SELECTED_MODE_TIMEOUT_MS);
    ConnectivityMgr().SetUserSelectedMode(true);
}
```

##### Set up as a DeviceDescription Server

As mentioned in section DeviceDescription, ocsensor assumes the server-side role in the device 
association established via DeviceDescription. 

Code of interest:
- [DeviceDescriptionServer.h](https://github.com/openweave/openweave-core/blob/master/src/adaptations/device-layer/include/Weave/DeviceLayer/internal/DeviceDescriptionServer.h)
- [DeviceDescriptionServer.cpp](https://github.com/openweave/openweave-core/blob/master/src/adaptations/device-layer/DeviceDescriptionServer.cpp)

However, note that ocsensor does not need to implement any code to support its role of DeviceDescription Server since this is a standard profile of Weave.
The call to ConnectivityMgr().SetUserSelectedModeTimeout() does trigger a call to DeviceDescriptionServer. 

##### Disabling UserSelectedMode

It is important to disable UserSelectedMode with the ocsensor at some point in time to 
prevent it from being associated with more than one lock.

For example, in a scenario with two locks and two sensors:
1. lock1 triggers an association request
1. sensor1 is placed is “association-ready” mode (UserSelectedMode)
1. sensor1 replies to lock1’s association request
1. lock1 and sensor1 are associated
1. lock2 triggers an association request
1. sensor1 is still in ”association-ready” mode and replies to lock2’s association request
1. lock2 and sensor1 are associated. Wrong!

To help mitigate the issue of sensor1 being associated with two locks (as is the case with 
steps 6 and 7), the following is put in place:
- When sensor1 is placed in “association-ready” mode (step 2), there is a timeout that will cancel that mode when it goes off (e.g. timeout value of 1 minute).
- When sensor1 replies to an association request (step 3), the timeout value is immediately reduced to the lowest  of X seconds (e.g. 5 seconds) and the timer’s residual value to help reduce the likelihood that step 6 will occur. This way, there is still a short period of a few seconds that allows to account for communication failures and retries, but this is a shorter period than what might have been left on the residual timeout period, lowering the likelihood that sensor1 is still in “association-ready” mode when step 5 occurs.

FIXME: To be implemented.

### Lock sample app modifications

##### Set up as a DeviceDescription Client

As mentioned in section DeviceDescription, the lock assumes the client-side role in the device 
discovery established via DeviceDescription. 

```asm
// Setup the DeviceDescription client.
WeaveLogProgress(Support, "Initializing DeviceDescriptionClient");
ret = mDeviceDescriptionClient.Init(&ExchangeMgr);
SuccessOrAbort(ret, "DeviceDescriptionClient.Init() failed.");
mDeviceDescriptionClient.OnIdentifyResponseReceived = OnIdentifyResponseReceivedHandler;
```

```asm
void LockDeviceController::OnIdentifyResponseReceivedHandler(void *appState, uint64_t nodeId, const IPAddress& nodeAddr, const IdentifyResponseMessage& respMsg)
{
    WeaveLogProgress(Support, "OnIdentifyResponseReceivedHandler");
    LockDeviceController & _this = GetLockDeviceController();
    WeaveDeviceDescriptor deviceDesc = respMsg.DeviceDesc;
    char ipAddrStr[64];
    nodeAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
    WeaveLogDetail(Support, "IdentifyResponse received from node %" PRIX64 " (%s)\n", nodeId, ipAddrStr);
    WeaveLogDetail(Support, "  Source Fabric Id: %016" PRIX64 "\n", deviceDesc.FabricId);
    WeaveLogDetail(Support, "  Source Vendor Id: %04X\n", (unsigned)deviceDesc.VendorId);
    WeaveLogDetail(Support, "  Source Product Id: %04X\n", (unsigned)deviceDesc.ProductId);
    WeaveLogDetail(Support, "  Source Product Revision: %04X\n", (unsigned)deviceDesc.ProductRevision);
    _this.mDeviceDescriptionClient.CancelExchange();
}
```

##### Trigger an association request

When a specific button on the developer kit for the lock device is pressed, it triggers the lock 
to send an “IdentifyRequest” to find an O/C sensor that is in “association-ready” mode.

```asm
void LockDeviceController::SendIdentifyRequestButtonHandler()
{
    WeaveLogProgress(Support, "LockDeviceController::SendIdentifyRequestButtonHandler()");

    LockDeviceController & _this = GetLockDeviceController();
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    IdentifyRequestMessage identifyReqMsg;
    nl::Inet::IPAddress ip_addr;

    if (!ConfigurationMgr().IsMemberOfFabric())
    {
        WeaveLogError(Support, "DeviceDiscovery err: Device not fabric provisioned");
        return;
    }
    uint16_t vendorId;
    ConfigurationMgr().GetVendorId(vendorId);
    uint16_t productId;
    ConfigurationMgr().GetProductId(productId);

    ip_addr = nl::Inet::IPAddress::MakeIPv6WellKnownMulticast(nl::Inet::kIPv6MulticastScope_Link,
                                                              nl::Inet::kIPV6MulticastGroup_AllNodes);
    identifyReqMsg.TargetFabricId   = ::nl::Weave::DeviceLayer::FabricState.FabricId;
    identifyReqMsg.TargetModes      = kTargetDeviceMode_UserSelectedMode;
    identifyReqMsg.TargetVendorId   = vendorId;
    identifyReqMsg.TargetProductId  = productId;
    identifyReqMsg.TargetDeviceId   = nl::Weave::kAnyNodeId;

    WeaveLogProgress(Support, "Sending the Identify request");
    err = _this.mDeviceDescriptionClient.SendIdentifyRequest(ip_addr, identifyReqMsg);
    if (err != WEAVE_NO_ERROR) {
        WeaveLogError(Support, "SendIdentifyRequest failed: [%d]", err);
        return;
    }
}
```

See [IANAConstants.h](https://github.com/openweave/openweave-core/blob/48d41755077abf98871bea413dd25bf09d3fdb09/src/inet/IANAConstants.h).

##### Disabling Device Association requests

It is important to disable device association requests once an association has been successfully 
established or automatically after a certain period of time if no response was received.

FIXME: TBD
