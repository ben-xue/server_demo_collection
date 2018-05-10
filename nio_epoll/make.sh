protoc -I=./proto  --cpp_out=./server/  x.proto

protoc -I=./proto  --python_out=./client/  x.proto

g++ ./server/server.cpp ./server/x.pb.cc -o s -lprotobuf

