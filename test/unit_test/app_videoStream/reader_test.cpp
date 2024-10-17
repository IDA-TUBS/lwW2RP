

#include <thread>
#include <chrono>

#include "reader_test.hpp"
#include <w2rp/log.hpp>

#include "img_config.hpp"

#include <opencv2/opencv.hpp>
 

using namespace w2rp;

Subscriber::Subscriber()
{
    logInfo("[APP] App started")
}

Subscriber::~Subscriber()
{
    
}
 
bool Subscriber::init(uint16_t participant_id ,std::string cfg, std::string setup)
{
    config::readerCfg r_config("READER_01", cfg, setup);

    reader = new Reader(participant_id, r_config);
    return true;
}

struct Subscriber::visualizeData
{
    cv::Mat img;
    
    visualizeData(cv::Mat img_):img(img_){}
    void operator()()
    {
        cv::imshow("sub", img);
        cv::waitKey(1);
    }
};


void Subscriber::rxThread()
{    
    logInfo("[APP] Subscriber running.")

    w2rp::Reader::sample payload;
    while (true)
    {
        reader->retrieveSample(payload);
        logInfo("\n----------------------------------------------------------------------------------------\n[APP] Received sample\n---------------------------------------------------------\n");
    
        
        unsigned char* imgData = new unsigned char[payload.second.length];
        memcpy(imgData, payload.second.data, payload.second.length);

        // Create a cv::Mat from the unsigned char array
        cv::Mat img(IMG_ROWS, IMG_COLS, CV_8UC1, imgData);

        // cv::imshow("Recieved Image", img);
        // cv::waitKey(1); // Wait for a key press to close the window
        visualizeData visualizer(img);
        std::thread visualizerThread(visualizer);
        visualizerThread.detach();

        
    }   
}



void Subscriber::run()
{    
    std::thread thread(&Subscriber::rxThread, this);
    
    thread.join();
}

int main()
{
    uint16_t p_id = 0x5340;
    
    // Alex' Setup
        std::string cfg_path = std::string(getenv("HOME")) + "/Documents/Code/lightweightW2RP/test/unit_test/app_videoStream/w2rp_config.json";
    std::string setup_path = std::string(getenv("HOME")) + "/Documents/Code/lightweightW2RP/test/unit_test/app_videoStream/setup_defines.json";

    // Daniel's Setup
    // std::string cfg_path = std::string(getenv("HOME")) + "/lightweightW2RP/test/unit_test/app_videoStream/w2rp_config.json";
    // std::string setup_path = std::string(getenv("HOME")) + "/lightweightW2RP/test/unit_test/app_videoStream/setup_defines.json";


    Subscriber mySub;
    if (mySub.init(p_id, cfg_path, setup_path))
    {
        mySub.run();
    }

    while(true)
    {

    }

    return 0;
}