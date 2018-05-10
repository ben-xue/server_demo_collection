import socket
import x_pb2
import os,time

addr = ("127.0.0.1",5000)

def my_send(sock,msg):
	s_msg = msg.SerializeToString()
	length = len(s_msg)

	print(length)

	s_head = x_pb2.head_msg()
	s_head.size = length
	s_head.type = x_pb2.CMD_INFO

	sock.send(s_head.SerializeToString())

	sock.send(s_msg)


def connect_2_server():
	s = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
	s.connect(addr)
	print("connect success")
	return s

if __name__ == "__main__":
	s = connect_2_server()

	msg = x_pb2.info()
	msg.data = "hello,world"

	my_send(s,msg)

	while True:
		time.sleep(10)
