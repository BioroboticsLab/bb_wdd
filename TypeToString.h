/*
 * TypeToString.h
 *
 *  Created on: 15.04.2014
 *      Author: sam
 */
#include "stdafx.h"

#ifndef TYPETOSTRING_H_
#define TYPETOSTRING_H_


namespace wdd
{

class TypeToString
{
public:
    TypeToString();
    ~TypeToString();
    static std::string typeToString(int type);
    static void printType(int type);
    static void printType(cv::Mat mat);
};

} /* namespace WaggleDanceDetector */

#endif /* TYPETOSTRING_H_ */
