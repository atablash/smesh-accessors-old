
#include <glog/logging.h>



struct Init {
	Init(){
		google::InstallFailureSignalHandler();
	}
} init;


