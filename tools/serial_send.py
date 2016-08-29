import serial, sys

serial_port = sys.argv[1]



port = serial.Serial(serial_port, baudrate=9600, timeout=3.0)
data = bytearray.fromhex('020C000621A58E0D0A03')
#print data
port.write(data)
