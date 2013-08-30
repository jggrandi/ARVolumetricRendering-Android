
/*==============================================================================
Copyright (c) 2010-2013 QUALCOMM Austria Research Center GmbH.
All Rights Reserved.

@file 
    LivAR.cpp

@brief
    Sample for LivAR

==============================================================================*/


#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <sys/time.h>

#ifdef USE_OPENGL_ES_1_1
#include <GLES/gl.h>
#include <GLES/glext.h>
#else
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif

 #include <sys/types.h>
 #include <android/asset_manager.h>
 #include <android/asset_manager_jni.h>

#include <QCAR/QCAR.h>
#include <QCAR/CameraDevice.h>
#include <QCAR/Renderer.h>
#include <QCAR/VideoBackgroundConfig.h>
#include <QCAR/Trackable.h>
#include <QCAR/TrackableResult.h>
#include <QCAR/Tool.h>
#include <QCAR/Tracker.h>
#include <QCAR/TrackerManager.h>
#include <QCAR/ImageTracker.h>
#include <QCAR/CameraCalibration.h>
#include <QCAR/UpdateCallback.h>
#include <QCAR/DataSet.h>


#include "SampleUtils.h"
#include "LivAR_Shaders.h"
#include "LookUpTable.h"
#include "wsg.h"

#ifdef __cplusplus
extern "C"
{
#endif


#ifdef USE_OPENGL_ES_2_0
unsigned int shaderProgramID    = 0;
GLfloat *volumeVertices;
GLuint textureLookUp;
GLuint textureId;
GLuint uniformLookUp;
GLuint uniformId;
GLuint touch=0;
#endif



float timeCounter, lastFrameTimeCounter, DT, prevTime = 0.0, FPS;
int         frame = 1, framesPerFPS;
struct timeval tv;
struct timeval tv0;
// Screen dimensions:
unsigned int screenWidth        = 0;
unsigned int screenHeight       = 0;

// Indicates whether screen is in portrait (true) or landscape (false) mode
bool isActivityInPortraitMode   = false;

// The projection matrix used for rendering virtual objects:
QCAR::Matrix44F projectionMatrix;

// Constants:
static const float kObjectScale = 300.f;

QCAR::DataSet* livAR_QRCode    = 0;

bool switchDataSetAsap          = false;

void InitTimeCounter() {
 gettimeofday(&tv0, NULL);
 framesPerFPS = 10; }

void UpdateTimeCounter() {
 lastFrameTimeCounter = timeCounter;
 gettimeofday(&tv, NULL);
 timeCounter = (float)(tv.tv_sec-tv0.tv_sec) + 0.000001*((float)(tv.tv_usec-tv0.tv_usec));
 DT = timeCounter - lastFrameTimeCounter; }

void CalculateFPS() {
 frame ++;

 if((frame%framesPerFPS) == 0) {
    FPS = ((float)(framesPerFPS)) / (timeCounter-prevTime);
    prevTime = timeCounter; } }

bool frontCamera = true;
bool cameraChanged=false;

GLuint loadRAWTexture(int inWidth, int inHeight, int inDepth) {
    
    short **imageData;
    FILE* inFile;
    int width = inWidth;
    int height = inHeight;
    int depth = inDepth;
    int widthxheight = width*height;

    //allocate memory for the image matrix
    imageData = (short**)malloc((depth) * sizeof(short*));

    for (int i=0; i < depth; i++)
        imageData[i] = (short*)malloc(sizeof(short) * width * height);
    
    //inFileName
    if( inFile = fopen("sdcard/body01.raw", "r" )  ){
        
        // read file into image matrix
        for( int i = 0; i < depth; i++ ){
            for( int j = 0; j < widthxheight; j++ ){
                short value;
                fread( &value, 1, sizeof(short), inFile );
                imageData[i][j] = value;
                //LOG("short data:%d",value);
            }
        }
        fclose(inFile);
        LOG("+++ Volume successfully loaded ");
    }else 
    {
        LOG("Error when loading volume");
        abort();
    }

    float *textureData;
    int supportedSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &supportedSize);
//  short *textureData;
    int texWidth = supportedSize;
    int texHeight = supportedSize;

//  int texWidth = 512;
//  int texHeight = 512;


    // allocate memory for the image matrix
    textureData = (float*)malloc(sizeof(float) * texWidth * texHeight);
//  textureData = (short*)malloc(sizeof(short) * texWidth * texHeight);
    float minValue = -1024.0;
    float maxValue = 3071.0;

    for (int i=0; i< texHeight * texWidth ; i++){
        textureData[i]=0.0f;
    }

    int octaveSupSize = supportedSize/8;
    int stepSize = (width*8)/supportedSize;
    for( int i = 0; i < depth; i++ ){
        for( int j = 0; j < widthxheight; j+=stepSize ){
            //int j2 = j/2;
            //int coord = ((i/8)*256+((j/512)/2))*2048 + (i%8)*256 + ((j%512)/2);
            int coord = ((i/8)*octaveSupSize+(j/width)/stepSize)*supportedSize + (i%8)*octaveSupSize + ((j%width)/stepSize);
            textureData[coord] = ((float)imageData[i][j] - minValue)/(maxValue-minValue);
            if (textureData[coord] < 0.0f)
                textureData[coord] = 0.0f;
            if (textureData[coord] > 1.0f )
                textureData[coord] = 1.0f;
        }
    }


    glEnable(GL_TEXTURE_2D);
    //glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);  
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
    LOG("1");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, texWidth, texHeight, 0, GL_LUMINANCE, GL_FLOAT, textureData);
    LOG("2");
    glGenerateMipmap(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_2D);
    return textureID;
}

