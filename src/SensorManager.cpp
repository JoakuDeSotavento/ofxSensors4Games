//
//  sensorManagement.cpp
//  ofxControlArkadeGames
//
//  Created by carles on 17/06/16.
//
//

#include "SensorManager.h"

// SINGLETON initalizations
bool SensorManager::instanceFlag = false;
SensorManager* SensorManager::single = NULL;

//----------------------------------------------

SensorManager* SensorManager::getInstance()
{
	if(! instanceFlag)
	{
		single = new SensorManager();
		instanceFlag = true;
		return single;
	}else{
		return single;
	}
}

//----------------------------------------------
SensorManager::SensorManager()
{
	
}
//----------------------------------------------
SensorManager::~SensorManager()
{}

//Getters
//-----------------------------------------
sensorType SensorManager::getSensorType(){
	return typeSensor;
}
//-----------------------------------------
sensorMode SensorManager::getSensorMode(){
	return modeSensor;
}

//-----------------------------------------
ofxJSONElement SensorManager::getParams()
{
	ofxJSONElement jsonParams;
	jsonParams.clear();

	for (int i = 0; i < playerAreas.size(); i++) {
		jsonParams["sensorParams"][ofToString(i+1,2)]["bAreaActive"] = ofToString(playerAreas[i].bAreaActive);
		jsonParams["sensorParams"][ofToString(i+1,2)]["rectAreaX"] = ofToString(playerAreas[i].rectArea.x);
		jsonParams["sensorParams"][ofToString(i+1,2)]["rectAreaY"] = ofToString(playerAreas[i].rectArea.y);
		jsonParams["sensorParams"][ofToString(i+1,2)]["rectAreaW"] = ofToString(playerAreas[i].rectArea.width);
		jsonParams["sensorParams"][ofToString(i+1,2)]["rectAreaH"] = ofToString(playerAreas[i].rectArea.height);
	}


	return jsonParams;
}

//-------------------------------------------------------------------
bool SensorManager::setParams(ofxJSONElement jsonFile)
{
	bool bLoaded = true;

	cout << "SensorManager Params, jsonFile = " << jsonFile << endl;

	for (int j = 0; j < jsonFile.size(); j++) {
		if (jsonFile[j]["sensorParams"].size() > 0) {
			cout << "SensorManager player = " << jsonFile[j]["sensorParams"] << endl;
				
			bLoaded = bLoaded + true;

			playerAreas.clear();
			for (int i = 0; i < numPlayersAreas; i++) {
				PlayerArea auxPlayerArea;
				auxPlayerArea.bAreaActive = ofToBool(jsonFile[j]["sensorParams"][ofToString(i + 1, 2)]["bAreaActive"].asString());
				auxPlayerArea.rectArea.x = ofToFloat(jsonFile[j]["sensorParams"][ofToString(i + 1, 2)]["rectAreaX"].asString());
				auxPlayerArea.rectArea.y = ofToFloat(jsonFile[j]["sensorParams"][ofToString(i + 1, 2)]["rectAreaY"].asString());
				auxPlayerArea.rectArea.width = ofToFloat(jsonFile[j]["sensorParams"][ofToString(i + 1, 2)]["rectAreaW"].asString());
				auxPlayerArea.rectArea.height = ofToFloat(jsonFile[j]["sensorParams"][ofToString(i + 1, 2)]["rectAreaH"].asString());
				playerAreas.push_back(auxPlayerArea);
			}
		}
	}


	//Small Hack if modeSensor was not saved set first VideoPlayer
	int axuSensorMode = -1;
	//Read from JSON
	//....
	if (axuSensorMode == -1) {
		setSensorMode(realTimeMode); //simulationMode
	}
	else {
		//Set the loaded mode
	}

	
	return bLoaded;
}

//Setters
//-----------------------------------------
void SensorManager::setSensorType(sensorType _type){
	typeSensor =_type;
}

//-----------------------------------------
void SensorManager::setSensorMode(sensorMode _mode) {
	modeSensor = _mode;
}


