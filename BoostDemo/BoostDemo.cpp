// BoostDemo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include <boost/lambda/lambda.hpp>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <string>

#include "TestJson.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <boost/regex.hpp>

using namespace std;

void TestLambda();
void TestXML();
void TestINI();


int main()
{
	boost::regex re("");

	bool bValue = true;

	scanf("%d", &bValue);

	printf("%d\n", bValue);


	TestJson();

	TestXML();

	TestINI();

	system("pause");
}

void TestLambda()
{
	using namespace boost::lambda;
	typedef std::istream_iterator<int> in;

	std::for_each(
		in(std::cin), in(), std::cout << (boost::lambda::_1 * 3) << " ");
}

void TestXML()
{
	boost::property_tree::ptree ptData;
	boost::property_tree::xml_parser::read_xml("center1.xml", ptData);

	cout << ptData.get<string>("Config.<xmlattr>.base") << endl;

	cout << ptData.get<string>("Config.CoreConfig.Net.InternalAddress") << endl;
}

void TestINI()
{
	boost::property_tree::ptree ptData;
	boost::property_tree::ini_parser::read_ini("CreatureInfo.ini", ptData);

	cout << ptData.get<string>("10101.describe") << endl;
}