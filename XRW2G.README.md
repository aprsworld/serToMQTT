# XRW2G

The XRW2G  has two modes.   The first is that it automatically reports every second and the second is that reports when polled.
You must know how the device is setup.   To me able to control the timing we have the pollat which allows you to
control where in the second the device will report.   See `pollat` below.

XRW2G has three counter channels and 8 analog channels.   So --special-handling can specify formuals to be applied to the
raw data so that it is more easily used by the end user.  Here is an example.   An anamoeter spins and the faster it it spnning
is an indication of wind speed.  Each manufacture many count full or partial revolutions and so then number or counts per unit time must be addjusted for each manufacturer.  For a great number of anamometers this is a linear function using the equation y = mx +b.
So if we could preload m and b then we could output directly the wind speed.   Here is how it is done:

`xrw2g_pulseCountAnemometer(0, "Ref", 0.5, 1.0, "Reference Anemometer", "m/s")` is added to special handling.  Please note that no assignments are being made, so do not use `=`.   Also do not cast values.   The program does not understand and it breakes the parsing
of the function.  

## xrw2g_pulseCountAnemometer

`xrw2g_pulseCountAnemometer(<pulseChannelNumber>,<channelName>,<M>,<B>,<Title><Units>)`

### pulseChannelNumber

There are three counters, so the values 0,1,2 are allowwed.

### channelName

This will be the json element name and so must follow json rules.   No specail changacters, or punctuation.

### M

This is the coefficient of x and can be either an integer or floating point number.   This cannot be variable or a named literal.   1, 2, or 17.456 are okay examples.



### B

This is the constant and can be either an integer or floating point number.   This cannot be variable or a named literal.   1, 2, or 17.456 are okay examples.


### Title

This is just a label store int eh data or metadata indicating the type of data.

### Units

This adds the dimentionality of the result.


## pollat

`pollat=750`  means that serToMQTT will poll the xrw2g 750 mseconds after the top of the second.   The range for pollat is 0-999.   When the mseconds is equal to or greater the 750 and less than 750 + 50 then the xrw2g will be polled.   In experimentation it
was found the pollat=750 almost always allows the entry to showup in the mqttLocalLoggerCSV in every second.   (It never failed but
no promises or guarantte is made.).

