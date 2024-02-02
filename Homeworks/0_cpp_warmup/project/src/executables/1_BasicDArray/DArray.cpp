// implementation of class DArray
#include "DArray.h"
#include <iostream>
#include <cassert>

// default constructor
DArray::DArray() {
	Init();
}

// set an array with default values
DArray::DArray(int nSize, double dValue) {
	Init();

	double* tempData = new double[nSize];
	if (tempData == nullptr)
	{
		std::cout << "�����ڴ治�㡣" << std::endl;
		return;
	}

	m_nSize = nSize;
	m_pData = tempData;
	for (int i = 0; i < m_nSize; i++)
	{
		m_pData[i] = dValue;
	}
}

DArray::DArray(const DArray& arr) {
	Init();

	//һ��ì�ܣ���deleteԭ���ݻ�ճ��ڴ棬��ʱ�������ڳ��ڴ�洢�µ����ݣ��������ܴ洢�����ݣ�������������ʧ��
	//������������ڴ棬���ܻᵼ���ڴ����ò��㡣

	double* tempData = new double[arr.m_nSize];
	if (tempData == nullptr)
	{
		std::cout << "�����ڴ治�㡣" << std::endl;
		return;
	}

	m_nSize = arr.m_nSize;
	m_pData = tempData;

	for (int i = 0; i < arr.m_nSize; i++)
	{
		m_pData[i] = arr.m_pData[i];
	}
}

// deconstructor
DArray::~DArray() {
	Free();
}

// display the elements of the array
void DArray::Print() const {
	std::cout << "double[" << m_nSize << "]: ";
	for (int i = 0; i < m_nSize; i++)
	{
		if (i != 0) std::cout << ", ";
		std::cout << GetAt(i);
	}
	std::cout << std::endl;
}

// initilize the array
void DArray::Init() {
	m_nSize = 0;
	m_pData = nullptr;
}

// free the array
void DArray::Free() {
	delete[] m_pData;
	m_pData = nullptr;
	m_nSize = 0;
}

// get the size of the array
int DArray::GetSize() const {
	return m_nSize;
}

// set the size of the array
void DArray::SetSize(int nSize) {
	if (m_nSize == nSize) return;
	if (nSize < 0)
	{
		std::cout << "���󣺲��ý����鳤������Ϊ������" << std::endl;
		return;
	}
	double* new_data = new double[nSize];
	if (new_data == nullptr)
	{
		std::cout << "�����ڴ治�㡣" << std::endl;
		return;
	}

	int minSize = nSize < m_nSize ? nSize : m_nSize;
	for (int i = 0; i < minSize; i++)
	{
		new_data[i] = m_pData[i];
	}
	for (int i = minSize; i < nSize; i++)
	{
		//�˴���һ��δ�������Ϊ����Size�����ȷ������Ĭ��ֵ�Ƕ��٣��˴�����Ϊ0��
		new_data[i] = 0.;
	}
	delete[] m_pData;
	m_nSize = nSize;
	m_pData = new_data;
}

// get an element at an index
const double& DArray::GetAt(int nIndex) const {
	assert(nIndex >= 0 && nIndex < m_nSize);
	return *(m_pData + nIndex);
}

// set the value of an element 
void DArray::SetAt(int nIndex, double dValue) {
	if (nIndex < 0 || nIndex >= m_nSize)
	{
		std::cout << "���������±�Խ�硣" << std::endl;
		return;
	}
	m_pData[nIndex] = dValue;
}

// overload operator '[]'
const double& DArray::operator[](int nIndex) const {
	return GetAt(nIndex);
}

// add a new element at the end of the array
void DArray::PushBack(double dValue) {
	SetSize(m_nSize + 1);
	m_pData[m_nSize - 1] = dValue;
}

// delete an element at some index
void DArray::DeleteAt(int nIndex) {
	if (nIndex < 0 || nIndex >= m_nSize)
	{
		std::cout << "���������±�Խ�硣" << std::endl;
		return;
	}
	for (int i = nIndex; i < m_nSize - 1; i++)
	{
		m_pData[i] = m_pData[i + 1];
	}
	SetSize(m_nSize - 1);
}

// insert a new element at some index
void DArray::InsertAt(int nIndex, double dValue) {
	if (nIndex < 0 || nIndex >= m_nSize + 1)
	{
		std::cout << "���������±�Խ�硣" << std::endl;
		return;
	}
	SetSize(m_nSize + 1);
	for (int i = m_nSize - 2; i >= nIndex; i--)
	{
		m_pData[i + 1] = m_pData[i];
	}
	m_pData[nIndex] = dValue;
}

// overload operator '='
DArray& DArray::operator = (const DArray& arr) {
	double* tempData = new double[arr.m_nSize];
	assert(tempData != nullptr);

	delete[] m_pData;
	m_nSize = arr.m_nSize;
	m_pData = tempData;

	for (int i = 0; i < m_nSize; i++)
	{
		m_pData[i] = arr.m_pData[i];
	}
	return *this;
}
