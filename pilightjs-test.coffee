Pilightjs = require('./pilightjs').Pilightjs

pilightjs = new Pilightjs()

pilightjs.on('log', (info) => console.log info )
pilightjs.on('broadcast', (msg) => console.log msg )

settings = {
    "port": 5000
    "firmware-update": 0
    "send-repeats": 10,
    "receive-repeats": 1,
}

devices = {
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

hardware = {
    "433gpio": {
      "sender": 0,
      "receiver": 1
    }
}

pilightjs.start(settings, devices, hardware)