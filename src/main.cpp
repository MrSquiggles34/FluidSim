#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main(int argc, char ** argv) {
	ofGLWindowSettings settings;
	settings.setGLVersion(3, 2);

	auto window = ofCreateWindow(settings);

	
	auto app = std::make_shared<ofApp>();
	/*app->argc = argc;
	app->argv = argv;*/

	ofRunApp(window, app);
	ofRunMainLoop();
}
