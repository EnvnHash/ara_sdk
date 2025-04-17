#include "../../../Libraries/GLBase/src/Res/ResInstance.h"
#include "../../../Libraries/Utilities/src/Log.h"
#include <string>

using namespace ara;
using namespace ara;
using namespace std;

int main(int argc, char* argv[]) {

	if (argc < 3) {

		printf("ERROR\n");
		printf("Incorrect number of arguments\n");
		printf("usage: rescomp folder_src_path compilation_dest_filepath\n");

		return -2;
	}

	filesystem::path folder=argv[1];
	filesystem::path destfile=argv[2];

	if (!filesystem::exists(folder)) {
		printf("ERROR: Source folder does not exist\n");
		return -2;
	}

	Instance rinst(folder.string(),"");

	if (rinst.Compile(destfile)) {
		printf("OK: Compiled file @ %s\n",destfile.string().c_str());
	}
	else {
		printf("ERROR: Cannot compile resource file\n");
		return -3;
	}

	return 0;
}

