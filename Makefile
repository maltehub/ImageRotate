PHONE_IP = 192.168.1.51

all:
	pebble build

run: 
	pebble install --phone ${PHONE_IP} && pebble logs --phone ${PHONE_IP}

clean:
	pebble clean