static void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);

}

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {

    }
}

 



GLuint loadShader(GLenum shaderType, const char* pSource) 
{
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOG("Could not compile shader %d:\n%s\n",shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        LOG("Vertex Error");
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        LOG("Fragment Error");
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LOG("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}




// Object to receive update callbacks from QCAR SDK
class LivAR_UpdateCallback : public QCAR::UpdateCallback
{   
    virtual void QCAR_onUpdate(QCAR::State& /*state*/)
    {
    //     if (switchDataSetAsap)
    //     {
    //         switchDataSetAsap = false;

    //         // Get the image tracker:
    //         QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    //         QCAR::ImageTracker* imageTracker = static_cast<QCAR::ImageTracker*>(
    //             trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER));
    //         if (imageTracker == 0 || dataSetStonesAndChips == 0 || dataSetTarmac == 0 ||
    //             imageTracker->getActiveDataSet() == 0)
    //         {
    //             LOG("Failed to switch data set.");
    //             return;
    //         }
            
    //         if (imageTracker->getActiveDataSet() == dataSetStonesAndChips)
    //         {
    //             imageTracker->deactivateDataSet(dataSetStonesAndChips);
    //             imageTracker->activateDataSet(dataSetTarmac);
    //         }
    //         else
    //         {
    //             imageTracker->deactivateDataSet(dataSetTarmac);
    //             imageTracker->activateDataSet(dataSetStonesAndChips);
    //         }
    //     }
     }
};

LivAR_UpdateCallback updateCallback;

JNIEXPORT int JNICALL
Java_com_qualcomm_volumeRendering_LivAR_LivAR_getOpenGlEsVersionNative(JNIEnv *, jobject)
{
#ifdef USE_OPENGL_ES_1_1        
    return 1;
#else
    return 2;
#endif
}


JNIEXPORT void JNICALL
Java_com_qualcomm_volumeRendering_LivAR_LivAR_setActivityPortraitMode(JNIEnv *, jobject, jboolean isPortrait)
{
    isActivityInPortraitMode = isPortrait;
}



JNIEXPORT int JNICALL
Java_com_qualcomm_volumeRendering_LivAR_LivAR_initTracker(JNIEnv *, jobject)
{
    LOG("Java_com_qualcomm_volumeRendering_LivAR_LivAR_initTracker");
    
    // Initialize the image tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::Tracker* tracker = trackerManager.initTracker(QCAR::Tracker::IMAGE_TRACKER);
    if (tracker == NULL)
    {
        LOG("Failed to initialize ImageTracker.");
        return 0;
    }

    LOG("Successfully initialized ImageTracker.");
    return 1;
}


JNIEXPORT void JNICALL
Java_com_qualcomm_volumeRendering_LivAR_LivAR_deinitTracker(JNIEnv *, jobject)
{
    LOG("Java_com_qualcomm_volumeRendering_LivAR_LivAR_deinitTracker");

    // Deinit the image tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    trackerManager.deinitTracker(QCAR::Tracker::IMAGE_TRACKER);
}


JNIEXPORT int JNICALL
Java_com_qualcomm_volumeRendering_LivAR_LivAR_loadTrackerData(JNIEnv *, jobject)
{
    LOG("Java_com_qualcomm_volumeRendering_LivAR_LivAR_loadTrackerData");
    
    // Get the image tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::ImageTracker* imageTracker = static_cast<QCAR::ImageTracker*>(
                    trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER));
    if (imageTracker == NULL)
    {
        LOG("Failed to load tracking data set because the ImageTracker has not"
            " been initialized.");
        return 0;
    }

    // Create the data sets:
    livAR_QRCode = imageTracker->createDataSet();
    if (livAR_QRCode == 0)
    {
        LOG("Failed to create a new tracking data.");
        return 0;
    }

    // Load the data sets:
    if (!livAR_QRCode->load("qrcode_logo.xml", QCAR::DataSet::STORAGE_APPRESOURCE))
    {
        LOG("Failed to load data set.");
        return 0;
    }

    // Activate the data set:
    if (!imageTracker->activateDataSet(livAR_QRCode))
    {
        LOG("Failed to activate data set.");
        return 0;
    }

    LOG("Successfully loaded and activated data set.");
    return 1;
}


JNIEXPORT int JNICALL
Java_com_qualcomm_volumeRendering_LivAR_LivAR_destroyTrackerData(JNIEnv *, jobject)
{
    LOG("Java_com_qualcomm_volumeRendering_LivAR_LivAR_destroyTrackerData");

    // Get the image tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::ImageTracker* imageTracker = static_cast<QCAR::ImageTracker*>(
        trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER));
    if (imageTracker == NULL)
    {
        LOG("Failed to destroy the tracking data set because the ImageTracker has not"
            " been initialized.");
        return 0;
    }
    
    if (livAR_QRCode != 0)
    {
        if (imageTracker->getActiveDataSet() == livAR_QRCode &&
            !imageTracker->deactivateDataSet(livAR_QRCode))
        {
            LOG("Failed to destroy the tracking data set StonesAndChips because the data set "
                "could not be deactivated.");
            return 0;
        }

        if (!imageTracker->destroyDataSet(livAR_QRCode))
        {
            LOG("Failed to destroy the tracking data set StonesAndChips.");
            return 0;
        }

        LOG("Successfully destroyed the data set StonesAndChips.");
        livAR_QRCode = 0;
    }

    return 1;
}


