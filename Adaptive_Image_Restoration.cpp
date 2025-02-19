#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/photo.hpp>
#include <experimental/filesystem>
#include <math.h> 
#include <iostream>

using namespace cv;
using namespace cv::dnn;
using namespace std;
namespace fs = std::experimental::filesystem;
cv::Mat red_channel_compensation(const cv::Mat& image) {
    std::vector<cv::Mat> channels;
    cv::split(image, channels);

    cv::Mat b = channels[0];
    cv::Mat g = channels[1];
    cv::Mat r = channels[2];

    cv::Scalar mean_r = cv::mean(r);
    cv::Scalar mean_g = cv::mean(g);
    double alpha = 0.3;

    cv::Mat r_compensated = r + alpha * (mean_g[0] - mean_r[0]);

    // Clip values to ensure they are within the valid range
    cv::Mat r_compensated_clipped;
    r_compensated.convertTo(r_compensated_clipped, CV_8UC1);
    cv::threshold(r_compensated_clipped, r_compensated_clipped, 255, 255, cv::THRESH_TRUNC);

    channels[2] = r_compensated_clipped;

    cv::Mat result;
    cv::merge(channels, result);

    return result;
}
cv::Mat blue_channel_compensation(const cv::Mat& image) {
    std::vector<cv::Mat> channels;
    cv::split(image, channels);

    cv::Mat b = channels[0];
    cv::Mat g = channels[1];
    cv::Mat r = channels[2];

    cv::Scalar mean_b = cv::mean(b);
    cv::Scalar mean_g = cv::mean(g);
    double alpha = 0.3;

    cv::Mat b_compensated = b + alpha * (mean_g[0] - mean_b[0]);

    // Clip values to ensure they are within the valid range
    cv::Mat b_compensated_clipped;
    b_compensated.convertTo(b_compensated_clipped, CV_8UC1);
    cv::threshold(b_compensated_clipped, b_compensated_clipped, 255, 255, cv::THRESH_TRUNC);

    channels[0] = b_compensated_clipped;

    cv::Mat result;
    cv::merge(channels, result);

    return result;
}

