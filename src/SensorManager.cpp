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

	jsonParams["sensorParams"]["bArea1"] = ofToString(bArea1);
	jsonParams["sensorParams"]["rectArea1X"] = ofToString(rectArea1.x);
	jsonParams["sensorParams"]["rectArea1Y"] = ofToString(rectArea1.y);
	jsonParams["sensorParams"]["rectArea1W"] = ofToString(rectArea1.width);
	jsonParams["sensorParams"]["rectArea1H"] = ofToString(rectArea1.height);
	

	jsonParams["sensorParams"]["bArea2"] = ofToString(bArea2);
	jsonParams["sensorParams"]["rectArea2X"] = ofToString(rectArea2.x);
	jsonParams["sensorParams"]["rectArea2Y"] = ofToString(rectArea2.y);
	jsonParams["sensorParams"]["rectArea2W"] = ofToString(rectArea2.width);
	jsonParams["sensorParams"]["rectArea2H"] = ofToString(rectArea2.height);

	return jsonParams;
}

//-------------------------------------------------------------------
bool SensorManager::setParams(ofxJSONElement jsonFile)
{
	bool bLoaded = true;

	bArea1 = ofToBool(jsonFile["sensorParams"]["bArea1"].asString());
	rectArea1.x = ofToInt(jsonFile["sensorParams"]["rectArea1X"].asString());
	rectArea1.y = ofToInt(jsonFile["sensorParams"]["rectArea1Y"].asString());
	rectArea1.width = ofToInt(jsonFile["sensorParams"]["rectArea1W"].asString());
	rectArea1.height = ofToInt(jsonFile["sensorParams"]["rectArea1H"].asString());

	bArea2 = ofToBool(jsonFile["sensorParams"]["bArea2"].asString());
	rectArea2.x = ofToInt(jsonFile["sensorParams"]["rectArea2X"].asString());
	rectArea2.y = ofToInt(jsonFile["sensorParams"]["rectArea2Y"].asString());
	rectArea2.width = ofToInt(jsonFile["sensorParams"]["rectArea2W"].asString());
	rectArea2.height = ofToInt(jsonFile["sensorParams"]["rectArea2H"].asString());
	
	return bLoaded;
}

//Setters
//-----------------------------------------
void SensorManager::setSensorType(sensorType _type){
	typeSensor =_type;
}
//-----------------------------------------
void  SensorManager::setSensorMode(sensorMode _mode){
	modeSensor = _mode;
}


//-----------------------------------------
void SensorManager::setup(sensorType _sensorType, sensorMode _sensorMode){
	
	typeSensor = _sensorType;
	modeSensor = _sensorMode;
	
	///////////////////////
	//Sensor Configurations
	bool bSensorReady = false;
	
	if (typeSensor == kinectSensor) {
#ifdef USE_SENSOR_KINECT
		bSensorReady = setupKinectSensor();
#endif
	}else if (typeSensor == cameraSensor){
		bSensorReady = setupCameraSensor();
		//Setup CV
		computerVisionSensor1.setup(1, 640, 480); //TODO Change this to JSon params loaded at a config file
		computerVisionSensor2.setup(2, 640, 480); //TODO Change this to JSon params loaded at a config file
		
	}else if(typeSensor == externalSickSensor){
		bSensorReady = setupExternalSickSensor();
	}
	
	if(bSensorReady == false){
		cout << "//*///*/*/*/*/*/*/* Error sensor setup **/*/*/*////**///" << endl;
		ofExit(0);
	}

	//For Computer Vision Setup some general vars
	sensorImage1.allocate(getWidth(), getHeight(), OF_IMAGE_COLOR);
	mySourcedSensorFbo.allocate(getWidth(), getHeight(), GL_RGB);
	//sensorImage2.allocate(cam.getWidth(), cam.getHeight(), OF_IMAGE_COLOR);
	//sourceImageRaw.allocate(cam.getWidth(), cam.getHeight(), OF_IMAGE_COLOR);
	
	ofRegisterKeyEvents(this);
	
}


