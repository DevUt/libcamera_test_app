#include <mutex>

#include <QMainWindow>
#include <QImage>
#include <QLabel>
#include <QSlider>

#include <libcamera/libcamera.h>

#define REQUEST_LIST_SIZE 1

using namespace libcamera;


// Main physical camera obj
static std::shared_ptr<libcamera::Camera> camera;

// Shared Request Q
static std::vector<std::unique_ptr<Request>> requestList;

// Here we would display the stream
static QImage viewfinder;

// The label to hold our viewfinder
static QLabel *viewfinder_label;

// Our brightness Slider
static QSlider *slider;

//Helper to q Requests
void qReq();

//requestCallback handler
static void requestComplete(Request *request);

//request Ceator
void createReq(libcamera::Stream *stream, libcamera::FrameBuffer *buffer);