#pragma once

class test
{
	int m_Value;
public:
	test(int v)
	: m_Value(v) {}
	
	void increment();
	int getValue();
};