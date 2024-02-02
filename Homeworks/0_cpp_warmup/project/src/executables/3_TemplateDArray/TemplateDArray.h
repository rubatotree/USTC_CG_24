#pragma once
#include <iostream>
#include <cassert>

// interfaces of Dynamic Array class DArray
template<class T>
class DArray {
public:
	DArray(); // default constructor
	DArray(int nSize, T dValue = static_cast<T>(0)); // set an array with default values
	DArray(const DArray& arr); // copy constructor
	~DArray(); // deconstructor

	void Print() const; // print the elements of the array

	int GetSize() const; // get the size of the array
	void SetSize(int nSize); // set the size of the array

	const T& GetAt(int nIndex) const; // get an element at an index
	void SetAt(int nIndex, T dValue); // set the value of an element

	T& operator[](int nIndex); // overload operator '[]'
	const T& operator[](int nIndex) const; // overload operator '[]'

	void PushBack(T dValue); // add a new element at the end of the array
	void DeleteAt(int nIndex); // delete an element at some index
	void InsertAt(int nIndex, T dValue); // insert a new element at some index

	DArray& operator = (const DArray& arr); //overload operator '='

private:
	T* m_pData; // the pointer to the array memory
	int m_nSize; // the size of the array
	int m_nMax;

private:
	void Init(); // initilize the array
	void Free(); // free the array
	void Reserve(int nSize); // allocate enough memory
};


// default constructor
template<typename T>
DArray<T>::DArray() {
	Init();
}

// set an array with default values
template<typename T>
DArray<T>::DArray(int nSize, T dValue) {
	Init();
	if (nSize <= 0) return;
	T* tempData = new T[nSize];
	if (tempData == nullptr)
	{
		std::cout << "错误：内存不足。" << std::endl;
		return;
	}

	if (m_nMax == 0) m_nMax = 1;
	while (m_nMax < nSize) m_nMax *= 2;

	m_nSize = nSize;
	m_pData = tempData;
	for (int i = 0; i < m_nSize; i++)
	{
		m_pData[i] = dValue;
	}
}

template<typename T>
DArray<T>::DArray(const DArray& arr) {
	Init();

	//一点矛盾：先delete原数据会空出内存，此时可能能腾出内存存储新的数据，但若不能存储新数据，则会造成数据损失。
	//如果先申请新内存，可能会导致内存利用不足。

	T* tempData = new T[arr.m_nMax];
	if (tempData == nullptr)
	{
		std::cout << "错误：内存不足。" << std::endl;
		return;
	}

	m_nMax = arr.m_nMax;
	m_nSize = arr.m_nSize;
	m_pData = tempData;

	for (int i = 0; i < m_nMax; i++)
	{
		m_pData[i] = arr.m_pData[i];
	}
}

// deconstructor
template<typename T>
DArray<T>::~DArray() {
	Free();
}

// display the elements of the array
template<typename T>
void DArray<T>::Print() const {
	std::cout << "DArray[" << m_nSize << "]: ";
	for (int i = 0; i < m_nSize; i++)
	{
		if (i != 0) std::cout << ", ";
		std::cout << GetAt(i);
	}
	std::cout << std::endl;
}

// initilize the array
template<typename T>
void DArray<T>::Init() {
	m_nSize = 0;
	m_nMax = 0;
	m_pData = nullptr;
}

// free the array
template<typename T>
void DArray<T>::Free() {
	delete[] m_pData;
	m_pData = nullptr;
	m_nSize = 0;
	m_nMax = 0;
}

// get the size of the array
template<typename T>
int DArray<T>::GetSize() const {
	return m_nSize;
}

// set the size of the array
template<typename T>
void DArray<T>::SetSize(int nSize) {
	if (m_nSize == nSize) return;
	if (nSize < 0)
	{
		std::cout << "错误：不得将数组长度设置为负数。" << std::endl;
		return;
	}
	if (nSize < m_nSize)
	{
		for (int i = nSize; i < m_nSize; i++)
		{
			m_pData[i] = 0;
		}
	}
	else if (nSize > m_nMax)
	{
		if (m_nMax == 0) m_nMax = 1;
		while (m_nMax < nSize) m_nMax *= 2;
		T* new_data = new T[m_nMax];
		if (new_data == nullptr)
		{
			std::cout << "错误：内存不足。" << std::endl;
			return;
		}

		int minSize = nSize < m_nSize ? nSize : m_nSize;
		for (int i = 0; i < minSize; i++)
		{
			new_data[i] = m_pData[i];
		}
		for (int i = minSize; i < m_nMax; i++)
		{
			//此处是一个未定义的行为。将Size扩大后不确定填充的默认值是多少，此处设置为0。
			new_data[i] = 0;
		}
		delete[] m_pData;
		m_pData = new_data;
	}
	m_nSize = nSize;
}

// get an element at an index
template<typename T>
const T& DArray<T>::GetAt(int nIndex) const {
	assert(nIndex >= 0 && nIndex < m_nSize);
	return *(m_pData + nIndex);
}

// set the value of an element 
template<typename T>
void DArray<T>::SetAt(int nIndex, T dValue) {
	if (nIndex < 0 || nIndex >= m_nSize)
	{
		std::cout << "错误：数组下标越界。" << std::endl;
		return;
	}
	m_pData[nIndex] = dValue;
}

// overload operator '[]'
template<typename T>
const T& DArray<T>::operator[](int nIndex) const {
	return GetAt(nIndex);
}

// add a new element at the end of the array
template<typename T>
void DArray<T>::PushBack(T dValue) {
	SetSize(m_nSize + 1);
	m_pData[m_nSize - 1] = dValue;
}

// delete an element at some index
template<typename T>
void DArray<T>::DeleteAt(int nIndex) {
	if (nIndex < 0 || nIndex >= m_nSize)
	{
		std::cout << "错误：数组下标越界。" << std::endl;
		return;
	}
	for (int i = nIndex; i < m_nSize - 1; i++)
	{
		m_pData[i] = m_pData[i + 1];
	}
	SetSize(m_nSize - 1);
}

// insert a new element at some index
template<typename T>
void DArray<T>::InsertAt(int nIndex, T dValue) {
	if (nIndex < 0 || nIndex >= m_nSize + 1)
	{
		std::cout << "错误：数组下标越界。" << std::endl;
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
template<typename T>
DArray<T>& DArray<T>::operator = (const DArray& arr) {
	T* tempData = new T[arr.m_nMax];
	assert(tempData != nullptr);

	delete[] m_pData;
	m_nMax = arr.m_nMax;
	m_nSize = arr.m_nSize;
	m_pData = tempData;

	for (int i = 0; i < m_nSize; i++)
	{
		m_pData[i] = arr.m_pData[i];
	}
	return *this;
}

