#ifndef STUDENT_HPP
#define STUDENT_HPP

#include <string>

// 学生信息：姓名、学号、成绩
class Student
{
private:
    std::string name;
    int id;
    double score;

public:
    Student();
    Student(std::string n, int i, double s);

    void setName(std::string n);
    void setId(int i);
    void setScore(double s);   // 改成绩

    std::string getName() const;
    int getId() const;
    double getScore() const;

    void print() const;        // 打印全部信息
};

#endif