//-----------------------------------------
void SensorManager::setup(sensorType _sensorType){
	
	typeSensor = _sensorType;
	
	///////////////////////
	//Sensor Configurations
	bool bSensorReady = false;
	
	if (typeSensor == kinectSensor) {
#ifdef USE_SENSOR_KINECT
		bSensorReady = setupKinectSensor();
		depthKinectImage.allocate(sensorWidth, sensorHeight, OF_IMAGE_GRAYSCALE);
		myThresholdKinect.allocate(sensorWidth, sensorHeight, OF_IMAGE_GRAYSCALE);
		
		computerVisionSensor1.setup(1, sensorWidth, sensorHeight, kinectSensor); //TODO Change this to JSon params loaded at a config file
		computerVisionSensor2.setup(2, sensorWidth, sensorHeight, kinectSensor); //TODO Change this to JSon params loaded at a config file
#endif
	}else if (typeSensor == cameraSensor){
		bSensorReady = setupCameraSensor();
		//Setup CV
		
		int numInitialAreas = 2;//Hack Initial Vector Areas
		for (int i = 0; i < numInitialAreas; i++) {
			SensorComputerVision computerVisionSensorAux;
			computerVisionSensorVector.push_back(computerVisionSensorAux);
			computerVisionSensorVector[i].setup(i+1, sensorWidth, sensorHeight, cameraSensor);
		}
		//computerVisionSensor1.setup(1, sensorWidth, sensorHeight, cameraSensor); //TODO Change this to JSon params loaded at a config file
		//computerVisionSensor2.setup(2, sensorWidth, sensorHeight, cameraSensor); //TODO Change this to JSon params loaded at a config file
		
	}else if(typeSensor == externalSickSensor){
		bSensorReady = setupExternalSickSensor();
	}
	
	if(bSensorReady == false){
		cout << "//*///*/*/*/*/*/*/* Error sensor setup **/*/*/*////**///" << endl;
		ofExit(0);
	}

	//For Computer Vision Setup some general vars
	mySourcedSensorFbo.allocate(getWidth(), getHeight(), GL_RGB);
	sourceImageRaw.allocate(getWidth(), getHeight(), OF_IMAGE_COLOR);
	for (int i = 0; i < computerVisionSensorVector.size(); i++) {
		ofImage auxImage;
		auxImage.allocate(getWidth(), getHeight(), OF_IMAGE_COLOR); // Check if this areas are set correctly at the end
		sensorImageVector.push_back(auxImage);
	}
	//sensorImage1.allocate(getWidth(), getHeight(), OF_IMAGE_COLOR);
	//sensorImage2.allocate(getWidth(), getHeight(), OF_IMAGE_COLOR);//This should be allocated, like now
	

	//Mouse and Keyboard Events
	ofRegisterKeyEvents(this);
	
}

//----------------------------
void SensorManager::resetVideoInterface() {


	setupCameraSensor();

	//bool bLoadedVid = false;
	//if (modeSensor == simulationMode) {
	//	bLoadedVid = videoPlayerCam.load("videos/1.mov");
	//	if (bLoadedVid) {
	//		videoPlayerCam.play();
	//		cout << "Setup videoPlayer = " << videoPlayerCam.getMoviePath() << endl;
	//	}
	//	else {
	//		//bVideoPlayer = false;
	//		modeSensor = simulationMode;
	//	}
	//}

	//if (modeSensor == realTimeMode) {
	//	//videoGrabber.listDevices();
	//	cam.setDeviceID(idVideoGrabber);
	//	cam.setDesiredFrameRate(30);
	//	cam.initGrabber(640, 480);
	//	cout << "Setup videoGrabber ID= " << idVideoGrabber << endl;
	//}

}

