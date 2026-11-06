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

    // 挑右灯条上一个点，看三个通道各自的值
    cv::Vec3b p = hsv.at<cv::Vec3b>(250, 672);
    std::cout << "H=" << (int)p[0] << " S=" << (int)p[1]
              << " V=" << (int)p[2] << std::endl;
    std::cout << "channels[0]=" << (int)channels[0].at<uchar>(250, 672) << std::endl;

    // 红色在 H 上绕过了 0 点，所以要分两段取，再并起来
    cv::Mat lower, upper, mask;
    cv::inRange(hsv, cv::Scalar(0, 43, 46), cv::Scalar(10, 255, 255), lower);
    cv::inRange(hsv, cv::Scalar(170, 43, 46), cv::Scalar(180, 255, 255), upper);
    cv::bitwise_or(lower, upper, mask);

    std::cout << "红色像素: " << cv::countNonZero(mask) << std::endl;

    // 找轮廓
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(mask, contours, hierarchy,
                     cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    std::cout << "轮廓个数: " << contours.size() << std::endl;

    // 按面积筛，太小的是反光和噪点
    cv::Mat boxed = img.clone();
    int kept = 0;
    for (size_t i = 0; i < contours.size(); i++)
    {
        double area = cv::contourArea(contours[i]);
        if (area < 500)
        {
            continue;
        }
        kept++;
        cv::Rect r = cv::boundingRect(contours[i]);
        std::cout << "灯条 " << kept << ": 面积=" << area
                  << "  矩形=[" << r.x << " " << r.y << " "
                  << r.width << " " << r.height << "]" << std::endl;
        cv::rectangle(boxed, r, cv::Scalar(0, 255, 0), 2);
    }
    std::cout << "筛完剩: " << kept << " 个" << std::endl;
    cv::imwrite("kept.png", boxed);

    // 换成最小外接矩形，能跟着灯条的倾斜转
    for (size_t i = 0; i < contours.size(); i++)
    {
        if (cv::contourArea(contours[i]) < 500)
        {
            continue;
        }
        cv::RotatedRect rr = cv::minAreaRect(contours[i]);
        cv::Mat pts;
        cv::boxPoints(rr, pts);

        std::cout << "中心=(" << rr.center.x << ", " << rr.center.y << ")"
                  << "  宽高=" << rr.size.width << " x " << rr.size.height
                  << "  角度=" << rr.angle << std::endl;
        for (int k = 0; k < pts.rows; k++)
        {
            std::cout << "  角点" << k << " = ("
                      << pts.at<float>(k, 0) << ", " << pts.at<float>(k, 1)
                      << ")" << std::endl;
        }
    }

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
