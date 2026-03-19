#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
	ofDisableArbTex();
	ofEnableAlphaBlending();

	// Get image to filter
	if (!sourceImage.load(argv[1])) {
		ofLogError() << "Failed to load source image";
		return;
	}
	if (!targetImage.load(argv[2])) {
		ofLogError() << "Failed to load target image";
		return;
	}

	// Shrink image if too large
	int maxDim = 2048;
	float scaleSource = glm::min(1.0f, float(maxDim) / glm::max(sourceImage.getWidth(), sourceImage.getHeight()));
	float scaleTarget = glm::min(1.0f, float(maxDim) / glm::max(targetImage.getWidth(), targetImage.getHeight()));
	if (scaleSource < 1.0f)
		sourceImage.resize(sourceImage.getWidth() * scaleSource, sourceImage.getHeight() * scaleSource);
	if (scaleTarget < 1.0f)
		targetImage.resize(targetImage.getWidth() * scaleTarget, targetImage.getHeight() * scaleTarget);

	// Set window size
	w = std::max(sourceImage.getWidth(), targetImage.getWidth());
	h = std::max(sourceImage.getHeight(), targetImage.getHeight());

	ofSetWindowShape(w, h);
	 
	// Initialize mask
	mask.resize(w * h, false);
}

//--------------------------------------------------------------
void ofApp::update() {
}

//--------------------------------------------------------------
void ofApp::draw() {
	if (appState == AppState::SelectingSource) {
		sourceImage.draw(0, 0);

		// Brush strokes
		ofSetColor(255, 0, 0, 50);

		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {
				if (mask[y * w + x]) {
					ofDrawRectangle(x, y, 1, 1);
				}
			}
		}

		ofNoFill();
		ofSetColor(255);
		ofDrawCircle(ofGetMouseX(), ofGetMouseY(), brushRadius);

	} else if (appState == AppState::CloningOnTarget) {
		targetImage.draw(0, 0);

		// Target box
		ofNoFill();
		ofSetColor(0, 255, 0);

		ofDrawRectangle(
			mouseXPreview,
			mouseYPreview,
			selectionWidth,
			selectionHeight
		);

		ofSetColor(255);
	}
}

//--------------------------------------------------------------
void ofApp::saveOutput() {
	if (!targetImage.save("output/output.png")) {
		ofLogError() << "Failed to save image!";
	} else {
		ofLogNotice() << "Saved image to output/output.png";
	}
}

//--------------------------------------------------------------
void ofApp::mousePressed(int mouseX, int mouseY, int button) {
	if (appState == AppState::SelectingSource) {
		brushing = true;
		brushMask(mouseX, mouseY);
	}
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int mouseX, int mouseY, int button) {
	if (brushing) {
		brushMask(mouseX, mouseY);
	}
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int mouseX, int mouseY) {
	mouseXPreview = mouseX;
	mouseYPreview = mouseY;
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int mouseX, int mouseY, int button) {
	if (appState == AppState::SelectingSource) {

		brushing = false;

		// Obtain the corners of the bounding box of the selected region
		int minX = w;
		int minY = h;
		int maxX = 0;
		int maxY = 0;

		for (int yy = 0; yy < h; yy++) {
			for (int xx = 0; xx < w; xx++) {
				if (mask[yy * w + xx]) {
					minX = std::min(minX, xx);
					maxX = std::max(maxX, xx);
					minY = std::min(minY, yy);
					maxY = std::max(maxY, yy);
				}
			}
		}

		selectionWidth = maxX - minX + 1;
		selectionHeight = maxY - minY + 1;

		// Crop the selected region from source
		selectedRegion.allocate(selectionWidth, selectionHeight, OF_IMAGE_COLOR);
		cloneMask.assign(selectionWidth * selectionHeight, false);

		for (int yy = 0; yy < selectionHeight; yy++) {
			for (int xx = 0; xx < selectionWidth; xx++) {

				int sx = minX + xx;
				int sy = minY + yy;

				selectedRegion.setColor(xx, yy, sourceImage.getColor(sx, sy));

				if (mask[sy * w + sx])
					cloneMask[yy * selectionWidth + xx] = true;
			}
		}

		selectedRegion.update();

		// Switch to Cloning mode
		appState = AppState::CloningOnTarget;
		ofLogNotice() << "Source region selected. Click on target to paste.";

	} else if (appState == AppState::CloningOnTarget) {
		pasteX = mouseX;
		pasteY = mouseY;

		// Clear the mask and mark the region to clone based on position and cloneMask
		mask.assign(w * h, false);

		for (int yy = 0; yy < selectionHeight; yy++) {
			for (int xx = 0; xx < selectionWidth; xx++) {

				if (!cloneMask[yy * selectionWidth + xx])
					continue;

				int tx = pasteX + xx;
				int ty = pasteY + yy;

				if (tx >= 0 && tx < w && ty >= 0 && ty < h) {
					mask[ty * w + tx] = true;
				}
			}
		}

		// Run seamless cloning
		computeDivergenceSeamless(targetImage, selectedRegion);
		solvePoissonSeamless(targetImage);

		targetImage.update();
		saveOutput();

		ofLogNotice() << "Pasted source region onto target.";
	}
}