//-----------------------------------------
void SensorManager::update(){
	
	bNewSensorFrame = false;
	
	if(typeSensor == kinectSensor){

#ifdef USE_SENSOR_KINECT

		kinect.update();
		
		// there is a new frame and we are connected
		if(kinect.isFrameNew()) {

			//if (modeSensor == realTimeMode) {
				
				sourceImageRaw.setFromPixels(kinect.getPixels().getPixels(), kinect.getWidth(), kinect.getHeight(), OF_IMAGE_COLOR);

				// load grayscale depth image from the kinect source
				depthKinectImage.setFromPixels(kinect.getDepthPixels());

				/**/
				//we do two thresholds - one for the far plane and one for the near plane
				//we do it ourselves - show people how they can work with the pixels
				ofPixels & pix = depthKinectImage.getPixels();
				int numPixels = pix.size();
				for (int i = 0; i < numPixels; i++) {
					if (pix[i] < nearThreshold && pix[i] > farThreshold) {
						pix[i] = 255;
					}
					else {
						pix[i] = 0;
					}
				}

				//Set pixels back to main cv image
				myThresholdKinect.setFromPixels(pix);
				
				//Shape Kinect Improves?
				//TODO for Kinect check how to:  scaling up, blurring, buffering, getting a new image from the result by finding contours and so on. You can get quite nice results then.


			//}
			//else {
			//	//TODO
			//}
			
				bNewSensorFrame = true;
		}

		

#endif

	}
	else if (typeSensor == cameraSensor){

		////////////////////////////////////////////////////////
		//ResetInterface if Video was not init & Update Internal VideoFrames
		if (modeSensor == realTimeMode) {
			if (!cam.isInitialized())resetVideoInterface();
			cam.update();
		}
		else if(modeSensor == simulationMode){
			if (!videoPlayerCam.isInitialized())resetVideoInterface();
			videoPlayerCam.update();
			if (cam.isInitialized())cam.close();
		}


		//Check if new Pixels
		if (modeSensor == simulationMode) bNewSensorFrame = videoPlayerCam.isFrameNew();
		else  bNewSensorFrame = cam.isFrameNew();
		
		
		if (bNewSensorFrame) {
			//Update raw pixels
			//Save it in a Image because later on is being used for others to share texture : Spout // TODO syphon
			if (modeSensor == simulationMode) {
				sourceImageRaw.setFromPixels(videoPlayerCam.getPixelsRef().getPixels(), getWidth(), getHeight(), OF_IMAGE_COLOR);
			}
			else {
				sourceImageRaw.setFromPixels(cam.getPixelsRef().getPixels(), getWidth(), getHeight(), OF_IMAGE_COLOR);
			}
		}
	}
	else if (typeSensor == externalSickSensor){
		//This updates osc data that contains all Blobs
		bNewSensorFrame = updateExternalSickSensor();
	}

	if (typeSensor == kinectSensor || typeSensor == cameraSensor) {
			//Diferent computer vision for All PlayerAreas
			//updateAllComputerVisionAreas();

			for (int i = 0; i < playerAreas.size(); i++) {
			
				if (playerAreas[i].bAreaActive) {

					if (typeSensor == cameraSensor)computerVisionSensorVector[i].udpateBackground();
					updateSubImagesFromImageRaw(playerAreas[i].rectArea, sensorImageVector[i]);
					computerVisionSensorVector[i].mainComputerVision(sensorImageVector[i]);

					//if (i == 0) {
					//	if(typeSensor == cameraSensor)computerVisionSensor1.udpateBackground();
					//	updateSubImagesFromImageRaw(playerAreas[i].rectArea, sensorImage1);
					//	computerVisionSensor1.mainComputerVision(sensorImage1);
					//}
					//else if (i == 1) {
					//	if (typeSensor == cameraSensor)computerVisionSensor2.udpateBackground();
					//	updateSubImagesFromImageRaw(playerAreas[i].rectArea, sensorImage2);
					//	computerVisionSensor2.mainComputerVision(sensorImage2);
					//}
				}

			}

	}
	

	//Some extra Operation at the end of sensor Dection ?
	//use  isNewSensorFrame for Lattely actions in other parts of code
	if(bNewSensorFrame){
		
	}

}

//-----------------------------------------
void SensorManager::applyMaskToImgVideoCam(ofRectangle _rectArea, ofImage & imageToMask) {

	mySourcedSensorFbo.begin();
	// Cleaning everthing with alpha mask on 0 in order to make it transparent for default
	ofClear(0, 0, 0, 0);

	//ofSetColor(0);
	//ofDrawRectangle(0,0, getWidth(), getHeight());

	//Draw Subsection
	ofSetColor(255);
	imageToMask.drawSubsection(_rectArea.x, _rectArea.y, _rectArea.width, _rectArea.height, _rectArea.x, _rectArea.y);

	
	mySourcedSensorFbo.end();

	//baseImageToMask.cropFrom(imageToMask, _rectArea.x, _rectArea.y, _rectArea.width, _rectArea.height);
	//baseImageToMask.update();
	ofPixels auxPixels;
	mySourcedSensorFbo.readToPixels(auxPixels);

	imageToMask.setFromPixels(auxPixels);

}

//-----------------------------------------
void SensorManager::updateSubImagesFromImageRaw(ofRectangle _rectArea, ofImage &image2Update) {
	//get Pixels from sensor ( VideoPlayer or VideoCamera )
	if (modeSensor == simulationMode) {
		image2Update.setFromPixels(sourceImageRaw.getPixelsRef().getPixels(), videoPlayerCam.getWidth(), videoPlayerCam.getHeight(), OF_IMAGE_COLOR);
	}
	else if (modeSensor == realTimeMode) {
		/*
		if (typeSensor == kinectSensor) {
			image2Update.setFromPixels(myThresholdKinect.getPixels().getPixels(), myThresholdKinect.getWidth(), myThresholdKinect.getHeight(), OF_IMAGE_GRAYSCALE);
		}
		else {*/
			image2Update.setFromPixels(sourceImageRaw.getPixelsRef().getPixels(), sourceImageRaw.getWidth(), sourceImageRaw.getHeight(), OF_IMAGE_COLOR);
		//}
	}

	image2Update.update();
	//Update this sensorImage and Crop desired Arae inside sensorImage1
	applyMaskToImgVideoCam(_rectArea, image2Update);
}

