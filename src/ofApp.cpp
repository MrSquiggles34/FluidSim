#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
	ofSetVerticalSync(true);
	ofSetLogLevel(OF_LOG_NOTICE);

	densityWidth = 1600;
	densityHeight = 900;
	simulationWidth = densityWidth / 2;
	simulationHeight = densityHeight / 2;
	windowWidth = ofGetWindowWidth();
	windowHeight = ofGetWindowHeight();

	// Velocity and Density Fbo
	velocityFbo.allocate(simulationWidth, simulationHeight, GL_RG32F);
	densityFbo.allocate(simulationWidth, simulationHeight, GL_RGBA32F);

	ftUtil::zero(velocityFbo);

	fluidFlow.setup(simulationWidth, simulationHeight, densityWidth, densityHeight);

	// Fluid dissapation setting
	fluidFlow.setDissipationVel(0.05f); // velocity slowly settles
	fluidFlow.setDissipationDen(0.0f);  // Density never fades
	fluidFlow.setDissipationTmp(0.0f);

	fluidFlow.setViscosityVel(0.4f);
	fluidFlow.setViscosityDen(0.2f);

	fluidFlow.setVorticity(0.15f);

	fluidFlow.setBuoyancyWeight(0.0f);   // no rising smoke effect
	fluidFlow.setBuoyancySigma(0.0f);
	fluidFlow.setBuoyancyAmbientTemperature(0.0f);

	// Initiall fluid layout
	densityFbo.begin();
	ofClear(255, 255, 255, 255);

	for (int i = 0; i < 600; i++) {
		float x = ofRandom(densityWidth);
		float y = ofRandom(densityHeight);

		ofSetColor(
			60 + ofRandom(60),
			80 + ofRandom(80),
			150 + ofRandom(80)
		);

		ofDrawEllipse(x, y,
			ofRandom(20, 120),
			ofRandom(10, 60));
	}
	densityFbo.end();

	fluidFlow.addDensity(densityFbo.getTexture());

	// Reset all flows together and modular processing
	flows.push_back(&fluidFlow);

	flowToolsLogo.load("flowtools.png");
	fluidFlow.addObstacle(flowToolsLogo.getTexture());

	lastTime = ofGetElapsedTimef();

	setupGui();
}

void ofApp::setupGui() {

	gui.setup("settings");
	gui.setDefaultBackgroundColor(ofColor(0, 0, 0, 127));
	gui.setDefaultFillColor(ofColor(160, 160, 160, 160));
	gui.add(guiFPS.set("average FPS", 0, 0, 60));
	gui.add(guiMinFPS.set("minimum FPS", 0, 0, 60));
	gui.add(toggleFullScreen.set("fullscreen (F)", false));
	toggleFullScreen.addListener(this, &ofApp::toggleFullScreenListener);
	gui.add(toggleGuiDraw.set("show gui (G)", true));
	gui.add(toggleReset.set("reset (R)", false));
	toggleReset.addListener(this, &ofApp::toggleResetListener);
	gui.add(outputWidth.set("output width", 1280, 256, 1920));
	gui.add(outputHeight.set("output height", 720, 144, 1080));
	gui.add(simulationScale.set("simulation scale", 2, 1, 4));
	gui.add(simulationFPS.set("simulation fps", 60, 1, 60));
	outputWidth.addListener(this, &ofApp::simulationResolutionListener);
	outputHeight.addListener(this, &ofApp::simulationResolutionListener);
	simulationScale.addListener(this, &ofApp::simulationResolutionListener);


	visualizationParameters.setName("visualization");
	visualizationParameters.add(visualizationName.set("name", "fluidFlow Density"));
	visualizationParameters.add(visualizationMode.set("mode", FLUID_DEN, OBSTACLE, FLUID_DEN));
	visualizationParameters.add(visualizationScale.set("scale", 1, 0.1, 10.0));
	visualizationParameters.add(toggleVisualizationScalar.set("show scalar", false));
	visualizationMode.addListener(this, &ofApp::visualizationModeListener);
	toggleVisualizationScalar.addListener(this, &ofApp::toggleVisualizationScalarListener);
	visualizationScale.addListener(this, &ofApp::visualizationScaleListener);

	gui.add(visualizationParameters);
	for (auto flow : flows) {
		gui.add(flow->getParameters());
	}

	if (!ofFile("settings.xml")) { gui.saveToFile("settings.xml"); }
	gui.loadFromFile("settings.xml");

	gui.minimizeAll();
	toggleGuiDraw = true;
}


//--------------------------------------------------------------
void ofApp::update() {
	float t = ofGetElapsedTimef();
	float dt = 1.0f / max(ofGetFrameRate(), 1.0f);

	// Accumulate Time
	pigmentTimer += dt;
	velocityTimer += dt;

	// Clear both framebuffers
	ftUtil::zero(velocityFbo);

	
	while (velocityTimer >= velocityInterval) {
		velocityTimer -= velocityInterval;
		injectVelocity();
	}

	fluidFlow.addVelocity(velocityFbo.getTexture(), 1.0f);

	while (pigmentTimer >= pigmentInterval) {
		pigmentTimer -= pigmentInterval;
		injectDensity();
	}

	// Freeze the final texture
	if (ofGetElapsedTimef() > 15.0f) {
		fluidFlow.setDissipationVel(1.0f);
		fluidFlow.setViscosityVel(1.0f);
	}

	fluidFlow.update(dt);
}

