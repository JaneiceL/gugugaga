#include "student.hpp"

#include <iostream>

Student::Student()
{
    name = "";
    id = 0;
    score = 0.0;
}

Student::Student(std::string n, int i, double s)
{
    name = n;
    id = i;
    score = s;
}

void Student::setName(std::string n)
{
    name = n;
}

void Student::setId(int i)
{
    id = i;
}

void Student::setScore(double s)
{
    score = s;
}

std::string Student::getName() const
{
    return name;
}

int Student::getId() const
{
    return id;
}

double Student::getScore() const
{
    return score;
}

void Student::print() const
{
    std::cout << "姓名: " << name
              << "  学号: " << id
              << "  成绩: " << score << std::endl;
}