//-----------------------------------------
bool SensorManager::isNewSensorFrame(){
	return bNewSensorFrame;
}

//-----------------------------------------
void SensorManager::draw(){

	//Draw GUI Sensors
	
	ofSetColor(255, 255, 255);
	//Draw some Sensor Option
	static bool isSensorWindow = true;//TODO may be hide this in a regular bool loaded from json config file
	if (isSensorWindow) {
		this->drawGuiSensorOptions(&isSensorWindow);
	}


	//Draw Sensors
	if(typeSensor == kinectSensor){
		
#ifdef USE_SENSOR_KINECT

		ofSetColor(255, 255, 255);
		
		for (int i = 0; i < playerAreas.size(); i++) {

			// draw from the live kinect
			kinect.drawDepth(marginDraw, i * sensorHeight*sensorDrawScale, kinect.width*sensorDrawScale, kinect.height*sensorDrawScale);
			
			if (i == 0) {
				computerVisionSensor1.draw(sensorDrawScale, marginDraw + i * sensorHeight*sensorDrawScale);//TODO FIX THIS WAY TO DRAW ! Complex way
			}
			else if (i == 1) {
				computerVisionSensor2.draw(sensorDrawScale, marginDraw + i * sensorHeight*sensorDrawScale);//TODO FIX THIS WAY TO DRAW ! Complex way
			}
			
		}


		
		// draw instructions
		ofSetColor(255, 255, 255);
		stringstream reportStream;
		
		if(kinect.hasAccelControl()) {
			reportStream << "accel is: " << ofToString(kinect.getMksAccel().x, 2) << " / "
			<< ofToString(kinect.getMksAccel().y, 2) << " / "
			<< ofToString(kinect.getMksAccel().z, 2) << endl;
		} else {
			reportStream << "Note: this is a newer Xbox Kinect or Kinect For Windows device," << endl
			<< "motor / led / accel controls are not currently supported" << endl << endl;
		}
		
		reportStream << "press p to switch between images and point cloud, rotate the point cloud with the mouse" << endl
		//<< "using opencv threshold = " << bThreshWithOpenCV <<" (press spacebar)" << endl
		<< "set near threshold " << nearThreshold << " (press: + -)" << endl
		<< "set far threshold " << farThreshold
		//<< " (press: < >) num blobs found " << contourFinder.nBlobs
		//<< ", fps: " << ofGetFrameRate() << endl
		<< "press c to close the connection and o to open it again, connection is: " << kinect.isConnected() << endl;
		
		if(kinect.hasCamTiltControl()) {
			reportStream << "press UP and DOWN to change the tilt angle: " << angle << " degrees" << endl
			<< "press 1-5 & 0 to change the led mode" << endl;
		}
		
		ofDrawBitmapString(reportStream.str(), 20, 652);
#endif

	}
	else if (typeSensor == cameraSensor){
		
		ofSetColor(255, 255, 255);

		///
		for (int i = 0; i < playerAreas.size(); i++) {

			if (playerAreas[i].bAreaActive) {

				computerVisionSensorVector[i].computerVisionImage.draw(marginDraw, marginDraw + i*sensorHeight*sensorDrawScale, sensorWidth*sensorDrawScale, sensorHeight*sensorDrawScale);
				computerVisionSensorVector[i].draw(sensorDrawScale, marginDraw + i*sensorHeight*sensorDrawScale);//TODO FIX THIS WAY TO DRAW ! Complex way

				ofEnableAlphaBlending();
				ofSetColor(255, 255, 255, 100);
				sourceImageRaw.draw(marginDraw, marginDraw + i*sensorHeight*sensorDrawScale, sensorWidth*sensorDrawScale, sensorHeight*sensorDrawScale);
				ofDisableAlphaBlending();
				ofSetColor(255, 255, 255);
				//if (i == 0) {
				//	computerVisionSensor1.computerVisionImage.draw(marginDraw, marginDraw + i*sensorHeight*sensorDrawScale, sensorWidth*sensorDrawScale, sensorHeight*sensorDrawScale);
				//	computerVisionSensor1.draw(sensorDrawScale, marginDraw + i*sensorHeight*sensorDrawScale);//TODO FIX THIS WAY TO DRAW ! Complex way

				//	ofEnableAlphaBlending();
				//	ofSetColor(255, 255, 255, 100);
				//	sourceImageRaw.draw(marginDraw, marginDraw + i*sensorHeight*sensorDrawScale, sensorWidth*sensorDrawScale, sensorHeight*sensorDrawScale);
				//	ofDisableAlphaBlending();
				//	ofSetColor(255, 255, 255);
				//}
				//else if (i == 1) {
				//	computerVisionSensor2.computerVisionImage.draw(marginDraw, marginDraw + i*sensorHeight*sensorDrawScale, sensorWidth*sensorDrawScale, sensorHeight*sensorDrawScale);
				//	computerVisionSensor2.draw(sensorDrawScale, marginDraw + i*sensorHeight*sensorDrawScale);

				//	ofEnableAlphaBlending();
				//	ofSetColor(255, 255, 255, 100);
				//	sourceImageRaw.draw(marginDraw, marginDraw + i*sensorHeight*sensorDrawScale, sensorWidth*sensorDrawScale, sensorHeight*sensorDrawScale);
				//	ofDisableAlphaBlending();
				//	ofSetColor(255, 255, 255);
				//}
				//TODO more ? or make this functions dynamic .
			}

		}

	}
	else if(typeSensor == externalSickSensor){
		
		ofSetColor(ofColor::grey);
		ofDrawRectangle(0, 0, sensorWidth, sensorHeight);
		
		ofSetColor(ofColor::greenYellow);
		
		for (int i = 0; i < sickBlobs.size() ; i++){
			ofDrawBitmapString("id: " + ofToString(sickBlobs[i].id, 0), sickBlobs[i].pos.x*sensorWidth*sensorDrawScale, sickBlobs[i].pos.y*sensorWidth*sensorDrawScale + 7);
			ofDrawCircle(sickBlobs[i].pos.x*sensorWidth*sensorDrawScale, sickBlobs[i].pos.y*sensorWidth*sensorDrawScale, 10); //Rescaling
		}
	}

	//Finally Draw Filter Rectangles Player Areas
	if (typeSensor == cameraSensor || typeSensor == kinectSensor) {
		for (int i = 0; i < playerAreas.size(); i++) {
			drawAreaRectangle(playerAreas[i].rectArea, i + 1);
		}
	}
	
}

