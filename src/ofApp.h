#pragma once

#include "ofMain.h"
#include "ofxFlowTools.h"
#include "ofxGui.h"

using namespace flowTools;

enum visualizationTypes { OBSTACLE, OBST_OFFSET, FLUID_BUOY, FLUID_VORT, FLUID_DIVE, FLUID_TMP, FLUID_PRS, FLUID_VEL, FLUID_DEN };

const vector<string> visualizationNames({ "obstacle", "obstacle offset", "fluid buoyancy", "fluid vorticity", "fluid divergence", "fluid temperature", "fluid pressure", "fluid velocity", "fluid density" });

class ofApp : public ofBaseApp {

public:
	void setup();
	void update();
	void draw();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);


	int	densityWidth;
	int densityHeight;

	int simulationWidth;
	int simulationHeight;

	int windowWidth;
	int windowHeight;

	// Mouse controls
	ofVec2f prevMouse;
	ofVec2f mouseVel;

	ofFbo velocityFbo;
	ofFbo densityFbo;
	ofFbo freezeMaskFbo;


	// Store fluid pipeline into flows
	vector< ftFlow* >		flows;
	ftFluidFlow				fluidFlow;

	// For importing obstacles
	ofImage					flowToolsLogo;

	// Inserting Density
	float pigmentTimer = 0.0f;
	float pigmentInterval = 30.0f;   

	// Inserting Velocity
	float velocityTimer = 0;
	float velocityInterval = 0.128f;

	void injectVelocity();
	void injectDensity();
	void injectCurlVelocity();
	void updateFreezeMask();

	ofParameter<int>		outputWidth;
	ofParameter<int>		outputHeight;
	ofParameter<int>		simulationScale;
	ofParameter<int>		simulationFPS;
	void simulationResolutionListener(int& _value);

	ofParameterGroup		visualizationParameters;
	ofParameter<int>		visualizationMode;
	ofParameter<string>		visualizationName;
	ofParameter<float>		visualizationScale;
	ofParameter<bool>		toggleVisualizationScalar;
	void visualizationModeListener(int& _value) { visualizationName.set(visualizationNames[_value]); }
	void visualizationScaleListener(float& _value) { for (auto flow : flows) { flow->setVisualizationScale(_value); } }
	void toggleVisualizationScalarListener(bool& _value) { for (auto flow : flows) { flow->setVisualizationToggleScalar(_value); } }


	ofxPanel			gui;
	void				setupGui();

	ofParameter<float>	guiFPS;
	ofParameter<float>	guiMinFPS;
	ofParameter<bool>	toggleFullScreen;
	ofParameter<bool>	toggleGuiDraw;
	ofParameter<bool>	toggleReset;

	void				toggleFullScreenListener(bool& _value) { ofSetFullscreen(_value); }
	void				toggleResetListener(bool& _value);
	void 				windowResized(ofResizeEventArgs& _resize) { windowWidth = _resize.width; windowHeight = _resize.height; }

	void				drawGui();
	deque<float>		deltaTimeDeque;
	float				lastTime;

};