JNIEXPORT void JNICALL
Java_com_qualcomm_volumeRendering_LivAR_LivAR_onQCARInitializedNative(JNIEnv *, jobject)
{
    // Register the update callback where we handle the data set swap:
    QCAR::registerCallback(&updateCallback);

    // Comment in to enable tracking of up to 2 targets simultaneously and
    // split the work over multiple frames:
    // QCAR::setHint(QCAR::HINT_MAX_SIMULTANEOUS_IMAGE_TARGETS, 2);
}

GLuint gProgramLowQualFragShader;
GLuint gProgramMediumQualFragShader;
GLuint gProgramHighQualFragShader;
GLuint gvPositionHandle;

float fpsOld ;
int m_windowCenterOld=0;

JNIEXPORT void JNICALL
Java_com_qualcomm_volumeRendering_LivAR_LivARRenderer_renderFrame(JNIEnv *, jobject)
{

    UpdateTimeCounter();
    CalculateFPS();    
    //LOG("Java_com_qualcomm_volumeRendering_LivAR_GLRenderer_renderFrame");

    // Clear color and depth buffer 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Get the state from QCAR and mark the beginning of a rendering section
    QCAR::State state = QCAR::Renderer::getInstance().begin();

    // Explicitly render the Video Background
    QCAR::Renderer::getInstance().drawVideoBackground();
       

    glEnable(GL_DEPTH_TEST);


    // We must detect if background reflection is active and adjust the culling direction. 
    // If the reflection is active, this means the post matrix has been reflected as well,
    // therefore standard counter clockwise face culling will result in "inside out" models. 
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    if(QCAR::Renderer::getInstance().getVideoBackgroundConfig().mReflection == QCAR::VIDEO_BACKGROUND_REFLECTION_ON)
        glFrontFace(GL_CW);  //Front camera
    else
        glFrontFace(GL_CCW);   //Back camera


    // Did we find any trackables this frame?
    for(int tIdx = 0; tIdx < state.getNumTrackableResults(); tIdx++)
    {
        if(m_windowCenterOld!=m_windowCenter)
        {
            InitializeLookUpTable(m_windowCenter, m_windowWidth);
            textureLookUp=createPreintegrationTable (m_lookUpTable);
        }
        m_windowCenterOld = m_windowCenter;

        // Get the trackable:
        const QCAR::TrackableResult* result = state.getTrackableResult(tIdx);
        const QCAR::Trackable& trackable = result->getTrackable();
        QCAR::Matrix44F modelViewMatrix =
            QCAR::Tool::convertPose2GLMatrix(result->getPose());        

        QCAR::Matrix44F modelViewProjection;
        QCAR::Matrix44F modelViewInverse;
        float scaleFactor = 400; 

        SampleUtils::translatePoseMatrix(0.0f, -scaleFactor/4-20, -scaleFactor/3, &modelViewMatrix.data[0]);
        SampleUtils::scalePoseMatrix(scaleFactor, scaleFactor, scaleFactor, &modelViewMatrix.data[0]);
        SampleUtils::rotatePoseMatrix(180,0,1,0,&modelViewMatrix.data[0]);
        SampleUtils::rotatePoseMatrix(90,1,0,0,&modelViewMatrix.data[0]);
        SampleUtils::multiplyMatrix(&projectionMatrix.data[0], &modelViewMatrix.data[0], &modelViewProjection.data[0]);
        SampleUtils::invMatrix(&modelViewMatrix.data[0], &modelViewInverse.data[0]);

        if(touch==0) {glUseProgram(gProgramLowQualFragShader);checkGlError("glUseProgram");}
        else if(touch==1) {glUseProgram(gProgramMediumQualFragShader);  checkGlError("glUseProgram");}
        else if(touch==2) {glUseProgram(gProgramHighQualFragShader);checkGlError("glUseProgram");}

        //glUniformMatrix4fv(glGetUniformLocation(gProgramHighQualFragShader, "mvp"), 1, GL_FALSE, (const GLfloat*)&modelViewProjection.data[0]);
        //glUniformMatrix4fv(glGetUniformLocation(gProgramHighQualFragShader, "modelViewInverse"), 1, GL_FALSE, (const GLfloat*)&modelViewInverse.data[0]);
        //glUniform1fv(glGetUniformLocation(gProgramHighQualFragShader, "sliceDistance"), 1, (const GLfloat*) &SPACING );


        glUniformMatrix4fv(glGetUniformLocation(gProgramLowQualFragShader, "mvp"), 1, GL_FALSE, (const GLfloat*)&modelViewProjection.data[0]);
        glUniformMatrix4fv(glGetUniformLocation(gProgramLowQualFragShader, "modelViewInverse"), 1, GL_FALSE, (const GLfloat*)&modelViewInverse.data[0]);
        glUniform1fv(glGetUniformLocation(gProgramLowQualFragShader, "sliceDistance"), 1, (const GLfloat*) &SPACING );


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glUniform1i(uniformId, 0); 
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureLookUp);
        glUniform1i(uniformLookUp, 1);
    
        float eyeVector[3];
        eyeVector[0] = modelViewMatrix.data[2];
        eyeVector[1] = modelViewMatrix.data[6];
        eyeVector[2] = modelViewMatrix.data[10];
        //normalize
        float eyeVectorMagnitude = sqrt((eyeVector[0] * eyeVector[0]) + (eyeVector[1] * eyeVector[1]) + (eyeVector[2] * eyeVector[2]));
        eyeVector[0] = eyeVector[0]/eyeVectorMagnitude;
        eyeVector[1] = eyeVector[1]/eyeVectorMagnitude;
        eyeVector[2] = eyeVector[2]/eyeVectorMagnitude;

        plot(eyeVector, &volumeVertices);

        SampleUtils::checkGlError("LivAR renderFrame");

    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    
    if(!(FPS == fpsOld))
    {
        LOG("FPS:%f",FPS);
    }
    fpsOld = FPS;

    QCAR::Renderer::getInstance().end();
}


