#include "server/x.pb.h"
#include  <fstream>
#include <iostream>
#include <google/protobuf/message_lite.h>

using namespace std;

int main()
{
	info m_info;
	m_info.set_data("hello,world");
	fstream output("./log.txt", ios::out | ios::trunc | ios::binary); 

	if (!m_info.SerializeToOstream(&output)) { 
		cerr << "Failed to write msg." << endl; 
		return -1; 
	} 
	return 0;
}