//--------------------------------------------------------------------
void SensorManager::drawAreaRectangle(ofRectangle _areaRect, int idSensor) {
	ofRectangle auxRectArea1 = _areaRect;
	auxRectArea1.x = _areaRect.x * sensorDrawScale;
	auxRectArea1.y = _areaRect.y *sensorDrawScale + (idSensor-1)*sensorHeight *sensorDrawScale;//Auto Down Draw Sensor
	auxRectArea1.width = _areaRect.getWidth() * sensorDrawScale;
	auxRectArea1.height = _areaRect.getHeight() *sensorDrawScale;

	//Draw it
	ofPushStyle();
	ofNoFill();
	ofSetColor(ofColor::red);
	ofDrawRectangle(auxRectArea1);
	ofPopStyle();

	ofSetColor(ofColor::white);
}

//---------------------------------------------------------------------
bool SensorManager::updateVideoFolderComboSelections(string _videosPaths) {
	
	videosAvailable.clear();
	
	svideosDirPath = _videosPaths; //TODO Create State Video or Sensor Ready or not Ready
	
	myVideosDir.listDir(svideosDirPath);
	if( selectedMovieIndex < myVideosDir.size() ){

		smovieFileName = myVideosDir.getPath(selectedMovieIndex); //Get the one preselected
		
	}//else{
		//cout << "Error No Video in the Default Video Folder = " << svideosDirPath << endl;
		//selectedMovieIndex = -1;
	//}
	
	
	for (int i = 0; i < myVideosDir.size(); i++) {
		videosAvailable.push_back(myVideosDir.getFile(i).getFileName());
	}
	
	//smoviePath = std::string(svideosDirPath.c_str());
	videoPlayerCam.stop();
	videoPlayerCam.close();
	bool bLoaded = videoPlayerCam.load(svideosDirPath+videosAvailable[selectedMovieIndex]);
	if(bLoaded) videoPlayerCam.play();
	if(bLoaded){
		if(sensorWidth != videoPlayerCam.getWidth()){
			sensorWidth = videoPlayerCam.getWidth();
			sensorHeight = videoPlayerCam.getHeight();
		}
		//TODO reset other important Vars?
		
	}
	
	return bLoaded;
}

//-----------------------------------------
void SensorManager::resetSimpleSensorCamera() {
	cam.close();
	cam.setDeviceID(selectedCameraIndex);
	cam.setup(640, 480);//TODO editable format
}

