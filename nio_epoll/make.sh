protoc -I=./proto  --cpp_out=./server/  x.proto

protoc -I=./proto  --cpp_out=./cpp_client  x.proto

protoc -I=./proto  --python_out=./python_client/  x.proto

g++ ./server/server.cpp ./server/x.pb.cc -o s -lprotobuf -g

g++ ./cpp_client/client.cpp ./server/x.pb.cc -o c -lprotobuf -g

