# OpenWeave MFGN Examples

Example applications for MFGN partners showing the use of OpenWeave for a variety of
device types and hardware platforms.

[OpenWeave](https://github.com/openweave/openweave-core/) is the open source release of
Nest's Weave technology, an application layer framework that provides a secure,
reliable communications backbone for Nest's products.

<hr>

<a name="intro"></a>

## Introduction

The OpenWeave example applications repo currently features two device types (lock and
open/close sensor) on two hardware platforms (Nordic nRF52840 (nrf5) and Silicon Labs
EFR32 MG12/MG21 (efr32)).

The example applications provide working demonstrations of connected devices built using OpenWeave,
OpenThread, and the SDKs of various hardware plaforms.
They are intended to serve both as a means to explore the workings of OpenWeave and OpenThread,
as well as a template for creating real products. It is important to note that these
example applications are not "production-ready", but efforts are made in the code to bring
attention to areas that developers must pay attention to for production readiness of their firmware.

The example applications build upon the [OpenWeave](https://github.com/openweave/openweave-core/)
and [OpenThread](https://github.com/openthread/openthread)
projects, which are incorporated as submodules and built from source.
A top-level Makefile orchestrates the entire build process, including building OpenWeave,
OpenThread and files from the SDK of the selected hardware platform.
The resultant image file can be flashed directly onto the dev kit of the hardware platform.

<a name="software-architecture"></a>

## Software Architecture

![Examples Architecture](doc/images/openweave-architecture.svg)

[FIXME: Get a diagram that is application-agnostic]

The examples are built on the Weave application layer framework provided by
[openweave-core](https://github.com/openweave/openweave-core/).
At the heart of this are the **Weave Core** components.  These components provide the essential
functionality required to speak Weave.
This includes code for encoding and decoding Weave messages, communicating Weave messages over
various transports (TCP, UDP, BLE), tracking Weave conversations (exchanges) and negotiating
secure communications.

The **Weave Profiles** sit atop the core components and provide support for specific types of
Weave interactions.  Central among these is the Weave Data Management profile (**WDM**),
which provides a generalized protocol for communicating state, configuration settings, commands
and events between Weave nodes.  Other profiles support device provisioning (pairing),
OTA software update, time synchronization, and device identification and control.

The **Weave Device Layer** serves to adapt the portable Weave Core and Profile components
to run in the context of a particular device platform. For the current examples, device platforms
supported are Nordic's nRF52840 and Silicon Labs EFR32 MG12/MG21.
Additionally, the Device Layer also provides platform-neutral
services (APIs) to the application for performing certain fundamental operations that are common
to all Weave devices.  These include managing a device’s persistent configuration,
managing its network connectivity, scheduling and orchestrating OTA software updates and others.

The examples make use of various components provided by the hardware platform SDKs,
including BLE support libraries, persistent storage management, crypto services, logging and others.
The platform's adaptation of **FreeRTOS** is used to support multi-threading and task
synchronization.

**OpenThread** provides the core Thread stack implementation and manages persistent storage of
Thread network configuration.  The **LwIP** Lightweight IP
stack provides IP services on top of Thread, including TCP, UDP and ICMPv6.

<a name="directory-structure"></a>

## Directory Structure

##### src/

All source code is contained in the src/ directory.

Each example application (device-type) has its own directory within the `examples/` directory. It contains
application-specific code with the following organization:

<pre>
   src/examples/
       [app-1]/
           include/
           platforms/
               [platform-1]/
                 ldscripts/
               [platform-2]/
               ...
           schema/
           traits/
           DeviceController.cpp
           main.cpp
           README.md
           WDMFeature.cpp
           ...
       [app-2]
           ...
</pre>

Directory `common/` contains all code that can be shared among applications.

<pre>
   src/common/
       include/
       platforms/
           [platform-1]/
               include/
               HardwarePlatform.cpp
               README.md
               ...
           [platform-2]/
               ...
           ...
       AltPrintf.c
       AppSoftwareUpdateManager.cpp
       AppTask.cpp
       Button.cpp
       ConnectivityState.cpp
       CXXExceptionStubs.cpp
       FreeRTOSNewlibLockSupport.c
       LED.cpp
       ...
</pre>

##### third_party/

Third-party dependencies are incorporated as submodules and built from source.

<pre>
    third_party/
        openthread/
        openweave-core/
        printf/
</pre>

<a name="application-components"></a>

## Application Components

Here is a high level overview of the processing flow for the example applications.

![Application Flow](doc/images/application-flow.svg)

#### Core Classes

<b>src/examples/[device-type]/main.cpp</b>

`main.cpp` performs all the initializations for the application: 
1) Delegates to HardwarePlatform for all platform-specific initializations 
2) Performs initializations that are platform-independent
3) Calls DeviceController for all application-specific initializations. 

It then calls AppTask to setup and start the FreeRTOS application task. 
And finally, it starts the FreeRTOS scheduler.

<b>
src/common/include/HardwarePlatform.h<br>
src/common/platforms/[platform]/HardwarePlatform.cpp<br>
</b><br>

`HardwarePlatform.h` defines the interface that encapsulates all platform-specific behavior. 
Its platform-specific implementation initializes all the LEDs and Buttons exposed by the devkit, and handles the proper dispatching of all button events triggered on that devkit.

<b>src/common/include/AppTask.h<br>
src/common/AppTask.cpp<br></b>

`AppTask` sets up and starts the FreeRTOS task that runs the application’s code. AppTask is initialized with a pointer to a function in the DeviceController that is called at each cycle of its event loop.

<b>src/examples/[device-type]/include/DeviceController.h<br>
src/examples/[device-type]/DeviceController.cpp<br></b>

`DeviceController` is the object that controls the overall behavior of the device.
The application task collaborates with the DeviceController by calling method 
EventLoopCycle() on every cycle of its event loop. 
This allows the DeviceController to do periodic tasks such as animating the LEDS, 
updating the state of a button long press, etc.

<b>src/examples/[device-type]/include/WDMFeature.h<br>
src/examples/[device-type]/WDMFeature.cpp<br></b>

`WDMFeature` encapsulates everything that is related to Weave Data Model (WDM).  
[FIXME: Will need help from Jay here. I think we need a document just to cover what happens 
in that class.]

#### Support Classes with platform dependencies

<b>
src/common/include/LED.h<br>
src/common/include/PlatformLED.h<br>
src/common/LED.cpp<br>
src/common/platforms/[platform]/[platform]LED.cpp<br>
</b><br>

The LED class encapsulates the generic behavior of a LED. Note that all LED objects are 
initialized in a platform-specific way in HardwarePlatform, and are accessible via 
method GetLEDs(). Platform-specific LED behavior is defined in interface PlatformLED. 
Each platform implements a subclass of PlatformLED (e.g. Nrf5LED). LED is initialized 
with an instance of PlatformLED to which it delegates all processing that is 
platform-specific.

[FIXME: I should be able to get rid of `PlatformLED` by simply having virtual functions in LED.h. However, previous attempt broke 
things (could not pair). Anyway, file an issue to fix that once everything is stable.]

<b>
src/common/include/Button.h<br>
src/common/Button.cpp<br>
</b><br>

The `Button` class encapsulates the generic behavior of a Button. All Button 
objects are initialized in a platform-specific way in HardwarePlatform, and are accessible 
via method GetButtons(). HardwarePlatform handles the proper dispatching of all button 
events triggered on the platform devkit, which is eventually handled by the event handler 
associated with the Button.

#### Support Classes without platform dependencies.

<b>
src/common/include/ConnectivityState.h<br>
src/common/ConnectivityState.cpp<br>
</b><br>

`ConnectivityState` monitors the provisioning/connectivity state of the device.
It is initialized with a specific LED, and is called at every event loop cycle by the DeviceController to check on the current connectivity state. If the state changes, then it updates accordingly the lighting pattern of its associated LED.

<b>
src/common/include/AppSoftwareUpdateManager.h<br>
src/common/AppSoftwareUpdateManager.cpp<br>
</b><br>

The `AppSoftwareUpdateManager` class encapsulates behavior associated with Software Updates.
Note that software updates are currently only partially supported.

#### WDM Schema

<b>
src/examples/[device-type]/schema<br>
</b><br>

The schema directory contains all the helper files for the traits defined in the device’s 
resource definition. These helper files are generated via WDLC. Note that they are 
currently provided and not generated.  
[FIXME: Need to prune our example applications so they only use publicly available traits, 
allowing us to: 1) include the resource definition in the source code, 2) generate the 
schema files via WDLC.]

