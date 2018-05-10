import socket
import x_pb2
import os,time

addr = ("127.0.0.1",5000)

#addr = ("120.78.164.80",30000)

def my_send(sock,msg):
	s_msg = msg.SerializeToString()
	length = len(s_msg)

	s_head = x_pb2.head_msg()
	s_head.size = length
	s_head.type = x_pb2.CMD_INFO

	ret1 = sock.send(s_head.SerializeToString())

	ret2 = sock.send(s_msg)


	if ret1 == 10 and ret2 == length:
		return True


def connect_2_server():
	s = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
	ret = s.connect(addr)
	print(ret,type(ret))
	print("connect success")
	return s

if __name__ == "__main__":
	s = connect_2_server()

	msg_login = x_pb2.on()
	msg_login.account = "xue"
	msg_login.passed = "12345"
	my_send(msg_login)

	msg = x_pb2.info()
	msg.data = "hello,world"

	msg1 = x_pb2.info()
	msg1.data = "this is fuck"

	if my_send(s,msg):
		print("msg send ok")

	if my_send(s,msg1):
		print("msg1 send ok")

	s.close()

