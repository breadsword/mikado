#ifndef LOG_H
#define LOG_H

#include <iostream>
// #define LOG std::cout << __FILE__ << "(" << __LINE__ << ")" << ": "
// #define LOG std::cout << __FILE_NAME__ << "(" << __LINE__ << ")" << ": "
#define LOG std::cout << __FUNCTION__ << "(" << __LINE__ << ")" << ": "
using std::endl;


#endif // LOG_H
