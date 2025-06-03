#** *****************************************************************************
# *
# * If not stated otherwise in this file or this component's LICENSE file the
# * following copyright and licenses apply:
# *
# * Copyright 2024 RDK Management
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *
# http://www.apache.org/licenses/LICENSE-2.0
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *
#* ******************************************************************************

# Curl command for activating HdmiCecSink plugin
activate_command = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc":"2.0","id":"3"
,"method": "Controller.1.activate", "params":{"callsign":"org.rdk.PowerManager"}}' http://127.0.0.1:55555/jsonrpc'''

# Curl command for deactivating HdmiCecSink plugin
deactivate_command = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc":"2.0","id":"3"
,"method": "Controller.1.deactivate", "params":{"callsign":"org.rdk.PowerManager"}}' http://127.0.0.1:55555/jsonrpc'''

# Store the expected output response for activate & deactivate curl command
expected_output_response = '{"jsonrpc":"2.0","id":3,"result":null}'

######################################################################################

# PowerManager Methods :

#Returns the over-temperature grace interval value
getOvertempGraceInterval = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.PowerManager.getOvertempGraceInterval"}' http://127.0.0.1:55555/jsonrpc'''

#Returns the current power state of the device
getPowerState = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.PowerManager.getPowerState"}' http://127.0.0.1:55555/jsonrpc'''

#Returns temperature threshold values
getThermalState = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.PowerManager.getThermalState"}' http://127.0.0.1:55555/jsonrpc'''

#Returns temperature threshold values. Not supported on all devices.
getTemperatureThresholds = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.PowerManager.getTemperatureThresholds"}' http://127.0.0.1:55555/jsonrpc'''

#Sets the over-temperature grace interval value. Not supported on all devices.
setOvertempGraceInterval = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.PowerManager.setOvertempGraceInterval","params": {"graceInterval": 60}}' http://127.0.0.1:55555/jsonrpc'''

#Sets the power state of the device.
setPowerState = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.PowerManager.setPowerState","params": {"keyCode": 30,"powerState": "ON","standbyReason": "APIUnitTest"}}' http://127.0.0.1:55555/jsonrpc'''

#Sets the deep sleep timeout period.
setDeepSleepTimer = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.PowerManager.setDeepSleepTimer","params": {"timeOut": 3}}' http://127.0.0.1:55555/jsonrpc'''

#Returns the reason for the device coming out of deep sleep
getLastWakeupReason = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.PowerManager.getLastWakeupReason"}' http://127.0.0.1:55555/jsonrpc'''

#Returns the last wakeup keycode.
getLastWakeupKeyCode = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.PowerManager.getLastWakeupKeyCode"}' http://127.0.0.1:55555/jsonrpc'''

#Requests that the system performs a reboot of the set-top box.
reboot = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.PowerManager.reboot","params": {"rebootRequestor": "SystemServicesPlugin","rebootReasonCustom": "FIRMWARE_FAILURE","rebootReasonOther": "FIRMWARE_FAILURE"}}' http://127.0.0.1:55555/jsonrpc'''

#Returns the network standby mode of the device. If network standby is true, the device supports WakeOnLAN and WakeOnWLAN actions in STR mode.
getNetworkStandbyMode = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.PowerManager.getNetworkStandbyMode"}' http://127.0.0.1:55555/jsonrpc'''

#This API will be deprecated in the future. Please refer setWakeupSrcConfiguration to Migrate. This API Enables or disables the network standby mode of the device. If network standby is enabled, the device supports WakeOnLAN and WakeOnWLAN actions in STR mode.
setNetworkStandbyMode = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.PowerManager.setNetworkStandbyMode","params": {"standbyMode": false}}' http://127.0.0.1:55555/jsonrpc'''

#Sets the wakeup source configuration for the input powerState. if you are using setNetworkStandbyMode API, Please do not use this API to set LAN and WIFI wakeup. Please migrate to setWakeupSrcConfiguration API to control all wakeup source settings. This API does not persist. Please call this API on Every bootup to set the values.
setWakeupSrcConfig = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.PowerManager.setWakeupSrcConfig","params": {"powerState": 4,"wakeupSources": 6}}' http://127.0.0.1:55555/jsonrpc'''



#Sets the mode of the set-top box for a specific duration before returning to normal mode. Valid modes are:NORMAL - The set-top box is operating in normal mode.EAS - The set-top box is operating in Emergency Alert System (EAS) mode. This mode is set when the device needs to perform certain tasks when entering EAS mode, such as setting the clock display or preventing the user from using the diagnostics menu.WAREHOUSE - The set-top box is operating in warehouse mode.
setSystemMode = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.PowerManager.setSystemMode","params": {"currentMode": 2,"newMode": 1}}' http://127.0.0.1:55555/jsonrpc'''


#Returns the power state before reboot.
getPowerStateBeforeReboot = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.PowerManager.getPowerStateBeforeReboot"}' http://127.0.0.1:55555/jsonrpc'''

#Sets the temperature threshold values. Not supported on all devices
setTemperatureThresholds = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.PowerManager.setTemperatureThresholds","params": {"high": 100.0,"critical": 110.0}}' http://127.0.0.1:55555/jsonrpc'''