void
configureVideoBackground()
{
    // Get the default video mode:
    QCAR::CameraDevice& cameraDevice = QCAR::CameraDevice::getInstance();
    QCAR::VideoMode videoMode = cameraDevice.
                                getVideoMode(QCAR::CameraDevice::MODE_DEFAULT);


    // Configure the video background
    QCAR::VideoBackgroundConfig config;
    config.mEnabled = true;
    config.mSynchronous = true;
    config.mPosition.data[0] = 0.0f;
    config.mPosition.data[1] = 0.0f;
    
    if (isActivityInPortraitMode)
    {
        //LOG("configureVideoBackground PORTRAIT");
        config.mSize.data[0] = videoMode.mHeight
                                * (screenHeight / (float)videoMode.mWidth);
        config.mSize.data[1] = screenHeight;

        if(config.mSize.data[0] < screenWidth)
        {
            LOG("Correcting rendering background size to handle missmatch between screen and video aspect ratios.");
            config.mSize.data[0] = screenWidth;
            config.mSize.data[1] = screenWidth * 
                              (videoMode.mWidth / (float)videoMode.mHeight);
        }
    }
    else
    {
        //LOG("configureVideoBackground LANDSCAPE");
        config.mSize.data[0] = screenWidth;
        config.mSize.data[1] = videoMode.mHeight
                            * (screenWidth / (float)videoMode.mWidth);

        if(config.mSize.data[1] < screenHeight)
        {
            LOG("Correcting rendering background size to handle missmatch between screen and video aspect ratios.");
            config.mSize.data[0] = screenHeight
                                * (videoMode.mWidth / (float)videoMode.mHeight);
            config.mSize.data[1] = screenHeight;
        }
    }

    LOG("Configure Video Background : Video (%d,%d), Screen (%d,%d), mSize (%d,%d)", videoMode.mWidth, videoMode.mHeight, screenWidth, screenHeight, config.mSize.data[0], config.mSize.data[1]);

    // Set the config:
    QCAR::Renderer::getInstance().setVideoBackgroundConfig(config);
}


