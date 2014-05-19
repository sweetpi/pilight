pilightjs = require('./build/Release/pilightjs')
pilightjs.setLogCallback( (prio, line) => console.log "log", prio, line)
pilightjs.setBroadcastCallback( (message) => console.log "broadcast", message);
console.log "before start"
pilightjs.startDaemon(JSON.stringify({
  "log-level": 4,
  "port": 5000,
  "standalone": 1,
  "pid-file": "/var/run/pilight.pid",
  "config-file": JSON.stringify({
    "devices": {
      "name": "Living",
      "weather": {
        "name": "Weather",
        "protocol": [ "generic_weather" ],
        "id": [{
          "id": 100
        }],
        "temperature": 2300,
        "humidity": 7600,
        "battery": 0
      },
      "dimmer": {
        "name": "Dimmer",
        "protocol": [ "generic_dimmer" ],
        "id": [{
          "id": 100
        }],
        "state": "on",
        "dimlevel": 0
      },
      "switch": {
        "name": "Switch",
        "protocol": [ "generic_switch" ],
        "id": [{
          "id": 100
        }],
        "state": "off"
      },
      "temperature": {
        "name": "Temperature 2",
        "protocol": [ "dht22" ],
        "id": [{
          "gpio": 4
        }],
        "humidity": 541,
        "temperature": 197,
        "poll-interval": 20
      },
      "rpitemp2": {
        "name": "Raspberry Pi Temp. 2",
        "protocol": [ "rpi_temp" ],
        "id": [{
          "id": 2
        }],
        "temperature": 42774
      }
    }
  }),
  "hardware-file": JSON.stringify({
    "433gpio": {
      "sender": 0,
      "receiver": 1
    }
  }),
  "log-file": "/var/log/pilight.log",
  "send-repeats": 10,
  "receive-repeats": 1,
  "whitelist": "",
  "firmware-update": 0
}))
console.log "after start"

setTimeout( =>
  pilightjs.sendMessage(JSON.stringify(
    "message": "send",
    "code": {
      "protocol": ["generic_switch"],
      "id": 100,
      "off": 1
    }
  ))
, 5000);

