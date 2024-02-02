#include "PolynomialList.h"
#include <iostream>
#include <cmath>
#include <fstream>
#include <cassert>
using namespace std;
const double epsilon = 1.0e-10;

PolynomialList::PolynomialList(const PolynomialList& other) {
    m_Polynomial.clear();
    for (const Term& term : other.m_Polynomial) AddOneTerm(term);
}

PolynomialList::PolynomialList(const string& file) {
    ReadFromFile(file);
}

PolynomialList::PolynomialList(const double* cof, const int* deg, int n) {
    m_Polynomial.clear();
    for (int i = 0; i < n; i++)
        AddOneTerm(Term(deg[i], cof[i]));
    compress();
}

PolynomialList::PolynomialList(const vector<int>& deg, const vector<double>& cof) {
    assert(deg.size() == cof.size());

    m_Polynomial.clear();
    int n = deg.size();
    for (int i = 0; i < n; i++)
        AddOneTerm(Term(deg[i], cof[i]));
    compress();
}

double PolynomialList::coff(int i) const {
    for (const Term& term : m_Polynomial)
    {
        //if (term.deg < i) break;
        if (term.deg == i) return term.cof;
    }
    return 0;
}

double& PolynomialList::coff(int i) {
    return AddOneTerm(Term(i, 0)).cof;
}

void PolynomialList::compress() {
    auto itr = m_Polynomial.begin();
    while (itr != m_Polynomial.end())
    {
        if (fabs(itr->cof) < epsilon)
            itr = m_Polynomial.erase(itr);
        else itr++;
    }
}

PolynomialList PolynomialList::operator+(const PolynomialList& right) const {
    PolynomialList* newPoly = new PolynomialList(*this);
    for (const Term& term : right.m_Polynomial) newPoly->AddOneTerm(term);
    newPoly->compress();
    return *newPoly;
}

PolynomialList PolynomialList::operator-(const PolynomialList& right) const {
    PolynomialList* newPoly = new PolynomialList(*this);
    for (const Term& term : right.m_Polynomial) newPoly->AddOneTerm(Term(term.deg, -term.cof));
    newPoly->compress();
    return *newPoly;
}

PolynomialList PolynomialList::operator*(const PolynomialList& right) const {
    PolynomialList* newPoly = new PolynomialList();
    for (const Term& term1 : m_Polynomial)
    {
        for (const Term& term2 : right.m_Polynomial)
        {
            newPoly->AddOneTerm(Term(term1.deg + term2.deg, term1.cof * term2.cof));
        }
    }
    newPoly->compress();
    return *newPoly; // you should return a correct value
}

PolynomialList& PolynomialList::operator=(const PolynomialList& right) {
    m_Polynomial.clear();
    for (const Term& term : right.m_Polynomial) AddOneTerm(term);
    return *this;
}

void PolynomialList::Print() const {
    bool headSign = false;
    for (const Term& term : m_Polynomial)
    {
        if (fabs(term.cof) < epsilon) continue;
        if (headSign && term.cof > 0) cout << "+";
        cout << term.cof;
        if (term.deg < 0) cout << "x^(" << term.deg << ")";
        if (term.deg > 0) cout << "x^" << term.deg;
        headSign = true;
    }
    cout << endl;
}

bool PolynomialList::ReadFromFile(const string& file) {
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
        AddOneTerm(Term(deg, coff));
    }
    compress();
    fin.close();
    return true;
}

PolynomialList::Term& PolynomialList::AddOneTerm(const Term& term) {
    auto itr = m_Polynomial.begin();
    while (itr != m_Polynomial.end())
    {
        if (itr->deg == term.deg)
        {
            itr->cof += term.cof;
            return *itr;
        }
        if (itr->deg > term.deg) break;
        itr++;
    }
    return *m_Polynomial.insert(itr, term);
}