//--------------------------------------------------------------
void ofApp::draw() {
	ofClear(0, 0);

	ofPushStyle();

	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	switch (visualizationMode.get()) {
	case OBSTACLE:		fluidFlow.drawObstacle(0, 0, windowWidth, windowHeight); break;
	case OBST_OFFSET:	fluidFlow.drawObstacleOffset(0, 0, windowWidth, windowHeight); break;
	case FLUID_BUOY:	fluidFlow.drawBuoyancy(0, 0, windowWidth, windowHeight); break;
	case FLUID_VORT:	fluidFlow.drawVorticity(0, 0, windowWidth, windowHeight); break;
	case FLUID_DIVE:	fluidFlow.drawDivergence(0, 0, windowWidth, windowHeight); break;
	case FLUID_TMP:		fluidFlow.drawTemperature(0, 0, windowWidth, windowHeight); break;
	case FLUID_PRS:		fluidFlow.drawPressure(0, 0, windowWidth, windowHeight); break;
	case FLUID_VEL:		fluidFlow.drawVelocity(0, 0, windowWidth, windowHeight); break;
	case FLUID_DEN:		fluidFlow.draw(0, 0, windowWidth, windowHeight); break;
	default: break;
	}


	// ===== STEP 6: STYLIZATION PASS =====
	ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);
	ofSetColor(100, 120, 180, 80);
	ofDrawRectangle(0, 0, windowWidth, windowHeight);

	// Optional highlight pass
	ofEnableBlendMode(OF_BLENDMODE_ADD);
	ofSetColor(20, 30, 50, 40);
	ofDrawRectangle(0, 0, windowWidth, windowHeight);

	if (toggleGuiDraw) {
		ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		drawGui();
	}
	ofPopStyle();
}

void ofApp::drawGui() {
	guiFPS = (int)(ofGetFrameRate() + 0.5);

	// calculate minimum fps
	float deltaTime = ofGetElapsedTimef() - lastTime;
	lastTime = ofGetElapsedTimef();
	deltaTimeDeque.push_back(deltaTime);

	while (deltaTimeDeque.size() > guiFPS.get())
		deltaTimeDeque.pop_front();

	float longestTime = 0;
	for (int i = 0; i < (int)deltaTimeDeque.size(); i++) {
		if (deltaTimeDeque[i] > longestTime)
			longestTime = deltaTimeDeque[i];
	}

	guiMinFPS.set(1.0 / longestTime);

	ofPushStyle();
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	gui.draw();
	ofPopStyle();
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	switch (key) {
	default: break;
	case '6': visualizationMode.set(FLUID_VORT); break;
	case '7': visualizationMode.set(FLUID_TMP); break;
	case '8': visualizationMode.set(FLUID_PRS); break;
	case '9': visualizationMode.set(FLUID_VEL); break;
	case '0': visualizationMode.set(FLUID_DEN); break;
	case 'G':toggleGuiDraw = !toggleGuiDraw; break;
	case 'F': toggleFullScreen.set(!toggleFullScreen.get()); break;
	case 'R': toggleReset.set(!toggleReset.get()); break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}

//--------------------------------------------------------------


//--------------------------------------------------------------
void ofApp::toggleResetListener(bool& _value) {
	if (_value) {
		for (auto flow : flows) { flow->reset(); }
		fluidFlow.addObstacle(flowToolsLogo.getTexture());
	}
	_value = false;
}

void ofApp::simulationResolutionListener(int& _value) {
	densityWidth = outputWidth;
	densityHeight = outputHeight;
	simulationWidth = densityWidth / simulationScale;
	simulationHeight = densityHeight / simulationScale;

	fluidFlow.resize(simulationWidth, simulationHeight, densityWidth, densityHeight);
	fluidFlow.addObstacle(flowToolsLogo.getTexture());
}

void ofApp::injectVelocity() {
	velocityFbo.begin();

	// IMPORTANT: zero velocity = black
	ofClear(0, 0, 0, 255);

	ofEnableBlendMode(OF_BLENDMODE_ADD);

	float t = ofGetElapsedTimef();

	float cx = simulationWidth * 0.5f;
	float cy = simulationHeight * 0.5f;

	for (int i = 0; i < 20; i++) {

		float angle = ofRandom(TWO_PI);
		float radius = ofRandom(simulationWidth * 0.3f);

		float x = cx + cos(angle) * radius;
		float y = cy + sin(angle) * radius;

		// Tangential swirl direction
		float vx = -sin(angle) * 5.0f;
		float vy = cos(angle) * 5.0f;

		ofSetColor(ofFloatColor(vx, vy, 0.0, 1.0));
		ofDrawCircle(x, y, 10);
	}

	velocityFbo.end();
}

void ofApp::injectDensity() {
	ftUtil::zero(densityFbo);

	densityFbo.begin();
	ofEnableBlendMode(OF_BLENDMODE_ADD);

	float x = ofRandom(simulationWidth);
	float y = ofRandom(simulationHeight);

	ofSetColor(
		80 + ofRandom(80),
		120 + ofRandom(80),
		200 + ofRandom(40)
	);

	ofDrawCircle(x, y, ofRandom(20, 60));
	densityFbo.end();

	fluidFlow.addDensity(densityFbo.getTexture(), 0.2f);
}