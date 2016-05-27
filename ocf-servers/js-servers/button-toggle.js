// This is a variation of the button.js OIC server
// implementation. It behaves differently in that
// pressing the button will toggle the value between
// 'true' and 'false' (instead of being 'true' when
// the button is pressed and 'false' otherwise.

var device = require('iotivity-node')('server'),
    _ = require('lodash'),
    buttonResource,
    sensorPin,
    notifyObserversTimeoutId,
    resourceTypeName = 'oic.r.button',
    resourceInterfaceName = '/a/button',
    hasUpdate = false,
    noObservers = false,
    sensorState = false,
    prevState = false;

// Require the MRAA library
var mraa = '';
try {
    mraa = require('mraa');
}
catch (e) {
    console.log('No mraa module: ' + e.message);
}

// Setup Button pin.
function setupHardware() {
    if (!mraa)
        return;

    sensorPin = new mraa.Gpio(4);
    sensorPin.dir(mraa.DIR_IN);
}

// This function construct the payload and returns when
// the GET request received from the client.
function getProperties() {

    if (mraa) {
        var buttonState = (sensorPin.read() == 1) ? true : false;

        // We care only when the button state is different.
        if (buttonState != prevState) {
            prevState = buttonState;

            if (buttonState == true && sensorState == false) {
                // Set the state ON, if the button is pressed and toggled off.
                sensorState = true;
                hasUpdate = true;
            } else if (buttonState == true && sensorState == true) {
                // Set the state OFF, if the button is pressed and toggled on.
                sensorState = false;
                hasUpdate = true;
            }
        }
    } else {
        // Simulate real sensor behavior. This is
        // useful for testing on desktop without mraa.
        sensorState = !sensorState;
        hasUpdate = true;
    }

    // Format the payload.
    var properties = {
        rt: resourceTypeName,
        id: 'button',
        value: sensorState
    };

    return properties;
}

// Set up the notification loop
function notifyObservers() {
    properties = getProperties();

    if (hasUpdate) {
        buttonResource.properties = properties;
        hasUpdate = false;

        console.log('\nbutton: Send the response: ' + sensorState);
        device.notify(buttonResource).catch(
            function(error) {
                console.log('button: Failed to notify observers.');
                noObservers = error.noObservers;
                if (noObservers) {
                    if (notifyObserversTimeoutId) {
                        clearTimeout(notifyObserversTimeoutId);
                        notifyObserversTimeoutId = null;
                    }
                }
            });
    }

    // After all our clients are complete, we don't care about any
    // more requests to notify.
    if (!noObservers) {
        notifyObserversTimeoutId = setTimeout(notifyObservers, 200);
    }
}

// Event handlers for the registered resource.
function observeHandler(request) {
    buttonResource.properties = getProperties();
    request.sendResponse(buttonResource).catch(handleError);

    noObservers = false;
    hasUpdate = true;

    if (!notifyObserversTimeoutId)
        setTimeout(notifyObservers, 200);
}

function retrieveHandler(request) {
    buttonResource.properties = getProperties();
    request.sendResponse(buttonResource).catch(handleError);
}

device.device = _.extend(device.device, {
    name: 'Smart Home Button Toggle Sensor'
});

function handleError(error) {
    console.log('button: Failed to send response with error ' + error +
    ' and result ' + error.result);
}

device.platform = _.extend(device.platform, {
    manufacturerName: 'Intel',
    manufactureDate: new Date('Fri Oct 30 10:04:17 EEST 2015'),
    platformVersion: '1.1.0',
    firmwareVersion: '0.0.1',
});

// Enable presence
device.enablePresence().then(
    function() {
        // Setup Button pin.
        setupHardware();

        console.log('\nCreate button resource.');

        // Register Button resource
        device.register({
            id: { path: resourceInterfaceName },
            resourceTypes: [ resourceTypeName ],
            interfaces: [ 'oic.if.baseline' ],
            discoverable: true,
            observable: true,
            properties: getProperties()
        }).then(
            function(resource) {
                console.log('button: register() resource successful');
                buttonResource = resource;

                // Add event handlers for each supported request type
                device.addEventListener('observerequest', observeHandler);
                device.addEventListener('retrieverequest', retrieveHandler);
            },
            function(error) {
                console.log('button: register() resource failed with: ' +
                    error);
            });
    },
    function(error) {
        console.log('button: device.enablePresence() failed with: ' + error);
    });

// Cleanup on SIGINT
process.on('SIGINT', function() {
    console.log('Delete Button Resource.');

    // Remove event listeners
    device.removeEventListener('observerequest', observeHandler);
    device.removeEventListener('retrieverequest', retrieveHandler);

    // Unregister resource.
    device.unregister(buttonResource).then(
        function() {
            console.log('button: unregister() resource successful');
        },
        function(error) {
            console.log('button: unregister() resource failed with: ' +
                error + ' and result ' + error.result);
        });

    // Disable presence
    device.disablePresence().then(
        function() {
            console.log('button: device.disablePresence() successful');
        },
        function(error) {
            console.log('button: device.disablePresence() failed with: ' + error);
        });

    // Exit
    process.exit(0);
});