//--------------------------------------------------------------
void ofApp::brushMask(int mouseX, int mouseY) {

	// Check a square around the mouse position, but only mark pixels within a radius. 
	for (int dy = -brushRadius; dy <= brushRadius; dy++) {
		for (int dx = -brushRadius; dx <= brushRadius; dx++) {
			if (dx * dx + dy * dy > brushRadius * brushRadius)
				continue;

			int x = mouseX + dx;
			int y = mouseY + dy;

			if (x < 0 || x >= sourceImage.getWidth() || y < 0 || y >= sourceImage.getHeight())
				continue;

			mask[y * w + x] = true;
		}
	}
}

//--------------------------------------------------------------
void ofApp::computeDivergenceSeamless(ofImage & targetImage, ofImage & sourceImage) {
	ofPixels & target = targetImage.getPixels();
	ofPixels & source = sourceImage.getPixels();

	// Assign indexes to masked pixels
	indexMap.resize(w * h, -1);
	int maskedPixels = 0;
	for (int yy = 0; yy < h; yy++) {
		for (int xx = 0; xx < w; xx++) {
			if (mask[yy * w + xx]) {
				indexMap[yy * w + xx] = maskedPixels++;
			}
		}
	}

	// Compute and store divergence of the gradient for masked pixels
	pixelsR.assign(maskedPixels, 0.0);
	pixelsG.assign(maskedPixels, 0.0);
	pixelsB.assign(maskedPixels, 0.0);

	for (int yy = 0; yy < h; yy++) {
		for (int xx = 0; xx < w; xx++) {

			// Skip non-masked pixels
			int i = indexMap[yy * w + xx];
			if (i == -1)
				continue;

			// Switch from target to clone coordinates
			int cloneX = xx - pasteX;
			int cloneY = yy - pasteY;

			if (cloneX < 0 || cloneX >= selectionWidth || cloneY < 0 || cloneY >= selectionHeight)
				continue;

			// Gather neighboring pixel colors
			ofColor cCenter = source.getColor(cloneX, cloneY);

			int cLeft = std::max(cloneX - 1, 0);
			int cRight = std::min(cloneX + 1, selectionWidth - 1);
			int cUp = std::max(cloneY - 1, 0);
			int cDown = std::min(cloneY + 1, selectionHeight - 1);

			ofColor colorLeft = source.getColor(cLeft, cloneY);
			ofColor colorRight = source.getColor(cRight, cloneY);
			ofColor colorUp = source.getColor(cloneX, cUp);
			ofColor colorDown = source.getColor(cloneX, cDown);

			// Divergence of the gradient 
			pixelsR[i] = 1.0 * (4.0 * cCenter.r - (colorLeft.r + colorRight.r + colorUp.r + colorDown.r));
			pixelsG[i] = 1.0 * (4.0 * cCenter.g - (colorLeft.g + colorRight.g + colorUp.g + colorDown.g));
			pixelsB[i] = 1.0 * (4.0 * cCenter.b - (colorLeft.b + colorRight.b + colorUp.b + colorDown.b));
		}
	}
}

//--------------------------------------------------------------
void ofApp::solvePoissonSeamless(ofImage & targetImage) {
	ofPixels & target = targetImage.getPixels();
	int N = pixelsR.size();
	if (N == 0)
		return;

	triplets.clear();
	bR.resize(N);
	bG.resize(N);
	bB.resize(N);

	for (int i = 0; i < N; i++) {
		bR[i] = pixelsR[i];
		bG[i] = pixelsG[i];
		bB[i] = pixelsB[i];
	}

	// Build sparse matrix A
	for (int yy = 0; yy < h; yy++) {
		for (int xx = 0; xx < w; xx++) {
			int i = indexMap[yy * w + xx];
			if (i == -1)
				continue;

			int count = 0;
			int neighbors[4][2] = { { xx - 1, yy }, { xx + 1, yy }, { xx, yy - 1 }, { xx, yy + 1 } };

			// Discard neighbors outside the image
			for (auto & n : neighbors) {
				int nx = n[0];
				int ny = n[1];
				if (nx < 0 || nx >= w || ny < 0 || ny >= h)
					continue;

				// Place a -1 in A for each neighbor pixel in the mask, and add the neighbor's color to the RHS if outside the mask
				int j = indexMap[ny * w + nx];
				if (j != -1) {
					triplets.push_back(T(i, j, -1)); 
					count++;

				} else {
					ofColor c = target.getColor(nx, ny);
					bR[i] += c.r;
					bG[i] += c.g;
					bB[i] += c.b;
					count++;
				}
			}

			triplets.push_back(T(i, i, count));
		}
	}

	A.resize(N, N);
	A.setFromTriplets(triplets.begin(), triplets.end());

	// Solve linear system
	Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;
	solver.analyzePattern(A);
	solver.factorize(A);

	Eigen::VectorXd xR = solver.solve(bR);
	Eigen::VectorXd xG = solver.solve(bG);
	Eigen::VectorXd xB = solver.solve(bB);

	// Copy solution into target image
	for (int yy = 0; yy < h; yy++) {
		for (int xx = 0; xx < w; xx++) {
			int i = indexMap[yy * w + xx];
			if (i == -1)
				continue;

			ofColor c;
			c.r = glm::clamp((int)round(xR[i]), 0, 255);
			c.g = glm::clamp((int)round(xG[i]), 0, 255);
			c.b = glm::clamp((int)round(xB[i]), 0, 255);
			target.setColor(xx, yy, c);
		}
	}

	targetImage.update(); 
	ofLogNotice() << "Poisson solve finished";
}
