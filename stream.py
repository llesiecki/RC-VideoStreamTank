# Web streaming example
# Source code from the official PiCamera package
# http://picamera.readthedocs.io/en/latest/recipes2.html#web-streaming

from io import BytesIO
from picamera import PiCamera
from socketserver import ThreadingMixIn
from threading import Condition
from http import server
import RPi.GPIO as GPIO
import signal
from gpiozero import CPUTemperature
import _thread
import asyncio
import websockets
from time import sleep
from netifaces import ifaddresses
from netifaces import AF_INET

class StreamingOutput(object):
    def __init__(self):
        self.frame = None
        self.buffer = BytesIO()
        self.condition = Condition()
	#

    def write(self, buf):
        if buf.startswith(b'\xff\xd8'):
            # New frame, copy the existing buffer's content and notify all
            # clients it's available
            self.buffer.truncate()
            with self.condition:
                self.frame = self.buffer.getvalue()
                self.condition.notify_all()
            self.buffer.seek(0)
        return self.buffer.write(buf)
	#
#

class StreamingHandler(server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/':
            self.send_response(301)
            self.send_header('Location', '/index.html')
            self.end_headers()
        elif self.path == '/index.html':
            content = PAGE.encode('utf-8')
            self.send_response(200)
            self.send_header('Content-Type', 'text/html')
            self.send_header('Content-Length', len(content))
            self.end_headers()
            self.wfile.write(content)
        elif self.path == '/stream.mjpg':
            self.send_response(200)
            self.send_header('Age', 0)
            self.send_header('Cache-Control', 'no-cache, private')
            self.send_header('Pragma', 'no-cache')
            self.send_header('Content-Type', 'multipart/x-mixed-replace; boundary=FRAME')
            self.end_headers()
            try:
                while True:
                    with output.condition:
                        output.condition.wait()
                        frame = output.frame
                    self.wfile.write(b'--FRAME\r\n')
                    self.send_header('Content-Type', 'image/jpeg')
                    self.send_header('Content-Length', len(frame))
                    self.end_headers()
                    self.wfile.write(frame)
                    self.wfile.write(b'\r\n')
            except Exception as e:
                print('Removed streaming client' + str(self.client_address) + ' : ' + str(e))
        else:
            self.send_error(404)
            self.end_headers()
	#
#

class StreamingServer(ThreadingMixIn, server.HTTPServer):
    allow_reuse_address = True
    daemon_threads = True
#

def keyboardInterruptHandler(signal, frame):
	fwd_r.stop()
	fwd_l.stop()
	bwd_r.stop()
	bwd_l.stop()
	cam.stop()
	GPIO.cleanup()
	exit(0)
#

def set_cam(pos):
	cam.ChangeDutyCycle(pos)
	sleep(0.3)
	cam.ChangeDutyCycle(0.0)
#

async def driver(websocket, path):
	w = False
	s = False
	a = False
	d = False
	duty_cycle = 20.0 # default speed
	turn_rate = 0.4 # difference between motors speeds when turning
	difference_compensation = 0.87 # a vaule to compensate that the right motor is a bit faster
	cam_pos = 4.0 # default camera angle
	
	while True:
		data = await websocket.recv()
		key = data.split()
		
		if len(key) != 2:
			continue
		
		if key[1] == "ArrowUp" and key[0] == "dn":
			cam_pos += 0.5
			if cam_pos > 7:
				cam_pos = 7
			_thread.start_new_thread( set_cam, (cam_pos, ) )

		if key[1] == "ArrowDown" and key[0] == "dn":
			cam_pos -= 0.2
			if cam_pos < 3:
				cam_pos = 3
			_thread.start_new_thread( set_cam, (cam_pos, ) )
			
		if key[1] == "w":
			w = key[0] == "dn"
		elif key[1] == "s":
			s = key[0] == "dn"
		elif key[1] == "a":
			a = key[0] == "dn"
		elif key[1] == "d":
			d = key[0] == "dn"
		elif (key[1] == "+" or key[1] == "=") and key[0] == "dn":
			duty_cycle += 10.0
			if duty_cycle > 100.0:
				duty_cycle = 100.0
		elif key[1] == "-" and key[0] == "dn":
			duty_cycle -= 10.0
			if duty_cycle < 0.0:
				duty_cycle = 0.0
		else:
			continue
		
		if w:
			if s: # W and S pressed at once:
				fwd_r.ChangeDutyCycle(0.0)
				bwd_r.ChangeDutyCycle(0.0)
				fwd_l.ChangeDutyCycle(0.0)
				bwd_l.ChangeDutyCycle(0.0)	
			else:
				if a:
					if d: # W, A, D pressed at once:
						fwd_r.ChangeDutyCycle(difference_compensation*duty_cycle)
						bwd_r.ChangeDutyCycle(0.0)
						fwd_l.ChangeDutyCycle(duty_cycle)
						bwd_l.ChangeDutyCycle(0.0)
					else: # W and A pressed at once:
						fwd_r.ChangeDutyCycle(difference_compensation*duty_cycle)
						bwd_r.ChangeDutyCycle(0.0)
						fwd_l.ChangeDutyCycle(duty_cycle*turn_rate)
						bwd_l.ChangeDutyCycle(0.0)
				else:
					if d: # W and D pressed at once:
						fwd_r.ChangeDutyCycle(difference_compensation*duty_cycle*turn_rate)
						bwd_r.ChangeDutyCycle(0.0)
						fwd_l.ChangeDutyCycle(duty_cycle)
						bwd_l.ChangeDutyCycle(0.0)
					else: # W pressed:
						fwd_r.ChangeDutyCycle(difference_compensation*duty_cycle)
						bwd_r.ChangeDutyCycle(0.0)
						fwd_l.ChangeDutyCycle(duty_cycle)
						bwd_l.ChangeDutyCycle(0.0)
		else:
			if s:
				if a:
					if d: # S, A, D pressed at once:
						fwd_r.ChangeDutyCycle(0.0)
						bwd_r.ChangeDutyCycle(difference_compensation*duty_cycle)
						fwd_l.ChangeDutyCycle(0.0)
						bwd_l.ChangeDutyCycle(duty_cycle)
					else: # S and A pressed at once:
						fwd_r.ChangeDutyCycle(0.0)
						bwd_r.ChangeDutyCycle(difference_compensation*duty_cycle)
						fwd_l.ChangeDutyCycle(0.0)
						bwd_l.ChangeDutyCycle(duty_cycle*turn_rate)
				else:
					if d: # S and D pressed at once:
						fwd_r.ChangeDutyCycle(0.0)
						bwd_r.ChangeDutyCycle(difference_compensation*duty_cycle*turn_rate)
						fwd_l.ChangeDutyCycle(0.0)
						bwd_l.ChangeDutyCycle(duty_cycle)
					else: # S pressed:
						fwd_r.ChangeDutyCycle(0.0)
						bwd_r.ChangeDutyCycle(difference_compensation*duty_cycle)
						fwd_l.ChangeDutyCycle(0.0)
						bwd_l.ChangeDutyCycle(duty_cycle)	
			else:
				if a:
					if d: # A and D pressed at once:
						fwd_r.ChangeDutyCycle(0.0)
						bwd_r.ChangeDutyCycle(0.0)
						fwd_l.ChangeDutyCycle(0.0)
						bwd_l.ChangeDutyCycle(0.0)
					else: # A pressed:
						fwd_r.ChangeDutyCycle(difference_compensation*duty_cycle)
						bwd_r.ChangeDutyCycle(0.0)
						fwd_l.ChangeDutyCycle(0.0)
						bwd_l.ChangeDutyCycle(duty_cycle)
				else:
					if d: # D pressed:
						fwd_r.ChangeDutyCycle(0.0)
						bwd_r.ChangeDutyCycle(difference_compensation*duty_cycle)
						fwd_l.ChangeDutyCycle(duty_cycle)
						bwd_l.ChangeDutyCycle(0.0)
					else: # all keys released:
						fwd_r.ChangeDutyCycle(0.0)
						bwd_r.ChangeDutyCycle(0.0)
						fwd_l.ChangeDutyCycle(0.0)
						bwd_l.ChangeDutyCycle(0.0)
#

def listener():
	asyncio.set_event_loop(asyncio.new_event_loop())
	start_server = websockets.serve(driver, ip_addr, websocket_port)
	asyncio.get_event_loop().run_until_complete(start_server)
	asyncio.get_event_loop().run_forever()
#

signal.signal(signal.SIGINT, keyboardInterruptHandler)

GPIO.setmode(GPIO.BCM)
GPIO.setup(1, GPIO.OUT) # GPIO 1 for camera servo
GPIO.setup(12, GPIO.OUT) # GPIO 12 for left motor, forward direction
GPIO.setup(16, GPIO.OUT) # GPIO 16 for right motor, forward direction
GPIO.setup(20, GPIO.OUT) # GPIO 20 for right motor, reverse direction
GPIO.setup(21, GPIO.OUT) # GPIO 21 for left motor, reverse direction

ip_addr=ifaddresses('wlan0')[AF_INET][0]['addr']
http_port=8000
websocket_port=8765

# set pins as PWM, frquency 400Hz
fwd_l = GPIO.PWM(12, 400) 
fwd_r = GPIO.PWM(16, 400)
bwd_r = GPIO.PWM(20, 400)
bwd_l = GPIO.PWM(21, 400)
# set pin as PWM, frquency 50Hz
cam = GPIO.PWM(1, 50)

#start generating PWM waveforms, 0% duty cycle
fwd_r.start(0)
fwd_l.start(0)
bwd_r.start(0)
bwd_l.start(0)
cam.start(0)

PAGE="""\
<html>
	<body>
		
		<center><img src="stream.mjpg" width="1280" height="720"></center>
		<script>
		
			//WebSocket for capturing the keyboard
			const socket = new WebSocket('ws://{local_ip_address}:{port}');
			
			document.addEventListener('keydown', function (event) {{
				if (event.defaultPrevented) {{
					return;
				}}
				socket.send("dn " + event.key);
			}});
			
			document.addEventListener('keyup', function (event) {{
				if (event.defaultPrevented) {{
					return;
				}}
				socket.send("up " + event.key);
			}});
			
		</script>
	</body>
</html>
""".format(local_ip_address=ip_addr, port=websocket_port)
	
_thread.start_new_thread( listener, () )
_thread.start_new_thread( set_cam, (4.0,) )

print("CPU temperature: " + str(CPUTemperature().temperature) + "*C")

with PiCamera(resolution='1280x720', framerate=30) as camera:
    output = StreamingOutput()
    camera.rotation = 180
    camera.start_recording(output, format='mjpeg')
	
    try:
        address = (ip_addr, http_port)
        server = StreamingServer(address, StreamingHandler)
        server.serve_forever()
    finally:
        camera.stop_recording()
#
