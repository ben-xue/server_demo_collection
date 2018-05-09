import socket
import x_pb2
import os

ip = "127.0.0.1"
port = 5000

def my_send(msg):
	pass

def connect_2_server():
	s = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
	s.connect(ip,port)

if __name__ == "__main__":
	#connect_2_server()

	msg = x_pb2.info()
	msg.data = "hello,world"

	s = msg.SerializeToString()
	print(s)

	print(len(s))

	with open("./a.txt","wb+") as f:
		f.write(s)
	# print(s)

	# q = "hello,world"
	# print(q)
	#my_send(msg)