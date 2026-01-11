from open_ephys.control import OpenEphysHTTPServer

gui = OpenEphysHTTPServer()

print(gui.status())

protocol = """{
"id": 1,
"name": "PATTERN 1",
"patternType": 1,
"sampleRate": 30000,
"maxVoltage": 500.0,
"triggerChannel": 0,
"gateChannel": -1,
"analogOutputChannel": 0,
"pulse": {
  "onDuration": 10000,
  "offDuration": 100,
  "delayDuration": 0,
  "repeatNumber": 1,
  "rampOnDuration": 0,
  "rampOffDuration": 0,
  "maxVoltage": 5.0
},
"sine": {
  "frequency": 5,
  "cycles": 1,
  "delayDuration": 0,
  "maxVoltage": 5.0
},
"custom": {
  "string": "0,0,0"
}}"""

#protocol = "test_protocol.xml"

gui.config(110, protocol)

gui.message("enable_output")