cv::Mat green_channel_compensation(const cv::Mat& image) {
    std::vector<cv::Mat> channels;
    cv::split(image, channels);

    cv::Mat b = channels[0];
    cv::Mat g = channels[1];
    cv::Mat r = channels[2];

    cv::Scalar mean_r = cv::mean(r);
    cv::Scalar mean_b = cv::mean(b);
    double alpha = 0.3;

    cv::Mat g_compensated = g + alpha * (mean_b[0] - mean_r[0]);

    // Clip values to ensure they are within the valid range
    cv::Mat g_compensated_clipped;
    g_compensated.convertTo(g_compensated_clipped, CV_8UC1);
    cv::threshold(g_compensated_clipped, g_compensated_clipped, 255, 255, cv::THRESH_TRUNC);

    channels[1] = g_compensated_clipped;

    cv::Mat result;
    cv::merge(channels, result);

    return result;
}
Mat white_balance(Mat img, float perc = 0.007)
{
    CV_Assert(img.data);

    // Accept only char  colour type matrices
    CV_Assert(img.depth() != sizeof(uchar));
    // CV_Assert(img.channels() != 3);


    // Compute the histograms of the three colour channels
    int histSize = 256;

    // Set the range
    float range[] = { 0, 256 };
    const float* histRange = { range };

    bool uniform = true; bool accumulate = false;

    Mat r_hist, g_hist, b_hist;

    Mat bgr[3];   //destination array
    split(img, bgr);//split source  

    // Compute the histogram
    calcHist(&bgr[2], 1, 0, Mat(), r_hist, 1, &histSize, &histRange, uniform, accumulate);
    calcHist(&bgr[1], 1, 0, Mat(), g_hist, 1, &histSize, &histRange, uniform, accumulate);
    calcHist(&bgr[0], 1, 0, Mat(), b_hist, 1, &histSize, &histRange, uniform, accumulate);

    Mat r_norm_hist = r_hist / bgr[2].total();
    Mat g_norm_hist = g_hist / bgr[1].total();
    Mat b_norm_hist = b_hist / bgr[0].total();
    



    // Now get the CDF

    std::vector<float> r_cdf;
    std::vector<float> g_cdf;
    std::vector<float> b_cdf;
    for (int i = 0; i < 256; i++)
    {
        if (i == 0)
        {
            r_cdf.push_back((r_norm_hist.at<float>(i, 0)));
            g_cdf.push_back((g_norm_hist.at<float>(i, 0)));
            b_cdf.push_back((b_norm_hist.at<float>(i, 0)));
        }
        else
        {
            r_cdf.push_back((r_norm_hist.at<float>(i, 0) + r_cdf[i-1]) );
            g_cdf.push_back((g_norm_hist.at<float>(i, 0) + g_cdf[i-1]) );
            b_cdf.push_back((b_norm_hist.at<float>(i, 0) + b_cdf[i-1]) );
        }
        
    }




    // Get the min and max pixel values indicating the 5% of pixels at the ends of the histograms
    float r_min_val = -1, g_min_val = -1, b_min_val = -1; // Dummy value
    float r_max_val = -1, g_max_val = -1, b_max_val = -1; // Dummy value

    for (int i = 0; i < 256; i++)
    {
        if (r_cdf[i] >= perc && r_min_val == -1)
        {
            r_min_val = (float)i;
        }

        if (r_cdf[i] >= (1.0 - perc) && r_max_val == -1)
        {
            cout << r_cdf[i] << endl;
            r_max_val = (float)i;
        }

        if (g_cdf[i] >= perc && g_min_val == -1)
        {
            g_min_val = (float)i;
        }

        if (g_cdf[i] >= (1.0 - perc) && g_max_val == -1)
        {
            g_max_val = (float)i;
        }

        if (b_cdf[i] >= perc && b_min_val == -1)
        {
            b_min_val = (float)i;
        }

        if (b_cdf[i] >= (1.0 - perc) && b_max_val == -1)
        {
            b_max_val = (float)i;
        }
    } 


    // Build look up table
    unsigned char r_lut[256], g_lut[256], b_lut[256];
    for (int i = 0; i < 256; i++)
    {   
        float index = (float)i;
        r_lut[i] = saturate_cast<uchar>(255.0 * ((index - r_min_val) / (r_max_val - r_min_val)));
        g_lut[i] = saturate_cast<uchar>(255.0 * ((index - g_min_val) / (g_max_val - g_min_val)));
        b_lut[i] = saturate_cast<uchar>(255.0 * ((index - b_min_val) / (b_max_val - b_min_val)));
    }



    Mat out = img.clone();
    MatIterator_<Vec3b> it, end;
    for (it = out.begin<Vec3b>(), end = out.end<Vec3b>(); it != end; it++)
    {

      (*it)[2] = r_lut[(*it)[2]];
      (*it)[1] = g_lut[(*it)[1]];
      (*it)[0] = b_lut[(*it)[0]];
    }

    
    return out;

}

float mean_pixel(Mat img)
{
    if (img.channels() > 2)
    {
        cvtColor(img.clone(), img, cv::COLOR_BGR2GRAY);
        return mean(img)[0];
    }
    else
    {
        return mean(img)[0];
    }
}

float auto_gamma_value(Mat img)
{
    float max_pixel = 255;
    float middle_pixel = 128;
    float pixel_range = 256;
    float mean_l = mean_pixel(img);

    float gamma = log(middle_pixel/pixel_range)/ log(mean_l/pixel_range); // Formula from ImageJ

    return gamma;

}


Mat gamma_correction(Mat img, float gamma=0)
{
    CV_Assert(img.data);

    // Accept only char type matrices
    CV_Assert(img.depth() != sizeof(uchar));


    if (gamma == 0) {gamma = auto_gamma_value(img);}


    // Build look up table
    unsigned char lut[256];
    for (int i = 0; i < 256; i++)
    {
        lut[i] = saturate_cast<uchar>(pow((float)(i / 255.0), gamma) * 255.0f);
    }

    Mat out = img.clone();
    const int channels = out.channels();
    switch (channels)
    {
    case 1:
    {
              MatIterator_<uchar> it, end;
              for (it = out.begin<uchar>(), end = out.end<uchar>(); it != end; it++)
                  *it = lut[(*it)];

              break;
    }
    case 3:
    {
              MatIterator_<Vec3b> it, end;
              for (it = out.begin<Vec3b>(), end = out.end<Vec3b>(); it != end; it++)
              {

                  (*it)[0] = lut[((*it)[0])];
                  (*it)[1] = lut[((*it)[1])];
                  (*it)[2] = lut[((*it)[2])];
              }

              break;

    }
    }

    return out;
}