JNIEXPORT void JNICALL
Java_com_qualcomm_volumeRendering_LivAR_LivAR_initApplicationNative(
                            JNIEnv* env, jobject obj, jint width, jint height)
{
    LOG("Java_com_qualcomm_volumeRendering_LivAR_LivAR_initApplicationNative");
    
    // Store screen dimensions
    screenWidth = width;
    screenHeight = height;
        
    // Handle to the activity class:
    jclass activityClass = env->GetObjectClass(obj);

    LOG("Java_com_qualcomm_volumeRendering_LivAR_LivAR_initApplicationNative finished");
}


JNIEXPORT void JNICALL
Java_com_qualcomm_volumeRendering_LivAR_LivAR_deinitApplicationNative(
                                                        JNIEnv* env, jobject obj)
{
    LOG("Java_com_qualcomm_volumeRendering_LivAR_LivAR_deinitApplicationNative");

    // Release texture resources
    // if (textures != 0)
    // {    
    //     for (int i = 0; i < textureCount; ++i)
    //     {
    //         delete textures[i];
    //         textures[i] = NULL;
    //     }
    
    //     delete[]textures;
    //     textures = NULL;
        
    //     textureCount = 0;
    // }
}


JNIEXPORT void JNICALL
Java_com_qualcomm_volumeRendering_LivAR_LivAR_startCamera(JNIEnv *,
                                                                         jobject)
{
    LOG("Java_com_qualcomm_volumeRendering_LivAR_LivAR_startCamera");
    
    // Select the camera to open, set this to QCAR::CameraDevice::CAMERA_FRONT 
    // to activate the front camera instead.
    QCAR::CameraDevice::CAMERA camera = QCAR::CameraDevice::CAMERA_DEFAULT;

    // Initialize the camera:
    if (!QCAR::CameraDevice::getInstance().init(camera))
        return;

    // Configure the video background
    configureVideoBackground();

    // Select the default mode:
    if (!QCAR::CameraDevice::getInstance().selectVideoMode(
                                QCAR::CameraDevice::MODE_DEFAULT))
        return;

    // Start the camera:
    if (!QCAR::CameraDevice::getInstance().start())
        return;

    // Uncomment to enable flash
    //if(QCAR::CameraDevice::getInstance().setFlashTorchMode(true))
    //    LOG("IMAGE TARGETS : enabled torch");

    // Uncomment to enable infinity focus mode, or any other supported focus mode
    // See CameraDevice.h for supported focus modes
    //if(QCAR::CameraDevice::getInstance().setFocusMode(QCAR::CameraDevice::FOCUS_MODE_INFINITY))
    //    LOG("IMAGE TARGETS : enabled infinity focus");

    // Start the tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::Tracker* imageTracker = trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER);
    if(imageTracker != 0)
        imageTracker->start();
}