//-----------------------------------------
void SensorManager::drawGuiSensorOptions(bool* opened){
	
	string sensorTextType = "Not configured Yet";
	if(typeSensor == kinectSensor){

#ifdef USE_SENSOR_KINECT

		sensorTextType = "Kinect 1";

		ImGui::Text(sensorTextType.c_str());
		//ImGui::Checkbox("bThreshWithOpenCV", &bThreshWithOpenCV);
		if (ImGui::SliderInt("nearThreshold", &nearThreshold, 0, 255)) {
			//kinect.setDepthClipping(nearThreshold, farThreshold);
		}
		if (ImGui::SliderInt("farThreshold", &farThreshold, 0, 255)) {
			//kinect.setDepthClipping(nearThreshold, farThreshold);
		}
		ImGui::Separator();
		
#endif
		//ImGui::PopItemWidth();
	}else if (typeSensor == cameraSensor){
		sensorTextType = "OF Camera";
		ImGui::Text(sensorTextType.c_str());
		
		int auxModeType = modeSensor;
		ComboCinder("Select Video Mode", &auxModeType, selectableSensorMode, selectableSensorMode.size());
		if (auxModeType != modeSensor)modeSensor = (sensorMode)auxModeType;//Check if has been modif

		if (modeSensor == realTimeMode) {
	
			ImGui::PushItemWidth(200);
			static int myGuiCameraIndex = 0;
			if (ImGui::InputInt("Index Camera", &myGuiCameraIndex, 1)) {//Step one by one
				if (myGuiCameraIndex > -1 && myGuiCameraIndex < cam.listDevices().size()) {
					selectedCameraIndex = myGuiCameraIndex;
				}
				else {
					myGuiCameraIndex = selectedCameraIndex;
				}
			}
			ImGui::PopItemWidth();


			if (ImGui::Button("Reset Camera", ImVec2(150, 20))) {
				resetSimpleSensorCamera();
			}

			if (ImGui::Button("Settings Camera", ImVec2(150, 30))) {
				cam.videoSettings();
			}

			//Spout
#if defined(USE_SHARECAM_SPOUT)
			ImGui::Checkbox("SPOUT the Camera", &bSpoutCameraActive);
			if(bSpoutCameraActive)sender.sendTexture(sourceImageRaw.getTexture(), "Camera");
#endif

		
		}
		else if(modeSensor == simulationMode){
			
			//TODO InputTextFilterCharacter
			static char cmoviePath[60] = "videos/default/"; // = svideosDirPath //This will load fisrt Video available at the this Folder
			ImGui::PushItemWidth(200);
			ImGui::InputText("Videos Path", cmoviePath, IM_ARRAYSIZE_TEMP2(cmoviePath));
			static int myGuiVideoIndex = 0;
			if(ImGui::InputInt("Index video", &myGuiVideoIndex, 1)){//Step one by one
				if(myGuiVideoIndex > -1 && myGuiVideoIndex < videosAvailable.size()){
					selectedMovieIndex = myGuiVideoIndex;
				}else{
					myGuiVideoIndex = selectedMovieIndex;
				}
			}
			
			ImGui::SameLine();
			ImGui::PopItemWidth();
			ImGui::Checkbox("Reset Path", &bResetMoviePath);
			
			
			if(bResetMoviePath && selectedMovieIndex != -1){
				updateVideoFolderComboSelections(cmoviePath);
				
				bResetMoviePath = false;
				cout << "Reset Movie to " << svideosDirPath+videosAvailable[selectedMovieIndex] << endl;
			}
			
			videoPlayerCam_pos = videoPlayerCam.getPosition();
			if(ImGui::SliderFloat("VideoCam Position", &videoPlayerCam_pos, 0, 1)){
				videoPlayerCam.setPosition(videoPlayerCam_pos);
			}
		}
		
	}
	else if(typeSensor == externalSickSensor){
		string auxmessage = "Sick received in OSC at PORT " + ofToString(PortRecvExt, 0);
		ImGui::Text(auxmessage.c_str());
	}
	else{
		cout << "SensorManager::Error Set Sensor Mode Type" << endl;
	}

	//AREAS FOR KINECT AND CAMERA mode type sensor AVAILABLE // check others...
	if (typeSensor == cameraSensor || typeSensor == kinectSensor) {
		//Selectable Areas to Do Computer Vision separately. TODO : set this into modulable Array of playersAreas Array
		ImGui::SliderInt("Num Player Areas", &numPlayersAreas, 0, 2);
		if (playerAreas.size() != numPlayersAreas) {
			if (playerAreas.size() < numPlayersAreas) {
				//Add Default playerAreas into vector 
				PlayerArea auxPlayerArea;
				auxPlayerArea.bAreaActive = true;
				auxPlayerArea.rectArea.setWidth(sensorWidth);
				auxPlayerArea.rectArea.setHeight(sensorHeight);

				playerAreas.push_back(auxPlayerArea);

				//This require that all controllerReconition clases are being setup previously
				
			}
			else {
				//TODO Remove last vector pos until get equal
				playerAreas.pop_back();//Test
			}
		}

		for (int i = 0; i < playerAreas.size(); i++) {
			string textAreaSensor_collapsing = "Area [" + ofToString(i + 1, 0)+"]";
			string textAreaSensor_bActive = "Active?##" + ofToString(i + 1, 0);
			string textAreaSensor_x = "X##" + ofToString(i + 1, 2);
			string textAreaSensor_y = "Y##" + ofToString(i + 1, 2);
			string textAreaSensor_w = "W##" + ofToString(i + 1, 2);
			string textAreaSensor_h = "H##" + ofToString(i + 1, 2);

			if (ImGui::CollapsingHeader(textAreaSensor_collapsing.c_str())) {
				ImGui::Checkbox(textAreaSensor_bActive.c_str(), &playerAreas[i].bAreaActive);
				ImGui::PushItemWidth(100);
				ImGui::SliderFloat(textAreaSensor_x.c_str(), &playerAreas[i].rectArea.x, 0, getWidth());//TODO? limit this until (getWidth() -rectArea1.width). Check how to do it dynamic slider ImGui
				ImGui::SliderFloat(textAreaSensor_y.c_str(), &playerAreas[i].rectArea.y, 0, getHeight());
				ImGui::SliderFloat(textAreaSensor_w.c_str(), &playerAreas[i].rectArea.width, 0, getWidth());
				ImGui::SliderFloat(textAreaSensor_h.c_str(), &playerAreas[i].rectArea.height, 0, getHeight());
				ImGui::PopItemWidth();
			}

			//TODO improve this manual mode
			if (playerAreas[i].bAreaActive)computerVisionSensorVector[i].drawGui();
			//if (i == 0 && playerAreas[i].bAreaActive)computerVisionSensor1.drawGui();
			//if (i == 1 && playerAreas[i].bAreaActive)computerVisionSensor2.drawGui();
		}
	}

}