/*
    Huang, S. C., Cheng, F. C., & Chiu, Y. S. (2013). 
    Efficient contrast enhancement using adaptive gamma correction with weighting distribution. 
    IEEE Transactions on Image Processing, 22(3), 1032-1041.
*/
Mat adaptive_gamma_correction(Mat img, float alpha=0)
{

    CV_Assert(img.data);

    // Accept only char type matrices
    CV_Assert(img.depth() != sizeof(uchar));

    // Automatically compute the alpha value
    if (alpha == 0) {alpha = auto_gamma_value(img);}
    cout << alpha << endl;
    // Get the image probability density function using the histogram
    Mat gray_img;
    if (img.channels() > 2)
    {
        cvtColor(img.clone(), gray_img, cv::COLOR_BGR2GRAY);
    }
    
    
    // Establish the number of bins
    int histSize = 256;

    // Set the range
    float range[] = { 0, 256 };
    const float* histRange = { range };

    bool uniform = true; bool accumulate = false;

    Mat hist;

    // Compute the histogram
    calcHist(&gray_img, 1, 0, Mat(), hist, 1, &histSize, &histRange, uniform, accumulate);

    Mat norm_hist = hist / gray_img.total();
    double pdf_min = 0;
    double pdf_max = 0;
    minMaxLoc(norm_hist, &pdf_min, &pdf_max);

    std::vector<float> pdf_weights;
    for (int i = 0; i < 256; i++)
    {
        pdf_weights.push_back(pdf_max * (pow(( norm_hist.at<float>(i, 0) - pdf_min) / (pdf_max - pdf_min), alpha)  ));
    }

    std::vector<float> cdf_weights;
    float pdf_weights_total = sum(pdf_weights)[0];
    for (int i = 0; i < 256; i++)
    {
        if (i == 0)
        {
            cdf_weights.push_back((pdf_weights[i]));
        }
        else
        {
            cdf_weights.push_back((pdf_weights[i] + cdf_weights[i-1]) );
        }
        
    }

    // Build look up table
    unsigned char lut[256];
    for (int i = 0; i < 256; i++)
    {   
        float gamma = 1 - cdf_weights[i];
        lut[i] = saturate_cast<uchar>(pow((float)(i / 255.0), gamma) * 255.0f);
    }

    Mat out = img.clone();
    const int num_channels = out.channels();
    switch (num_channels)
    {
    case 1:
    {
              MatIterator_<uchar> it, end;
              for (it = out.begin<uchar>(), end = out.end<uchar>(); it != end; it++)
                  *it = lut[(*it)];

              break;
    }
    case 3:
    {
              MatIterator_<Vec3b> it, end;
              for (it = out.begin<Vec3b>(), end = out.end<Vec3b>(); it != end; it++)
              {

                  (*it)[0] = lut[((*it)[0])];
                  (*it)[1] = lut[((*it)[1])];
                  (*it)[2] = lut[((*it)[2])];
              }

              break;

    }
    }

    return out;

}
cv::Vec3f calculate_global_mean(const cv::Mat& image) {
    cv::Scalar mean_value = cv::mean(image);
    return cv::Vec3f(mean_value[0], mean_value[1], mean_value[2]);
}

cv::Vec3f calculate_local_mean(const cv::Mat& image, int window_size) {
    cv::Mat blurred;
    cv::blur(image, blurred, cv::Size(window_size, window_size));
    cv::Scalar mean_value = cv::mean(blurred);
    return cv::Vec3f(mean_value[0], mean_value[1], mean_value[2]);
}

cv::Mat adaptive_gray_world(const cv::Mat& image, float global_weight, int window_size) {
    cv::Vec3f global_mean = calculate_global_mean(image);
    cv::Vec3f local_mean = calculate_local_mean(image, window_size);

    cv::Mat compensated_image;
    image.convertTo(compensated_image, CV_32FC3);

    compensated_image.forEach<cv::Vec3f>(
        [&global_mean, &local_mean, &global_weight](cv::Vec3f& pixel, const int* position) -> void {
            pixel[0] *= global_weight * global_mean[0] + (1 - global_weight) * local_mean[0] / global_mean[0];
            pixel[1] *= global_weight * global_mean[1] + (1 - global_weight) * local_mean[1] / global_mean[1];
            pixel[2] *= global_weight * global_mean[2] + (1 - global_weight) * local_mean[2] / global_mean[2];
        });

    cv::normalize(compensated_image, compensated_image, 0, 255, cv::NORM_MINMAX);
    compensated_image.convertTo(compensated_image, CV_8UC3);
    return compensated_image;
}

