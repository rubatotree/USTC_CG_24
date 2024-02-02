#include "PolynomialMap.h"

#include <iostream>
#include <fstream>
#include <cassert>
#include <cmath>

using namespace std;
const double epsilon = 1.0e-10;

PolynomialMap::PolynomialMap(const PolynomialMap& other) {
	m_Polynomial.clear();
	for (auto itr : other.m_Polynomial)
		m_Polynomial[itr.first] = itr.second;
}

PolynomialMap::PolynomialMap(const string& file) {
	ReadFromFile(file);
}

PolynomialMap::PolynomialMap(const double* cof, const int* deg, int n) {
	m_Polynomial.clear();
	for (int i = 0; i < n; i++)
		m_Polynomial[deg[i]] = cof[i];
	compress();
}

PolynomialMap::PolynomialMap(const vector<int>& deg, const vector<double>& cof) {
	assert(deg.size() == cof.size());

	m_Polynomial.clear();
	int n = deg.size();
	for (int i = 0; i < n; i++)
		m_Polynomial[deg[i]] = cof[i];
	compress();
}

double PolynomialMap::coff(int i) const {
	for (auto itr : m_Polynomial)
	{
		if (itr.first == i) return itr.second;
	}
	return 0;
}

double& PolynomialMap::coff(int i) {
	return m_Polynomial[i];
}

void PolynomialMap::compress() {
	for (auto itr = m_Polynomial.rbegin(); itr != m_Polynomial.rend(); itr++)
	{
		if (fabs(itr->second) < epsilon) itr->second = 0;
	}
}

PolynomialMap PolynomialMap::operator+(const PolynomialMap& right) const {
	PolynomialMap* newPoly = new PolynomialMap(*this);
	for (auto itr : right.m_Polynomial)
		newPoly->m_Polynomial[itr.first] += itr.second;
	newPoly->compress();
	return *newPoly;
}

PolynomialMap PolynomialMap::operator-(const PolynomialMap& right) const {
	PolynomialMap* newPoly = new PolynomialMap(*this);
	for (auto itr : right.m_Polynomial)
		newPoly->m_Polynomial[itr.first] -= itr.second;
	newPoly->compress();
	return *newPoly;
}

PolynomialMap PolynomialMap::operator*(const PolynomialMap& right) const {
	PolynomialMap* newPoly = new PolynomialMap();
	for (auto itr1 : m_Polynomial)
		for (auto itr2 : right.m_Polynomial)
			newPoly->m_Polynomial[itr1.first + itr2.first] += itr1.second * itr2.second;
	newPoly->compress();
	return *newPoly;
}

PolynomialMap& PolynomialMap::operator=(const PolynomialMap& right) {
	m_Polynomial.clear();
	for (auto itr : right.m_Polynomial)
		m_Polynomial[itr.first] = itr.second;
	return *this;
}

void PolynomialMap::Print() const {
	bool headSign = false;
	for (auto itr : m_Polynomial)
	{
		if (fabs(itr.second) < epsilon) continue;
		if (headSign && itr.second > 0) cout << "+";
		cout << itr.second;
		if (itr.first < 0) cout << "x^(" << itr.first << ")";
		if (itr.first > 0) cout << "x^" << itr.first;
		headSign = true;
	}
	cout << endl;
}

bool PolynomialMap::ReadFromFile(const string& file) {
	ifstream fin;
	fin.open(file);
	if (!fin.is_open()) return false;

	int n;
	char type;
	if (!(fin >> type >> n)) return false;
	if (type != 'P') return false;

	m_Polynomial.clear();
	for (int i = 0; i < n; i++)
	{
		int deg;
		double coff;
		if (!(fin >> deg >> coff)) return false;
		m_Polynomial[deg] = coff;
	}
	compress();
	fin.close();
	return true;
}