#### Device Traits — Sinks and Sources

<b>src/examples/[device-type]/traits</b><br><br>
The traits directory hosts all the “source” and “sink” classes for the device traits.  
[FIXME: See FIXME for WDMFeature. This doc on WDM should include information about sinks 
and sources as well.]

NOTE: The DeviceController is the source of truth for the state of the device.
It relays any state change to the appropriate TraitDataSource instance which is accessible 
via WDMFeature.get[trait].
Since the service proxies the device state, the service requests the information and 
eventually is sync’ed with the device.  
[FIXME: verify that and document that in the source code.]

#### Configuration
<b>
src/common/include/OpenThreadConfig.h<br>
</b>

Overrides to default OpenThread configuration.  
[FIXME: Split into common and app-specific.]

<b>src/common/include/WeaveProjectConfig.h</b><br>
Overrides the default OpenWeave configuration.  
[FIXME: Split into common and app-specific.]

<b>src/common/platforms/[platform]/include/[filename].h</b><br>

Overrides the default platform configuration.  
For nrf5: src/common/platforms/nrf5/include/app_config.h

<b>src/examples/[device-type]/platforms/[platform]/ldscripts/[filename].ld</b><br>

[FIXME: Provide links to docs that describe the content of that file]

## Hardware Platforms Documentation

- [Nordic nrf5](common/nrf5/README.md)
- [Silicon Labs efr32](common/nrf5/README.md)

## Applications Documentation

- [Lock](examples/lock/README.md)
- [Open/Close Sensor](examples/ocsensor/README.md)
