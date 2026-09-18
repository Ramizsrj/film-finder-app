#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){

	// Always log to a real file on disk (not just OutputDebugString/console,
	// which can be lost if the app crashes before stdio flushes reach it).
	ofLogToFile("filmFinder_log.txt", false);
	ofSetLogLevel(OF_LOG_NOTICE);

	//Use ofGLFWWindowSettings for more options like multi-monitor fullscreen
	ofGLWindowSettings settings;
	settings.setSize(1024, 768);
	settings.windowMode = OF_WINDOW; //can also be OF_FULLSCREEN

	auto window = ofCreateWindow(settings);

	ofRunApp(window, std::make_shared<ofApp>());
	ofRunMainLoop();

}
