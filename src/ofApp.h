#pragma once

#include "ofMain.h"

#include <Eigen/Dense>
#include <Eigen/Sparse>

class ofApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();

	// Parse arguments
	int argc;
	char ** argv; 


	// Rendering objects
	ofImage sourceImage;
	ofImage targetImage;
	ofImage originalSource; // keep original source for gradient computation
	ofFbo framebuffer;


	// Image details
	int w;
	int h;


	// Saving
	void saveOutput();


	// Masking ----------------------------------------------

	int startX;
	int startY;
	int endX;
	int endY;

	int pasteX;
	int pasteY;

	int mouseXPreview;
	int mouseYPreview;

	bool brushing = false;
	int brushRadius = 40;

	vector<bool> mask;
	std::vector<bool> cloneMask;

	void mousePressed(int mouseX, int mouseY, int button);
	void mouseDragged(int mouseX, int mouseY, int button);
	void mouseReleased(int mouseX, int mouseY, int button);
	void mouseMoved(int mouseX, int mouseY);
	void brushMask(int mouseX, int mouseY);


	// Divergence computation -------------------------------
	// Map from pixel coordinates to index in masked pixel list, -1 for non-masked pixels
	vector<int> indexMap;

	// divergence vectors for each color channel
	vector<double> pixelsR;
	vector<double> pixelsG;
	vector<double> pixelsB;

	void computeDivergenceSeamless(ofImage & targetImage, ofImage & sourceImage);


	// Poisson Equation -------------------------------------
	typedef Eigen::Triplet<double> T;
	// Build the sparse matrix A 
	std::vector<T> triplets;
	Eigen::SparseMatrix<double> A; 

	// Right hand side vectors for each color channel
	Eigen::VectorXd bR; 
	Eigen::VectorXd bG;
	Eigen::VectorXd bB;
	

	void solvePoissonSeamless(ofImage & targetImage);

	// Selection tools --------------------------------------
	enum class AppState {
		SelectingSource, 
		CloningOnTarget 
	};

	AppState appState = AppState::SelectingSource;

	// Store the cloned source region
	ofImage selectedRegion;
	int selectionWidth = 0;
	int selectionHeight = 0;

};
