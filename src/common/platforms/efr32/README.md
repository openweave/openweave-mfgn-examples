# EFR32 Hardware Platform

## Introduction

![EFR32 MG12 + WSTK](../../../../doc/images/efr32-mg12-wstk.jpg)

The
 [Silicon Labs efr32mg12](https://www.silabs.com/products/wireless/mesh-networking/efr32mg12-series-1-socs) is used for the development of all example applications built for the "efr32" hardware platform.

**SEGGER RTT** support
provides access to log output using the J-Jink debug probe built in to the EFR32 Wireless Starter Kit (**WSTK**) Kit.  

The **Silicon Labs EFR32 SDK** provides a BLE protocol stack and Dynamic Multiprotocol (**DMP**) support for simultaneous use of BLE and Thread.

<a name="building"></a>

## Building

* Download and install the [Silicon Labs Simplicity Studio and SDK for Thread and Zigbee version v2.7](https://www.silabs.com/products/development-tools/software/simplicity-studio)

Extract the SimplicityStudio-v4.tgz archive to where you want to install Simplicity Studio and follow the instructions in README.txt found within the extracted archive to complete installation.  The remaining instructions assume SimplicityStudio_v4 is installed in the user's home directory.

In Simplicity Studio from the Launcher perspective click on the "Update Software" button.  The Package Manager window will Open.  Ensure that the following SDKs are installed (as of January 2020).
  
  * Bluetooth 2.13.0.0
  * Flex 2.7.0.0

* Download and install a suitable ARM gcc tool chain: [GNU Arm Embedded Toolchain 7-2018-q2-update](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads/7-2018-q2-update)
(Direct download link: 
[Linux](https://developer.arm.com/-/media/Files/downloads/gnu-rm/7-2018q2/gcc-arm-none-eabi-7-2018-q2-update-linux.tar.bz2) 
[Mac OS X](https://developer.arm.com/-/media/Files/downloads/gnu-rm/7-2018q2/gcc-arm-none-eabi-7-2018-q2-update-mac.tar.bz2)) 

* Install some additional tools:

         # Linux
         $ sudo apt-get install git make automake libtool ccache
         
         # Mac OS X
         $ brew install automake libtool ccache

* Set the following environment variables based on the locations/versions of the packages installed above:

        export EFR32_SDK_ROOT=${HOME}/path/to/gecko_sdk_suite/

* Clone the example application into the app subdirectory of the v2.7 SDK

         $ cd ~/SimplicityStudio_v4/developer/sdks/gecko_sdk_suite/v2.7/app
         $ git clone https://github.com/openweave/openweave-mfgn-examples.git

         $ cd openweave-mfgn-examples
         $ git submodule update --init

* Supported hardware:

    MG12 boards:
    
    - BRD4161A / SLWSTK6000B / Wireless Starter Kit / 2.4GHz@19dBm
    - BRD4166A / SLTB004A / Thunderboard Sense 2 / 2.4GHz@10dBm
         
* Run make to build the application:

         $ make APP=[application] PLATFORM=efr32 BOARD=BRD4161A clean
         $ make


<a name="initializing"></a>

## Initializing the EFR32 module

The example application is designed to run on the Silicon Labs SDK development kit.  Prior to installing
the application, the device's flash memory should be erased.

* Connect the host machine to the J-Link Interface MCU USB connector on the EFR32 WSTK.

* Use the Makefile to erase the flash:

        $ make APP=[application] PLATFORM=efr32 BOARD=BRD4161A erase        

* To erase a specific device using its serial number:

        $ make APP=[application] PLATFORM=efr32 BOARD=BRD4161A SERIALNO=440113717 erase  


<a name="flashing"></a>

## Flashing the Application

* To rebuild the image and flash the example app:

        $ make APP=[application] PLATFORM=efr32 BOARD=BRD4161A flash

* To rebuild the image and flash a specific device using its serial number:

        $ make APP=[application] PLATFORM=efr32 BOARD=BRD4161A SERIALNO=440113717 flash 

* To flash an existing image without rebuilding:

        $ make APP=[application] PLATFORM=efr32 BOARD=BRD4161A flash-app


<a name="view-logging"></a>

## Viewing Logging Output

The example application is built to use the SEGGER Real Time Transfer (RTT) facility for log output.  RTT is a feature built-in to the J-Link Interface MCU on the WSTK development board. It allows bi-directional communication with an embedded application without the need for a dedicated UART.

Using the RTT facility requires downloading and installing the *SEGGER J-Link Software and Documentation Pack* ([web site](https://www.segger.com/downloads/jlink#J-LinkSoftwareAndDocumentationPack)).  Alternatively the *SEGGER Ozone - J-Link Debugger* can be used to view RTT logs.

* Download the J-Link installer by navigating to the appropriate URL and agreeing to the license agreement. 

 * [JLink_Linux_x86_64.deb](https://www.segger.com/downloads/jlink/JLink_Linux_x86_64.deb)
 * [JLink_MacOSX.pkg](https://www.segger.com/downloads/jlink/JLink_MacOSX.pkg)


* Install the J-Link software

        $ cd ~/Downloads
        $ sudo dpkg -i JLink_Linux_V*_x86_64.deb

* In Linux, grant the logged in user the ability to talk to the development hardware via the linux tty device (/dev/ttyACMx) by adding them to the dialout group.

        $ sudo usermod -a -G dialout ${USER} 

Once the above is complete, log output can be viewed using the JLinkExe tool in combination with JLinkRTTClient as follows:

* Run the JLinkExe tool with arguments to autoconnect to the WSTK board:

    For MG12 use:
        
        $ JLinkExe -device EFR32MG12PXXXF1024 -if JTAG -speed 4000 -autoconnect 1

* In a second terminal, run the JLinkRTTClient to view logs:

        $ JLinkRTTClient
        
                
<a name="debugging"></a>

## Debugging with SEGGER Ozone

The SEGGER Ozone J-Link Debugger is a full featured debugger and Real-Time-Telemetry (RTT) viewer that is available for Linux and macOS.

* Download and install SEGGER Ozone Debugger:

[SEGGER Ozone J-Link Debugger](https://www.segger.com/products/development-tools/ozone-j-link-debugger)

* Enable debugging with the DEBUG=1 switch in the make command, e.g.

         $ make APP=[application] PLATFORM=efr32 BOARD=BRD4161A DEBUG=1
         
* Start SEGGER Ozone Debugger:
  
         $ /opt/SEGGER/ozone/<version>/Ozone
         
* Open the New Project Wizard, either from the File->New menu or the Dialog Box that appears at startup.

  * Target Device
    Set Device to Cortex-M4 for MG12

  * Connection Settings.
    Select JTAG for MG12, 4Mhz Target Interface Speed
  
  * Program File
    Select ./build/openweave-efr32-[application]-example.out
    
* Select the Start Debugging->Attach to Running Program from the Debug menu. After a short delay the debugger will connect to the device.

* To view RTT Log output, ensure the Terminal pane is enabled from the View menu.

* Confirm RTT is working from the J-Link Control Panel.

  * Open the J-Link Control Panel pane from the View menu.
  * Click on the RTT tab button at the top of the J-Link Control Panel
  * The Status field shall show Located RTT Control Block
  
If the control block cannot be found open the Global Data pane from the View menu.

The top line of the Global Data pane is a search box. Search for \_SEGGER\_RTT. Copy its location into the Control Block Address text box in the J-Link Control Panel and click the Start button in the J-Link Control Panel.



