pilight = require('./build/Release/pilightjs')

class Pilightjs extends require('events').EventEmitter

  prioToLevel:
    0: 'error'
    1: 'error'
    2: 'error'
    3: 'error'
    4: 'warn'
    5: 'info'
    6: 'info'
    7: 'debug'

  start: (settings, devices, hardware) ->
    devices.name = 'Devices'
    config = {
      "log-level": 4, # ignored by c module
      "port": settings['port'],
      "standalone": 1, # should allways be 1
      "pid-file": "/var/run/pilight.pid", # ignored by c module
      "config-file": JSON.stringify({devices}),
      "hardware-file": JSON.stringify(hardware),
      "log-file": "/var/log/pilight.log", # ignored by c module
      "send-repeats": settings['send-repeats'],
      "receive-repeats": settings['receive-repeats'],
      "whitelist": "",
      "firmware-update": settings['firmware-update']
    }


    pilight.setLogCallback( (prio, line) => @emit("log", {level: @prioToLevel[prio], message: line}) )
    pilight.setBroadcastCallback( (message) => @emit("broadcast", JSON.parse(message)) )
    pilight.startDaemon(JSON.stringify(config))

  sendMessage: (message) ->
    pilight.sendMessage(JSON.stringify(message))


module.exports.Pilightjs = Pilightjs