# include "stdafx.h"

#include <boost/lambda/lambda.hpp>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <boost/property_tree/xml_parser.hpp>

#include <boost/property_tree/ini_parser.hpp>

using namespace std;

void TestJson()
{
	stringstream ssData;
	ssData << "{\"startMember\":[\"57d90d043b2e6a9a0156a1f0\"], \"acceptMember\" : [\"581b1b975e3c7ada391b2aa1\"], \"password\" : \"123456\", \"startTime\" : \"1702170000\", \"deadline\" : \"1702180000\", \"_id\" : \"58a547ed9d54fb2471c7f84e\"}";

	boost::property_tree::ptree ptData;
	boost::property_tree::json_parser::read_json(ssData, ptData);

	cout << "data: " << endl;
	for (auto it = ptData.begin(); it != ptData.end(); ++it)
	{
		cout << it->first << ": " << it->second.get_value<string>() << endl;
	}
	cout << endl;

	boost::property_tree::ptree ptStartMember = ptData.get_child("startMember");
	cout << "startMember: ";
	for (auto it = ptStartMember.begin(); it != ptStartMember.end(); ++it)
	{
		cout << it->second.get_value<string>() << " ";
	}
	cout << endl;

	boost::property_tree::ptree ptAcceptMember = ptData.get_child("acceptMember");
	cout << "acceptMember: ";
	for (auto it = ptAcceptMember.begin(); it != ptAcceptMember.end(); ++it)
	{
		cout << it->second.get_value<string>() << " ";
	}
	cout << endl;

	cout << "password: " << ptData.get<string>("password") << endl;
}
