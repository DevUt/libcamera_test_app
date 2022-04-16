#include <iostream>
#include <stdlib.h>
#include <iomanip>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>

#include <sys/mman.h>

#include <QApplication>
#include <QImage>
#include <QLabel>

#include <libcamera/libcamera.h>

#define REQUEST_LIST_SIZE 1

using namespace libcamera;

// Main physical camera obj
static std::shared_ptr<libcamera::Camera> camera;

// Shared Request Q
static std::vector<std::unique_ptr<Request>> requestList;

// flag to indicate that threads should now exit
std::atomic_bool quitThread;

// flag to indicate if requestHasCompleted
std::atomic_bool reqCompleted = true;

// Here we would display the stream
static QImage viewfinder;

// The label to hold our viewfinder
static QLabel *viewfinder_label;

/* We use createReq to send 1 request,
 * we don't want it
 */
std::mutex requestLock;

void qReq();

static void requestComplete(Request *request)
{
    std::cout << "Entered requestComplete \n";
    std::unique_lock rlck(requestLock);
    std::cout << "Entered requestComplete and locked\n";
    if (request->status() == Request::RequestCancelled)
        return;

    // Extract the frambuffer filled with images
    auto &buffers = request->buffers();

    // Iterate over the buffer pairs
    // [Stream,FrameBuffer] and
    // print the metadata associated
    for (auto bufferPair : buffers)
    {
        FrameBuffer *buffer = bufferPair.second;
        const FrameMetadata &metadata = buffer->metadata();
        std::cout << " seq: " << std::setw(6) << std::setfill('0') << metadata.sequence << '\n';

        // Find the size of the buffer
        const FrameBuffer::Plane &plane = buffer->planes().front();
        size_t size = buffer->metadata().planes().front().bytesused;
        void *mem = mmap(NULL, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), 0);

        viewfinder.loadFromData((const unsigned char *)mem, int(size));
        viewfinder_label->setPixmap(QPixmap::fromImage(viewfinder));
        viewfinder_label->show();
        munmap(mem, size);
    }
    requestList.pop_back();
    reqCompleted = true;
    // request->reuse(Request::ReuseBuffers); // No reusing as the paramters may change hence create a new request
    // camera->queueRequest(request);
}

void createReq(libcamera::Stream *stream, libcamera::FrameBuffer *buffer)
{
    while (true)
    {
        std::cout << "Entered createReq\n";
        if (quitThread)
            return;
        std::unique_lock lck(requestLock);
        std::cout << "Entered createReq and locked\n";
        if(requestList.size() >= REQUEST_LIST_SIZE){
            std::cout<<"Skipping \n";
            continue;
        }
        std::unique_ptr<Request> request = camera->createRequest();
        if (!request)
        {
            std::cerr << "Couldn't create a request\n";
            exit(-ENOMEM);
        }
        std::cout<<"Created req\n";
        int ret = request->addBuffer(stream, buffer);
        if (ret < 0)
        {
            std::cerr << "Can't set buffer for request\n";
            exit(ret);
        }
        std::cout<<"Added buffer\n";
        requestList.push_back(std::move(request));
        qReq();
    }
}


void qReq()
{

    if (quitThread)
        return;
    std::cout << "Entered qReq\n";
    if (!reqCompleted || requestList.empty())
        return;
    std::cout << "Attempt to Queue request\n";
    camera->queueRequest(requestList.back().get());
    reqCompleted = false;
    std::cout << "Queued request\n";
}

int main(int argc, char *argv[])
{
    QApplication window(argc, argv);
    viewfinder_label = new QLabel;
    // Handles to the CameraManager
    std::unique_ptr<CameraManager> cm = std::make_unique<CameraManager>();

    cm->start();

    // Iterate over the camera Ids
    for (auto const &cam : cm->cameras())
    {
        std::cout << "Camera Id: " << cam->id() << '\n';
    }

    // If we have no camera exit
    if (cm->cameras().empty())
    {
        std::cerr << "No cameras\n";
        cm->stop();
        return EXIT_FAILURE;
    }
    // Select a camera
    camera = cm->cameras()[0];
    std::string camId = camera->id();

    // Get the camera lock
    camera->acquire();
    // Config for the camera
    std::unique_ptr<CameraConfiguration> config = camera->generateConfiguration({StreamRole::Viewfinder});

    // Get selected camera's StreamConfig
    StreamConfiguration &streamConfig = config->at(0);
    std::cout << "The default viewFinder config is: " << streamConfig.toString() << '\n';

    /* At this point we don't deal with invalid configs
     * as we don't make any changes to the config
     * this is just a placeholder so I don't forget to check
     */
    if (config->validate() == CameraConfiguration::Status::Invalid)
    {
        std::cerr << "Invalid camera config\n";
        return EXIT_FAILURE;
    }

    // Set the validated or adjusted config
    camera->configure(config.get());

    FrameBufferAllocator *allocator = new FrameBufferAllocator(camera);

    for (StreamConfiguration &cfg : *config)
    {
        int ret = allocator->allocate(cfg.stream());
        if (ret < 0)
        {
            std::cerr << "Error\n";
            return -ENOMEM;
        }
        size_t allocated_size = allocator->buffers(cfg.stream()).size();
        std::cout << "Allocated " << allocated_size << " buffers for stream\n";
    }
    // Setup a singleStream from the first config
    Stream *stream = streamConfig.stream();
    const auto &buffers = allocator->buffers(stream);
    // std::vector<std::unique_ptr<Request>> requests;

    // Register callback function for request completed signal
    camera->requestCompleted.connect(requestComplete);
    // Q requests to the camera to capture
    // for (unsigned int i = 0; i < buffers.size(); ++i)
    // {
    //     std::unique_ptr<Request> request = camera->createRequest();
    //     if (!request)
    //     {
    //         std::cerr << "Couldn't create a request\n";
    //         return -ENOMEM;
    //     }

    //     const std::unique_ptr<FrameBuffer> &buffer = buffers[i];
    //     int ret = request->addBuffer(stream, buffer.get());
    //     if (ret < 0)
    //     {
    //         std::cerr << "Can't set buffer for request\n";
    //         return ret;
    //     }

    //     requests.push_back(std::move(request));
    // }

    // Finaly start the camera
    camera->start();
    std::thread createReqThread(createReq, stream, buffers.front().get());
    createReqThread.detach();
    // for (std::unique_ptr<libcamera::Request> &request : requests)
    // {
    //     camera->queueRequest(request.get());
    // }
    // Setup the window
    std::cout << "Starting window\n";
    int ret = window.exec();
    std::cout << "Quiting window\n";

    quitThread = true;
    // Clean Up
    camera->stop();
    allocator->free(stream);
    delete allocator;
    camera->release();
    camera.reset();
    cm->stop();
    delete viewfinder_label;
    return ret;
}
