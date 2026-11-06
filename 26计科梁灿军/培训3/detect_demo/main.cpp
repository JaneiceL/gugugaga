#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#include <opencv2/opencv.hpp>

// 一根灯条的信息
struct Bar
{
    cv::RotatedRect rect;
    cv::Point2f pts[4];   // 四个角点
    float tilt;           // 长边偏离竖直的角度
    float length;         // 长边
    float width;          // 短边
};

// 从掩膜里挑出所有像灯条的轮廓
void findBars(const cv::Mat& mask, std::vector<Bar>& bars)
{
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(mask, contours, hierarchy,
                     cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    bars.clear();

    for (size_t i = 0; i < contours.size(); i++)
    {
        if (cv::contourArea(contours[i]) < 500)
        {
            continue;
        }

        Bar b;
        b.rect = cv::minAreaRect(contours[i]);

        // minAreaRect 不保证哪条边是长边，先统一，再算倾角
        if (b.rect.size.width > b.rect.size.height)
        {
            b.length = b.rect.size.width;
            b.width = b.rect.size.height;
            b.tilt = 90.0f - b.rect.angle;
        }
        else
        {
            b.length = b.rect.size.height;
            b.width = b.rect.size.width;
            b.tilt = b.rect.angle;
        }

        // 179 度和 -1 度其实是同一根竖直的灯条，先折回同一圈
        while (b.tilt >= 90.0f)
        {
            b.tilt -= 180.0f;
        }
        while (b.tilt < -90.0f)
        {
            b.tilt += 180.0f;
        }

        // 灯条是细长的，太胖的不要
        if (b.length < 2.0f * b.width)
        {
            continue;
        }

        cv::Mat pts;
        cv::boxPoints(b.rect, pts);
        for (int k = 0; k < 4; k++)
        {
            b.pts[k] = cv::Point2f(pts.at<float>(k, 0), pts.at<float>(k, 1));
        }

        bars.push_back(b);
    }
}

// 从一堆灯条里挑最像一对的两根
bool pickPair(const std::vector<Bar>& bars, size_t& ia, size_t& ib)
{
    float best = 1e9f;
    bool found = false;

    for (size_t i = 0; i < bars.size(); i++)
    {
        for (size_t j = i + 1; j < bars.size(); j++)
        {
            float dAngle = std::fabs(bars[i].tilt - bars[j].tilt);
            float lenMax = std::max(bars[i].length, bars[j].length);
            float dLen = std::fabs(bars[i].length - bars[j].length) / lenMax;

            float dx = bars[i].rect.center.x - bars[j].rect.center.x;
            float dy = bars[i].rect.center.y - bars[j].rect.center.y;
            float dist = std::sqrt(dx * dx + dy * dy);

            // 不平行、长短差太多、离得太近或太远，都不像一对
            // 15 度不是拍的：相机斜着看同一块板，两根灯条在图上本来就差十几度
            if (dAngle > 15.0f) continue;
            if (dLen > 0.4f) continue;
            if (dist < 0.8f * lenMax) continue;
            if (dist > 4.0f * lenMax) continue;

            float score = dAngle + dLen * 20.0f;
            if (score < best)
            {
                best = score;
                ia = i;
                ib = j;
                found = true;
            }
        }
    }

    return found;
}

// 取灯条上朝左或朝右的那条长边，把两个端点带出去
// takeLeft=true 取靠左的长边，false 取靠右的长边
bool longEdge(const Bar& b, bool takeLeft, cv::Point2f& p1, cv::Point2f& p2)
{
    bool has = false;
    float bestMid = 0.0f;

    for (int k = 0; k < 4; k++)
    {
        const cv::Point2f& a = b.pts[k];
        const cv::Point2f& c = b.pts[(k + 1) % 4];

        float dx = a.x - c.x;
        float dy = a.y - c.y;
        float len = std::sqrt(dx * dx + dy * dy);

        // 只要长边。短边长度就是灯条宽度，差得远
        if (len < b.length * 0.8f)
        {
            continue;
        }

        float mid = (a.x + c.x) * 0.5f;
        if (!has || (takeLeft ? mid < bestMid : mid > bestMid))
        {
            has = true;
            bestMid = mid;
            p1 = a;
            p2 = c;
        }
    }

    return has;
}

// 把灯条和装甲板画出来
void drawResult(cv::Mat& canvas, const Bar& l, const Bar& r)
{
    for (int k = 0; k < 4; k++)
    {
        cv::line(canvas, l.pts[k], l.pts[(k + 1) % 4], cv::Scalar(0, 255, 0), 2);
        cv::line(canvas, r.pts[k], r.pts[(k + 1) % 4], cv::Scalar(0, 255, 0), 2);
    }

    for (int k = 0; k < 4; k++)
    {
        cv::circle(canvas, l.pts[k], 4, cv::Scalar(0, 0, 255), -1);
        cv::circle(canvas, r.pts[k], 4, cv::Scalar(0, 0, 255), -1);
    }

    // 装甲板的四个角：左灯条最左的长边 + 右灯条最右的长边
    // 不能按 x 排序取点：灯条一歪，x 最小的两个点会落在短边上
    cv::Point2f a1, a2, b1, b2;
    if (!longEdge(l, true, a1, a2) || !longEdge(r, false, b1, b2))
    {
        return;
    }

    std::vector<cv::Point2f> quad = {a1, a2, b1, b2};

    // 按极角排序，才不会连成交叉的八字
    cv::Point2f c(0.0f, 0.0f);
    for (size_t i = 0; i < quad.size(); i++)
    {
        c += quad[i];
    }
    c *= 0.25f;
    std::sort(quad.begin(), quad.end(),
              [&c](const cv::Point2f& a, const cv::Point2f& b) {
                  return std::atan2(a.y - c.y, a.x - c.x) < std::atan2(b.y - c.y, b.x - c.x);
              });

    for (int k = 0; k < 4; k++)
    {
        cv::line(canvas, quad[k], quad[(k + 1) % 4], cv::Scalar(0, 255, 255), 2);
    }
}

// 处理一帧，返回画好的图；found 告诉你这一帧到底有没有配上
cv::Mat detect(const cv::Mat& frame, bool& found)
{
    cv::Mat hsv, m1, m2, mask;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, cv::Scalar(0, 43, 46), cv::Scalar(10, 255, 255), m1);
    cv::inRange(hsv, cv::Scalar(170, 43, 46), cv::Scalar(180, 255, 255), m2);
    cv::bitwise_or(m1, m2, mask);

    cv::Mat canvas = frame.clone();

    std::vector<Bar> bars;
    findBars(mask, bars);

    size_t ia = 0, ib = 0;
    found = pickPair(bars, ia, ib);
    if (!found)
    {
        return canvas;
    }

    const Bar& a = bars[ia];
    const Bar& b = bars[ib];
    if (a.rect.center.x < b.rect.center.x)
    {
        drawResult(canvas, a, b);
    }
    else
    {
        drawResult(canvas, b, a);
    }

    return canvas;
}

