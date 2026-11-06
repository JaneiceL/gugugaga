#include <iostream>

#include <opencv2/opencv.hpp>

int main(int argc, char** argv)
{
    // 没给参数就读 armor.png
    std::string path = "armor.png";
    if (argc > 1)
    {
        path = argv[1];
    }

    cv::Mat img = cv::imread(path);
    if (img.empty())
    {
        std::cout << "图片读取失败: " << path << std::endl;
        return -1;
    }

    std::cout << "宽: " << img.cols << "  高: " << img.rows << std::endl;

    cv::Mat hsv;
    cv::cvtColor(img, hsv, cv::COLOR_BGR2HSV);

    // 拆成 H、S、V 三张单通道图
    std::vector<cv::Mat> channels;
    cv::split(hsv, channels);

    // 红色在 H 上绕过了 0 点，所以要分两段取，再并起来
    cv::Mat lower, upper, mask;
    cv::inRange(hsv, cv::Scalar(0, 43, 46), cv::Scalar(10, 255, 255), lower);
    cv::inRange(hsv, cv::Scalar(170, 43, 46), cv::Scalar(180, 255, 255), upper);
    cv::bitwise_or(lower, upper, mask);

    std::cout << "红色像素: " << cv::countNonZero(mask) << std::endl;

    // 原图太大，缩一半摆成 2x2，一屏能看全
    std::vector<cv::Mat> views = {channels[0], channels[1], channels[2], mask};
    std::vector<std::string> names = {"H", "S", "V", "red_mask"};

    for (size_t i = 0; i < views.size(); i++)
    {
        cv::Mat small;
        cv::resize(views[i], small, cv::Size(), 0.6, 0.6);
        cv::imshow(names[i], small);
        cv::moveWindow(names[i], (int)(i % 2) * 640, (int)(i / 2) * 320);
    }

    cv::imwrite("red_mask.png", mask);
    std::cout << "已保存 red_mask.png" << std::endl;

    cv::waitKey(0);
    cv::destroyAllWindows();

    return 0;
}