Mat applyCLAHE(const Mat& image, float clipLimit, Size tileGridSize) {
    Mat dst;
    vector<Mat> channels;
    split(image, channels);
    
    Ptr<CLAHE> clahe = createCLAHE(clipLimit, tileGridSize);
    clahe->apply(channels[0], channels[0]);
    clahe->apply(channels[1], channels[1]);
    clahe->apply(channels[2], channels[2]);
    
    merge(channels, dst);
    
    return dst;
}
Mat invertImage(const Mat& image) {
    // Maximum intensity value (usually 255 for 8-bit images)
    int max_intensity = 255;

    // Create a matrix to store the inverted image
    Mat inverted_image = Mat::zeros(image.size(), image.type());

    // Invert each pixel value
    subtract(max_intensity, image, inverted_image);

    return inverted_image;
}

Mat increaseSharpness(const Mat& image, double sigma, double alpha) {
    // Convert the image to grayscale
    Mat gray;
    cvtColor(image, gray, COLOR_BGR2GRAY);

    // Apply Gaussian blur
    Mat blurred;
    GaussianBlur(gray, blurred, Size(0, 0), sigma);

    // Calculate the unsharp mask
    Mat mask = gray - blurred;

    // Add the unsharp mask to the original image with a weighted factor (alpha)
    Mat sharpened;
    addWeighted(gray, 1 + alpha, mask, alpha, 0, sharpened);

    // Normalize the sharpened image to ensure values are within [0, 255]
    normalize(sharpened, sharpened, 0, 255, NORM_MINMAX);

    // Convert the sharpened grayscale image to BGR color space
    Mat sharpened_bgr;
    cvtColor(sharpened, sharpened_bgr, COLOR_GRAY2RGB);

    return sharpened_bgr;
}

int main() {
   
    cv::Mat image = cv::imread("2_jpg.rf.44aa32b801a85f59a77f05f285ce348b.jpg");
    
    // Check if the image is loaded successfully
    if (image.empty()) {
        std::cerr << "Error: Could not read the image file." << std::endl;
        return 1;
    }
    double sigma = 0.1; // Gaussian blur standard deviation
    double alpha = 0.65;

    cv::Mat compensated_image = green_channel_compensation(image);
    cv::Mat final_compensated_image = blue_channel_compensation(compensated_image);
    Mat out, img;
    img = final_compensated_image;
    cv::imshow("finalmid Image", img);
    fastNlMeansDenoisingColored(img, out);
    imwrite("denoised.png", out);

    cout << "White balancing ..." << endl;
    out = white_balance(out.clone());
    imwrite("white_balanced.png", out);

    cout << "Adaptive gamma correction ..." << endl;
    out = adaptive_gamma_correction(out.clone(), 2);


    cout << "Saving ..." << endl;


    cv::Mat adap_gray_image = adaptive_gray_world(out, 0.5, 5); // global weight, local window size
    float clipLimit = 2.0;
    Size tileGridSize(8, 8);

    // Apply CLAHE
    Mat dst = applyCLAHE(adap_gray_image, clipLimit, tileGridSize);
    // // Apply green channel compensation
    // // cv::Mat green_compensated_image = green_channel_compensation(image);
    // double sigma = 0.1; // Gaussian blur standard deviation
    // double alpha = 0.65; // Weighting factor for the unsharp mask

    // Increase sharpness
   // Mat sharpened = increaseSharpness(dst, sigma, alpha);
    //cv::imshow("sharpen Image", sharpened);
   // Mat finale = invertImage(sharpened);
    imwrite("out.png", dst);
    // Display the original and compensated images
    cv::imshow("Original Image", image);
    cv::imshow("Final Compensated Image", dst);
    // cv::imshow("Green Compensated Image", green_compensated_image);
    // Display the original and compensated images
    // cv::imshow("Original Image", image);
    // cv::imshow("Compensated Image", compensated_image);
    cv::waitKey(0);


    return 0;
}