//-----------------------------------------
void SensorManager::update(){
	
	bNewSensorFrame = false;
	
	if(typeSensor == kinectSensor){

#ifdef USE_SENSOR_KINECT

		kinect.update();
		
		// there is a new frame and we are connected
		if(kinect.isFrameNew()) {
			
			// load grayscale depth image from the kinect source
			computerVisionImage.setFromPixels(kinect.getDepthPixels());
			
			//we do two thresholds - one for the far plane and one for the near plane
			//we do it ourselves - show people how they can work with the pixels
			ofPixels & pix = computerVisionImage.getPixels();
			int numPixels = pix.size();
			for(int i = 0; i < numPixels; i++) {
				if(pix[i] < nearThreshold && pix[i] > farThreshold) {
					pix[i] = 255;
				} else {
					pix[i] = 0;
				}
			}
			
			//Set pixels back to main cv image
			computerVisionImage.setFromPixels(pix);

			// find contours which are between the size of 20 pixels and 1/3 the w*h pixels.
			// also, find holes is set to true so we will get interior contours as well....
			//contourFinder.findContours(grayImage, minSizeBlob, maxSizeBlob, numBlobs, false);
			contourFinder.findContours(computerVisionImage);
			bNewSensorFrame = true;
		}

		

#endif

	}
	else if (typeSensor == cameraSensor){
		
		if(modeSensor == simulationMode){
			videoPlayerCam.update();
			bNewSensorFrame = videoPlayerCam.isFrameNew();
		}
		else if(modeSensor == realTimeMode){
			cam.update();
			bNewSensorFrame = cam.isFrameNew();
		}
		
		if (bArea1)computerVisionSensor1.udpateBackground();
		if (bArea2)computerVisionSensor2.udpateBackground();
		
		if(bNewSensorFrame) {
			
			if (bArea1) {
				updateSourceImageRaw(rectArea1, sensorImage1);
				computerVisionSensor1.mainComputerVision(sensorImage1);
			}
			
			if (bArea2) {
				/*WIP*/
				updateSourceImageRaw(rectArea2, sensorImage2);
				computerVisionSensor2.mainComputerVision(sensorImage2);
			}
		}
		
	}
	else if (typeSensor == externalSickSensor){
		
		//This updates osc data that contains all Blobs
		bNewSensorFrame = updateExternalSickSensor();

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
void SensorManager::updateSourceImageRaw(ofRectangle _rectArea, ofImage &image2Update) {
	//get Pixels from sensor ( VideoPlayer or VideoCamera )
	if (modeSensor == simulationMode) {
		image2Update.setFromPixels(videoPlayerCam.getPixelsRef().getPixels(), videoPlayerCam.getWidth(), videoPlayerCam.getHeight(), OF_IMAGE_COLOR);
	}
	else if (modeSensor == realTimeMode) {
		image2Update.setFromPixels(cam.getPixelsRef().getPixels(), videoPlayerCam.getWidth(), videoPlayerCam.getHeight(), OF_IMAGE_COLOR);
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

	//TODO , Contour Finder not drawing now, and Second Image paint in red.

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
		

		// draw from the live kinect
		kinect.drawDepth(marginDraw, marginDraw, kinect.width*sensorDrawScale, kinect.height*sensorDrawScale);
		kinect.draw(2*kinect.width*sensorDrawScale, marginDraw, kinect.width*sensorDrawScale, kinect.height*sensorDrawScale);
			
		computerVisionImage.draw(marginDraw, sensorWidth*sensorDrawScale, kinect.width*sensorDrawScale, kinect.height*sensorDrawScale);
		
		ofSetColor(255, 0, 0);
		ofPushMatrix();
		ofTranslate(marginDraw, sensorWidth*sensorDrawScale);
		ofScale(sensorDrawScale, sensorDrawScale);
		contourFinder.draw();
		ofPopMatrix();

		
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

		//--------------------------------------
		//Draw Raw Sensor images
		//if (modeSensor == realTimeMode) cam.draw(marginDraw, marginDraw, sensorWidth*sensorDrawScale, sensorHeight*sensorDrawScale);
		//if (modeSensor == simulationMode && bArea1) computerVisionSensor1.computerVisionImage.draw(marginDraw, marginDraw, sensorWidth*sensorDrawScale, sensorHeight*sensorDrawScale);
		//if (modeSensor == simulationMode && bArea2) computerVisionSensor2.computerVisionImage.draw(marginDraw, marginDraw, sensorWidth*sensorDrawScale, sensorHeight*sensorDrawScale);


		if (bArea1) {
			computerVisionSensor1.computerVisionImage.draw(marginDraw, marginDraw, sensorWidth*sensorDrawScale, sensorHeight*sensorDrawScale);
			computerVisionSensor1.draw(sensorDrawScale, marginDraw);
		}
		if (bArea2) {
			//TODO draw this in a different place
			computerVisionSensor2.computerVisionImage.draw(marginDraw, marginDraw + sensorHeight*sensorDrawScale, sensorWidth*sensorDrawScale, sensorHeight*sensorDrawScale);
			computerVisionSensor2.draw(sensorDrawScale, marginDraw + sensorHeight*sensorDrawScale);
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

	//last Draw if camera options for filtering image
	if (typeSensor == cameraSensor) {
		drawAreaRectangle(rectArea1, 1);
		drawAreaRectangle(rectArea2, 2);
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
		ImGui::SliderInt("nearThreshold", &nearThreshold, 0, 255);
		ImGui::SliderInt("farThreshold", &farThreshold, 0, 255);
		ImGui::Separator();
		//TODO missing this property in ofxCv
		//ImGui::SliderInt("numBlobs ", &numBlobs, 1, 20);
		ImGui::SliderInt("accuracyMaxSizeBlob", &maxBlobsAccuracyMaxValue, 0, sensorWidth *sensorHeight*sensorDrawScale);
		
		if(ImGui::SliderInt("minBlobs ", &minSizeBlob, 5, maxBlobsAccuracyMaxValue)){
			contourFinder.setMinAreaRadius(minSizeBlob);
		}
		
		
		if(ImGui::SliderInt("maxBlobs ", &maxSizeBlob, marginDraw, maxBlobsAccuracyMaxValue)){
			contourFinder.setMaxAreaRadius(maxSizeBlob);
		}
		
#endif
		//ImGui::PopItemWidth();
	}else if (typeSensor == cameraSensor){
		sensorTextType = "OF Camera";
		ImGui::Text(sensorTextType.c_str());
		
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

#ifdef OF_VIDEO_CAPTURE_DIRECTSHOW
	#ifdef USE_FULL_DIRECTSHOW_ACCESS
				//WIP. This require public access to VI (Directshow) at OF
				static bool bCameraComposite = false;
				if (ImGui::Checkbox("WIP - RESET TO COMPOSITE", &bCameraComposite)) {
					if (bCameraComposite) {
						shared_ptr<ofDirectShowGrabber> auxGrabber = shared_ptr <OF_VID_GRABBER_TYPE>(new OF_VID_GRABBER_TYPE);
						auxGrabber->VI.setupDevice(selectedCameraIndex, VI_COMPOSITE);
						cam.setGrabber(auxGrabber);
						ofExit();
					}				
				}
	#endif
#endif
			if (ImGui::Button("Reset Camera", ImVec2(150, 20))) {
				resetSimpleSensorCamera();
			}

			if (ImGui::Button("Settings Camera", ImVec2(150, 30))) {
				cam.videoSettings();
			}


		
		}
		else if(modeSensor == simulationMode){
			
			//TODO InputTextFilterCharacter
			static char cmoviePath[60] = "videos/default/"; //This will load fisrt Video available at the this Folder
			ImGui::PushItemWidth(200);
			ImGui::InputText("Videos Path", cmoviePath, IM_ARRAYSIZE(cmoviePath));
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

		

		if (ImGui::CollapsingHeader("Area Sensor 1")) {
			ImGui::Checkbox("Active Area 1", &bArea1);
			ImGui::PushItemWidth(100);
			ImGui::SliderFloat("X Rect Sensor 1 ", &rectArea1.x, 0, getWidth());//TODO limit this until (getWidth() -rectArea1.width). Check how to do it dynamic slider ImGui
			ImGui::SliderFloat("Y Rect Sensor 1 ", &rectArea1.y, 0, getHeight());
			ImGui::SliderFloat("W Rect Sensor 1 ", &rectArea1.width, 0, getWidth());
			ImGui::SliderFloat("H Rect Sensor 1 ", &rectArea1.height, 0, getHeight());

			//ImGui::SliderFloat("X posDrawArea1 ", &posDrawArea1.x, 0, getWidth());
			//ImGui::SliderFloat("Y posDrawArea1 ", &posDrawArea1.y, 0, getHeight());
			ImGui::PopItemWidth();
		}

		if (ImGui::CollapsingHeader("Area Sensor 2")) {
			ImGui::Checkbox("Active Area 2", &bArea2);
			ImGui::PushItemWidth(100);
			ImGui::SliderFloat("X Rect Sensor 2 ", &rectArea2.x, 0, getWidth());
			ImGui::SliderFloat("Y Rect Sensor 2 ", &rectArea2.y, 0, getHeight());
			ImGui::SliderFloat("W Rect Sensor 2 ", &rectArea2.width, 0, getWidth());
			ImGui::SliderFloat("H Rect Sensor 2 ", &rectArea2.height, 0, getHeight());

			//ImGui::SliderFloat("X posDrawArea2 ", &posDrawArea2.x, 0, getWidth());
			//ImGui::SliderFloat("Y posDrawArea2 ", &posDrawArea2.y, 0, getHeight());
			ImGui::PopItemWidth();
		}
		

		if(bArea1)computerVisionSensor1.drawGui();
		if(bArea2)computerVisionSensor2.drawGui();
		
	}
	else if(typeSensor == externalSickSensor){
		string auxmessage = "Sick received in OSC at PORT " + ofToString(PortRecvExt, 0);
		ImGui::Text(auxmessage.c_str());
	}
	else{
		cout << "SensorManager::Error Set Sensor Mode Type" << endl;
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
		cam.setup(640, 480);

		rectArea1.set(0, 0, cam.getWidth(), cam.getHeight());

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
		
	}else if(modeSensor == simulationMode){
		
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
	
	
	//Computer Vision Stuff
	//Main Image used to Cv
	computerVisionImage.allocate(kinect.width, kinect.height, OF_IMAGE_GRAYSCALE);
	//filter minSizeBlob, maxSizeBlob, numBlobs
	contourFinder.setMinAreaRadius(minSizeBlob);
	contourFinder.setMaxAreaRadius(maxSizeBlob);
	contourFinder.setThreshold(numBlobs);
	
	///////////////////////////////////
	//General SensorData for others
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


