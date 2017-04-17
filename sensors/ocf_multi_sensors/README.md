# Zephyr OCF Multi Sensor
This is the example of a multiple sensor device hosting multiple OCF resources on an Arduino 101 board, using Zephyr and iotivity-constrained. The device collects temperature, ambient light data, controls an LED light and a color light. It advertises itself and provides sensor data to a connected device using 6LowPAN over BLE.

## Sensor wiring
The following table shows how to connect the sensors to the [Grove base shield](http://wiki.seeed.cc/Base_Shield_V2/). Since the Arduino 101 works at 3.3V and the sensors support the voltage, don't forget to turn the switch on the Grove base shield to 3V3 position.

|       Sensor      |  Type  |  Port  |          Link            |
|:-----------------:|:------:|:------:|:------------------------:|
| Temperature       | Analog |   A0   |[Grove temperature sensor](http://wiki.seeed.cc/Grove-Temperature_Sensor_V1.2)|
| Light             | Analog |   A1   |[Grove light sensor](http://wiki.seeed.cc/Grove-Light_Sensor/)                |
| Motion            |  GPIO  |   D2   |[Grove motion sensor](http://wiki.seeed.cc/Grove-PIR_Motion_Sensor/)          |
| LED               |  GPIO  |   D3   |[Grove LED](https://www.seeedstudio.com/Grove-Red-LED-p-1142.html)            |
| Chainable RGB LED |  GPIO  |   D7   |[Grove Chainable RGB LED](http://wiki.seeed.cc/Grove-Chainable_RGB_LED/)      |

## Building and flashing the applications
In order to build the Zephyr app for the Arduino 101 board, first setup the Zephyr dev environment following this guide:
* Setup dev environment on Host following this [guide](https://www.zephyrproject.org/doc/getting_started/getting_started.html)
* Download Zephyr v1.7.0 source code and setup environmental variables
```
    $ git clone https://gerrit.zephyrproject.org/r/zephyr && cd zephyr
    # git checkout tags/v1.7.0
    $ source ./zephyr-env.sh
```
* Following this [instruction](https://www.zephyrproject.org/doc/boards/x86/arduino_101/doc/board.html#flashing-the-bluetooth-core) to build and flash the Zephyr bluetooth firmware to the board

**Note**: You can learn more information from this [wiki](https://www.zephyrproject.org/doc/boards/x86/arduino_101/doc/board.html)

* Download iotivity-constrained project source code and prepare the environment
```
    $ git clone https://github.com/iotivity/iotivity-constrained && cd iotivity-constrained
    $ git checkout 8128d3f
    $ git submodule update --init
    $ export IOTIVITY_CONSTRAINED_BASE=$PWD
```

In the app source folder sensors/ocf_multi_sensor, use the following commands to build and flash the app
to Arduino 101 board:

### ARC core application
```
    $ cd arc
    $ make pristine && make
    $ dfu-util -a sensor_core -D outdir/arduino_101_sss/zephyr.bin
```
### x86 core application
```
    $ cd ../x86
    $ make pristine && make
    $ dfu-util -a x86_app -D outdir/arduino_101/zephyr.bin
```
**Note**: By default, the name of the device is defined as "Zephyr OCF Sensors", but it is possible to change it by modifying the *Kconfig* *CONFIG_BLUETOOTH_DEVICE_NAME* in the prj.conf.
## How to setup a BLE connection
Use the following steps to connect the device to an Ubuntu host with BLE connectivity. User should be root
```
    # hciconfig # You should see the device hci0
    # rfkill unblock bluetooth
    # hciconfig hci0 up
    # modprobe bluetooth_6lowpan
    # echo 1 > /sys/kernel/debug/bluetooth/6lowpan_enable
    # bluetoothctl
    [bluetooth]# scan on
    [bluetooth]# scan off
    [bluetooth]# devices # You will see the "Zephyr OCF Sensors" with the address
    [bluetooth]# quit

    # echo "connect <Zephyr OCF Sensors Address> 2"  >/sys/kernel/debug/bluetooth/6lowpan_control
    (e.g. echo "connect E6:DF:CD:CE:65:14 2" >/sys/kernel/debug/bluetooth/6lowpan_control)

    # ifconfig  ##You should see bt0 interface, if everything was okay
```

### Run the tests on host browser:
The sensors and their properties can be addressed by by sending RESTful API commands directly to the IPv6 address of the device. However it would be easier and more intuitive by using tool like *IoT-rest-api-server*. Follow this [link](https://github.com/01org/iot-rest-api-server) to install the tool to your host computer

* Discovery: http://localhost:8000/api/oic/res
You should see many OCF resources.

* Get: http://localhost:8000/api/oic/a/temperature?di=\<device ID\>
You should see the temperature properties.

* Get: http://localhost:8000/api/oic/a/illuminance?di=\<device ID\>
You should see the ambient light properties.

* Get: http://localhost:8000/api/oic/a/motion?di=\<device ID\>
You should see the motion detection properties.
