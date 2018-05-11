import socket
import x_pb2
import os,time
import struct

HEAD_MSG_SIZE = 10

addr = ("127.0.0.1",5000)

#addr = ("120.78.164.80",30000)

def my_send(sock,msg,type1):
	target_str = msg.SerializeToString()
	length = len(target_str)
	print(length)
	len_bytes = struct.pack("i",length)
	type_bytes = struct.pack("i",type1)

	sock.send(len_bytes+type_bytes)
	sock.send(target_str)

def connect_2_server():
	s = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
	ret = s.connect(addr)
	print("connect success")
	return s

if __name__ == "__main__":
	s = connect_2_server()

	msg_login = x_pb2.on()
	msg_login.account = "xue"
	msg_login.passwd = "12345"
	my_send(s,msg_login,x_pb2.CMD_ON)

	msg = x_pb2.info()
	msg.data = "hello,world"

	msg1 = x_pb2.info()
	msg1.data = "this is fuck"

	my_send(s,msg,x_pb2.CMD_INFO)

	my_send(s,msg1,x_pb2.CMD_INFO)

	time.sleep(1)

	s.close()



