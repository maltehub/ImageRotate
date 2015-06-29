PHONE_IP = 10.41.101.206

all:
	pebble build

run: 
	pebble install --phone ${PHONE_IP} && pebble logs --phone ${PHONE_IP}

clean:
	pebble clean