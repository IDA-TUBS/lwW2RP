

#include <thread>
#include <chrono>

#include "writer_test.hpp"
#include <w2rp/log.hpp>

#include "img_config.hpp"

#include <opencv2/opencv.hpp>

using namespace w2rp;

cv::Mat idaLogo;

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
    // cv::Mat img(IMG_ROWS, IMG_COLS, CV_8UC1 , cv::Scalar(0));       
    // cv::Scalar line_Color(255);//Color of the line
    // cv::Point starting(0, counterRow);//Starting Point of the line
    // cv::Point ending(IMG_COLS, counterRow);//Ending Point of the line
    // line(img, starting, ending, line_Color, 3);//using line() function to draw the line

    // size_t imgSize = img.total() * img.elemSize();
    // logDebug("[APP] sending image of size " << imgSize)

    // // Allocate memory for the unsigned char array
    // unsigned char* imgData = new unsigned char[imgSize];
    // memcpy(imgData, img.data, imgSize);


    // counterRow++;
    // if(counterRow == (IMG_ROWS - 3))
    // {
    //     counterRow = 0;
    // }

    // // Shift the image downward by one row (wrap around)
    // cv::Mat shifted_img = idaLogo.clone(); // Create a copy to modify

    // // Move the bottom row to the top
    // shifted_img.rowRange(1, shifted_img.rows).copyTo(shifted_img.rowRange(0, shifted_img.rows - 1));
    // shifted_img.row(shifted_img.rows - 1).setTo(cv::Scalar(0)); // Set the last row to black
    cv::Mat shifted_img(idaLogo.size(), idaLogo.type()); // Create a matrix to store the shifted image

    // Move the image down: copy the last row to the first row
    idaLogo.rowRange(idaLogo.rows - 1, idaLogo.rows).copyTo(shifted_img.rowRange(0, 1)); // Copy the last row to the first row
    idaLogo.rowRange(0, idaLogo.rows - 1).copyTo(shifted_img.rowRange(1, idaLogo.rows)); // Copy the rest down by one row

    idaLogo = shifted_img;
    size_t imgSize = idaLogo.total() * idaLogo.elemSize();
    unsigned char* imgData = new unsigned char[imgSize];
    memcpy(imgData, idaLogo.data, imgSize);

    SerializedPayload *payload = new SerializedPayload(imgData, imgSize);
    // if(writer->write(payload))
    if(writer->write(payload))
    {
        // sample transmitted successfully
        cv::imshow("Sent Image", idaLogo);
        // cv::waitKey(1); // Wait for a key press to close the window
        // return true;
    }
    else
    {
        // problem occured
        return false;
    }
    
}

std::string generateTimestamp() {
    // Get the current time
    std::time_t now = std::time(nullptr);
    
    // Convert to a tm structure
    std::tm* localTime = std::localtime(&now);
    
    // Create a string stream to format the date and time
    std::stringstream ss;
    ss << std::put_time(localTime, "%Y%m%d_%H%M%S");

    // Return the formatted string
    return ss.str();
}

int main()
{

    w2rp::init_console_log();
    std::string log_path = "/lightweightW2RP/test/logs/writer_test_video_";
    w2rp::init_file_log(log_path, generateTimestamp());

    idaLogo = cv::imread(std::string(getenv("HOME")) + "/Documents/Code/lightweightW2RP/test/unit_test/app_videoStream/IDA_Logo_Icon_Only.png");
    if (idaLogo.empty()) {
        std::cout << "Error loading image!" << std::endl;
        return -1;
    }
    // cv::imshow("Image", idaLogo);
    // cv::waitKey(1);

    cv::cvtColor(idaLogo, idaLogo, cv::COLOR_BGR2GRAY); // Convert to grayscale
    cv::bitwise_not(idaLogo, idaLogo); // Invert the image

    cv::resize(idaLogo, idaLogo, cv::Size(IMG_COLS, IMG_ROWS));

    uint16_t p_id = 0x8517;

    // // Alex Setup
    std::string cfg_path = std::string(getenv("HOME")) + "/Documents/Code/lightweightW2RP/test/unit_test/app_videoStream/w2rp_config.json";
    std::string setup_path = std::string(getenv("HOME")) + "/Documents/Code/lightweightW2RP/test/unit_test/app_videoStream/setup_defines.json";

    // Daniel Setup
    // std::string cfg_path = std::string(getenv("HOME")) + "/lightweightW2RP/test/unit_test/app_videoStream/w2rp_config.json";
    // std::string setup_path = std::string(getenv("HOME")) + "/lightweightW2RP/test/unit_test/app_videoStream/setup_defines.json";
    
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