int main(int argc, char** argv)
{
    std::string src = "armor.png";
    if (argc > 1)
    {
        src = argv[1];
    }

    // 能当图片读就当图片，读不了再当视频
    cv::Mat img = cv::imread(src);
    if (!img.empty())
    {
        bool found = false;
        cv::Mat result = detect(img, found);
        cv::imwrite("result.png", result);
        std::cout << "已保存 result.png，检出: " << (found ? "是" : "否") << std::endl;

        cv::imshow("result", result);
        cv::waitKey(0);
        cv::destroyAllWindows();
        return 0;
    }

    cv::VideoCapture cap(src);
    if (!cap.isOpened())
    {
        std::cout << "打不开: " << src << std::endl;
        return -1;
    }

    std::cout << "按 ESC 退出" << std::endl;

    // 按原视频的帧率放。waitKey(1) 只等 1 毫秒，570 帧几秒就冲完了
    double fps = cap.get(cv::CAP_PROP_FPS);
    if (fps < 1.0)
    {
        fps = 30.0;
    }
    int delay = (int)(1000.0 / fps);

    bool esc = false;
    int frames = 0;
    int hits = 0;
    while (true)
    {
        cv::Mat frame;
        if (!cap.read(frame))
        {
            break;
        }

        bool found = false;
        cv::Mat result = detect(frame, found);
        cv::imshow("detect", result);

        frames++;
        if (found)
        {
            hits++;
        }

        if (cv::waitKey(delay) == 27)
        {
            esc = true;
            break;
        }
    }

    std::cout << "处理了 " << frames << " 帧，配对成功 " << hits << " 帧" << std::endl;

    // 播完停住，别一闪就关。按 ESC 主动退出的就不用再等了
    if (!esc)
    {
        std::cout << "按任意键关闭窗口" << std::endl;
        cv::waitKey(0);
    }

    return 0;
}
