

pilite = require('./build/Release/pilite')


class PilightDaemon extends require('events').EventEmitter

  prioToLevel:
    0: 'error'
    1: 'error'
    2: 'error'
    3: 'error'
    4: 'warn'
    5: 'info'
    6: 'info'
    7: 'debug'
  start: (config) ->
    pilite.setLogCallback( (prio, line) => @emit("log", {level: @prioToLevel[prio], message: line}) )
    pilite.setBroadcastCallback( (message) => @emit("broadcast", JSON.parse(message)) )
    pilite.startDaemon(JSON.stringify(config))

  sendMessage: (message) ->
    pilite.sendMessage(JSON.stringify(message))


module.exports.PilightDaemon = PilightDaemon

# config = {
#   "log-level": 4,
#   "port": 5000,
#   "standalone": 1,
#   "pid-file": "/var/run/pilight.pid",
#   "config-file": JSON.stringify({
#     "devices": {
#       "name": "Living",
#       "weather": {
#         "name": "Weather",
#         "protocol": [ "generic_weather" ],
#         "id": [{
#           "id": 100
#         }],
#         "temperature": 2300,
#         "humidity": 7600,
#         "battery": 0
#       },
#       "dimmer": {
#         "name": "Dimmer",
#         "protocol": [ "generic_dimmer" ],
#         "id": [{
#           "id": 100
#         }],
#         "state": "on",
#         "dimlevel": 0
#       },
#       "switch": {
#         "name": "Switch",
#         "protocol": [ "generic_switch" ],
#         "id": [{
#           "id": 100
#         }],
#         "state": "off"
#       },
#       "temperature": {
#         "name": "Temperature 2",
#         "protocol": [ "dht22" ],
#         "id": [{
#           "gpio": 4
#         }],
#         "humidity": 541,
#         "temperature": 197,
#         "poll-interval": 20
#       },
#       "rpitemp2": {
#         "name": "Raspberry Pi Temp. 2",
#         "protocol": [ "rpi_temp" ],
#         "id": [{
#           "id": 2
#         }],
#         "temperature": 42774
#       }
#     }
#   }),
#   "hardware-file": JSON.stringify({
#     "433gpio": {
#       "sender": 0,
#       "receiver": 1
#     }
#   }),
#   "log-file": "/var/log/pilight.log",
#   "send-repeats": 10,
#   "receive-repeats": 1,
#   "whitelist": "",
#   "firmware-update": 0
# }

# daemon = new PilightDaemon()

# daemon.on('log', (info) => console.log info )
# daemon.on('broadcast', (msg) => console.log msg )

# daemon.start(config)