JNIEXPORT void JNICALL
Java_com_qualcomm_volumeRendering_LivAR_LivAR_stopCamera(JNIEnv *, jobject)
{
    LOG("Java_com_qualcomm_volumeRendering_LivAR_LivAR_stopCamera");

    // Stop the tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::Tracker* imageTracker = trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER);
    if(imageTracker != 0)
        imageTracker->stop();
    
    QCAR::CameraDevice::getInstance().stop();
    QCAR::CameraDevice::getInstance().deinit();
}


JNIEXPORT void JNICALL
Java_com_qualcomm_volumeRendering_LivAR_LivAR_setProjectionMatrix(JNIEnv *, jobject)
{
    LOG("Java_com_qualcomm_volumeRendering_LivAR_LivAR_setProjectionMatrix");

    // Cache the projection matrix:
    const QCAR::CameraCalibration& cameraCalibration =
                                QCAR::CameraDevice::getInstance().getCameraCalibration();
    projectionMatrix = QCAR::Tool::getProjectionGL(cameraCalibration, 2.0f, 2500.0f);
}

// ----------------------------------------------------------------------------
// Activates Camera Flash
// ----------------------------------------------------------------------------
JNIEXPORT jboolean JNICALL
Java_com_qualcomm_volumeRendering_LivAR_LivAR_activateFlash(JNIEnv*, jobject, jboolean flash)
{
    return QCAR::CameraDevice::getInstance().setFlashTorchMode((flash==JNI_TRUE)) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_qualcomm_volumeRendering_LivAR_LivAR_autofocus(JNIEnv*, jobject)
{
    return QCAR::CameraDevice::getInstance().setFocusMode(QCAR::CameraDevice::FOCUS_MODE_TRIGGERAUTO) ? JNI_TRUE : JNI_FALSE;
}


JNIEXPORT jboolean JNICALL
Java_com_qualcomm_volumeRendering_LivAR_LivAR_setFocusMode(JNIEnv*, jobject, jint mode)
{
    int qcarFocusMode;

    switch ((int)mode)
    {
        case 0:
            qcarFocusMode = QCAR::CameraDevice::FOCUS_MODE_NORMAL;
            break;
        
        case 1:
            qcarFocusMode = QCAR::CameraDevice::FOCUS_MODE_CONTINUOUSAUTO;
            break;
            
        case 2:
            qcarFocusMode = QCAR::CameraDevice::FOCUS_MODE_INFINITY;
            break;
            
        case 3:
            qcarFocusMode = QCAR::CameraDevice::FOCUS_MODE_MACRO;
            break;
    
        default:
            return JNI_FALSE;
    }
    
    return QCAR::CameraDevice::getInstance().setFocusMode(qcarFocusMode) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_qualcomm_volumeRendering_LivAR_LivAR_switchCameras(JNIEnv*, jobject, jboolean cam)
{
    //by Jeronimo

    LOG("Java_com_qualcomm_volumeRendering_LivAR_LivAR_switchCamera");
    QCAR::CameraDevice::CAMERA camera;
    if((int)cam==0)
        camera = QCAR::CameraDevice::CAMERA_BACK;
    else if ((int)cam==1)    
        camera = QCAR::CameraDevice::CAMERA_FRONT;


    QCAR::CameraDevice::getInstance().stop();
    
    QCAR::CameraDevice::getInstance().init(camera);

    configureVideoBackground();

    // Select the default mode:
    QCAR::CameraDevice::getInstance().selectVideoMode(QCAR::CameraDevice::MODE_DEFAULT);
        

    // Start the camera:
    QCAR::CameraDevice::getInstance().start();

    // Start the tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::Tracker* imageTracker = trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER);
    if(imageTracker != 0)
        imageTracker->start();

}



JNIEXPORT void JNICALL
Java_com_qualcomm_volumeRendering_LivAR_LivARRenderer_initRendering(
                                                    JNIEnv* env, jobject obj, jobject assetManager,jstring filename)
{
    LOG("Java_com_qualcomm_volumeRendering_LivAR_LivARRenderer_initRendering");
    InitTimeCounter();
    // Define clear color
    glClearColor(0.0f, 0.0f, 0.0f, QCAR::requiresAlpha() ? 0.0f : 1.0f);
    

    gProgramLowQualFragShader = createProgram(gVertexShader, gLowQualFragShader);
    gProgramMediumQualFragShader = createProgram(gVertexShader, gMediumQualFragShader);
    gProgramHighQualFragShader = createProgram(gVertexShader, gHighQualFragShader);

    vertexHandle = glGetAttribLocation(gProgramLowQualFragShader,"vertexPos");
    textureCoordHandle  = glGetAttribLocation(gProgramLowQualFragShader, "texCoord");
    //vertexHandle = glGetAttribLocation(gProgramHighQualFragShader,"vertexPos");
    //textureCoordHandle  = glGetAttribLocation(gProgramHighQualFragShader, "texCoord");

    if(!gProgramLowQualFragShader){LOG("Could not create LowQ Program");return ;}
    if(!gProgramMediumQualFragShader){LOG("Could not create MedQ Program");return ;}
    if(!gProgramHighQualFragShader){LOG("Could not create HighQ Program");return ;}

    InitializeLookUpTable(m_windowCenter, m_windowWidth);
    textureLookUp=createPreintegrationTable (m_lookUpTable);
    textureId=loadRAWTexture(512,512,43); //original
    //textureId=loadRAWTexture(512,512,13); //mod

    uniformLookUp = glGetUniformLocation(gProgramLowQualFragShader, "lookupTableSampler");
    uniformId = glGetUniformLocation(gProgramLowQualFragShader, "volumeSampler");    
    //uniformLookUp = glGetUniformLocation(gProgramHighQualFragShader, "lookupTableSampler");
    //uniformId = glGetUniformLocation(gProgramHighQualFragShader, "volumeSampler");

    checkGlError("InitRendering");

}


JNIEXPORT void JNICALL
Java_com_qualcomm_volumeRendering_LivAR_LivARRenderer_updateRendering(
                        JNIEnv* env, jobject obj, jint width, jint height)
{
    LOG("Java_com_qualcomm_volumeRendering_LivAR_LivARRenderer_updateRendering");

    // Update screen dimensions
    screenWidth = width;
    screenHeight = height;

    // Reconfigure the video background
    configureVideoBackground();
}

bool touchActive=false;
bool moveActive=false;
float x_old=0;
JNIEXPORT void JNICALL
Java_com_qualcomm_volumeRendering_LivAR_LivAR_nativeTouchEvent(
                        JNIEnv* env, jobject obj, jint actionType, jint pointerId, jfloat x, jfloat y)
{   
    switch(actionType)
    {
        case 0:
            if(!touchActive)
            {
                touchActive=true;
                x_old=x;
                //LOG("TOUCH!,%d, %f, %f, %f, %f", pointerId,x,y,x_old);
            }
        break;
        case 1:
            if(touchActive)
                touchActive=false;
            moveActive=true;
            if(x_old>x)
            {
                m_windowCenter=m_windowCenter+3;
                if(m_windowCenter>256)
                    m_windowCenter=256;
                //LOG("x_old>x %d", m_windowCenter);
            }
            else if(x_old<x)
            {
                //LOG("x_old<x %d", m_windowCenter);
                m_windowCenter=m_windowCenter-3;
                if(m_windowCenter<-256)
                    m_windowCenter=-256;
            }
            else 
            {
                //LOG("==");
            }
            x_old=x;
            //LOG("MOVE!,%d, %f, %f, %f, %f", pointerId,x,y,x_old);     
        break;
        case 2:
            if(touchActive)
            {
                //LOG("UP TOUCH!,%d, %f, %f", pointerId,x,y);       
                if(touch<2)
                {
                    touch++;
                    if(touch==0)LOG("++LOWQ");
                    else if(touch==1)LOG("++MEDQ");
                    else if(touch==2)LOG("++HIGHQ");
                }
                else
                    touch=0;
                touchActive=false;
            }
            else if(moveActive)
            {
                moveActive=false;
                //LOG("UP MOVE!,%d, %f, %f", pointerId,x,y);        
            }
            x_old=x;
        break;
        case 3:
        
        break;
        default:
        break;
    }
}


#ifdef __cplusplus
}
#endif
