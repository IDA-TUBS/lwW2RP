

#include <thread>
#include <chrono>

#include "writer_test.hpp"
#include <w2rp/log.hpp>

#include "img_config.hpp"

#include <opencv2/opencv.hpp>

using namespace w2rp;



Publisher::Publisher()
{

}

Publisher::~Publisher()
{
    
}

bool Publisher::init(uint16_t participant_id ,std::string cfg, std::string setup)
{
    counterRow = 0;

    config::writerCfg w_config("WRITER_01", cfg, setup);
    
    writer = new Writer(participant_id, w_config);
    // writer = new Writer(participant_id);
    // writer = Writer(participant_id);
    return true;
}


void Publisher::runThread(uint32_t number_samples)
{    
    logInfo("\n[APP] Publisher running.")
    uint32_t i = 0;
    while(true)
    {
        logInfo("\n----------------------------------------------------------------------------------------\n[APP] - Sending sample with index: " << i << "\n----------------------------------------------------------------------------------------\n")
        if (!publish())
        {
            --i;
        }
        else
        {
            // sending worked?!
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << std::endl << std::endl;
    }
    
}


void Publisher::run()
{    
    int number_samples = 100;
    std::thread thread(&Publisher::runThread, this, number_samples);
    
    thread.join();
}


bool Publisher::publish()
{
    cv::Mat img(IMG_ROWS, IMG_COLS, CV_8UC1 , cv::Scalar(0));       
    cv::Scalar line_Color(255);//Color of the line
    cv::Point starting(0, counterRow);//Starting Point of the line
    cv::Point ending(IMG_COLS, counterRow);//Ending Point of the line
    line(img, starting, ending, line_Color, 3);//using line() function to draw the line

    size_t imgSize = img.total() * img.elemSize();
    logDebug("[APP] sending image of size " << imgSize)

    // Allocate memory for the unsigned char array
    unsigned char* imgData = new unsigned char[imgSize];
    memcpy(imgData, img.data, imgSize);


    counterRow++;
    if(counterRow == (IMG_ROWS - 3))
    {
        counterRow = 0;
    }
    
    SerializedPayload *payload = new SerializedPayload(imgData, imgSize);
    // if(writer->write(payload))
    if(writer->write(payload))
    {
        // sample transmitted successfully
        // cv::imshow("Sent Image", img);
        // cv::waitKey(1); // Wait for a key press to close the window
        return true;
    }
    else
    {
        // problem occured
        return false;
    }
    
}


int main()
{
    uint16_t p_id = 0x8517;

    // // Alex Setup
    // std::string cfg_path = std::string(getenv("HOME")) + "/Documents/Code/lightweightW2RP/test/app_videoStream/w2rp_config.json";
    // std::string setup_path = std::string(getenv("HOME")) + "/Documents/Code/lightweightW2RP/test/app_videoStream/setup_defines.json";

    // Daniel Setup
    std::string cfg_path = std::string(getenv("HOME")) + "/Documents/Code/lightweightW2RP/test/app_videoStream/w2rp_config.json";
    std::string setup_path = std::string(getenv("HOME")) + "/Documents/Code/lightweightW2RP/test/app_videoStream/setup_defines.json";
    
    Publisher myPub;
    if (myPub.init(p_id, cfg_path, setup_path))
    {
        myPub.run();
    }

    while(true)
    {

    }

    return 0;
}