//-----------------------------------------
int SensorManager::getWidth(){
	return sensorWidth;
}

//-----------------------------------------
int SensorManager::getHeight(){
	return sensorHeight;
}

//-----------------------------------------
void SensorManager::exit(){
	
	if(typeSensor == kinectSensor){

#ifdef USE_SENSOR_KINECT
		kinect.setCameraTiltAngle(0); // zero the tilt on exit
		kinect.close();
#endif
		
	}
	else if(typeSensor == cameraSensor){
		//I guess this is not really necesary. They do it internally
		//videoPlayerCam.close();
		//cam.close();
	}
}

//------------------------------------------
void SensorManager::keyReleased(ofKeyEventArgs & args){}
void SensorManager::keyPressed(ofKeyEventArgs & args){
	
	if(typeSensor == kinectSensor){

#ifdef USE_SENSOR_KINECT

		switch (args.key) {
			case ' ':
				//bThreshWithOpenCV = !bThreshWithOpenCV;
				break;
				
			case '>':
			case '.':
				farThreshold ++;
				if (farThreshold > 255) farThreshold = 255;
				break;
				
			case '<':
			case ',':
				farThreshold --;
				if (farThreshold < 0) farThreshold = 0;
				break;
				
			case '+':
			case '=':
				nearThreshold ++;
				if (nearThreshold > 255) nearThreshold = 255;
				break;
				
			case '-':
				nearThreshold --;
				if (nearThreshold < 0) nearThreshold = 0;
				break;
				
			case 'w':
				kinect.enableDepthNearValueWhite(!kinect.isDepthNearValueWhite());
				break;
				
			case 'o':
				kinect.setCameraTiltAngle(angle); // go back to prev tilt
				kinect.open();
				break;
				
			case 'c':
				kinect.setCameraTiltAngle(0); // zero the tilt
				kinect.close();
				break;
				
			case '1':
				kinect.setLed(ofxKinect::LED_GREEN);
				break;
				
			case '2':
				kinect.setLed(ofxKinect::LED_YELLOW);
				break;
				
			case '3':
				kinect.setLed(ofxKinect::LED_RED);
				break;
				
			case '4':
				kinect.setLed(ofxKinect::LED_BLINK_GREEN);
				break;
				
			case '5':
				kinect.setLed(ofxKinect::LED_BLINK_YELLOW_RED);
				break;
				
			case '0':
				kinect.setLed(ofxKinect::LED_OFF);
				break;
				
			case OF_KEY_UP:
				angle++;
				if(angle>30) angle=30;
				kinect.setCameraTiltAngle(angle);
				break;
				
			case OF_KEY_DOWN:
				angle--;
				if(angle<-30) angle=-30;
				kinect.setCameraTiltAngle(angle);
				break;
		}
#endif
	}
}


//-----------------------------------------
bool SensorManager::setupCameraSensor(){
	
	bool bConnected = false;
	
	if(modeSensor == realTimeMode){
		
		cam.setVerbose(true);
		cam.listDevices();
		cam.setDeviceID(selectedCameraIndex);
		cam.setup(640, 480); // TODO get this from a JSON data config...TODO

		//rectArea1.set(0, 0, cam.getWidth(), cam.getHeight());
		//TODO load Json pre values
		for (int i = 0; i < playerAreas.size(); i++) {
			playerAreas[i].rectArea.set(playerAreas[i].rectArea.x, playerAreas[i].rectArea.y, playerAreas[i].rectArea.width, playerAreas[i].rectArea.height);
		}

		sensorWidth = cam.getWidth();
		sensorHeight = cam.getHeight();
		
		if(cam.isInitialized()){
			bConnected = true;
		}
		else {
			//second try
			//cout << "second try" << endl;
			//shared_ptr<ofDirectShowGrabber> auxGrabber = shared_ptr <OF_VID_GRABBER_TYPE>(new OF_VID_GRABBER_TYPE);
			//auxGrabber->VI.setupDevice(selectedCameraIndex, VI_USB);
			//cam.setGrabber(auxGrabber);
			//cam.setup(640, 480);
		}
		
	}
	
	if (bConnected == false)modeSensor = simulationMode;

	if(modeSensor == simulationMode){
		
#ifdef TARGET_WIN32
		svideosDirPath = "videos/default/";
#else
		svideosDirPath = "videos/default/";
#endif
		
		selectedMovieIndex = 0; //Default Index
		bConnected = updateVideoFolderComboSelections(svideosDirPath);

	}
	
	return bConnected;
}


//-----------------------------------------
bool SensorManager::setupKinectSensor(){
	
	bool bConnected = false;

#ifdef USE_SENSOR_KINECT

	
	// enable depth->video image calibration
	kinect.setRegistration(true);
	
	kinect.init();
	//kinect.init(true); // shows infrared instead of RGB video image
	//kinect.init(false, false); // disable video image (faster fps)
	
	kinect.open();		// opens first available kinect
	//kinect.open(1);	// open a kinect by id, starting with 0 (sorted by serial # lexicographically))
	//kinect.open("A00362A08602047A");	// open a kinect using it's unique serial #
	
	// print the intrinsic IR sensor values
	if(kinect.isConnected()) {
		ofLogNotice() << "sensor-emitter dist: " << kinect.getSensorEmitterDistance() << "cm";
		ofLogNotice() << "sensor-camera dist:  " << kinect.getSensorCameraDistance() << "cm";
		ofLogNotice() << "zero plane pixel size: " << kinect.getZeroPlanePixelSize() << "mm";
		ofLogNotice() << "zero plane dist: " << kinect.getZeroPlaneDistance() << "mm";
		
		bConnected = true;
	}
	
	// zero the tilt on startup
	angle = 0;
	kinect.setCameraTiltAngle(angle);
	
	//Related to ComputerVision
	nearThreshold = 230;
	farThreshold = 70;

	//kinect.setDepthClipping(nearThreshold, farThreshold);

	///////////////////////////////////
	//General SensorData Dimensions
	sensorWidth = kinect.width;
	sensorHeight = kinect.height;

#endif	
	return bConnected;
}


//-----------------------------------------
bool SensorManager::setupExternalSickSensor(){
		receiverExt.setup(PortRecvExt);
		sensorWidth = 640; //Fake Dims
		sensorHeight = 480;//
		//We supouse this is allways right? But port could be ocupied... //TODO check this case
	return true;
	
}

//-----------------------------------------
bool SensorManager::updateExternalSickSensor(){
	
	int counterBlobsId;
	bool hasReceivedSomeThing = false;
	
	if(receiverExt.hasWaitingMessages() > 0){
		
		counterBlobsId = 0;//Manual counter
		sickBlobs.clear();//Will Work? //Check in wich frame was sent to avoid delays?
		//cout << "SensorManager::update OSC => Checking how long take this Timer = " << ofToString(ofGetElapsedTimeMillis(),0) << endl;
	}
	
	// check for waiting messages
	while(receiverExt.hasWaitingMessages()){
		// get the next message
		ofxOscMessage m;
		receiverExt.getNextMessage(m);
		
		//get all sick blobs
		if(m.getAddress() == "/SickBlobs"){
			
			sickData auxSickData;
			
			//int auxId = m.getArgAsFloat(0); //In future
			int auxId = counterBlobsId;
			auxSickData.id = auxId;
			
			ofPoint auxPos;
			auxPos.x = m.getArgAsFloat(0); //x
			auxPos.y = m.getArgAsFloat(1); //y
			auxSickData.pos = auxPos;
			
			sickBlobs.push_back(auxSickData);
			
			//manual counter
			counterBlobsId++;
			
			hasReceivedSomeThing = true;
		}
		
		
	}

	return hasReceivedSomeThing;
}


